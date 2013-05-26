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

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "browse.h"
#include "types.h"

struct dir_list *current = NULL;
struct dir_list *lvs = NULL;
struct str_list *lv_sets = NULL;

int is_hidden(const struct dirent *);
int is_lv(const struct dirent *);

void update_dir_list(void) {
	char pwd[PATH_MAX];

	if (getcwd(pwd, PATH_MAX) == NULL) {
		perror("getcwd");
		return;
	}

	if (current != NULL) free_dir_list(current);

	current = malloc(sizeof(struct dir_list));
	if (current == NULL) {
		perror("can't allocate mem for dir_list");
		exit(1);
	}

	current->n = scandir(pwd, &current->dents, is_hidden, alphasort);

	current->stats = malloc(current->n * sizeof(struct stat *));
	for (int i = 0; i < current->n; ++i) {
		current->stats[i] = malloc(sizeof(struct stat));
		if (stat(current->dents[i]->d_name, current->stats[i])) {
			fprintf(stderr, "file %s: ", current->dents[i]->d_name);
			perror("stat");
		}
	}
}

void update_lv_lists(void) {
	char name[32], *dash;
	int add;

	if (lvs != NULL) free_dir_list(lvs);
	if (lv_sets != NULL) free_str_list(lv_sets);

	lvs = malloc(sizeof(struct dir_list));
	lvs->n = scandir("/dev/store", &lvs->dents, is_lv, alphasort);

	lv_sets = new_str_list(lvs->n);

	lvs->stats = malloc(lvs->n * sizeof(struct stat *));
	for (int i = 0; i < lvs->n; ++i) lvs->stats[i] = NULL;

	for (int i = 0; i < lvs->n; ++i) {
		strncpy(name, lvs->dents[i]->d_name, 32);
		if (is_android(name)) {
			dash = strchr(name, '-');
			*dash = '\0';
			add = 1;
			for (int j = 0; j < lv_sets->used; ++j)
				if (!strcmp(name, lv_sets->data[j]))
					add = 0;
			if (add) append_to_str_list(lv_sets, name);

		}
	}
}

void free_dir_list(struct dir_list *list) {
	for (int i = 0; i < list->n; ++i) {
		if (list->dents[i] != NULL) free(list->dents[i]);
		if (list->stats[i] != NULL) free(list->stats[i]);
	}
	if (list->dents != NULL) free(list->dents);
	if (list->stats != NULL) free(list->stats);
	if (list != NULL) free(list);
}

int cmp_ext(char *file, char *ext) {
	char *dot = strrchr(file, '.');
	if (dot == NULL) return 1;
	return strcasecmp(dot, ext);
}

int cmp_base(char *file, char *base) {
	int ret;
	return strncasecmp(file, base, strchr(file, '.') - file - 1);
}

int is_lv(const struct dirent *dir) {
 	return isalpha(dir->d_name[0]);
}

int is_hidden(const struct dirent *dir) {
	if (dir->d_name[0] != '.') return 1; // include non-dot files.
	if ((dir->d_name[1] == '.') &&       // include parent dir.
		(dir->d_name[2] == '\0')) return 1;
	return 0;			    // exclude hiddens and current dir.
}

// this operates on LV names under /dev/store *only*
int is_android(char *name) {
	if (!strncmp(name, "android", 7)) return 1;
	if (!strncmp(name, "cm-", 3)) return 1;
	return 0;
}

int file_is_dir(int num) {
	return S_ISDIR(current->stats[num]->st_mode);
}

char * file_name(int num) {
	return current->dents[num]->d_name;
}

int file_count(void) {
	return current->n;
}

int lv_count(void) {
	return lvs->n;
}

int lv_set_count(void) {
	return lv_sets->used;
}

char * lv_name(int num) {
	return lvs->dents[num]->d_name;
}

char * lv_set_name(int num) {
	return lv_sets->data[num];
}
