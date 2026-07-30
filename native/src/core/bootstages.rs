use crate::consts::{APP_PACKAGE_NAME, BBPATH, DATABIN, KSU_BIN_DIR, MODULEROOT, SECURE_DIR, SUSFS_KMOD_LOAD_MARKER, SUSFS_KMOD_PATH};
use crate::daemon::ShadowMaskD;
use crate::ffi::{
    DbEntryKey, RequestCode, check_key_combo, exec_common_scripts, exec_module_scripts,
    get_shadowmask_tmp, initialize_denylist,
};
use crate::logging::setup_logfile;
use crate::module::disable_modules;
use crate::mount::{clean_mounts, setup_preinit_dir};
use crate::resetprop::get_prop;
use crate::selinux::restorecon;
use base::const_format::concatcp;
use base::{BufReadExt, FsPathBuilder, ResultExt, cstr, error, info};
use bitflags::bitflags;
use nix::fcntl::OFlag;
use std::io::BufReader;
use std::os::unix::net::UnixStream;
use std::process::{Command, Stdio};
use std::sync::atomic::Ordering;

bitflags! {
    #[derive(Default)]
    pub struct BootState : u32 {
        const PostFsDataDone = 1 << 0;
        const LateStartDone = 1 << 1;
        const BootComplete = 1 << 2;
        const SafeMode = 1 << 3;
    }
}

impl ShadowMaskD {
    fn setup_shadowmask_env(&self) -> bool {
        info!("* Initializing ShadowMask environment");

        let mut buf = cstr::buf::default();

        let app_bin_dir = buf
            .append_path(self.app_data_dir())
            .append_path("0")
            .append_path(APP_PACKAGE_NAME)
            .append_path("install");

        // Alternative binaries paths
        let alt_bin_dirs = &[
            cstr!("/cache/data_adb/shadowmask"),
            cstr!("/data/shadowmask"),
            app_bin_dir,
        ];
        for dir in alt_bin_dirs {
            if dir.exists() {
                cstr!(DATABIN).remove_all().ok();
                dir.copy_to(cstr!(DATABIN)).ok();
                dir.remove_all().ok();
            }
        }
        cstr!("/cache/data_adb").remove_all().ok();

        // Directories in /data/adb
        cstr!(SECURE_DIR).follow_link().chmod(0o700).log_ok();
        cstr!(DATABIN).mkdir(0o755).log_ok();
        cstr!(MODULEROOT).mkdir(0o755).log_ok();
        cstr!(concatcp!(SECURE_DIR, "/post-fs-data.d"))
            .mkdir(0o755)
            .log_ok();
        cstr!(concatcp!(SECURE_DIR, "/service.d"))
            .mkdir(0o755)
            .log_ok();
        restorecon();

        let busybox = cstr!(concatcp!(DATABIN, "/busybox"));
        if !busybox.exists() {
            return false;
        }

        let tmp_bb = buf.append_path(get_shadowmask_tmp()).append_path(BBPATH);
        tmp_bb.mkdirs(0o755).ok();
        tmp_bb.append_path("busybox");
        busybox.copy_to(tmp_bb).ok();
        tmp_bb.follow_link().chmod(0o755).log_ok();

        // Install busybox applets
        Command::new(&tmp_bb)
            .arg("--install")
            .arg("-s")
            .arg(tmp_bb.parent_dir().unwrap_or_default())
            .stdout(Stdio::null())
            .stderr(Stdio::null())
            .status()
            .log_ok();

        // shadowmask32 and shadowmaskpolicy are not installed into ramdisk and has to be copied
        // from data to shadowmask tmp
        let shadowmask32 = cstr!(concatcp!(DATABIN, "/shadowmask32"));
        if shadowmask32.exists() {
            let tmp = buf.append_path(get_shadowmask_tmp()).append_path("shadowmask32");
            shadowmask32.copy_to(tmp).log_ok();
        }
        let shadowmaskpolicy = cstr!(concatcp!(DATABIN, "/shadowmaskpolicy"));
        if shadowmaskpolicy.exists() {
            let tmp = buf
                .append_path(get_shadowmask_tmp())
                .append_path("shadowmaskpolicy");
            shadowmaskpolicy.copy_to(tmp).log_ok();
        }

        // Setup SUSFS compatibility directory for susfs4ksu-module
        self.setup_susfs_env();
        // Load SUSFS kernel module (provides sus_path/mount/kstat/map without kernel patches)
        self.load_susfs_kmod();

        true
    }

