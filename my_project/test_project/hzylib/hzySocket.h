#ifndef HZYSOCKET_H__
#define HZYSOCKET_H__

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

#define prt(fmt, arg...) \
	do { \
        printf("[ball] %s(%d) "fmt, __func__, __LINE__, ##arg);\
	}while(0)

bool hzyBindPort(int* pnListenSock, int nPort);
bool hzyWaitClientConn(int* pnClientSock, int nListenSock);
bool hzyRecvSocketData(int nSocket, void *pData, int nLen);
bool hzySendSocketData(int nClientSocket, void* pData, int nLen);

#endif