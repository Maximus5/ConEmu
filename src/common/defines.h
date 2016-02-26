
/*
Copyright (c) 2009-2016 Maximus5
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
#define TODO(s) __pragma(message (FILE_LINE /*"warning: "*/ "TODO: " s))
#define WARNING(s) __pragma(message (FILE_LINE /*"warning: "*/ "WARN: " s))
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

#if defined(DONT_USE_SEH) || defined(__GNUC__)
#undef USE_SEH
#elif defined(_DEBUG)
#define USE_SEH
#endif

#ifdef USE_SEH
	#if defined(_MSC_VER) && !defined(HIDE_USE_EXCEPTION_INFO)
	#pragma message ("Compiling USING exception handler")
	#endif

	#define SAFETRY   __try
	#define SAFECATCH __except(EXCEPTION_EXECUTE_HANDLER)
	#define SAFECATCHFILTER(f) __except(f)
#else
	#if defined(_MSC_VER) && !defined(HIDE_USE_EXCEPTION_INFO)
	#pragma message ("Compiling NOT using exception handler")
	#endif

	#define SAFETRY   if (true)
	#define SAFECATCH else
	#define SAFECATCHFILTER(f) else
#endif


#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)

#ifdef ARRAYSIZE
#define countof(a) (ARRAYSIZE(a)) // (sizeof((a))/(sizeof(*(a))))
#else
#define countof(a) (sizeof((a))/(sizeof(*(a))))
#endif
#define ZeroStruct(s) memset(&(s), 0, sizeof(s))

#define SafeCloseHandle(h) { if ((h) && (h)!=INVALID_HANDLE_VALUE) { HANDLE hh = (h); (h) = NULL; if (hh!=INVALID_HANDLE_VALUE) CloseHandle(hh); } }
#define SafeRelease(p) if ((p)!=NULL) { (p)->Release(); (p)=NULL; }
#define SafeDeleteObject(h) if ((h)!=NULL) { DeleteObject((h)); (h)=NULL; }

#if defined(_MSC_VER) && (_MSC_VER <= 1500)
	#undef HAS_CPP11
#elif defined(__GNUC__)
	#define HAS_CPP11
#else
	#define HAS_CPP11
#endif

#if defined(HAS_CPP11)
	#define RVAL_REF &&
	#define FN_DELETE = delete
#else
	#define RVAL_REF
	#define FN_DELETE
#endif

#define ScopedObject_Cat2(n,i) n ## i
#define ScopedObject_Cat1(n,i) ScopedObject_Cat2(n,i)
#define ScopedObject(cls) cls ScopedObject_Cat1(cls,__LINE__)

#define isDriveLetter(c) ((c>=L'A' && c<=L'Z') || (c>=L'a' && c<=L'z'))
#define isDigit(c) (c>=L'0' && c<=L'9')
#define isAlpha(c) (IsCharAlpha(c))
#define isSpace(c) (wcschr(L" \xA0\t\r\n",c)!=NULL)

#define LODWORD(ull) ((DWORD)((ULONGLONG)(ull) & 0x00000000ffffffff))
#define LOLONG(ull)  ((LONG)LODWORD(ull))
#define HIDWORD(ull) ((DWORD)((ULONGLONG)(ull)>>32))
#define LOSHORT(ll)  ((SHORT)LOWORD(ll))

#define RectWidth(rc) ((rc).right-(rc).left)
#define RectHeight(rc) ((rc).bottom-(rc).top)
#define LOGRECTCOORDS(rc) (rc).left, (rc).top, (rc).right, (rc).bottom
#define LOGRECTSIZE(rc) RectWidth(rc), RectHeight(rc)

#define _abs(n) (((n)>=0) ? (n) : -(n))

#define LDR_IS_DATAFILE(hm)      ((((ULONG_PTR)(hm)) & (ULONG_PTR)1) == (ULONG_PTR)1)
#define LDR_IS_IMAGEMAPPING(hm)  ((((ULONG_PTR)(hm)) & (ULONG_PTR)2) == (ULONG_PTR)2)
#define LDR_IS_RESOURCE(hm)      (LDR_IS_IMAGEMAPPING(hm) || LDR_IS_DATAFILE(hm))

#ifndef WM_WTSSESSION_CHANGE
#define WM_WTSSESSION_CHANGE 0x02B1
#endif

#define CONFORECOLOR(x) ((x & 0xF))
#define CONBACKCOLOR(x) ((x & 0xF0)>>4)

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
// И еще
#ifdef _DEBUG
	#define RELEASEDEBUGTEST(rel,dbg) dbg
	#define DEBUGTEST(dbg) dbg
#else
	#define RELEASEDEBUGTEST(rel,dbg) rel
	#define DEBUGTEST(dbg)
#endif
// GNU
#ifndef __GNUC__
	#define VCGCCTEST(vc,gcc) vc
#else
	#define VCGCCTEST(vc,gcc) gcc
#endif

#if defined(DEPRECATED)
#undef DEPRECATED
#endif
#ifdef __GNUC__
#define DEPRECATED(msg,func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED(msg,func) __declspec(deprecated(msg)) func
#else
#define DEPRECATED(msg,func) func
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
extern void _DEBUGSTR(LPCWSTR s);
#define DEBUGSTR(s) _DEBUGSTR(s)
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

#include "kl_parts.h"
#include "MStrSafe.h"
#include "Memory.h"

// Compares the *pv value with the cmp value. If the *pv value is equal to the cmp value,
// the 0 value is stored in the address specified by Destination.
// Otherwise, no operation is performed.
// pv MUST be either DWORD* or LONG*
#define InterlockedCompareZero(pv,cmp) InterlockedCompareExchange((LONG*)pv,0,cmp)
