#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <android/dlext.h>
#include <dlfcn.h>
#include <sys/system_properties.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <string>

#include <lsplt.hpp>

#include <base.hpp>

#include "zygisk.hpp"
#include "module.hpp"

using namespace std;

// ---------------------------------------------------------------------------
// Root-detection evasion for DenyList target processes
//
// Applied in app_specialize_pre() immediately after fork, before the app's
// own code runs.  All hooks operate only inside the target process — Zygote
// itself and non-DenyList apps are unaffected.
//
// Three layers of defence:
//  1. Property spoofing  — __system_property_get hook hides root/debug props
//                          and returns a stock verified-boot fingerprint so
//                          Play Integrity / SafetyNet verdicts pass.
//  2. File-access hiding — open/openat/access/faccessat/stat/lstat/fstatat
//                          return ENOENT for su binaries, Magisk paths, and
//                          any path containing "magisk" or root-manager names.
//  3. /proc filtering    — When the app opens /proc/self/maps or
//                          /proc/net/unix the returned fd points to an
//                          in-memory copy with Magisk/Zygisk lines stripped,
//                          so memory-map and socket scans come up clean.
// ---------------------------------------------------------------------------

// ---- 1. Property spoofing -------------------------------------------------

struct SpoofEntry {
    const char *key;
    const char *value;
};

// Pixel 6 (oriole) Android 13 TQ3A stock fingerprint — widely accepted by
// Google Play Integrity and common game anti-cheat integrity checks.
#define STOCK_FINGERPRINT \
    "google/oriole/oriole:13/TQ3A.230901.001/10750268:user/release-keys"

static const SpoofEntry SPOOF_PROPS[] = {
    // Core build identity
    {"ro.build.tags",                   "release-keys"},
    {"ro.build.type",                   "user"},
    {"ro.product.build.tags",           "release-keys"},
    {"ro.debuggable",                   "0"},
    {"ro.secure",                       "1"},
    {"ro.adb.secure",                   "1"},
    // Verified-boot / bootloader state — checked by Play Integrity and games
    {"ro.boot.verifiedbootstate",       "green"},
    {"ro.boot.flash.locked",            "1"},
    {"ro.boot.vbmeta.device_state",     "locked"},
    {"ro.boot.veritymode",              "enforcing"},
    {"ro.boot.warranty_bit",            "0"},
    {"ro.warranty_bit",                 "0"},
    // Build fingerprint — Play Integrity cross-checks against Google's
    // certified device list; all three partitions should agree.
    {"ro.build.fingerprint",            STOCK_FINGERPRINT},
    {"ro.vendor.build.fingerprint",     STOCK_FINGERPRINT},
    {"ro.system.build.fingerprint",     STOCK_FINGERPRINT},
    {"ro.product.build.fingerprint",    STOCK_FINGERPRINT},
};

using prop_get_fn_t = int (*)(const char *, char *);
static prop_get_fn_t old_property_get = nullptr;

static int new_property_get(const char *key, char *value) {
    if (key && value) {
        for (const auto &e : SPOOF_PROPS) {
            if (strcmp(key, e.key) == 0) {
                strlcpy(value, e.value, PROP_VALUE_MAX);
                return static_cast<int>(strlen(e.value));
            }
        }
    }
    return old_property_get(key, value);
}

// ---- 2. File-access hiding ------------------------------------------------

// Any path whose string contains one of these fragments will be made
// invisible to the target process (ENOENT returned).
static const char *HIDDEN_PATH_FRAGS[] = {
    "magisk",
    "zygisk",
    "/sbin/su",
    "/sbin/.su",
    "/system/xbin/su",
    "/system/bin/su",
    "/system/bin/.ext/.su",
    "/system/usr/we-need-root",
    "/system/app/Superuser",
    "/system/app/SuperSU",
    "/system/app/KingUser",
    "/system/app/Kinguser",
    "supersu",
    "SuperSU",
    ".superuser",
    nullptr,
};

static bool path_is_hidden(const char *path) {
    if (!path) return false;
    for (int i = 0; HIDDEN_PATH_FRAGS[i]; ++i) {
        if (strstr(path, HIDDEN_PATH_FRAGS[i])) return true;
    }
    return false;
}

// ---- 3. /proc filtering ---------------------------------------------------

static bool is_proc_maps_path(const char *path) {
    if (!path) return false;
    if (strcmp(path, "/proc/self/maps") == 0) return true;
    // Match /proc/<digits>/maps
    if (strncmp(path, "/proc/", 6) != 0) return false;
    const char *p = path + 6;
    while (*p >= '0' && *p <= '9') ++p;
    return strcmp(p, "/maps") == 0;
}

