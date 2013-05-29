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

extern struct boot_item *menu;
extern int menu_size;

//number of selected file.
int sel;

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/reboot.h>

const char keyboard[4][4][14][2] = { {
	{ "`","1","2","3","4","5","6","7","8","9","0","-","=","\\" },
	{ "", "q","w","e","r","t","y","u","i","o","p","[","]", ""  },
	{ "", "a","s","d","f","g","h","j","k","l",";","'", "", ""  },
 	{ "", "z","x","c","v","b","n","m",",",".","/", "", "", ""  }
}, {
	{ "~","!","#","@","$","%","^","&","*","(",")","_","+","|" },
	{ "", "Q","W","E","R","T","Y","U","I","O","P","{","}",""  },
	{ "", "A","S","D","F","G","H","J","K","L",":","\"","",""  },
	{ "", "Z","X","C","V","B","N","M","<",">","?", "", "",""  }
}, {
	{ "`","1","2","3","4","5","6","7","8","9","0","-","=","\\" },
	{ "", "Q","W","E","R","T","Y","U","I","O","P","[","]",""   },
	{ "", "A","S","D","F","G","H","J","K","L",";","'","", ""   },
	{ "", "Z","X","C","V","B","N","M",",",".","/", "", "",""   }
}, {
	{ "~","!","#","@","$","%","^","&","*","(",")","_","+","|" },
	{ "", "q","w","e","r","t","y","u","i","o","p","{","}", ""  },
	{ "", "a","s","d","f","g","h","j","k","l",":","\"", "", ""  },
 	{ "", "z","x","c","v","b","n","m","<",">","?", "", "", ""  }
} };

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
			status("have a nice day");
			quit = 1;
		} else if (in_box(192, 128, 340, 88) && confirm("power off")) {
			status("powering off...");
			sync();
			reboot(RB_POWER_OFF);
		} else if (in_box(548, 128, 232, 88) && confirm("reboot")) {
			status("rebooting...");
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
		text_box("install .zip to new volume set", 16,214, 556,52, 2,
			0xFFFFFFFF,0xFF00FFFF,0xFFFFFFFF);
		text_box("install .zip to existing volume set", 16,282, 644,52, 2,
			0xFFFFFFFF,0xFF00FFFF,0xFFFFFFFF);
		text_box("install .tar.gz to new volume", 16,350, 537,52, 2,
			0xFFFFFFFF,0xFFFFFF00,0xFFFFFFFF);
		text_box("install .tar.gz to existing volume", 16,418, 628,52, 2,
			0xFFFFFFFF,0xFFFFFF00,0xFFFFFFFF);
		text_box("install uImage kernel to boot partition", 16,486, 718,52, 2,
			0xFFFFFFFF,0xFFFF00FF,0xFFFFFFFF);
		text_box("install kexec .tar to existing volume", 16,554, 682,52, 2,
			0xFFFFFFFF,0xFFFF00FF,0xFFFFFFFF);
		text_box("replace moboot with nsboot", 16,622, 502,52, 2,
			0xFFFFFFFF,0xFFFF0000,0xFFFFFFFF);

		ts_read(&ts_x, &ts_y);

		if (in_box(16, 128, 124, 70)) ret = 1;
		else if (in_box(16, 214, 556, 52)) {
			filename = select_file(EXT, ".zip");
			if (filename == NULL) continue;
			flags = android_options();
			if (confirm("install Android"))
                                install_android(filename, NULL, flags);
		} else if (in_box(16, 282, 644, 52)) {
			filename = select_file(EXT, ".zip");
			if (filename == NULL) continue;
			lv_set = select_lv_set();
			if (lv_set == NULL) continue;
			flags = android_options();
			if (confirm("install Android"))
                                install_android(filename, lv_set, flags);
		} else if (in_box(16, 350, 537, 52)) {
			filename = select_file(EXT, ".gz");
			if (filename == NULL) continue;
                        size = size_screen("for new volume", 1728, 8192, 256);
			if (confirm("install .tar.gz file"))
                                install_native(filename, NULL, size);
		} else if (in_box(16, 418, 628, 52)) {
			filename = select_file(EXT, ".gz");
			if (filename == NULL) continue;
			lv = select_lv(0);
			if (lv == NULL) continue;
			if (confirm("install .tar.gz file"))
	                                install_native(filename, lv, 0);
		} else if (in_box(16, 486, 718, 52)) {
			filename = select_file(BASE, "uImage.");
			if (filename == NULL) continue;
			if (confirm("install uImage"))
                                install_uimage(filename);
		} else if (in_box(16, 554, 682, 52)) {
			filename = select_file(EXT, ".tar");
			if (filename == NULL) continue;
			if (confirm("install kexec .tar"))
                                install_tar(filename);
		} else if (in_box(16, 622, 502, 52)) {
			if  (confirm("replace moboot")) replace_moboot();
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
			0xFFFFFFFF,0xFF00FF00,0xFFFFFFFF);
		text_box("create volume set", 16,486, 322,52, 2,
			0xFF000000,0xFFFFFFFF,0xFF000000);
		text_box("create volume", 354,486, 250,52, 2,
			0xFF000000,0xFFFFFFFF,0xFF000000);

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
			if (confirm("reclaim media space")) resize_media(0);
		} else if (in_box(16, 282, 322, 52)) {
			free_boot_items();
			scan_boot_lvs();
		} else if (in_box(354, 282, 232, 52))
			mount_lv(select_lv(0));
		else if (in_box(602, 282, 304, 52))
			mount_lv_set(select_lv_set());
		else if (in_box(16, 350, 250, 52)) {
			lv = select_lv(1);
			if (lv == NULL) continue;
			if (confirm("format volume")) wipe_lv(lv);
		} else if (in_box(282, 350, 322, 52)) {
			lv_set = select_lv_set();
			if (lv_set == NULL) continue;
			if (confirm("format volume set")) wipe_lv_set(lv_set);
		} else if (in_box(620, 350, 268, 52))
			umount_lv(select_lv(0));
		else if (in_box(16, 418, 415, 52)) {
			lv = select_lv(0);
			if (lv == NULL) continue;
			bname = text_input("Enter the name for your backup:");
			if (bname == NULL) continue;
			if (confirm("backing up takes > 5 min")) lv_to_tgz(lv, bname);
			free(bname);
		} else if (in_box(444, 418, 322, 52)) {
			if (getcwd(pwd, PATH_MAX) == NULL) {
				stperror("can't getcwd");
				continue;
			}
			cmd = text_input(pwd);
			if ((cmd == NULL) || (cmd[0] == '\0')) continue;
			if (code = WEXITSTATUS(system(cmd)))
				steprintf("your shell command exited with code %d", code);
			free(cmd);
		} else if (in_box(16, 486, 322, 52)) {
			lv_set = text_input("enter volume set name - example: android42");
			if ((lv_set == NULL) || (lv_set[0] == '\0')) continue;
			sys = size_screen("for system volume", 256, 512, 16);
			dat = size_screen("for data volume", 200, 512, 16);
			cac = size_screen("for cache volume", 192, 256, 8);
			if (confirm("create volume set")) new_lv_set(lv_set, sys, dat, cac);
			free(lv_set);
		} else if (in_box(354, 486, 250, 52)) {
			lv = text_input("enter volume name - example: arch-root");
			if ((lv == NULL) || (lv[0] == '\0')) continue;
			sys = size_screen("for new volume", 256, 10240, 256);
			if (confirm("create volume")) new_lv(lv, sys);
			free(lv);
		}
	}
}

