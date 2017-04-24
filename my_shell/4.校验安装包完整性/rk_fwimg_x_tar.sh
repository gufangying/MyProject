#!/bin/bash
#将img文件解包

head_name="RK_IMG_HEAD"
end_name="RK_IMG_END"
img_name="Firmware.img"

#解包
tar -xf RKIMGTAR.tar

names=(${head_name} ${end_name} ${img_name})

for((i=0;i<=2;i++));do
	if [ ! -e ${names[${i}]} ];then
		echo "1.${names[${i}]} Is Not Existed "
		exit		
	fi
done

echo "x_tar Successful"

#获取img文件长度
img_length=$(ls -l ${img_name} | awk '{print $5}')
echo "1. img_length = ${img_length}"

#读取头文件中的长度
head_info=$(cat ${head_name})
echo "2. head_info = ${head_info}"

#比较img_length和head_info是否相等
if [ ${img_length} != ${head_info} ];then
	echo "3. Error : img File Is Not Fine!"
	exit
fi
echo "3. img_length = head_info, img File Is OK"

#比较尾文件
end_info=$(cat ${end_name})
echo "4. end_info = ${end_info}"
if [ "end" != ${end_info} ];then
	echo "5. Error : end Fine Is Not Fine"
	exit
fi
echo "5. end File Is OK, Now Start Update"

#执行升级
fwupdate_main -i /mnt/sdcard/nor/${img_name} -d /tmp -t 3 -r -T 0
if [ $? -ne 0 ];then
	echo "6. Update Failed"
	exit
fi
echo "6. Update Successful"
