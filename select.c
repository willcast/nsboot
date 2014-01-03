/*
	nsboot/select.c
	dynamically generated selection menus

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

#include "fb.h"
#include "file.h"
#include "log.h"
#include "input.h"
#include "install.h"
#include "menus.h"
#include "select.h"
#include "osk.h"
#include "boot.h"

extern boot_item *menu;
extern int menu_size;
int sel;

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/reboot.h>

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
		int *by, *kept;
		int j, nkept;
		int match, keep;
		uint16_t mode;

		sel = -1;

		clear_screen();
		START_LIST(8, 48, 448, 0xFF404040, 0xFF808080, 0xFFC0C0C0)

		update_dir_list();
		n = file_count();
		by = malloc(n * sizeof(int));
		kept = malloc(n * sizeof(int));
		npages = 1 + n / perpage;

		pwd = getcwd(pwd, PATH_MAX);
		text(pwd, 16, 24, 1, 1, 0xFF606060, 0xFF000000);

		DRAW_LINE(backbtn, "back", "return to previous menu without making a selection"); 
		if (pagenum > 1) {
			DRAW_LINE(prevpage, "previous page", "not all files are shown"); 
		}

		j = 0;
		for (int i = 0; i < n; ++i) {
			switch (filtloc) {
			case ANY:
				match = 1;
				break;
			case BASE:
				match = !cmp_base(file_name(i), filter);
				break;
			case EXT:
				match = !cmp_ext(file_name(i), filter);
				break;
			case NAME:
				match = !strcasecmp(file_name(i), filter);
				break;
			default:
				match = 0;
			}

			if (file_is_dir(i) || match) {
				kept[j] = i; 
				++j;
			}
		}
		nkept = j;

		for (int i = perpage * (pagenum - 1); i < nkept; ++i) {
			if (i > pagenum * perpage) break;

			snprintf(str, sizeof(str), "permissions = %o", file_mode(kept[i]) & 07777);

			USECOLR = !USECOLR; 
			CURCOLR = USECOLR ? BG1COLR : BG2COLR; 
			current_y += 48; 
			by[i] =  current_y; 
			fill_rect(0, by[i], 1024, 44, CURCOLR);
			text(file_name(kept[i]), col1_x, by[i] + 8, 1, 1, 0xFFFFFFFF, CURCOLR);
			text(str, col2_x, by[i] + 8, 1, 1, 0xFFFFFFFF, CURCOLR);
		}
		if (pagenum < npages) {
			DRAW_LINE(nextpage, "next page", "not all files are shown");
		}

		while (sel == -1) {
			update_screen();
			input_read();

			if (BTN_CLICKED(backbtn)) {
				free(by);
				free(kept);
				return NULL;
			}
			for (int i = perpage * (pagenum - 1); i <= nkept; ++i)
				if (was_clicked(0, by[i], 1024, 44))
					sel = kept[i];
			if (BTN_CLICKED(prevpage)) {
				--pagenum;
				break;
			} else if (BTN_CLICKED(nextpage)) {
				++pagenum;
				break;
			}
		}

		if (sel != -1) {
			free(by);
			free(kept);
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

		update_screen();
		input_read();
		if (BTN_CLICKED(backbtn)) ret = 2;

		for (int i = PERPAGE * (pagenum - 1); i < n; ++i)
			if (was_clicked(0, by[i], 1024, 44)) {
				if (i > pagenum * PERPAGE) break;
				selected_lv = strdup(lv_name(i));
				ret = 1;				
			}
		if (BTN_CLICKED(prevpage)) --pagenum;
		else if (BTN_CLICKED(nextpage)) ++pagenum;
	} while (!ret);
	
	free(by);

	return selected_lv;
}

char * select_lv_set(void) {
	int *by, pagenum = 1, npages;
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

		update_screen();
		input_read();
		if (BTN_CLICKED(backbtn)) ret = 2;

		for (int i = PERPAGE * (pagenum - 1); i < n; ++i)
			if (was_clicked(0, by[i], 1024, 44)) {
				if (i > pagenum * PERPAGE) break;
				selected_set = strdup(lv_set_name(i));
				ret = 1;				
			}
		if (BTN_CLICKED(prevpage)) --pagenum;
		else if (BTN_CLICKED(nextpage)) ++pagenum;
	} while (!ret);

	free(by);

	return selected_set;
}

// 0 for false, 1 for true
int confirm(const char *label) {
	int ret = -1;
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
		update_screen();
		input_read();

		if (BTN_CLICKED(yesbtn)) ret = 1;
		else if (BTN_CLICKED(nobtn)) ret = 0;
	}
	return ret;
}

void boot_menu(void) {
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
			boot_item *cur_item = menu + i; 

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
			update_screen();
			input_read();

			if (BTN_CLICKED(backbtn)) ret = 1;

			for (int i = 0; i < menu_size; ++i) {
				if (i > pagenum * PERPAGE) break;
				if (was_clicked(0, by[i], 1024, 44))
					boot_kexec(i);
			}
			if (BTN_CLICKED(nextpage)) ++pagenum;
			else if (BTN_CLICKED(prevpage)) --pagenum;
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

