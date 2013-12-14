/*
	nsboot/lvset.h
	header for Logical Volume Set related functions

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

#ifndef NSBOOT_LVSET_H
#define NSBOOT_LVSET_H

// Android 3 volume group functions
void mount_lv_set(const char *);
void umount_lv_set(const char *);
void new_lv_set(const char *, int, int, int);
void wipe_lv_set(const char *);
void delete_lv_set(const char *);

char * deduce_lv_set(const char *);
void deduce_lv_set_size(const char *, int *, int *, int *);

#endif

