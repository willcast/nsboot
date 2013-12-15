#	nsboot/Makefile
#	Makefile for nsboot
#
#	Copyright (C) 2013 Will Castro
#
#	This file is part of nsboot.
#
#	nsboot is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 2 of the License, or
#	(at your option) any later version.
#
#	nsboot is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with nsboot.  If not, see <http://www.gnu.org/licenses/>

CC := gcc
CFLAGS := -D_GNU_SOURCE -std=gnu99 -Os -Wall -ggdb
LDFLAGS := -static
SOURCES := boot.c fb.c file.c init.c input.c install.c lib.c log.c lv.c \
	lvset.c main.c menus.c osk.c select.c types.c
HEADERS := boot.h fb.h file.h init.h input.h install.h lib.h log.h lv.h \
	lvset.h menus.h osk.h select.h types.h
OBJECTS := $(SOURCES:.c=.o)
EXECUTABLE := nsboot
UIMAGE := uImage.nsboot
UKERNEL := uKernel
KERNEL := zImage
URAMDISK := uRamdisk
RAMDISK := nsboot_ramdisk.img

empty :=
space := $(empty) $(empty) 

all: $(UIMAGE) $(UKERNEL) $(URAMDISK) $(RAMDISK) $(KERNEL) $(EXECUTABLE) $(OBJECTS) $(HEADERS)

$(UIMAGE): $(UKERNEL) $(URAMDISK)
	mkimage -A arm -O linux -T multi -C none -a 0x40208000 -e 0x40208000 \
	-n nsboot_multi -d $(UKERNEL):$(URAMDISK) $@

$(URAMDISK): $(RAMDISK)
	mkimage -A arm -O linux -T ramdisk -C none -a 0x60000000 -e 0x60000000 \
	-n nsboot_ramdisk -d $< $@

$(UKERNEL): $(KERNEL)
	mkimage -A arm -O linux -T kernel -C none -a 0x40208000 -e 0x40208000 \
	-n nsboot_kernel -d $< $@

$(RAMDISK): $(EXECUTABLE)
	cp $(EXECUTABLE) root/bin
	cp t.fnt root/
	(cd root && find . | cpio -o --format=newc > ../ramdisk.cpio)
	lzma -c ramdisk.cpio > $@

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean boot push_nsboot push_android push_android_recovery push_linux

clean: 
	rm -f $(IMAGE) $(UKERNEL) $(URAMDISK) $(RAMDISK) $(EXECUTABLE) $(OBJECTS)

# this is temporary, after a boot/reboot the old bootloader will resume control
boot: $(UIMAGE)
	novacom boot mem:// < $(UIMAGE)

# use at nsboot menu
push_nsboot: $(UIMAGE)
	adb push $(UIMAGE) /mnt/boot/uImage.nsboot
	adb shell sync

# use when running compatible Android ROM
push_android: $(UIMAGE)
	adb shell mkdir -p /boot
	adb shell mount -o rw /dev/block/mmcblk0p13 /boot
	adb push $(UIMAGE) /boot/uImage.nsboot
	adb shell sync

# use when running compatible Android recovery
push_android_recovery: $(UIMAGE)
	adb shell mount /boot
	adb push $(UIMAGE) /boot/uImage.nsboot
	adb shell sync

# use when running compatible desktop Linux
push_linux:
	adb shell mount -o remount,rw /boot
	adb push $(UIMAGE) /boot/uImage.nsboot
	adb shell sync

# pushing to webOS needs investigation as to how to put a file.
