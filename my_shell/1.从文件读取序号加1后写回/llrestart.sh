#!/bin/bash
index_file="./index_restart"

echo "========================================"
echo "========================================"
echo "sleep 5s and then reboot, ctrl + c stop"

index="0"

if [ -a ${index_file} ]
then
	echo "file index_restart, existed"
	index=$(cat ${index_file})
	echo "index = ${index}"
	if [[ "0" == ${index} ]]
	then
		echo "index = 0, set index = 1"
	else
		index=`expr ${index} + 1`
		echo "index + 1 = ${index}"
		#write index to index_file
		echo ${index}>${index_file}
	fi
else
	echo "file index_restart, not existed"
fi

echo "========================================"
echo "========================================"