    /// Create /data/adb/ksu/bin/ and populate it with the SUSFS userspace tool.
    ///
    /// The susfs4ksu-module expects:
    ///   - /data/adb/ksu/bin/  (directory) to exist at install time
    ///   - /data/adb/ksu/bin/ksu_susfs  (binary)
    ///   - /data/adb/ksu/bin/sus_su     (binary, deprecated — kept for compatibility)
    ///
    /// The module's customize.sh copies its own bundled binaries into this directory,
    /// overwriting whatever we pre-populate.  We still pre-populate from DATABIN so
    /// the directory is valid even before the module is installed.
    fn setup_susfs_env(&self) {
        info!("* Setting up SUSFS compatibility directory");

        // Create /data/adb/ksu/bin/
        let ksu_bin = cstr!(KSU_BIN_DIR);
        if ksu_bin.mkdirs(0o755).is_err() {
            // Directory already exists — that is fine
        }
        ksu_bin.follow_link().chmod(0o755).log_ok();

        // Copy ksu_susfs binary from ShadowMask DATABIN if present
        let src_susfs = cstr!(concatcp!(DATABIN, "/ksu_susfs"));
        if src_susfs.exists() {
            let dst = cstr!(concatcp!(KSU_BIN_DIR, "/ksu_susfs"));
            src_susfs.copy_to(dst).log_ok();
            dst.follow_link().chmod(0o755).log_ok();
            info!("* SUSFS: deployed ksu_susfs to {}", KSU_BIN_DIR);
        }

        // Copy sus_su binary from ShadowMask DATABIN if present (deprecated but kept
        // for compatibility with modules that still reference it)
        let src_sus_su = cstr!(concatcp!(DATABIN, "/sus_su"));
        if src_sus_su.exists() {
            let dst = cstr!(concatcp!(KSU_BIN_DIR, "/sus_su"));
            src_sus_su.copy_to(dst).log_ok();
            dst.follow_link().chmod(0o755).log_ok();
            info!("* SUSFS: deployed sus_su to {}", KSU_BIN_DIR);
        }
    }


    /// Load the ShadowMask SUSFS kernel module (shadowmask_sus.ko).
    ///
    /// Provides SUSFS-compatible hiding (sus_path, sus_mount, sus_kstat, sus_map)
    /// via kallsyms + syscall-table hooks — no kernel source patches required.
    /// Compatible with the ksu_susfs userspace tool (same syscall ABI).
    /// Safe to call on kernels that don't support modules — errors are non-fatal.
    fn load_susfs_kmod(&self) {
        // Skip if already loaded
        if cstr!(SUSFS_KMOD_LOAD_MARKER).exists() {
            info!("* SUSFS kmod already active");
            return;
        }

        let kmod = cstr!(SUSFS_KMOD_PATH);
        if !kmod.exists() {
            info!("* SUSFS kmod not present — userspace tool will return ERR_CMD_NOT_SUPPORTED");
            return;
        }

        info!("* Loading SUSFS kmod: {}", SUSFS_KMOD_PATH);
        let status = Command::new("insmod")
            .arg(SUSFS_KMOD_PATH)
            .stdout(Stdio::null())
            .stderr(Stdio::null())
            .status();

        match status {
            Ok(s) if s.success() =>
                info!("* SUSFS kmod loaded — hiding features active"),
            Ok(s) =>
                info!("* SUSFS kmod insmod exit {}", s.code().unwrap_or(-1)),
            Err(e) =>
                info!("* SUSFS kmod insmod error: {e}"),
        }
    }

