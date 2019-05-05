// Debugger.cpp: implementation of the CDebugger class.
//
//////////////////////////////////////////////////////////////////////
#include "Debugger.h"
#include <concrt.h>
#include <winternl.h>

BOOL gs_bContinue = TRUE;

//////////////////////////////////////////////////////////////////////////

//��������ָ�룬���ڵ����¼��ַ�
typedef DWORD(CDebugger::*PFNDispatchEvent)(void);
//�����¼�map����
static map<DWORD, PFNDispatchEvent> gs_mapEventID_PFN;

//������������ķַ�
typedef BOOL(CDebugger::*PFNDispatchInput)(int argc, int pargv[], const char *pszBuf);
static map<const char *, PFNDispatchInput, Compare> gs_mapInput_PFN;


//�ƶ������ĸ�����Ӧ�����ĸ�func����
void CDebugger::DispatchCommand()
{
	//�洢�¼�ID�Լ���Ӧ�Ĵ�����
#define DISPATCHEVENT(ID, pfn)  gs_mapEventID_PFN[ID] = pfn;	
		DISPATCHEVENT(EXCEPTION_DEBUG_EVENT, &CDebugger::OnExceptDispatch)				//�쳣�¼��ַ�
		DISPATCHEVENT(CREATE_THREAD_DEBUG_EVENT, &CDebugger::OnCreateThread)			//�����߳�
		DISPATCHEVENT(CREATE_PROCESS_DEBUG_EVENT, &CDebugger::OnCreateProcess)			//��������
		DISPATCHEVENT(EXIT_THREAD_DEBUG_EVENT, &CDebugger::OnExitThread)				//�˳��߳�
		DISPATCHEVENT(EXIT_PROCESS_DEBUG_EVENT, &CDebugger::OnExitProcess)				//�˳�����
		DISPATCHEVENT(LOAD_DLL_DEBUG_EVENT, &CDebugger::OnLoadDLL)						//����Dll
		DISPATCHEVENT(UNLOAD_DLL_DEBUG_EVENT, &CDebugger::OnUnLoadDLL)					//ж��Dll
		DISPATCHEVENT(OUTPUT_DEBUG_STRING_EVENT, &CDebugger::OnOutputDebugString)		//���������Ϣ
		DISPATCHEVENT(EXCEPTION_ACCESS_VIOLATION, &CDebugger::OnAccessViolation)		//���ʳ�ͻ
		DISPATCHEVENT(EXCEPTION_BREAKPOINT, &CDebugger::OnBreakPoint)					//�ϵ�
		DISPATCHEVENT(EXCEPTION_SINGLE_STEP, &CDebugger::OnSingleStep)					//����

		//�洢�û������ַ����Լ���Ӧ�Ĵ�����
#define DISPATCHINPUT(str, pfn)  gs_mapInput_PFN[str] = pfn;
		DISPATCHINPUT("bm", &CDebugger::DoBM)
		DISPATCHINPUT("bml", &CDebugger::DoBML)
		DISPATCHINPUT("bmpl", &CDebugger::DoBMPL)
		DISPATCHINPUT("bmc", &CDebugger::DoBMC)
		DISPATCHINPUT("bp", &CDebugger::DoBP);
		DISPATCHINPUT("bpl", &CDebugger::DoBPL);
		DISPATCHINPUT("bpc", &CDebugger::DoBPC);
		DISPATCHINPUT("t", &CDebugger::DoStepInto);
		DISPATCHINPUT("g", &CDebugger::DoGo);
		DISPATCHINPUT("r", &CDebugger::DoShowRegs);
		DISPATCHINPUT("bh", &CDebugger::DoBH);
		DISPATCHINPUT("bhl", &CDebugger::DoBHL);
		DISPATCHINPUT("bhc", &CDebugger::DoBHC);
		DISPATCHINPUT("p", &CDebugger::DoStepOver);
		DISPATCHINPUT("u", &CDebugger::DoShowASM);			//�鿴������
		DISPATCHINPUT("e", &CDebugger::DoModifyOpCode);			//�޸�op
		DISPATCHINPUT("d", &CDebugger::DoShowData);
		DISPATCHINPUT("?", &CDebugger::DoShowHelp);
		DISPATCHINPUT("help", &CDebugger::DoShowHelp);
		//DISPATCHINPUT("q",   &  CDebugger::Quit);
		DISPATCHINPUT("es", &CDebugger::DoExport);
		DISPATCHINPUT("ls", &CDebugger::DoImport);
		DISPATCHINPUT("log", &CDebugger::DoLog);
		DISPATCHINPUT("trace", &CDebugger::DoTrace);
		DISPATCHINPUT("vseh", &CDebugger::DoShowSEH);
		DISPATCHINPUT("mseh", &CDebugger::MonitorSEH);
		DISPATCHINPUT("modl", &CDebugger::DoListModule);
		DISPATCHINPUT("modi", &CDebugger::DoListModuleImport);
		DISPATCHINPUT("mode", &CDebugger::DoListModuleExport);
		DISPATCHINPUT("bptj", &CDebugger::DoBPtj);

		DISPATCHINPUT("dump", &CDebugger::DoDump);





}