static bool is_proc_net_unix(const char *path) {
    return path && strcmp(path, "/proc/net/unix") == 0;
}

// Read fd, filter lines for which keep_fn returns false, write result to a
// new memfd, close the original fd, and return the memfd.
static int make_filtered_fd(int orig_fd,
                            bool (*drop_line)(const char *line, size_t len)) {
    // Read original content
    std::string content;
    char buf[4096];
    ssize_t n;
    while ((n = ::read(orig_fd, buf, sizeof(buf))) > 0)
        content.append(buf, static_cast<size_t>(n));
    ::close(orig_fd);

    // Filter line by line
    std::string out;
    out.reserve(content.size());
    size_t pos = 0;
    while (pos < content.size()) {
        size_t nl = content.find('\n', pos);
        size_t end = (nl == std::string::npos) ? content.size() : nl + 1;
        const char *line = content.c_str() + pos;
        size_t line_len = end - pos;
        if (!drop_line(line, line_len))
            out.append(line, line_len);
        pos = end;
    }

    // Write to anonymous memfd
#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif
    int mfd = (int)syscall(__NR_memfd_create, "proc_view", MFD_CLOEXEC);
    if (mfd < 0) return -1;  // graceful: caller sees a closed fd
    ::write(mfd, out.c_str(), out.size());
    ::lseek(mfd, 0, SEEK_SET);
    return mfd;
}

static bool maps_drop_line(const char *line, size_t) {
    return strstr(line, "magisk") || strstr(line, "zygisk");
}

static bool unix_drop_line(const char *line, size_t) {
    return strstr(line, "magisk") != nullptr;
}

// ---- Hook implementations -------------------------------------------------

using open_fn_t    = int (*)(const char *, int, mode_t);
using openat_fn_t  = int (*)(int, const char *, int, mode_t);
using access_fn_t  = int (*)(const char *, int);
using faccess_fn_t = int (*)(int, const char *, int, int);
using stat_fn_t    = int (*)(const char *, struct stat *);
using fstatat_fn_t = int (*)(int, const char *, struct stat *, int);

static open_fn_t    old_open    = nullptr;
static openat_fn_t  old_openat  = nullptr;
static access_fn_t  old_access  = nullptr;
static faccess_fn_t old_faccessat = nullptr;
static stat_fn_t    old_stat    = nullptr;
static stat_fn_t    old_lstat   = nullptr;
static fstatat_fn_t old_fstatat = nullptr;

static int new_open(const char *path, int flags, mode_t mode) {
    if (path_is_hidden(path)) { errno = ENOENT; return -1; }
    int fd = old_open(path, flags, mode);
    if (fd >= 0 && (flags & O_ACCMODE) == O_RDONLY) {
        if (is_proc_maps_path(path)) return make_filtered_fd(fd, maps_drop_line);
        if (is_proc_net_unix(path))  return make_filtered_fd(fd, unix_drop_line);
    }
    return fd;
}

static int new_openat(int dirfd, const char *path, int flags, mode_t mode) {
    if (path_is_hidden(path)) { errno = ENOENT; return -1; }
    int fd = old_openat(dirfd, path, flags, mode);
    if (fd >= 0 && (flags & O_ACCMODE) == O_RDONLY) {
        if (is_proc_maps_path(path)) return make_filtered_fd(fd, maps_drop_line);
        if (is_proc_net_unix(path))  return make_filtered_fd(fd, unix_drop_line);
    }
    return fd;
}

static int new_access(const char *path, int mode) {
    if (path_is_hidden(path)) { errno = ENOENT; return -1; }
    return old_access(path, mode);
}

static int new_faccessat(int dirfd, const char *path, int mode, int flags) {
    if (path_is_hidden(path)) { errno = ENOENT; return -1; }
    return old_faccessat(dirfd, path, mode, flags);
}

static int new_stat(const char *path, struct stat *buf) {
    if (path_is_hidden(path)) { errno = ENOENT; return -1; }
    return old_stat(path, buf);
}

static int new_lstat(const char *path, struct stat *buf) {
    if (path_is_hidden(path)) { errno = ENOENT; return -1; }
    return old_lstat(path, buf);
}

static int new_fstatat(int dirfd, const char *path, struct stat *buf, int flags) {
    if (path_is_hidden(path)) { errno = ENOENT; return -1; }
    return old_fstatat(dirfd, path, buf, flags);
}

