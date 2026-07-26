-26 18:18:47.450  6159  6159 F DEBUG   : Build fingerprint: 'samsung/r12sxxx/r12s:15/AP3A.240905.015.A2/S721BXXS7BYH1:user/release-keys'
07-26 18:18:47.450  6159  6159 F DEBUG   : Revision: '24'
07-26 18:18:47.450  6159  6159 F DEBUG   : ABI: 'arm64'
07-26 18:18:47.450  6159  6159 F DEBUG   : Processor: '6'
07-26 18:18:47.450  6159  6159 F DEBUG   : Timestamp: 2026-07-26 18:18:47.226508274+0300
07-26 18:18:47.450  6159  6159 F DEBUG   : Process uptime: 3s
07-26 18:18:47.450  6159  6159 F DEBUG   : Cmdline: com.sec.android.app.launcher
07-26 18:18:47.450  6159  6159 F DEBUG   : pid: 6079, tid: 6079, name: id.app.launcher  >>> com.sec.android.app.launcher <<<
07-26 18:18:47.450  6159  6159 F DEBUG   : uid: 10140
07-26 18:18:47.450  6159  6159 F DEBUG   : signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 0x000000754fa94828
07-26 18:18:47.450  6159  6159 F DEBUG   :     x0  00000000ffffff9c  x1  b400007689853d10  x2  0000000000000000  x3  0000000000000000
07-26 18:18:47.450  6159  6159 F DEBUG   :     x4  0000000000000000  x5  b40000771980f870  x6  0000000000000000  x7  b40000771980f7b0
07-26 18:18:47.450  6159  6159 F DEBUG   :     x8  71c6e3574fc78a65  x9  71c6e3574fc78a65  x10 0000000000000073  x11 000000002cadbf38
07-26 18:18:47.450  6159  6159 F DEBUG   :     x12 0000007ff7752ad8  x13 0000000000000005  x14 000000006f814509  x15 00000000ebad6a89
07-26 18:18:47.450  6159  6159 F DEBUG   :     x16 0000007892fcc198  x17 000000754fa94828  x18 00000078a3f70000  x19 b400007579815850
07-26 18:18:47.450  6159  6159 F DEBUG   :     x20 0000007ff7753fac  x21 b400007689853d10  x22 000000006f287a58  x23 0000000000000000
07-26 18:18:47.450  6159  6159 F DEBUG   :     x24 000000006f287a68  x25 000000006f286e28  x26 000000006f286e20  x27 0000000000000004
07-26 18:18:47.451  6159  6159 F DEBUG   :     x28 0000007ff7753ec0  x29 0000007ff7753e70
07-26 18:18:47.451  6159  6159 F DEBUG   :     lr  000000754b77a33c  sp  0000007ff7753e70  pc  000000754fa94828  pst 0000000060000000
07-26 18:18:47.451  6159  6159 F DEBUG   : 38 total frames
07-26 18:18:47.451  6159  6159 F DEBUG   : backtrace:
07-26 18:18:47.451  6159  6159 F DEBUG   :       #00 pc 000000754fa94828  <unknown>
07-26 18:18:47.451  6159  6159 F DEBUG   :       #01 pc 0000000000024338  /apex/com.android.art/lib64/libjavacore.so (Linux_access(_JNIEnv*, _jobject*, _jstring*, int)+76) (BuildId: 8d85311607fdef69ea901998b2a938b1)
07-26 18:18:47.451  6159  6159 F DEBUG   :       #02 pc 000000000037ef70  /apex/com.android.art/lib64/libart.so (art_quick_generic_jni_trampoline+144) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.451  6159  6159 F DEBUG   :       #03 pc 0000000002316e4c  /memfd:jit-zygote-cache (deleted) (offset 0x2000000) (libcore.io.BlockGuardOs.access+220)
07-26 18:18:47.451  6159  6159 F DEBUG   :       #04 pc 000000000231a3ac  /memfd:jit-zygote-cache (deleted) (offset 0x2000000) (libcore.io.ForwardingOs.access+60)
07-26 18:18:47.451  6159  6159 F DEBUG   :       #05 pc 0000000000780fa0  /apex/com.android.art/lib64/libart.so (nterp_helper+4016) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.451  6159  6159 F DEBUG   :       #06 pc 000000000047c210  /system/framework/framework.jar (android.app.ActivityThread$AndroidOs.access+48)
07-26 18:18:47.451  6159  6159 F DEBUG   :       #07 pc 00000000020afab8  /memfd:jit-zygote-cache (deleted) (offset 0x2000000) (java.io.UnixFileSystem.checkAccess+472)
07-26 18:18:47.451  6159  6159 F DEBUG   :       #08 pc 00000000020841c8  /memfd:jit-zygote-cache (deleted) (offset 0x2000000) (java.io.File.exists+200)
07-26 18:18:47.451  6159  6159 F DEBUG   :       #09 pc 0000000000780fa0  /apex/com.android.art/lib64/libart.so (nterp_helper+4016) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.451  6159  6159 F DEBUG   :       #10 pc 0000000000040080  /apex/com.android.conscrypt/javalib/conscrypt.jar (com.android.org.conscrypt.TrustedCertificateStore$PreloadHolder.shouldUseApex+80)
07-26 18:18:47.451  6159  6159 F DEBUG   :       #11 pc 0000000000780088  /apex/com.android.art/lib64/libart.so (nterp_helper+152) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.451  6159  6159 F DEBUG   :       #12 pc 00000000000401aa  /apex/com.android.conscrypt/javalib/conscrypt.jar (com.android.org.conscrypt.TrustedCertificateStore$PreloadHolder.<clinit>+38)
07-26 18:18:47.451  6159  6159 F DEBUG   :       #13 pc 0000000000368a40  /apex/com.android.art/lib64/libart.so (art_quick_invoke_static_stub+640) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.451  6159  6159 F DEBUG   :       #14 pc 0000000000474524  /apex/com.android.art/lib64/libart.so (art::ClassLinker::InitializeClass(art::Thread*, art::Handle<art::mirror::Class>, bool, bool)+5260) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #15 pc 0000000000577cd0  /apex/com.android.art/lib64/libart.so (art::ClassLinker::EnsureInitialized(art::Thread*, art::Handle<art::mirror::Class>, bool, bool)+160) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #16 pc 000000000035346c  /apex/com.android.art/lib64/libart.so (artQuickToInterpreterBridge+1564) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #17 pc 000000000037f098  /apex/com.android.art/lib64/libart.so (art_quick_to_interpreter_bridge+88) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #18 pc 0000000000780088  /apex/com.android.art/lib64/libart.so (nterp_helper+152) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #19 pc 0000000000040e86  /apex/com.android.conscrypt/javalib/conscrypt.jar (com.android.org.conscrypt.TrustedCertificateStore.setDefaultUserDirectory+14)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #20 pc 0000000000780024  /apex/com.android.art/lib64/libart.so (nterp_helper+52) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #21 pc 000000000048769e  /system/framework/framework.jar (android.app.ActivityThread.main+50)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #22 pc 0000000000368a40  /apex/com.android.art/lib64/libart.so (art_quick_invoke_static_stub+640) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #23 pc 00000000003644f4  /apex/com.android.art/lib64/libart.so (_jobject* art::InvokeMethod<(art::PointerSize)8>(art::ScopedObjectAccessAlreadyRunnable const&, _jobject*, _jobject*, _jobject*, unsigned long)+732) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #24 pc 00000000006c8834  /apex/com.android.art/lib64/libart.so (art::Method_invoke(_JNIEnv*, _jobject*, _jobject*, _jobjectArray*) (.__uniq.165753521025965369065708152063621506277)+32) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #25 pc 000000000037ef70  /apex/com.android.art/lib64/libart.so (art_quick_generic_jni_trampoline+144) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #26 pc 0000000000780fa0  /apex/com.android.art/lib64/libart.so (nterp_helper+4016) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #27 pc 000000000068c246  /system/framework/framework.jar (com.android.internal.os.RuntimeInit$MethodAndArgsCaller.run+18)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #28 pc 0000000000781d64  /apex/com.android.art/lib64/libart.so (nterp_helper+7540) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #29 pc 0000000000691d6c  /system/framework/framework.jar (com.android.internal.os.ZygoteInit.main+616)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #30 pc 0000000000368a40  /apex/com.android.art/lib64/libart.so (art_quick_invoke_static_stub+640) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #31 pc 0000000000353f6c  /apex/com.android.art/lib64/libart.so (art::ArtMethod::Invoke(art::Thread*, unsigned int*, unsigned int, art::JValue*, char const*)+204) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #32 pc 0000000000351f20  /apex/com.android.art/lib64/libart.so (art::JValue art::InvokeWithVarArgs<_jmethodID*>(art::ScopedObjectAccessAlreadyRunnable const&, _jobject*, _jmethodID*, std::__va_list)+512) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #33 pc 000000000073d468  /apex/com.android.art/lib64/libart.so (art::JNI<true>::CallStaticVoidMethodV(_JNIEnv*, _jclass*, _jmethodID*, std::__va_list)+104) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #34 pc 00000000000dd290  /system/lib64/libandroid_runtime.so (_JNIEnv::CallStaticVoidMethod(_jclass*, _jmethodID*, ...)+104) (BuildId: 453f1ad2c6c9e008f60b953e47f8e8cb)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #35 pc 00000000000f27ac  /system/lib64/libandroid_runtime.so (android::AndroidRuntime::start(char const*, android::Vector<android::String8> const&, bool)+924) (BuildId: 453f1ad2c6c9e008f60b953e47f8e8cb)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #36 pc 00000000000045b8  /system/bin/app_process64 (main+1284) (BuildId: 1f5f76a0ce03532e20635d39e8f94827)
07-26 18:18:47.452  6159  6159 F DEBUG   :       #37 pc 0000000000054990  /apex/com.android.runtime/lib64/bionic/libc.so (__libc_init+116) (BuildId: 3549de9a967b5089252c4ca16436800c)
07-26 18:18:47.469  6159  6159 I DEBUG   : dumpstate is already running, so skip
07-26 18:18:47.474   501   501 E tombstoned: Tombstone written to: tombstone_14
07-26 18:18:47.475  6172  6172 I crash_dump64: performing dump of process 6089 (target tid = 6089)
07-26 18:18:47.479  1422  6192 I DropBoxManagerService: add tag=system_app_native_crash isTagEnabled=true flags=0x2
07-26 18:18:47.480  1422  1574 E NativeTombstoneManager: Tombstone has invalid selinux label (u:r:platform_app:s0:c512,c768��), ignoring
07-26 18:18:47.486  1422  6194 W ActivityManager: crash : com.sec.android.app.launcher,10140
07-26 18:18:47.489  1422  1466 D ActivityManager: !@AM_BOOT_PROGRESS , ensureBootCompleted booting:false /enableScreen:false /caller:com.android.server.am.ActivityManagerService.ensureBootCompleted:45 com.android.server.am.ActivityManagerService$UiHandler.handleMessage$com$android$server$am$ActivityManagerService$UiHandler:645
07-26 18:18:47.496  1422  1615 I HqmInfo::c: checkAppError: list is null
07-26 18:18:47.497  1422  1574 E NativeTombstoneManager: Tombstone has invalid selinux label (u:r:platform_app:s0:c512,c768��), ignoring
07-26 18:18:47.508   798   798 I Zygote  : Process 6079 exited due to signal 11 (Segmentation fault)
07-26 18:18:47.613   441   441 I vold    : Vfat : statfs return value : 0
07-26 18:18:47.623   441   441 D voldUtils: BindMount /mnt/media_rw/0228-5F04/.android_secure to /mnt/secure/asec: File exists
07-26 18:18:47.624   441   441 D vold    : Waiting for sdcardfs to spin up...
07-26 18:18:47.638  6195  6195 W sdcard  : Device explicitly enabled sdcardfs
07-26 18:18:47.643   441   441 D vold    : Finished sdcardfs, status: 0
07-26 18:18:47.643   441   441 I vold    : Mounting public fuse volume
07-26 18:18:47.645   893   898 D HeatmapThread: wait_for_battery_event : occured : add@/devices/virtual/bdi/0:51
07-26 18:18:47.645   441   441 I voldUtils: Bind mounting /mnt/runtime/full/0228-5F04 to /mnt/pass_through/0/0228-5F04
07-26 18:18:47.646   441   441 D voldUtils: BindMount /mnt/runtime/full/0228-5F04 to /mnt/pass_through/0/0228-5F04: No such file or directory
07-26 18:18:47.647  1422  1636 I StorageSessionController: On volume mount VolumeInfo{public:179,1}:
07-26 18:18:47.647  1422  1636 I StorageSessionController: type=PUBLIC diskId=disk:179,0 partGuid= mountFlags=VISIBLE_FOR_WRITE mountUserId=0 state=CHECKING
07-26 18:18:47.647  1422  1636 I StorageSessionController:  fsType=vfat fsUuid=0228-5F04 path=/storage/0228-5F04 internalPath=/mnt/media_rw/0228-5F04
07-26 18:18:47.648  1422  1636 I StorageSessionController: Creating and starting session with id: public:179,1
07-26 18:18:47.649  1422  1636 I StorageUserConnection: Binding to the ExternalStorageService for user 0
07-26 18:18:47.651  1422  1636 V GrammaticalInflectionUtils: AttributionSource: AttributionSource { uid = 10269, packageName = null, attributionTag = null, token = android.os.Binder@ba3dba7, deviceId = 0, next = null } does not have READ_SYSTEM_GRAMMATICAL_GENDER permission.
07-26 18:18:47.656  1422  1636 I StorageUserConnection: Bound to the ExternalStorageService for user 0
07-26 18:18:47.679   798   798 D Zygote  : Forked child process 6196
07-26 18:18:47.686  1422  1477 I Watchdog: Interesting Java process com.google.android.providers.media.module started. Pid 6196
07-26 18:18:47.686  1422  1477 I ActivityManager: Start proc 6196:com.google.android.providers.media.module/u0a269 for service {com.google.android.providers.media.module/com.android.providers.media.fuse.ExternalStorageServiceImpl}
07-26 18:18:47.713  6172  6172 F DEBUG   : *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
07-26 18:18:47.713  6172  6172 F DEBUG   : Build fingerprint: 'samsung/r12sxxx/r12s:15/AP3A.240905.015.A2/S721BXXS7BYH1:user/release-keys'
07-26 18:18:47.713  6172  6172 F DEBUG   : Revision: '24'
07-26 18:18:47.713  6172  6172 F DEBUG   : ABI: 'arm64'
07-26 18:18:47.713  6172  6172 F DEBUG   : Processor: '7'
07-26 18:18:47.714  6172  6172 F DEBUG   : Timestamp: 2026-07-26 18:18:47.500214235+0300
07-26 18:18:47.714  6172  6172 F DEBUG   : Process uptime: 3s
07-26 18:18:47.714  6172  6172 F DEBUG   : Cmdline: com.google.android.apps.messaging:rcs
07-26 18:18:47.714  6172  6172 F DEBUG   : pid: 6089, tid: 6089, name: s.messaging:rcs  >>> com.google.android.apps.messaging:rcs <<<
07-26 18:18:47.714  6172  6172 F DEBUG   : uid: 10239
07-26 18:18:47.714  6172  6172 F DEBUG   : signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 0x000000754fa94828
07-26 18:18:47.714  6172  6172 F DEBUG   :     x0  00000000ffffff9c  x1  b400007689858590  x2  0000000000000000  x3  0000000000000000
07-26 18:18:47.714  6172  6172 F DEBUG   :     x4  0000000000000000  x5  b40000771980f870  x6  0000000000000000  x7  b40000771980f7b0
07-26 18:18:47.714  6196  6196 I Magisk  : zygisk64: spoof: all hooks installed for denylist process
07-26 18:18:47.714  6172  6172 F DEBUG   :     x8  a8e70a7d7cd7be18  x9  a8e70a7d7cd7be18  x10 0000000000000073  x11 000000002cadbf38
07-26 18:18:47.714  6172  6172 F DEBUG   :     x12 0000007ff7752ad8  x13 0000000000000005  x14 000000006f814509  x15 00000000ebad6a89
07-26 18:18:47.714  6172  6172 F DEBUG   :     x16 0000007892fcc198  x17 000000754fa94828  x18 00000078a3f70000  x19 b400007579815850
07-26 18:18:47.714  6172  6172 F DEBUG   :     x20 0000007ff7753fac  x21 b400007689858590  x22 000000006f287a58  x23 0000000000000000
07-26 18:18:47.714  6172  6172 F DEBUG   :     x24 000000006f287a68  x25 000000006f286e28  x26 000000006f286e20  x27 0000000000000004
07-26 18:18:47.714  6172  6172 F DEBUG   :     x28 0000007ff7753ec0  x29 0000007ff7753e70
07-26 18:18:47.714  6172  6172 F DEBUG   :     lr  000000754b77a33c  sp  0000007ff7753e70  pc  000000754fa94828  pst 0000000060000000
07-26 18:18:47.714  6172  6172 F DEBUG   : 38 total frames
07-26 18:18:47.714  6172  6172 F DEBUG   : backtrace:
07-26 18:18:47.714  6172  6172 F DEBUG   :       #00 pc 000000754fa94828  <unknown>
07-26 18:18:47.714  6172  6172 F DEBUG   :       #01 pc 0000000000024338  /apex/com.android.art/lib64/libjavacore.so (Linux_access(_JNIEnv*, _jobject*, _jstring*, int)+76) (BuildId: 8d85311607fdef69ea901998b2a938b1)
07-26 18:18:47.714  6172  6172 F DEBUG   :       #02 pc 000000000037ef70  /apex/com.android.art/lib64/libart.so (art_quick_generic_jni_trampoline+144) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.714  6172  6172 F DEBUG   :       #03 pc 0000000002316e4c  /memfd:jit-zygote-cache (deleted) (offset 0x2000000) (libcore.io.BlockGuardOs.access+220)
07-26 18:18:47.714  6172  6172 F DEBUG   :       #04 pc 000000000231a3ac  /memfd:jit-zygote-cache (deleted) (offset 0x2000000) (libcore.io.ForwardingOs.access+60)
07-26 18:18:47.714  6172  6172 F DEBUG   :       #05 pc 0000000000780fa0  /apex/com.android.art/lib64/libart.so (nterp_helper+4016) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.714  6172  6172 F DEBUG   :       #06 pc 000000000047c210  /system/framework/framework.jar (android.app.ActivityThread$AndroidOs.access+48)
07-26 18:18:47.714  6172  6172 F DEBUG   :       #07 pc 00000000020afab8  /memfd:jit-zygote-cache (deleted) (offset 0x2000000) (java.io.UnixFileSystem.checkAccess+472)
07-26 18:18:47.714  6172  6172 F DEBUG   :       #08 pc 00000000020841c8  /memfd:jit-zygote-cache (deleted) (offset 0x2000000) (java.io.File.exists+200)
07-26 18:18:47.714  6172  6172 F DEBUG   :       #09 pc 0000000000780fa0  /apex/com.android.art/lib64/libart.so (nterp_helper+4016) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.714  6172  6172 F DEBUG   :       #10 pc 0000000000040080  /apex/com.android.conscrypt/javalib/conscrypt.jar (com.android.org.conscrypt.TrustedCertificateStore$PreloadHolder.shouldUseApex+80)
07-26 18:18:47.714  6172  6172 F DEBUG   :       #11 pc 0000000000780088  /apex/com.android.art/lib64/libart.so (nterp_helper+152) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.714  6172  6172 F DEBUG   :       #12 pc 00000000000401aa  /apex/com.android.conscrypt/javalib/conscrypt.jar (com.android.org.conscrypt.TrustedCertificateStore$PreloadHolder.<clinit>+38)
07-26 18:18:47.714  6172  6172 F DEBUG   :       #13 pc 0000000000368a40  /apex/com.android.art/lib64/libart.so (art_quick_invoke_static_stub+640) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.714  6172  6172 F DEBUG   :       #14 pc 0000000000474524  /apex/com.android.art/lib64/libart.so (art::ClassLinker::InitializeClass(art::Thread*, art::Handle<art::mirror::Class>, bool, bool)+5260) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.715  6172  6172 F DEBUG   :       #15 pc 0000000000577cd0  /apex/com.android.art/lib64/libart.so (art::ClassLinker::EnsureInitialized(art::Thread*, art::Handle<art::mirror::Class>, bool, bool)+160) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.715  6172  6172 F DEBUG   :       #16 pc 000000000035346c  /apex/com.android.art/lib64/libart.so (artQuickToInterpreterBridge+1564) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.715  6172  6172 F DEBUG   :       #17 pc 000000000037f098  /apex/com.android.art/lib64/libart.so (art_quick_to_interpreter_bridge+88) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.715  6172  6172 F DEBUG   :       #18 pc 0000000000780088  /apex/com.android.art/lib64/libart.so (nterp_helper+152) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.715  6172  6172 F DEBUG   :       #19 pc 0000000000040e86  /apex/com.android.conscrypt/javalib/conscrypt.jar (com.android.org.conscrypt.TrustedCertificateStore.setDefaultUserDirectory+14)
07-26 18:18:47.715  6172  6172 F DEBUG   :       #20 pc 0000000000780024  /apex/com.android.art/lib64/libart.so (nterp_helper+52) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.715  6172  6172 F DEBUG   :       #21 pc 000000000048769e  /system/framework/framework.jar (android.app.ActivityThread.main+50)
07-26 18:18:47.715  6172  6172 F DEBUG   :       #22 pc 0000000000368a40  /apex/com.android.art/lib64/libart.so (art_quick_invoke_static_stub+640) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.715  6172  6172 F DEBUG   :       #23 pc 00000000003644f4  /apex/com.android.art/lib64/libart.so (_jobject* art::InvokeMethod<(art::PointerSize)8>(art::ScopedObjectAccessAlreadyRunnable const&, _jobject*, _jobject*, _jobject*, unsigned long)+732) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.715  6172  6172 F DEBUG   :       #24 pc 00000000006c8834  /apex/com.android.art/lib64/libart.so (art::Method_invoke(_JNIEnv*, _jobject*, _jobject*, _jobjectArray*) (.__uniq.165753521025965369065708152063621506277)+32) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.715  6172  6172 F DEBUG   :       #25 pc 000000000037ef70  /apex/com.android.art/lib64/libart.so (art_quick_generic_jni_trampoline+144) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.715  6172  6172 F DEBUG   :       #26 pc 0000000000780fa0  /apex/com.android.art/lib64/libart.so (nterp_helper+4016) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.716  6172  6172 F DEBUG   :       #27 pc 000000000068c246  /system/framework/framework.jar (com.android.internal.os.RuntimeInit$MethodAndArgsCaller.run+18)
07-26 18:18:47.716  6172  6172 F DEBUG   :       #28 pc 0000000000781d64  /apex/com.android.art/lib64/libart.so (nterp_helper+7540) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.716  6172  6172 F DEBUG   :       #29 pc 0000000000691d6c  /system/framework/framework.jar (com.android.internal.os.ZygoteInit.main+616)
07-26 18:18:47.716  6172  6172 F DEBUG   :       #30 pc 0000000000368a40  /apex/com.android.art/lib64/libart.so (art_quick_invoke_static_stub+640) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.716  6172  6172 F DEBUG   :       #31 pc 0000000000353f6c  /apex/com.android.art/lib64/libart.so (art::ArtMethod::Invoke(art::Thread*, unsigned int*, unsigned int, art::JValue*, char const*)+204) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.716  6172  6172 F DEBUG   :       #32 pc 0000000000351f20  /apex/com.android.art/lib64/libart.so (art::JValue art::InvokeWithVarArgs<_jmethodID*>(art::ScopedObjectAccessAlreadyRunnable const&, _jobject*, _jmethodID*, std::__va_list)+512) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.716  6172  6172 F DEBUG   :       #33 pc 000000000073d468  /apex/com.android.art/lib64/libart.so (art::JNI<true>::CallStaticVoidMethodV(_JNIEnv*, _jclass*, _jmethodID*, std::__va_list)+104) (BuildId: a0856b3dbc826e8cee9d66738ac739d8)
07-26 18:18:47.716  6172  6172 F DEBUG   :       #34 pc 00000000000dd290  /system/lib64/libandroid_runtime.so (_JNIEnv::CallStaticVoidMethod(_jclass*, _jmethodID*, ...)+104) (BuildId: 453f1ad2c6c9e008f60b953e47f8e8cb)
07-26 18:18:47.716  6172  6172 F DEBUG   :       #35 pc 00000000000f27ac  /system/lib64/libandroid_runtime.so (android::AndroidRuntime::start(char const*, android::Vector<android::String8> const&, bool)+924) (BuildId: 453f1ad2c6c9e008f60b953e47f8e8cb)
07-26 18:18:47.716  6172  6172 F DEBUG   :       #36 pc 00000000000045b8  /system/bin/app_process64 (main+1284) (BuildId: 1f5f76a0ce03532e20635d39e8f94827)
07-26 18:18:47.716  6172  6172 F DEBUG   :       #37 pc 0000000000054990  /apex/com.android.runtime/lib64/bionic/libc.so (__libc_init+116) (BuildId: 3549de9a967b5089252c4ca16436800c)
07-26 18:18:47.725  6190  6190 E dmabuf_dump: Unable to access: /sys/kernel/dmabuf/buffers: No such file or directory
07-26 18:18:47.729  6196  6196 I rs.media.module: Using CollectorTypeCMC GC.
07-26 18:18:47.733  6202  6202 E dmabuf_dump: Unable to access: /sys/kernel/dmabuf/buffers: No such file or directory
07-26 18:18:47.735  6196  6196 E rs.media.module: Not starting debugger since process cannot load the jdwp agent.
07-26 18:18:47.738   501   501 E tombstoned: Tombstone written to: tombstone_15
07-26 18:18:47.743  1422  6208 W ActivityManager: crash : com.google.android.apps.messaging,10239
07-26 18:18:47.743  1422  6206 I DropBoxManagerService: add tag=system_app_native_crash isTagEnabled=true flags=0x2
07-26 18:18:47.744  1422  1466 D ActivityManager: !@AM_BOOT_PROGRESS , ensureBootCompleted booting:false /enableScreen:false /caller:com.android.server.am.ActivityManagerService.ensureBootCompleted:45 com.android.server.am.ActivityManagerService$UiHandler.handleMessage$com$android$server$am$ActivityManagerService$UiHandler:645
07-26 18:18:47.754  1422  1615 I HqmInfo::c: checkAppError: list is null
07-26 18:18:47.762  1422  1574 E NativeTombstoneManager: Tombstone has invalid selinux label (u:r:priv_app:s0:c512,c768��), ignoring
07-26 18:18:47.768  1422  1574 E NativeTombstoneManager: Tombstone has invalid selinux label (u:r:priv_app:s0:c512,c768��), ignoring
07-26 18:18:47.778  2928  2928 I adbd    : Remote process closed the socket (on MSG_PEEK)
07-26 18:18:47.779   798   798 I Zygote  : Process 6089 exited due to signal 11 (Segmentation fault)
07-26 18:18:48.582  1422  1476 W ActivityManager: Process ProcessRecord{72c5b59 5979:com.samsung.android.sm.provider/1000} failed to attach
07-26 18:18:48.583  1422  1476 W ActivityManager: Unable to launch app com.samsung.android.lool/1000 for provider com.samsung.android.sm: launching app became null
07-26 18:18:48.583  1422  1476 E ActivityManager: Do not bringing down SystemUI services : ServiceRecord{fecf1cf u0 com.android.systemui/.wallpapers.ImageWallpaper c:android}
07-26 18:18:48.583  1422  1485 D ActivityThread: holder's provider is null
07-26 18:18:48.583  1422  1485 E ActivityThread: Failed to find provider info for com.samsung.android.sm
07-26 18:18:48.583  1422  1485 I IAFDDBManager: in update,  mSMDBInitReTryCnt=33
07-26 18:18:48.583  1422  1485 D IAFDDBManager: syncDBType(): mCurDBIndex=0, curDBVer=5
07-26 18:18:48.584  1422  1485 D IAFDDBManager: syncDBType(): mCurDBIndex=0, curDBVer=5
07-26 18:18:48.584  1422  1476 E ActivityManager: Do not bringing down SystemUI services : ServiceRecord{b1ce65c u0 com.android.systemui/.SystemUIService c:android}
07-26 18:18:48.584  1422  1476 E ActivityManager: Do not bringing down SystemUI services : ServiceRecord{7c00c48 u0 com.android.systemui/.keyguard.KeyguardService c:android}
07-26 18:18:48.585  1422  1476 W Process : Unable to open /proc/5979/status
07-26 18:18:48.585  1422  1476 W ActivityManager: Not TGL 5979:com.android.server.am.ProcessRecord.killLocked:7 com.android.server.am.ActivityManagerService.handleProcessStartOrKillTimeoutLocked:226
07-26 18:18:48.603   798   798 D Zygote  : Forked child process 6211
07-26 18:18:48.605  1422  1477 I ActivityManager: Start proc 6211:com.samsung.android.sm.provider/1000 for content provider {com.samsung.android.lool/com.samsung.android.sm.database.SmProvider}
07-26 18:18:48.633  6211  6211 I Magisk  : zygisk64: spoof: all hooks installed for denylist process
07-26 18:18:48.650  6211  6211 I oid.sm.provider: Using CollectorTypeCMC GC.
07-26 18:18:48.653  6211  6211 E oid.sm.provider: Not starting debugger since process cannot load the jdwp agent.
07-26 18:18:48.669   902   902 I SurfaceFlinger: SFWD update time=211387697426
^C
