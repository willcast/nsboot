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

const char *update_str = "cat META-INF/com/google/android/updater-script | grep -v '^\\(assert\\)\\|\\(package_extract_.*\\)\\|\\(^mount\\)\\|\\(format\\)\\|\\(run_program\\)\\|\\(^umount\\)\\|\\(^unmount\\)\\|\\(show_progress\\)\\|\\(ui_print\\)\\|\\(^delete\\)' | sed 's/^set_perm(\\([0123456789]\\+\\), \\+\\([0123456789]\\+\\), \\([0123456789]\\+\\), \"\\(.*\\)\");/chown \\1:\\2 \\4 ; chmod \\3 \\4/' | sed 's/^set_perm_recursive(\\([0123456789]\\+\\), \\+\\([0123456789]\\+\\), \\([0123456789]\\+\\), \\([0123456789]\\+\\), \"\\(.*\\)\");/chown -R \\1:\\2 \\5 ; chmod -R \\3 \\5 ; find \\5 -type f -exec chmod \\4 {} \\\\;/' | sed 's/,$/ \\\\/' | sed 's/^symlink(\\(\"[A-z\\.\\-]*\"\\), /export LINK=\\1 ; for i in /' | sed 's/\\(\"\\)\\|\\(,\\)//g' | sed 's/);/ ; do ln -s $LINK $i ; done/' | sed 's/\\/system/system/g' > updatescript";

int test_file(const char *path) {
	struct stat test;

	if (stat(path, &test) == -1) return 0;
	return 1;
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

	sprintf(cmd, "/mnt/%s/boot", lv);
	mkdir(cmd, 0755);

	sprintf(cmd, "cp -f /mnt/tar/* /mnt/%s/boot/", lv);
	status(cmd);

	if (code = WEXITSTATUS(system(cmd)))
		steprintf("copy of archive contents failed with code %d", code);
}

void install_uimage(char *file) {
	char cmd[PATH_MAX];
	int code;
	int verbose_fd;
	char *dot;

	stprintf("installing uImage file %s", file);

	dot = strchr(file, '.');
	// we do NOT want to let users install to /boot/uImage!
	if (dot == NULL) return;
	++dot;

	sprintf(cmd, "cp %s /mnt/boot", file);
	if (code = WEXITSTATUS(system(cmd))) {
		steprintf("uImage copy failed with code %d", code);
		return;
	}

	sprintf(cmd, "/mnt/boot/moboot.verbose.%s", dot);
	verbose_fd = open(cmd, O_WRONLY | O_TRUNC);
	if (verbose_fd < 0) {
		stperror("can't open moboot.verbose");
		return;
	}
	if (write(verbose_fd, "yes", 4) != 4)
		stperror("can't write to moboot.verbose");

	close(verbose_fd);
	return;
}

void replace_moboot(void) {
	int code;

	status("replacing noboot with nsboot");

	if (!test_file("/mnt/boot/uImage.nsboot")) {
		steprintf("nsboot is not installed in boot");
		return;
	}

	if (chdir("/mnt/boot") == -1) {
		stperror("error changing directory to boot");
		return;
	}

	if (code = WEXITSTATUS(system("mv uImage uImage.old"))) {
		steprintf("relocation of old bootloader failed with code %d", code);
		status_error("WARNING: YOU MAY NEED TO USE NOVACOM TO REINSTALL A BOOTLOADER");
		return;
	}

	if (symlink("uImage.nsboot", "uImage") == -1) {
		stperror("error symlinking nsboot as main bootloader");
		status_error("WARNING: YOU MAY NEED TO USE NOVACOM TO REINSTALL A BOOTLOADER");
	}
}
