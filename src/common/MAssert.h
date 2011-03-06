
#pragma once

#ifndef _CRT_WIDE
#define __CRT_WIDE(_String) L ## _String
#define _CRT_WIDE(_String) __CRT_WIDE(_String)
#endif

#ifdef _DEBUG
	#include <crtdbg.h>

	enum CEAssertMode
	{
		am_Default = 0,
		am_Thread = 1,
		am_Pipe = 2
	};
	extern CEAssertMode gAllowAssertThread;

	int MyAssertProc(const wchar_t* pszFile, int nLine, const wchar_t* pszTest);
	void MyAssertTrap();
	void MyAssertShutdown();
	extern bool gbInMyAssertTrap;

	#define MY_ASSERT_EXPR(expr, msg) \
		if (!(expr)) { \
			if (1 != MyAssertProc(_CRT_WIDE(__FILE__), __LINE__, msg)) \
				MyAssertTrap(); \
		}

	//extern int MDEBUG_CHK;
	//extern char gsz_MDEBUG_TRAP_MSG_APPEND[2000];
	//#define MDEBUG_TRAP1(S1) {strcpy(gsz_MDEBUG_TRAP_MSG_APPEND,(S1));_MDEBUG_TRAP(__FILE__,__LINE__);}
	#define MCHKHEAP MY_ASSERT_EXPR(_CrtCheckMemory(),L"_CrtCheckMemory failed");

	#define ASSERT(expr)   MY_ASSERT_EXPR((expr), _CRT_WIDE(#expr))
	#define ASSERTE(expr)  MY_ASSERT_EXPR((expr), _CRT_WIDE(#expr))

#else
	#define MY_ASSERT_EXPR(expr, msg)
	#define ASSERT(expr)
	#define ASSERTE(expr)
	#define MCHKHEAP
#endif

#undef _ASSERT
#define _ASSERT ASSERT
#undef _ASSERTE
#define _ASSERTE ASSERTE
