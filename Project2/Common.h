#pragma once
#include <windows.h>
#include <Tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <map>
#include <list>
#include <vector>
#include <algorithm>


#include "Decode2Asm.h"
#pragma comment(lib,"MyDisAsm.lib")


using namespace std;

#define MAXBUF  (1024 * 100)
extern char g_szBuf[MAXBUF];

//////////////////////////////////////////////////////////////////////////
//for map<const char *, value>
class Compare
{
public:
	bool operator() (const char * pszSRC, const char * pszDST) const
	{
		return _stricmp(pszSRC, pszDST) < 0;
	}
};

//////////////////////////////////////////////////////////////////////////
//ģ����ؽṹ
typedef struct _tagModule
{
	DWORD   dwImageBase;    //Ĭ�ϼ��ص�ַ
	DWORD   modBaseAddr;    //ʵ�ʼ��صĵ�ַ
	DWORD   modBaseSize;
	DWORD   dwOEP;
	HANDLE  hFile;          //LOAD_DLL_DEBUG_INFO
	DWORD   dwBaseOfCode;
	DWORD   dwSizeOfCode;
	char   szName[MAX_MODULE_NAME32 + 1];
	char   szPath[MAX_PATH];
}tagModule;

//ԭ��
extern "C" void __stdcall Decode2AsmOpcode(IN PBYTE pCodeEntry,   // ��Ҫ����ָ���ַ
	OUT char* strAsmCode,        // �õ������ָ����Ϣ
	OUT char* strOpcode,         // ������������Ϣ
	OUT UINT* pnCodeSize,        // ����ָ���
 	IN UINT nAddress);

void
SafeClose(HANDLE handle);

/************************************************************************/
/*
Function :���Խ�ָ�����ļ����ص��ڴ��У�
/************************************************************************/
BOOL
LoadFile(char *pszFileName, char **ppFileBuf, long *pnFileSize);

