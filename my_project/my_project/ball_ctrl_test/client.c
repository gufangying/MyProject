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
	char magic[8];					/* ʶ��key������Ϊ��Bcmd����ӦΪ��Back */
	unsigned int type;					    /* ������߻�Ӧ���� */
	unsigned int data;					    /* ������߻�Ӧ���� */
};

typedef struct
{
	int argc;
	char** argv;
	sem_t SemSendCmd;		//�����̷߳��������ź���
	int nCmd;				//��������
	int nData;				//���Ʋ���
	bool bIsExitSYS;		//�Ƿ��˳�������
}MainParam;


#define YUV_SIZE				    1280 * 1080 * 3 / 2								/*һ����Ƭ��С*/
#define VIDEO_NUMBER				4												/*����ͷ����*/
#define PICTUREBUF					YUV_SIZE * VIDEO_NUMBER * SERIES_TIME			/*�����Ƭ�Ļ�����*/
#define SERIES_TIME					1												/*���Ĵ���,1��ʾ��һ��*/


#define MAGIC_CMD					 "Bcmd"
#define MAGIC_BACK					 "Back"

#define BALL_MAX_MAGIC_LEN          8

#define BALL_CMD_KEEP_ALIVE 		0   //������
#define BALL_CMD_DATE_OPERATE		1   //���ڲ�������
#define BALL_CMD_CHANGE_MODE		2   //�л�ģʽ����
#define BALL_CMD_START_PREVIEW		3   //��ʼԤ������
#define BALL_CMD_STOP_PREVIEW		4   //ֹͣԤ������
#define BALL_CMD_START_VIDEO		5   //��ʼ¼������
#define BALL_CMD_STOP_VIDEO			6   //ֹͣ¼������
#define BALL_CMD_READ_VIDEO_TIME 	7   //��ȡ¼��ʱ������
#define BALL_CMD_TAKE_PHOTO			8   //��������
#define BALL_CMD_SET_EXP_COMP		9   //�����عⲹ������,����ͻ�Ӧ�������
#define BALL_CMD_SET_WT_BAL_MODE	10  //���ð�ƽ��ģʽ����


/* ������͸�Dock�Ļ�Ӧ�������£� */
#define BALL_ACK_READ_DATE					11   //��ȡ���ڻ�Ӧ
#define BALL_ACK_READ_VIDEO_TIME			12   //��ȡ¼��ʱ���Ӧ
#define BALL_ACK_TAKE_PHOTO					13   //���ջ�Ӧ
#define BALL_CMD_GET_BAL_STATUS				14   /* ��ȡ���״̬, ����ͻ�Ӧ�������*/
#define BALL_ACK_CHECK_PCIE  				15   /* ���PCIE��Ӧ*/
#define BALL_ACK_CMD_UPDATE_FIRMWARE   		16   /*���������Ӧ*/
#define BALL_ACK_CMD_UPDATE_APP     		17   /*���APP������Ӧ*/
#define BALL_ACK_APP_VERSION                18   /*�鿴����汾�Ż�Ӧ*/
#define BALL_ACK_GET_TEMPERATURE			19	 /*��ȡ����¶Ȼ�Ӧ*/


/*��Ƶͨ������*/
#define    BALL_CMD_START_AUDIO     		21    /*��ʼ¼��*/
#define    BALL_CMD_STOP_AUDIO      		22    /*ֹͣ¼��*/

/*����*/
#define    BALL_CMD_UPDATE_MASTER   		23    /*��������,�Ӱ�����ͷ��APP����*/
#define    BALL_CMD_UPDATE_SLAVE    		24    /*��������,�Ӱ�APP����*/

#define    BALL_CMD_TAKEPHOTO_SERIES   		25    /*����*/
#define    BALL_CMD_CHECK_PCIE  	   		26    /*���PCIE*/
#define    BALL_CMD_CHECK_APPVERSION   		27    /*�鿴APP�汾*/
#define	   BALL_CMD_GET_TEMPERATURE			28	  /*��ȡ����¶�*/
#define    BALL_CMD_SET_ISO		            29	  /*�������ISO*/
#define    BALL_CMD_SET_SCENE				30	  /*���ó���*/
#define    BALL_CMD_GET_CHCEK_HW			31	  /*��ȡӲ���Լ���Ϣ*/


//�˽ṹ�����ڷ��������������ĸ������ݰ�
//��������:  BALL_CMD_TAKE_PHOTO
//Ԥ������:  BALL_CMD_START_PREVIEW
typedef struct
{
	u8 ucSettingMode			:1;  /* �����������ģʽ(0: �Զ��� 1:�ֶ�)��ȡֵ�� enSettingMode */
	u8 ucSceneMode				:3;  /* ����ģʽ��ȡֵ�� enSceneMode */
	u8 ucExpComp				:4;  /* �عⲹ��: ��2EV����13����λ��ȡֵ��Χ�� enExposureCompensation */
	u8 ucWhiteBalanceMode		:4;  /* ��ƽ��ģʽ: �ṩ7��ģʽ��ȡֵ��Χ�� enWhiteBalanceMode */
	u8 ucISO					:3;  /* ISO(�й�ȣ���Ƭ�й�ʱ���ٶ�): ȡֵ��Χ�� enISO */
	u8 ucVNPFlag				:1;  /* ��ä�ĵ�ʱ������ä�ı�־(����1��ʾä��) */
}stBallcmdData;


