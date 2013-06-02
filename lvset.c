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

#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>

#include "fb.h"
#include "lv.h"
#include "lvset.h"
#include "install.h"
#include "log.h"

void wipe_lv_set(const char *lv_set) {
	char lv[PATH_MAX];

	logprintf("0wiping volumes starting with %s", lv_set);

	sprintf(lv, "%s-system", lv_set);
	wipe_lv(lv);
	sprintf(lv, "%s-data", lv_set);
	wipe_lv(lv);
	sprintf(lv, "%s-cache", lv_set);
	wipe_lv(lv);
}

void umount_lv_set(const char *lv_set) {
	char lv[PATH_MAX];

	logprintf("0unmounting volumes starting with %s", lv_set);

	sprintf(lv, "%s-system", lv_set);
	umount_lv(lv);
	sprintf(lv, "%s-data", lv_set);
	umount_lv(lv);
	sprintf(lv, "%s-cache", lv_set);
	umount_lv(lv);
}

void mount_lv_set(const char *base) {
	char src[PATH_MAX], targ[PATH_MAX];

	logprintf("0mounting volumes starting with %s", base);

	sprintf(targ, "/mnt/%s", base);
	mkdir(targ, 0755);

	sprintf(targ, "/mnt/%s/system", base);
	mkdir(targ, 0755);
	sprintf(src, "/dev/store/%s-system", base);
	if (mount(src, targ, "ext4", MS_SILENT, NULL) == -1)
		logperror("can't mount system volume");

	sprintf(targ, "/mnt/%s/data", base);
	mkdir(targ, 0755);
	sprintf(src, "/dev/store/%s-data", base);
	if (mount(src, targ, "ext4", MS_SILENT, NULL) == -1)
		logperror("can't mount data volume");

	sprintf(targ, "/mnt/%s/cache", base);
	mkdir(targ, 0755);
	sprintf(src, "/dev/store/%s-cache", base);
	if (mount(src, targ, "ext4", MS_SILENT, NULL) == -1)
		logperror("can't mount cache volume");
}

char * deduce_lv_set(const char *name) {
	char *base = basename(name);
	char *result = NULL;

	if ((!strncasecmp("froyo-", base, 6)) ||
	   (!strncasecmp("gapps-mdpi", base, 10)))
		result = strdup("android22");
	else if ((!strncasecmp("update-cm7", base, 10)) ||
		(!strncasecmp("update-cm-7", base, 11)) ||
		(!strncasecmp("cm-7", base, 4)) ||
		(!strncasecmp("gapps-gb", base, 8)))
		result = strdup("android23");
	else if ((!strncasecmp("update-cm9", base, 10)) ||
		(!strncasecmp("update-cm-9", base, 11)) ||
		(!strncasecmp("cm-9", base, 4)) ||
		(!strncasecmp("gapps-ics", base, 9)))
		result = strdup("android40");
	else if ((!strncasecmp("update-cm10-", base, 10)) ||
		(!strncasecmp("update-cm-10-", base, 11)) ||
		(!strncasecmp("cm-10-", base, 6)) ||
		(!strncasecmp("skz_tenderloin-1.20", base, 19)) ||
		(!strncasecmp("gapps-jb-20120", base, 14)) ||
		(!strncasecmp("gapps-jb-201210", base, 15)))
		result = strdup("android41");
	else if ((!strncasecmp("ev", base, 2)) ||
		(!strncasecmp("cm-10.1", base, 7)) ||
		(!strncasecmp("skz_tenderloin-2.00", base, 19)) ||
		(!strncasecmp("gapps-jb", base, 8)))
		result = strdup("android42");

	return result;
}

void new_lv_set(const char *name, int sys, int data, int cache) {
	char lv[32];
	long space;

	logprintf("0%s", "creating volume set with name %s", name);

	space = get_free_vg_space();
	if (space == -1) return;

	resize_lv("media", RS_FREE, sys + data + cache);

	sprintf(lv, "%s-system", name);
	new_lv(lv, sys);

	sprintf(lv, "%s-data", name);
	new_lv(lv, data);

	sprintf(lv, "%s-cache", name);
	new_lv(lv, cache);
}

void deduce_lv_set_size(const char *name, int *sys, int *dat, int *cac) {
	if (!strcmp(name, "android22")) {
		*sys = 256;	*dat = 448;	*cac = 200;
	} else if (!strcmp(name, "android23")) {
		*sys = 304;	*dat = 504;	*cac = 200;
	} else if (!strcmp(name, "android40")) {
		*sys = 352;	*dat = 754;	*cac = 200;
	} else if (!strcmp(name, "android41")) {
		*sys = 448;	*dat = 754;	*cac = 200;
	} else if (!strcmp(name, "android42")) {
		*sys = 448;	*dat = 754;	*cac = 200;
	}
}
