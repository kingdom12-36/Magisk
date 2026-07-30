#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <susfs_defs.h>
#include <susfs_utils.h>
#include "open_redirect.h"

#define CMD_SUSFS_ADD_OPEN_REDIRECT 0x555c0

enum UID_SCHEME {
	UID_NON_APP_PROC = 0,
	UID_ROOT_PROC_EXCEPT_SU_PROC,
	UID_NON_SU_PROC,
	UID_UMOUNTED_APP_PROC,
	UID_UMOUNTED_PROC,
};

struct st_susfs_open_redirect {
	char  target_pathname[SUSFS_MAX_LEN_PATHNAME];
	char  redirected_pathname[SUSFS_MAX_LEN_PATHNAME];
	int   uid_scheme;
	int   err;
};

void open_redirect_print_help(void) {
	log("    add_open_redirect </target/path> </redirected/path> <uid_scheme>\n");
	log("      |--> Redirect the target path to be opened with user defined path and pre-defined uid scheme\n");
	log("      |--> <uid_scheme>\n");
	log("             |--> 0: Effective for non-app processes (uid < 10000)\n");
	log("             |--> 1: Effective for non-su processes of which uid is 0\n");
	log("             |--> 2: Effective for non-su processes\n");
	log("             |--> 3: Effective for umounted app processes (uid >= 10000)\n");
	log("             |--> 4: Effective for all umounted processes\n");
	log("\n");
}

static void print_help(void) {
	print_help_banner();
	open_redirect_print_help();
}

int add_open_redirect(int argc, char *argv[]) {
	struct st_susfs_open_redirect info = {0};
	char resolved_target[PATH_MAX];
	char resolved_redirected[PATH_MAX];

	if (argc != 5) {
		print_help();
		return -EINVAL;
	}

	if (*argv[2] == '\0') {
		log("[-] argv[2] is empty\n");
		return -EINVAL;
	}

	if (*argv[3] == '\0') {
		log("[-] argv[3] is empty\n");
		return -EINVAL;
	}

	if (!isNumeric(argv[4])) {
		log("[-] argv[4] is not numeric\n");
		return -EINVAL;
	}

	if (!realpath(argv[2], resolved_target)) {
		log("[-] failed to get realpath from target: %s\n", argv[2]);
		return errno;
	}

	if (!realpath(argv[3], resolved_redirected)) {
		log("[-] failed to get realpath from redirected: %s\n", argv[3]);
		return errno;
	}

	strncpy(info.target_pathname, resolved_target, SUSFS_MAX_LEN_PATHNAME - 1);
	strncpy(info.redirected_pathname, resolved_redirected, SUSFS_MAX_LEN_PATHNAME - 1);
	info.uid_scheme = atoi(argv[4]);
	info.err = ERR_CMD_NOT_SUPPORTED;
	syscall(SYS_reboot, KSU_INSTALL_MAGIC1, SUSFS_MAGIC, CMD_SUSFS_ADD_OPEN_REDIRECT, &info);
	PRT_MSG_IF_CMD_NOT_SUPPORTED(info.err, CMD_SUSFS_ADD_OPEN_REDIRECT);
	return info.err;
}