void* DataThread(void* pMainParam);

//��ȡ��Ƭ����
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


//д��Ƭ����
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


//���մ���
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

	/*������Ƭ�ļ�*/
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


//�ж��ļ��Ƿ����
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


//����������
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


//��������Ĭ�ϲ���
void SetDefCapParam(int* pnData)
{
						//�ֶ�1, ����, �ع�, ��ƽ��, ISO
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


//�����߳�
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
	#if 1		//�ֶ�����
			printf("INPUT CMD, DATA>>>");
			scanf("%d%d", &nCmd, &nData);
	#else		//�Զ�����, ����ѭ������
			static int nRuntime = 2000;
			if(nRuntime <= 0)
			{
				printf("Over \n");
				break;
			}
			nRuntime--;

			sleep(5);
			//ѭ�����Կ�ʼԤ����ֹͣԤ��
			if(1 == nRuntime % 2)
			{
				//��ʼԤ��
				////�����߳�
				pthread_create(&DataID, NULL, (void *)&DataThread, (void*)pMainParam);
				sleep(5);
				
				printf("Start Preview, nRunTime = %d ======== \n", nRuntime);
				nCmd = 3;
				nData = 0;
			}
			else
			{
				//ֹͣԤ��
				printf("Stop Preview======== \n");
				nCmd = 4;
				nData = 0;
			}
	#endif
#else
			//����
			//nCmd = 24;
			//nData = 0;
#endif			
			//�ȴ�����
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

			//���Կ�ʼԤ��
			if(nCmd == BALL_CMD_START_PREVIEW)
			{
				SetDefCapParam(&cmd_pack.data);
			}

			//ä��
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
			
			//�鿴�汾
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

			//����,���ն������ã��������վͰ�SERIES_TIME��Ϊ1
			if(nCmd == 8)
			{
				TakePhoto(sockfd);
			}

			if(nCmd == -1)
				break;
	    }
	}
}


//��������
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


//�����߳�
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

	//����ȷ�ϰ�
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

	//��������
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


//���������߳�
void CreateDataThread(MainParam* pParam)
{
	pthread_t DataID;

	printf("(CreateDataThread) Enter \n");
	pthread_create(&DataID, NULL, (void *)&DataThread, (void*)pParam);
}


//���ѿ����߳�, ��������
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

//����ָ�붨��
typedef bool (*pHandleFunc)(MainParam*);

//ӳ��ṹ
typedef struct
{
	int nCmd;				//����
	pHandleFunc pFunc;		//������
	char szName[128];		//������
	char szInfo[128];		//��Ϣ
}MAP;

//ӳ��
MAP g_Map[] = { {2, QuitProc, "QuitProc", "The system is to Quit. \n"},
				{3, AutoProc, "AutoProc", "The system is to run by auto. \n"},
				{4, ManualProc, "ManualProc", "The system is to run by Manual. \n"},
				{1, HelpProc, "HelpProc", "SYSTEM Help Info. \n"},
				{0, NULL, "", ""}
				};
				

//�˳�������
bool QuitProc(MainParam* pParam)
{
	pParam->bIsExitSYS = true;

	return true;
}

//����������
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

//�Զ�������
bool AutoProc(MainParam* pParam)
{
	int nData = 0;
	int nRuntime = 2000;		//ͳ�ƴ���
	
	
	printf("(AutoProc) Enter \n");

	while(1)
	{
		printf("nRuntime = %d \n", 2000 + 1 - nRuntime);
		if(0 == nRuntime)
		{
			break;
		}
		
		//���������߳�
		CreateDataThread(pParam);

		//��ʼԤ��
		sleep(3);
		SendCmdProc(pParam, 3, 0);

		int i = 0;
		nData = 0;
		for(i = 0; i < 13; i++)
		{
			//�����ع�
			sleep(3);
			SendCmdProc(pParam, BALL_CMD_SET_EXP_COMP, nData);

			sleep(3);
			//���ð�ƽ��
			SendCmdProc(pParam, BALL_CMD_SET_WT_BAL_MODE, nData);

			//����ISO
			sleep(3);
			SendCmdProc(pParam, BALL_CMD_SET_ISO, nData);

			//���ó���
			sleep(3);
			SendCmdProc(pParam, BALL_CMD_SET_SCENE, nData);

			//����
			sleep(3);
			SendCmdProc(pParam, BALL_CMD_TAKE_PHOTO, nData);

			nData++;
		}

		//ֹͣԤ��
		sleep(5);
		SendCmdProc(pParam, 4, 0);

		nRuntime--;
	}

	return true;
}

//�ֶ�������
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

		getchar();		//�õ��м�Ŀո�
		
		nData = getchar();
		if('\n' == nData)
		{
			continue;
		}

		nCmd -= 48;
		nData -= 48;
#endif
		getchar();		//�õ����Ļس�

		if(0 == nCmd && 0 == nData)
		{
			//����ֹͣԤ������
			SendCmdProc(pParam, 4, 0);
			break;
		}
		else if(1 == nCmd && 1 == nData)
		{
			//���������߳�
			CreateDataThread(pParam);
		}
		else
		{
			//���ѿ����̷߳��Ϳ�������
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

	////�����߳�
	pthread_create(&CtrlID, NULL, (void *)&CtrlThread, (void*)&Param);
	usleep(300 * 1000);

	char nChose = 0;
	char szTip[128] = "";
	
	//�Զ�����
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

		//�ַ�ת����
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
