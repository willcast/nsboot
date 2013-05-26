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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <linux/limits.h>

#include "browse.h"
#include "fb.h"
#include "touch.h"
#include "screens.h"
#include "boot.h"
#include "install.h"

int main(int argc, char **argv) {
	int i;
	char ts_name[PATH_MAX];

	if (load_font("t.fnt")) exit(1);

	parse_proc_cmdline();

	if (argc < 2) strcpy(ts_name, "/dev/input/event6");
	else strcpy(ts_name, argv[1]);

	for (i = 0; i < 5; ++i) {
		if (!fb_init()) break;
		usleep(250000);
	}
	if (i == 5) exit(1);
	atexit(fb_close);

	for (i = 0; i < 5; ++i) {
		if (!ts_open(ts_name)) break;
		usleep(250000);
	}
	if (i == 5) exit(1);
	atexit(ts_close);

	mount_lv("media");
	mkdir("/mnt/media/nsboot", 0755);

	mount_lv("root");

	symlink_binaries();

	if (chdir("/mnt/media") == -1) perror("chdir /mnt/media");
	mkdir("/mnt/firmware", 0755);
	mount("/dev/mmcblk0p1", "/mnt/firmware", "vfat",
		MS_RDONLY | MS_SILENT, NULL);
	mkdir("/mnt/boot", 0755);
	mount("/dev/mmcblk0p13", "/mnt/boot", "ext4", MS_SILENT, NULL);

	set_brightness(16);

	main_menu();

	umount("/mnt/firmware");
	umount("/mnt/boot");
	umount_lv("media");
	umount_lv("root");
	exit(0);
}

int cmpstringp(const void *p1, const void *p2) {
	return strcasecmp(*(char*const*)p1, *(char*const*)p2);
}
