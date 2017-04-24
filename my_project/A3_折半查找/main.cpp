#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <fstream>

using namespace std;

//nKey:需要找的数值
int SearchBinary(const int* pArray, const int nSize, const int nKey)
{
	cout<<"nSize = "<<nSize<<endl<<"nKey = "<<nKey<<endl;

	int nMid = 0, nLow = 0, nHigh = nSize - 1;
	int nIndex = 1;

	while(nLow <= nHigh)
	{
		cout<<nIndex++<<":"<<endl;
		cout<<"\t nLow = "<<nLow<<"\t"<<"nMid = "<<nMid<<"\t"<<"nHigh = "<<nHigh<<endl;

		nMid = (nLow + nHigh) / 2;

		if(nKey == pArray[nMid])
		{
			//找到
			return nMid;
		}
		else if(nKey < pArray[nMid])
		{
			//左边
			nHigh = nMid - 1;
		}
		else
		{
			//右边
			nLow = nMid + 1;
		}
	}

	//未找到
	return -1;
}


int main(int argc, char* argv[])
{
	const int nSize = 5;
	int Array[nSize] = {1, 2, 3, 4, 5};

	cout<<"i = "<<SearchBinary(Array, nSize, 1)<<endl;

	return 0;
}