    fn post_fs_data(&self) -> bool {
        setup_logfile();
        info!("** post-fs-data mode running");

        self.preserve_stub_apk();

        // Check secure dir
        let secure_dir = cstr!(SECURE_DIR);
        if !secure_dir.exists() {
            if self.sdk_int < 24 {
                secure_dir.mkdir(0o700).log_ok();
            } else {
                error!("* {} is not present, abort", SECURE_DIR);
                return true;
            }
        }

        self.prune_su_access();

        if !self.setup_shadowmask_env() {
            error!("* ShadowMask environment incomplete, abort");
            return true;
        }

        // Check safe mode
        let boot_cnt = self.get_db_setting(DbEntryKey::BootloopCount);
        self.set_db_setting(DbEntryKey::BootloopCount, boot_cnt + 1)
            .log()
            .ok();
        let safe_mode = boot_cnt >= 2
            || get_prop(cstr!("persist.sys.safemode")) == "1"
            || get_prop(cstr!("ro.sys.safemode")) == "1"
            || check_key_combo();

        if safe_mode {
            info!("* Safe mode triggered");
            // Disable all modules and zygisk so next boot will be clean
            disable_modules();
            self.set_db_setting(DbEntryKey::ZygiskConfig, 0).log_ok();
            return true;
        }

        exec_common_scripts(cstr!("post-fs-data"));
        self.zygisk_enabled.store(
            self.get_db_setting(DbEntryKey::ZygiskConfig) != 0,
            Ordering::Release,
        );
        initialize_denylist();
        self.handle_modules();
        clean_mounts();

        false
    }

    fn late_start(&self) {
        setup_logfile();
        info!("** late_start service mode running");

        exec_common_scripts(cstr!("service"));
        if let Some(module_list) = self.module_list.get() {
            exec_module_scripts(cstr!("service"), module_list);
        }
    }

    fn boot_complete(&self) {
        setup_logfile();
        info!("** boot-complete triggered");

        // Reset the bootloop counter once we have boot-complete
        self.set_db_setting(DbEntryKey::BootloopCount, 0).log_ok();

        // At this point it's safe to create the folder
        let secure_dir = cstr!(SECURE_DIR);
        if !secure_dir.exists() {
            secure_dir.mkdir(0o700).log_ok();
        }

        setup_preinit_dir();
        self.ensure_manager();
        if self.zygisk_enabled.load(Ordering::Relaxed) {
            self.zygisk.lock().reset(true);
        }
    }

    pub fn boot_stage_handler(&self, client: UnixStream, code: RequestCode) {
        // Make sure boot stage execution is always serialized
        let mut state = self.boot_stage_lock.lock();

        match code {
            RequestCode::POST_FS_DATA => {
                if check_data() && !state.contains(BootState::PostFsDataDone) {
                    if self.post_fs_data() {
                        state.insert(BootState::SafeMode);
                    }
                    state.insert(BootState::PostFsDataDone);
                }
            }
            RequestCode::LATE_START => {
                drop(client);
                if state.contains(BootState::PostFsDataDone) && !state.contains(BootState::SafeMode)
                {
                    self.late_start();
                    state.insert(BootState::LateStartDone);
                }
            }
            RequestCode::BOOT_COMPLETE => {
                drop(client);
                if state.contains(BootState::PostFsDataDone) {
                    state.insert(BootState::BootComplete);
                    self.boot_complete()
                }
            }
            _ => {}
        }
    }
}

fn check_data() -> bool {
    if let Ok(file) = cstr!("/proc/mounts").open(OFlag::O_RDONLY | OFlag::O_CLOEXEC) {
        let mut mnt = false;
        BufReader::new(file).for_each_line(|line| {
            if line.contains(" /data ") && !line.contains("tmpfs") {
                mnt = true;
                return false;
            }
            true
        });
        if !mnt {
            return false;
        }
        let crypto = get_prop(cstr!("ro.crypto.state"));
        return if !crypto.is_empty() {
            if crypto != "encrypted" {
                // Unencrypted, we can directly access data
                true
            } else {
                // Encrypted, check whether vold is started
                !get_prop(cstr!("init.svc.vold")).is_empty()
            }
        } else {
            // ro.crypto.state is not set, assume it's unencrypted
            true
        };
    }
    false
}
