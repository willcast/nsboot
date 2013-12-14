/*
	nsboot/types.h
	header for useful common data types for nsboot and their supporting
	functions

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

#ifndef NSBOOT_TYPES_H
#define NSBOOT_TYPES_H

typedef struct {
	char **data;
	int size;
	int used;
} str_list;

// string list functions
 str_list * new_str_list(int size);
void free_str_list(str_list *);
void append_to_str_list(str_list *, char *);

#endif

