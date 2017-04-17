/*client.c*/

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <assert.h>
#include<semaphore.h>


#define PORT	6666
#define IP	"192.168.0.148"
#define BUFFER_SIZE 0x280000
#define DOCK_BUF 1024

typedef unsigned char u8;

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

struct ball_packet
{
	char magic[8];					/* 识别key：命令为：Bcmd；回应为：Back */
	unsigned int type;					    /* 命令或者回应类型 */
	unsigned int data;					    /* 命令或者回应数据 */
};

typedef struct
{
	int argc;
	char** argv;
	sem_t SemSendCmd;		//控制线程发送命令信号量
	int nCmd;				//控制命令
	int nData;				//控制参数
	bool bIsExitSYS;		//是否退出主程序
}MainParam;


#define YUV_SIZE				    1280 * 1080 * 3 / 2								/*一张照片大小*/
#define VIDEO_NUMBER				4												/*摄像头个数*/
#define PICTUREBUF					YUV_SIZE * VIDEO_NUMBER * SERIES_TIME			/*存放照片的缓冲区*/
#define SERIES_TIME					1												/*连拍次数,1表示拍一次*/


#define MAGIC_CMD					 "Bcmd"
#define MAGIC_BACK					 "Back"

#define BALL_MAX_MAGIC_LEN          8

#define BALL_CMD_KEEP_ALIVE 		0   //心跳包
#define BALL_CMD_DATE_OPERATE		1   //日期操作命令
#define BALL_CMD_CHANGE_MODE		2   //切换模式命令
#define BALL_CMD_START_PREVIEW		3   //开始预览命令
#define BALL_CMD_STOP_PREVIEW		4   //停止预览命令
#define BALL_CMD_START_VIDEO		5   //开始录像命令
#define BALL_CMD_STOP_VIDEO			6   //停止录像命令
#define BALL_CMD_READ_VIDEO_TIME 	7   //读取录制时间命令
#define BALL_CMD_TAKE_PHOTO			8   //拍照命令
#define BALL_CMD_SET_EXP_COMP		9   //设置曝光补偿命令,命令和回应都是这个
#define BALL_CMD_SET_WT_BAL_MODE	10  //设置白平衡模式命令


/* 球机发送给Dock的回应定义如下： */
#define BALL_ACK_READ_DATE					11   //读取日期回应
#define BALL_ACK_READ_VIDEO_TIME			12   //读取录制时间回应
#define BALL_ACK_TAKE_PHOTO					13   //拍照回应
#define BALL_CMD_GET_BAL_STATUS				14   /* 读取球机状态, 命令和回应都是这个*/
#define BALL_ACK_CHECK_PCIE  				15   /* 检测PCIE回应*/
#define BALL_ACK_CMD_UPDATE_FIRMWARE   		16   /*球机升级回应*/
#define BALL_ACK_CMD_UPDATE_APP     		17   /*球机APP升级回应*/
#define BALL_ACK_APP_VERSION                18   /*查看球机版本号回应*/
#define BALL_ACK_GET_TEMPERATURE			19	 /*获取球机温度回应*/


/*音频通道控制*/
#define    BALL_CMD_START_AUDIO     		21    /*开始录音*/
#define    BALL_CMD_STOP_AUDIO      		22    /*停止录音*/

/*升级*/
#define    BALL_CMD_UPDATE_MASTER   		23    /*升级主板,从板摄像头和APP程序*/
#define    BALL_CMD_UPDATE_SLAVE    		24    /*升级主板,从板APP程序*/

#define    BALL_CMD_TAKEPHOTO_SERIES   		25    /*连拍*/
#define    BALL_CMD_CHECK_PCIE  	   		26    /*检测PCIE*/
#define    BALL_CMD_CHECK_APPVERSION   		27    /*查看APP版本*/
#define	   BALL_CMD_GET_TEMPERATURE			28	  /*获取球机温度*/
#define    BALL_CMD_SET_ISO		            29	  /*设置球机ISO*/
#define    BALL_CMD_SET_SCENE				30	  /*设置场景*/
#define    BALL_CMD_GET_CHCEK_HW			31	  /*读取硬件自检信息*/


