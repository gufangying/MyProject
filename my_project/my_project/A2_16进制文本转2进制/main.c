#include "stdio.h"
#include "stdlib.h"
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

char szTxtBuf[1024 * 1024 * 10] = "";
char szDesBuf[1024 * 1024 * 10] = "";

void Hex2Ascii(char* pszData, char* pszDesBuf, int nLen);


//去除空格
void DeleteBlack(char* pszName)
{
	int i = 0;
	int j = 0;
	int nTemp = 0;
	int nSize = 0;
	char szTemp[5] = "";
	FILE* pFile = NULL;


	printf("Enter \n");

	do{
		if(NULL == pszName){
			printf("NULL == pszName \n");
			break;
		}

		pFile = fopen(pszName, "r");
		if(NULL == pFile){
			printf("fopen %s Failed \n", pszName);
			break;
		}

		//获取长度
		fseek(pFile, 0, SEEK_END);
		//获取文件长度;
		nSize = ftell(pFile);
		printf("nSize = %d \n", nSize);

		fseek(pFile, 0, SEEK_SET);
		if(1 != fread(szTxtBuf, nSize, 1, pFile)){
			printf("fread %s Failed \n", pszName);
			break;
		}

		fclose(pFile);

		//去除空格
		for(i = 0, j = 0; i < nSize; i++){
			if(szTxtBuf[i] != ' '){
				szDesBuf[j] = szTxtBuf[i];
				j++;
			}
		}

		nSize = j + 1;
		printf("nSize = %d \n", nSize);

		//转2进制
#if 1
		Hex2Ascii(szDesBuf, szTxtBuf, nSize);
		nSize = nSize / 2;
#endif

		pFile = fopen(pszName, "wb+");
		if(NULL == pFile){
			printf("fopen test.jpg Failed \n");
			break;
		}

		if(1 != fwrite(szTxtBuf, nSize, 1, pFile)){
			printf("fwrite test.jpg Failed \n");
			break;
		}

		fclose(pFile);

		printf("Successful \n");
	}while(0);
}

//单个字符转化
int Hex2Int(char cHex)
{
	int nRet = 0;

	if(cHex >= '0' && cHex <= '9'){
		nRet = (cHex - 48);
	}
	else
	{
		nRet = (cHex - 55);
	}

	//printf("(Hex2Int) nRet = %d \n", nRet);

	return nRet;
}

//解析一个字节:如将两个字符"10", 解析成整型16
int GetWord(char* pData)
{
	int i = 0;
	int nRet = 0;
	int nTemp = 0;

	for(i = 0; i < 2; i++){
		nTemp = Hex2Int(pData[i]);

		if(1 != i){
			nRet += nTemp * 16 * (1 - i);
		}
		else{
			nRet += nTemp;
		}
	}

	//printf("(GetWord) nRet = %d \n", nRet);

	return nRet;
}

//16进制字符转ascii
void Hex2Ascii(char* pszData, char* pszDesBuf, int nLen)
{
	int i = 0;
	int j = 0;
	int nRet = 0;


	do{
		if(NULL == pszData){
			printf("NULL == pszData \n");
			break;
		}

		for(i = 0, j = 0; i <= nLen - 2; i += 2){
			nRet = GetWord(&pszData[i]);
			memcpy(pszDesBuf + j++, &nRet, 1);
		}
	}while(0);
}

int main(int argc, char* argv[])
{
	printf("argc = %d, argv[1] = %s \n", argc, argv[1]);

	DeleteBlack(argv[1]);

	//GetWord("11");

	return 0;
}

