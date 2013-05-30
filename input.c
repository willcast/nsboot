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

long size_screen(const char *msg, long min, long max) {
	long size;
	do {
		size = num_input(msg);
		if (size < min) steprintf("size too small, lower limit is %ld", min);
		if (size > max) steprintf("size too big, upper limit is %ld", max);
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
