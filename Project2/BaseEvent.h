#pragma once

#include "Menu.h"

//�����¼��ķ�װ����Ϊ����                                                                    

class CBaseEvent
{
public:
	CBaseEvent();
	virtual ~CBaseEvent();

public:
	//�����¼����
	DEBUG_EVENT m_debugEvent;		//�����¼�����
	CONTEXT m_Context;				//�߳�������
	HANDLE m_hProcess;				
	HANDLE m_hThread;

	DWORD  m_dwOEP;         //������ڵ�ַ
	DWORD  m_dwBaseOfImage; //
	DWORD  m_dwSizeOfImage; //
	DWORD  m_dwBaseOfCode;  //
	DWORD  m_dwSizeOfCode;  //
	HANDLE m_hFileProcess;  //
	DWORD  m_dwFS;          //����Ƶ��ѯ��TIB

	CMENU *m_pMenu;			//���˵���ָ��
	BOOL m_bTalk;           //�Ƿ����û����н������û�����
	DWORD m_dwAddr;         //��Ҫ����ĵ�ַ

	BOOL  m_bAccessVioTF;   //���ʳ�ͻ�ĵ�����
	BOOL  m_bNormalBPTF;    //��ͨ�ϵ㵥��
	BOOL  m_bUserTF;        //�û����õ�TF
	BOOL  m_bHWBPTF;        //����Ӳ���ϵ�
	BOOL  m_bStepOverTF;    //���ڵ�������
	BOOL  m_bTraceTF;       //����׷��
	BOOL  m_bTJBPTF;	//�����ϵ�

	BOOL  m_bTmpBP;         //��ʱ��ͨ�ϵ�

	DWORD m_dwTraceBegin;   //ָ��Ҫ���ٵķ�Χ
	DWORD m_dwTraceEnd;
	BOOL  m_bTrace;         //����׷��
	BOOL  m_bTraceAll;      //��������ģ��

	DWORD m_dwLastAddr;     //����t��p�쳣����

	
	char m_path[MAX_PATH];	//���Խ��̵�·����

};
