/*
	nsboot/screens.c
	static navigation menus for nsboot

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
	int _y = 48;
	int quit = 0;
	char *file1;

	DECL_LINE(rebootbutton);
	DECL_LINE(offbutton);
	DECL_LINE(bootbutton);
	DECL_LINE(installbutton);
	DECL_LINE(utilitybutton);

	while (!quit) {
		strcpy(sbstr, "nsboot");

		clear_screen();

		START_LIST(8, 48, 304, 0xFF0000C0, 0xFF0000FF, 0xFF8080FF)

		DRAW_LINE(rebootbutton, "reboot", "reboot the tablet now")
		DRAW_LINE(offbutton, "off", "turn the tablet off now")
		DRAW_LINE(bootbutton, "boot", "enter the boot menu")
		DRAW_LINE(installbutton, "install", "enter the installer menu")
		DRAW_LINE(utilitybutton, "utilities", "enter the utilities menu");

		update_screen();
		ts_read(&ts_x, &ts_y);

		if PRESSED(offbutton) {
			logprintf("1powering off...");
			sync();
			reboot(RB_POWER_OFF);
			quit = 1;
		} else if PRESSED(rebootbutton) {
			logprintf("1rebooting...");
			sync();
			reboot(RB_AUTOBOOT);
			quit = 1;
		} else if PRESSED(bootbutton) {
			boot_menu();
 		} else if PRESSED(installbutton) {
			installer_menu();
		} else if PRESSED(utilitybutton) {
			util_menu();
		}
	}
}

void installer_menu(void) {
	int ts_x, ts_y;
	int ret = 0;
	char *filename;

	DECL_LINE(backbutton);
	DECL_LINE(zipbutton);
	DECL_LINE(tgzbutton);
	DECL_LINE(tarbutton);
	DECL_LINE(uimgbutton);
	DECL_LINE(replacebutton);

	while (!ret) {
		strcpy(sbstr, "install");

		clear_screen();

		START_LIST(16, 48, 416, 0xFF00C000, 0xFF00FF00, 0xFF80FF80);

		DRAW_LINE(backbutton, "back", "return to main menu");
		DRAW_LINE(zipbutton, "install .zip", "install a .zip file (Android ROM/gapps/mod)");
		DRAW_LINE(tgzbutton, "install .tar.gz", "install a .tar.gz file (Linux distro, or nsboot backup)");
		DRAW_LINE(tarbutton, "install .tar", "install a .tar file (kexec kernel)");
		DRAW_LINE(uimgbutton, "install uImage", "install a kernel for moboot");
		DRAW_LINE(replacebutton, "make nsboot default", "make sure all your OSes work with it first");

		update_screen();
		ts_read(&ts_x, &ts_y);

		if PRESSED(backbutton) ret = 1;
		else if PRESSED(zipbutton) ret = zip_menu();
		else if PRESSED(tgzbutton) ret = tgz_menu();
		else if PRESSED(tarbutton) ret = tar_menu();
		else if (PRESSED(replacebutton) && confirm("replace moboot"))
			replace_moboot();
		else if PRESSED(uimgbutton) {
			filename = select_file(BASE, "uImage.");
			if (filename == NULL) continue;
			if (confirm("install uImage"))
                                install_uimage(filename);
		}
	}
}	

int zip_menu(void) {
	char *filename = NULL;
	char *lv_set = NULL;
	int ret = 0;
	int ts_x, ts_y;
	int flags;	

	DECL_LINE(backbutton);
	DECL_LINE(homebutton);
	DECL_LINE(selzip);
	DECL_LINE(sellvset);
	DECL_TOGL(wipesys, 1);
	DECL_TOGL(wipedat, 1);
	DECL_TOGL(wipecac, 1);
	DECL_LINE(doit);

	while (!ret) {
		strcpy(sbstr, "install ZIP");

		clear_screen();

		START_LIST(16, 48, 384, 0xFF00C000, 0xFF00FF00, 0xFF80FF80);		

		DRAW_LINE(backbutton, "back", "return to installer menu");	
		DRAW_LINE(homebutton, "home", "return to main menu");	
		DRAW_LINE(selzip, "select zip", "browse to the ZIP you want to install");
		DRAW_LINE(sellvset, "select volume set", "select the volume set to install to (you can create one\n"
												"from utilities)");
		DRAW_TOGL(wipesys, "wipe system", "ON if you want to wipe /system before installing\n"
										  "for installing new ROMs)");
		DRAW_TOGL(wipedat, "wipe data", "ON if you want to wipe /data (aka factory reset)\n"
										"before installing");
		DRAW_TOGL(wipecac, "wipe cache", "ON if you want to wipe /cache (should almost always say YES)");
		DRAW_LINE(doit, "install now", "install the ZIP now");

		update_screen();
		ts_read(&ts_x, &ts_y);

		if PRESSED(backbutton) ret = 1;
		else if PRESSED(homebutton) ret = 2;
		else if PRESSED(selzip) filename = select_file(EXT, ".zip");
		else if PRESSED(sellvset) lv_set = select_lv_set();
		else if (PRESSED(doit) && (filename != NULL) && (lv_set != NULL)) {
			flags = togl_wipesys * WIPE_SYSTEM +
					togl_wipedat * WIPE_DATA + 
					togl_wipecac * WIPE_CACHE;
			install_android(filename, lv_set, flags);	 
		}

		DO_TOGL(wipesys);
		DO_TOGL(wipedat);
		DO_TOGL(wipecac);
	}
	return (ret & 2);
}

int tgz_menu(void) {
	char *filename = NULL;
	char *lv = NULL;
	int ret = 0;
	int ts_x, ts_y;

	DECL_LINE(backbutton);
	DECL_LINE(homebutton);
	DECL_LINE(seltgz);
	DECL_LINE(sellv);
	DECL_TOGL(wipe, 1);
	DECL_LINE(doit);

	while (!ret) {
		clear_screen();

		strcpy(sbstr, "install .tar.gz");

		START_LIST(16, 48, 352, 0xFF00C000, 0xFF00FF00, 0xFF80FF80);		

		DRAW_LINE(backbutton, "back", "return to installer menu");	
		DRAW_LINE(homebutton, "home", "return to main menu");	
		DRAW_LINE(seltgz, "select tarchive", "browse to the tarchive you want to install");
		DRAW_LINE(sellv, "select volume", "select the volume to install to (you can create one\n"
						" from utilities)");
		DRAW_TOGL(wipe, "wipe", "ON if you want to wipe before installing (should usually say YES)");
		DRAW_LINE(doit, "install now", "install the tarchive now");

		update_screen();
		ts_read(&ts_x, &ts_y);

		if PRESSED(backbutton) ret = 1;
		else if PRESSED(homebutton) ret = 2;
		else if PRESSED(seltgz) filename = select_file(EXT, ".gz");
		else if PRESSED(sellv) lv = select_lv(0);
		else if (PRESSED(doit) && (filename != NULL))
			install_native(filename, lv, -1);

		DO_TOGL(wipe);
	}
	return (ret & 2);
}

int tar_menu(void) {
	char *filename = NULL;
	char *lv = NULL;
	int ret = 0;
	int ts_x, ts_y;

	DECL_LINE(backbutton);
	DECL_LINE(homebutton);
	DECL_LINE(seltar);
	DECL_LINE(sellv);
	DECL_LINE(doit);

	while (!ret) {
		clear_screen();

		strcpy(sbstr, "install .tar");

		START_LIST(16, 48, 352, 0xFF00C000, 0xFF00FF00, 0xFF80FF80);		

		DRAW_LINE(backbutton, "back", "return to installer menu");	
		DRAW_LINE(homebutton, "home", "return to main menu");	
		DRAW_LINE(seltar, "select tarchive", "browse to the kexec tarchive you want to install");
		DRAW_LINE(sellv, "select volume", "select the volume to install to");
		DRAW_LINE(doit, "install now", "install the kexec tarchive now");

		update_screen();
		ts_read(&ts_x, &ts_y);

		if PRESSED(backbutton) ret = 1;
		else if PRESSED(homebutton) ret = 2;
		else if PRESSED(seltar) filename = select_file(EXT, ".tar");
		else if PRESSED(sellv) lv = select_lv(0);
		else if (PRESSED(doit) && (filename != NULL))
			install_tar(filename, lv);
	}
	return (ret & 2);
}

void util_menu(void) {
	int ts_x, ts_y;
	int ret = 0;
	char *file = NULL;

	DECL_LINE(backbutton);
	DECL_LINE(lvbutton);
	DECL_LINE(lvsetbutton);
	DECL_LINE(miscbutton);
	DECL_LINE(filebutton);
	
	while (!ret) {
		strcpy(sbstr, "utilities");

		clear_screen();
		START_LIST(16, 48, 384, 0xFFC00000, 0xFFFF0000, 0xFFFF8080);

		DRAW_LINE(backbutton, "back", "return to main menu");	
		DRAW_LINE(lvbutton, "manage volumes", "create, delete, wipe, check, back up,\n"
											"mount or resize volumes");
		DRAW_LINE(lvsetbutton, "manage volume sets", "create, delete, wipe, check, back up,\n"
													"mount or resize volume sets");
		DRAW_LINE(miscbutton, "misc. options", "display or save log, set display brightness,\n"
												"or run a shell command\n");
		DRAW_LINE(filebutton, "files", "enter the file browser");

		update_screen();
		ts_read(&ts_x, &ts_y);

		if (PRESSED(backbutton)) ret = 1;
		else if (PRESSED(lvbutton)) ret = lv_menu();
		else if (PRESSED(lvsetbutton)) ret = lv_set_menu();
		else if (PRESSED(miscbutton)) ret = misc_menu();
 		else if PRESSED(filebutton) {
			file = select_file(ANY, NULL);
			if (file == NULL) continue;
			task_menu(file);
		}	
	}
}

int lv_menu(void) {
	int ts_x, ts_y;
	int ret = 0;
	long size;
	char *lv, *name, str[256];

	DECL_LINE(backbutton);
	DECL_LINE(homebutton);
	DECL_LINE(createbutton);
	DECL_LINE(deletebutton);
	DECL_LINE(fmtbutton);
	DECL_LINE(chkbutton);
	DECL_LINE(backupbutton);
	DECL_LINE(restorebutton);
	DECL_LINE(mntbutton);
	DECL_LINE(umntbutton);
	DECL_LINE(resizebutton);

	while (!ret) {
		strcpy(sbstr, "lv manager");

		clear_screen();
		START_LIST(16, 48, 352, 0xFFC00000, 0xFFFF0000, 0xFFFF8080);
		
		DRAW_LINE(backbutton, "back", "return to utility menu");	
		DRAW_LINE(homebutton, "home", "return to main menu");	
		DRAW_LINE(createbutton, "create volume", "create a logical volume (you need free space first)");
		DRAW_LINE(deletebutton, "delete volume", "delete a logical volume, this will render all data\n"
												"on it inaccessible and free space for new volumes");
		DRAW_LINE(fmtbutton, "format volume", "format a logical volume");
		DRAW_LINE(chkbutton, "check volume", "check the filesystem of a logical volume for errors\n"
												"(please note that nsboot might reboot after this)");
		DRAW_LINE(backupbutton, "back up volume", "create a .tar.gz file (tarchive) which is a replica\n"
												 " of the contents of a volume, you can restore it later");
		DRAW_LINE(restorebutton, "restore volume", "restore a previously-created volume backup tarchive");
		DRAW_LINE(mntbutton, "mount volume", "mount a volume so you can use it in the file browser\n");
		DRAW_LINE(umntbutton, "unmount volume", "unmount a volume so you can format/resize/delete/check it");
		DRAW_LINE(resizebutton, "resize volume", "resize the a volume (along with its filesystem)");

		update_screen();
		ts_read(&ts_x, &ts_y);

		if PRESSED(backbutton) ret = 1;
		else if PRESSED(homebutton) ret = 2;
		else if (PRESSED(createbutton)) {
			lv = text_input("enter volume name - example: arch-root");
			if ((lv == NULL) || (lv[0] == '\0')) continue;
			size = size_screen("enter size of  new volume", 8, 65536);
			if (confirm("create volume")) new_lv(lv, size);
			free(lv);		
		} else if (PRESSED(deletebutton)) {
			lv = select_lv(1);
			if (lv == NULL) continue;
			if (confirm("delete volume")) delete_lv(lv);
		} else if (PRESSED(fmtbutton)) {
			lv = select_lv(1);
			if (lv == NULL) continue;
			if (confirm("format volume")) wipe_lv(lv);
		} else if (PRESSED(	chkbutton)) {
			lv = select_lv(0);
			if (lv == NULL) continue;
			if (confirm("check volume - system may reboot on its own")) check_lv(lv);
		} else if (PRESSED(backupbutton)) {
			lv = select_lv(0);
			if (lv == NULL) continue;
			name = text_input("Enter the name for your backup:");
			if ((name == NULL) || (name[0] == '\0')) continue;
			if (confirm("backing up takes > 5 min")) lv_to_tgz(lv, name);
			free(name);
		} else if (PRESSED(restorebutton)) tgz_menu();
		else if (PRESSED(mntbutton)) {
			lv = select_lv(0);
			if (lv == NULL) continue;
			mount_lv(lv);
		} else if (PRESSED(umntbutton)) {
			lv = select_lv(0);
			if (lv == NULL) continue;
			umount_lv(lv);
		} else if (PRESSED(resizebutton)) {
			lv = select_lv(0);
			if (lv == NULL) continue;
			snprintf(str, sizeof(str), "volume %s current size %ld MiB", lv, get_lv_size(lv));
			size = size_screen(str, 8, 65536);
			if (confirm("resize volume, preserving data")) resize_lv(lv, RS_SET, size);
		}
	}
	return (ret & 2);
}

int lv_set_menu(void) {
	int ts_x, ts_y;
	int ret = 0;
	long sys, dat, cac;
	char *lv_set;
	char *bname;

	DECL_LINE(backbutton);
	DECL_LINE(homebutton);
	DECL_LINE(createbutton);
	DECL_LINE(deletebutton);
	DECL_LINE(fmtbutton);
	DECL_LINE(mntbutton);
	DECL_LINE(resizebutton);

	while (!ret) {
		strcpy(sbstr, "lv manager");

		clear_screen();
		START_LIST(16, 48, 400, 0xFFC00000, 0xFFFF0000, 0xFFFF8080);
		
		DRAW_LINE(backbutton, "back", "return to utility menu");	
		DRAW_LINE(homebutton, "home", "return to main menu");	
		DRAW_LINE(createbutton, "create volume", "create a volume set (you need free space first)");
		DRAW_LINE(deletebutton, "delete volume", "delete a volume set, this will render all data\n"
											"on it inaccessible and free space for new volumes");	
		DRAW_LINE(fmtbutton, "format volume", "format a volume set");
		DRAW_LINE(mntbutton, "mount volume", "mount a volume set so you can use it in the file browser\n");
		DRAW_LINE(resizebutton, "resize volume", "resize the a volume set (along with its filesystems)");

		update_screen();
		ts_read(&ts_x, &ts_y);

		if PRESSED(backbutton) ret = 1;
		else if PRESSED(homebutton) ret = 2;
		else if (PRESSED(createbutton)) {
			lv_set = text_input("enter volume set name - example: android42");
			if ((lv_set == NULL) || (lv_set[0] == '\0')) continue;
			sys = size_screen("for system volume", 240, 640);
			dat = size_screen("for data volume", 200, 2048);
			cac = size_screen("for cache volume", 160, 256);
			if (confirm("create volume set")) new_lv_set(lv_set, sys, dat, cac);
			free(lv_set);
		} else if (PRESSED(deletebutton)) {
			lv_set = select_lv_set();
			if (lv_set == NULL) continue;
			if (confirm("delete volume set")) delete_lv_set(lv_set);
		} else if (PRESSED(fmtbutton)) {
			lv_set = select_lv_set();
			if (lv_set == NULL) continue;
			if (confirm("format volume set")) wipe_lv_set(lv_set);
		} else if (PRESSED(mntbutton)) {
			lv_set = select_lv_set();
			if (lv_set == NULL) continue;
			mount_lv_set(lv_set);
		}
	}
	return (ret & 2);
}

int misc_menu(void) {
	int ts_x, ts_y;
	int ret = 0, code, bright;
	char *cmd, *name;
	char pwd[PATH_MAX];

	DECL_LINE(backbutton);
	DECL_LINE(homebutton);
	DECL_LINE(shellcmd);
	DECL_LINE(displog);
	DECL_LINE(dumplog);
	DECL_LINE(setbright);

	while (!ret) {
		strcpy(sbstr, "misc.");

		clear_screen();
		START_LIST(16, 48, 352, 0xFFC00000, 0xFFFF0000, 0xFFFF8080);
		
		DRAW_LINE(backbutton, "back", "return to utility menu");	
		DRAW_LINE(homebutton, "home", "return to main menu");	
		DRAW_LINE(shellcmd, "shell command", "run a command inside nsboot's bash shell");
		DRAW_LINE(displog, "display log", "display the nsboot log");
		DRAW_LINE(dumplog, "save log", "save the nsboot log to a file under /mnt/media/nsboot/logs/");
		DRAW_LINE(setbright, "brightness", "set the brightness of the LCD");

		update_screen();
		ts_read(&ts_x, &ts_y);

		if (PRESSED(backbutton)) ret = 1;
		else if (PRESSED(homebutton)) ret = 2;
		else if (PRESSED(shellcmd)) {
			if (getcwd(pwd, PATH_MAX) == NULL) {
				logperror("can't getcwd");
				continue;
			}
			cmd = text_input(pwd);
			if ((cmd == NULL) || (cmd[0] == '\0')) continue;
			if (code = WEXITSTATUS(system(cmd)))
				logprintf("2your shell command exited with code %d", code);
			free(cmd);
		} else if (PRESSED(displog)) display_wholelog();
		else if (PRESSED(dumplog)) {
			name = text_input("please enter log name:");
			if ((name == NULL) || (name[0] == '\0')) continue;
			mount_lv("media");
			mkdir("/mnt/media/nsboot/logs/", 0755);
			snprintf(pwd, sizeof(pwd), "/mnt/media/nsboot/logs/%s.log", name);
			dump_log_to_file(pwd);	
		} else if (PRESSED(setbright)) {
			bright = size_screen("set LCD brightness (4-255)", 4, 255);
			set_brightness(bright);
		}
	}
	return (ret & 2);
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

		update_screen();
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

		update_screen(); 
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

