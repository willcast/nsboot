/*
	nsboot/install.c
	system installation related functions

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

#include <linux/limits.h>

#include "install.h"
#include "lib.h"
#include "log.h"
#include "lv.h"
#include "lvset.h"

const char *update_str = "cat META-INF/com/google/android/updater-script"
"| grep -v '^\\(assert\\)\\|\\(package_extract_.*\\)\\|\\(^mount\\)\\|\\(format\\)\\|\\(run_program\\)\\|\\(^umount\\)\\|\\(^unmount\\)\\|\\(show_progress\\)\\|\\(ui_print\\)\\|\\(^delete\\)' "
"| sed 's/^set_perm(\\([0123456789]\\+\\), \\+\\([0123456789]\\+\\), \\([0123456789]\\+\\), \"\\(.*\\)\");/chown \\1:\\2 \\4 ; chmod \\3 \\4/' | sed 's/^set_perm_recursive(\\([0123456789]\\+\\), \\+\\([0123456789]\\+\\), \\([0123456789]\\+\\), \\([0123456789]\\+\\), \"\\(.*\\)\");/chown -R \\1:\\2 \\5 ; chmod -R \\3 \\5 ; find \\5 -type f -exec chmod \\4 {} \\\\;/' "
"| sed 's/,$/ \\\\/' "
"| sed 's/^symlink(\\(\"[\\/A-z\\.\\-]*\"\\), /export LINK=\\1 ; for i in /' "
"| sed 's/\\(\"\\)\\|\\(,\\)//g' "
"| sed 's/);/ ; do ln -s $LINK $i ; done/' "
"| sed 's/\\/system/system/g'"
"> updatescript";

void install_native(const char *tarname, const char *lv, int size) {
	char cmd[PATH_MAX];
	int code;

	logprintf("0installing new native Linux distro to %s", lv);

	if (lv == NULL) lv = deduce_lv(tarname);

	if (!lv_exists(lv)) {
		if (size > 0) new_lv(lv, size);
		else {
			logprintf("2volume does not exist");
			return;
		}
	}

	umount_lv(lv);

	if (size != -1) wipe_lv(lv);
	mount_lv(lv);

	snprintf(cmd, sizeof(cmd), "/mnt/%s", lv);
	if (chdir(cmd) == -1)
		logperror("can't chdir into target root");

	logprintf("0extracting .tar.gz file, this may take 10+ minutes");
	sysprintf("tar -xzpf %s", tarname);
}

void install_android(const char *zipname, const char *base, int flags) {
	char cmd[PATH_MAX], pwd[PATH_MAX];
	int sys, dat, cac;			// volume sizes
	int code;

	logprintf("0installing to %s from file %s", base, zipname);

	if (base == NULL) base = deduce_lv_set(zipname);

	umount_lv_set(base);

	snprintf(cmd, sizeof(cmd), "%s-data", base);
	if (!lv_exists(cmd)) {
		deduce_lv_set_size(base, &sys, &dat, &cac);
		new_lv_set(base, sys, dat, cac);
		flags |= WIPE_SYSTEM | WIPE_DATA | WIPE_CACHE;
	}
	if (flags & WIPE_DATA)
		wipe_lv(cmd);

	if (flags & WIPE_SYSTEM) {
		snprintf(cmd, sizeof(cmd), "%s-system", base);
		wipe_lv(cmd);
	}

	if (flags & WIPE_CACHE) {
		snprintf(cmd, sizeof(cmd), "%s-cache", base);
		wipe_lv(cmd);
	}

	mount_lv_set(base);

	if (getcwd(pwd, PATH_MAX) == NULL)
		logperror("getcwd");
	snprintf(cmd, sizeof(cmd), "/mnt/%s", base);
	if (chdir(cmd) == -1)
		logperror("chdir into root of new install");

	logprintf("0extracting zip file");
	sysprintf("unzip -qo %s", zipname);

	snprintf(cmd, sizeof(cmd), "/mnt/%s/system/etc/firmware/q6.mdt", base);
	if (!test_file(cmd)) {
		mount_lv("firmware");
		logprintf("0copying firmware");
		sysprintf("cp /mnt/firmware/IMAGE/* /mnt/%s/system/etc/firmware",
			base);
	}

	// convert update script into shell script and run it
	logprintf("0converting updater script");
	system_logged(update_str);
	logprintf("0running updater script");

	if (!is_lv_mounted("root")) mount_lv("root");
	chmod("updatescript", 0755);
	system_logged("bash updatescript");

	snprintf(cmd, sizeof(cmd), "/mnt/%s/boot.img", base);
	if (test_file(cmd) && (flags & INST_MOBOOT)) {
		snprintf(cmd, sizeof(cmd),
			"/mnt/%s/moboot.splash.CyanogenMod.tga", base);
		if (test_file(cmd)) {
			logprintf("0installing moboot splash");
			sysprintf("cp moboot.splash.CyanogenMod.tga "
				"/mnt/boot/moboot.splash.%s.tga",
				base);
		}

		logprintf("0patching uImage");
		system_logged("uimage-extract boot.img");
		system_logged("mv ramdisk.img ramdisk.gz");
		system_logged("gunzip ramdisk.gz");

		mkdir("extracted", 0755);
		if (chdir("extracted") == -1)
			logperror("error changing directory into extraction "
				" area");
		system_logged("cpio -i < ../ramdisk");

		sysprintf("sed -i -e 's/cm-system/%s-system/'"
			" -e 's/cm-data/%s-data/' -e 's/cm-cache/%s-cache/' "
			"init.tenderloin.rc", base, base, base);

		system_logged("find . | cpio -o --format=newc | lzma > ../ramdisk");

		if (chdir("..") == -1)
			logperror("changing into parent directory failed");

		system_logged("mkimage -A arm -O linux -T kernel -C none "
			"-a 0x40208000 -e 0x40208000 -n kernel -d kernel.img "
			"uKernel");
		system_logged("mkimage -A arm -O linux -T ramdisk -C none "
			"-a 0x60000000 -e 0x60000000 -n nsboot_patched_ramdisk "
			"-d ramdisk uRamdisk");
		system_logged("mkimage -A arm -O linux -T multi -C none "
			"-a 0x40208000 -e 0x40208000 -n nsboot_patched_multi "
			"-d uKernel:uRamdisk uImage");

		sysprintf("mv uImage /mnt/boot/uImage.%s", base);

		unlink("kernel.img");
		unlink("ramdisk");
		unlink("uImage");
	}

	if (chdir(pwd) == -1) logperror("restoration of working directory failed");
}

void delete_lv_set(const char *set) {
	char lv[32];

	logprintf("0deleting volume set %s", set);

	snprintf(lv, sizeof(lv), "%s-system", set);
	delete_lv(lv);
	snprintf(lv, sizeof(lv), "%s-data", set);
	delete_lv(lv);
	snprintf(lv, sizeof(lv), "%s-cache", set);
	delete_lv(lv);
}

void install_tar(const char *file, const char *instlv) {
	char cmd[PATH_MAX], line[PATH_MAX], *lv;
	int code;
	FILE *cfg_fp;

	logprintf("0installing kexec .tar file %s", file);
	system_logged("rm -rf /mnt/tar");
	mkdir("/mnt/tar", 0755);

	sysprintf("tar -xf %s -C /mnt/tar", file);

	cfg_fp = fopen("/mnt/tar/smackme.cfg", "r");
	if (cfg_fp == NULL) return;

	if (fgets(line, sizeof(line), cfg_fp) == NULL)

	logperror("fgets on smackme.cfg failed");
	fclose(cfg_fp);

	// chomp newline...
	// otherwise cp messes up in truly bizarre ways!
	lv = strchr(line, '\n');
	*lv = '\0';

	// the install device in the cfg file is the whole path
	lv = strrchr(line, '/');

	if (lv == NULL) return;
	++lv;
	if (lv[0] == '\0') return;

	if (instlv != NULL) {
		logprintf("1changing root LV from %s to %s", lv, instlv);
		sysprintf("sed -ie 's/%s/%s/' /mnt/tar/boot.cfg", lv, instlv);

		lv = strdup(instlv);
	}

	mount_lv(lv);

	sprintf(cmd, "/mnt/%s/boot", lv);
	mkdir(cmd, 0755);

	sysprintf("cp -f /mnt/tar/* /mnt/%s/boot/", lv);
}

void install_uimage(char *file) {
	char cmd[PATH_MAX];
	int code;
	int verbose_fd;
	char *dot;

	logprintf("0installing uImage file %s", file);

	dot = strchr(file, '.');
	// we do NOT want to let users install to /boot/uImage!
	if (dot == NULL) return;
	++dot;

	sysprintf("cp %s /mnt/boot", file);

	sprintf(cmd, "/mnt/boot/moboot.verbose.%s", dot);
	qfprintf(cmd, "yes");

	return;
}

void replace_moboot(void) {
	int code;

	logprintf("0replacing noboot with nsboot");

	if (!test_file("/mnt/boot/uImage.nsboot")) {
		logprintf("2nsboot is not installed in boot");
		return;
	}

	if (chdir("/mnt/boot") == -1) {
		logperror("error changing directory to boot");
		return;
	}

	system_logged("mv uImage uImage.old");

	if (test_file("uImage") || !test_file("uImage.old")) {
		logprintf("2relocation of old bootloader failed!");
		logprintf("3%s", "WARNING: YOU MAY NEED TO USE NOVACOM TO REINSTALL A BOOTLOADER");
		sleep(1);
		return;
	}

	if (symlink("uImage.nsboot", "uImage") == -1) {
		logperror("error symlinking nsboot as main bootloader!");
		logprintf("3%s", "WARNING: YOU MAY NEED TO USE NOVACOM TO REINSTALL A BOOTLOADER");
		sleep(1);
	}
}