//此结构体用于发送以下球机命令的附带数据包
//拍照命令:  BALL_CMD_TAKE_PHOTO
//预览命令:  BALL_CMD_START_PREVIEW
typedef struct
{
	u8 ucSettingMode			:1;  /* 拍摄参数设置模式(0: 自动， 1:手动)，取值见 enSettingMode */
	u8 ucSceneMode				:3;  /* 场景模式，取值见 enSceneMode */
	u8 ucExpComp				:4;  /* 曝光补偿: ±2EV，共13个档位，取值范围见 enExposureCompensation */
	u8 ucWhiteBalanceMode		:4;  /* 白平衡模式: 提供7种模式，取值范围见 enWhiteBalanceMode */
	u8 ucISO					:3;  /* ISO(感光度，胶片感光时的速度): 取值范围见 enISO */
	u8 ucVNPFlag				:1;  /* 有盲拍的时候用作盲拍标志(等于1表示盲拍) */
}stBallcmdData;


void* DataThread(void* pMainParam);

//获取照片索引
int GetPhotoIndex(int* pnIndex)
{
	FILE* pFile = NULL;
	char szTemp[128] = "";


	pFile = fopen("PhotoIndex", "rb");
	if(NULL == pFile)
	{
		printf("fopen PhotoIndex File Error \n");
		return -1;
	}

	if(1 != fread(szTemp, 6, 1, pFile))
	{
		printf("fread PhotoIndex Error \n");
		return -1;
	}

	*pnIndex = atoi(szTemp);
	if(NULL != pFile)
	{
		fclose(pFile);
		pFile = NULL;
	}

	printf("PhotoIndex = %d \n", *pnIndex);

	return 0;
}


//写照片索引
int SetPhotoIndex(int nIndex)
{
	FILE* pFile = NULL;
	char szTemp[128] = "";


	pFile = fopen("PhotoIndex", "wb");
	if(NULL == pFile)
	{
		printf("fopen PhotoIndex File Error \n");
		return -1;
	}

	sprintf(szTemp, "%06d", nIndex);
	printf("szTemp = %s \n", szTemp);
	if(1 != fwrite(szTemp, 6, 1, pFile))
	{
		printf("fwrite PhotoIndex Error \n");
		return -1;
	}

	if(NULL != pFile)
	{
		fclose(pFile);
		pFile = NULL;
	}

	return 0;
}


//拍照处理
int TakePhoto(int sockfd)
{
	int i = 0;
	int nRet = 0;
	int nIndex = 0;
	char szCmd[128] = "";
	char szPath[128] = "";
	char szFileName[32] = "";
	char *pPictureBuf = NULL;
	struct timeval TimeStart, TimeStop;
	pPictureBuf = (char*)malloc(PICTUREBUF);
	FILE *pFile[VIDEO_NUMBER * SERIES_TIME] = {NULL};
	if(NULL == pPictureBuf)
	{
		printf("malloc error \n");
		return 0;
	}

	gettimeofday(&TimeStart, NULL);

	GetPhotoIndex(&nIndex);
	sprintf(szPath, "./PHOTO/%06d/", nIndex);
	sprintf(szCmd, "mkdir %s", szPath);
	system(szCmd);

	for(i = 0; i < VIDEO_NUMBER * SERIES_TIME; i++)
	{
		sprintf(szFileName, "%s%6d_%d.yuv", szPath, nIndex, i + 1);
		pFile[i] = NULL;
		pFile[i] = fopen(szFileName, "wb+");
		if(NULL == pFile)
		{
			printf("fopen error %d\n", i);
			return 0;
		}
	}

	int nSum = 0;
	while(nSum < PICTUREBUF)
	{
		nRet = recv(sockfd, pPictureBuf + nSum, PICTUREBUF - nSum, 0);
		if(nRet <=0 )
		{
			printf("recv data error \n");
			return 0;
		}
		else
		{
			nSum += nRet;
		}
	}

	/*保存照片文件*/
	for(i = 0; i < VIDEO_NUMBER * SERIES_TIME; i++)
	{
		nRet = fwrite(pPictureBuf + i * YUV_SIZE, YUV_SIZE, 1, pFile[i]);
		if(nRet != 1)
		{
			printf("fwrite error \n");
		}
		fclose(pFile[i]);
	}

	gettimeofday(&TimeStop, NULL);
	printf(" take photo time is : %ld \n", (TimeStop.tv_sec - TimeStart.tv_sec)*1000 + (TimeStop.tv_usec - TimeStart.tv_usec)/1000);

	nIndex++;
	SetPhotoIndex(nIndex);
}


