
/*
Copyright (c) 2009-2017 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#ifndef MASSERT_HEADER_DEFINED
#define MASSERT_HEADER_DEFINED

#ifndef _CRT_WIDE
#define __CRT_WIDE(_String) L ## _String
#define _CRT_WIDE(_String) __CRT_WIDE(_String)
#endif

typedef int (*AppMsgBox_t)(LPCTSTR lpText, UINT uType, LPCTSTR lpCaption /*= NULL*/, HWND hParent /*= NULL*/, bool abLock /*= true*/);
extern AppMsgBox_t AssertMsgBox;

typedef bool (*HooksUnlockerProc_t)(bool bUnlock);
extern HooksUnlockerProc_t gfnHooksUnlockerProc /* = NULL*/;

LPCWSTR StripSourceRoot(LPCWSTR pszFile);

#ifdef _DEBUG
class CHooksUnlocker
{
protected:
	// Members
	static LONG mn_LockCount;
	static bool mb_Processed;
	bool mb_WasDebugger;

public:
	CHooksUnlocker();
	~CHooksUnlocker();
};
#define HooksUnlocker CHooksUnlocker HkUnlck
#else
#define HooksUnlocker
#endif

#if defined(_DEBUG)
	#if !defined(__GNUC__)
	#include <crtdbg.h>
	#endif

	enum CEAssertMode
	{
		am_Default = 0,
		am_Thread = 1,
		am_Pipe = 2
	};
	extern CEAssertMode gAllowAssertThread;

	int MyAssertProc(const wchar_t* pszFile, int nLine, const wchar_t* pszTest, bool abNoPipe);
	void MyAssertDumpToFile(const wchar_t* pszFile, int nLine, const wchar_t* pszTest);
	void MyAssertTrap();
	void MyAssertShutdown();
	extern LONG gnInMyAssertTrap;

	#define MY_ASSERT_EXPR(expr, msg, nopipe) \
		if (!(expr)) { \
			if (1 != MyAssertProc(_CRT_WIDE(__FILE__), __LINE__, msg, nopipe)) \
				MyAssertTrap(); \
		}

	//extern int MDEBUG_CHK;
	//extern char gsz_MDEBUG_TRAP_MSG_APPEND[2000];
	//#define MDEBUG_TRAP1(S1) {strcpy(gsz_MDEBUG_TRAP_MSG_APPEND,(S1));_MDEBUG_TRAP(__FILE__,__LINE__);}
	extern bool gbAllowChkHeap;
	#if !defined(__GNUC__)
	#define MCHKHEAP MY_ASSERT_EXPR(!gbAllowChkHeap||_CrtCheckMemory(),L"_CrtCheckMemory failed",false);
	#else
	#define MCHKHEAP
	#define _CrtDbgBreak()
	#endif

	#define ASSERT(expr)   MY_ASSERT_EXPR((expr), _CRT_WIDE(#expr), false)
	#define ASSERTE(expr)  MY_ASSERT_EXPR((expr), _CRT_WIDE(#expr), false)
	#define ASSERTEX(expr) MY_ASSERT_EXPR((expr), _CRT_WIDE(#expr), true)

#else
	#define MY_ASSERT_EXPR(expr, msg)
	#define ASSERT(expr)
	#define ASSERTE(expr)
	#define ASSERTEX(expr)
	#define MCHKHEAP
	#define MyAssertTrap()
#endif

#undef _ASSERT
#undef _ASSERTE
#undef _ASSERTEX

#ifdef _DEBUG
bool MyAssertSkip(const wchar_t* pszFile, int nLine, const wchar_t* pszTest, bool abNoPipe);
#else
#define MyAssertSkip(pszFile, nLine, pszTest, abNoPipe)
#endif

#if CE_UNIT_TEST==1

	extern bool gbVerifyFailed;
	extern bool gbVerifyStepFailed;

	#include <stdio.h>
	extern const char* PointToName(const char* asFileOrPath);
	#define _ASSERT(x) if (!(x)) { gbVerifyStepFailed=gbVerifyFailed=true; \
		MyAssertSkip(_CRT_WIDE(__FILE__), __LINE__, L"Unit test assertion", true); \
		printf(\
		"    \033[91m" \
		    "ASSERTION: %s\n" \
		"         FILE: %s LINE: %i" \
		"\033[m\n", (#x), PointToName(__FILE__), __LINE__); }
	#define _ASSERTE(x) _ASSERT(x)
	#define _ASSERTEX(x) _ASSERT(x)

	#define Verify_Step(step) gbVerifyStepFailed = false; printf("  \033[36m%s\033[m\n" , step)
	#define Verify_MsgOk(msg) printf("  \033[32m" "%s" "\033[m\n",(msg))
	#define Verify_MsgFail(msg) printf("  \033[91m" "%s" "\033[m\n",(msg))
	#define Verify4(cond,errfmt,a1,a2,a3,a4) \
		if (!(cond)) printf("    \033[91m" errfmt "\033[m\n",(a1),(a2),(a3),(a4)), gbVerifyStepFailed=gbVerifyFailed=true; \
		        else printf("    \033[32m" errfmt "\033[m\n",(a1),(a2),(a3),(a4))
	#define Verify3(cond,errfmt,a1,a2,a3) Verify4(cond,errfmt,a1,a2,a3,0)
	#define Verify2(cond,errfmt,a1,a2) Verify3(cond,errfmt,a1,a2,0)
	#define Verify1(cond,errfmt,a1) Verify2(cond,errfmt,a1,0)
	#define Verify0(cond,errfmt) Verify1(cond,errfmt,0)
	#define WVerify4(cond,errfmt,a1,a2,a3,a4) \
		if (!(cond)) wprintf(L"    \033[91m" errfmt L"\033[m\n",(a1),(a2),(a3),(a4)), gbVerifyStepFailed=gbVerifyFailed=true; \
		        else wprintf(L"    \033[32m" errfmt L"\033[m\n",(a1),(a2),(a3),(a4))
	#define WVerify3(cond,errfmt,a1,a2,a3) WVerify4(cond,errfmt,a1,a2,a3,0)
	#define WVerify2(cond,errfmt,a1,a2) WVerify3(cond,errfmt,a1,a2,0)
	#define WVerify1(cond,errfmt,a1) WVerify2(cond,errfmt,a1,0)
	#define WVerify0(cond,errfmt) WVerify1(cond,errfmt,0)

#else

	#define _ASSERT(x) ASSERT(x)
	#define _ASSERTE(x) ASSERTE(x)
	#define _ASSERTEX(x) ASSERTEX(x)

#endif

#endif // #ifndef MASSERT_HEADER_DEFINED