bool CDebugger::injectDll(DWORD dwPid, HANDLE hProcess)
{

	char pszDllPath[MAX_PATH] = { "��PEB������Dll.dll" };

	bool	bRet = false;
	HANDLE	hRemoteThread = 0;
	LPVOID	pRemoteBuff = NULL;
	SIZE_T 	dwWrite = 0;
	DWORD	dwSize = 0;


	//1. ��Զ�̽����Ͽ����ڴ�ռ�
	pRemoteBuff = VirtualAllocEx(
		hProcess,
		NULL,
		64 * 1024,/*��С��64Kb*/
		MEM_COMMIT,/*Ԥ�����ύ*/
		PAGE_EXECUTE_READWRITE/*�ɶ���д��ִ�е�����*/
	);
	if (pRemoteBuff == NULL)
	{
		printf("��Զ�̽����Ͽ��ٿս�ʧ��\n");
		goto _EXIT;
	}


	//2. ��DLL·��д�뵽�¿����ڴ�ռ���
	dwSize = strlen(pszDllPath) + 1;
	WriteProcessMemory(
		hProcess,
		pRemoteBuff,/* Ҫд��ĵ�ַ */
		pszDllPath,	/* Ҫд������ݵĵ�ַ*/
		dwSize,		/* д����ֽ��� */
		&dwWrite	/* ���룺����ʵ��д����ֽ���*/
	);

	if (dwWrite != dwSize)
	{
		printf("д��Dll·��ʧ��\n");
		goto _EXIT;
	}


	//3. ����Զ���߳�
	//   Զ���̴߳����ɹ���,DLL�ͻᱻ����,DLL�����غ�DllMain����
	//	 �ͻᱻִ��,�����Ҫִ��ʲô����,����DllMain�е��ü���.

	hRemoteThread = CreateRemoteThread(
		hProcess,
		0, 0,
		(LPTHREAD_START_ROUTINE)LoadLibraryA,  /* �̻߳ص����� */
		pRemoteBuff,							/* �ص��������� */
		0, 0);

	// �ȴ�Զ���߳��˳�.
	// �˳��˲��ͷ�Զ�̽��̵��ڴ�ռ�.
	WaitForSingleObject(hRemoteThread, -1);


	bRet = true;


_EXIT:
	// �ͷ�Զ�̽��̵��ڴ�
	VirtualFreeEx(hProcess, pRemoteBuff, 0, MEM_RELEASE);
	// �رս��̾��
	CloseHandle(hProcess);

	return bRet;

}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDebugger::CDebugger()
{
	m_pDllEvent = NULL;
	m_pDllEvent = new CDllEvent;
	assert(m_pDllEvent != NULL);

	m_pProcessEvent = NULL;
	m_pProcessEvent = new CProcessEvent;
	assert(m_pProcessEvent != NULL);

	m_pExceptEvent = NULL;
	m_pExceptEvent = new CExceptEvent;
	assert(m_pExceptEvent != NULL);

	
	
	
	this->DispatchCommand();


}

CDebugger::~CDebugger()
{

}

//ȷ��ֻ��һ��ʵ��
CDebugger * CDebugger::CreateSystem(void)
{
	static CDebugger *pobj = new CDebugger;
	return pobj;
}

//ɾ������
void CDebugger::DestorySystem(void)
{
	delete this;
}


/*������ѭ��1) ��ӡѡ��2) �ַ��쳣�¼�3) �û�����4) �ַ��û�ָ�� */      
void CDebugger::Run(void)
{
	BOOL bRet = TRUE;
	char ch;
	while (true)
	{
		//��ʾ���˵�
		m_pMenu->ShowMainMenu();
		//��ȡ�û����˵�ѡ��
		m_pMenu->GetCH(&ch);

		if (ch == '1')//���Խ���
		{
			bRet = this->DebugNewProcess();
		}
		else if (ch == '2')//���ӽ���
		{
			bRet = this->DebugAttachedProcess();
		}
		else if (ch == '3')//��ʾ����
		{
			bRet = this->DoShowHelp();
		}
		else if (ch == '0')	//�˳�
		{
			break;
		}
	}
}
HANDLE temp;
//���Խ���
BOOL CDebugger::DebugNewProcess()
{
	BOOL bRet = TRUE;
	char szFilePath[MAX_PATH];

	//ѡ��Ҫ���Ե��ļ�
	bRet = m_pMenu->SelectFile(szFilePath);
	if (!bRet)
	{
		return bRet;
	}


	//������Ϣ�ṹ�����
	PROCESS_INFORMATION pi = { 0 };
	//������Ϣ
	STARTUPINFO si = { 0 };
	si.cb = sizeof(STARTUPINFO);

	//�򿪽���
	bRet = ::CreateProcess(NULL,
		szFilePath,
		NULL,
		NULL,
		FALSE,
		DEBUG_ONLY_THIS_PROCESS| CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&si,
		&pi);
	if (!bRet)//�򿪽���ʧ��
	{
		CMENU::ShowErrorMessage();	//��ʾ������Ϣ
		return FALSE;
	}


	strcpy_s(m_path, szFilePath);

	//��������
	//AntiPEBDebug( pi.hProcess);

	
	temp = pi.hProcess;

	//���Խ���
	this->DebugProcess();

	return TRUE;
}

