/*
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

#include "browse.h"
#include "fb.h"
#include "install.h"
#include "screens.h"
#include "touch.h"
#include "boot.h"
#include "input.h"
#include "lv.h"
#include "lvset.h"
#include "lib.h"

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/reboot.h>

#include "log.h"
#include "init.h"

extern int sel;

void main_menu(void) {
	int ts_x, ts_y;
	int quit = 0;
	char *file1;

	while (!quit) {
		clear_screen();
		text("main menu", 8, 8, 6, 6, 0xFFFFFFFF, 0xFF000000);

		text_box("quit", 16,128, 160,88, 4, 0xFFFFFFFF,0xFFFF0000,0xFFFFFFFF);
		text_box("power off", 192,128, 340,88, 4, 0xFFFFFFFF,0xFFFF0000,0xFFFFFFFF);
		text_box("reboot", 548,128, 232,88, 4, 0xFFFFFFFF,0xFFFF0000,0xFFFFFFFF);
		text_box("boot menu", 16,224, 336,88, 4, 0xFFFFFFFF,0xFF008000,0xFFFFFFFF);
		text_box("installer menu", 368,224, 520,88, 4, 0xFFFFFFFF,0xFF0000FF,0xFFFFFFFF);
		text_box("utility menu", 16,320, 448,88, 4, 0xFFFFFFFF, 0xFF606060, 0xFFFFFFFF);
		text_box("file browser", 496,320, 448,88, 4, 0xFFFFFFFF,0xFF606060, 0xFFFFFFFF);
		text_box("information", 16,416, 412,88, 4, 0xFFFFFFFF, 0xFF606060, 0xFFFFFFFF);

		ts_read(&ts_x, &ts_y);

		if (in_box(16, 128, 160, 88)) {
			logprintf("0have a nice day");
			quit = 1;
		} else if (in_box(192, 128, 340, 88) && confirm("power off")) {
			logprintf("1powering off...");
			sync();
			reboot(RB_POWER_OFF);
		} else if (in_box(548, 128, 232, 88) && confirm("reboot")) {
			logprintf("1rebooting...");
			sync();
			reboot(RB_AUTOBOOT);
		} else if (in_box(16, 224, 336, 88))
			boot_menu();
		else if (in_box(368, 224, 520, 88))
			installer_menu();
		else if (in_box(16, 320, 448, 88))
			util_menu();
		else if (in_box(480, 320, 448, 88)) {
			file1 =	select_file(ANY, NULL);
			if (file1 == NULL) continue;
			task_menu(file1);
		} else if (in_box(16, 416, 412, 88))
     			info_screen();
	}
}

void installer_menu(void) {
	int ts_x, ts_y;
	int ret = 0;
	char *filename, *lv, *lv_set;
	int flags;
	long size;

	while (!ret) {
		clear_screen();
		text("installer menu", 8, 8, 6, 6, 0xFF0000FF, 0xFF000000);

		text_box("back", 16,128, 124,70, 3, 0xFFFFFFFF,0xFF606060,0xFFFFFFFF);

		text(".zip file to set:", 16,214, 2,2, 0xFF00FFFF, 0xFF000000);
		text_box("default", 374,214, 142,52, 2,
			0xFFFFFFFF,0xFF00FFFF,0xFFFFFFFF);
		text_box("existing", 534,214, 160,52, 2,
			0xFFFFFFFF,0xFF00FFFF,0xFFFFFFFF);
		text_box("custom", 710,214, 124,52, 2,
			0xFFFFFFFF,0xFF00FFFF,0xFFFFFFFF);

		text(".tar.gz file to LV:", 16,282, 2,2, 0xFFFFFF00, 0xFF000000);
		text_box("default", 374,282, 142,52, 2,
			0xFFFFFFFF,0xFFFFFF00,0xFFFFFFFF);
		text_box("existing", 534,282, 160,52, 2,
			0xFFFFFFFF,0xFFFFFF00,0xFFFFFFFF);
		text_box("custom", 710,282, 124,52, 2,
			0xFFFFFFFF,0xFFFFFF00,0xFFFFFFFF);
		text_box("no wipe", 850,282, 142,52, 2,
			0xFFFFFFFF,0xFFFFFF00,0xFFFFFFFF);

		text("kexec .tar to LV:", 16,360, 2,2, 0xFFFF00FF, 0xFF000000);
		text_box("default", 374,360, 142,52, 2,
			0xFFFFFFFF,0xFFFF00FF,0xFFFFFFFF);
		text_box("existing", 534,360, 160,52, 2,
			0xFFFFFFFF,0xFFFF00FF,0xFFFFFFFF);

		text_box("uImage kernel to boot partition", 16,428, 574,52, 2,
			0xFFFFFFFF,0xFF0000FF,0xFFFFFFFF);

		text_box("replace moboot with nsboot", 16,486, 484,52, 2,
			0xFFFFFFFF,0xFFFF0000,0xFFFFFFFF);

		ts_read(&ts_x, &ts_y);

		if (in_box(16, 128, 124, 70)) ret = 1;
		else if (in_box(374, 214, 142, 52)) {
			filename = select_file(EXT, ".zip");
			if (filename == NULL) continue;
			flags = android_options();
			if (confirm("install Android"))
                                install_android(filename, NULL, flags);
		} else if (in_box(534, 214, 160, 52)) {
			filename = select_file(EXT, ".zip");
			if (filename == NULL) continue;
			lv_set = select_lv_set();
			if (lv_set == NULL) continue;
			flags = android_options();
			if (confirm("install Android"))
                                install_android(filename, lv_set, flags);
		} else if (in_box(714, 214, 124, 52)) {
			filename = select_file(EXT, ".zip");
			if (filename == NULL) continue;
			lv_set = text_input("enter custom LV set name:");
			if ((lv_set == NULL) || (lv_set[0] == '\0')) continue;
			flags = android_options();
			if (confirm("install Android"))
				install_android(filename, lv_set, flags);
		} else if (in_box(374, 282, 142, 52)) {
			filename = select_file(EXT, ".gz");
			if (filename == NULL) continue;
                        size = size_screen("for new volume", 1728, 8192);
			if (confirm("install .tar.gz file"))
                                install_native(filename, NULL, size);
		} else if (in_box(534, 282, 160, 52)) {
			filename = select_file(EXT, ".gz");
			if (filename == NULL) continue;
			lv = select_lv(0);
			if (lv == NULL) continue;
			if (confirm("install .tar.gz file"))
	                         install_native(filename, lv, 0);
		} else if (in_box(714, 282, 124, 52)) {
			filename = select_file(EXT, ".gz");
			if (filename == NULL) continue;
			lv = text_input("enter custom LV name:");
			if ((lv == NULL) || (lv[0] == '\0')) continue;
                        size = size_screen("for new volume", 1728, 8192);
			if (confirm("install .tar.gz file"))
				install_native(filename, lv, size);
		} else if (in_box(850, 282, 160, 52)) {
			filename = select_file(EXT, ".gz");
			if (filename == NULL) continue;
			lv = select_lv(0);
			if (lv == NULL) continue;
			if (confirm("install .tar.gz file"))
	                         install_native(filename, lv, -1);
		} else if (in_box(374, 360, 142, 52)) {
			filename = select_file(EXT, ".tar");
			if (filename == NULL) continue;
			if (confirm("install kexec .tar"))
                                install_tar(filename, NULL);
		} else if (in_box(534, 360, 142, 52)) {
			filename = select_file(EXT, ".tar");
			if (filename == NULL) continue;
			lv = select_lv(0);
			if (lv == NULL) continue;
			if (confirm("install kexec .tar"))
                                install_tar(filename, lv);
		} else if (in_box(16, 428, 718, 52)) {
			filename = select_file(BASE, "uImage.");
			if (filename == NULL) continue;
			if (confirm("install uImage"))
                                install_uimage(filename);
		} else if (in_box(16, 486, 502, 52)) {
			if (confirm("replace moboot")) replace_moboot();
		}
	}
}

void util_menu(void) {
	int ts_x, ts_y;
	int ret = 0, code;
	long sys, dat, cac;
	char *bname, *lv, *lv_set, *cmd;
	char pwd[PATH_MAX];

	while (!ret) {
		clear_screen();
		text("utility menu", 8, 8, 6, 6, 0xFF808080, 0xFF000000);

		text_box("back", 16,128, 124,70, 3, 0xFFFFFFFF,0xFF606060,0xFFFFFFFF);
		text_box("delete volume", 16,214, 250,52, 2,
			0xFFFFFFFF,0xFFFF0000,0xFFFFFFFF);
		text_box("delete volume set", 282,214, 322,52, 2,
			0xFFFFFFFF,0xFFFF0000,0xFFFFFFFF);
		text_box("reclaim media space", 620,214, 358,52, 2,
			0xFFFFFFFF,0xFF00C000,0xFFFFFFFF);
		text_box("rescan boot items", 16,282, 322,52, 2,
			0xFFFFFFFF,0xFF0000FF,0xFFFFFFFF);
		text_box("mount volume", 354,282, 232,52, 2,
			0xFFFFFFFF,0xFF606060,0xFFFFFFFF);
		text_box("mount volume set", 602,282, 304,52, 2,
			0xFFFFFFFF,0xFF606060,0xFFFFFFFF);
		text_box("format volume", 16,350, 250,52, 2,
			0xFFFFFFFF,0xFFFF0000,0xFFFFFFFF);
		text_box("format volume set", 282,350, 322,52, 2,
			0xFFFFFFFF,0xFFFF0000,0xFFFFFFFF);
		text_box("unmount volume", 620,350, 268,52, 2,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("create volume tarchive", 16,418, 412,52, 2,
			0xFFFFFFFF,0xFF0000FF,0xFFFFFFFF);
		text_box("run shell command", 444,418, 322,52, 2,
			0xFF00FF00,0xFF000000,0xFFC0C0C0);
		text_box("create volume set", 16,486, 322,52, 2,
			0xFF000000,0xFFFFFFFF,0xFF000000);
		text_box("create volume", 354,486, 250,52, 2,
			0xFF000000,0xFFFFFFFF,0xFF000000);
		text_box("check volume", 620,486, 232,52, 2,
			0xFFFFFFFF,0xFF00C000,0xFFFFFFFF);
		text_box("resize volume", 16,558, 250,52, 2,
			0xFFFFFFFF,0xFF0000C0,0xFFFFFFFF);
		text_box("display log", 282,558, 214,52, 2,
			0xFFFFFFFF,0xFF000000,0xFFFFFFFF);
		text_box("dump log to file", 512,558, 304,52, 2,
			0xFFFFFFFF,0xFF000000,0xFFFFFFFF);
		text_box("set brightness", 16,626, 268,52, 2,
			0x00000000,0xFFFFFFFF,0x00000000);
		text_box("start adbd", 300,626, 196,52, 2,
			0x00000000,0xFFFFFFFF,0x00000000);

		ts_read(&ts_x, &ts_y);

		if (in_box(16, 128, 124, 70)) ret = 1;
		else if (in_box(16, 214, 250, 52)) {
			lv = select_lv(1);
			if (lv == NULL) continue;
			if (confirm("delete volume")) delete_lv(lv);
		} else if (in_box(282, 214, 322, 52)) {
			lv_set = select_lv_set();
			if (lv_set == NULL) continue;
			if (confirm("delete volume set")) delete_lv_set(lv_set);
		} else if (in_box(620, 214, 358, 52)) {
			if (confirm("reclaim media space")) resize_lv("media", RS_RECLAIM, 0);
		} else if (in_box(16, 282, 322, 52)) {
			free_boot_items();
			scan_boot_lvs();
		} else if (in_box(354, 282, 232, 52)) {
			lv = select_lv(0);
			if (lv == NULL) continue;
			mount_lv(lv);
		} else if (in_box(602, 282, 304, 52)) {
			lv_set = select_lv_set();
			if (lv_set == NULL) continue;
			mount_lv_set(lv_set);
		} else if (in_box(16, 350, 250, 52)) {
			lv = select_lv(1);
			if (lv == NULL) continue;
			if (confirm("format volume")) wipe_lv(lv);
		} else if (in_box(282, 350, 322, 52)) {
			lv_set = select_lv_set();
			if (lv_set == NULL) continue;
			if (confirm("format volume set")) wipe_lv_set(lv_set);
		} else if (in_box(620, 350, 268, 52)) {
			lv = select_lv(0);
			if (lv == NULL) continue;
			umount_lv(lv);
		} else if (in_box(16, 418, 415, 52)) {
			lv = select_lv(0);
			if (lv == NULL) continue;
			bname = text_input("Enter the name for your backup:");
			if ((bname == NULL) || (bname[0] == '\0')) continue;
			if (confirm("backing up takes > 5 min")) lv_to_tgz(lv, bname);
			free(bname);
		} else if (in_box(444, 418, 322, 52)) {
			if (getcwd(pwd, PATH_MAX) == NULL) {
				logperror("can't getcwd");
				continue;
			}
			cmd = text_input(pwd);
			if ((cmd == NULL) || (cmd[0] == '\0')) continue;
			if (code = WEXITSTATUS(system(cmd)))
				logprintf("2your shell command exited with code %d", code);
			free(cmd);
		} else if (in_box(16, 486, 322, 52)) {
			lv_set = text_input("enter volume set name - example: android42");
			if ((lv_set == NULL) || (lv_set[0] == '\0')) continue;
			sys = size_screen("for system volume", 240, 640);
			dat = size_screen("for data volume", 200, 2048);
			cac = size_screen("for cache volume", 160, 256);
			if (confirm("create volume set")) new_lv_set(lv_set, sys, dat, cac);
			free(lv_set);
		} else if (in_box(354, 486, 250, 52)) {
			lv = text_input("enter volume name - example: arch-root");
			if ((lv == NULL) || (lv[0] == '\0')) continue;
			sys = size_screen("for new volume", 8, 65536);
			if (confirm("create volume")) new_lv(lv, sys);
			free(lv);
		} else if (in_box(620, 486, 232, 52)) {
			lv = select_lv(0);
			if (lv == NULL) continue;
			if (confirm("check volume - system may reboot on its own")) check_lv(lv);
		} else if (in_box(16, 558, 250, 52)) {
			lv = select_lv(0);
			if (lv == NULL) continue;
			snprintf(pwd, sizeof(pwd), "volume %s current size %ld MiB", lv, get_lv_size(lv));
			sys = size_screen(pwd, 8, 65536);
			if (confirm("resize volume, preserving data")) resize_lv(lv, RS_SET, sys);
		} else if (in_box(282, 558, 214, 52))
			display_wholelog();
		else if (in_box(512, 558, 304, 52)) {
			bname = text_input("please enter log name:");
			if ((bname == NULL) || (bname[0] == '\0')) continue;
			mount_lv("media");
			mkdir("/mnt/media/nsboot/logs/", 0755);
			snprintf(pwd, sizeof(pwd), "/mnt/media/nsboot/logs/%s.log", bname);
			dump_log_to_file(pwd);
		} else if (in_box(16, 626, 268, 52)) {
			sys = size_screen("set LCD brightness (4-255)", 4, 255);
			set_brightness(sys);
		} else if (in_box(300, 626, 196, 52))
			start_adbd();
	}
}

int android_options(void) {
	int ret = 0, done = 0;
	int ts_x, ts_y;

	while (!done) {
		clear_screen();
		text("install options", 16, 16, 4, 4, 0xFFFFFFFF, 0x00000000);
		text_box("accept", 16,104, 178,70, 3, 0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("wipe system", 16,208, 412,88, 4,
			(ret & WIPE_SYSTEM) ? 0xFFFFFFFF : 0x00000000,
			(ret & WIPE_SYSTEM) ? 0x00000000 : 0xFFFFFFFF,
			(ret & WIPE_SYSTEM) ? 0xFFFFFFFF : 0x00000000);

		text_box("wipe data", 16,312, 340,88, 4,
			(ret & WIPE_DATA) ? 0xFFFFFFFF : 0x00000000,
			(ret & WIPE_DATA) ? 0x00000000 : 0xFFFFFFFF,
			(ret & WIPE_DATA) ? 0xFFFFFFFF : 0x00000000);

		text_box("wipe cache", 16,424, 376,88, 4,
			(ret & WIPE_CACHE) ? 0xFFFFFFFF : 0x00000000,
			(ret & WIPE_CACHE) ? 0x00000000 : 0xFFFFFFFF,
			(ret & WIPE_CACHE) ? 0xFFFFFFFF : 0x00000000);

		text_box("install to moboot", 16,536, 628,88, 4,
			(ret & INST_MOBOOT) ? 0xFFFFFFFF : 0x00000000,
			(ret & INST_MOBOOT) ? 0x00000000 : 0xFFFFFFFF,
			(ret & INST_MOBOOT) ? 0xFFFFFFFF : 0x00000000);

		ts_read(&ts_x, &ts_y);

		if (in_box(16, 208, 412, 88)) ret ^= WIPE_SYSTEM;
		else if (in_box(16, 312, 340, 88)) ret ^= WIPE_DATA;
		else if (in_box(16, 424, 376, 88)) ret ^= WIPE_CACHE;
		else if (in_box(16, 328, 628, 88)) ret ^= INST_MOBOOT;
		else if (in_box(16, 104, 178, 70)) done = 1;
	}
	return ret;
}

void info_screen(void) {
	int ts_x, ts_y;
	int ret = 0;
	char date_str[128], battery_str[32];
	long charge_full, charge_now;
	float percent;
	FILE *fp;

	qfscanf("/sys/class/power_supply/battery/charge_full", "%ld", 
		&charge_full);

	while (!ret) {
		clear_screen();

		fp = popen("date", "r");
		if (date_str == NULL) {
			logperror("error invoking date cmd");
			return;
		}

		if (fgets(date_str, sizeof(date_str), fp) == NULL) {
			logperror("error piping from date cmd");
			return;
		}
		pclose(fp);

		qfscanf("/sys/class/power_supply/battery/charge_now", "%ld",
			&charge_now);

		percent = (float)charge_now / (float)charge_full * 100.0;
		snprintf(battery_str, sizeof(battery_str),
			"battery level = %.1f%%", percent);

		text("information", 16,16, 4,4, 0xFFFFFFFF, 0xFF000000);
		text("nsboot by willcast, 2013", 16,104, 2,2, 0xFF808080, 0xFF000000);
		text(date_str, 16,156, 2,2, 0xFF808080, 0xFF000000);
		text(battery_str, 16,198, 2,2, 0xFF808080, 0xFF000000);
		text("touch to update", 16,240, 2,2, 0xFF808080, 0xFF000000);

		text_box("back", 16,282, 160,70, 3, 0xFFFFFFFF,0xFF808080,0xFFFFFFFF);

		ts_read(&ts_x, &ts_y);

		if (in_box(16, 249, 160, 70)) ret = 1;
	}
}

void task_menu(const char *file1) {
	char *file2, *slash;
	int ts_x, ts_y;
	int done = 0, mode;

	mode = file_mode(sel);

	while (!done) {
		clear_screen();

		text("tasks for file:", 16,16, 4,4, 0xFFFFFFFF, 0xFF000000);
		text(file1, 16,104, 1,1, 0xFFC0C0C0, 0xFF000000);

		text_box("cancel", 16,148, 232,88, 4, 0xFFFFFFFF,
		0xFFFF0000, 0xFFFFFFFF);
		text_box("okay", 264,148, 160,88, 4, 0xFFFFFFFF,
		0xFF00FF00, 0xFFFFFFFF);
		text_box("move item", 16,252, 259,70, 3, 0xFFFFFFFF,
			0xFF0000C0, 0xFFFFFFFF);
		text_box("rename item", 291,252, 313,70, 3, 0xFFFFFFFF,
			0xFF0000C0, 0xFFFFFFFF);
		text_box("copy item", 16,338, 259,70, 3, 0xFFFFFFFF,
			0xFF0000FF, 0xFFFFFFFF);
		text_box("delete item", 291,338, 313,70, 3, 0xFFFFFFFF,
			0xFFFF0000, 0xFFFFFFFF);
		text_box("make directory", 16,424, 394,70, 3, 0xFFFFFFFF,
			0xFFC0C0C0, 0xFFFFFFF);

		text("user", 16,500, 2,2, 0xFFFF0000, 0xFF000000);
		text_box("read", 176,500, 88,52, 2,
			(mode & 00400) ? 0xFF000000 : 0xFFFFFFFF,
			(mode & 00400) ? 0xFFFFFFFF : 0xFF000000,
			(mode & 00400) ? 0xFF000000 : 0xFFFFFFFF);
		text_box("write", 276,500, 106,52, 2,
			(mode & 00200) ? 0xFF000000 : 0xFFFFFFFF,
			(mode & 00200) ? 0xFFFFFFFF : 0xFF000000,
			(mode & 00200) ? 0xFF000000 : 0xFFFFFFFF);
		text_box("execute", 398,500, 142,52, 2,
			(mode & 00100) ? 0xFF000000 : 0xFFFFFFFF,
			(mode & 00100) ? 0xFFFFFFFF : 0xFF000000,
			(mode & 00100) ? 0xFF000000 : 0xFFFFFFFF);

		text("group", 16,570, 2,2, 0xFF00FF00, 0xFF000000);
		text_box("read", 176,570, 88,52, 2,
			(mode & 00040) ? 0xFF000000 : 0xFFFFFFFF,
			(mode & 00040) ? 0xFFFFFFFF : 0xFF000000,
			(mode & 00040) ? 0xFF000000 : 0xFFFFFFFF);
		text_box("write", 276,570, 106,52, 2,
			(mode & 00020) ? 0xFF000000 : 0xFFFFFFFF,
			(mode & 00020) ? 0xFFFFFFFF : 0xFF000000,
			(mode & 00020) ? 0xFF000000 : 0xFFFFFFFF);
		text_box("execute", 398,570, 142,52, 2,
			(mode & 00010) ? 0xFF000000 : 0xFFFFFFFF,
			(mode & 00010) ? 0xFFFFFFFF : 0xFF000000,
			(mode & 00010) ? 0xFF000000 : 0xFFFFFFFF);

		text("everyone", 16,640, 2,2, 0xFF0000FF, 0xFF000000);
		text_box("read", 176,640, 88,52, 2,
			(mode & 00004) ? 0xFF000000 : 0xFFFFFFFF,
			(mode & 00004) ? 0xFFFFFFFF : 0xFF000000,
			(mode & 00004) ? 0xFF000000 : 0xFFFFFFFF);
		text_box("write", 276,640, 106,52, 2,
			(mode & 00002) ? 0xFF000000 : 0xFFFFFFFF,
			(mode & 00002) ? 0xFFFFFFFF : 0xFF000000,
			(mode & 00002) ? 0xFF000000 : 0xFFFFFFFF);
		text_box("execute", 398,640, 142,52, 2,
			(mode & 00001) ? 0xFF000000 : 0xFFFFFFFF,
			(mode & 00001) ? 0xFFFFFFFF : 0xFF000000,
			(mode & 00001) ? 0xFF000000 : 0xFFFFFFFF);

		text("special", 16,710, 2,2, 0xFFFFFFFF, 0xFF000000);
		text_box("set UID", 158,710, 142,52, 2,
			(mode & 01000) ? 0xFF000000 : 0xFFFFFFFF,
			(mode & 01000) ? 0xFFFFFFFF : 0xFF000000,
			(mode & 01000) ? 0xFF000000 : 0xFFFFFFFF);
		text_box("set GID", 316,710, 142,52, 2,
			(mode & 02000) ? 0xFF000000 : 0xFFFFFFFF,
			(mode & 02000) ? 0xFFFFFFFF : 0xFF000000,
			(mode & 02000) ? 0xFF000000 : 0xFFFFFFFF);
		text_box("sticky", 474,710, 142,52, 2,
			(mode & 04000) ? 0xFF000000 : 0xFFFFFFFF,
			(mode & 04000) ? 0xFFFFFFFF : 0xFF000000,
			(mode & 04000) ? 0xFF000000 : 0xFFFFFFFF);

		ts_read(&ts_x, &ts_y);

		if (in_box(16, 148, 232, 88)) done = 1;
		else if (in_box(264, 148, 160, 88)) {
			chmod(file1, mode);
			done = 1;
		} else if (in_box(16, 252, 259, 70)) {
			file2 = select_file(ANY, NULL);
			if (file2 == NULL) return;
			if (confirm("move file - files maybe be overwritten")) {
				slash = strrchr(file2, '/');
				*slash = '\0';

				move_file(file1, file2);
			}
			done = 1;
 		} else if (in_box(291, 252, 313, 70)) {
			do {
				file2 = text_input("enter new name:");
				slash = strchr(file2, '/');
				if (slash != NULL) {
					logprintf("2new name can't contain a slash");
					free(file2);
				}
			} while (slash != NULL);
			if (confirm("rename file - files may be overwritten"))
				move_file(file1, file2);
			free(file2);
			done = 1;
		} else if (in_box(16, 338, 259, 70)) {
			file2 = select_file(ANY, NULL);
			if (file2 == NULL) return;

			if (confirm("copy file - destination will be overwritten")) {
				slash = strrchr(file2, '/');
				*slash = '\0';

				copy_file(file1, file2);
			}
		} else if (in_box(291, 338, 313, 70) && confirm("delete file permanently")) {
			delete_file(file1);
			done = 1;
		} else if (in_box(16, 424, 394, 70)) {
			do {
				file2 = text_input("enter directory name:");
				slash = strchr(file2, '/');
				if (slash != NULL) {
					logprintf("2name can't contain a slash");
					free(file2);
				}
			} while (slash != NULL);
			mkdir(file2, 0755);
			logprintf("0%s", "directory %s created with mode 0755", file2);
			free(file2);
		} else if (in_box(176, 500, 88, 52)) mode ^= 00400;
		else if (in_box(276, 500, 106, 52))  mode ^= 00200;
		else if (in_box(398, 500, 142, 52))  mode ^= 00100;
		else if (in_box(176, 570, 88, 52))   mode ^= 00040;
		else if (in_box(276, 570, 106, 52))  mode ^= 00020;
		else if (in_box(398, 570, 142, 52))  mode ^= 00010;
		else if (in_box(176, 640, 88, 52))   mode ^= 00004;
		else if (in_box(276, 640, 106, 52))  mode ^= 00002;
		else if (in_box(398, 640, 142, 52))  mode ^= 00001;
		else if (in_box(158, 710, 142, 52))  mode ^= 01000;
		else if (in_box(316, 710, 142, 52))  mode ^= 02000;
		else if (in_box(474, 710, 142, 52))  mode ^= 04000;
	}
	chmod(file1, mode);
}