//判断文件是否存在
bool IsFileExisted(char* pszName)
{
	int nRet = 0;
	assert(NULL != pszName);


	nRet = access(pszName, F_OK);

	if(0 == nRet)
	{
		nRet = true;
		printf("(IsFileExisted) %s Is Existed \n", pszName);
	}
	else
	{
		nRet = false;
		printf("(IsFileExisted) %s Is Not Existed \n", pszName);
	}

	return nRet;
}


//发送心跳包
void* HeartBeatThd(void* arg)
{
	int sock_client = *(int*)arg;
	int sendbytes = 0;
	struct ball_packet cmd_pack = {"Bcmd", BALL_CMD_KEEP_ALIVE, 0};

	//printf("(HeartBeatThd) Enter \n");

	while(1)
	{
		sendbytes = send(sock_client, &cmd_pack, sizeof(cmd_pack), 0);
		if(sendbytes < 0)
		{
			printf("send cmd failed\n");
		}

		sleep(3);
	}

	//printf("(HeartBeatThd) Quit \n");
}


//设置拍摄默认参数
void SetDefCapParam(int* pnData)
{
						//手动1, 场景, 曝光, 白平衡, ISO
	stBallcmdData szParam = {1,  2,    7,      1,    3, 	1};
	//stBallcmdData szParam = {0,  0,    0,      0,    0, 	0};
	memcpy(pnData, &szParam, sizeof(szParam));


	printf("ucExpComp = %u \n", szParam.ucExpComp);
	printf("ucISO = %u \n", szParam.ucISO);
	printf("ucSceneMode = %u \n", szParam.ucSceneMode);
	printf("ucSettingMode = %u \n", szParam.ucSettingMode);
	printf("ucVNPFlag = %u \n", szParam.ucVNPFlag);
	printf("ucWhiteBalanceMode = %u \n", szParam.ucWhiteBalanceMode);
}


