#pragma once

#include "DllEvent.h"
#include "ProcessEvent.h"
#include "ExceptEvent.h"
#pragma comment(linker, "/NODEFAULTLIB:libcd.lib")
/************************************************************************/
/*���ڷַ���ͬ�����¼��ķ�װ                                                                    */
/************************************************************************/
//BOOL g_GoFlag = FALSE;




class CDebugger : public CBaseEvent
{
public:
	//the main entry for user to start
	static CDebugger *CreateSystem(void);
	virtual void Run(void);
	virtual void DestorySystem(void);

public:
	CDebugger();
	virtual ~CDebugger();

	//���Ժ͸��ӵ���
	virtual BOOL DebugNewProcess();
	virtual BOOL DebugAttachedProcess();

	//����������
	virtual BOOL DebugProcess();

	//��������¼�
	virtual DWORD OnExceptDispatch();  //-->OnAccessViolation(), OnBreakPoint(), OnSingleStep()
	virtual DWORD OnCreateThread();    //-->CProcessEvent.
	virtual DWORD OnCreateProcess();   //-->CProcessEvent.
	virtual DWORD OnExitThread();      //-->CProcessEvent
	virtual DWORD OnExitProcess();     //-->CProcessEvent
	virtual DWORD OnLoadDLL();         //-->CDllEvent
	virtual DWORD OnUnLoadDLL();       //-->CDllEvent
	virtual DWORD OnOutputDebugString();//-->CDllEvent
	virtual DWORD OnAccessViolation();  //-->CExceptEvent
	virtual DWORD OnBreakPoint();       //-->CExceptEvent
	virtual DWORD OnSingleStep();       //-->CExceptEvent

	//������û�����
	virtual BOOL DoShowData(int argc, int pargv[], const char *pszBuf); //-->CBaseEvent
	virtual BOOL DoShowASM(int argc, int pargv[], const char *pszBuf);  //-->CBaseEvent
	virtual BOOL DoModifyOpCode(int argc, int pargv[], const char *pszBuf);  //-->CBaseEvent
	virtual BOOL DoShowRegs(int argc, int pargv[], const char *pszBuf); //-->CBaseEvent
	virtual BOOL DoShowHelp(int argc = NULL, int pargv[] = NULL, const char *pszBuf = NULL);
	virtual BOOL Quit(int argc, int pargv[], const char *pszBuf);

	virtual BOOL DoStepOver(int argc, int pargv[], const char *pszBuf); //-->CExceptEvent
	virtual BOOL DoStepInto(int argc, int pargv[], const char *pszBuf); //-->CExceptEvent
	virtual BOOL DoGo(int argc, int pargv[], const char *pszBuf);      //-->CExceptEvent
	virtual BOOL DoBP(int argc, int pargv[], const char *pszBuf);       //-->CExceptEvent                      
	virtual BOOL DoBPL(int argc, int pargv[], const char *pszBuf);      //-->CExceptEvent
	virtual BOOL DoBPC(int argc, int pargv[], const char *pszBuf);      //-->CExceptEvent

	virtual BOOL DoBML(int argc, int pargv[], const char *pszBuf);      //-->CExceptEvent
	virtual BOOL DoBMPL(int argc, int pargv[], const char *pszBuf);      //-->CExceptEvent

	virtual BOOL DoBH(int argc, int pargv[], const char *pszBuf);       //-->CExceptEvent
	virtual BOOL DoBHL(int argc, int pargv[], const char *pszBuf);       //-->CExceptEvent
	virtual BOOL DoBHC(int argc, int pargv[], const char *pszBuf);       //-->CExceptEvent

	virtual BOOL DoBPtj(int argc, int pargv[], const char *pszBuf);       //-->CExceptEvent


	//���ڽű����������룬������־
	virtual BOOL DoExport(int argc, int pargv[], const char *pszBuf);
	virtual BOOL DoImport(int argc, int pargv[], const char *pszBuf);
	virtual BOOL DoLog(int argc, int pargv[], const char *pszBuf);

	//��չ����
	virtual BOOL DoTrace(int argc, int pargv[], const char *pszBuf);
	virtual BOOL DoListModule(int argc, int pargv[], const char *pszBuf);
	virtual BOOL DoListModuleImport(int argc, int pargb[], const char *pszBuf);	//ģ�鵼���
	virtual BOOL DoListModuleExport(int argc, int pargb[], const char * pszBuf);

public:
	virtual BOOL DoBM(int argc, int pargv[], const char *pszBuf);       //-->CExceptEvent
	virtual BOOL DoBM(int argc, int pargv[], const char *pszBuf, BOOL bTrace);       //-->CExceptEvent
	virtual BOOL DoBMC(int argc, int pargv[], const char *pszBuf);      //-->CExceptEvent


	virtual BOOL RemoveTrace(tagModule *pModule);
	virtual BOOL MonitorSEH(int argc, int pargv[], const char *pszBuf);
	virtual BOOL DoShowSEH(int argc, int pargv[], const char *pszBuf);

	//
	virtual BOOL ReadBuf(CBaseEvent *pEvent, HANDLE hProcess, LPVOID lpAddr, LPVOID lpBuf, SIZE_T nSize);
	virtual BOOL GetModule(CBaseEvent *pEvent, DWORD dwAddr, tagModule *pModule);

	BOOL DoDump(int argc, int pargv[], const char *pszBuf);
	void AntiPEBDebug(HANDLE hDebugProcess);

	

private:
	//
	void DispatchCommand();

	bool injectDll(DWORD dwPid, HANDLE hProcess);//Dllע��

protected:
	CDllEvent * m_pDllEvent;
	CProcessEvent *m_pProcessEvent;
	CExceptEvent *m_pExceptEvent;
};