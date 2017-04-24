#!/bin/bash
INDEX=1

#这个脚本可以读取INDEX的值, 没运行一次脚本, 将INDEX的值加1,起到计数的作用
#开机的时候执行这个脚本可以记录开机的次数

#首先利用循环读取INDEX22的值
flag=0
index=0
index_new=0

while read line
do
	if [ 1 = ${flag} ]
	then
		#截取第二行的index值
		index=${line:6:5}
		echo "index = ${index}"
		index_new=`expr ${index} + 1`
		echo "index_new=${index_new}"
		
		#修改INDEX的值
		sed -i "s#INDEX=${index}#INDEX=${index_new}#" ${0}
		exit 0
	else
		echo ${line}
		flag=`expr ${flag} + 1`
	fi	
done < ${0}
