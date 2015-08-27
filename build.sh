#!/bin/bash
#description: build kernel.
#author: Justin Zhao
#date: 2015-5-29

export ARCH=arm64
march=`uname -m`
if [ "$march" != "aarch64" ]; then
	export CROSS_COMPILE=aarch64-linux-gnu-
fi

sudo make mrproper
#cp config.dbg arch/arm64/configs/defconfig
make defconfig

rm *.out
make Image -j14 > build0.out 2>&1

if [ "$march" = "aarch64" ]; then
	sudo make modules -j14 > build1.out 2>&1
	sudo make modules_install -j14 > build2.out 2>&1
	sudo make firmware_install -j14 > build3.out 2>&1
fi

make hisilicon/hip05-d02.dtb

sudo cp /boot/Image /boot/Image.bak
sudo cp /boot/hip05-d02.dtb /boot/hip05-d02.dtb.bak
sudo cp arch/arm64/boot/Image /boot/
sudo cp arch/arm64/boot/dts/hisilicon/hip05-d02.dtb /boot/

find . -name Image
find . -name hip05-d02.dtb
