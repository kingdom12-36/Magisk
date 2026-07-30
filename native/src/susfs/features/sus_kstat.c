#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <errno.h>
#include <susfs_defs.h>
#include <susfs_utils.h>
#include "sus_kstat.h"

#define CMD_SUSFS_ADD_SUS_KSTAT           0x55570
#define CMD_SUSFS_UPDATE_SUS_KSTAT        0x55571
#define CMD_SUSFS_ADD_SUS_KSTAT_STATICALLY 0x55572

#define KSTAT_SPOOF_INO          (1 << 0)
#define KSTAT_SPOOF_DEV          (1 << 1)
#define KSTAT_SPOOF_NLINK        (1 << 2)
#define KSTAT_SPOOF_SIZE         (1 << 3)
#define KSTAT_SPOOF_ATIME_TV_SEC (1 << 4)
#define KSTAT_SPOOF_ATIME_TV_NSEC (1 << 5)
#define KSTAT_SPOOF_MTIME_TV_SEC (1 << 6)
#define KSTAT_SPOOF_MTIME_TV_NSEC (1 << 7)
#define KSTAT_SPOOF_CTIME_TV_SEC (1 << 8)
#define KSTAT_SPOOF_CTIME_TV_NSEC (1 << 9)
#define KSTAT_SPOOF_BLOCKS       (1 << 10)
#define KSTAT_SPOOF_BLKSIZE      (1 << 11)
#define KSTAT_AUTO_SPOOF_FULL_CLONE (1 << 30)

struct st_susfs_sus_kstat {
	int        is_statically;
	unsigned long target_ino;
	char       target_pathname[SUSFS_MAX_LEN_PATHNAME];
	unsigned long spoofed_ino;
	unsigned long spoofed_dev;
	unsigned int  spoofed_nlink;
	long long  spoofed_size;
	long       spoofed_atime_tv_sec;
	unsigned long spoofed_atime_tv_nsec;
	long       spoofed_mtime_tv_sec;
	unsigned long spoofed_mtime_tv_nsec;
	long       spoofed_ctime_tv_sec;
	unsigned long spoofed_ctime_tv_nsec;
	long long  spoofed_blocks;
	long       spoofed_blksize;
	int        flags;
	int        err;
};

static void copy_from_stat_to_sus_kstat(struct st_susfs_sus_kstat *info, struct stat *sb) {
	info->spoofed_ino           = sb->st_ino;
	info->spoofed_dev           = sb->st_dev;
	info->spoofed_nlink         = sb->st_nlink;
	info->spoofed_size          = sb->st_size;
	info->spoofed_atime_tv_sec  = sb->st_atim.tv_sec;
	info->spoofed_atime_tv_nsec = sb->st_atim.tv_nsec;
	info->spoofed_mtime_tv_sec  = sb->st_mtim.tv_sec;
	info->spoofed_mtime_tv_nsec = sb->st_mtim.tv_nsec;
	info->spoofed_ctime_tv_sec  = sb->st_ctim.tv_sec;
	info->spoofed_ctime_tv_nsec = sb->st_ctim.tv_nsec;
	info->spoofed_blocks        = sb->st_blocks;
	info->spoofed_blksize       = sb->st_blksize;
	info->flags |= KSTAT_SPOOF_INO | KSTAT_SPOOF_DEV | KSTAT_SPOOF_NLINK |
	               KSTAT_SPOOF_SIZE | KSTAT_SPOOF_ATIME_TV_SEC | KSTAT_SPOOF_ATIME_TV_NSEC |
	               KSTAT_SPOOF_MTIME_TV_SEC | KSTAT_SPOOF_MTIME_TV_NSEC |
	               KSTAT_SPOOF_CTIME_TV_SEC | KSTAT_SPOOF_CTIME_TV_NSEC |
	               KSTAT_SPOOF_BLOCKS | KSTAT_SPOOF_BLKSIZE;
}

void sus_kstat_print_help(void) {
	log("    add_sus_kstat </path/to/file_or_directory>\n");
	log("      |--> Store the current stat of the target path to kernel and spoof it for umounted app process\n");
	log("\n");
	log("    update_sus_kstat </path/to/file_or_directory>\n");
	log("      |--> Update the spoofed stat with the current stat of the target path\n");
	log("\n");
	log("    add_sus_kstat_statically </path/to/file_or_directory>\n");
	log("      |--> Same as add_sus_kstat but store stat by inode number statically (faster lookup)\n");
	log("\n");
}

static void print_help(void) {
	print_help_banner();
	sus_kstat_print_help();
}

