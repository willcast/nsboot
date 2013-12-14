/*
	nsboot/browse.h
	header for dynamically generated lists of objects

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

#ifndef NSBOOT_BROWSE_H
#define NSBOOT_BROWSE_H

#include <dirent.h>
#include <sys/stat.h>

#define LV_MAX 64

struct dir_list {
	struct dirent **dents;
	struct stat **stats;
	int n;
};

enum filter_spec { ANY, BASE, EXT, NAME };

void update_dir_list(void);
void update_lv_lists(void);
void free_dir_list(struct dir_list *);

int cmp_base(char *, char *);
int cmp_ext(char *, char *);

void file_menu(char *, enum filter_spec);

void sel_file(int);
void sel_lv(int);

int file_is_dir(int);
char * file_name(int);
int file_count(void);
int file_mode(int);

char * lv_name(int);
int lv_count(void);

char * lv_set_name(int);
int lv_set_count(void);

int is_android(char *);

int is_immutable(const char *);

void copy_file(const char *, const char *);
void move_file(const char *, const char *);
void delete_file(const char *);

extern long lv_sizes[LV_MAX];
#endif