// ---- Master installer -----------------------------------------------------

static void install_spoof_hooks() {
    // Locate libc.so once; reuse dev+inode for all hooks.
    ino_t libc_inode = 0;
    dev_t libc_dev   = 0;
    for (auto &map : lsplt::MapInfo::Scan()) {
        if (map.path.ends_with("/libc.so")) {
            libc_inode = map.inode;
            libc_dev   = map.dev;
            break;
        }
    }
    if (!libc_inode) {
        ZLOGE("spoof: libc.so not found in maps\n");
        return;
    }

    struct HookEntry { const char *sym; void *hook; void **backup; };
    const HookEntry hooks[] = {
        // property
        {"__system_property_get", (void *)new_property_get, (void **)&old_property_get},
        // file access
        {"open",        (void *)new_open,       (void **)&old_open},
        {"openat",      (void *)new_openat,     (void **)&old_openat},
        {"access",      (void *)new_access,     (void **)&old_access},
        {"faccessat",   (void *)new_faccessat,  (void **)&old_faccessat},
        {"stat",        (void *)new_stat,       (void **)&old_stat},
        {"lstat",       (void *)new_lstat,      (void **)&old_lstat},
        {"fstatat",     (void *)new_fstatat,    (void **)&old_fstatat},
    };
    for (const auto &h : hooks) {
        if (!lsplt::RegisterHook(libc_dev, libc_inode, h.sym, h.hook, h.backup))
            ZLOGE("spoof: failed to register hook for %s\n", h.sym);
    }
    if (!lsplt::CommitHook())
        ZLOGE("spoof: CommitHook failed\n");
    else
        ZLOGI("spoof: all hooks installed for denylist process\n");
}

static int zygisk_request(int req) {
    int fd = connect_daemon(RequestCode::ZYGISK);
    if (fd < 0) return fd;
    write_int(fd, req);
    return fd;
}

ZygiskModule::ZygiskModule(int id, void *handle, void *entry)
    : id(id), handle(handle), entry{entry}, api{}, mod{nullptr} {
    // Make sure all pointers are null
    memset(&api, 0, sizeof(api));
    api.base.impl = this;
    api.base.registerModule = &ZygiskModule::RegisterModuleImpl;
}

bool ZygiskModule::RegisterModuleImpl(ApiTable *api, long *module) {
    if (api == nullptr || module == nullptr)
        return false;

    long api_version = *module;
    // Unsupported version
    if (api_version > ZYGISK_API_VERSION)
        return false;

    // Set the actual module_abi*
    api->base.impl->mod = { module };

    // Fill in API accordingly with module API version
    if (api_version >= 1) {
        api->v1.hookJniNativeMethods = hookJniNativeMethods;
        api->v1.pltHookRegister = [](auto a, auto b, auto c, auto d) {
            if (g_ctx) g_ctx->plt_hook_register(a, b, c, d);
        };
        api->v1.pltHookExclude = [](auto a, auto b) {
            if (g_ctx) g_ctx->plt_hook_exclude(a, b);
        };
        api->v1.pltHookCommit = []() { return g_ctx && g_ctx->plt_hook_commit(); };
        api->v1.connectCompanion = [](ZygiskModule *m) { return m->connectCompanion(); };
        api->v1.setOption = [](ZygiskModule *m, auto opt) { m->setOption(opt); };
    }
    if (api_version >= 2) {
        api->v2.getModuleDir = [](ZygiskModule *m) { return m->getModuleDir(); };
        api->v2.getFlags = [](auto) { return ZygiskModule::getFlags(); };
    }
    if (api_version >= 4) {
        api->v4.pltHookCommit = lsplt::CommitHook;
        api->v4.pltHookRegister = [](dev_t dev, ino_t inode, const char *symbol, void *fn, void **backup) {
            if (dev == 0 || inode == 0 || symbol == nullptr || fn == nullptr)
                return;
            lsplt::RegisterHook(dev, inode, symbol, fn, backup);
        };
        api->v4.exemptFd = [](int fd) { return g_ctx && g_ctx->exempt_fd(fd); };
    }

    return true;
}

bool ZygiskModule::valid() const {
    if (mod.api_version == nullptr)
        return false;
    switch (*mod.api_version) {
        case 5:
        case 4:
        case 3:
        case 2:
        case 1:
            return mod.v1->impl && mod.v1->preAppSpecialize && mod.v1->postAppSpecialize &&
                   mod.v1->preServerSpecialize && mod.v1->postServerSpecialize;
        default:
            return false;
    }
}

