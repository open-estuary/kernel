#!/bin/bash
#description: build kernel.
#author: Justin Zhao
#date: 2015-5-29

if [ x"arm32" = x"$1" ]; then
    export ARCH=arm
else
    export ARCH=arm64
fi

march=`uname -m`
export CROSS_COMPILE=
if [ "$march" != "aarch64" -a "$march" != "arm" ]; then
    if [ "$ARCH" = "arm" ]; then
	    export CROSS_COMPILE=arm-linux-gnueabihf-
    else
	    export CROSS_COMPILE=aarch64-linux-gnu-
    fi
fi

rm *.out 2>/dev/null
sudo make mrproper

if [ "$ARCH" = "arm" ]; then
    make hisi_defconfig
    make zImage -j14 > build0.out 2>&1
    make hip04-d01.dtb
    cat arch/arm/boot/zImage arch/arm/boot/dts/hip04-d01.dtb >.kernel
else
    make defconfig
    make Image -j14 > build0.out 2>&1
    make hisilicon/hip05-d02.dtb
fi

if [ "$march" = "aarch64" -o "$march" = "arm" ]; then
	sudo make modules -j14 > build1.out 2>&1
	sudo make modules_install -j14 > build2.out 2>&1
	sudo make firmware_install -j14 > build3.out 2>&1
fi

if [ "$ARCH" = "arm" ]; then
    sudo cp arch/arm/boot/zImage /boot/
    sudo cp arch/arm/boot/dts/hip04-d01.dtb /boot/

#    scp .kernel justinzh@192.168.1.107:~/

    find . -name zImage
    find . -name hip04-d01.dtb
else
    sudo cp /boot/Image /boot/Image.bak
    sudo cp /boot/hip05-d02.dtb /boot/hip05-d02.dtb.bak
    sudo cp arch/arm64/boot/Image /boot/
    sudo cp arch/arm64/boot/dts/hisilicon/hip05-d02.dtb /boot/

#    scp arch/arm64/boot/Image justin@192.168.1.107:~/ftp/
#    scp arch/arm64/boot/dts/hisilicon/hip05-d02.dtb justin@192.168.1.107:~/ftp/

    find . -name Image
    find . -name hip05-d02.dtb
fi
