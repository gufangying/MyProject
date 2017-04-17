#!/bin/bash

######################
#修改主从版应用程序名
######################
master_name="M_medium_server"
slave_name="S_medium_server"
master_dir="/datadisk/ee/medium/ball/ap/ball/mxc_vpu/master/"
slave_dir="/datadisk/ee/medium/ball/ap/ball/mxc_vpu/slave/"
ball_dir="/home/root/burn/"

cp /datadisk/ee/medium/ball/ap/ball/mxc_vpu/master/medium_server	/datadisk/ee/medium/ball/ap/ball/mxc_vpu/master/M_medium_server
cp /datadisk/ee/medium/ball/ap/ball/mxc_vpu/slave/medium_server		/datadisk/ee/medium/ball/ap/ball/mxc_vpu/slave/S_medium_server

##################
#上传文件到ftp服务器
##################
hostip="192.168.0.148"

login_name="medium"
password="medium"

ftp -v -n  $hostip << EOF 
user $login_name $password 
binary
prompt
cd ${ball_dir} 
lcd ${master_dir}
mput ${master_name}
cd ${ball_dir}
lcd ${slave_dir}
mput ${slave_name}
bye
EOF