BOOL getSeDebugPrivilge()
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return false;

	//��ȡSEDEBUG��Ȩ��LUID
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	//��ȡ������̵Ĺػ���Ȩ
	if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0))
		return false;

	return true;
}


//�����Ѿ����еĽ���(���ӽ��̣�                                                        
BOOL CDebugger::DebugAttachedProcess()
{
	m_pMenu->ShowInfo("Please Enter the PID:\r\n");
	//system("taskmgr");

	//��ȡpid
	int argc;
	int pargv[MAXBYTE];
	m_pMenu->GetInput(&argc, pargv, g_szBuf, MAXBUF);

// 	DWORD dwPID;
// 	scanf_s("%X", &dwPID);
	
 	DWORD dwPID = strtoul(g_szBuf, NULL, 10);
 	assert(dwPID != 0 && dwPID != ULONG_MAX);


	getSeDebugPrivilge();

	
	//DWORD dwPID = GetAttachPID();

	if (DebugActiveProcess(dwPID))//ʹ���������ӵ�һ������̲��ҵ�����,�ɹ�����ֵΪ����ֵ��
	{
		this->DebugProcess();
		return TRUE;
	}
	else {
		CMENU::ShowErrorMessage();	//��ʾ������Ϣ
	}

	return FALSE;
}





//������ѭ�� 1) ��ʾ���˵� 2�������¼����� 3�����û����� 4���û��������                                                 
BOOL CDebugger::DebugProcess()
{
	//����һ������ָ�룬�������ͣ�HANDLE���������ͣ�DWORD, BOOL, DWORD
	typedef HANDLE(WINAPI *OPENTHREAD)(DWORD, BOOL, DWORD);

	//��ȡKernel32,dll ��OpenThread��ַ����������ָ��ָ����
	OPENTHREAD pfnOpenThread = (OPENTHREAD)GetProcAddress(GetModuleHandle("Kernel32"), "OpenThread");
	assert(pfnOpenThread != NULL);	//ʧ�����˳�

									//���ڽ����û�����
	int argc;			//���յĸ���
	int pargv[MAXBYTE];	//���յ��ַ�������ָ��

						//���ڴ����¼��ķַ�
	map<DWORD, PFNDispatchEvent>::iterator itevt;//������
	map<DWORD, PFNDispatchEvent>::iterator itevtend = gs_mapEventID_PFN.end();//ָ��ĩβ�ĵ�����
	PFNDispatchEvent pfnEvent = NULL;	//����һ��PFNDispatchEventָ��

										//���ڴ�������ָ��ķַ�
	map<const char *, PFNDispatchInput, Compare>::iterator itinput;
	map<const char *, PFNDispatchInput, Compare>::iterator itinputend = gs_mapInput_PFN.end();
	PFNDispatchInput pfnInput = NULL;

	BOOL bRet = TRUE;
	DWORD dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;//Ĭ������Ϊ�����¼�û�б�����




	while (gs_bContinue)
	{
		//�ȴ������¼�
		bRet = ::WaitForDebugEvent(&m_debugEvent, INFINITE);
		if (!bRet)
		{
			CMENU::ShowErrorMessage();
			return FALSE;
		}

		dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;//Ĭ������Ϊ�����¼�û�б�����
		m_bTalk = FALSE;//�������û�����

						//������̾��
		m_hProcess = ::OpenProcess(
			PROCESS_ALL_ACCESS,
			FALSE,
			m_debugEvent.dwProcessId
		);
		if (NULL == m_hProcess)
		{
			CMENU::ShowErrorMessage();
			return FALSE;
		}

		//AntiPEBDebug(m_hProcess);

		//�����߳̾��������ָ��ָ��OpenThread
		m_hThread = pfnOpenThread(
			THREAD_ALL_ACCESS,	//��ȫ������
			FALSE,																//�Ƿ�̳�
			m_debugEvent.dwThreadId
		);
		if (NULL == m_hThread)
		{
			CMENU::ShowErrorMessage();
			return FALSE;
		}

		//�������������
		m_Context.ContextFlags = { CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS };
		bRet = ::GetThreadContext(m_hThread, &m_Context);
		if (!bRet)
		{
			CMENU::ShowErrorMessage();
			return FALSE;
		}


		//�����¼��ַ�
		itevt = gs_mapEventID_PFN.find(m_debugEvent.dwDebugEventCode);
		if (itevt != itevtend)
		{
			pfnEvent = (*itevt).second;//����ָ��Ϊmap�еĵڶ���������value��
			dwContinueStatus = (this->*pfnEvent)();	//�ַ��¼�������
		}

		//���û����н���
		while (m_bTalk)
		{
			m_pMenu->GetInput(&argc, pargv, g_szBuf, MAXBUF);

			//�û��������
			itinput = gs_mapInput_PFN.find(g_szBuf);//���Ҷ�Ӧ������
			if (itinput != itinputend)
			{
				pfnInput = (*itinput).second;
				(this->*pfnInput)(argc, pargv, g_szBuf);
			}
			else
			{
				m_pMenu->ShowInfo("Invalid Input\r\n");
			}
		}

		//�ָ������ģ����رվ��     
		bRet = ::SetThreadContext(m_hThread, &m_Context);
		if (!bRet)
		{
			CMENU::ShowErrorMessage();
			return FALSE;
		}

		::SafeClose(m_hThread);
		::SafeClose(m_hProcess);

		::ContinueDebugEvent(
			m_debugEvent.dwProcessId,
			m_debugEvent.dwThreadId,
			dwContinueStatus);

	}

	return TRUE;
}

