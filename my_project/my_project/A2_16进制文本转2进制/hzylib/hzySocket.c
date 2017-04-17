#include "hzySocket.h"

bool hzyBindPort(int* pnListenSock, int nPort)
{
	int on = 1;
	int nSock = -1;
	struct sockaddr_in sAddr;

	nSock = socket(AF_INET, SOCK_STREAM, 0);
	if(nSock < 0)
	{
		printf("socket Error \n");
		return false;
	}

	setsockopt(nSock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	sAddr.sin_family = AF_INET;
	sAddr.sin_port = htons(nPort);
	sAddr.sin_addr.s_addr = INADDR_ANY;
	if(bind(nSock,  (struct sockaddr *)&sAddr, sizeof(sAddr)) < 0)
	{
		printf("bind Error \n");
		return false;
	}

	listen(nSock,3);

	*pnListenSock = nSock;
	return true;
}


bool hzyWaitClientConn(int* pnClientSock, int nListenSock)
{
	int nSock = -1;
	struct sockaddr_in sAddrClient;
	socklen_t SockSize = sizeof(sAddrClient);

	if ((nSock = accept (nListenSock, (struct sockaddr *) &sAddrClient, &SockSize)) < 0)
	{
		printf("Accept Error \n");
		return false;
	}

	*pnClientSock = nSock;

	return true;
}

bool hzyRecvSocketData(int nSocket, void *pData, int nLen)
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


bool hzySendSocketData(int nClientSocket, void* pData, int nLen)
{
	int nRet = 0;

	if(nLen > 0)
	{
		nRet = send(nClientSocket, pData, nLen, 0);

		if(nRet <= 0)
		{
			return false;
		}

		return true;
	}

	return false;
}





