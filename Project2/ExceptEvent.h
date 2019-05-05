
#pragma once

#include "BaseEvent.h"

/************************************************************************
�����ϵ㣬���������ʳ�ͻ ��Ӳ���ϵ㣬�洢���ϵ�* /
/************************************************************************/


//�����ڴ�ϵ�Ľṹ
typedef struct _tagMemBP
{
#define MEMBP_ACCESS 0		 //���ʶϵ�
#define MEMBP_WRITE  1		 //д��ϵ�
#define MEMBP_EXECUTE  2	 //д��ϵ�

	DWORD dwAddr;
	DWORD dwSize;
	BOOL  bTrace;        //����׷��
	DWORD dwType;        //�ϵ����ͣ����ʡ�д��
	bool operator == (const _tagMemBP &obj)
	{
		return ((dwAddr == obj.dwAddr)
			&& (dwSize == obj.dwSize)
			&& (dwType == obj.dwType)
			&& (bTrace == obj.bTrace)
			);
	}
}tagMemBP;


//����ҳ�����ض�λ��Ľṹ��ά��ҳ�ڶϵ���Ϣ

//�ڴ�ϵ��ڷ�ҳ�ڵı�ʾ
typedef struct _tagMemBPInPage
{
	WORD wOffset;       //��ҳ�ڵ�ƫ��
	WORD wSize;         //��ҳ�ڵĴ�С
	BOOL bTrace;        //����׷��
	bool operator == (const _tagMemBPInPage &obj)
	{
		return ((wOffset == obj.wOffset)
			&& (wSize == obj.wSize)
			);
	}
}tagMemBPInPage;

//��ҳ��ϵ�
typedef struct _tagPageBP
{
	DWORD dwPageAddr;   //��ҳ��ַ
	DWORD dwOldProtect;
	DWORD dwNewProtect;
	list<tagMemBPInPage> lstMemBP;
}tagPageBP;

//////////////////////////////////////////////////////////////////////////
//������ͨ�ϵ�Ľṹ
//�����Ϊ��ͨ�ϵ㣬һ��ϵ㣬����˵��INT3������CC������Ϊ���ǵ��ֽڵ���Ȩָ���������������
typedef struct _tagNormalBP
{
	byte oldvalue;			//ԭ����
	byte bTmp : 1;          //��ʱ�ϵ㣬Debugger�ڲ�����
	byte bPerment : 1;		//�û�ͨ��bp����
	byte bDisabled : 1;     //���ڴ����int3��������ͨ�ϵ�
}tagNormalBP;

//////////////////////////////////////////////////////////////////////////
//����Ӳ���Ľṹ
typedef struct _tagDR7
{
	unsigned /*char*/ GL0 : 2;
	unsigned /*char*/ GL1 : 2;
	unsigned /*char*/ GL2 : 2;
	unsigned /*char*/ GL3 : 2;
	unsigned /*char*/ GLE : 2;     // 11
	unsigned /*char*/ Reserv0 : 3; // 001
	unsigned /*char*/ GD : 1;     // 0
	unsigned /*char*/ Reserv1 : 2; //00
	unsigned /*char*/ RW0 : 2;
	unsigned /*char*/ LEN0 : 2;
	unsigned /*char*/ RW1 : 2;
	unsigned /*char*/ LEN1 : 2;
	unsigned /*char*/ RW2 : 2;
	unsigned /*char*/ LEN2 : 2;
	unsigned /*char*/ RW3 : 2;
	unsigned /*char*/ LEN3 : 2;
#define DR7INIT 0x00000700  //Reserv1:00 GD:0 Reserv0:001  GELE:11
}tagDR7;

typedef struct _tagDR6
{
	unsigned /*char*/ B0 : 1;
	unsigned /*char*/ B1 : 1;
	unsigned /*char*/ B2 : 1;
	unsigned /*char*/ B3 : 1;
	unsigned /*char*/ Reserv0 : 8;      //11111111
	unsigned /*char*/ Reserv1 : 1;    //0
	unsigned /*char*/ BD : 1;
	unsigned /*char*/ BS : 1;
	unsigned /*char*/ BT : 1;
	unsigned /*char*/ Reserv2 : 16;              //set to 1
}tagDR6;