// filtloc and filter operate on the filenames and determine whether they
// may be selected:
// ALL: don't filter, in this case, filter can be NULL
// BASE: filter with cmp_base(filename, filter) - only show when part up to
//       last extension matches
// EXT: filter with cmp_ext(filename, filter) - only show when (last) extension
//      matches
// NAME: filter with strcasecmp(filename, filter) - only show when whole name
//       matches
char * select_file(enum filter_spec filtloc, char *filter) {
	char *selected_file;
	selected_file = malloc(PATH_MAX * sizeof(char));

	selected_file[0] = '\0';

	while (selected_file[0] == '\0') {
		int btn_x = 16, btn_y = 250, btn_w;
		char *pwd = NULL;
		int n;
		int *bw, *bx, *by, *keep;
		int ts_x, ts_y;

		sel = -1;

		clear_screen();

		update_dir_list();
		n = file_count();

		bx = malloc(n * sizeof(int));
		by = malloc(n * sizeof(int));
		bw = malloc(n * sizeof(int));
		keep = malloc(n * sizeof(int));

		pwd = getcwd(pwd, PATH_MAX);

		text("select a file", 16, 16, 4, 4, 0xFF00FF00, 0xFF000000);
		text(pwd, 16, 112, 2, 2, 0xFF606060, 0xFF000000);

		text_box("back", 16,164, 160,70, 3,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);

		for (int i = 0; i < n; ++i) {
			btn_w = strlen(file_name(i)) * 9 + 16;
			if (btn_x + btn_w > FBWIDTH) {
				btn_y += 34;
				btn_x = 16;
			}
			if (btn_y + 26 > FBHEIGHT) break;
			switch (filtloc) {
			case ANY:
				keep[i] = 1;
				break;
			case BASE:
				keep[i] = !cmp_base(file_name(i), filter);
				break;
			case EXT:
				keep[i] = !cmp_ext(file_name(i), filter);
				break;
			case NAME:
				keep[i] = !strcasecmp(file_name(i), filter);
				break;
			}

			if (file_is_dir(i)) keep[i] = 1; // always keep directories
			if (keep[i])
				text_box(file_name(i), btn_x,btn_y,
					btn_w,26, 1, 0x00000000, 0xFFFFFFFF,
					0xFFFFFFFF);
			else
				text_box(file_name(i), btn_x, btn_y,
					btn_w,26, 1, 0xFF606060, 0x00000000,
					0x00000000);

			bx[i] = btn_x;
			by[i] = btn_y;
			bw[i] = btn_w;

			btn_x += btn_w + 16;
		}

		while (sel == -1) {
			ts_read(&ts_x, &ts_y);
			if (in_box(16, 164, 160, 70)) {
				free(bx);
				free(by);
				free(bw);
				free(keep);
				return NULL;
			}
			for (int i = 0; i < n; ++i)
				if (keep[i] && in_box(bx[i], by[i], bw[i], 26))
					sel = i;
		}

		free(bx);
		free(by);
		free(bw);
		free(keep);

		if (file_is_dir(sel)) {
			if (chdir(file_name(sel)) == -1)
				perror("chdir");
		} else {
			strcpy(selected_file, pwd);
			strcat(selected_file, "/");
			strcat(selected_file, file_name(sel));
		}
	}

	return selected_file;
}

