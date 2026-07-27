#pragma once

#define JAVA_PACKAGE_NAME "com.shadowmask"
#define SECURE_DIR      "/data/adb"
#define MODULEROOT      SECURE_DIR "/modules"
#define DATABIN         SECURE_DIR "/magisk"
#define SHADOWMASKDB        SECURE_DIR "/shadowmask.db"

// tmpfs paths
#define INTLROOT      ".shadowmask"
#define MIRRDIR       INTLROOT "/mirror"
#define PREINITMIRR   INTLROOT "/preinit"
#define DEVICEDIR     INTLROOT "/device"
#define PREINITDEV    DEVICEDIR "/preinit"
#define WORKERDIR     INTLROOT "/worker"
#define BBPATH        INTLROOT "/busybox"
#define ROOTOVL       INTLROOT "/rootdir"
#define SHELLPTS      INTLROOT "/pts"
#define MAIN_CONFIG   INTLROOT "/config"
#define MAIN_SOCKET   DEVICEDIR "/socket"

constexpr const char *applet_names[] = { "su", "resetprop", nullptr };

#define POST_FS_DATA_WAIT_TIME       40
#define POST_FS_DATA_SCRIPT_MAX_TIME 35

// Unconstrained domain the daemon and root processes run in
#define SEPOL_PROC_DOMAIN   "shadowmask"
#define SHADOWMASK_PROC_CON     "u:r:" SEPOL_PROC_DOMAIN ":s0"
// Unconstrained file type that anyone can access
#define SEPOL_FILE_TYPE     "shadowmask_file"
#define SHADOWMASK_FILE_CON     "u:object_r:" SEPOL_FILE_TYPE ":s0"