int add_sus_kstat(int argc, char *argv[]) {
	struct st_susfs_sus_kstat info = {0};
	struct stat sb;
	char resolved_pathname[PATH_MAX];

	if (argc != 3) {
		print_help();
		return -EINVAL;
	}

	if (*argv[2] == '\0') {
		log("[-] argv[2] is empty\n");
		return -EINVAL;
	}

	if (!realpath(argv[2], resolved_pathname)) {
		log("[-] failed to get realpath from path: %s\n", argv[2]);
		return errno;
	}

	info.err = get_file_stat(resolved_pathname, &sb);
	if (info.err) {
		log("[-] failed to get stat from path: '%s'\n", resolved_pathname);
		return info.err;
	}

	strncpy(info.target_pathname, resolved_pathname, SUSFS_MAX_LEN_PATHNAME - 1);
	info.is_statically = false;
	info.target_ino    = sb.st_ino;
	copy_from_stat_to_sus_kstat(&info, &sb);
	info.err = ERR_CMD_NOT_SUPPORTED;
	syscall(SYS_reboot, KSU_INSTALL_MAGIC1, SUSFS_MAGIC, CMD_SUSFS_ADD_SUS_KSTAT, &info);
	PRT_MSG_IF_CMD_NOT_SUPPORTED(info.err, CMD_SUSFS_ADD_SUS_KSTAT);
	return info.err;
}

int update_sus_kstat(int argc, char *argv[]) {
	struct st_susfs_sus_kstat info = {0};
	struct stat sb;
	char resolved_pathname[PATH_MAX];

	if (argc != 3) {
		print_help();
		return -EINVAL;
	}

	if (*argv[2] == '\0') {
		log("[-] argv[2] is empty\n");
		return -EINVAL;
	}

	if (!realpath(argv[2], resolved_pathname)) {
		log("[-] failed to get realpath from path: %s\n", argv[2]);
		return errno;
	}

	info.err = get_file_stat(resolved_pathname, &sb);
	if (info.err) {
		log("[-] failed to get stat from path: '%s'\n", resolved_pathname);
		return info.err;
	}

	strncpy(info.target_pathname, resolved_pathname, SUSFS_MAX_LEN_PATHNAME - 1);
	info.is_statically = false;
	info.target_ino    = sb.st_ino;
	copy_from_stat_to_sus_kstat(&info, &sb);
	info.err = ERR_CMD_NOT_SUPPORTED;
	syscall(SYS_reboot, KSU_INSTALL_MAGIC1, SUSFS_MAGIC, CMD_SUSFS_UPDATE_SUS_KSTAT, &info);
	PRT_MSG_IF_CMD_NOT_SUPPORTED(info.err, CMD_SUSFS_UPDATE_SUS_KSTAT);
	return info.err;
}

int add_sus_kstat_statically(int argc, char *argv[]) {
	struct st_susfs_sus_kstat info = {0};
	struct stat sb;
	char resolved_pathname[PATH_MAX];

	if (argc != 3) {
		print_help();
		return -EINVAL;
	}

	if (*argv[2] == '\0') {
		log("[-] argv[2] is empty\n");
		return -EINVAL;
	}

	if (!realpath(argv[2], resolved_pathname)) {
		log("[-] failed to get realpath from path: %s\n", argv[2]);
		return errno;
	}

	info.err = get_file_stat(resolved_pathname, &sb);
	if (info.err) {
		log("[-] failed to get stat from path: '%s'\n", resolved_pathname);
		return info.err;
	}

	strncpy(info.target_pathname, resolved_pathname, SUSFS_MAX_LEN_PATHNAME - 1);
	info.is_statically = true;
	info.target_ino    = sb.st_ino;
	copy_from_stat_to_sus_kstat(&info, &sb);
	info.err = ERR_CMD_NOT_SUPPORTED;
	syscall(SYS_reboot, KSU_INSTALL_MAGIC1, SUSFS_MAGIC, CMD_SUSFS_ADD_SUS_KSTAT_STATICALLY, &info);
	PRT_MSG_IF_CMD_NOT_SUPPORTED(info.err, CMD_SUSFS_ADD_SUS_KSTAT_STATICALLY);
	return info.err;
}

int update_sus_kstat_full_clone(int argc, char *argv[]) {
	struct st_susfs_sus_kstat info = {0};
	struct stat sb;
	char resolved_pathname[PATH_MAX];

	if (argc != 3) {
		print_help();
		return -EINVAL;
	}

	if (*argv[2] == '\0') {
		log("[-] argv[2] is empty\n");
		return -EINVAL;
	}

	if (!realpath(argv[2], resolved_pathname)) {
		log("[-] failed to get realpath from path: %s\n", argv[2]);
		return errno;
	}

	info.err = get_file_stat(resolved_pathname, &sb);
	if (info.err) {
		log("[-] failed to get stat from path: '%s'\n", resolved_pathname);
		return info.err;
	}

	strncpy(info.target_pathname, resolved_pathname, SUSFS_MAX_LEN_PATHNAME - 1);
	info.is_statically = false;
	info.target_ino    = sb.st_ino;
	info.flags        |= KSTAT_AUTO_SPOOF_FULL_CLONE;
	copy_from_stat_to_sus_kstat(&info, &sb);
	info.err = ERR_CMD_NOT_SUPPORTED;
	syscall(SYS_reboot, KSU_INSTALL_MAGIC1, SUSFS_MAGIC, CMD_SUSFS_UPDATE_SUS_KSTAT, &info);
	PRT_MSG_IF_CMD_NOT_SUPPORTED(info.err, CMD_SUSFS_UPDATE_SUS_KSTAT);
	return info.err;
}
