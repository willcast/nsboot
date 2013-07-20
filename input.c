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
#include "log.h"
#include "install.h"
#include "screens.h"
#include "input.h"
#include "touch.h"
#include "boot.h"

extern struct boot_item *menu;
extern int menu_size;
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
	char *selected_file, str[256];
	int ts_x, ts_y;
	int npages, pagenum = 1;
	const int perpage = PERPAGE - 1;

	DECL_LINE(backbtn);
	DECL_LINE(prevpage);
	DECL_LINE(nextpage);

	strcpy(sbstr, "file browser");

	selected_file = malloc(PATH_MAX * sizeof(char));

	selected_file[0] = '\0';

	while (selected_file[0] == '\0') {
		char *pwd = NULL;
		int n;
		int *by, *keep;
		int reali = 0;
		uint16_t mode;

		sel = -1;

		clear_screen();
		START_LIST(8, 48, 448, 0xFF404040, 0xFF808080, 0xFFC0C0C0)

		update_dir_list();
		n = file_count();
		by = malloc(n * sizeof(int));
		keep = malloc(n * sizeof(int));
		npages = 1 + n / perpage;

		pwd = getcwd(pwd, PATH_MAX);
		text(pwd, 16, 24, 1, 1, 0xFF606060, 0xFF000000);

		DRAW_LINE(backbtn, "back", "return to previous menu without making a selection"); 
		if (pagenum > 1) {
			DRAW_LINE(prevpage, "previous page", "not all files are shown"); 
		}

		for (int i = perpage * (pagenum - 1); i < n; ++i) {
			if (i > pagenum * perpage) break;

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
			default:
				keep[i] = 0;
			}

			if (file_is_dir(i)) keep[i] = 1; // always keep directories
			if (!keep[i]) break;

			snprintf(str, sizeof(str), "permissions = %o", file_mode(i) & 07777);

			USECOLR = !USECOLR; 
			CURCOLR = USECOLR ? BG1COLR : BG2COLR; 
			current_y += 48; 
			by[reali] =  current_y; 
			fill_rect(0, by[reali], 1024, 44, CURCOLR);
			text(file_name(i), col1_x, by[reali] + 8, 1, 1, 0xFFFFFFFF, CURCOLR);
			text(str, col2_x, by[reali] + 8, 1, 1, 0xFFFFFFFF, CURCOLR);
	
			++reali;
		}
		if (pagenum < npages) {
			DRAW_LINE(nextpage, "next page", "not all files are shown");
		}

		while (sel == -1) {
			ts_read(&ts_x, &ts_y);
			if (PRESSED(backbtn)) {
				free(by);
				free(keep);
				return NULL;
			}
			for (int i = 0; i <= reali; ++i)
				if (in_box(0, by[i], 1024, 44))
					sel = i;
			if (PRESSED(prevpage)) {
				--pagenum;
				break;
			} else if (PRESSED(nextpage)) {
				++pagenum;
				break;
			}
		}

		if (sel != -1) {
			free(by);
			free(keep);
			pagenum = 1;
	
			if (file_is_dir(sel)) {
				if (chdir(file_name(sel)) == -1)
					perror("chdir");
			} else {
				strcpy(selected_file, pwd);
				strcat(selected_file, "/");
				strcat(selected_file, file_name(sel));
			}
		}
	}

	return selected_file;
}

char * select_lv(int disable_android) {
	int n;
	int ts_x, ts_y;
	int *by, pagenum = 1, npages, ret = 0;	
	char *selected_lv = NULL, str[64];

	DECL_LINE(backbtn);
	DECL_LINE(prevpage);
	DECL_LINE(nextpage);

	strcpy(sbstr, "select volume");

	update_lv_lists();
	n = lv_count();
	by = malloc(n * sizeof(int));
	npages = 1 + n / PERPAGE;

	do {
		clear_screen();
		START_LIST(8, 32, 352, 0xFF404040, 0xFF808080, 0xFFC0C0C0)

		DRAW_LINE(backbtn, "back", "return to previous menu without making a selection"); 
		if (pagenum > 1) {
			DRAW_LINE(prevpage, "previous page", "not all volumes are shown"); 
		}

		for (int i = PERPAGE * (pagenum - 1); i < n; ++i) {
			if (i > pagenum * PERPAGE) break;
			if (is_android(lv_name(i)) && disable_android)
				continue;

			snprintf(str, sizeof(str), "size = %ld MiB", lv_sizes[i]);

			USECOLR = !USECOLR; 
			CURCOLR = USECOLR ? BG1COLR : BG2COLR; 
			current_y += 48; 
			by[i] =  current_y; 
			fill_rect(0, by[i], 1024, 44, CURCOLR);
			text(lv_name(i), col1_x, by[i] + 8, 2, 2, 0xFFFFFFFF, CURCOLR);
			text(str, col2_x, by[i] + 8, 1, 1, 0xFFFFFFFF, CURCOLR);
		}
		if (pagenum < npages) {
			DRAW_LINE(nextpage, "next page", "not all volumes are shown");
		}

		ts_read(&ts_x, &ts_y);
		if (PRESSED(backbtn)) ret = 2;

		for (int i = PERPAGE * (pagenum - 1); i < n; ++i)
			if (in_box(0, by[i], 1024, 44)) {
				if (i > pagenum * PERPAGE) break;
				selected_lv = strdup(lv_name(i));
				ret = 1;				
			}
		if (PRESSED(prevpage)) --pagenum;
		else if (PRESSED(nextpage)) ++pagenum;
	} while (!ret);
	
	free(by);

	return selected_lv;
}

