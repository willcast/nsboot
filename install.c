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
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>

#include "fb.h"
#include "install.h"

const char *update_str = "cat META-INF/com/google/android/updater-script | grep -v '^\\(assert\\)\\|\\(package_extract_.*\\)\\|\\(^mount\\)\\|\\(format\\)\\|\\(run_program\\)\\|\\(^umount\\)\\|\\(^unmount\\)\\|\\(show_progress\\)\\|\\(ui_print\\)\\|\\(^delete\\)' | sed 's/^set_perm(\\([0123456789]\\+\\), \\+\\([0123456789]\\+\\), \\([0123456789]\\+\\), \"\\(.*\\)\");/chown \\1:\\2 \\4 ; chmod \\3 \\4/' | sed 's/^set_perm_recursive(\\([0123456789]\\+\\), \\+\\([0123456789]\\+\\), \\([0123456789]\\+\\), \\([0123456789]\\+\\), \"\\(.*\\)\");/chown -R \\1:\\2 \\5 ; chmod -R \\3 \\5 ; find \\5 -type f -exec chmod \\4 {} \\\\;/' | sed 's/,$/ \\\\/' | sed 's/^symlink(\\(\"[A-z\\.\\-]*\"\\), /export LINK=\\1 ; for i in /' | sed 's/\\(\"\\)\\|\\(,\\)//g' | sed 's/);/ ; do ln -s $LINK $i ; done/' | sed 's/\\/system/system/g' > updatescript";

int test_file(const char *path) {
	struct stat test;

	if (stat(path, &test) == -1) return 0;
	return 1;
}

void mount_lv(const char *lv) {
	char dev[PATH_MAX], mtpt[PATH_MAX];

	stprintf("mounting volume %s", lv);

	sprintf(dev, "/dev/store/%s", lv);
	sprintf(mtpt, "/mnt/%s", lv);

	mkdir(mtpt, 0755);

	// only two relevant fstypes on HPTP.
	mount(dev, mtpt, "ext4", MS_SILENT, NULL);
	mount(dev, mtpt, "vfat", MS_SILENT, NULL);
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
	char cmd[PATH_MAX];
	int code;

	if (!is_lv_mounted("root")) mount_lv("root");
	umount_lv(lv);

	stprintf("making ext4 filesystem on volume %s", lv);

	sprintf(cmd, "mkfs.ext4 /dev/store/%s", lv);
	if (code = WEXITSTATUS(system(cmd)))
		steprintf("mkfs invocation failed with exit code %d", code);
}

void wipe_lv_set(const char *lv_set) {
	char lv[PATH_MAX];

	stprintf("wiping volumes starting with %s", lv_set);

	sprintf(lv, "%s-system", lv_set);
	wipe_lv(lv);
	sprintf(lv, "%s-data", lv_set);
	wipe_lv(lv);
	sprintf(lv, "%s-cache", lv_set);
	wipe_lv(lv);
}

void umount_lv_set(const char *lv_set) {
	char lv[PATH_MAX];

	stprintf("unmounting volumes starting with %s", lv_set);

	sprintf(lv, "%s-system", lv_set);
	umount_lv(lv);
	sprintf(lv, "%s-data", lv_set);
	umount_lv(lv);
	sprintf(lv, "%s-cache", lv_set);
	umount_lv(lv);
}

void delete_lv(const char *lv) {
	char cmd[PATH_MAX];
	int code;

	stprintf("deleting volume %s", lv);

	umount_lv(lv);

	sprintf(cmd, "lvm lvremove -f store/%s", lv);
	if (code = WEXITSTATUS(system(cmd)))
		steprintf("lvremove invocation failed with exit code %d", code);
}

void new_lv(const char *lv, const int size_mb) {
	char cmd[PATH_MAX];
	long space;
	int code;

	if (lv_exists(lv)) return;

	space = get_free_lvm_space();
	if (space == -1) return;

	if (space < size_mb) resize_media(space - size_mb);

	stprintf("creating a volume named %s with a size of %d MiB",
		lv, size_mb);
	sprintf(cmd, "lvm lvcreate -n %s -L %dM store", lv, size_mb);
	if (code = WEXITSTATUS(system(cmd)))
		steprintf("lvcreate invocation failed with exit code %d", code);
}

