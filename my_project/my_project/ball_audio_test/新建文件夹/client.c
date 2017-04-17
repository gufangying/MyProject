/*client.c*/

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>

#define PORT	7777
#define DOCK_BUF 1024

struct dock {
    char magic_key[5];
    int cmd_type;
    int data_size;
    char buf[DOCK_BUF];

}dock = {"EvoS"};

struct frame_info {
    unsigned int  seqno;
    unsigned int  iframe;
    unsigned int frame_id;
    unsigned int len[4];
}net_enc;


int connect_server(int* p_socket, int n_port)
{
	*p_socket;
	int Socket;
	struct sockaddr_in Addr;
	Addr.sin_family = AF_INET;
	Addr.sin_port = htons(n_port);
	Addr.sin_addr.s_addr = inet_addr("192.168.0.166");

    /* 创建socket */
	Socket = socket(AF_INET, SOCK_STREAM, 0);

	while(1)
	{
		if (connect(Socket, (struct sockaddr *) &Addr, sizeof(struct sockaddr) ) < 0)
		{
			printf("error:connect server failed!\n");
			sleep(1);
		}
		else
		{
			//printf("info:connect server successful!\n");
			*p_socket = Socket;
			return 1;
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int BUFFER_SIZE = 2700000;
	char buf[2700000];
	int file_flag = 1;
	int ret = 0;

	int nFlag;					////标志位,是否是帧头信息;
	int nIsFirstIFrame = 0;	////标志位,是否是第一个关键帧
	int sendbytes;
	int nSum;

	int m_sockBallStream;

	int i;
	struct dock dock_statu;
	strcpy(dock_statu.magic_key, "EvoS");

	FILE *pFileView[4];			////保存预览流文件

	pFileView[0] = NULL;
	pFileView[1] = NULL;
	pFileView[2] = NULL;
	pFileView[3] = NULL;

	pFileView[0] = fopen("View1.h264", "wb+");
	pFileView[1] = fopen("View2.h264", "wb+");
	pFileView[2] = fopen("View3.h264", "wb+");
	pFileView[3] = fopen("View4.h264", "wb+");

	////连接球机服务器数据端口
	if(0 == connect_server(&m_sockBallStream, PORT) )
	{
		////connect error
		close(m_sockBallStream);
		printf("connect server failed\n");
		return 0;
	}

	sendbytes = 0;

	if(send(m_sockBallStream, &dock_statu, sizeof(dock_statu), 0) <= 0)
	{
		close(m_sockBallStream);
		return 0;
	}
	printf("(data thread) send successful\n");

	while(1)
    {
        sendbytes = recv(m_sockBallStream,&net_enc,sizeof(net_enc),0);
        if(sendbytes != sizeof(net_enc))
        {
            perror("recv head ");
            printf("net_enc err,exit:%lu head:%d\n",(sendbytes),sizeof(net_enc));
            exit(-1);
        }
        int enc_len = net_enc.len[0] + net_enc.len[1] + net_enc.len[2] + net_enc.len[3];
        printf("head:%d,need recv:%d\n",sendbytes,enc_len,net_enc.len[0],net_enc.len[1]);
        sendbytes = 0;
        while(sendbytes < enc_len)
        {
            int recv_len = recv(m_sockBallStream,&buf[sendbytes],(enc_len - sendbytes),0);
            if(recv_len <= 0)
            {
                perror("recv frame ");
                printf("recv enc err,exit\n");
                exit(-1);
            }
            sendbytes += recv_len;
        }
        printf("recv size;%d video1:%d,video2:%d,video3:%d,video4:%d\n",sendbytes,net_enc.len[0],net_enc.len[1],net_enc.len[2],net_enc.len[3]);
        if(file_flag == 1)
        {
            if(net_enc.len[0] != 0)
            ret = fwrite(buf,net_enc.len[0],1,pFileView[0]);
            if( 1 != ret )
            {
                printf("line 1 ret:%d\n",ret);
                perror("fwrite");
                return -1;
            }
            if(net_enc.len[1] != 0)
            {
            ret = fwrite(&buf[net_enc.len[0]],net_enc.len[1],1,pFileView[1]);
            if( 1 != ret )
            {
                printf("line 2 ret:%d\n",ret);
                perror("fwrite");
                return -1;
            }
            }
            if(net_enc.len[2] != 0)
            {
            ret = fwrite(&buf[net_enc.len[0]+net_enc.len[1]],net_enc.len[2],1,pFileView[2]);
            if( 1 != ret )
            {
                printf("line 3 ret:%d\n",ret);
                perror("fwrite");
                return -1;
            }
            }
            if(net_enc.len[3] != 0)
            {
            ret = fwrite(&buf[enc_len-net_enc.len[3]],net_enc.len[3],1,pFileView[3]);
            if( 1 != ret )
            {
                printf("line 4 ret:%d\n",ret);
                perror("fwrite");
                return -1;
            }

             }
         }
    }

	////关闭文件
	for(i = 0; i < 4; i++)
	{
		if(NULL != pFileView[i])
			fclose(pFileView[i]);
	}
    return 0;

	printf("the end of data thread\n");
}
