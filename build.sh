#!/bin/bash
#description: build kernel.
#author: Justin Zhao
#date: 2015-5-29

export ARCH=arm64
march=`uname -m`
if [ "$march" != "aarch64" ]; then
	export CROSS_COMPILE=aarch64-linux-gnu-
fi
if [ "$march" = "arm" ]; then
    export ARCH=arm
	export CROSS_COMPILE=
fi

rm *.out 2>/dev/null
sudo make mrproper

if [ "$march" = "arm" ]; then
    export ARCH=arm
    make hisi_defconfig
    make zImage -j14 > build0.out 2>&1
    make hip04-d01.dtb
    cat arch/arm/boot/zImage arch/arm/boot/dts/hip04-d01.dtb >.kernel
else
    make defconfig
    make Image -j36 > build0.out 2>&1
    make hisilicon/hip05-d02.dtb
fi

if [ "$march" = "aarch64" -o "$march" = "arm" ]; then
	sudo make modules -j14 > build1.out 2>&1
	sudo make modules_install -j14 > build2.out 2>&1
	sudo make firmware_install -j14 > build3.out 2>&1
fi

if [ "$march" = "arm" ]; then
    sudo cp arch/arm/boot/zImage /boot/
    sudo cp arch/arm/boot/dts/hip04-d01.dtb /boot/

    find . -name zImage
    find . -name hip04-d01.dtb
else
    sudo cp /boot/Image /boot/Image.bak
    sudo cp /boot/hip05-d02.dtb /boot/hip05-d02.dtb.bak
    sudo cp arch/arm64/boot/Image /boot/
    sudo cp arch/arm64/boot/dts/hisilicon/hip05-d02.dtb /boot/

    scp arch/arm64/boot/Image justin@192.168.1.107:~/ftp/
    scp arch/arm64/boot/dts/hisilicon/hip05-d02.dtb justin@192.168.1.107:~/ftp/

    find . -name Image
    find . -name hip05-d02.dtb
fi
