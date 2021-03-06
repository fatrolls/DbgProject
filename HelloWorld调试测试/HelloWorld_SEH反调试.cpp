// HelloWorld调试测试.cpp: 定义控制台应用程序的入口点。
//
// HelloWorld.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <windows.h>
#pragma warning(disable: 4733) // 忽略“内联 asm 分配到FS:0”错误


void(*g_funExceptionReturn)() = nullptr;



void RightFun()
{
	MessageBox(0, L"正常运行！", L"SEH反调试测试", MB_OK);
	system("pause");
	for (int i = 0; i < 5; ++i)
		printf("hello world\n");

	// 	__try {
	// 		*(int*)0 = 0;
	// 	}
	// 	__except (EXCEPTION_EXECUTE_HANDLER) {
	// 		printf("__except\n");
	// 	}
	printf("exit\n");
	// 删除异常处理器
	//     运行至此处时栈顶的信息是EXCEPTION_REGISTRATION_RECORD，里面
	// 保存着其最初运行的已成处理器，我们将其保存到FS:[0]处后就相当于
	// 摘除了前面注册的SEH异常链信息。
	__asm pop dword ptr fs : [0];
	g_funExceptionReturn();
}



void WrongFun()
{
	MessageBox(0, L"检测到调试器！", L"SEH反调试测试", MB_OK);
	ExitProcess(0);
	// 删除异常处理器
	//     运行至此处时栈顶的信息是EXCEPTION_REGISTRATION_RECORD，里面
	// 保存着其最初运行的已成处理器，我们将其保存到FS:[0]处后就相当于
	// 摘除了前面注册的SEH异常链信息。
	__asm pop dword ptr fs : [0];
}



EXCEPTION_DISPOSITION ExpHandel_A(
	EXCEPTION_RECORD              *pExceptionRecord,   // 异常状态描述
	EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,  // 异常注册框架
	CONTEXT                       *pContextRecord,     // 返回线程上下文
	PVOID                         pDispatcherContext) // 分发器上下文（系统使用，无需关注）
{
	// 1. 获取调试标志位
	BYTE IsBeginDebug = false;
	__asm {
		push eax;
		mov eax, dword ptr fs : [0x30]; // 获取PEB
		mov al, byte ptr ds : [eax + 2]; // 获取PEB.BeginDebug
		mov IsBeginDebug, al;
		pop eax;
	};

	// 2. 备份正确的异常返回地址
	g_funExceptionReturn = (void(*)())pContextRecord->Eip;

	// 3. 根据标志位执行不同的SEH异常处理流程
	if (IsBeginDebug)
		pContextRecord->Eip = (DWORD)WrongFun;
	else
		pContextRecord->Eip = (DWORD)RightFun;

	// 4. 返回“已经正常处理”
	return ExceptionContinueExecution;
}



EXCEPTION_DISPOSITION ExpHandel_B(
	EXCEPTION_RECORD              *pExceptionRecord,   // 异常状态描述
	EXCEPTION_REGISTRATION_RECORD *pEstablisherFrame,  // 异常注册框架
	CONTEXT                       *pContextRecord,     // 返回线程上下文
	PVOID                         pDispatcherContext) // 分发器上下文（系统使用，无需关注）
{
	// 返回“运行下一个异常处理器”
	return ExceptionContinueSearch;
}


int _tmain(int argc, _TCHAR* argv[])
{
	// 1. 添加SEH异常链
	// 以下异常链添加完成后为：
	// [Start]                                                                  [End]
	// *-------------*    *-------------*    *------------*            *------------* 
	// | Next -------+--> | Next -------+--> | Next ------+--> ... --> | 0xFFFFFFFF |
	// | ExpHandel_B |    | ExpHandel_A |    | ?????????? |            | ?????????? |    
	// *-------------*    *-------------*    *------------*            *------------*    
	__asm { /* 添加SEH异常链A */
		push ExpHandel_A;
		push dword ptr fs : [0];
		mov dword ptr fs : [0], esp;
	};
	__asm { /* 添加SEH异常链B */
		push ExpHandel_B;
		push dword ptr fs : [0];
		mov dword ptr fs : [0], esp;
	};

	// 2. 人为制造异常
	int *pTest = nullptr;
	*pTest = 0;

	return 0;
}