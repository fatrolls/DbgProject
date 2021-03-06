// HelloWorld_PEB反调试.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <windows.h>

bool PEB_BegingDebugged()
{
	bool BegingDebugged = false;
	_asm
	{
		mov eax, DWORD ptr FS : [0x30];		//获取PEB地址
		mov al, BYTE ptr DS : [eax + 0x02];	//获取PEB.BegingDebugged
		mov BegingDebugged, al
	}
	return BegingDebugged;
}

int main()
{
	if (PEB_BegingDebugged())
	{
		MessageBox(NULL, L"发现调试程序!", L"警告", MB_OK);
		ExitProcess(0);
	}
	for (int i = 0; i < 5; ++i)
		printf("hello world\n");
	printf("exit\n");
	system("pause");
	return 0;
}