int lv_exists(const char *lv) {
	char dev[PATH_MAX];

	sprintf(dev, "/dev/store/%s", lv);
	return test_file(dev);
}

void mount_lv_set(const char *base) {
	char src[PATH_MAX], targ[PATH_MAX];

	stprintf("mounting volumes starting with %s", base);

	sprintf(targ, "/mnt/%s", base);
	mkdir(targ, 0755);

	sprintf(targ, "/mnt/%s/system", base);
	mkdir(targ, 0755);
	sprintf(src, "/dev/store/%s-system", base);
	if (mount(src, targ, "ext4", MS_SILENT, NULL) == -1)
		stperror("can't mount system volume");

	sprintf(targ, "/mnt/%s/data", base);
	mkdir(targ, 0755);
	sprintf(src, "/dev/store/%s-data", base);
	if (mount(src, targ, "ext4", MS_SILENT, NULL) == -1)
		stperror("can't mount data volume");

	sprintf(targ, "/mnt/%s/cache", base);
	mkdir(targ, 0755);
	sprintf(src, "/dev/store/%s-cache", base);
	if (mount(src, targ, "ext4", MS_SILENT, NULL) == -1)
		stperror("can't mount cache volume");
}

void install_native(const char *tarname, const char *lv, int size) {
	char cmd[PATH_MAX];
	int code;

	stprintf("installing new native Linux distro to %s", lv);

	if (lv == NULL) lv = deduce_lv(tarname);

	if (!lv_exists(lv))
		new_lv(lv, size);

	if (is_lv_mounted(lv))
		umount_lv(lv);

	wipe_lv(lv);
	mount_lv(lv);

	sprintf(cmd, "/mnt/%s", lv);
	if (chdir(cmd) == -1)
		stperror("can't chdir into target root");

	sprintf(cmd, "tar -xzpf %s", tarname, lv);
	status("extracting .tar.gz file, this may take 10+ minutes");
	if (code = WEXITSTATUS(system(cmd)))
		steprintf("extraction failed with exit code %d", code);
}

