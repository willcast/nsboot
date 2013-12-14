/*
	nsboot/boot.c
	functions to handle boot items

	Copyright (C) 2013 Will Castro

	This file is part of nsboot.

	nsboot is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	nsboot is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with nsboot.  If not, see <http://www.gnu.org/licenses/>
*/

#include <sys/stat.h>

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <linux/limits.h>

#include "fb.h"
#include "boot.h"
#include "types.h"
#include "install.h"
#include "browse.h"
#include "lv.h"
#include "log.h"
#include "init.h"
#include "lib.h"


const char *keepable_args[] = { "fbcon=e", "klog=", "klog_len=",
	"nduid=", "boardtype=" , "fb=" };

char *kept_cmdline;

struct boot_item *menu = NULL;
int menu_size = 0;

void parse_proc_cmdline(void) {
	int cmdline_fd;
	int nchars;

	char cmdline[PROC_CMDLINE_MAX], *cur_arg;

	kept_cmdline = malloc(PROC_CMDLINE_MAX);
	kept_cmdline[0] = '\0';

	cmdline_fd = open("/proc/cmdline", O_RDONLY);

	if (cmdline_fd < 0) {
		logperror("1error opening /proc/cmdline");
		return;
	}

	nchars = read(cmdline_fd, cmdline, PROC_CMDLINE_MAX - 1);
	cmdline[nchars] = '\0';

	close(cmdline_fd);

	cur_arg = strtok(cmdline, " ");
	while (cur_arg != NULL) {
		check_keep_arg(cur_arg);
		cur_arg = strtok(NULL, " ");
	}
}

void check_keep_arg(char *arg) {
	char *nduid, serialno[68];
	for (int i = 0; i < array_size(keepable_args); ++i) {
		const char *chk_arg = keepable_args[i];
		if (!strncmp(arg, chk_arg, strlen(chk_arg))) {
			strcat(kept_cmdline, arg);
			strcat(kept_cmdline, " ");
		}
	}
	if (!strncmp(arg, "nduid=", 6)) {
		nduid = arg + 6;
		// "androidboot.serialno=" length is 25 
		// actual serial num. length is 40
		// whole string plus null byte is 66
		// round up one dword, to 68
		if (nduid_to_serialno(nduid, serialno) < 0) {
			logprintf("1error generating android serialno");
			return;
		}
		strcat(kept_cmdline, serialno);
		strcat(kept_cmdline, " ");
	}
}

// read a kexecboot compatible config file
// actual path read is /mnt/$LV/boot/boot.cfg
void read_kb_file_from(char *lv) {
	FILE *cfg_fp;
	char line[128], path[PATH_MAX], *equals;
	struct boot_item *entry;

	strcpy(path, "/mnt/");
	strcat(path, lv);
	strcat(path, "/boot/boot.cfg");

	cfg_fp = fopen(path, "r");
	if (cfg_fp == NULL) {
		logperror("error opening kexecboot cfgfile");
		return;
	}

	while (!(feof(cfg_fp) || ferror(cfg_fp))) {
		if (fgets(line, sizeof(line), cfg_fp) == NULL) {
			fclose(cfg_fp);
			return;
		}

		// ignore comments
		if (line[0] == '#') continue;

		// strip newline char
		equals = strchr(line, '\n');
		if (equals != NULL) *equals = '\0';

		equals = strchr(line, '=');
		// ignore invalid lines
		if (equals == NULL) continue;

		*equals = '\0';
		++equals;

		// 'equals' should now point to the value

		if (!strcmp(line, "LABEL")) {
			entry = add_boot_item(equals);
			entry->cfgdev = strdup(lv);
		} else if (!strcmp(line, "KERNEL")) {
			strcpy(path, "/mnt/");
			strcat(path, lv);
			strcat(path, equals);
			entry->kernel = strdup(path);
 		} else if (!strcmp(line, "INITRD")) {
			strcpy(path, "/mnt/");
			strcat(path, lv);
			strcat(path, equals);
			entry->initrd = strdup(path);
		} else if (!strcmp(line, "APPEND"))
			strcat(entry->append, equals);
		else if (!strcmp(line, "PRIORITY"))
			entry->prio = atoi(equals);
		else
			continue;
	}
	fclose(cfg_fp);
}

