
/*
Copyright (c) 2009-2012 Maximus5
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

#include <windows.h>
#include <wchar.h>
//#if !defined(__GNUC__)
//#include <crtdbg.h>
//#else
//#define _ASSERTE(f)
//#endif

#include "usetodo.hpp"

#ifndef EXTERNAL_HOOK_LIBRARY
#define EXTERNAL_HOOK_LIBRARY
#endif

// with line number
#if !defined(_MSC_VER)

#define TODO(s)
#define WARNING(s)
#define PRAGMA_ERROR(s)

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

//#define _ASSERT(f)
//#define _ASSERTE(f)

#else

#define STRING2(x) #x
#define STRING(x) STRING2(x)
#define FILE_LINE __FILE__ "(" STRING(__LINE__) "): "
#ifdef HIDE_TODO
#define TODO(s)
#define WARNING(s)
#else
// Чтобы было удобнее работать с ErrorList - для TODO тоже префикс warning
#define TODO(s) __pragma(message (FILE_LINE "warning: TODO: " s))
#define WARNING(s) __pragma(message (FILE_LINE "warning: WARN: " s))
#endif
#define PRAGMA_ERROR(s) __pragma(message (FILE_LINE "error: " s))

//#ifdef _DEBUG
//#include <crtdbg.h>
//#endif

#endif

#ifdef _WIN64
#ifndef WIN64
WARNING("WIN64 was not defined");
#define WIN64
#endif
#endif

#ifdef _DEBUG
#define USE_SEH
#endif

#ifdef USE_SEH
	#if defined(_MSC_VER)
	#pragma message ("Compiling USING exception handler")
	#endif

	#define SAFETRY   __try
	#define SAFECATCH __except(EXCEPTION_EXECUTE_HANDLER)
#else
	#if defined(_MSC_VER)
	#pragma message ("Compiling NOT using exception handler")
	#endif

	#define SAFETRY   if (true)
	#define SAFECATCH else
#endif


#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)
#ifdef ARRAYSIZE
#define countof(a) (ARRAYSIZE(a)) // (sizeof((a))/(sizeof(*(a))))
#else
#define countof(a) (sizeof((a))/(sizeof(*(a))))
#endif
#define ZeroStruct(s) memset(&(s), 0, sizeof(s))

#define isDriveLetter(c) ((c>=L'A' && c<=L'Z') || (c>=L'a' && c<=L'z'))
#define isDigit(c) (c>=L'0' && c<=L'9')


// Для облегчения кодинга - возвращает значение для соответствующей платформы
#ifdef _WIN64
	#define WIN3264TEST(v32,v64) v64
#else
	#define WIN3264TEST(v32,v64) v32
#endif
// Чтобы можно было пользовать 64битные значения в wsprintf
#ifdef _WIN64
	#define WIN3264WSPRINT(ptr) (DWORD)(((DWORD_PTR)ptr)>>32), (DWORD)(((DWORD_PTR)ptr) & ((DWORD)-1))
#else
	#define WIN3264WSPRINT(ptr) (DWORD)(ptr)
#endif

#ifdef CONEMU_MINIMAL
#undef SHOWDEBUGSTR
#endif

#ifdef _DEBUG
extern wchar_t gszDbgModLabel[6];
#define CHEKCDBGMODLABEL if (gszDbgModLabel[0]==0) { \
		wchar_t szFile[MAX_PATH]; GetModuleFileName(NULL, szFile, MAX_PATH); \
		wchar_t* pszName = wcsrchr(szFile, L'\\'); \
		if (_wcsicmp(pszName, L"\\conemu.exe")==0) wcscpy_c(gszDbgModLabel, L"gui"); \
		else if (_wcsicmp(pszName, L"\\conemu64.exe")==0) wcscpy_c(gszDbgModLabel, L"gui"); \
		else if (_wcsicmp(pszName, L"\\conemuc.exe")==0) wcscpy_c(gszDbgModLabel, L"srv"); \
		else if (_wcsicmp(pszName, L"\\conemuc64.exe")==0) wcscpy_c(gszDbgModLabel, L"srv"); \
		else wcscpy_c(gszDbgModLabel, L"dll"); \
	}
#ifdef SHOWDEBUGSTR
#define DEBUGSTR(s) { \
	MCHKHEAP; CHEKCDBGMODLABEL; SYSTEMTIME st; GetLocalTime(&st); wchar_t szDEBUGSTRTime[1040]; _wsprintf(szDEBUGSTRTime, SKIPLEN(countof(szDEBUGSTRTime)) L"%i:%02i:%02i.%03i(%s.%i) ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, gszDbgModLabel, GetCurrentThreadId()); \
	if (lstrlen(s) < 1000) { \
		wcscat_c(szDEBUGSTRTime, s); OutputDebugString(szDEBUGSTRTime); \
	} else { \
		OutputDebugString(szDEBUGSTRTime); OutputDebugString(s); \
	}}
#else
#ifndef DEBUGSTR
#define DEBUGSTR(s)
#endif
#endif
#else
#ifndef DEBUGSTR
#define DEBUGSTR(s)
#endif
#endif

#include "MStrSafe.h"
#include "Memory.h"