//控制线程
void* CtrlThread(void* pMainParam)
{
	assert(NULL != pMainParam);
	MainParam* pParam = (MainParam*)pMainParam;
	int argc = pParam->argc;
	char** argv = pParam->argv;

    int sockfd, sendbytes;
    char buf[BUFFER_SIZE];
    struct hostent *host;
    struct sockaddr_in serv_addr;
    FILE *fp[4];
    int ret;
    int i;
    int file_flag = 0;
    pthread_t HeartBeatThdId;
    pthread_t DataID;


	//test
	IsFileExisted("PHOTO");

	printf("argc = %d \n", argc);
	for(i = 0; i < argc; i++)
	{
		printf("argv[%d] = %s \n", i, argv[i]);
	}

	switch(argc)
	{
		case 1:
			host = gethostbyname(IP);
			break;
		case 2:
			host = gethostbyname(argv[1]);
			break;
		case 3:
			break;
	}

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(serv_addr.sin_zero), 8);

	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
    {
        printf("socket error\n");
        return 0;
    }

    if(connect(sockfd,(struct sockaddr *)&serv_addr, sizeof(struct sockaddr))== -1)
    {
        perror("connect");
        return 0;
    }

	pthread_create(&HeartBeatThdId, NULL, (void *)&HeartBeatThd, (void*)&sockfd);

	if(1)
	{
		int nCmd = 0;
		int nRec = 0;
		int nData = 0;
		struct ball_packet stBallAckPkt;
		struct ball_packet cmd_pack = {"Bcmd", 1, 0};

	     while(1)
	    {
#if 0
	#if 1		//手动输入
			printf("INPUT CMD, DATA>>>");
			scanf("%d%d", &nCmd, &nData);
	#else		//自动输入, 控制循环次数
			static int nRuntime = 2000;
			if(nRuntime <= 0)
			{
				printf("Over \n");
				break;
			}
			nRuntime--;

			sleep(5);
			//循环测试开始预览、停止预览
			if(1 == nRuntime % 2)
			{
				//开始预览
				////数据线程
				pthread_create(&DataID, NULL, (void *)&DataThread, (void*)pMainParam);
				sleep(5);
				
				printf("Start Preview, nRunTime = %d ======== \n", nRuntime);
				nCmd = 3;
				nData = 0;
			}
			else
			{
				//停止预览
				printf("Stop Preview======== \n");
				nCmd = 4;
				nData = 0;
			}
	#endif
#else
			//升级
			//nCmd = 24;
			//nData = 0;
#endif			
			//等待唤醒
			sem_wait(&(pParam->SemSendCmd));
			nCmd = pParam->nCmd;
			nData = pParam->nData;
			
			cmd_pack.type = nCmd;
			cmd_pack.data = nData;
			
			printf("(CtrlThread) Cmd = %d, Data = %d \n", cmd_pack.type, cmd_pack.data);

			if(nCmd == 1 && cmd_pack.data != 0)
			{
				time_t Time;
				time(&Time);
				cmd_pack.data = Time;
			}

			//测试开始预览
			if(nCmd == BALL_CMD_START_PREVIEW)
			{
				SetDefCapParam(&cmd_pack.data);
			}

			//盲拍
			if(nCmd == BALL_CMD_TAKE_PHOTO && cmd_pack.data == 2)
			{
				SetDefCapParam(&cmd_pack.data);
			}
			
			sendbytes = send(sockfd, &cmd_pack, sizeof(cmd_pack), 0);
			if(sendbytes < 0)
			{
				printf("send cmd failed\n");
			}

			if(nCmd == 1 ||  nCmd == 8 || nCmd == 9 || nCmd == 25 || nCmd == 24 || nCmd == 27 || nCmd == 28)
			{
				nRec = recv(sockfd, &stBallAckPkt, sizeof(stBallAckPkt), 0);
				if(nRec <=0 )
				{
					printf("recv head error \n");
					return 0;
				}
				printf("recv head, Ack = %d, data = %d \n", stBallAckPkt.type, stBallAckPkt.data);
			}
			
			//查看版本
			if(nCmd == 27)
			{
				char szVeision[6] = "";
				nRec = recv(sockfd, szVeision, 6, 0);
				if(nRec <=0 )
				{
					printf("recv head error \n");
					return 0;
				}
				printf("ball app virsion : %s \n", szVeision);
			}

			//连拍,拍照都可以用，测试拍照就把SERIES_TIME改为1
			if(nCmd == 8)
			{
				TakePhoto(sockfd);
			}

			if(nCmd == -1)
				break;
	    }
	}
}


//接收数据
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