char * select_lv(int disable_android) {
	int btn_x = 16, btn_y = 208, btn_w;
	int n;
	int ts_x, ts_y;
	int *bx, *by, *bw;
	char *selected_lv = NULL;

	clear_screen();

	update_lv_lists();
	n = lv_count();

	text("select a volume", 16, 16, 4, 4, 0xFFFFFFFF, 0xFF000000);

	text_box("back", 16,104, 160,70, 3, 0xFFFFFFFF, 0xFF606060, 0xFFFFFFFF);

	bx = malloc(n * sizeof(int));
	by = malloc(n * sizeof(int));
	bw = malloc(n * sizeof(int));

	for (int i = 0; i < n; ++i) {
		btn_w = strlen(lv_name(i)) * 9 + 16;
		if (btn_x + btn_w > FBWIDTH) {
			btn_y += 34;
			btn_x = 16;
		}
		if (btn_y + 26 > FBHEIGHT) break;
		if (is_android(lv_name(i)) && disable_android)
			text_box(lv_name(i), btn_x, btn_y,
				btn_w,26, 1, 0xFF606060, 0x00000000,
				0x00000000);
		else
			text_box(lv_name(i), btn_x,btn_y,
				btn_w,26, 1, 0x00000000, 0xFFFFFFFF,
				0xFFFFFFFF);

		bx[i] = btn_x;
		by[i] = btn_y;
		bw[i] = btn_w;

		btn_x += btn_w + 16;
	}

	while (selected_lv == NULL) {
		ts_read(&ts_x, &ts_y);
		if (in_box(16,104, 160,70))
			break;
		for (int i = 0; i < n; ++i)
			if (in_box(bx[i], by[i], bw[i], 26))
				selected_lv = strdup(lv_name(i));
	}

	free(bx);
	free(by);
	free(bw);

	return selected_lv;
}

