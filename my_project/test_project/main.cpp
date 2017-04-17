#include "stdio.h"
#include "stdlib.h"
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

class Foo
{
public:
	Foo(int nData) : m_nData(nData){
		cout<<m_nData<<endl;
	}

	int Get(){
		return m_nData;
	}
private:
	int m_nData;
};

class Bar
{
public:
	Bar(){

	}

	int FooVal(){
		return m_Foo.Get();
		//return m_nData;
	}

private:
	static int m_nData;
	static Foo m_Foo;
};

Foo Bar::m_Foo = Foo(3);

int main(int argc, char* argv[])
{
	Bar objB;

	cout<<objB.FooVal()<<endl;

	return 0;
}