//数据线程
void* DataThread(void* pMainParam)
{
	assert(NULL != pMainParam);
	MainParam* pParam = (MainParam*)pMainParam;
	int argc = pParam->argc;
	char** argv = pParam->argv;

    int sockfd, sendbytes;
    char buf[BUFFER_SIZE];
    struct hostent *host;
    struct sockaddr_in serv_addr;
    FILE *fp[4];
    int ret;
    int i;
    int file_flag = 0;
    pthread_t HeartBeatThdId;

	printf("(DataThread) Enter \n");
	
	switch(argc)
	{
		case 1:
			host = gethostbyname(IP);
			break;
		case 2:
			host = gethostbyname(argv[1]);
			break;
		case 3:
			break;
	}

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(7777);
    serv_addr.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(serv_addr.sin_zero), 8);

	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
    {
        printf("socket error\n");
        return 0;
    }

    if(connect(sockfd,(struct sockaddr *)&serv_addr, sizeof(struct sockaddr))== -1)
    {
        perror("connect");
        return 0;
    }

	//发送确认包
    dock.buf[0] = '1';
    if ((sendbytes = send(sockfd, &dock, sizeof(dock), 0)) == -1)
    {
        perror("send");
        return 0;
    }
    else
    {
    	printf("send:%d\n", sendbytes);
    }

	//接收数据
	while(1)
	{
		if(false == LLRecvSocketData(sockfd, &net_enc, sizeof(net_enc)) )
        {
            printf("net_enc err, exit:%d head:%ld\n", (sendbytes), sizeof(net_enc));
            break;
        }

        int enc_len = net_enc.len[0] + net_enc.len[1] + net_enc.len[2] + net_enc.len[3];

        sendbytes = 0;
        while(sendbytes < enc_len)
        {
            int recv_len = recv(sockfd, &buf[sendbytes], (enc_len - sendbytes), 0);
            if(recv_len <= 0)
            {
                printf("recv enc err,exit\n");
                break;
            }
            sendbytes += recv_len;
        }
	}

	printf("(DataThread) Enter \n");
}


//创建数据线程
void CreateDataThread(MainParam* pParam)
{
	pthread_t DataID;

	printf("(CreateDataThread) Enter \n");
	pthread_create(&DataID, NULL, (void *)&DataThread, (void*)pParam);
}


//唤醒控制线程, 发送命令
void SendCmdProc(MainParam* pParam, int nCmd, int nData)
{
	pParam->nCmd = nCmd;
	pParam->nData = nData;
	sem_post(&(pParam->SemSendCmd));
	
	printf("(SendCmdProc) Cmd = %d, Data = %d \n", pParam->nCmd, pParam->nData);
}


/***********************************************************************************/
bool QuitProc(MainParam* pParam);
bool HelpProc(MainParam* pParam);
bool AutoProc(MainParam* pParam);
bool ManualProc(MainParam* pParam);

//函数指针定义
typedef bool (*pHandleFunc)(MainParam*);

//映射结构
typedef struct
{
	int nCmd;				//命令
	pHandleFunc pFunc;		//处理函数
	char szName[128];		//函数名
	char szInfo[128];		//信息
}MAP;

//映射
MAP g_Map[] = { {2, QuitProc, "QuitProc", "The system is to Quit. \n"},
				{3, AutoProc, "AutoProc", "The system is to run by auto. \n"},
				{4, ManualProc, "ManualProc", "The system is to run by Manual. \n"},
				{1, HelpProc, "HelpProc", "SYSTEM Help Info. \n"},
				{0, NULL, "", ""}
				};
				

//退出处理函数
bool QuitProc(MainParam* pParam)
{
	pParam->bIsExitSYS = true;

	return true;
}

//帮助处理函数
bool HelpProc(MainParam* pParam)
{
	printf("Help Info : \n");
	printf("CMD\t\tINFO\n");

	MAP* pMap = g_Map;

	while(NULL != pMap->pFunc)
	{
		printf("%d\t%s", pMap->nCmd, pMap->szInfo);
		pMap++;
	}
	
	return true;
}