char * select_lv_set(void) {
	int btn_x = 16, btn_y = 208, btn_w;
	int *bx, *by, *bw;
	int ts_x, ts_y;
	int n;
	char *selected_set = NULL;

	clear_screen();

	update_lv_lists();
	n = lv_set_count();

	bx = malloc(n * sizeof(int));
	by = malloc(n * sizeof(int));
	bw = malloc(n * sizeof(int));

	text("select a volume set", 16, 16, 4, 4, 0xFFFFFFFF, 0xFF000000);
	text_box("back", 16,104, 160,70, 3, 0xFFFFFFFF,0xFF808080,0xFFFFFFFF);

	for (int i = 0; i < n; ++i) {
		btn_w = strlen(lv_set_name(i)) * 9 + 16;
		if (btn_x + btn_w > FBWIDTH) {
			btn_y += 34;
			btn_x = 16;
		}
		if (btn_y + 26 > FBHEIGHT) break;
		text_box(lv_set_name(i), btn_x,btn_y,
			btn_w,26, 1, 0x00000000, 0xFFFFFFFF,
			0xFFFFFFFF);

		bx[i] = btn_x;
		by[i] = btn_y;
		bw[i] = btn_w;

		btn_x += btn_w + 16;
	}

	while (selected_set == NULL) {
		ts_read(&ts_x, &ts_y);
		if (in_box(16, 104, 160, 70))
			break;
		for (int i = 0; i < n; ++i)
			if (in_box(bx[i], by[i], bw[i], 26))
				selected_set = strdup(lv_set_name(i));
	}

	free(bx);
	free(by);
	free(bw);

	return selected_set;
}

