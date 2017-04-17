#!/bin/bash

##################
#从ftp服务器下载下载文件,参数1是序列号
##################
#hostip="192.168.2.148"
hostip="192.168.0.81"

login_name="ftpuser"
password="ftpuser"

DIR_NAME=$(date +%F-%T)
#mkdir $DIR_NAME

#cd $DIR_NAME

#参数1是序列号
filename="${1}.evo"
echo $filename

ftp -v -n  $hostip << EOF 
user $login_name $password 
binary
prompt
cd output_data
mget ${filename}
bye
EOF
