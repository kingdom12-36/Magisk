#![allow(clippy::missing_safety_doc)]

use logging::setup_klog;
// Has to be pub so all symbols in that crate is included
pub use shadowmaskpolicy;
use mount::{is_device_mounted, switch_root};
use rootdir::{OverlayAttr, inject_shadowmask_rc};

#[path = "../include/consts.rs"]
mod consts;
mod getinfo;
mod init;
mod logging;
mod mount;
mod rootdir;
mod selinux;
mod twostage;

#[cxx::bridge]
pub mod ffi {
    #[derive(Debug)]
    struct KeyValue {
        key: String,
        value: String,
    }

    struct BootConfig {
        skip_initramfs: bool,
        force_normal_boot: bool,
        rootwait: bool,
        emulator: bool,
        slot: [c_char; 3],
        dt_dir: [c_char; 64],
        fstab_suffix: [c_char; 32],
        hardware: [c_char; 32],
        hardware_plat: [c_char; 32],
        boot_mode: [c_char; 16],
        partition_map: Vec<KeyValue>,
    }

    struct ShadowMaskInit {
        preinit_dev: String,
        mount_list: Vec<String>,
        argv: *mut *mut c_char,
        config: BootConfig,
        overlay_con: Vec<OverlayAttr>,
    }

    unsafe extern "C++" {
        include!("init.hpp");

        #[cxx_name = "Utf8CStr"]
        type Utf8CStrRef<'a> = base::Utf8CStrRef<'a>;

        unsafe fn shadowmask_proxy_main(argc: i32, argv: *mut *mut c_char) -> i32;
        fn backup_init() -> Utf8CStrRef<'static>;

        // Constants
        fn split_plat_cil() -> Utf8CStrRef<'static>;
        fn preload_lib() -> Utf8CStrRef<'static>;
        fn preload_policy() -> Utf8CStrRef<'static>;
        fn preload_ack() -> Utf8CStrRef<'static>;
    }

    #[namespace = "rust"]
    extern "Rust" {
        fn setup_klog();
        fn inject_shadowmask_rc(fd: i32, tmp_dir: Utf8CStrRef);
        fn switch_root(path: Utf8CStrRef);
        fn is_device_mounted(dev: u64, target: Pin<&mut CxxString>) -> bool;
    }

    // BootConfig
    extern "Rust" {
        fn print(self: &BootConfig);
    }
    unsafe extern "C++" {
        fn init(self: &mut BootConfig);
        type kv_pairs;
        fn set(self: &mut BootConfig, config: &kv_pairs);
    }

    // ShadowMaskInit
    extern "Rust" {
        type OverlayAttr;
        fn parse_config_file(self: &mut ShadowMaskInit);
        fn mount_overlay(self: &mut ShadowMaskInit, dest: Utf8CStrRef);
        fn handle_sepolicy(self: &mut ShadowMaskInit);
        fn restore_overlay_contexts(self: &ShadowMaskInit);
    }
    unsafe extern "C++" {
        // Used in Rust
        fn mount_system_root(self: &mut ShadowMaskInit) -> bool;
        fn patch_rw_root(self: &mut ShadowMaskInit);
        fn patch_ro_root(self: &mut ShadowMaskInit);

        // Used in C++
        unsafe fn setup_tmp(self: &mut ShadowMaskInit, path: *const c_char);
        fn collect_devices(self: &ShadowMaskInit);
        fn mount_preinit_dir(self: &mut ShadowMaskInit);
        unsafe fn find_block(self: &ShadowMaskInit, partname: *const c_char) -> u64;
        unsafe fn patch_fissiond(self: &mut ShadowMaskInit, tmp_path: *const c_char);
    }
}