// 0 for false, 1 for true
int confirm(const char *label) {
	int ret = -1;
	int ts_x, ts_y;

	clear_screen();
	text("are you sure you want to:", 16,16, 4,4, 0xFFC0C0C0, 0x00000000);
	text(label, 16,104, 2,2, 0xFF808080, 0x00000000);

	text_box("yes", 16,208, 151,106, 5, 0xFFFFFFFF,0xFF00FF00,0xFFFFFFFF);
	text_box("no", 16,340, 106,106, 5, 0xFFFFFFFF,0xFFFF0000,0xFFFFFFFF);

	while (ret == -1) {
		ts_read(&ts_x, &ts_y);
		if (in_box(16, 208, 151, 106)) ret = 1;
		else if (in_box(16, 340, 106, 106)) ret = 0;
	}
	return ret;
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

void boot_menu(void) {
	int ts_x, ts_y;
	int sel = -1;
	int *by, *bw;

	clear_screen();

	text("select a boot entry", 16, 16, 4, 4, 0xFFFFFFFF, 0xFF000000);
	text_box("back", 16,104, 160,70, 3, 0xFFFFFFFF,0xFF808080,0xFFFFFFFF);

	if (!menu_size) scan_boot_lvs();

	by = malloc(menu_size * sizeof(int));
	bw = malloc(menu_size * sizeof(int));

	for (int i = 0; i < menu_size; ++i) {
		int btn_y = 208 + i * 34;
		struct boot_item *cur_item = menu + i; 
		int btn_w = 16 + strlen(cur_item->label) * 9;

		text_box(cur_item->label, 16,btn_y, btn_w,26, 1,
			0x00000000, 0xFFFFFFFF, 0xFFFFFFFF);

		by[i] = btn_y;
		bw[i] = btn_w;
	}

	while (sel == -1) {
		ts_read(&ts_x, &ts_y);
		if (in_box(16, 104, 160, 70)) break;

		for (int i = 0; i < menu_size; ++i) {
			if (in_box(16, by[i], bw[i], 26))
				boot_kexec(i);
		}
	}

	free(by);
	free(bw);
}

void info_screen(void) {
	int ts_x, ts_y;
	int ret = 0;
	char date_str[128], battery_str[32];
	long charge_full, charge_now;
	float percent;
	FILE *fp;

	while (!ret) {
		clear_screen();

		fp = popen("date", "r");
		if (date_str == NULL) {
			stperror("error invoking date cmd");
			return;
		}

		if (fgets(date_str, sizeof(date_str), fp) == NULL) {
			stperror("error piping from date cmd");
			return;
		}
		fclose(fp);

		text("information", 16,16, 4,4, 0xFFFFFFFF, 0xFF000000);
		text("nsboot by willcast, 2013", 16,104, 2,2, 0xFF808080, 0xFF000000);
		text(date_str, 16,156, 2,2, 0xFF808080, 0xFF000000);
		text("touch to update", 16,198, 2,2, 0xFF808080, 0xFF000000);

		text_box("back", 16,282, 160,70, 3, 0xFFFFFFFF,0xFF808080,0xFFFFFFFF);

		ts_read(&ts_x, &ts_y);

		if (in_box(16, 249, 160, 70)) ret = 1;
	}
}

long size_screen(const char *msg, long min, long max, int step) {
	int ts_x, ts_y;
	int ret = 0;
	long size;
	char size_str[64];

	size = min;

	while (!ret) {
		clear_screen();

		snprintf(size_str, sizeof(size_str), "size: %ld MiB", size);

		text("select volume size", 16,16, 4,4, 0xFFFFFFFF,0xFF000000);
		text(msg, 16,104, 2,2, 0xFF808080, 0xFF000000);
		text(size_str, 16,192, 2,2, 0xFF808080, 0xFF000000);

		text_box("okay", 16,280, 160,70, 3, 0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("decrease", 16,388, 232,70, 3, 0xFF000000,0xFFFFFFFF,0xFF000000);
		text_box("increase", 264,388, 232,70, 3, 0xFF000000,0xFFFFFFFF,0xFF000000);

		ts_read(&ts_x, &ts_y);

		if (in_box(16, 280, 160, 70)) ret = 1;
		else if (in_box(16, 388, 232, 70) && (size - step >= min))
			size -= step;
		else if (in_box(264, 388, 232, 70) && (size + step <= max))
			size += step;
	}

	return size;
}

char * text_input(const char *prompt) {
	int ts_x, ts_y;
	int done = 0;
	char *output;
	int cur_ch = 0, shift = 0, caps = 0, n;

	output = malloc(128 * sizeof(char));;
	if (output == NULL) {
		stperror("can't allocate textbuf");
		return NULL;
	}
	memset(output, '\0', 128 * sizeof(char));

	while (!done) {
		n = shift + caps*2;
		if (cur_ch > 126) cur_ch = 126;
		clear_screen();

		text(prompt, 16,16, 2,2, 0xFFFFFFFF, 0xFF000000);
		text(output, 0,68, 1,1, 0xFF808080, 0xFF000000);

		for (int y = 0; y < 4; ++y) for (int x = 0; x < 14; ++x)
			if (keyboard[n][y][x][0] != '\0')
				text_box(keyboard[n][y][x], 16+64*x,384+64*y, 52,52,
					2, 0xFFFFFFFF,0xFF808080,0xFFFFFFFF);

		text_box("back", 912,384, 52,52, 1,
			0xFF000000,0xFFFFFFFF,0xFF000000);
		text_box("caps", 16,512, 52,52, 1,
			0xFF000000,0xFFFFFFFF,0xFF000000);
		text_box("enter", 784,512, 116,52, 2,
			0xFF000000,0xFFFFFFFF,0xFF000000);
		text_box("shift", 16,576, 52,52, 1,
			0xFF000000,0xFFFFFFFF,0xFF000000);
		text_box("shift", 784,576, 116,52, 2,
			0xFF000000,0xFFFFFFFF,0xFF000000);
		text_box("", 208,640, 448,52, 1,
			0xFF000000,0xFFFFFFFF,0xFF000000);

		ts_read(&ts_x, &ts_y);

		for (int y = 0; y < 4; ++y) for (int x = 0; x < 14; ++x)
			if (in_box((16+64*x), (384+64*y), 52,52) &&
				(keyboard[n][y][x][0] != '\0')) {
					output[cur_ch] = keyboard[n][y][x][0];
					++cur_ch;
					shift = 0;
					break;
			}

		if (in_box(912, 384, 52, 52)) {
			if ((output[cur_ch] == '\0') && cur_ch) --cur_ch;
			output[cur_ch] = '\0';
			if (cur_ch) --cur_ch;
		} else if (in_box(16, 512, 52, 52))
			caps = !caps;
		else if (in_box(784, 512, 116, 52))
			done = 1;
		else if (in_box(16, 576, 116, 52) || in_box(784, 576, 116, 52))
			shift = !shift;
		else if (in_box(208, 640, 448, 52))
			output[cur_ch++] = ' ';

	}

	return output;
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
					status_error("new name can't contain a slash");
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
			unlink(file1);
			done = 1;
		} else if (in_box(16, 424, 394, 70)) {
			do {
				file2 = text_input("enter directory name:");
				slash = strchr(file2, '/');
				if (slash != NULL) {
					status_error("name can't contain a slash");
					free(file2);
				}
			} while (slash != NULL);
			mkdir(file2, 0755);
			stprintf("directory %s created with mode 0755", file2);
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

