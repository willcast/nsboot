*Ninja Swag* Touch Bootloader
=============================
For HP TouchPad (aka: topaz, tenderloin). Enables Extreme Multibooting.

Features
========
Integrated (basic) Android recovery functions:
----------------------------------------------
 * install .zip
 * wipe system, data or cache
 * adb support (root shell or push/pull)
 * backup (one .tgz per partition/volume)

Extract .tar.gz files into root of any mountable partition
----------------------------------------------------------
 * Used to install most native/"desktop" Linux distributions ported to TP.
 * For a clean installation, format ("wipe") the destination beforehand.
 * If you need to install archives on top of each other, DON'T format in between each.
 * This is also how you can restore backups. Format the destination, and then select the backup to "install".

File management
---------------
 * browse directories / select files
 * delete, copy, move, rename, change permissions

Partition/volume management
---------------------------
 * mount/unmount LVM2 volumes
 * check and repair ext[234] or msdos/vfat filesystems
 * create or delete LVM2 volumes on internal storage (/dev/store/...)
 * resize volumes (must have a supported FS)
 * rename volumes

Currently bootable operating systems
------------------------------------
Stock OS
 * webOS 3.0.5

Android ROM ports
 * Qcom Froyo (2.2) remastered version by myself.
 * Official CM7 alpha 3.5.
 * Official CM9 nightlies.
 * CM10 nightlies from jcsullins.
 * "Schzoid" 4.2 and 4.3.

Android recovery ports
 * TWRP 2.5 official builds.

Desktop Linux ports
 * Ubuntu 12.10 and 13.04 armhf ported by myself.
 * Fedora 18 (freedreno testbed) ported by Rob Clark.
 * Slackware ARM ported by myself.
 * Bodhi Linux armel (no longer supported)

Installation
============
nsboot can be compiled natively on the TP (using desktop linux ports or a 
chroot), or you can cross-compile it from other hardware. The latter is
recommended.

Build dependencies
------------------
 * gcc
 * mkimage (u-boot-tools)
 * GNU make
 * kernel (see below)
 * ...or use the precompiled kernel, "zImage"

Kernel information
------------------
The kernel used is a modified version of Linux 2.6.35. You can find the
source at http://github.com/willcast/ubuntu_kernel_tenderloin. You'll have
to check out the "kexec" branch, not the default one, and configure with
with the "tenderloin_willcast_defconfig".

Build instructions
------------------
Just run "make".

To cross-compile, simply set CC to a suitable compiler, like this:
    make CC=arm-linux-gnueabihf-gcc
(The compiler need not be in $PATH, and scratchbox2 is no longer required or
supported.)

To test boot the uImage, plug in the TouchPad, place it in bootie/recovery
mode, and:
    make boot
(Obviously, you can only do that when you're cross compiling, which is why
that's the best method.)

To "push" (install) the built nsboot uImage onto the boot partition...
...when at the nsboot menu:
    make push_nsboot
...when running a compatible* Android ROM
    make push_android
...when running a compatible* Android recovery
    make push_android_recovery
...when running a compatible* desktop Linux port
    make push_linux

* just try it, if there are errors, then it's incompatible.

External programs used in the image:
------------------------------------
All are linked statically, can be stripped, and go in the subdirectory named
"root" within the source.
 * busybox (main binary, default config)
 * bash (main binary)
 * moboot ("uimage-extract" utility)
 * u-boot-tools ("mkimage" utility)
 * lvm2 (main binary "lvm")
 * kexec-tools (main binary "kexec", with hardboot patch)
 * ts_srv (main binary, configure for single touch)
 * GNU tar (main binary)
 * pigz (main binary)