char * select_lv_set(void) {
	int *by, pagenum = 1, npages;
	int ts_x, ts_y;
	int n, ret = 0;
	char *selected_set = NULL;

	DECL_LINE(backbtn);
	DECL_LINE(prevpage);
	DECL_LINE(nextpage);

	strcpy(sbstr, "select volume set");

	update_lv_lists();
	n = lv_set_count();
	by = malloc(n * sizeof(int));
	npages = 1 + n / PERPAGE;
	
	do {
		clear_screen();
		START_LIST(8, 32, 352, 0xFF404040, 0xFF808080, 0xFFC0C0C0)

		DRAW_LINE(backbtn, "back", "return to previous menu without making a selection"); 
		if (pagenum > 1) {
			DRAW_LINE(prevpage, "previous page", "not all volume sets are shown"); 
		}
		for (int i = PERPAGE * (pagenum - 1); i < n; ++i) {
			if (i > pagenum * PERPAGE) break;
		
			USECOLR = !USECOLR; 
			CURCOLR = USECOLR ? BG1COLR : BG2COLR; 
			current_y += 48; 
			by[i] =  current_y; 
			fill_rect(0, by[i], 1024, 44, CURCOLR);
			text(lv_set_name(i), col1_x, by[i] + 8, 2, 2, 0xFFFFFFFF, CURCOLR);
		}
		if (pagenum < npages) {
			DRAW_LINE(nextpage, "next page", "not all volume sets are shown");
		}

		ts_read(&ts_x, &ts_y);
		if (PRESSED(backbtn)) ret = 2;

		for (int i = PERPAGE * (pagenum - 1); i < n; ++i)
			if (in_box(0, by[i], 1024, 44)) {
				if (i > pagenum * PERPAGE) break;
				selected_set = strdup(lv_set_name(i));
				ret = 1;				
			}
		if (PRESSED(prevpage)) --pagenum;
		else if (PRESSED(nextpage)) ++pagenum;
	} while (!ret);

	free(by);

	return selected_set;
}

// 0 for false, 1 for true
int confirm(const char *label) {
	int ret = -1;
	int ts_x, ts_y;
	char str[64];

	DECL_LINE(yesbtn);
	DECL_LINE(nobtn);

	strcpy(sbstr, "confirm action");

	if (label == NULL)
		strcpy(str, "are you sure?");
	else
		sprintf(str, "are you sure you want to %s?", label);

	clear_screen();

	START_LIST(16, 96, 416, 0xFF00FF00, 0xFF0000FF, 0xFFC0C0C0);

	text(str, 16,16, 2,2, 0xFFC0C0C0, 0x00000000);

	DRAW_LINE(yesbtn, "yes", "perform the action");
	DRAW_LINE(nobtn, "no", "cancel the action and return to the previous screen");

	while (ret == -1) {
		ts_read(&ts_x, &ts_y);
		if (PRESSED(yesbtn)) ret = 1;
		else if (PRESSED(nobtn)) ret = 0;
	}
	return ret;
}