void install_android(const char *zipname, const char *base, int flags) {
	char cmd[PATH_MAX], pwd[PATH_MAX];
	int sys, dat, cac;			// volume sizes
	int code;

	stprintf("installing to %s from file %s", base, zipname);

	if (base == NULL) base = deduce_lv_set(zipname);

	umount_lv_set(base);

	sprintf(cmd, "%s-data", base);
	if (!lv_exists(cmd)) {
		deduce_lv_set_size(base, &sys, &dat, &cac);
		new_lv_set(base, sys, dat, cac);
		flags |= WIPE_SYSTEM | WIPE_DATA | WIPE_CACHE;
	}
	if (flags & WIPE_DATA)
		wipe_lv(cmd);

	if (flags & WIPE_SYSTEM) {
		sprintf(cmd, "%s-system", base);
		wipe_lv(cmd);
	}

	if (flags & WIPE_CACHE) {
		sprintf(cmd, "%s-cache", base);
		wipe_lv(cmd);
	}

	mount_lv_set(base);

	if (getcwd(pwd, PATH_MAX) == NULL)
		stperror("getcwd");
	sprintf(cmd, "/mnt/%s", base);
	if (chdir(cmd) == -1)
		stperror("chdir into root of new install");

	sprintf(cmd, "unzip -qo %s", zipname);
	status("extracting zip file");
	if (code = WEXITSTATUS(system(cmd)))
		steprintf("unzipping failed with exit code %d", code);;

	sprintf(cmd, "/mnt/%s/system/etc/firmware/q6.mdt", base);
	if (!test_file(cmd)) {
		mount_lv("firmware");
		status("copying firmware");
		sprintf(cmd,
			"cp /mnt/firmware/IMAGE/* /mnt/%s/system/etc/firmware",
			base);
		if (code = WEXITSTATUS(system(cmd)))
			steprintf("firmware copy failed with code %d", code);
	}

	// convert update script into shell script and run it
	status("converting updater script");
	if (code = WEXITSTATUS(system(update_str)))
		steprintf("conversion pipeline failed with code %d", code);
	status("running updater script");

	if (!is_lv_mounted("root")) mount_lv("root");
	chmod("updatescript", 0755);
	if (code = WEXITSTATUS(system("bash updatescript")))
		stprintf("updater script exited with code %d", code);

	sprintf(cmd, "/mnt/%s/boot.img", base);
	if (test_file(cmd) && (flags & INST_MOBOOT)) {
		sprintf(cmd, "/mnt/%s/moboot.splash.CyanogenMod.tga", base);
		if (test_file(cmd)) {
			status("installing moboot splash");
			sprintf(cmd,
				"cp moboot.splash.CyanogenMod.tga /mnt/boot/moboot.splash.%s.tga",
				base);
			if (code = WEXITSTATUS(system(cmd)))
				steprintf("splash copy failed with code %d", code);
		}

		status("patching uImage");
		if (code = WEXITSTATUS(system("uimage-extract boot.img")))
			steprintf("uImage extraction failed with code %d", code);
		if (code = WEXITSTATUS(system("mv ramdisk.img ramdisk.gz")))
			steprintf("renaming of ramdisk failed with code %d", code);
		if (code = WEXITSTATUS(system("gunzip ramdisk.gz")))
			steprintf("ramdisk gunzipping failed with code %d", code);

		mkdir("extracted", 0755);
		if (chdir("extracted") == -1)
			stperror("error changing directory into extraction area");
		if (code = WEXITSTATUS(system("cpio -i < ../ramdisk")))
			steprintf("extraction of ramdisk failed with code %d", code);

		sprintf(cmd, "sed -i -e 's/cm-system/%s-system/' -e "
	"'s/cm-data/%s-data/' -e 's/cm-cache/%s-cache/' init.tenderloin.rc",
		base, base, base);
		if (code = WEXITSTATUS(system(cmd)))
			steprintf("init script patching failed with code %s", code);
		if (code = WEXITSTATUS(system("find . | cpio -o --format=newc"
			"| lzma > ../ramdisk")))
			steprintf("repacking of ramdisk failed with code %d", code);

		if (chdir("..") == -1) stperror("changing into parent directory failed");
		if (code = WEXITSTATUS(system("mkimage -A arm -O linux -T kernel -C "
	"none -a 0x40208000 -e 0x40208000 -n kernel -d kernel.img uKernel")))
			steprintf("mkimage for kernel failed with code %d", code);
		if (code = WEXITSTATUS(system("mkimage -A arm -O linux -T ramdisk -C"
	" none -a 0x60000000 -e 0x60000000 -n ramdisk -d ramdisk uRamdisk")))
			steprintf("mkimage for ramdisk failed with code %d", code);
		if (code = WEXITSTATUS(system("mkimage -A arm -O linux -T multi -C "
	"none -a 0x40208000 -e 0x6000000 -n multi -d uKernel:uRamdisk uImage")))
			steprintf("mkimage for multi image failed with code %d", code);

		sprintf(cmd, "mv uImage /mnt/boot/uImage.%s", base);
		if (code = WEXITSTATUS(system(cmd)))
			steprintf("installation of patched uImage failed with code %d", code);
		unlink("kernel.img");
		unlink("ramdisk");
		unlink("uImage");
	}

	unlink("boot.img");
	unlink("updatescript");
	if (chdir(pwd) == -1) stperror("restoration of working directory failed");
}

void delete_lv_set(const char *set) {
	char lv[32];

	status("deleting volume set");

	sprintf(lv, "%s-system", set);
	delete_lv(lv);
	sprintf(lv, "%s-data", set);
	delete_lv(lv);
	sprintf(lv, "%s-cache", set);
	delete_lv(lv);
}

void install_tar(char *file) {
	char cmd[PATH_MAX], line[PATH_MAX], *lv;
	int code;
	FILE *cfg_fp;

	stprintf("installing kexec .tar file %s", file);
	if (code = WEXITSTATUS(system("rm -rf /mnt/tar")))
		steprintf("cleaning of work area failed with code %d", code);
	mkdir("/mnt/tar", 0755);

	sprintf(cmd, "tar -xf %s -C /mnt/tar", file);
	if (code = WEXITSTATUS(system(cmd)))
		steprintf("untarring failed with code %d", code);

	cfg_fp = fopen("/mnt/tar/smackme.cfg", "r");
	if (cfg_fp == NULL) return;

	if (fgets(line, sizeof(line), cfg_fp) == NULL)
		stperror("fgets on smackme.cfg failed");
	fclose(cfg_fp);

	// chomp newline
	// otherwise cp messes up in truly bizarre ways
	lv = strchr(line, '\n');
	*lv = '\0';

	// the install device in the cfg file is the whole path
	lv = strrchr(line, '/');
	if (lv == NULL) return;
	++lv;
	if (lv[0] == '\0') return;

	mount_lv(lv);

	sprintf(cmd, "cp -r /mnt/tar /mnt/%s/boot", lv);
	status(cmd);

	if (code = WEXITSTATUS(system(cmd)))
		steprintf("copy of archive contents failed with code %d", code);
}

