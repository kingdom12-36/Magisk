#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <android/dlext.h>
#include <dlfcn.h>
#include <sys/system_properties.h>
#include <sys/xattr.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <string>

#include <lsplt.hpp>

#include <base.hpp>

#include "zygisk.hpp"
#include "module.hpp"

// Defined in hook.cpp — prevents munmap of ShadowMask when PLT hooks point into us.
extern void disable_shadowmask_unmap();

using namespace std;

// ---------------------------------------------------------------------------
// Root-detection evasion for DenyList target processes
//
// Applied in app_specialize_pre() immediately after fork, before the app's
// own code runs.  All hooks operate only inside the target process — Zygote
// itself and non-DenyList apps are unaffected.
//
// Six layers of defence:
//  1. Property spoofing  — __system_property_get AND __system_property_read_callback
//                          hooks hide root/debug props and return a stock
//                          verified-boot fingerprint covering both the legacy
//                          and modern prop-read APIs.
//  2. File-access hiding — open/openat/access/faccessat/stat/lstat/fstatat/
//                          stat64/lstat64 return ENOENT for su binaries, the
//                          /data/adb secure directory, and any path containing
//                          "shadowmask", "zygisk", or root-manager names.
//  3. /proc filtering    — /proc/self/maps, /proc/net/unix, /proc/mounts,
//                          /proc/self/mountinfo — returned fds point to
//                          in-memory copies with ShadowMask/Zygisk lines stripped.
//  4. SELinux attr spoof — /proc/self/attr/current reads return a clean
//                          untrusted_app context so the ShadowMask domain is never
//                          exposed.
//  5. SELinux xattr hide — getxattr/lgetxattr/fgetxattr replace any
//                          shadowmask_file / shadowmask_log_file label with
//                          u:object_r:system_file:s0, covering both path-based
//                          and fd-based callers (getfilecon uses lgetxattr).
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
    // Native bridge — Zygisk sets this to "libzygisk.so" to bootstrap itself.
    // An app reading it would see "libzygisk.so" and instantly know Zygisk is active.
    // Spoof it back to "0" (the default "no native bridge" value).
    {"ro.dalvik.vm.native.bridge",      "0"},
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
                strncpy(value, e.value, PROP_VALUE_MAX - 1);
                value[PROP_VALUE_MAX - 1] = '\0';
                return static_cast<int>(strlen(e.value));
            }
        }
    }
    return old_property_get(key, value);
}

// Hook __system_property_read_callback — modern alternative to __system_property_get.
// Many detection tools call __system_property_find + __system_property_read_callback
// instead of the legacy API, so we must intercept this path too.
struct PropInfo; // opaque bionic type
using prop_read_cb_fn_t = void (*)(
    const PropInfo *,
    void (*)(void * /*cookie*/, const char * /*name*/, const char * /*value*/, uint32_t /*serial*/),
    void * /*cookie*/
);
static prop_read_cb_fn_t old_prop_read_callback = nullptr;

// We wrap the caller's callback so we can substitute spoof values by name.
struct PropReadCtx {
    void (*orig_cb)(void *, const char *, const char *, uint32_t);
    void *orig_cookie;
};

static void prop_intercept_cb(void *cookie,
                               const char *name, const char *value, uint32_t serial) {
    auto *ctx = static_cast<PropReadCtx *>(cookie);
    for (const auto &e : SPOOF_PROPS) {
        if (name && strcmp(name, e.key) == 0) {
            ctx->orig_cb(ctx->orig_cookie, name, e.value, serial);
            return;
        }
    }
    ctx->orig_cb(ctx->orig_cookie, name, value, serial);
}

static void new_prop_read_callback(
    const PropInfo *pi,
    void (*cb)(void *, const char *, const char *, uint32_t),
    void *cookie)
{
    PropReadCtx ctx{ cb, cookie };
    old_prop_read_callback(pi, prop_intercept_cb, &ctx);
}

// ---- 2. File-access hiding ------------------------------------------------

