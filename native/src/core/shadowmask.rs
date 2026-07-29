use crate::consts::{APPLET_NAMES, SHADOWMASK_VER_CODE, SHADOWMASK_VERSION, POST_FS_DATA_WAIT_TIME};
use crate::daemon::connect_daemon;
use crate::ffi::{RequestCode, denylist_cli, get_shadowmask_tmp, install_module, unlock_blocks};
use crate::mount::find_preinit_device;
use crate::selinux::restorecon;
use crate::socket::{Decodable, Encodable};
use argh::FromArgs;
use base::{CmdArgs, EarlyExitExt, LoggedResult, Utf8CString, argh, clone_attr};
use nix::poll::{PollFd, PollFlags, PollTimeout};
use std::ffi::c_char;
use std::os::fd::AsFd;
use std::process::exit;

fn print_usage() {
    eprintln!(
        r#"ShadowMask - Multi-purpose Utility

Usage: shadowmask [applet [arguments]...]
   or: shadowmask [options]...

Options:
   -c                        print current binary version
   -v                        print running daemon version
   -V                        print running daemon version code
   --list                    list all available applets
   --remove-modules [-n]     remove all modules, reboot if -n is not provided
   --install-module ZIP      install a module zip file

Advanced Options (Internal APIs):
   --daemon                  manually start shadowmask daemon
   --stop                    remove all shadowmask changes and stop daemon
   --[init trigger]          callback on init triggers. Valid triggers:
                             post-fs-data, service, boot-complete, zygote-restart
   --unlock-blocks           set BLKROSET flag to OFF for all block devices
   --restorecon              restore selinux context on ShadowMask files
   --clone-attr SRC DEST     clone permission, owner, and selinux context
   --clone SRC DEST          clone SRC to DEST
   --sqlite SQL              exec SQL commands to ShadowMask database
   --path                    print ShadowMask tmpfs mount path
   --denylist ARGS           denylist config CLI
   --preinit-device          resolve a device to store preinit files

Available applets:
     {}
"#,
        APPLET_NAMES.join(", ")
    );
}

#[derive(FromArgs)]
struct Cli {
    #[argh(subcommand)]
    action: ShadowMaskAction,
}

#[derive(FromArgs)]
#[argh(subcommand)]
enum ShadowMaskAction {
    LocalVersion(LocalVersion),
    Version(Version),
    VersionCode(VersionCode),
    List(ListApplets),
    RemoveModules(RemoveModules),
    InstallModule(InstallModule),
    Daemon(StartDaemon),
    Stop(StopDaemon),
    PostFsData(PostFsData),
    Service(ServiceCmd),
    BootComplete(BootComplete),
    ZygoteRestart(ZygoteRestart),
    UnlockBlocks(UnlockBlocks),
    RestoreCon(RestoreCon),
    CloneAttr(CloneAttr),
    CloneFile(CloneFile),
    Sqlite(Sqlite),
    Path(PathCmd),
    DenyList(DenyList),
    PreInitDevice(PreInitDevice),
}

#[derive(FromArgs)]
#[argh(subcommand, name = "-c")]
struct LocalVersion {}

#[derive(FromArgs)]
#[argh(subcommand, name = "-v")]
struct Version {}

#[derive(FromArgs)]
#[argh(subcommand, name = "-V")]
struct VersionCode {}

#[derive(FromArgs)]
#[argh(subcommand, name = "--list")]
struct ListApplets {}

#[derive(FromArgs)]
#[argh(subcommand, name = "--remove-modules")]
struct RemoveModules {
    #[argh(switch, short = 'n')]
    no_reboot: bool,
}

#[derive(FromArgs)]
#[argh(subcommand, name = "--install-module")]
struct InstallModule {
    #[argh(positional)]
    zip: Utf8CString,
}

#[derive(FromArgs)]
#[argh(subcommand, name = "--daemon")]
struct StartDaemon {}

#[derive(FromArgs)]
#[argh(subcommand, name = "--stop")]
struct StopDaemon {}

#[derive(FromArgs)]
#[argh(subcommand, name = "--post-fs-data")]
struct PostFsData {}

#[derive(FromArgs)]
#[argh(subcommand, name = "--service")]
struct ServiceCmd {}

#[derive(FromArgs)]
#[argh(subcommand, name = "--boot-complete")]
struct BootComplete {}

#[derive(FromArgs)]
#[argh(subcommand, name = "--zygote-restart")]
struct ZygoteRestart {}

#[derive(FromArgs)]
#[argh(subcommand, name = "--unlock-blocks")]
struct UnlockBlocks {}

#[derive(FromArgs)]
#[argh(subcommand, name = "--restorecon")]
struct RestoreCon {}

#[derive(FromArgs)]
#[argh(subcommand, name = "--clone-attr")]
struct CloneAttr {
    #[argh(positional)]
    from: Utf8CString,
    #[argh(positional)]
    to: Utf8CString,
}

