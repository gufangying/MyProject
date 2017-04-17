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
#include <stdbool.h>

#define PORT 7777
#define BUFFER_SIZE 0x280000
#define DOCK_BUF 1024
struct dock {
    char magic_key[5];
    int cmd_type;
    int data_size;
    char buf[DOCK_BUF];

}dock = {"EvoS"};

struct netstr {
    unsigned int  seqno;
    unsigned int  iframe;
    unsigned int frame_id;
    unsigned int len[4];
}net_enc;


bool LLRecvSocketData(int nSocket, void *pData, int nLen)
{
	int nRet = 0;
	int nRecvedLen = 0;

	while(nRecvedLen < nLen)
	{
		nRet = recv(nSocket, pData + nRecvedLen, nLen - nRecvedLen, 0);
		if(nRet <= 0)
		{
			return false;
		}

		nRecvedLen += nRet;
	}

	return true;
}

//#define SAVE_FRAME		//将视频保存为一帧一帧
#define FRAME_NUM	1000//帧个数

int main(int argc, char *argv[])
{
    int sockfd, sendbytes;
    char buf[BUFFER_SIZE];
    struct hostent *host;
    struct sockaddr_in serv_addr;
    FILE *fp[4];
    int ret;
    int i;
    int file_flag = 1;
#ifdef SAVE_FRAME
	int nFrameFlag = 0;
	FILE* pFrame[FRAME_NUM];
	char szFrameName[128] = "";
	system("rm ./VideoFrame/*");
	for(i = 0; i < FRAME_NUM; i++)
	{
		pFrame[i] = NULL;
		sprintf(szFrameName, "./VideoFrame/%d.264", i + 1);
		pFrame[i] = fopen(szFrameName, "wb");
		if(NULL == pFrame[i])
		{
			printf("fopen pFrame[%d] Error \n", i);
			return 0;
		}
	}
#endif

    //test
    host = gethostbyname("192.168.0.148");

    memset(buf, 0, sizeof(buf));
    for(i = 0;i < 4 ;i++)
    {
        sprintf(buf, "%s_%d.h264", "priview",i+1);
        fp[i] = fopen(buf,"wb");
        if(fp[i] == NULL)
        {
            perror("fopen");
            return 0;
        }
    }

    if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
    {
        printf("socket error\n");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(serv_addr.sin_zero), 8);

    if(connect(sockfd,(struct sockaddr *)&serv_addr, sizeof(struct sockaddr))== -1)
    {
        perror("connect");
        exit(1);
    }

    dock.buf[0] = '1';
    if ((sendbytes = send(sockfd, &dock, sizeof(dock), 0)) == -1)
    {
        perror("send");
        exit(1);
    }
    else
    	printf("send:%d\n",sendbytes);

    while(1)
    {
        if(false == LLRecvSocketData(sockfd, &net_enc, sizeof(net_enc)) )
        {
            perror("recv head ");
            printf("net_enc err,exit:%d head:%ld\n",(sendbytes),sizeof(net_enc));
            exit(-1);
        }

        int enc_len = net_enc.len[0] + net_enc.len[1] + net_enc.len[2] + net_enc.len[3];

		//时间戳
        //printf(" time = %d\n", net_enc.frame_id);

        sendbytes = 0;
        while(sendbytes < enc_len)
        {
            int recv_len = recv(sockfd,&buf[sendbytes],(enc_len - sendbytes),0);
            if(recv_len <= 0)
            {
                perror("recv frame ");
                printf("recv enc err,exit\n");
                exit(-1);
            }
            sendbytes += recv_len;
        }

		//数据长度
        printf("ID:%d, IFrame:%d, video1:%d,video2:%d,video3:%d,video4:%d\n",net_enc.seqno, net_enc.iframe, net_enc.len[0],net_enc.len[1],net_enc.len[2],net_enc.len[3]);

		if(file_flag == 1)
        {
            if(net_enc.len[0] != 0)
            {
#if 0
	            ret = fwrite(buf,net_enc.len[0],1,fp[0]);
	            if( 1 != ret )
	            {
	                printf("line 1 ret:%d\n",ret);
	                perror("fwrite");
	                return -1;
	            }
#endif
	            //保存帧文件
#ifdef SAVE_FRAME
				if(nFrameFlag < FRAME_NUM)
				{
					printf("nFrameFlag = %d \n", nFrameFlag);
					ret = fwrite(buf, net_enc.len[0], 1, pFrame[nFrameFlag++]);
		            if( 1 != ret )
		            {
		                printf("fwrite pFrame[%d] Error \n", nFrameFlag - 1);
		                return -1;
		            }
	            }
	            else
	            {
					if(NULL != pFrame[0])
					{
						int nI = 0;
						for(nI = 0; nI < FRAME_NUM; nI++)
						{
							fclose(pFrame[nI]);
							pFrame[nI] = NULL;
						}
					}
	            }
#endif
            }
#if 0
            if(net_enc.len[1] != 0)
            {
	            ret = fwrite(&buf[net_enc.len[0]],net_enc.len[1],1,fp[1]);
	            if( 1 != ret )
	            {
	                printf("line 2 ret:%d\n",ret);
	                perror("fwrite");
	                return -1;
	            }
            }
            if(net_enc.len[2] != 0)
            {
	            ret = fwrite(&buf[net_enc.len[0]+net_enc.len[1]],net_enc.len[2],1,fp[2]);
	            if( 1 != ret )
	            {
	                printf("line 3 ret:%d\n",ret);
	                perror("fwrite");
	                return -1;
	            }
            }
            if(net_enc.len[3] != 0)
            {
	            ret = fwrite(&buf[enc_len-net_enc.len[3]],net_enc.len[3],1,fp[3]);
	            if( 1 != ret )
	            {
	                printf("line 4 ret:%d\n",ret);
	                perror("fwrite");
	                return -1;
	            }
             }
#endif
         }
    }

    for(i=0;i<4;i++)
    	fclose(fp[i]);
    close(sockfd);

    exit(0);
    }