// Any path whose string contains one of these fragments will be made
// invisible to the target process (ENOENT returned).
static const char *HIDDEN_PATH_FRAGS[] = {
    "shadowmask",
    "zygisk",
    "/data/adb",       // secure root directory — any subpath leaks installer state
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

// Helper: match /proc/self/<suffix> or /proc/<digits>/<suffix>.
// Uses only strncmp/strcmp — no heap, no buffer, no snprintf.
static bool is_proc_self_file(const char *path, const char *suffix) {
    if (!path) return false;
    // Fast path: /proc/self/<suffix>
    if (strncmp(path, "/proc/self/", 11) == 0 &&
        strcmp(path + 11, suffix) == 0)
        return true;
    // /proc/<digits>/<suffix>
    if (strncmp(path, "/proc/", 6) != 0) return false;
    const char *p = path + 6;
    while (*p >= '0' && *p <= '9') ++p;
    if (*p != '/') return false;
    ++p;
    return strcmp(p, suffix) == 0;
}

static bool is_proc_maps_path(const char *path) {
    return is_proc_self_file(path, "maps");
}

static bool is_proc_net_unix(const char *path) {
    return path && strcmp(path, "/proc/net/unix") == 0;
}

// /proc/mounts  and  /proc/self/mountinfo (or /proc/<pid>/mountinfo)
static bool is_proc_mounts_path(const char *path) {
    if (!path) return false;
    if (strcmp(path, "/proc/mounts") == 0) return true;
    return is_proc_self_file(path, "mounts");
}

static bool is_proc_mountinfo_path(const char *path) {
    return is_proc_self_file(path, "mountinfo");
}

// /proc/self/attr/current — SELinux domain of this process
static bool is_proc_attr_current(const char *path) {
    return is_proc_self_file(path, "attr/current");
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
    return strstr(line, "shadowmask") || strstr(line, "zygisk");
}

static bool unix_drop_line(const char *line, size_t) {
    return strstr(line, "shadowmask") != nullptr;
}

// Drop mount entries that reference shadowmask/zygisk bind-mounts.
// Both /proc/mounts and /proc/self/mountinfo use whitespace-delimited
// fields where the mount-point or source may contain "shadowmask" or ".shadowmask".
static bool mounts_drop_line(const char *line, size_t) {
    return strstr(line, "shadowmask") || strstr(line, ".shadowmask") || strstr(line, "zygisk");
}

// Return a memfd whose content is a clean SELinux process context so that
// /proc/self/attr/current never reveals the shadowmask domain.
static int make_selinux_attr_fd() {
#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif
    // Use the standard untrusted_app context; adjust if needed.
    static const char ctx[] = "u:r:untrusted_app:s0\n";
    int mfd = (int)syscall(__NR_memfd_create, "attr_view", MFD_CLOEXEC);
    if (mfd < 0) return -1;
    ::write(mfd, ctx, sizeof(ctx) - 1);
    ::lseek(mfd, 0, SEEK_SET);
    return mfd;
}

// ---- Hook implementations -------------------------------------------------

using open_fn_t    = int (*)(const char *, int, mode_t);
using openat_fn_t  = int (*)(int, const char *, int, mode_t);
using access_fn_t  = int (*)(const char *, int);
using faccess_fn_t = int (*)(int, const char *, int, int);
using stat_fn_t    = int (*)(const char *, struct stat *);
using fstatat_fn_t   = int (*)(int, const char *, struct stat *, int);
using getxattr_fn_t  = ssize_t (*)(const char *, const char *, void *, size_t);
using lgetxattr_fn_t = ssize_t (*)(const char *, const char *, void *, size_t);
using fgetxattr_fn_t = ssize_t (*)(int, const char *, void *, size_t);
// stat64 / lstat64 — distinct bionic symbols on 32-bit; aliases on 64-bit.
// Using void* for the struct pointer avoids pulling in _LARGEFILE64_SOURCE
// guards: we only inspect the path argument, never the stat buffer.
using stat64_fn_t  = int (*)(const char *, void *);
using lstat64_fn_t = int (*)(const char *, void *);

static open_fn_t      old_open      = nullptr;
static openat_fn_t    old_openat    = nullptr;
static access_fn_t    old_access    = nullptr;
static faccess_fn_t   old_faccessat = nullptr;
static stat_fn_t      old_stat      = nullptr;
static stat_fn_t      old_lstat     = nullptr;
static fstatat_fn_t   old_fstatat   = nullptr;
static stat64_fn_t    old_stat64    = nullptr;
static lstat64_fn_t   old_lstat64   = nullptr;
static getxattr_fn_t  old_getxattr  = nullptr;
static lgetxattr_fn_t old_lgetxattr = nullptr;
static fgetxattr_fn_t old_fgetxattr = nullptr;

// Clean SELinux label substituted for any shadowmask_* context.
// Length (25 chars + NUL) intentionally matches SHADOWMASK_FILE_CON / SHADOWMASK_LOG_FILE_CON
// so no buffer arithmetic is needed; we use memcpy for safety.
static const char CLEAN_FILE_CTX[] = "u:object_r:system_file:s0";
static constexpr size_t   CLEAN_FILE_CTX_LEN = sizeof(CLEAN_FILE_CTX) - 1; // excl. NUL

// Replace the SELinux context in `value` if it mentions "shadowmask".
// `buf_size` is the caller-provided buffer capacity (the `size` arg they passed).
static void maybe_clean_selinux_xattr(const char *name, void *value,
                                      ssize_t ret, size_t buf_size) {
    if (ret <= 0 || !name || !value) return;
    if (strcmp(name, "security.selinux") != 0) return;
    char *ctx = static_cast<char *>(value);
    if (!strstr(ctx, "shadowmask")) return;
    // Guard: only overwrite if our replacement fits in the caller's buffer.
    if (buf_size < CLEAN_FILE_CTX_LEN + 1) return;
    memcpy(ctx, CLEAN_FILE_CTX, CLEAN_FILE_CTX_LEN + 1); // includes NUL
}

static ssize_t new_getxattr(const char *path, const char *name, void *value, size_t size) {
    if (path_is_hidden(path)) { errno = ENODATA; return -1; }
    ssize_t ret = old_getxattr(path, name, value, size);
    maybe_clean_selinux_xattr(name, value, ret, size);
    return ret;
}

static ssize_t new_lgetxattr(const char *path, const char *name, void *value, size_t size) {
    if (path_is_hidden(path)) { errno = ENODATA; return -1; }
    ssize_t ret = old_lgetxattr(path, name, value, size);
    maybe_clean_selinux_xattr(name, value, ret, size);
    return ret;
}

// fd-based xattr read — can't filter by path, but still cleans the label value.
static ssize_t new_fgetxattr(int fd, const char *name, void *value, size_t size) {
    ssize_t ret = old_fgetxattr(fd, name, value, size);
    maybe_clean_selinux_xattr(name, value, ret, size);
    return ret;
}

static int new_open(const char *path, int flags, mode_t mode) {
    if (path_is_hidden(path)) { errno = ENOENT; return -1; }
    // Spoof /proc/self/attr/current before real open — no fd needed
    if ((flags & O_ACCMODE) == O_RDONLY && is_proc_attr_current(path))
        return make_selinux_attr_fd();
    int fd = old_open(path, flags, mode);
    if (fd >= 0 && (flags & O_ACCMODE) == O_RDONLY) {
        if (is_proc_maps_path(path))     return make_filtered_fd(fd, maps_drop_line);
        if (is_proc_net_unix(path))      return make_filtered_fd(fd, unix_drop_line);
        if (is_proc_mounts_path(path))   return make_filtered_fd(fd, mounts_drop_line);
        if (is_proc_mountinfo_path(path))return make_filtered_fd(fd, mounts_drop_line);
    }
    return fd;
}

static int new_openat(int dirfd, const char *path, int flags, mode_t mode) {
    if (path_is_hidden(path)) { errno = ENOENT; return -1; }
    if ((flags & O_ACCMODE) == O_RDONLY && is_proc_attr_current(path))
        return make_selinux_attr_fd();
    int fd = old_openat(dirfd, path, flags, mode);
    if (fd >= 0 && (flags & O_ACCMODE) == O_RDONLY) {
        if (is_proc_maps_path(path))     return make_filtered_fd(fd, maps_drop_line);
        if (is_proc_net_unix(path))      return make_filtered_fd(fd, unix_drop_line);
        if (is_proc_mounts_path(path))   return make_filtered_fd(fd, mounts_drop_line);
        if (is_proc_mountinfo_path(path))return make_filtered_fd(fd, mounts_drop_line);
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

// stat64 / lstat64 — on 32-bit these are separate symbols with 64-bit st_ino/st_size;
// we only need to check the path, so the void* buf pointer is never dereferenced.
static int new_stat64(const char *path, void *buf) {
    if (path_is_hidden(path)) { errno = ENOENT; return -1; }
    return old_stat64(path, buf);
}

static int new_lstat64(const char *path, void *buf) {
    if (path_is_hidden(path)) { errno = ENOENT; return -1; }
    return old_lstat64(path, buf);
}

// ---- Master installer -----------------------------------------------------

static void install_spoof_hooks() {
    struct HookEntry { const char *sym; void *hook; void **backup; };
    const HookEntry hooks[] = {
        // property — legacy API and modern callback API
        {"__system_property_get",             (void *)new_property_get,       (void **)&old_property_get},
        {"__system_property_read_callback",   (void *)new_prop_read_callback, (void **)&old_prop_read_callback},
        // file access — path-based
        {"open",        (void *)new_open,       (void **)&old_open},
        {"openat",      (void *)new_openat,     (void **)&old_openat},
        {"access",      (void *)new_access,     (void **)&old_access},
        {"faccessat",   (void *)new_faccessat,  (void **)&old_faccessat},
        {"stat",        (void *)new_stat,       (void **)&old_stat},
        {"lstat",       (void *)new_lstat,      (void **)&old_lstat},
        {"fstatat",     (void *)new_fstatat,    (void **)&old_fstatat},
        // stat64/lstat64 — separate symbols on 32-bit processes
        {"stat64",      (void *)new_stat64,     (void **)&old_stat64},
        {"lstat64",     (void *)new_lstat64,    (void **)&old_lstat64},
        // SELinux xattr — path-based and fd-based callers
        {"getxattr",    (void *)new_getxattr,   (void **)&old_getxattr},
        {"lgetxattr",   (void *)new_lgetxattr,  (void **)&old_lgetxattr},
        {"fgetxattr",   (void *)new_fgetxattr,  (void **)&old_fgetxattr},
    };

    size_t registered_count = 0;

    for (auto &map : lsplt::MapInfo::Scan()) {
        // استهداف خرائط الـ ELF الرئسية الفعالة فقط (offset == 0)
        if (map.offset != 0 || !map.is_private || !(map.perms & PROT_READ)) continue;

        // استثناء مكتبة سامسونج libzygisk ومكتبات النظام الحساسة لتفادي Scudo Heap Corruption
        if (map.path.ends_with("/libzygisk.so") ||
            map.path.ends_with("/libandroid_runtime.so") ||
            map.path.ends_with("/libart.so")) {
            continue;
        }

        // تطبيق الـ Hooks على بقية المكتبات المسموحة فقط
        for (const auto &h : hooks) {
            if (lsplt::RegisterHook(map.dev, map.inode, h.sym, h.hook, h.backup)) {
                registered_count++;
            }
        }
    }

    if (registered_count == 0) {
        ZLOGW("spoof: no hooks registered\n");
        return;
    }

    if (!lsplt::CommitHook())
        ZLOGE("spoof: CommitHook failed\n");
    else {
        ZLOGI("spoof: all hooks installed safely for denylist process\n");
        // Keep ShadowMask mapped for process lifetime to prevent dangling GOT pointers
        disable_shadowmask_unmap();
    }
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

    // ── Option D safety net ─────────────────────────────────────────────────
    // Always exempt the ShadowMask manager from spoof hooks by name, regardless
    // of whether the daemon has resolved its UID yet (first boot, randomised-
    // package-name scenarios, or any get_manager_uid() race).
    //
    // Without this guard the manager process receives install_spoof_hooks() and
    // its own FIFO path (".shadowmask/su_request_<pid>") is matched by
    // path_is_hidden() → canWrite() returns false → SuRequestActivity bails
    // before showing the permission dialog → no grant, no superuser list entry.
    if (process && strstr(process, "com.shadowmask")) {
        info_flags |= +ZygiskStateFlags::ProcessIsShadowMaskApp;
    }
    // ───────────────────────────────────────────────────────────────────────

    rust::Vec<int> module_fds;
    owned_fd fd = get_module_info(args.app->uid, module_fds);
    if ((info_flags & UNMOUNT_MASK) == UNMOUNT_MASK) {
        ZLOGI("[%s] is on the denylist\n", process);
        flags |= DO_REVERT_UNMOUNT;
        // Skip for the ShadowMask Manager itself — it needs to read its own
        // mount entries and SELinux labels to detect that ShadowMask is active.
        if (!(info_flags & +ZygiskStateFlags::ProcessIsShadowMaskApp)) {
            install_spoof_hooks();
        }
    } else {
        if (fd >= 0) {
            run_modules_pre(module_fds);
        }
        // Auto-deny: hide ShadowMask from every app that has not been explicitly
        // granted root access and is not the ShadowMask Manager itself.
        //
        // This means you never have to touch the DenyList at all — games,
        // banking apps, and everything else see a clean unrooted phone by
        // default.  Only apps you have explicitly allowed root (via the
        // superuser prompt) can see that ShadowMask exists.
        if (!(info_flags & +ZygiskStateFlags::ProcessIsShadowMaskApp) &&
            !(info_flags & +ZygiskStateFlags::ProcessGrantedRoot)) {
            install_spoof_hooks();
        }
    }
}

void ZygiskContext::app_specialize_post() {
    run_modules_post();
    if (info_flags & +ZygiskStateFlags::ProcessIsShadowMaskApp) {
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

            // Find all failed module ids and send it back to shadowmaskd
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