#[derive(FromArgs)]
#[argh(subcommand, name = "--clone")]
struct CloneFile {
    #[argh(positional)]
    from: Utf8CString,
    #[argh(positional)]
    to: Utf8CString,
}

#[derive(FromArgs)]
#[argh(subcommand, name = "--sqlite")]
struct Sqlite {
    #[argh(positional)]
    sql: String,
}

#[derive(FromArgs)]
#[argh(subcommand, name = "--path")]
struct PathCmd {}

#[derive(FromArgs)]
#[argh(subcommand, name = "--denylist")]
struct DenyList {
    #[argh(positional, greedy)]
    args: Vec<String>,
}

#[derive(FromArgs)]
#[argh(subcommand, name = "--preinit-device")]
struct PreInitDevice {}

impl ShadowMaskAction {
    fn exec(self) -> LoggedResult<i32> {
        use ShadowMaskAction::*;
        match self {
            LocalVersion(_) => {
                #[cfg(debug_assertions)]
                println!("{SHADOWMASK_VERSION}:SHADOWMASK:D ({SHADOWMASK_VER_CODE})");
                #[cfg(not(debug_assertions))]
                println!("{SHADOWMASK_VERSION}:SHADOWMASK:R ({SHADOWMASK_VER_CODE})");
            }
            Version(_) => {
                let mut fd = connect_daemon(RequestCode::CHECK_VERSION, false)?;
                let ver = String::decode(&mut fd)?;
                println!("{ver}");
            }
            VersionCode(_) => {
                let mut fd = connect_daemon(RequestCode::CHECK_VERSION_CODE, false)?;
                let ver = i32::decode(&mut fd)?;
                println!("{ver}");
            }
            List(_) => {
                for name in APPLET_NAMES {
                    println!("{name}");
                }
            }
            RemoveModules(self::RemoveModules { no_reboot }) => {
                let mut fd = connect_daemon(RequestCode::REMOVE_MODULES, false)?;
                let do_reboot = !no_reboot;
                do_reboot.encode(&mut fd)?;
                return Ok(i32::decode(&mut fd)?);
            }
            InstallModule(self::InstallModule { zip }) => {
                install_module(&zip);
            }
            Daemon(_) => {
                let _ = connect_daemon(RequestCode::START_DAEMON, true)?;
            }
            Stop(_) => {
                let mut fd = connect_daemon(RequestCode::STOP_DAEMON, false)?;
                return Ok(i32::decode(&mut fd)?);
            }
            PostFsData(_) => {
                let fd = connect_daemon(RequestCode::POST_FS_DATA, true)?;
                let mut pfd = [PollFd::new(fd.as_fd(), PollFlags::POLLIN)];
                nix::poll::poll(
                    &mut pfd,
                    PollTimeout::try_from(POST_FS_DATA_WAIT_TIME * 1000)?,
                )?;
            }
            Service(_) => {
                let _ = connect_daemon(RequestCode::LATE_START, true)?;
            }
            BootComplete(_) => {
                let _ = connect_daemon(RequestCode::BOOT_COMPLETE, false)?;
            }
            ZygoteRestart(_) => {
                let _ = connect_daemon(RequestCode::ZYGOTE_RESTART, false)?;
            }
            UnlockBlocks(_) => {
                unlock_blocks();
            }
            RestoreCon(_) => {
                restorecon();
            }
            CloneAttr(self::CloneAttr { from, to }) => {
                clone_attr(&from, &to)?;
            }
            CloneFile(self::CloneFile { from, to }) => {
                from.copy_to(&to)?;
            }
            Sqlite(self::Sqlite { sql }) => {
                let mut fd = connect_daemon(RequestCode::SQLITE_CMD, false)?;
                sql.encode(&mut fd)?;
                loop {
                    let line = String::decode(&mut fd)?;
                    if line.is_empty() {
                        return Ok(0);
                    }
                    println!("{line}");
                }
            }
            Path(_) => {
                let tmp = get_shadowmask_tmp();
                if tmp.is_empty() {
                    return Ok(1);
                } else {
                    println!("{tmp}");
                }
            }
            DenyList(self::DenyList { mut args }) => {
                return Ok(denylist_cli(&mut args));
            }
            PreInitDevice(_) => {
                let name = find_preinit_device();
                if name.is_empty() {
                    return Ok(1);
                } else {
                    println!("{name}");
                }
            }
        };
        Ok(0)
    }
}

pub fn shadowmask_main(argc: i32, argv: *mut *mut c_char) -> i32 {
    if argc < 2 {
        print_usage();
        exit(1);
    }
    let mut cmds = CmdArgs::new(argc, argv.cast()).0;
    // We need to manually inject "--" so that all actions can be treated as subcommands
    cmds.insert(1, "--");
    let cli = Cli::from_args(&cmds[..1], &cmds[1..]).on_early_exit(print_usage);
    cli.action.exec().unwrap_or(1)
}
