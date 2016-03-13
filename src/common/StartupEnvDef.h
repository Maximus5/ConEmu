
/*
Copyright (c) 2009-2014 Maximus5
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

struct CE_CONSOLE_HISTORY_INFO
{
	UINT  cbSize;
	UINT  HistoryBufferSize;
	UINT  NumberOfHistoryBuffers;
	DWORD dwFlags;
};

struct CE_HANDLE_INFO
{
	HANDLE hStd;
	DWORD  nMode;
};

struct CEStartupEnv
{
	size_t cbSize;
	STARTUPINFOW si;
	HMONITOR hStartMon;
	DWORD OsBits;
	DWORD nConVisible;
	HWND  hConWnd;
	OSVERSIONINFOEXW os;
	CE_CONSOLE_HISTORY_INFO hi;
	CE_HANDLE_INFO hIn, hOut, hErr;
	LPCWSTR pszCmdLine;
	LPCWSTR pszExecMod;
	LPCWSTR pszWorkDir;
	LPCWSTR pszPathEnv;
	size_t  cchPathLen;
	BOOL    bIsRemote; // SM_REMOTESESSION - Informational
	int     nBPP;      // BITSPIXEL - Informational
	UINT    bIsWine;   // Informational (FALSE/TRUE/ or 2 in ConEmuHk)
	UINT    bIsWinPE;  // Informational (FALSE/TRUE/ or 2 in ConEmuHk)
	UINT    bIsAdmin;  // Informational (FALSE/TRUE/ or 2 in ConEmuHk)
	BOOL    bIsReactOS;
	BOOL    bIsDbcs;
	UINT    nAnsiCP, nOEMCP;
	BOOL    bIsPerMonitorDpi;
	UINT    nMonitorsCount;
	struct MyMonitorInfo {
		HMONITOR hMon;
		RECT  rcMonitor;
		RECT  rcWork;
		DWORD dwFlags;
		// 0=GetDeviceCaps(hdc,*); 1=MDT_Effective_DPI; 2=MDT_Angular_DPI; 3=MDT_Raw_DPI;
		struct { int x, y; } dpis[4];
		TCHAR szDevice[CCHDEVICENAME];
	} Monitors[16];
	// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Console\TrueTypeFont
	LPCWSTR pszRegConFonts; // "Index/CP ~t Name ~t Index/CP ..."
	// [HKCU|HKLM]\Software\Microsoft\Command Processor
	LPCWSTR pszAutoruns; // "  <cmdline>\r\n..."
};

extern CEStartupEnv* gpStartEnv;
