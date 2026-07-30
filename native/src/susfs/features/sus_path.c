#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <errno.h>
#include <susfs_defs.h>
#include <susfs_utils.h>
#include "sus_path.h"

#define CMD_SUSFS_ADD_SUS_PATH      0x55550
#define CMD_SUSFS_ADD_SUS_PATH_LOOP 0x55553

struct st_susfs_sus_path {
	char  target_pathname[SUSFS_MAX_LEN_PATHNAME];
	int   err;
};

void sus_path_print_help(void) {
	log("    add_sus_path </path/of/file_or_directory>\n");
	log("      |--> Added path and all its sub-paths will be hidden for umounted app process\n");
	log("      * Important Notes *\n");
	log("      - Only effective for umounted process with uid >= 10000\n");
	log("\n");
	log("    add_sus_path_loop </path/of/file_or_directory>\n");
	log("      |--> Same as add_sus_path but re-flags the path for each new spawned umounted app\n");
	log("      * Important Notes *\n");
	log("      - Only effective for umounted process with uid >= 10000\n");
	log("\n");
}

static void print_help(void) {
	print_help_banner();
	sus_path_print_help();
}

int add_sus_path(int argc, char *argv[]) {
	struct st_susfs_sus_path info = {0};
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

	strncpy(info.target_pathname, resolved_pathname, SUSFS_MAX_LEN_PATHNAME - 1);
	info.err = ERR_CMD_NOT_SUPPORTED;
	syscall(SYS_reboot, KSU_INSTALL_MAGIC1, SUSFS_MAGIC, CMD_SUSFS_ADD_SUS_PATH, &info);
	PRT_MSG_IF_CMD_NOT_SUPPORTED(info.err, CMD_SUSFS_ADD_SUS_PATH);
	return info.err;
}

int add_sus_path_loop(int argc, char *argv[]) {
	struct st_susfs_sus_path info = {0};

	if (argc != 3) {
		print_help();
		return -EINVAL;
	}

	if (*argv[2] == '\0') {
		log("[-] argv[2] is empty\n");
		return -EINVAL;
	}

	strncpy(info.target_pathname, argv[2], SUSFS_MAX_LEN_PATHNAME - 1);
	info.err = ERR_CMD_NOT_SUPPORTED;
	syscall(SYS_reboot, KSU_INSTALL_MAGIC1, SUSFS_MAGIC, CMD_SUSFS_ADD_SUS_PATH_LOOP, &info);
	PRT_MSG_IF_CMD_NOT_SUPPORTED(info.err, CMD_SUSFS_ADD_SUS_PATH_LOOP);
	return info.err;
}
