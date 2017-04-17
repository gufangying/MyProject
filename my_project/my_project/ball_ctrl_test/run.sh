#!/bin/bash
CPUINFO=`cat /proc/cpuinfo | grep processor | wc -l`

#APP Name
APPName=medium_server
#App bk Name
AppBKName=medium_server.bk

if [ $CPUINFO -eq 4 ];then
    PLATFORM=master
else
    PLATFORM=slave
fi

killall -9 ${APPName}
killall -9 ${AppBKName}
insmod  /home/root/driver/$PLATFORM/ov10640_490_camera.ko
insmod /home/root/driver/$PLATFORM/ov10640_490_camera_mipi.ko

cd video
chmod 777 /home/root/vpu/$PLATFORM/${APPName}
/home/root/vpu/$PLATFORM/${APPName} -C  /home/root/vpu/config_enc &

#sleep 30 S
sleep 3

ExeName=""
for i in 1 2 3 4 5 6 7 8 9 10;do
	ExeName=$(ps|grep -o ${APPName})
	if [ "${ExeName}" = "${APPName}" ];then
		echo ExeName Is ${ExeName}
		echo "${APPName} Is running"
		exit
	else
		sleep 3
		echo SLEEP 3 S
	fi
done

echo AppBKName Is ${AppBKName}
echo "${APPName} Is Not running, Now Run ${AppBKName}"
killall -9 ${AppBKName}
cd video
chmod 777 /home/root/vpu/$PLATFORM/${AppBKName}
/home/root/vpu/$PLATFORM/${AppBKName} -C  /home/root/vpu/config_enc &