typedef struct _tagHWBP
{
	DWORD dwAddr;
	DWORD dwType;
	DWORD dwLen;
	DWORD *pDRAddr[4];      //for DR0 ~ DR3
	DWORD RW[4];          //for DR7:RW0 ~ RW3
#define HWBP_EXECUTE 0  //ִֻ��ָ��
#define HWBP_WRITE   1  //ֻ����д��
#define HWBP_ACCESS  3  //���� �� ִ��
#define STREXECUTE  "Execute"	//ִ��
#define STRWRITE    "Write"		//д��
#define STRACCESS   "Access"	//����
}tagHWBP;

//////////////////////////////////////////////////////////////////////////
//�����IA1.pdf 3.4.3 EFLAG�Ĵ���
typedef struct _tagEFlags
{
	unsigned /*char*/ CF : 1;
	unsigned /*char*/ Reserv1 : 1; //1
	unsigned /*char*/ PF : 1;
	unsigned /*char*/ Reserv2 : 1; //0
	unsigned /*char*/ AF : 1;
	unsigned /*char*/ Reserv3 : 1; //0
	unsigned /*char*/ ZF : 1;
	unsigned /*char*/ SF : 1;
	unsigned /*char*/ TF : 1;
	unsigned /*char*/ IF : 1;
	unsigned /*char*/ DF : 1;
	unsigned /*char*/ OF : 1;
	//others
	unsigned /*char*/ IOPL : 2;
	unsigned /*char*/ NT : 1;
	unsigned /*char*/ Reserv4 : 1; //0
	unsigned /*char*/ Remain : 16;
}tagEFlags;

//////////////////////////////////////////////////////////////////////////
//about seh
typedef struct _tagSEH
{
	DWORD ptrNext;      //pointer to next seh record
	DWORD dwHandler;    //SEH handler
}tagSEH;

//////////////////////////////////////////////////////////////////////////


//�����ϵ�
typedef	struct tagTJBP
{
	DWORD dwAddr;
	char strExx[4];		//�Ĵ���
	char  strSymbol[4];	//����
	DWORD   dwValue;	//ֵ
}tagTJBP;


class CExceptEvent : public CBaseEvent
{
public:
	CExceptEvent();
	virtual ~CExceptEvent();

public:
	//debug event
	virtual DWORD OnAccessViolation(CBaseEvent *pEvent);
	virtual DWORD OnBreakPoint(CBaseEvent *pEvent);
	virtual DWORD OnSingleStep(CBaseEvent *pEvent);

	//user input
	virtual BOOL DoStepOver(CBaseEvent *pEvent/*, int argc, int pargv[], const char *pszBuf*/);
	virtual BOOL DoStepInto(CBaseEvent *pEvent/*, int argc, int pargv[], const char *pszBuf*/);
	virtual BOOL DoGo(CBaseEvent *pEvent, int argc, int pargv[], const char *pszBuf);
	virtual BOOL DoBP(CBaseEvent *pEvent, int argc, int pargv[], const char *pszBuf);
	BOOL DoBPtemp(CBaseEvent * pEvent, DWORD dwAddr);
	virtual BOOL DoBPL(CBaseEvent *pEvent/*, int argc, int pargv[], const char *pszBuf*/);
	virtual BOOL DoBPC(CBaseEvent *pEvent, int argc, int pargv[], const char *pszBuf);

	virtual BOOL DoBM(CBaseEvent *pEvent, int argc, int pargv[], const char *pszBuf, BOOL bTrace);
	virtual BOOL DoBML(CBaseEvent *pEvent, int argc, int pargv[], const char *pszBuf);
	virtual BOOL DoBMPL(CBaseEvent *pEvent, int argc, int pargv[], const char *pszBuf);
	virtual BOOL DoBMC(CBaseEvent *pEvent, int argc, int pargv[], const char *pszBuf);

	virtual BOOL DoBH(CBaseEvent *pEvent, int argc, int pargv[], const char *pszBuf);
	virtual BOOL DoBHL(CBaseEvent *pEvent/*, int argc, int pargv[], const char *pszBuf*/);
	virtual BOOL DoBHC(CBaseEvent *pEvent, int argc, int pargv[], const char *pszBuf);

	virtual BOOL DoBPtj(CBaseEvent *pEvent, int argc, int pargv[], const char *pszBuf);//�����ϵ�
	virtual BOOL dump(CBaseEvent *pEvent, int argc, int pargv[], const char *pszBuf);