int ZygiskModule::connectCompanion() const {
    if (int fd = zygisk_request(+ZygiskRequest::ConnectCompanion); fd >= 0) {
#ifdef __LP64__
        write_any<bool>(fd, true);
#else
        write_any<bool>(fd, false);
#endif
        write_int(fd, id);
        return fd;
    }
    return -1;
}

int ZygiskModule::getModuleDir() const {
    if (owned_fd fd = zygisk_request(+ZygiskRequest::GetModDir); fd >= 0) {
        write_int(fd, id);
        return recv_fd(fd);
    }
    return -1;
}

void ZygiskModule::setOption(zygisk::Option opt) {
    if (g_ctx == nullptr)
        return;
    switch (opt) {
        case zygisk::FORCE_DENYLIST_UNMOUNT:
            g_ctx->flags |= DO_REVERT_UNMOUNT;
            break;
        case zygisk::DLCLOSE_MODULE_LIBRARY:
            unload = true;
            break;
    }
}

uint32_t ZygiskModule::getFlags() {
    return g_ctx ? (g_ctx->info_flags & ~PRIVATE_MASK) : 0;
}

void ZygiskModule::tryUnload() const {
    if (unload) dlclose(handle);
}

// -----------------------------------------------------------------

#define call_app(method)               \
switch (*mod.api_version) {            \
case 1:                                \
case 2: {                              \
    AppSpecializeArgs_v1 a(args);      \
    mod.v1->method(mod.v1->impl, &a);  \
    break;                             \
}                                      \
case 3:                                \
case 4:                                \
case 5:                                \
    mod.v1->method(mod.v1->impl, args);\
    break;                             \
}

void ZygiskModule::preAppSpecialize(AppSpecializeArgs_v5 *args) const {
    call_app(preAppSpecialize)
}

void ZygiskModule::postAppSpecialize(const AppSpecializeArgs_v5 *args) const {
    call_app(postAppSpecialize)
}

void ZygiskModule::preServerSpecialize(ServerSpecializeArgs_v1 *args) const {
    mod.v1->preServerSpecialize(mod.v1->impl, args);
}

void ZygiskModule::postServerSpecialize(const ServerSpecializeArgs_v1 *args) const {
    mod.v1->postServerSpecialize(mod.v1->impl, args);
}

// -----------------------------------------------------------------

void ZygiskContext::plt_hook_register(const char *regex, const char *symbol, void *fn, void **backup) {
    if (regex == nullptr || symbol == nullptr || fn == nullptr)
        return;
    regex_t re;
    if (regcomp(&re, regex, REG_NOSUB) != 0)
        return;
    mutex_guard lock(hook_info_lock);
    register_info.emplace_back(RegisterInfo{re, symbol, fn, backup});
}

void ZygiskContext::plt_hook_exclude(const char *regex, const char *symbol) {
    if (!regex) return;
    regex_t re;
    if (regcomp(&re, regex, REG_NOSUB) != 0)
        return;
    mutex_guard lock(hook_info_lock);
    ignore_info.emplace_back(IgnoreInfo{re, symbol ?: ""});
}

void ZygiskContext::plt_hook_process_regex() {
    if (register_info.empty())
        return;
    for (auto &map : lsplt::MapInfo::Scan()) {
        if (map.offset != 0 || !map.is_private || !(map.perms & PROT_READ)) continue;
        for (auto &reg: register_info) {
            if (regexec(&reg.regex, map.path.data(), 0, nullptr, 0) != 0)
                continue;
            bool ignored = false;
            for (auto &ign: ignore_info) {
                if (regexec(&ign.regex, map.path.data(), 0, nullptr, 0) != 0)
                    continue;
                if (ign.symbol.empty() || ign.symbol == reg.symbol) {
                    ignored = true;
                    break;
                }
            }
            if (!ignored) {
                lsplt::RegisterHook(map.dev, map.inode, reg.symbol, reg.callback, reg.backup);
            }
        }
    }
}

bool ZygiskContext::plt_hook_commit() {
    {
        mutex_guard lock(hook_info_lock);
        plt_hook_process_regex();
        for (auto& reg: register_info) {
            regfree(&reg.regex);
        }
        for (auto& ign: ignore_info) {
            regfree(&ign.regex);
        }
        register_info.clear();
        ignore_info.clear();
    }
    return lsplt::CommitHook();
}

// -----------------------------------------------------------------

