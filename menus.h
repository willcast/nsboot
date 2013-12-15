/*
	nsboot/menus.h
	header for static navigation menus for nsboot

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

#ifndef NSBOOT_MENUS_H
#define NSBOOT_MENUS_H


#define START_LIST(list_x, list_y, help_x, colr1, colr2, colr3) \
	int current_y = list_y - 48; \
	int col0_x = list_x, col1_x = list_x + 32; \
	int col2_x = help_x, USECOLR = 0; \
	uint32_t CURCOLR = colr1; \
	uint32_t BG1COLR = colr1; \
	uint32_t BG2COLR = colr2; \
	uint32_t SELCOLR = colr3;

#define DECL_LINE(label) \
	int btn_y_ ## label = -44;

#define DRAW_LINE(label, _txt, _help) \
	do { \
	 	USECOLR = !USECOLR; \
		CURCOLR = USECOLR ? BG1COLR : BG2COLR; \
		current_y += 48; \
		btn_y_ ## label = current_y; \
		fill_rect(0, btn_y_ ## label, 1024, 44, CURCOLR); \
		text(_txt, col1_x, btn_y_ ## label + 8, 2, 2, 0xFFFFFFFF, CURCOLR); \
		text(_help, col2_x, btn_y_ ## label + 8, 1, 1, 0xFFFFFFFF, CURCOLR); \
	} while (0);

#define PRESSED(label) in_box(0, btn_y_ ## label, 1024, 44)

#define DECL_TOGL(toglbl, state) \
		DECL_LINE(toglbl); \
		int togl_ ## toglbl = state;

#define DRAW_TOGL(toglbl, _txt, _help) \
		DRAW_LINE(toglbl, _txt, _help); \
		text(togl_ ## toglbl ? "on" : "off", col0_x, btn_y_ ## toglbl + 16, 1,1, 0xFFFFFFFF, CURCOLR);

#define DO_TOGL(toglbl) \
	if (PRESSED(toglbl)) togl_ ## toglbl = !togl_ ## toglbl;

void main_menu(void);

void installer_menu(void);
int zip_menu(void);
int tgz_menu(void);
int tar_menu(void);

void util_menu(void);
int lv_menu(void);
int lv_set_menu(void);
int misc_menu(void);

void info_screen(void);

void task_menu(const char *);

int android_options(void);

#endif
