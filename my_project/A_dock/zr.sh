#!/bin/bash
###############0
user=ll1297281826
ip=192.168.0.243

#################################
if [ -d /usr/data/dock ];then
	echo "rm old /usr/data/dock"
	rm -rf /usr/data/dock
fi

scp -r $user@$ip:/datadisk/ee/ee_archer_debug/dock  /usr/data
if [ ! -d /usr/data/dock ];then
	echo "scp /usr/data/dock fail"
	exit 1
fi

cd /usr/data/dock
make clean
make
