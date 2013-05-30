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
#include "install.h"

void mount_lv(const char *lv) {
	char dev[PATH_MAX], mtpt[PATH_MAX];
	char *type;

	stprintf("mounting volume %s", lv);

	sprintf(dev, "/dev/store/%s", lv);
	sprintf(mtpt, "/mnt/%s", lv);

	mkdir(mtpt, 0755);

	type = get_lv_fstype(lv);

	mount(dev, mtpt, type, MS_SILENT, NULL);
}

// /proc/mounts displays the real device location, not the symbolic
// link we used to mount it, so readlink is necessary.
int is_lv_mounted(const char *lv) {
	FILE *mounts_fp;
	char mount_str[128], real_dev[64], dev[32];
	int nchars;

	sprintf(dev, "/dev/store/%s", lv);
	nchars = readlink(dev, real_dev, sizeof(real_dev) - 1);
	real_dev[nchars] = '\0';

	mounts_fp = fopen("/proc/mounts", "r");

	while (!feof(mounts_fp)) {
		if (fgets(mount_str, 128, mounts_fp) == NULL) return 0;
		if (!strncmp(mount_str, real_dev, nchars - 1)) {
			fclose(mounts_fp);
			return 1;
		}
	}

	fclose(mounts_fp);
	return 0;
}

void umount_lv(const char *lv) {
	char target[64];
	char *dash;

	stprintf("unmounting volume %s", lv);

	// Unmount it if it is mounted directly under /mnt.
	sprintf(target, "/mnt/%s", lv);

	while (!umount(target)) ;

	// Unmount it if it is mounted one level deep (aka if
	// it was mounted by mount_lv_set()).
	dash = strchr(target, '-');
	if (dash == NULL) return;

	*dash = '/';
	while (!umount(target)) ;
}

void wipe_lv(const char *lv) {
	char cmd[PATH_MAX], *type;
	int code;

	if (!strcmp(lv, "root")) {
		status_error("root volume may not be wiped");
		return;
	}

	if (!is_lv_mounted("root")) mount_lv("root");
	umount_lv(lv);

	if (!strcmp(type, "swap"))
		snprintf(cmd, sizeof(cmd), "mkswap /dev/store/%s", lv);
	else if (!strcmp(type, "msdos") || !strcmp(type, "vfat"))
		snprintf(cmd, sizeof(cmd), "mkdosfs /dev/store/%s", lv);
	else if (!strncmp(type, "ext", 3))
		sprintf(cmd, "mke2fs -t %s /dev/store/%s", type, lv);
	else
		sprintf(cmd, "mke2fs -t ext4 /dev/store/%s", lv);

	stprintf("making %s filesystem on volume %s", type, lv);

	if (code = WEXITSTATUS(system(cmd)))
		steprintf("mkfs invocation failed with exit code %d", code);
}

void delete_lv(const char *lv) {
	char cmd[PATH_MAX];
	int code;

	if (!strcmp(lv, "root")) {
		status_error("root volume may not be removed");
		return;
	}

	stprintf("deleting volume %s", lv);

	umount_lv(lv);

	sprintf(cmd, "lvm lvremove -f store/%s", lv);
	if (code = WEXITSTATUS(system(cmd)))
		steprintf("lvremove invocation failed with exit code %d", code);
}

void new_lv(const char *lv, const  long size_mb) {
	char cmd[PATH_MAX];
	long space;
	int code;

	if (lv_exists(lv)) return;

	space = get_free_vg_space();
	if (space == -1) return;

	if (space < size_mb) resize_lv("media", RS_FREE, size_mb);

	stprintf("creating a volume named %s with a size of %ld MiB",
		lv, size_mb);
	sprintf(cmd, "lvm lvcreate -n %s -L %ld'M' store", lv, size_mb);
	if (code = WEXITSTATUS(system(cmd)))
		steprintf("lvcreate invocation failed with exit code %d", code);
}

int lv_exists(const char *lv) {
	char dev[PATH_MAX];

	sprintf(dev, "/dev/store/%s", lv);
	return test_file(dev);
}

char * deduce_lv(const char *filename) {
	char *base = basename(filename);
	char *result = NULL;

	if (!strncasecmp("TouchPadBuntuRootfs", base, 19))
		result = strdup("ubuntu-root");
	else if (!strncasecmp("ArchLinuxARM-touchpad", base, 21))
		result = strdup("arch-root");
	else if (!strncasecmp("bodhi-touchpad", base, 14))
		result = strdup("bodhi-root");
	else if (!strncasecmp("HPTPSlackware", base, 13))
		result = strdup("slackware-root");
	else if (!strncasecmp("HPTPFedora", base, 10))
		result = strdup("fedora-root");

	return result;
}