void boot_menu(void) {
	int ts_x, ts_y;
	int npages, pagenum = 1;
	int *by, ret = -1;
	char helpstr[256];

	DECL_LINE(backbtn);
	DECL_LINE(prevpage);
	DECL_LINE(nextpage);

	if (!menu_size) scan_boot_lvs();
	by = malloc(menu_size * sizeof(int));
	npages = 1 + menu_size / PERPAGE;

	strcpy(sbstr, "boot menu");

	clear_screen();

	do {
		clear_screen();
		START_LIST(0, 32, 384, 0xFF404040, 0xFF808080, 0xFFC0C0C0)
		col1_x = 8;

		DRAW_LINE(backbtn, "back", "return to previous menu without making a selection"); 
		if (pagenum > 1) {
			DRAW_LINE(prevpage, "previous page", "not all available kernels are shown"); 
		}

		for (int i = PERPAGE * (pagenum - 1); i < menu_size; ++i) {
			struct boot_item *cur_item = menu + i; 

			if (i > pagenum * PERPAGE) break;

			snprintf(helpstr, sizeof(helpstr), "volume is %s,\nkernel is %s", 
					cur_item->cfgdev, cur_item->kernel);

			USECOLR = !USECOLR; 
			CURCOLR = USECOLR ? BG1COLR : BG2COLR; 
			current_y += 48; 
			by[i] =  current_y; 
			fill_rect(0, by[i], 1024, 44, CURCOLR);
			text(cur_item->label, col1_x, by[i] + 8, 1, 1, 0xFFFFFFFF, CURCOLR);
			text(helpstr, col2_x, by[i] + 8, 1, 1, 0xFFFFFFFF, CURCOLR);
		}

		while (ret == -1) {
			ts_read(&ts_x, &ts_y);
			if (PRESSED(backbtn)) ret = 1;

			for (int i = 0; i < menu_size; ++i) {
				if (i > pagenum * PERPAGE) break;
				if (in_box(0, by[i], 1024, 44))
					boot_kexec(i);
			}
			if (PRESSED(nextpage)) ++pagenum;
			else if (PRESSED(prevpage)) --pagenum;
		}
	} while (!ret);

	free(by);
}

long size_screen(const char *msg, long min, long max) {
	long size;
	do {
		size = num_input(msg);
		if (size < min) logprintf("1size too small, lower limit is %ld", min);
		if (size > max) logprintf("1size too big, upper limit is %ld", max);
	} while ((size < min) || (size > max));
	return size;
}

long num_input(const char *prompt) {
	int ts_x, ts_y;
	int done = 0, cur_ch = 0;
	char buf[12];

	memset(buf, '\0', sizeof(buf));

	while (!done) {
		if (cur_ch > sizeof(buf) - 2)
			cur_ch = sizeof(buf) - 2;

		clear_screen();

		text(prompt, 16,16, 2,2, 0xFFFFFFFF, 0xFF000000);
		text(buf, 16,68, 6,6, 0xFFFFFFFF, 0xFF000000);

		text_box("7", 16,192, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("8", 156,192, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("9", 296,192, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("4", 16,332, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("5", 156,332, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("6", 296,332, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("1", 16,472, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("2", 156,472, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("3", 296,472, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("back", 16,612, 124,124, 1,
			0xFFFFFFFF,0xFF000000,0xFFFFFFFF);
		text_box("0", 156,612, 124,124, 6,
			0xFFFFFFFF,0xFF808080,0xFFFFFFFF);
		text_box("enter", 296,612, 124,124, 1,
			0xFFFFFFFF,0xFF000000,0xFFFFFFFF);

		ts_read(&ts_x, &ts_y);

		if (in_box(16, 174, 124, 124)) buf[cur_ch++] = '7';
		else if (in_box(156, 192, 124, 124)) buf[cur_ch++] = '8';
		else if (in_box(296, 192, 124, 124)) buf[cur_ch++] = '9';
		else if (in_box(16, 332, 124, 124)) buf[cur_ch++] = '4';
		else if (in_box(156, 332, 124, 124)) buf[cur_ch++] = '5';
		else if (in_box(296, 332, 124, 124)) buf[cur_ch++] = '6';
		else if (in_box(16, 472, 124, 124)) buf[cur_ch++] = '1';
		else if (in_box(156, 472, 124, 124)) buf[cur_ch++] = '2';
		else if (in_box(296, 472, 124, 124)) buf[cur_ch++] = '3';
		else if (in_box(16, 612, 124, 124)) {
			if ((buf[cur_ch] == '\0') && cur_ch) --cur_ch;
			buf[cur_ch] = '\0';
		} else if (in_box(156, 612, 124, 124)) buf[cur_ch++] = '0';
		else if (in_box(296, 612, 124, 124)) done = 1;
	}

	return atol(buf);
}

char * text_input(const char *prompt) {
	int ts_x, ts_y;
	int done = 0;
	char *output;
	int cur_ch = 0, shift = 0, caps = 0, n;

	output = malloc(128 * sizeof(char));;
	if (output == NULL) {
		logperror("can't allocate textbuf");
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
			caps ? 0xFFFFFFFF : 0xFF000000,
			caps ? 0xFF000000 : 0xFFFFFFFF,
			caps ? 0xFFFFFFFF : 0xFF000000);
		text_box("enter", 784,512, 116,52, 2,
			0xFF000000,0xFFFFFFFF,0xFF000000);
		text_box("shift", 16,576, 52,52, 1,
			shift ? 0xFFFFFFFF : 0xFF000000,
			shift ? 0xFF000000 : 0xFFFFFFFF,
			shift ? 0xFFFFFFFF : 0xFF000000);
		text_box("shift", 784,576, 116,52, 2,
			shift ? 0xFFFFFFFF : 0xFF000000,
			shift ? 0xFF000000 : 0xFFFFFFFF,
			shift ? 0xFFFFFFFF : 0xFF000000);
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