void free_boot_items(void) {
	struct boot_item *item;

	for (int i = 0; i < menu_size - 1; ++i) {
		item = menu + i;
		if (item->label != NULL) free(item->label);
		if (item->append != NULL) free(item->append);
		if (item->cfgdev != NULL) free(item->cfgdev);
		if (item->kernel != NULL) free(item->kernel);
		if (item->initrd != NULL) free(item->initrd);
	}
	menu_size = 0;
	free(menu);
}

struct boot_item * add_boot_item(char *item_label) {
	struct boot_item *new_menu, *new_item;
	++menu_size;

	new_menu = realloc(menu, menu_size * sizeof(struct boot_item));
	if (new_menu == NULL) {
		logperror("can't allocate for new boot menu");
		return NULL;
	}
	menu = new_menu;

	new_item = menu + menu_size - 1;
	new_item->label = strdup(item_label);
	new_item->cfgdev = NULL;
	new_item->kernel = NULL;
	new_item->initrd = NULL;
	new_item->append = malloc(PROC_CMDLINE_MAX);
	strcpy(new_item->append, kept_cmdline);
	new_item->prio = 0;

	return new_item;
}

void scan_boot_lvs(void) {
	char filename[PATH_MAX];

	update_lv_lists();

	for (int i = 0; i < lv_count(); ++i) {
		mount_lv(lv_name(i));
		snprintf(filename, sizeof(filename), "/mnt/%s/boot/boot.cfg", lv_name(i));
		if (test_file(filename)) read_kb_file_from(lv_name(i));
	}
}

void boot_kexec(int entry_num) {
	char cmd[1024];
	int code;
	struct boot_item *entry = menu + entry_num;

	if (entry->kernel == NULL) {
		logprintf("2no kernel specified!");
		return;
	}

	mount_lv(entry->cfgdev);
	snprintf(cmd, sizeof(cmd), "mount -o remount,ro %s", entry->cfgdev);
	if (code = WEXITSTATUS(system(cmd)))
		logprintf("1can't remount read only, code %d", code);

	strcpy(cmd, "kexec --load-hardboot --mem-min=0x48000000 --mem-max=0x4FFFFFFF ");
	if (entry->initrd != NULL) {
		strcat(cmd, " --initrd='");
		strcat(cmd, entry->initrd);
		strcat(cmd, "'");
	}
	if (entry->append != NULL) {
		strcat(cmd, " --append='");
		strcat(cmd, entry->append);
		strcat(cmd, "'");
	}
	strcat(cmd, " '");
	strcat(cmd, entry->kernel);
	strcat(cmd, "'");

	logprintf("0loading kexec kernel");
	if (code = WEXITSTATUS(system(cmd))) {
		logprintf("2error loading kernel, code %d", code);
		return;
	}

	umount_lv(entry->cfgdev);

	logprintf("0shutting down services");
	do_shutdown();

	logprintf("0booting kernel");
	if (WEXITSTATUS(system("kexec --exec")))
		logprintf("2error calling kexec");

	logprintf("3kexec failed!");
	exit(1);
}

// Thanks to jcsullins' moboot source for this
// implementation and the following hexval()
int nduid_to_serialno(char *nduid, char *serialno) {
	int nduid_len, i, j, k;
	char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
	uint32_t val;
	int hval, l;

	nduid_len = strlen(nduid);

	if (nduid_len != 40) return -1;

	for (i = 0, k = 0, j = 0, val = 0; i < nduid_len; i++) {
		hval = hexval(nduid[i]);
		if (hval < 0) return -2;

		val |= ((unsigned)hval & 0xF) << (k * 4);

		k++;

		if (k == 5) {
			for (l = 0; l < 4; l++) {
				serialno[j++] = charset[(val & 0x1F)];
				val >>= 5;
			}
			k = 0;
			val = 0;
		}
	}
	serialno[j] = '\0';
	return 0;
}