void resize_lv(const char *lv, enum resizemode mode, long arg) {
	char cmd[PATH_MAX], *type;
	long oldsize, newsize, space, free;
        int code;

	oldsize = get_lv_size(lv);
	if (oldsize == -1) return;
	space = get_free_vg_space();
	if (space == -1) return;
	free = get_free_lv_space(lv);
	if (free == -1) return;
	type = get_lv_fstype(lv);
	if (!strcmp(type, "none")) return;

	mount_lv("root");

	arg = abs(arg);

	switch (mode) {
	case RS_GROW:
		newsize = oldsize + arg;
		stprintf("growing volume %s to %ld MiB", lv, newsize);
		break;
	case RS_SHRINK:
		newsize = oldsize - arg;
		stprintf("shrinking volume %s to %ld MiB", lv, newsize);
		break;
	case RS_SET:
		newsize = arg;
		stprintf("resizing volume %s to %ld MiB", lv, newsize);
		break;
	case RS_RECLAIM:
		newsize = oldsize + space;
		stprintf("reclaming %ld MiB of free space into volume %s", space, lv);
		break;
	case RS_FREE:
		newsize = oldsize + space - arg;
		stprintf("making %ld MiB of space by shrinking volume %s", arg, lv);
		break;
	}

	if (free < oldsize - newsize) {
		steprintf("free space is too low to shrink, %ld MiB more is needed",
			oldsize - newsize - free);
		return;
	}
	if (strncmp(type, "ext", 3) || (newsize < oldsize)) umount_lv(lv);
	if (!strcmp(lv, "root") && (newsize < oldsize)) {
		status_error("root volume may not be shrunken");
		return;
	}

	if (newsize > oldsize + space) resize_lv("media", RS_FREE, newsize - oldsize);

	check_lv(lv);

	if (newsize > oldsize) {
		sprintf(cmd, "lvm lvresize -fL %ld'M' store/%s", newsize, lv);
		if (code = WEXITSTATUS(system(cmd))) {
			steprintf("lvresize invocation failed with code %d", code); 
			return;
                }
	}

	umount_lv(lv);
	if (!strcmp(type, "vfat") || !strcmp(type, "msdos")) {
		status("resizing FAT filesystem");
		sprintf(cmd, "resizefat /dev/store/%s %ld'M'", lv, newsize);
		if (code = WEXITSTATUS(system(cmd))) {
        	        steprintf("resizefat invocation failed with code %d", code);
	                return;
        	}
	} else if (!strncmp(type, "ext", 3)) {
		stprintf("resizing %s filesystem", type);
		sprintf(cmd, "resize2fs /dev/store/%s %ld'M'", lv, newsize);
		if (code = WEXITSTATUS(system(cmd))) {
			steprintf("resize2fs invocation failed with code %d", code);
			return;
		}
	}

	if (newsize < oldsize) {
		sprintf(cmd, "lvm lvresize -fL %ld'M' store/%s", newsize, lv);
		if (code = WEXITSTATUS(system(cmd))) {
			steprintf("lvresize invocation failed with code %d", code); 
			return;
                }
	}

	if (!strcmp(type, "swap")) wipe_lv(lv);

	mount_lv(lv);
}

long get_free_vg_space(void) {
	FILE *lvm_fp;
	char line[128];
	long ret = 0;

	lvm_fp = popen(
		"lvm vgs -o free --units m store | tail -n 1 | awk '{print $1}'"
		, "r");
	if (lvm_fp == NULL) {
		stperror("error invoking lvm vgs pipeline");
		return -1;
	}
	if (fgets(line, sizeof(line), lvm_fp) == NULL) {
		stperror("error reading lvm vgs pipeline");
		fclose(lvm_fp);
		return -1;
	}

	fclose(lvm_fp);
	ret = atol(line);

	stprintf("found %ld MiB of free LVM space", ret);
	return ret;
}