//自动处理函数
bool AutoProc(MainParam* pParam)
{
	int nData = 0;
	int nRuntime = 2000;		//统计次数
	
	
	printf("(AutoProc) Enter \n");

	while(1)
	{
		printf("nRuntime = %d \n", 2000 + 1 - nRuntime);
		if(0 == nRuntime)
		{
			break;
		}
		
		//连接数据线程
		CreateDataThread(pParam);

		//开始预览
		sleep(3);
		SendCmdProc(pParam, 3, 0);

		int i = 0;
		nData = 0;
		for(i = 0; i < 13; i++)
		{
			//设置曝光
			sleep(3);
			SendCmdProc(pParam, BALL_CMD_SET_EXP_COMP, nData);

			sleep(3);
			//设置白平衡
			SendCmdProc(pParam, BALL_CMD_SET_WT_BAL_MODE, nData);

			//设置ISO
			sleep(3);
			SendCmdProc(pParam, BALL_CMD_SET_ISO, nData);

			//设置场景
			sleep(3);
			SendCmdProc(pParam, BALL_CMD_SET_SCENE, nData);

			//拍照
			sleep(3);
			SendCmdProc(pParam, BALL_CMD_TAKE_PHOTO, nData);

			nData++;
		}

		//停止预览
		sleep(5);
		SendCmdProc(pParam, 4, 0);

		nRuntime--;
	}

	return true;
}

//手动处理函数
bool ManualProc(MainParam* pParam)
{
	int nData = 0;
	int nCmd = 0;
	pthread_t DataID;


	printf("(ManualProc) Enter \n");
	printf("Input Cmd Data, 0 0 Quit; 1 1 Create DataThread \n");
	
	while(1)
	{
		printf("Input Cmd Data(arg: 0 0) >>>> ");
		scanf("%d %d", &nCmd, &nData);
#if 0
		nCmd = getchar();
		if('\n' == nCmd)
		{
			continue;
		}

		getchar();		//拿掉中间的空格
		
		nData = getchar();
		if('\n' == nData)
		{
			continue;
		}

		nCmd -= 48;
		nData -= 48;
#endif
		getchar();		//拿掉最后的回车

		if(0 == nCmd && 0 == nData)
		{
			//发送停止预览命令
			SendCmdProc(pParam, 4, 0);
			break;
		}
		else if(1 == nCmd && 1 == nData)
		{
			//创建数据线程
			CreateDataThread(pParam);
		}
		else
		{
			//唤醒控制线程发送控制命令
			SendCmdProc(pParam, nCmd, nData);
		}

		sleep(1);
	}

	return true;
}


int main(int argc, char *argv[])
{
	MainParam Param;
	pthread_t CtrlID;


	Param.argc = argc;
	Param.argv = argv;
	Param.bIsExitSYS = false;
	sem_init(&(Param.SemSendCmd), 0, 0);

	////控制线程
	pthread_create(&CtrlID, NULL, (void *)&CtrlThread, (void*)&Param);
	usleep(300 * 1000);

	char nChose = 0;
	char szTip[128] = "";
	
	//自动测试
	printf("\n----------------Main Page---------------- \n");
	HelpProc(&Param);
	while(1)
	{
		MAP* pMap = g_Map;
		
		printf("You Chose > ");
		nChose = getchar();

		if('\n' == nChose)
		{
			continue;
		}
		getchar();

		//字符转数字
		nChose = nChose - 48;
		
		while(NULL != pMap->pFunc)
		{
			if(nChose == pMap->nCmd)
			{
				sprintf(szTip, "\n--------%s Page-------- \n", pMap->szName);
				printf("%s", szTip);
				pMap->pFunc(&Param);

				if(Param.bIsExitSYS)
					goto EXIT;

				
				break;
			}
			else
				pMap++;

			if(NULL == pMap->pFunc)
			{
				printf("No Such Chose X X X X. \n");
			}
		}

		printf("\n----------------Main Page---------------- \n");
	}
EXIT:
	printf("--------GOOD BYE-------- \n");
}
