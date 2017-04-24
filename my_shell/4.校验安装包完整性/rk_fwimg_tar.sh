#!/bin/bash
#将img文件打包, 加入协议头,协议尾

head_name="RK_IMG_HEAD"
end_name="RK_IMG_END"
img_name="Firmware.img"

names=(${head_name} ${end_name} ${img_name})

for((i=0;i<=2;i++));do
	if [ ! -e ${names[${i}]} ];then
		echo "1.${names[${i}]} Is Not Existed "
		
		touch ${names[${i}]}
		if [ $? -eq 0 ];then
			echo "1.touch ${names[${i}]} Successful"
		else
			echo "1.touch ${names[${i}]} Faild"
			exit
		fi
	fi
done


#增加尾部信息
echo "end" >> ${end_name}

#获取img文件长度
img_length=$(ls -l ${img_name} | awk '{print $5}')

echo "1. img_length = ${img_length}"

#将长度写入头文件
echo "${img_length}" >> ${head_name}

#打包
tar -cf RKIMGTAR.tar ${head_name} ${img_name} ${end_name}
echo "tar Successful"