//�ַ��쳣�¼�                                                                   
DWORD CDebugger::OnExceptDispatch()
{
	map<DWORD, PFNDispatchEvent>::iterator it;
	map<DWORD, PFNDispatchEvent>::iterator itend = gs_mapEventID_PFN.end();
	PFNDispatchEvent pfnEvent = NULL;
	DWORD dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;

	//�ַ�
	it = gs_mapEventID_PFN.find(m_debugEvent.u.Exception.ExceptionRecord.ExceptionCode);
	if (it != itend)
	{
		pfnEvent = (*it).second;
		dwContinueStatus = (this->*pfnEvent)();
	}

	return dwContinueStatus;
}

/************************************************************************
���ܣ�������Щ�����¼��ַ������ȵ���ͬ���¼�������
1) Create(/Exit)Process(/Thread) --->CProcessEvent
2) Load(/Unload)Dll --> CDllEvent
3) DebugString --> CDllEvent
4) Exception (BreakPoint, AccessViolation, SingleStep) --> CExceptEvent*/
/************************************************************************/
DWORD CDebugger::OnCreateThread()
{
	return m_pProcessEvent->OnCreateThread(this);
}

DWORD CDebugger::OnCreateProcess()
{
	//AntiPEBDebug(temp);

	return m_pProcessEvent->OnCreateProcess(this);
}

DWORD CDebugger::OnExitThread()
{
	return m_pProcessEvent->OnExitThread(this);
}

DWORD CDebugger::OnExitProcess()
{
	return m_pProcessEvent->OnExitProcess(this);
}

DWORD CDebugger::OnLoadDLL()
{
	return m_pDllEvent->OnLoad(this);
}
DWORD CDebugger::OnUnLoadDLL()
{
	return m_pDllEvent->OnUnload(this);
}

DWORD CDebugger::OnOutputDebugString()
{
	return m_pDllEvent->OnOutputString(this);
}

//�����쳣�¼�����
DWORD CDebugger::OnAccessViolation()
{
	return m_pExceptEvent->OnAccessViolation(this);
}

DWORD CDebugger::OnBreakPoint()
{
	return m_pExceptEvent->OnBreakPoint(this);
}

DWORD CDebugger::OnSingleStep()
{
	return m_pExceptEvent->OnSingleStep(this);
}

/************************************************************************
���ܣ��û���������ķַ�
1) ShowASM, ShowData, ShowRegs --> CBaseEvent
2) others (BP, BPL, BPC, BM, BH .etc) ---> CExceptEvent
/************************************************************************/
BOOL CDebugger::DoShowASM(int argc, int pargv[], const char *pszBuf)
{
	//u [addr]
	m_bTalk = TRUE;
	return m_pExceptEvent->DoShowASM(this, argc, pargv, pszBuf);
}

BOOL CDebugger::DoModifyOpCode(int argc, int pargv[], const char * pszBuf)
{

	//e [addr]
	m_bTalk = TRUE;
	return m_pExceptEvent->DoModifyOpCode(this, argc, pargv, pszBuf);
}



BOOL CDebugger::DoShowData(int argc, int pargv[], const char *pszBuf)
{
	//d [addr]
	m_bTalk = TRUE;

	m_pExceptEvent->DoShowData(this, argc, pargv, pszBuf);
	return TRUE;
}

//��ʾ������Ϣ
BOOL CDebugger::DoShowRegs(int argc, int pargv[], const char *pszBuf)
{
	//r
	m_bTalk = TRUE;
	m_pExceptEvent->DoShowRegs(this);
	return TRUE;
}