int ZygiskContext::get_module_info(int uid, rust::Vec<int> &fds) {
    if (int fd = zygisk_request(+ZygiskRequest::GetInfo); fd >= 0) {
        write_int(fd, uid);
        write_string(fd, process);
#ifdef __LP64__
        write_any<bool>(fd, true);
#else
        write_any<bool>(fd, false);
#endif
        xxread(fd, &info_flags, sizeof(info_flags));
        if (zygisk_should_load_module(info_flags)) {
            fds = recv_fds(fd);
        }
        return fd;
    }
    return -1;
}

void ZygiskContext::sanitize_fds() {
    zygisk_close_logd();

    if (!is_child()) {
        return;
    }

    if (can_exempt_fd() && !exempted_fds.empty()) {
        auto update_fd_array = [&](int old_len) -> jintArray {
            jintArray array = env->NewIntArray(static_cast<int>(old_len + exempted_fds.size()));
            if (array == nullptr)
                return nullptr;

            env->SetIntArrayRegion(
                    array, old_len, static_cast<int>(exempted_fds.size()), exempted_fds.data());
            for (int fd : exempted_fds) {
                if (fd >= 0 && fd < allowed_fds.size()) {
                    allowed_fds[fd] = true;
                }
            }
            *args.app->fds_to_ignore = array;
            return array;
        };

        if (jintArray fdsToIgnore = *args.app->fds_to_ignore) {
            int *arr = env->GetIntArrayElements(fdsToIgnore, nullptr);
            int len = env->GetArrayLength(fdsToIgnore);
            for (int i = 0; i < len; ++i) {
                int fd = arr[i];
                if (fd >= 0 && fd < allowed_fds.size()) {
                    allowed_fds[fd] = true;
                }
            }
            if (jintArray newFdList = update_fd_array(len)) {
                env->SetIntArrayRegion(newFdList, 0, len, arr);
            }
            env->ReleaseIntArrayElements(fdsToIgnore, arr, JNI_ABORT);
        } else {
            update_fd_array(0);
        }
    }

    // Close all forbidden fds to prevent crashing
    auto dir = xopen_dir("/proc/self/fd");
    int dfd = dirfd(dir.get());
    for (dirent *entry; (entry = xreaddir(dir.get()));) {
        int fd = parse_int(entry->d_name);
        if ((fd < 0 || fd >= allowed_fds.size() || !allowed_fds[fd]) && fd != dfd) {
            close(fd);
        }
    }
}

bool ZygiskContext::exempt_fd(int fd) {
    if ((flags & POST_SPECIALIZE) || (flags & SKIP_CLOSE_LOG_PIPE))
        return true;
    if (!can_exempt_fd())
        return false;
    exempted_fds.push_back(fd);
    return true;
}

bool ZygiskContext::can_exempt_fd() const {
    return (flags & APP_FORK_AND_SPECIALIZE) && args.app->fds_to_ignore;
}

static int sigmask(int how, int signum) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, signum);
    return sigprocmask(how, &set, nullptr);
}

void ZygiskContext::fork_pre() {
    // Do our own fork before loading any 3rd party code
    // First block SIGCHLD, unblock after original fork is done
    sigmask(SIG_BLOCK, SIGCHLD);
    pid = old_fork();

    if (!is_child())
        return;

    // Record all open fds
    auto dir = xopen_dir("/proc/self/fd");
    for (dirent *entry; (entry = xreaddir(dir.get()));) {
        int fd = parse_int(entry->d_name);
        if (fd < 0 || fd >= allowed_fds.size()) {
            close(fd);
            continue;
        }
        allowed_fds[fd] = true;
    }
    // The dirfd will be closed once out of scope
    allowed_fds[dirfd(dir.get())] = false;
    // logd_fd should be handled separately
    if (int fd = zygisk_get_logd(); fd >= 0) {
        allowed_fds[fd] = false;
    }
}

void ZygiskContext::fork_post() {
    // Unblock SIGCHLD in case the original method didn't
    sigmask(SIG_UNBLOCK, SIGCHLD);
}

