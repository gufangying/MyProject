#include "main.h"

void MemTest(int nSize /*KB*/)
{
	int nFlag = 0;
	char* g_szBuf1;
	char* g_szBuf2;
	struct timeval timeStart, timeStop;

	g_szBuf1 = (char*)malloc(nSize);
	g_szBuf2 = (char*)malloc(nSize);

	while(1)
	{
		//开始时间
		if(nFlag == 0)
		{
			gettimeofday(&timeStart, NULL);
			nFlag++;
		}

		memcpy(g_szBuf1, g_szBuf2, nSize);

		//停止时间
		if(nFlag == 1)
		{
			gettimeofday(&timeStop, NULL);
			nFlag++;
			printf("memcpy %dK DATA Used Time Is : %ld ms \n", nSize / 1024, (timeStop.tv_sec + timeStop.tv_usec / 1000) - (timeStart.tv_sec + timeStart.tv_usec / 1000) );
		}
	}
}


int main(int argc, char* argv[])
{
	int nSize = 0; 		////k字节
	if(argc < 2)
	{
		printf("./mem_test.out nSize, nSize is KB\n");
		return 0;
	}

	nSize = atoi(argv[1]);
	printf("memcpy size is %dKB \n", nSize);

	MemTest(nSize * 1024);
}
