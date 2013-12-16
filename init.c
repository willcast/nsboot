/*
	nsboot/init.c
	early userspace start up functions

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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/limits.h>	

#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/stat.h>

#include "boot.h"
#include "fb.h"
#include "init.h"
#include "input.h"
#include "lib.h"
#include "log.h"
#include "lv.h"

void do_init(void) {
	int i;
	int need_mdev = 0;
	char uinput_dir[PATH_MAX];

	atexit(do_cleanup);

	putenv("PATH=/bin");

	init_log();
	logprintf("0welcome to nsboot");

	logprintf("0rebuilding root filesystem");
	rebuild_fs();

	logprintf("0mounting /dev (devtmpfs)");
	if (mount("devtmpfs", "/dev", "devtmpfs", MS_SILENT, NULL) == -1) {
		logperror("error mounting /dev as devtmpfs");
		logprintf("0retrying /dev mount with plain tmpfs");
		if (mount("tmpfs", "/dev", "tmpfs", MS_SILENT, NULL) == -1)
			logperror("error mounting /dev/ as tmpfs");
	
		need_mdev = 1;
	}	

	logprintf("0mounting /proc");
	mount("none", "/proc", "proc", MS_SILENT, NULL);

	logprintf("0enabling core dumps");
	enable_coredumps();

	logprintf("0quieting kernel messages");
	qfprintf("/proc/sys/kernel/printk", "2");

	logprintf("0parsing kernel cmdline");
	parse_proc_cmdline();

	logprintf("0installing busybox");
	system_logged("busybox --install -s");

	logprintf("0symlinking binaries");
	symlink_binaries();

	logprintf("0mounting /sys");
	mount("none", "/sys", "sysfs", MS_SILENT, NULL);

	logprintf("0mounting /dev/pts");
	mkdir("/dev/pts", 0755);
	mount("none", "/dev/pts", "devpts", MS_SILENT, NULL);

	if (need_mdev) {
		logprintf("0starting legacy hotplug (mdev)");
		system_logged("touch /etc/mdev.conf");
		qfprintf("/proc/sys/kernel/hotplug", "/bin/mdev");
		system_logged("mdev -s");
	}

	logprintf("0initializing vibrator");
	vibrator_open("/sys/devices/virtual/timed_output/vibrator/enable");

	logprintf("0loading font file");
	if (load_font("/t.fnt")) {
		logprintf("3failed to load font");
		exit(1);
	}

	for (i = 0; i < 5; ++i) {
		logprintf("0initializing graphics (attempt %d)", i + 1);
		if (!fb_init()) break;
		usleep(300000);
	}
	if (i == 5) {
		logprintf("3failed to init graphics!");
		exit(1);
	}
	atexit(fb_close);

	logprintf("0reducing display brightness");
	set_brightness(48);

	start_adbd();

	logprintf("0starting touchscreen service");
	system_logged("tssrv >/var/tssrv.out 2>/var/tssrv.err &");

	if (need_mdev)
		strcpy(uinput_dir, "/dev");
	else
		strcpy(uinput_dir, "/dev/input");

	if (input_open(uinput_dir)) {
		logprintf("3failed to set up input!");
		exit(1);
	}
	atexit(input_close);

	logprintf("0mounting base partitions");

	logprintf("0 - /mnt/firmware (/dev/mmcblk0p1)");
	mount("/dev/mmcblk0p1", "/mnt/firmware", "vfat",
		MS_RDONLY | MS_SILENT, NULL);

	logprintf("0 - /mnt/boot (/dev/mmcblk0p13)");
	mount("/dev/mmcblk0p13", "/mnt/boot", "ext4", MS_SILENT, NULL);

	logprintf("0starting lvm");
	system_logged("rm -rf /etc/lvm/cache");
	system_logged("lvm vgchange -a y");
	system_logged("lvm vgmknodes");

	logprintf("0mounting base volumes");
	mount_lv("media");
	mkdir("/mnt/media/nsboot", 0755);
	if (chdir("/mnt/media") == -1)
		logperror("can't chdir into /mnt/media");

	logprintf("0mounting /tmp");
	mount("tmpfs", "/tmp", "tmpfs", MS_SILENT, NULL);

	mount_lv("root");
}

void do_shutdown(void) {
	logprintf("0stopping tssrv");
	system_logged("killall tssrv");

	system_logged("killall adbd");
	qfprintf("/sys/class/usb_composite/adb/enable", "0");
	qfprintf("/sys/class/usb_composite/rndis/enable", "0");

	logprintf("0unmounting base volumes");
	umount_lv("media");
	umount_lv("root");

	logprintf("0unmounting base partitions");
	logprintf("0 - /mnt/boot");
	umount("/mnt/boot");

	logprintf("0 - /mnt/firmware");
	umount("/mnt/firmware");

	logprintf("0disabling framebuffer");
	if (is_fb_available()) fb_close();

	logprintf("0disabling input");
	input_close();

	logprintf("0unmounting all volumes");
	(void)chdir("/");
	sync();
	qfprintf("/proc/sysrq-trigger", "u");

	logprintf("0syncing");
	sync();
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
	unlink("/bin/gzip");
	if (symlink("/bin/pigz", "/bin/gzip") == -1)
		logperror("symlink gzip failed");
}

void rebuild_fs(void) {
	mkdir("/dev", 0755);
	mkdir("/etc", 0755);
	mkdir("/mnt", 0755);
	mkdir("/mnt/boot", 0755);
	mkdir("/mnt/firmware", 0755);
	mkdir("/proc", 0755);
	mkdir("/sys", 0755);
	mkdir("/tmp", 0755);
	mkdir("/usr", 0755);
	mkdir("/var", 0755);
	
	if (symlink("/", "/system") == -1)
		logperror("symlink /system failed");
	if (symlink("/bin", "/sbin") == -1)
		logperror("symlink /sbin failed");
	if (symlink("/bin", "/usr/bin") == -1)
		logperror("symlink /usr/bin failed");
	if (symlink("/sbin", "/usr/sbin") == -1)
		logperror("symlink /usr/sbin failed");
}

void enable_coredumps(void) {
	struct rlimit unlim;

	unlim.rlim_cur = RLIM_INFINITY;
	unlim.rlim_max = RLIM_INFINITY;

	qfprintf("/proc/sys/kernel/core_pattern", "/var/core-%%e");

	setrlimit(RLIMIT_CORE, &unlim);
}

void restart_adbd(void) {
	stop_adbd();
	start_adbd();
}

void stop_adbd(void) {
	logprintf("0stopping adbd");
	system_logged("killall adbd");
}

void start_adbd(void) {
	logprintf("0starting adbd");
	qfprintf("/sys/class/usb_composite/adb/enable", "1");
	qfprintf("/sys/class/usb_composite/rndis/enable", "1");
	chmod("/dev/android_adb", 0777);
	system_logged("adbd >/var/adbd.log 2>/var/adbd.err &");
}

