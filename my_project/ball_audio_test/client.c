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

#define PORT 8888
#define BUFFER_SIZE 0x280000
#define DOCK_BUF 1024
#define IP "192.168.0.148"

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

typedef int u32;
/*音频发送数据头结构*/
struct sAudioHead
{
	u32 nTimestamp;			/*时间戳*/
	u32 nDataLen;		/*数据长度*/
};

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


//#define SAVE_FRAME			//将音频一帧一帧的保存
#define FRAME_NUM	2000		//帧个数

int main(int argc, char *argv[])
{
    int sockfd, sendbytes;
    char buf[BUFFER_SIZE];
    struct hostent *host;
    struct sockaddr_in serv_addr;
	struct sAudioHead AudioPack;
    int ret;
    int i;
	FILE *pAACFile = NULL;

#ifdef SAVE_FRAME
	FILE *pFrame[FRAME_NUM];
	int nFrameFlag = 0;			//帧计数
	char szFrameName[128] = "";
	system("rm ./AudioFrame/*");
	/*
	for(i = 0; i < FRAME_NUM; i++)
	{
		pFrame[i] = NULL;
		sprintf(szFrameName, "./AudioFrame/%d.aac", i + 1);
		pFrame[i] = fopen(szFrameName, "wb+");
		if(NULL == pFrame[i])
		{
			printf("fopen pFrame[%d] Error \n", i);
			return 0;
		}
	}
	printf("Open Frame File Successful \n");
	*/
#endif

	pAACFile = fopen("./Audio.aac", "wb+");
	if(NULL == pAACFile)
	{
		printf("fopen AACFile error \n");
		return 0;
	}

    //test
    host = gethostbyname(IP);
    if(2 == argc)
    {
		host = gethostbyname(argv[1]);
    }

    memset(buf, 0, sizeof(buf));

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

	int enc_len = 0;
	int nHeadLen = sizeof(AudioPack);
	int recv_len = 0;
	int nSumlen = 0;		//记录总接收长度
	sendbytes = 0;

	int nIndex = 0;
    while(1)
    {
    	sendbytes = 0;
		recv_len = 0;

#ifdef SAVE_FRAME
		pFrame[nFrameFlag] = NULL;
		sprintf(szFrameName, "./AudioFrame/%d.aac", nFrameFlag + 1);
		pFrame[nFrameFlag] = fopen(szFrameName, "wb+");
		if(NULL == pFrame[nFrameFlag])
		{
			printf("fopen pFrame[%d] Error \n", nFrameFlag);
			goto EXIT;
		}
#endif

        while(sendbytes < nHeadLen)
        {
            recv_len = recv(sockfd, &AudioPack + sendbytes, nHeadLen - sendbytes, 0);
			if(recv_len <= 0)
			{
				printf("recv head error \n");
				goto EXIT;
			}

			sendbytes += recv_len;
        }
        enc_len = AudioPack.nDataLen;

		//时间戳
		printf("Start AudioPack.Timestamp = %d \n", AudioPack.nTimestamp);
#if 0
		if(nIndex == 0)
		{
			printf("AudioPack.nDataLen = %d, Start AudioPack.Timestamp = %d \n", AudioPack.nDataLen, AudioPack.nTimestamp);
		}
		if(nIndex == 1763)
		{
			nIndex = -1;
			printf("AudioPack.nDataLen = %d, End AudioPack.Timestamp = %d \n", AudioPack.nDataLen, AudioPack.nTimestamp);
		}
		nIndex++;
#endif
        sendbytes = 0;
		recv_len = 0;
        while(sendbytes < enc_len)
        {
            recv_len = recv(sockfd, &buf[sendbytes],(enc_len - sendbytes),0);
            if(recv_len <= 0)
            {
                printf("recv enc err,exit\n");
                goto EXIT;
            }
            sendbytes += recv_len;
        }

        nSumlen += enc_len;

#ifdef SAVE_FRAME
		if(nFrameFlag < FRAME_NUM)
		{
			fwrite(buf, 1, enc_len, pFrame[nFrameFlag]);
			fclose(pFrame[nFrameFlag]);
			pFrame[nFrameFlag] = NULL;
			nFrameFlag++;
			printf("nFrameFlag = %d \n", nFrameFlag);
		}
#else
		fwrite(buf, 1, enc_len, pAACFile);
        //printf("recv size = %d \n",sendbytes);
#endif
    }

EXIT:
	printf("nSumlen = %d \n", nSumlen);

	if(NULL != pAACFile)
	{
		fclose(pAACFile);
	}
    close(sockfd);
}