void install_uimage(char *file) {
	char cmd[64];
	int code;

	stprintf("installing uImage file %s", file);

	sprintf(cmd, "cp %s /mnt/boot", file);
	if (code = WEXITSTATUS(system(cmd)))
		steprintf("uImage copy failed with code %d", code);
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

char * deduce_lv_set(const char *name) {
	char *base = basename(name);
	char *result = NULL;

	if ((!strncasecmp("Froyo-", base, 6)) ||
	   (!strncasecmp("gapps-mdpi", base, 9)))
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
	else if ((!strncasecmp("update-cm10", base, 10)) ||
		(!strncasecmp("update-cm-10", base, 11)) ||
		(!strncasecmp("cm-10", base, 4)) ||
		(!strncasecmp("gapps-jb-20120", base, 14)) ||
		(!strncasecmp("gapps-jb-201210", base, 14)))
		result = strdup("android41");
	else if ((!strncasecmp("ev", base, 2)) ||
		(!strncasecmp("gapps-jb", base, 9)))
		result = strdup("android42");

	return result;
}

// resize media partition, adding the (possibly negative)
// change to its current size. 0 maximizes the size.
void resize_media(long change) {
	char cmd[PATH_MAX];
	long size, new_size, space;
        int code;

	size = get_media_size();
	if (size == -1) return;
	space = get_free_lvm_space();
	if (space == -1) return;

	mount_lv("root");

	umount_lv("media");

	if (!change) {
		status("reclaiming media space");
		change = space;
	}

	new_size = size + change;

	if (change > 0) {
		stprintf("growing media volume by %ld MiB", change);
		sprintf(cmd, "lvm lvresize -fL %ld'M' store/media", new_size);
		if (code = WEXITSTATUS(system(cmd))) {
			steprintf("lvresize invocation failed with code %d", code); 
			mount_lv("media");
			return;
                }
	}

	stprintf("resizing vfat filesystem to %ld MiB", new_size);
	sprintf(cmd, "resizefat -f /dev/store/media %ld'M'", new_size);
	if (code = WEXITSTATUS(system(cmd))) {
                steprintf("resizefat invocation failed with code %d", code);
		mount_lv("media");
                return;
        }

	if (change < 0) {
		stprintf("shrinking media volume by %ld MiB", -change);
		sprintf(cmd, "lvm lvresize -fL %ld'M' store/media", new_size);
		if (code = WEXITSTATUS(system(cmd))) {
			steprintf("lvresize invocation failed with code %d", code); 
			mount_lv("media");
			return;
                }
	}

	mount_lv("media");
}

void new_lv_set(const char *name, int sys, int data, int cache) {
	char lv[32];
	long space;

	stprintf("creating volume set with name %s", name);

	space = get_free_lvm_space();
	if (space == -1) return;

	resize_media(space - sys - data - cache);

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

long get_free_lvm_space(void) {
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

long get_media_size(void) {
	FILE *lvm_fp;
	char line[128];
	long ret = 0;

	lvm_fp = popen(
		"lvm lvdisplay -c store/media | awk -F: '{print $7}'",
		"r");
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

	stprintf("current media volume size is %ld MiB", ret);
	return ret;
}

void symlink_binaries(void) {
	if (symlink("/mnt/root/usr/lib", "/usr/lib") == -1)
		stperror("symlink /usr/lib failed");
	if (symlink("/mnt/root/lib", "/lib") == -1)
		stperror("symlink /lib failed");
	if (symlink("/mnt/root/bin/resizefat", "/bin/resizefat") == -1)
		stperror("symlink resizefat failed");
	if (symlink("/mnt/root/sbin/mke2fs.e2fsprogs", "/bin/mkfs.ext4") == -1)
		stperror("symlink mkfs.ext4 failed");
	if (symlink("/mnt/root/sbin/dosfsck", "/bin/dosfsck"))
		stperror("symlink dosfsck failed");
}
