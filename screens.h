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

#ifndef NSBOOT_SCREENS_H
#define NSBOOT_SCREENS_H

void start_screen(void);
void main_menu(void);
void installer_menu(void);
void boot_menu(void);
void util_menu(void);
void info_screen(void);

char * select_file(enum filter_spec, char *);
char * select_lv(int);
char * select_lv_set(void);

int confirm(const char *);
long size_screen(long, long, int);

int android_options(void);

#endif
