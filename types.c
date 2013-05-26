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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types.h"

struct str_list * new_str_list(int init_size) {
	struct str_list *ret;

	ret = malloc(sizeof(struct str_list));

	if (ret == NULL) {
		perror("can't allocate str_list");
		exit(1);
	}

	ret->size = init_size;
	ret->used = 0;
	ret->data = malloc(init_size * sizeof(char *));
	if (ret->data == NULL) {
		perror("can't allocate string data");
		exit(1);
	}

	for (int i = 0; i < init_size; ++i)
		ret->data[i] = NULL;

	return ret;
}

void free_str_list(struct str_list *list) {
	for (int i = 0; i < list->used; ++i)
		if (list->data[i] != NULL)
			free(list->data[i]);
	free(list->data);
	free(list);
}

void append_to_str_list(struct str_list *list, char *str) {
	if (list->used >= list->size) {
		int newsize = list->size * 2;
		char **newdata = realloc(list->data, newsize * sizeof(char *));
		if (newdata == NULL) {
			perror("can't realloc string data");
			exit(1);
		}
		list->data = newdata;

		for (int i = list->size; i < newsize; ++i)
			list->data[i] = NULL;

		list->size = newsize;
	}
	list->data[list->used] = strdup(str);
	++list->used;
}