long get_lv_size(const char *lv) {
	FILE *lvm_fp;
	char cmd[PATH_MAX], line[128];
	long ret = 0;

	snprintf(cmd, sizeof(cmd), "lvm lvdisplay -c store/%s |"
		"awk -F: '{print $7}'", lv);
	lvm_fp = popen(cmd, "r");
	if (lvm_fp == NULL) {
		stperror("error invoking lvdisplay pipeline");
		return -1;
	}
	if (fgets(line, sizeof(line), lvm_fp) == NULL) {
		stperror("error reading lvdisplay pipeline");
		fclose(lvm_fp);
		return -1;
	}

	fclose(lvm_fp);
	ret = atol(line) / 2048;

	stprintf("current size of volume %s is %ld MiB", lv, ret);
	return ret;
}

void lv_to_tgz(const char *lv, const char *bname) {
	int code;
	char cmd[PATH_MAX];

	stprintf("backing up volume %s to basename %s", lv, bname);

	mount_lv("media");
	mount_lv(lv);

	mkdir("/mnt/media/nsboot/backups", 0755);

	sprintf(cmd, "/mnt/%s", lv);
	if (chdir(cmd) == -1) {
		stperror("can't chdir into source fs");
		return;
	}

	snprintf(cmd, sizeof(cmd),
		 "tar --numeric-owner -czpf /mnt/media/nsboot/backups/%s.tar.gz .",
		bname);

	if (code = WEXITSTATUS(system(cmd)))
		steprintf("tar creation failed with code %d", code);
}

char * get_lv_fstype(const char *lv) {
	FILE *blkid_fp;
	char cmd[PATH_MAX], *ret, *quote;

	stprintf("determining fstype for lv %s", lv);

	snprintf(cmd, sizeof(cmd), "blkid /dev/store/%s", lv);
	blkid_fp = popen(cmd, "r");
	if (blkid_fp == NULL) {
		stperror("error opening pipe to blkid");
		return "?";
	}

	if (fgets(cmd, sizeof(cmd), blkid_fp) == NULL) {
		stperror("error reading from blkid pipe");
		fclose(blkid_fp);
		return "?";
	}

	fclose(blkid_fp);

	// chomp trailing newline just in case
	ret = strchr(cmd, '\n');
	if (ret != NULL) *ret = '\0';

	// only care about fstype
	ret = strstr(cmd, "TYPE=");
	if (ret == NULL) return "none";

	// start right after 1st quote
	ret = strchr(ret, '\"');
	if (ret == NULL) return "none";
	++ret;

	// end right before 2nd quote
	quote = strchr(ret, '\"');
	if (quote != NULL) *quote = '\0';

	for (int i = 0; i < strlen(ret); ++i)
		ret[i] = tolower(ret[i]);

	stprintf("blkid detected fstype of %s", ret);

	return strdup(ret);
}

void check_lv(const char *lv) {
	int code;
	char cmd[PATH_MAX], *type;

	stprintf("checking filesystem on volume %s", lv);

	umount_lv(lv);

	type = get_lv_fstype(lv);
	if (!strcmp(type, "none") || !strcmp(type, "swap")) return;
	else if (!strncmp(type, "ext", 3))
		snprintf(cmd, sizeof(cmd), "e2fsck -fy /dev/store/%s", lv);
	else if (!strcmp(type, "vfat") || !strcmp(type, "msdos"))
		snprintf(cmd, sizeof(cmd), "dosfsck -aw /dev/store/%s", lv);
	else {
		steprintf("error: unknown fstype %s", type);
		return;
	}

	code = WEXITSTATUS(system(cmd));
	if (code > 7) {
		steprintf("error: fsck quit with abnormal code %d", code);
		return;
	}
	if (code & 1) status_error("warning: errors were fixed");
	if (code & 4) status_error("warning: errors were left unfixed");
	if (code & 2) {
		status_error("system restart was requested, will reboot in 5s");
		sleep(3);
		sync();
		reboot(RB_AUTOBOOT);
	}
}

long get_free_lv_space(const char *lv) {
	FILE *df_fp;
	char cmd[PATH_MAX];
	long ret;

	mount_lv(lv);

	snprintf(cmd, sizeof(cmd), "df -m /dev/store/%s | awk '{print $4}'", lv);
	df_fp = popen(cmd, "r");

	if (df_fp == NULL) {
		stperror("can't open pipe from df");
		return -1;
	}

	if (fgets(cmd, sizeof(cmd), df_fp) == NULL) {
		stperror("can't read from df pipe");
		fclose(df_fp);
		return -1;
	}

	fclose(df_fp);
	ret = atol(cmd);

	stprintf("found %ld MiB of free space on volume %s", ret, lv);
	return ret;
}