void ZygiskContext::run_modules_pre(rust::Vec<int> &fds) {
    for (int i = 0; i < fds.size(); ++i) {
        owned_fd fd = fds[i];
        struct stat s{};
        if (fstat(fd, &s) != 0 || !S_ISREG(s.st_mode)) {
            fds[i] = -1;
            continue;
        }
        android_dlextinfo info {
            .flags = ANDROID_DLEXT_USE_LIBRARY_FD,
            .library_fd = fd,
        };
        if (void *h = android_dlopen_ext("/jit-cache", RTLD_LAZY, &info)) {
            if (void *e = dlsym(h, "zygisk_module_entry")) {
                modules.emplace_back(i, h, e);
            }
        } else if (flags & SERVER_FORK_AND_SPECIALIZE) {
            ZLOGW("Failed to dlopen zygisk module: %s\n", dlerror());
            fds[i] = -1;
        }
    }

    for (auto it = modules.begin(); it != modules.end();) {
        it->onLoad(env);
        if (it->valid()) {
            ++it;
        } else {
            it = modules.erase(it);
        }
    }

    for (auto &m : modules) {
        if (flags & APP_SPECIALIZE) {
            m.preAppSpecialize(args.app);
        } else if (flags & SERVER_FORK_AND_SPECIALIZE) {
            m.preServerSpecialize(args.server);
        }
    }
}

void ZygiskContext::run_modules_post() {
    flags |= POST_SPECIALIZE;
    for (const auto &m : modules) {
        if (flags & APP_SPECIALIZE) {
            m.postAppSpecialize(args.app);
        } else if (flags & SERVER_FORK_AND_SPECIALIZE) {
            m.postServerSpecialize(args.server);
        }
        m.tryUnload();
    }
}

void ZygiskContext::app_specialize_pre() {
    flags |= APP_SPECIALIZE;

    rust::Vec<int> module_fds;
    owned_fd fd = get_module_info(args.app->uid, module_fds);
    if ((info_flags & UNMOUNT_MASK) == UNMOUNT_MASK) {
        ZLOGI("[%s] is on the denylist\n", process);
        flags |= DO_REVERT_UNMOUNT;
        // Install all root-detection evasion hooks: property spoofing,
        // file-access hiding, and /proc content filtering.
        install_spoof_hooks();
    } else if (fd >= 0) {
        run_modules_pre(module_fds);
    }
}

void ZygiskContext::app_specialize_post() {
    run_modules_post();
    if (info_flags & +ZygiskStateFlags::ProcessIsMagiskApp) {
        setenv("ZYGISK_ENABLED", "1", 1);
    }

    // Cleanups
    env->ReleaseStringUTFChars(args.app->nice_name, process);
}

void ZygiskContext::server_specialize_pre() {
    rust::Vec<int> module_fds;
    if (owned_fd fd = get_module_info(1000, module_fds); fd >= 0) {
        if (module_fds.empty()) {
            write_int(fd, 0);
        } else {
            run_modules_pre(module_fds);

            // Find all failed module ids and send it back to magiskd
            vector<int> failed_ids;
            for (int i = 0; i < module_fds.size(); ++i) {
                if (module_fds[i] < 0) {
                    failed_ids.push_back(i);
                }
            }
            write_vector(fd, failed_ids);
        }
    }
}

void ZygiskContext::server_specialize_post() {
    run_modules_post();
}

// -----------------------------------------------------------------

void ZygiskContext::nativeSpecializeAppProcess_pre() {
    process = env->GetStringUTFChars(args.app->nice_name, nullptr);
    ZLOGV("pre  specialize [%s]\n", process);
    // App specialize does not check FD
    flags |= SKIP_CLOSE_LOG_PIPE;
    app_specialize_pre();
}

void ZygiskContext::nativeSpecializeAppProcess_post() {
    ZLOGV("post specialize [%s]\n", process);
    app_specialize_post();
}

void ZygiskContext::nativeForkSystemServer_pre() {
    ZLOGV("pre  forkSystemServer\n");
    flags |= SERVER_FORK_AND_SPECIALIZE;
    process = "system_server";

    fork_pre();
    if (is_child()) {
        server_specialize_pre();
    }
    sanitize_fds();
}

void ZygiskContext::nativeForkSystemServer_post() {
    if (is_child()) {
        ZLOGV("post forkSystemServer\n");
        server_specialize_post();
    }
    fork_post();
}

void ZygiskContext::nativeForkAndSpecialize_pre() {
    process = env->GetStringUTFChars(args.app->nice_name, nullptr);
    ZLOGV("pre  forkAndSpecialize [%s]\n", process);
    flags |= APP_FORK_AND_SPECIALIZE;

    fork_pre();
    if (is_child()) {
        app_specialize_pre();
    }
    sanitize_fds();
}

void ZygiskContext::nativeForkAndSpecialize_post() {
    if (is_child()) {
        ZLOGV("post forkAndSpecialize [%s]\n", process);
        app_specialize_post();
    }
    fork_post();
}
