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

#include <sys/signal.h>
#include <sys/resource.h>
#include <linux/limits.h>

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
#include <sys/time.h>

#include "browse.h"
#include "fb.h"
#include "touch.h"
#include "screens.h"
#include "boot.h"
#include "init.h"
#include "lv.h"
#include "log.h"

void do_init(void) {
	char ts_name[PATH_MAX];
	int code, i;

	atexit(do_cleanup);

	putenv("PATH=/bin");

	strcpy(ts_name, "/dev/event2");

	init_log();
	logprintf("0welcome to nsboot");

	logprintf("0symlinking binaries");
	symlink_binaries();

	logprintf("0parsing /proc/cmdline");
	parse_proc_cmdline();

	logprintf("0mounting virtual filesystems");

	mkdir("/dev", 0755);
	logprintf("0 - /dev");
	mount("tmpfs", "/dev", "tmpfs", MS_SILENT, NULL);

	mkdir("/dev/pts", 0755);
	logprintf("0 - /dev/pts");
	mount("none", "/dev/pts", "devpts", MS_SILENT, NULL);

	mkdir("/proc", 0755);
	logprintf("0 - /proc");
	mount("none", "/proc", "proc", MS_SILENT, NULL);

	mkdir("/sys", 0755);
	logprintf("0 - /sys");
	mount("none", "/sys", "sysfs", MS_SILENT, NULL);

	logprintf("0disabling kernel messages");
	code = WEXITSTATUS(system("echo 2 > /proc/sys/kernel/printk"));

	logprintf("0enabling core dumps");
	enable_coredumps();

	logprintf("0installing busybox");
	if (code = WEXITSTATUS(system("busybox --install -s"))) {
		logprintf("3busybox install failed with code %d", code);
		exit(1);
	}

	logprintf("0starting hotplug");
	code = system("touch /etc/mdev.conf");
	code = system("echo /bin/mdev > /proc/sys/kernel/hotplug");
	if (code = WEXITSTATUS(system("mdev -s")))
		logprintf("2hotplug failed with code %d", code);

	logprintf("0loading font file");
	if (load_font("/t.fnt")) {
		logprintf("3failed to load font");
		exit(1);
	}

	for (i = 0; i < 5; ++i) {
		logprintf("0initializing graphics - attempt %d", i);
		if (!fb_init()) break;
		logprintf("2 - attempt failed!");
		usleep(250000);
	}
	if (i == 5) {
		logprintf("3giving up on FB!");
		exit(1);
	}
	atexit(fb_close);

	logprintf("0starting adbd");
	code = system("echo 1 > /sys/class/usb_composite/adb/enable");
	code = system("echo 1 > /sys/class/usb_composite/rndis/enable");
	chmod("/dev/android_adb", 0777);
	if (code = WEXITSTATUS(system("adbd >/var/adbd.log 2>/var/adbd.err &")))
		logprintf("2adbd failed to start, code %d");

	logprintf("0starting touchscreen service");
	if (code = WEXITSTATUS(system("tssrv &"))) {
		logprintf("3tssrv failed to start, code %d");
		exit(1);
	}

	for (i = 0; i < 5; ++i) {
		logprintf("0initializing touch screen - attempt %d", i);
		if (!ts_open(ts_name)) break;
		logprintf("2 - attempt failed!");
		usleep(400000);
	}
	if (i == 5) {
		logprintf("3giving up on TS!");
		exit(1);
	}
	atexit(ts_close);

	logprintf("0mounting base partitions");

	mkdir("/mnt/firmware", 0755);
	logprintf("0 - /mnt/firmware (/dev/mmcblk0p1)");
	mount("/dev/mmcblk0p1", "/mnt/firmware", "vfat",
		MS_RDONLY | MS_SILENT, NULL);

	mkdir("/mnt/boot", 0755);
	logprintf("0 - /mnt/boot (/dev/mmcblk0p13)");
	mount("/dev/mmcblk0p13", "/mnt/boot", "ext4", MS_SILENT, NULL);

	logprintf("0starting lvm");
	code = system("rm -rf /etc/lvm/cache");
	code = system("lvm vgchange -a y");
	code = system("lvm vgmknodes");

	logprintf("0mounting base volumes");
	mount_lv("media");
	mkdir("/mnt/media/nsboot", 0755);
	if (chdir("/mnt/media") == -1)
		logperror("can't chdir into /mnt/media");

	mount_lv("root");
}

void do_shutdown(void) {
	int code;

	logprintf("0stopping tssrv");
	code = system("killall tssrv");

	logprintf("0stopping adbd");
	code = system("killall adbd");

	logprintf("0unmounting base volumes");
	umount_lv("media");
	umount_lv("root");

	logprintf("0unmounting base partitions");

	logprintf("0 - /mnt/boot");
	umount("/mnt/boot");

	logprintf("0 - /mnt/firmware");	
	umount("/mnt/firmware");
}

void do_cleanup(void) {
	if (is_fb_available()) fb_close();
	display_wholelog();
}

void symlink_binaries(void) {
	if (symlink("/mnt/root/usr/lib", "/usr/lib") == -1)
		logperror("symlink /usr/lib failed");
	if (symlink("/mnt/root/lib", "/lib") == -1)
		logperror("symlink /lib failed");

	unlink("/bin/resizefat");
	if (symlink("/mnt/root/bin/resizefat", "/bin/resizefat") == -1)
		logperror("symlink resizefat failed");
	unlink("/bin/mke2fs");
	if (symlink("/mnt/root/sbin/mke2fs.e2fsprogs", "/bin/mke2fs") == -1)
		logperror("symlink mke2fs failed");
	unlink("/bin/e2fsck");
	if (symlink("/mnt/root/sbin/e2fsck.e2fsprogs", "/bin/e2fsck") == -1)
		logperror("symlink e2fsck failed");
	unlink("/bin/resize2fs");
	if (symlink("/mnt/root/sbin/resize2fs", "/bin/resize2fs") == -1)
		logperror("symlink resize2fs failed");
	unlink("/bin/dosfsck");
	if (symlink("/mnt/root/usr/sbin/dosfsck", "/bin/dosfsck") == -1)
		logperror("symlink dosfsck failed");


}

void enable_coredumps(void) {
	struct rlimit unlim;
	int code;

	unlim.rlim_cur = RLIM_INFINITY;
	unlim.rlim_max = RLIM_INFINITY;

	code = system("echo '/var/core-%e' > /proc/sys/kernel/core_pattern");

	setrlimit(RLIMIT_CORE, &unlim);
}

