#!/bin/bash
#description: report building progress.
#author: Justin Zhao
#date: 2015-5-29

while [ "1" = "1" ]
do
	for i in {3..0}
	do
		if [ -f build"$i".out ]; then
			case $i in
			0) echo "make Image->"`tail -n 1 build"$i".out`;;
			1) echo "sudo make modules->"`tail -n 1 build"$i".out`;;
			2) echo "sudo make modules_install->"`tail -n 1 build"$i".out`;;
			3) echo "sudo make firmware_install->"`tail -n 1 build"$i".out`;;
			esac
			break
		else
			if [ $i = 0 ]; then
				echo "Starting..."
			fi
		fi
	done

	sleep 1
done
