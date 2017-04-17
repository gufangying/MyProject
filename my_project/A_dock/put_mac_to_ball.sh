#!/bin/bash

##################
#上传文件到ftp服务器
##################
hostip="192.168.0.38"

login_name="medium"
password="medium"

DIR_NAME=$(date +%F-%T)


filename="mac"
echo $filename

ftp -v -n  $hostip << EOF 
user $login_name $password 
binary
prompt
cd /home/root/video/ 
mput ${filename}
bye
EOF
