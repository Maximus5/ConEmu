
/*
Copyright (c) 2009-2010 Maximus5
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
#include <strsafe.h>
//#if !defined(__GNUC__)
//#include <crtdbg.h>
//#else
//#define _ASSERTE(f)
//#endif

#include "usetodo.hpp"

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

    #define _ASSERT(f)
    #define _ASSERTE(f)
    
#else

    #define STRING2(x) #x
    #define STRING(x) STRING2(x)
    #define FILE_LINE __FILE__ "(" STRING(__LINE__) "): "
    #ifdef HIDE_TODO
    #define TODO(s) 
    #define WARNING(s) 
    #else
    #define TODO(s) __pragma(message (FILE_LINE "TODO: " s))
    #define WARNING(s) __pragma(message (FILE_LINE "warning: " s))
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

template <size_t size>
int swprintf_c(wchar_t (&Buffer)[size], const wchar_t *Format, ...)
{
	va_list argList;
	va_start(argList, Format);
	int nRc;
#ifdef _DEBUG
	nRc = StringCchVPrintfW(Buffer, size, Format, argList);
#else
	nRc = wvsprintf(Buffer, Format, argList);
#endif
	return nRc;
};
template <size_t size>
int swprintf_add(wchar_t* Buffer, wchar_t (&BufferStart)[size], const wchar_t *Format, ...)
{
	va_list argList;
	va_start(argList, Format);
	size_t SizeLeft = size - (Buffer - BufferStart);
	if (SizeLeft > size)
	{
		_ASSERTE(SizeLeft <= size);
		return E_POINTER;
	}	
	int nRc;
#ifdef _DEBUG
	nRc = StringCchVPrintfW(Buffer, SizeLeft, Format, argList);
#else
	nRc = wvsprintf(Buffer, Format, argList);
#endif
	return nRc;
};
template <size_t size>
int swprintf_add(int Shift, wchar_t (&BufferStart)[size], const wchar_t *Format, ...)
{
	va_list argList;
	va_start(argList, Format);
	size_t SizeLeft = size - Shift;
	if (SizeLeft > size)
	{
		_ASSERTE(SizeLeft <= size);
		return E_POINTER;
	}
	int nRc;
#ifdef _DEBUG
	nRc = StringCchVPrintfW(BufferStart+Shift, SizeLeft, Format, argList);
#else
	nRc = wvsprintf(BufferStart+Shift, Format, argList);
#endif
	return nRc;
};
template <size_t size>
int wcscpy_add(wchar_t* Buffer, wchar_t (&BufferStart)[size], const wchar_t *Str)
{
	size_t SizeLeft = size - (Buffer - BufferStart);
	if (SizeLeft > size)
	{
		_ASSERTE(SizeLeft <= size);
		return E_POINTER;
	}	
	int nRc = StringCchCopyW(Buffer, SizeLeft, Str);
	return nRc;
};
template <size_t size>
int wcscpy_add(int Shift, wchar_t (&BufferStart)[size], const wchar_t *Str)
{
	size_t SizeLeft = size - Shift;
	if (SizeLeft > size)
	{
		_ASSERTE(SizeLeft <= size);
		return E_POINTER;
	}	
	int nRc = StringCchCopyW(BufferStart+Shift, SizeLeft, Str);
	return nRc;
};
template <size_t size>
int wcscat_add(wchar_t* Buffer, wchar_t (&BufferStart)[size], const wchar_t *Str)
{
	size_t SizeLeft = size - (Buffer - BufferStart);
	if (SizeLeft > size)
	{
		_ASSERTE(SizeLeft <= size);
		return E_POINTER;
	}	
	int nRc = StringCchCatW(Buffer, SizeLeft, Str);
	return nRc;
};
template <size_t size>
int wcscat_add(int Shift, wchar_t (&BufferStart)[size], const wchar_t *Str)
{
	size_t SizeLeft = size - Shift;
	if (SizeLeft > size)
	{
		_ASSERTE(SizeLeft <= size);
		return E_POINTER;
	}	
	int nRc = StringCchCatW(BufferStart+Shift, SizeLeft, Str);
	return nRc;
};
template <size_t size>
int wcscpy_c(wchar_t (&Dst)[size], const wchar_t *Src)
{
	int nRc = StringCchCopyW(Dst, size, Src);
	return nRc;
}
template <size_t size>
int wcscat_c(wchar_t (&Dst)[size], const wchar_t *Src)
{
	int nRc = StringCchCatW(Dst, size, Src);
	return nRc;
}
//#undef _wcscpy_c
//inline errno_t _wcscpy_c(wchar_t *Dst, size_t size, const wchar_t *Src)
//{
//	return StringCchCopyW(Dst, size, Src);
//}
//#undef _wcscat_c
//inline errno_t _wcscat_c(wchar_t *Dst, size_t size, const wchar_t *Src)
//{
//	return StringCchCatW(Dst, size, Src);
//}
#define _wcscpy_c(Dst,Size,Src) StringCchCopyW(Dst, Size, Src)
#define _wcscat_c(Dst,Size,Src) StringCchCatW(Dst, Size, Src)


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
	#define DEBUGSTR(s) { MCHKHEAP; CHEKCDBGMODLABEL; SYSTEMTIME st; GetLocalTime(&st); wchar_t szDEBUGSTRTime[40]; swprintf_c(szDEBUGSTRTime, L"%i:%02i:%02i.%03i(%s.%i) ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, gszDbgModLabel, GetCurrentThreadId()); OutputDebugString(szDEBUGSTRTime); OutputDebugString(s); }
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