BOOL CDebugger::DoShowHelp(int argc/*=NULL*/, int pargv[]/*=NULL*/, const char *pszBuf/*=NULL*/)
{
	static char szBuf[1024];
	_snprintf_s(szBuf, 1024, 
		"-------------------����--------------------\r\n"
		"����	 ��ʽ                  ����\r\n"
		"t       t                     ����\r\n"
		"p       p                     ����\r\n"
		"g       g [addr]              ����\r\n"
		"r       r                     �鿴�Ĵ���\r\n"
		"u       u [addr]              �鿴������\r\n"
		"d       d [addr]              �ڴ����ݲ鿴\r\n"
		"modl    modl                  �鿴ģ����Ϣ\r\n"
		"modi    modi                  �鿴ģ�鵼���\r\n"
		"mode    mode                  �鿴ģ�鵼����\r\n"
		"bm      bm addr a|w|e len     �ڴ�ϵ�����\r\n"
		"bml     bml                   �ڴ�ϵ�鿴\r\n"
		"bmpl    bmpl                  ��ҳ���ڴ�ϵ�鿴\r\n"
		"bmc     bmc id (from bml)     Ӳ���ϵ�ɾ��\r\n"
		"bp      bp addr               һ��ϵ�����\r\n"
		"bpl     bpl                   һ��ϵ�鿴\r\n"
		"bpc     bpc id (from bpl)     һ��ϵ�ɾ��\r\n"
		"bh      bh addr e|w|a 1|2|4   Ӳ���ϵ�����\r\n"
		"bhl     bhl                   Ӳ���ϵ�鿴\r\n"
		"bhc     bhc id (from bhl)     Ӳ���ϵ�ɾ��\r\n"
		"help/?  help/?	               ����\r\n"
	);

	/*_snprintf_s(szBuf, 1024,
		"----------------����-----------------\r\n"
		"����	 ��ʽ                ����\r\n"
		"t		 t                   ����\r\n"
		"p		 p                   ����\r\n"
		"g		 g [addr]            ����\r\n"
		"r		 r                   �Ĵ����鿴\r\n"
		"u		 u [addr]            ���鿴\r\n"
		"d		 d [addr]            �ڴ����ݲ鿴\r\n"
		"modl	 modl				 �鿴ģ����Ϣ\r\n"
		"modi    modi				 �鿴ģ�鵼���\r\n"
		"mode    mode				 �鿴ģ�鵼����\r\n"
		"bm		 bm addr a|w len     �ڴ�ϵ�����\r\n"
		"bml	 bml                 �ڴ�ϵ�鿴\r\n"
		"bmpl	 bmpl                ��ҳ���ڴ�ϵ�鿴\r\n"
		"bmc	 bmc id (from bml)   Ӳ���ϵ�ɾ��\r\n"
		"bp		 bp addr             һ��ϵ�����\r\n"
		"bpl	 bpl                 һ��ϵ�鿴\r\n"
		"bpc	 bpc id (from bpl)   һ��ϵ�ɾ��\r\n"
		"bh		 bh addr e|w|a 1|2|4 Ӳ���ϵ�����\r\n"
		"bhl	 bhl                 Ӳ���ϵ�鿴\r\n"
		"bhc	 bhc id (from bhl)   Ӳ���ϵ�ɾ��\r\n"
		"log	 log                 ��¼����\r\n"
		"vseh	 vseh                �鿴seh ��\r\n"
		"mseh	 mseh                ��seh���ı仯���м��\r\n"
		"trace	 trace addrbegin addrend [dll1] [dll2]  ��ָ������Ĵ������trace\r\n"
		"help/?	 help/?                ����\r\n"
	);*/

	m_pMenu->ShowInfo(szBuf);

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////

//��������
BOOL CDebugger::DoStepOver(int argc, int pargv[], const char *pszBuf)
{
	//p
	m_bTalk = FALSE;
	return m_pExceptEvent->DoStepOver(this/*, argc, pargv, pszBuf*/);
}

//��������
BOOL CDebugger::DoStepInto(int argc, int pargv[], const char *pszBuf)
{
	//t
	m_bUserTF = TRUE;
	m_bTalk = FALSE;
	return m_pExceptEvent->DoStepInto(this/*, argc, pargv, pszBuf*/);
}

BOOL CDebugger::DoGo(int argc, int pargv[], const char *pszBuf)
{
	//g [addr]
	m_bTalk = FALSE;
	//g_GoFlag = TRUE;
	return m_pExceptEvent->DoGo(this, argc, pargv, pszBuf);
}

BOOL CDebugger::DoBP(int argc, int pargv[], const char *pszBuf)
{
	//bp addr
	m_bTalk = TRUE;
	return m_pExceptEvent->DoBP(this, argc, pargv, pszBuf);
}

BOOL CDebugger::DoBPL(int argc, int pargv[], const char *pszBuf)
{
	//bpl 
	m_bTalk = TRUE;
	return m_pExceptEvent->DoBPL(this/*, argc, pargv, pszBuf*/);
}

BOOL CDebugger::DoBPC(int argc, int pargv[], const char *pszBuf)
{
	//bpc id
	m_bTalk = TRUE;
	return m_pExceptEvent->DoBPC(this, argc, pargv, pszBuf);
}

//�����ϵ�
BOOL CDebugger::DoBPtj(int argc, int pargv[], const char *pszBuf)
{
	//bptj exx >|=|< value
	m_bTalk = TRUE;
	return m_pExceptEvent->DoBPtj(this, argc, pargv, pszBuf);
}

BOOL CDebugger::DoBM(int argc, int pargv[], const char *pszBuf)
{
	//bm addr a|w len
	m_bTalk = TRUE;
	return m_pExceptEvent->DoBM(this, argc, pargv, pszBuf, FALSE);
}

BOOL CDebugger::DoBM(int argc, int pargv[], const char *pszBuf, BOOL bTrace)
{
	//this is used for debugger
	m_bTalk = TRUE;
	return m_pExceptEvent->DoBM(this, argc, pargv, pszBuf, bTrace);
}

BOOL
CDebugger::DoBML(int argc, int pargv[], const char *pszBuf)
{
	//bml
	m_bTalk = TRUE;
	return m_pExceptEvent->DoBML(this, argc, pargv, pszBuf);
}

BOOL
CDebugger::DoBMPL(int argc, int pargv[], const char *pszBuf)
{
	//bmpl
	m_bTalk = TRUE;
	return m_pExceptEvent->DoBMPL(this, argc, pargv, pszBuf);
}

BOOL
CDebugger::DoBMC(int argc, int pargv[], const char *pszBuf)
{
	//bmc id
	m_bTalk = TRUE;
	return m_pExceptEvent->DoBMC(this, argc, pargv, pszBuf);
}

BOOL
CDebugger::DoBH(int argc, int pargv[], const char *pszBuf)
{
	//bh addr a|w|e 1|2|4
	m_bTalk = TRUE;
	return m_pExceptEvent->DoBH(this, argc, pargv, pszBuf);
}

BOOL
CDebugger::DoBHL(int argc, int pargv[], const char *pszBuf)
{
	//bhl
	m_bTalk = TRUE;
	return m_pExceptEvent->DoBHL(this/*,argc, pargv, pszBuf*/);
}

BOOL
CDebugger::DoBHC(int argc, int pargv[], const char *pszBuf)
{
	//bhc id
	m_bTalk = TRUE;
	return m_pExceptEvent->DoBHC(this, argc, pargv, pszBuf);
}

BOOL CDebugger::Quit(int argc, int pargv[], const char *pszBuf)
{
	//��δ���
	m_bTalk = FALSE;
	gs_bContinue = FALSE;
	return TRUE;
}

/************************************************************************/
/*
Function :�������û����루�ڵ��Թ����У����浽�ļ���                                                                */
/************************************************************************/
BOOL
CDebugger::DoExport(int argc, int pargv[], const char *pszBuf)
{
	//export
	m_pMenu->ExportScript();
	return TRUE;
}

/************************************************************************/
/*
Function :���ر���Ľű���������ʷ��¼����ִ��    */
/************************************************************************/
BOOL
CDebugger::DoImport(int argc, int pargv[], const char *pszBuf)
{
	//import
	m_pMenu->ImportScript();
	return TRUE;
}

/************************************************************************/
/*
Function :�������еĲ��������������������Ļ�Ͽ���ʲô
��ʱ�鵵                                       */
/************************************************************************/
BOOL CDebugger::DoLog(int argc, int pargv[], const char *pszBuf)
{
	//log
	m_bTalk = TRUE;
	m_pMenu->Log();
	return TRUE;
}

BOOL CDebugger::DoTrace(int argc, int pargv[], const char *pszBuf)
{
	//trace addrstart addrend [dll1] [dll2]
	m_bTalk = TRUE;
	m_bTrace = TRUE;
	m_pMenu->PreTrace();

	//���ָ�������trace
	m_pExceptEvent->DoTrace(this, argc, pargv, pszBuf);

	//�������ģ���trace
	if (3 == argc)
	{
		//��������ģ�����trace
		this->m_bTraceAll = FALSE;
	}
	else
	{
		//�ų�����Щģ�����trace
		this->m_bTraceAll = TRUE;
		m_pDllEvent->DoTrace(this, argc, pargv, pszBuf);
	}

	return TRUE;
}

BOOL
CDebugger::DoShowSEH(int argc, int pargv[], const char *pszBuf)
{
	//vseh
	m_bTalk = TRUE;
	return m_pExceptEvent->DoShowSEH(this, argc, pargv, pszBuf);
}

BOOL
CDebugger::MonitorSEH(int argc, int pargv[], const char *pszBuf)
{
	//������ʹ��
	return m_pExceptEvent->MonitorSEH(this);
}

BOOL
CDebugger::ReadBuf(CBaseEvent *pEvent,
	HANDLE hProcess,
	LPVOID lpAddr,
	LPVOID lpBuf,
	SIZE_T nSize)
{
	return m_pExceptEvent->ReadBuf(pEvent, hProcess, lpAddr, lpBuf, nSize);
}

//�鿴ģ��
BOOL CDebugger::DoListModule(int argc, int pargv[], const char *pszBuf)
{
	return m_pDllEvent->DoListModule(this/*, argc, pargv, pszBuf*/);
}

//ģ�鵼���
BOOL CDebugger::DoListModuleImport(int argc, int pargb[], const char * pszBuf)
{

	return m_pDllEvent->DoListModuleImport(this/*, argc, pargv, pszBuf*/);
}

//ģ�鵼����
BOOL CDebugger::DoListModuleExport(int argc, int pargb[], const char * pszBuf)
{

	return m_pDllEvent->DoListModuleExport(this/*, argc, pargv, pszBuf*/);
}

BOOL CDebugger::RemoveTrace(tagModule *pModule)
{
	return m_pExceptEvent->RemoveTrace(this, pModule);
}

BOOL CDebugger::GetModule(CBaseEvent *pEvent, DWORD dwAddr, tagModule *pModule)
{
	return m_pDllEvent->GetModule(pEvent, dwAddr, pModule);
}



//#include <Winternl.h>
//���뱻���Խ��̵ľ�����ڲ��޸�PEB��ֵ
void CDebugger::AntiPEBDebug(HANDLE hDebugProcess)
{

	typedef NTSTATUS(WINAPI*pfnNtQueryInformationProcess)
		(HANDLE ProcessHandle, ULONG ProcessInformationClass,
			PVOID ProcessInformation, UINT32 ProcessInformationLength,
			UINT32* ReturnLength);

	typedef struct _MY_PEB {               // Size: 0x1D8
		UCHAR           InheritedAddressSpace;
		UCHAR           ReadImageFileExecOptions;
		UCHAR           BeingDebugged;              //Debug���б�־
		UCHAR           SpareBool;
		HANDLE          Mutant;
		HINSTANCE       ImageBaseAddress;           //������صĻ���ַ
		struct _PEB_LDR_DATA    *Ldr;                //Ptr32 _PEB_LDR_DATA
		struct _RTL_USER_PROCESS_PARAMETERS  *ProcessParameters;
		ULONG           SubSystemData;
		HANDLE         ProcessHeap;
		KSPIN_LOCK      FastPebLock;
		ULONG           FastPebLockRoutine;
		ULONG           FastPebUnlockRoutine;
		ULONG           EnvironmentUpdateCount;
		ULONG           KernelCallbackTable;
		LARGE_INTEGER   SystemReserved;
		struct _PEB_FREE_BLOCK  *FreeList;
		ULONG           TlsExpansionCounter;
		ULONG           TlsBitmap;
		LARGE_INTEGER   TlsBitmapBits;
		ULONG           ReadOnlySharedMemoryBase;
		ULONG           ReadOnlySharedMemoryHeap;
		ULONG           ReadOnlyStaticServerData;
		ULONG           AnsiCodePageData;
		ULONG           OemCodePageData;
		ULONG           UnicodeCaseTableData;
		ULONG           NumberOfProcessors;
		LARGE_INTEGER   NtGlobalFlag;               // Address of a local copy
		LARGE_INTEGER   CriticalSectionTimeout;
		ULONG           HeapSegmentReserve;
		ULONG           HeapSegmentCommit;
		ULONG           HeapDeCommitTotalFreeThreshold;
		ULONG           HeapDeCommitFreeBlockThreshold;
		ULONG           NumberOfHeaps;
		ULONG           MaximumNumberOfHeaps;
		ULONG           ProcessHeaps;
		ULONG           GdiSharedHandleTable;
		ULONG           ProcessStarterHelper;
		ULONG           GdiDCAttributeList;
		KSPIN_LOCK      LoaderLock;
		ULONG           OSMajorVersion;
		ULONG           OSMinorVersion;
		USHORT          OSBuildNumber;
		USHORT          OSCSDVersion;
		ULONG           OSPlatformId;
		ULONG           ImageSubsystem;
		ULONG           ImageSubsystemMajorVersion;
		ULONG           ImageSubsystemMinorVersion;
		ULONG           ImageProcessAffinityMask;
		ULONG           GdiHandleBuffer[0x22];
		ULONG           PostProcessInitRoutine;
		ULONG           TlsExpansionBitmap;
		UCHAR           TlsExpansionBitmapBits[0x80];
		ULONG           SessionId;
	} MY_PEB, *PMY_PEB;


	HMODULE NtdllModule = GetModuleHandle("ntdll.dll");
	pfnNtQueryInformationProcess NtQueryInformationProcess = (pfnNtQueryInformationProcess)GetProcAddress(NtdllModule, "NtQueryInformationProcess");
	PROCESS_BASIC_INFORMATION  pbi = { 0 };	//������Ϣ
	UINT32  ReturnLength = 0;
	DWORD	dwOldProtect;	//ԭ�ڴ�ҳ����
	MY_PEB* Peb = (MY_PEB*)malloc(sizeof(MY_PEB));


	//���״̬ NTSTATUS>=0Ϊ�ɹ���NTSTATUS<0Ϊ����
	NTSTATUS Status = NtQueryInformationProcess(hDebugProcess, ProcessBasicInformation, &pbi, (UINT32)sizeof(pbi), (UINT32*)&ReturnLength);

	if (NT_SUCCESS(Status))
	{
		if (!ReadProcessMemory(hDebugProcess, (LPVOID)pbi.PebBaseAddress, Peb, sizeof(MY_PEB), NULL))
		{
			printf("Ҫ�޸ĵ��ڴ��ַ��Ч\r\n");
			
			return;
		}

		Peb->BeingDebugged = 0;
		Peb->NtGlobalFlag.u.HighPart = 0;

		WriteProcessMemory(hDebugProcess, (LPVOID)pbi.PebBaseAddress, Peb, sizeof(MY_PEB), NULL);
		
	}
}





BOOL CDebugger::DoDump(int argc, int pargv[], const char *pszBuf)
{
	//dump [addr]
	m_bTalk = TRUE;

	//return m_pExceptEvent->dump(this, argc, pargv, pszBuf);


	
	//dumpǰ�����жϵ�

	char* strPath = m_path;

	HANDLE hFile = this->m_hFileProcess;
	//CloseHandle(hFile);
	//HANDLE hFile = CreateFile(strPath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);




	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("�����ļ�ʧ��,\n");
		printf("%s\n", GetLastError());
		return FALSE;
	}
	IMAGE_DOS_HEADER dos;//dosͷ

	IMAGE_NT_HEADERS nt;
	//��dosͷ
	if (ReadProcessMemory(this->m_hProcess, (LPVOID)this->m_dwBaseOfImage, &dos, sizeof(IMAGE_DOS_HEADER), NULL) == FALSE)
	{
		return FALSE;
	}


	//��ntͷ
	if (ReadProcessMemory(this->m_hProcess, (BYTE *)this->m_dwBaseOfImage + dos.e_lfanew, &nt, sizeof(IMAGE_NT_HEADERS), NULL) == FALSE)
	{
		return FALSE;
	}


	//��ȡ���������������С
	DWORD secNum = nt.FileHeader.NumberOfSections;
	PIMAGE_SECTION_HEADER Sections = new IMAGE_SECTION_HEADER[secNum];
	//��ȡ����
	if (ReadProcessMemory(this->m_hProcess,
		(BYTE *)this->m_dwBaseOfImage + dos.e_lfanew + sizeof(IMAGE_NT_HEADERS),
		Sections,
		secNum * sizeof(IMAGE_SECTION_HEADER),
		NULL) == FALSE)
	{
		return FALSE;
	}

	//�������н����Ĵ�С
	DWORD allsecSize = 0;
	DWORD maxSec;//���Ľ���

	maxSec = 0;

	for (int i = 0; i < secNum; ++i)
	{
		allsecSize += Sections[i].SizeOfRawData;

	}

	//dos
	//nt
	//�����ܴ�С
	DWORD topsize = secNum * sizeof(IMAGE_SECTION_HEADER) + sizeof(IMAGE_NT_HEADERS) + dos.e_lfanew;

	//ʹͷ��С�����ļ�����
	if ((topsize&nt.OptionalHeader.FileAlignment) != topsize)
	{
		topsize &= nt.OptionalHeader.FileAlignment;
		topsize += nt.OptionalHeader.FileAlignment;
	}
	DWORD ftsize = topsize + allsecSize;
	//�����ļ�ӳ��
	HANDLE hMap = CreateFileMapping(hFile,
		NULL, PAGE_READWRITE,
		0,
		ftsize,
		0);

	if (hMap == NULL)
	{
		DWORD er = GetLastError();
		printf("�����ļ�ӳ��ʧ��\n");
		return FALSE;
	}

	//������ͼ
	LPVOID lpmem = MapViewOfFile(hMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

	if (lpmem == NULL)
	{
		delete[] Sections;
		CloseHandle(hMap);
		printf("������ͼʧ��\n");
		return FALSE;
	}
	PBYTE bpMem = (PBYTE)lpmem;
	memcpy(lpmem, &dos, sizeof(IMAGE_DOS_HEADER));
	//����dossub ��С

	DWORD subSize = dos.e_lfanew - sizeof(IMAGE_DOS_HEADER);

	if (ReadProcessMemory(this->m_hProcess, (BYTE *)this->m_dwBaseOfImage + sizeof(IMAGE_DOS_HEADER), bpMem + sizeof(IMAGE_DOS_HEADER), subSize, NULL) == FALSE)
	{
		delete[] Sections;
		CloseHandle(hMap);
		UnmapViewOfFile(lpmem);
		return FALSE;
	}

	nt.OptionalHeader.ImageBase = (DWORD)this->m_dwBaseOfImage;
	//����NTͷ
	memcpy(bpMem + dos.e_lfanew, &nt, sizeof(IMAGE_NT_HEADERS));

	//�������
	memcpy(bpMem + dos.e_lfanew + sizeof(IMAGE_NT_HEADERS), Sections, secNum * sizeof(IMAGE_SECTION_HEADER));

	for (int i = 0; i < secNum; ++i)
	{
		if (ReadProcessMemory(
			this->m_hProcess, (BYTE *)this->m_dwBaseOfImage + Sections[i].VirtualAddress,
			bpMem + Sections[i].PointerToRawData,
			Sections[i].SizeOfRawData,
			NULL) == FALSE)
		{
			delete[] Sections;
			CloseHandle(hMap);
			UnmapViewOfFile(lpmem);
			return FALSE;
		}
	}
	if (FlushViewOfFile(lpmem, 0) == false)
	{
		delete[] Sections;
		CloseHandle(hMap);
		UnmapViewOfFile(lpmem);
		printf("���浽�ļ�ʧ��\n");
		return FALSE;
	}
	delete[] Sections;
	CloseHandle(hMap);
	UnmapViewOfFile(lpmem);
	MessageBox(0, "ok", 0, 0);
	return TRUE;

}