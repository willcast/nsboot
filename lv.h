/*
	nsboot/lv.h
	header for Logical Volume related functions

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

#ifndef NSBOOT_LV_H
#define NSBOOT_LV_H

enum resizemode {
	RS_GROW,
	RS_SHRINK,
	RS_SET,
	RS_RECLAIM,
	RS_FREE
};

// Logical volume functions
void mount_lv(const char *);
void umount_lv(const char *);
void wipe_lv(const char *);
void delete_lv(const char *);
void new_lv(const char *, const long);
void lv_to_tgz(const char *, const char *);
void check_lv(const char *);
void resize_lv(const char *, enum resizemode, long);

char * deduce_lv(const char *);
long get_free_vg_space(void);
long get_free_lv_space(const char *);
long get_lv_size(const char *);
int is_lv_mounted(const char *);
int lv_exists(const char *);
char * get_lv_fstype(const char *);

#endif

