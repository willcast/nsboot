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

#ifndef NSBOOT_INSTALL_H
#define NSBOOT_INSTALL_H

#define WIPE_SYSTEM 	0x01
#define WIPE_DATA	0x02
#define WIPE_CACHE	0x04
#define INST_MOBOOT	0x08

// Utility functions
int test_file(const char *);

// Logical volume functions
void mount_lv(const char *);
void umount_lv(const char *);
int is_lv_mounted(const char *);
int lv_exists(const char *);
void wipe_lv(const char *);
void delete_lv(const char *);
void new_lv(const char *, const int);
long get_free_lvm_space(void);
long get_media_size(void);

// Android 3 volume group functions
void mount_lv_set(const char *);
void umount_lv_set(const char *);
void new_lv_set(const char *, int, int, int);
void wipe_lv_set(const char *);
void delete_lv_set(const char *);

void install_native(const char *, const char *);
void install_android(const char *, const char *, int);
void install_uimage(char *);
void install_tar(char *);

char * deduce_lv(const char *);
char * deduce_lv_set(const char *);
void deduce_lv_set_size(const char *, int *, int *, int *);

void resize_media(long);

void symlink_binaries(void);

#endif

