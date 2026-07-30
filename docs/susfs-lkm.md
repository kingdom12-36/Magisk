# SUSFS Kernel Module (LKM) — shadowmask_sus.ko

ShadowMask implements SUSFS-compatible hiding as a **Loadable Kernel Module**,
giving ~90% of real SUSFS functionality on any kernel where:

- `CONFIG_MODULES=y`
- `CONFIG_MODULE_SIG` **not set** (no signature enforcement)
- `CONFIG_KALLSYMS_ALL=y`

No kernel source patching or recompilation required.

---

## How It Works

```
ksu_susfs add_sus_path /data/adb
   │
   ▼
syscall(SYS_reboot, 0xDEADBEEF, 0xFAFAFAFA, CMD_SUSFS_ADD_SUS_PATH, &info)
   │
   ▼  ◄── shadowmask_sus.ko intercepts here (syscall table hook)
kernel list: sm_sus_paths += "/data/adb"
   │
   ▼  ◄── proc_filter.c hooks /proc/pid/maps + mountinfo seq_file
app reads /proc/self/maps → "/data/adb" entries removed
```

The module intercepts the same `SYS_reboot` syscall that kernel-compiled SUSFS
uses. The existing `ksu_susfs` userspace tool works **unchanged**.

---

## Feature Coverage

| SUSFS Feature | Kernel-patch | LKM (this) | Method |
|---|---|---|---|
| `sus_path` | ✅ | ✅ ~90% | syscall hook + path matching |
| `sus_mount` | ✅ | ✅ ~95% | mountinfo seq_file filter |
| `sus_kstat` | ✅ | ✅ ~85% | kstat struct overlay |
| `sus_map` | ✅ | ✅ ~90% | maps seq_file filter |
| `open_redirect` | ✅ | ✅ ~80% | redirect list in kernel |
| `set_uname` | ✅ | ✅ 100% | direct `init_uts_ns` write |
| `set_cmdline` | ✅ | ✅ 100% | proc cmdline override |
| `enable_log` | ✅ | ✅ 100% | module log flag |

---

## Building

The `.ko` must be compiled **against the kernel tree** — it is not built via NDK.

### In Ocin4everKernel CI (recommended)

Add to `.github/workflows/build.yml` after the kernel is built:

```yaml
- name: Build ShadowMask SUSFS kmod
  run: |
    SM_ROOT=${{ github.workspace }}/ShadowMask
    KDIR=${{ github.workspace }}/out/arch/arm64/boot   # adjust to your out path
    KOUT=$(dirname $(find ${{ github.workspace }}/out -name "Module.symvers" | head -1))
    
    make -C $KOUT \
         M=$SM_ROOT/native/src/kmod \
         ARCH=arm64 \
         CROSS_COMPILE=aarch64-linux-android- \
         modules
    
    cp $SM_ROOT/native/src/kmod/shadowmask_sus.ko \
       ${{ github.workspace }}/out/target/shadowmask_sus.ko

- name: Package shadowmask_sus.ko with ShadowMask assets
  run: |
    cp out/target/shadowmask_sus.ko ShadowMask/assets/shadowmask_sus.ko
```

### Manual build

```bash
# After building Ocin4everKernel:
KERNEL_OUT=/path/to/kernel/out   # dir containing Module.symvers

make -C $KERNEL_OUT \
     M=$(pwd)/native/src/kmod \
     ARCH=arm64 \
     CROSS_COMPILE=aarch64-linux-android- \
     modules

# Output: native/src/kmod/shadowmask_sus.ko
```

---

## Integration into ShadowMask

At boot, `bootstages.rs` calls `load_susfs_kmod()`:

```
/data/adb/shadowmask/shadowmask_sus.ko   ← deployed here at install
         │
         ▼ insmod (via load_susfs_kmod())
/sys/module/shadowmask_sus/              ← marker that module is loaded
         │
         ▼ ksu_susfs tool can now use all SUSFS commands
```

If the `.ko` is not present (e.g., kernel doesn't support modules),
`load_susfs_kmod()` logs a message and returns — nothing breaks.

---

## Security Notes

- The module runs in kernel space with full privileges
- SELinux context: `shadowmask` domain (inherits from ShadowMask daemon)
- `CONFIG_STRICT_MODULE_RWX=y` is respected — only data pointers are modified,
  not code pages
- The module is removed cleanly on unload via `rmmod shadowmask_sus`
- All lists are freed on module exit

---

## Source Files

```
native/src/kmod/
├── shadowmask_sus.h    — structs, ABI constants, extern declarations
├── shadowmask_sus.c    — module init/exit, global list state
├── syscall_hook.c      — SYS_reboot interception + SUSFS command dispatch
├── proc_filter.c       — /proc/pid/maps and mountinfo seq_file filtering
└── Makefile            — kernel module build rules
```
