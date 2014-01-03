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

#include <ctype.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/limits.h>

#include "boot.h"
#include "file.h"
#include "init.h"
#include "lib.h"
#include "log.h"
#include "lv.h"

const char *keepable_args[] = { "fbcon=e", "klog=", "klog_len=",
	"nduid=", "boardtype=" , "fb=" };

char *kept_cmdline;

boot_item *menu = NULL;
int menu_size = 0;

void parse_proc_cmdline(void) {
	int cmdline_fd;
	int nchars;

	char cmdline[PROC_CMDLINE_MAX];
	char *cur_arg;

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

	strcpy(path, "/mnt/");
	strcat(path, lv);
	strcat(path, "/boot/boot.cfg");

	cfg_fp = fopen(path, "r");
	if (cfg_fp == NULL) {
		logperror("error opening kexecboot cfgfile");
		return;
	}

	boot_item *entry = NULL;
	int line_num = 1;
	while (!(feof(cfg_fp) || ferror(cfg_fp))) {
		char *start;

		if (fgets(line, sizeof(line), cfg_fp) == NULL) {
			fclose(cfg_fp);
			return;
		}

		start = line;
		// ignore leading white space
		while (isspace(*start)) ++start;

		// ignore comments
		if (start[0] == '#') continue;

		// strip newline char
		equals = strchr(start, '\n');
		if (equals != NULL) *equals = '\0';

		equals = strchr(start, '=');
		// ignore invalid lines (without equal sign)
		if (equals == NULL) continue;

		*equals = '\0';
		++equals;

		// 'equals' should now point to the right side value
	
		if (!strcasecmp(start, "LABEL")) {
			entry = add_boot_item(equals);
			entry->cfgdev = strdup(lv);
		} 

		// other directives only make sense after LABEL
		if (entry != NULL) {
			if (!strcasecmp(start, "KERNEL")) {
				char tmp_path[PATH_MAX];

				strncpy(tmp_path, "/mnt/", sizeof(tmp_path));
				strncat(tmp_path, lv, sizeof(tmp_path));
				strncat(tmp_path, equals, sizeof(tmp_path));
				entry->kernel = strdup(tmp_path);
 			} else if (!strcasecmp(start, "INITRD")) {
				char tmp_path[PATH_MAX];

				strncpy(tmp_path, "/mnt/", sizeof(tmp_path));
				strncat(tmp_path, lv, sizeof(tmp_path));
				strncat(tmp_path, equals, sizeof(tmp_path));
				entry->initrd = strdup(tmp_path);
			} else if (!strcasecmp(start, "APPEND")) {
				strncat(entry->append, equals,
					sizeof(entry->append));
			} else if (!strcasecmp(start, "PRIORITY")) {
				char *endptr;
				entry->prio = (int)strtol(equals, &endptr, 10);
				if (*endptr != '\0') {
					logprintf("1non-numeric PRIORITY value, line %d, file %s",
						line_num, path);
					entry->prio = 0;
				}
			} else if (!strcasecmp(start, "LABEL")) {
				// already handled above
				continue;
			} else {
				logprintf("1unknown config directive '%s', line %d, file %s",
					line, line_num, path);
				continue;
			}
		} else {
			logprintf("1LABEL must be set first but wasm't, line %d, file %s",
				line_num, path);
		}

		++line_num;
	}
	fclose(cfg_fp);
}

void free_boot_items(void) {
	boot_item *item;

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

boot_item * add_boot_item(char *item_label) {
	boot_item *new_menu, *new_item;
	++menu_size;

	new_menu = realloc(menu, menu_size * sizeof(boot_item));
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
	boot_item *entry = menu + entry_num;

	if (entry->kernel == NULL) {
		logprintf("2no kernel specified!");
		return;
	}

	mount_lv(entry->cfgdev);
	sysprintf("mount -o remount,ro %s", entry->cfgdev);

	strcpy(cmd, "kexec --load-hardboot --mem-min=0x42000000 ");
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
	system_logged(cmd);

	umount_lv(entry->cfgdev);

	logprintf("0shutting down services");
	do_shutdown();

	logprintf("0booting kernel");
	system_logged("kexec --exec");

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
