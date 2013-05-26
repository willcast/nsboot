*Ninja Swag* Touch Bootloader
=============================

Build dependencies (native and cross):
--------------------------------------
 * gcc
 * mkimage (u-boot-tools)
 * kernel source: http://github.com/willcast/ubuntu-kernel-tenderloin BRANCH: "kexec"
 * or use the precompiled kernel uImage (uImage.test)

Cross build dependencies:
-------------------------
 * scratchbox2 (sb2)
 * arm-linux-gnueabi-gcc, arm-linux-gnueabihf-gcc or arm-eabi-gcc 

To make a uImage from x86:
--------------------------
    sb2 make uImage

To make only the binary from x86:
---------------------------------
    sb2 make

To make a debuggable test binary from x86:
------------------------------------------
    sb2 make nsboot-test

To make a uImage from ARM:
--------------------------
    make uImage

To make the binary from ARM:
----------------------------
    make

To make a testing binary (with debugging) on ARM:
-------------------------------------------------
    make nsboot-test

To clean the build directory (not the mock root):
-------------------------------------------------
    make clean

External programs (unmodified source, statically linked and stripped) used in the image:
----------------------------------------------------------------------------------------
 * busybox (busybox)
 * bash (bash)
 * moboot (tools/uimage-extract)
 * u-boot-tools (mkimage)
 * lvm2 (lvm)
 * kexec-tools (kexec)
 * ts_srv (tssrv)
 * GNU tar
