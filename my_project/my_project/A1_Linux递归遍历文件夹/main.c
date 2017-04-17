#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

//pPath:��ȫ��,��·��; pName:������,����·��; nLeval:���ļ��в���
void ListDir(char* pPath, char* pName, int nLeval)
{
	DIR* pDir = NULL;
	struct stat stStat;
	struct dirent* pEnt = NULL;
	char szFileName[256] = "";
	int i = 0;
	char szTab[256] = "";
	
	
	for(i = 1; i < nLeval; i++)
	{
		strcat(szTab, "\t");
	}
	
	strcat(szTab, pName);
	printf("%s \n", szTab);
	
	lstat(pPath, &stStat);
	if(S_ISDIR(stStat.st_mode))
	{
		//printf("DIR  %s \n", pPath);
		
		//�ļ���
		//ĩβ����'/'
		strcat(pPath, "/");
		
		pDir = opendir(pPath);
		if(NULL != pDir)
		{
			while(pEnt = readdir(pDir))
			{
				szFileName[0] = '\0';
				if(0 == strcmp(".", pEnt->d_name) || 0 == strcmp("..", pEnt->d_name))
				{
					continue;
				}
				
				strcat(szFileName, pPath);
				strcat(szFileName, pEnt->d_name);
				
				ListDir(szFileName, pEnt->d_name, nLeval+1);
			}
		}
		
		closedir(pDir);
	}
	else
	{		
		//printf("FILE %s \n", pPath);
	}
}

int main(int argc, char* argv[])  
{
	//ĩβ���ܴ�б��'/'
	char szDir[128] = "/datadisk/A_ball";
	
    ListDir(szDir, szDir, 0);

    return 0;  
}  

