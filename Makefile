# makefile for nsboot

nsboot: *.[ch]
	gcc -D_GNU_SOURCE -std=gnu99 -static -Os -o nsboot *.c
	strip nsboot

nsboot-test: *.[ch]
	gcc -D_GNU_SOURCE -std=gnu99 -g -o nsboot-test *.c

clean: 
	rm -f nsboot .rd .urd uImage

uImage: nsboot uKernel.test
	rm -f .rd.lzma
	cp nsboot root/bin
	cp t.fnt root
	(cd root && find . | cpio -o --format=newc > ../.rd)
	lzma .rd
	mkimage -A arm -O linux -T ramdisk -C none -a 0x60000000 -e 0x60000000 \
		-n nsboot-ramdisk -d .rd.lzma .urd
	mkimage -A arm -O linux -T multi -C none -a 0x40208000 -e 0x40208000 \
		-n nsboot-multi -d uKernel.test:.urd uImage

boot: uImage
	novacom boot mem:// < uImage

scp: uImage
	ssh root@hptp1 mount -o remount,rw /boot/moboot
	scp uImage root@hptp1:/boot/moboot/uImage.nsboot
	ssh root@hptp1 reboot

push: uImage
	adb shell mount -o remount,rw /boot
	adb push uImage /boot/uImage.nsboot
	adb reboot