	//show
	virtual BOOL DoShowData(CBaseEvent *pEvent, int argc, int pargv[], const char *pszBuf);
	virtual BOOL DoShowASM(CBaseEvent *pEvent, int argc, int pargv[], const char *pszBuf);
	virtual BOOL DoModifyOpCode(CBaseEvent *pEvent, int argc, int pargv[], const char *pszBuf);
	virtual void ShowTwoASM(CBaseEvent *pEvent, DWORD dwAddr = NULL);
	virtual void DoShowRegs(CBaseEvent *pEvent);
	virtual const char * ShowOneASM(CBaseEvent *pEvent, DWORD dwAddr = NULL,
		UINT *pnCodeSize = NULL);
	virtual const char * GetOneASM(CBaseEvent *pEvent, DWORD dwAddr = NULL,
		UINT *pnCodeSize = NULL, BOOL bGetAPIName = TRUE);


	//extended function
	virtual BOOL DoTrace(CBaseEvent *pEvent, int argc, int pargv[], const char *pszBuf);
	virtual BOOL RemoveTrace(CBaseEvent *pEvent, tagModule *pModule);
	virtual BOOL DoShowSEH(CBaseEvent *pEvent, int argc, int pargv[], const char *pszBuf);
	virtual BOOL MonitorSEH(CBaseEvent *pEvent);
	virtual DWORD GetTIB(CBaseEvent *pEvent);

	//
	virtual BOOL ReadBuf(CBaseEvent *pEvent, HANDLE hProcess, LPVOID lpAddr, LPVOID lpBuf, SIZE_T nSize);
protected:
	//���ڶϵ�
	BOOL CheckHitMemBP(CBaseEvent *pEvent, DWORD dwAddr, tagPageBP *ppageBP);
	BOOL CheckBMValidity(CBaseEvent *pEvent, tagMemBP *pMemBP);
	BOOL IsPageValid(CBaseEvent *pEvent, DWORD dwAddr);
	BOOL IsPageReadable(CBaseEvent *pEvent, DWORD dwAddr);
	BOOL HasMemBP(CBaseEvent *pEvent, DWORD dwAddr, tagPageBP **ppPageBP);
	BOOL HasNormalBP(CBaseEvent *pEvent, DWORD dwAddr, tagNormalBP **ppNormalBP);
	BOOL HasOtherMemBP(CBaseEvent *pEvent, DWORD dwPageAddr, tagPageBP **ppPageBP, DWORD *pnTotal);
	BOOL SetHWBP(CBaseEvent *pEvent, tagHWBP *pHWBP);
	BOOL HasHitHWBP(CBaseEvent *pEvent);
	BOOL CheckHitTJBP(CBaseEvent *pEvent ,DWORD dwAddr);		//�Ƿ���������ϵ�

	virtual BOOL IsCall(CBaseEvent *pEvent, DWORD dwAddr, UINT *pnLen);	//�Ƿ��ǵ��ú���
	virtual BOOL IsJxx(CBaseEvent *pEvent, DWORD dwAddr, UINT *pnLen);	//�Ƿ�����ת
	virtual BOOL GetAPIName(CBaseEvent *pEvent, DWORD dwCode, const char *pszBuf, char szAPIName[]);
	virtual BOOL GetAPINameFromOuter(CBaseEvent *pEvent, DWORD dwAddr, tagModule *pModule, char szAPIName[]);
	virtual BOOL IsSameModule(CBaseEvent *pEvent, DWORD dwCode, DWORD dwAddr, tagModule *pModule);

	

	//����׷��
	virtual void PrefetchCode(CBaseEvent *pEvent);

protected:
	list<tagMemBP> m_lstMemBP;                 //�������ڴ�ϵ�
	map<DWORD, tagPageBP> m_mapPage_PageBP;    //����ҳά���Ķϵ�

	map<DWORD, tagNormalBP> m_mapAddr_NormBP;  //һ��ϵ�

	vector<tagTJBP>m_vecTJBP;	//�����ϵ�

	map<char *, char *, Compare> m_mapModule_Export;  //ģ��ĵ�����Ϣ

	DWORD m_dwPageSize;

	//����ɾ���ظ�ָ���repxx
	char m_szLastASM[MAXBYTE];
};

