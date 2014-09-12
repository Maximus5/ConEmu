
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

//#ifdef _DEBUG
//#define USE_LOCK_SECTION
//#endif

#define HIDE_USE_EXCEPTION_INFO
#include <windows.h>
#include "defines.h"
#include "MAssert.h"
#include "MFileLog.h"
#include "MSectionSimple.h"
#include "StartupEnvDef.h"
#include "WinObjects.h"
#include "../ConEmu/version.h"
#pragma warning(disable: 4091)
#include <shlobj.h>
//#pragma warning(default: 4091)
//#include "Monitors.h"
#include <TlHelp32.h>

#ifdef _DEBUG
#define DebugString(x) //OutputDebugString(x)
#define DebugStringA(x) //OutputDebugStringA(x)
#define DEBUGSTRLOG(x) //OutputDebugStringA(x)
#else
#define DebugString(x) //OutputDebugString(x)
#define DebugStringA(x) //OutputDebugStringA(x)
#define DEBUGSTRLOG(x)
#endif


MFileLog::MFileLog(LPCWSTR asName, LPCWSTR asDir /*= NULL*/, DWORD anPID /*= 0*/)
{
	mpcs_Lock = new MSectionSimple(true);
	mh_LogFile = NULL;
	ms_FilePathName = NULL;
	ms_DefPath = (asDir && *asDir) ? lstrdup(asDir) : NULL;
	InitFileName(asName, anPID);
}

HRESULT MFileLog::InitFileName(LPCWSTR asName /*= NULL*/, DWORD anPID /*= 0*/)
{
	if (!anPID) anPID = GetCurrentProcessId();

	if (!asName || !*asName) asName = L"LogFile";

	size_t cchMax = lstrlen(asName)+16;
	ms_FileName = (wchar_t*)malloc(cchMax*sizeof(*ms_FileName));
	if (!ms_FileName)
	{
		_ASSERTEX(ms_FileName);
		return E_UNEXPECTED;
	}

	_wsprintf(ms_FileName, SKIPLEN(cchMax) L"%s-%u.log", asName, anPID);

	return S_OK;

	//wchar_t szTemp[MAX_PATH]; szTemp[0] = 0;
	//GetTempPath(MAX_PATH-16, szTemp);

	//if (!asDir || !*asDir)
	//{
	//	wcscat_c(szTemp, L"ConEmuLog");
	//	CreateDirectoryW(szTemp, NULL);
	//	wcscat_c(szTemp, L"\\");
	//	asDir = szTemp;
	//}

	//int nDirLen = lstrlenW(asDir);
	//wchar_t szFile[MAX_PATH*2];
	//_wsprintf(szFile, SKIPLEN(countof(szFile)) L"%s-%u.log", asName ? asName : L"LogFile", anPID);
	//int nFileLen = lstrlenW(szFile);
	//int nCchMax = nDirLen+nFileLen+3;
	//ms_FilePathName = (wchar_t*)calloc(nCchMax,2);
	//_wcscpy_c(ms_FilePathName, nCchMax, asDir);

	//if (nDirLen > 0 && ms_FilePathName[nDirLen-1] != L'\\')
	//	_wcscat_c(ms_FilePathName, nCchMax, L"\\");

	//_wcscat_c(ms_FilePathName, nCchMax, szFile);
}

MFileLog::~MFileLog()
{
	CloseLogFile();
	SafeFree(ms_DefPath);
	SafeDelete(mpcs_Lock);
}

void MFileLog::CloseLogFile()
{
	SafeCloseHandle(mh_LogFile);

	SafeFree(ms_FilePathName);
	SafeFree(ms_FileName);
}

// Returns 0 if succeeded, otherwise - GetLastError() code
HRESULT MFileLog::CreateLogFile(LPCWSTR asName /*= NULL*/, DWORD anPID /*= 0*/, DWORD anLevel /*= 0*/)
{
	if (!this)
		return -1;

	// ms_FileName мог быть проинициализирован в конструкторе, поэтому CloseLogFile не зовем
	if (mh_LogFile && mh_LogFile != INVALID_HANDLE_VALUE)
	{
		SafeCloseHandle(mh_LogFile);
	}

	if (asName)
	{
		// А вот если указали новое имя - нужно все передернуть
		CloseLogFile();

		HRESULT hr = InitFileName(asName, anPID);
		if (FAILED(hr))
			return -1;
	}

	if (!ms_FileName || !*ms_FileName)
	{
		return -1;
	}

	DWORD dwErr = (DWORD)-1;

	wchar_t szLevel[16] = L"";
	if (anLevel > 0)
		_wsprintf(szLevel, SKIPLEN(countof(szLevel)) L"[%u]", anLevel);
	wchar_t szVer4[8] = L"";
	lstrcpyn(szVer4, WSTRING(MVV_4a), countof(szVer4));
	wchar_t szConEmu[64];
	_wsprintf(szConEmu, SKIPLEN(countof(szConEmu)) L"ConEmu %u%02u%02u%s%s[%s%s] log%s",
		MVV_1, MVV_2, MVV_3, szVer4[0]&&szVer4[1]?L"-":L"", szVer4,
		WIN3264TEST(L"32",L"64"), RELEASEDEBUGTEST(L"",L"D"), szLevel);

	if (!ms_FilePathName)
	{
		// Первое открытие. нужно сформировать путь к лог-файлу
		mh_LogFile = NULL;

		size_t cchMax, cchNamLen = lstrlen(ms_FileName);

		if (ms_DefPath && *ms_DefPath)
		{
			size_t cchDirLen = lstrlen(ms_DefPath);
            cchMax = cchDirLen + cchNamLen + 3;

            ms_FilePathName = (wchar_t*)calloc(cchMax,sizeof(*ms_FilePathName));
            if (!ms_FilePathName)
            	return -1;

        	_wcscpy_c(ms_FilePathName, cchMax, ms_DefPath);
        	if (ms_DefPath[cchMax-1] != L'\\')
        		_wcscat_c(ms_FilePathName, cchMax, L"\\");
    		_wcscat_c(ms_FilePathName, cchMax, ms_FileName);


    		mh_LogFile = CreateFileW(ms_FilePathName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    		// Нет прав на запись в текущую папку?
			if (mh_LogFile == INVALID_HANDLE_VALUE)
			{
				dwErr = GetLastError();
				if (dwErr == ERROR_ACCESS_DENIED/*5*/)
					mh_LogFile = NULL;
				SafeFree(ms_FilePathName);
			}
		}

		if (mh_LogFile == NULL)
		{
			wchar_t szDesktop[MAX_PATH+1] = L"";
			if (S_OK == SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY|CSIDL_FLAG_CREATE, NULL, 0/*SHGFP_TYPE_CURRENT*/, szDesktop))
			{
				size_t cchDirLen = lstrlen(szDesktop);
	            cchMax = cchDirLen + cchNamLen + 32;

	            ms_FilePathName = (wchar_t*)calloc(cchMax,sizeof(*ms_FilePathName));
	            if (!ms_FilePathName)
	            	return -1;

	        	_wcscpy_c(ms_FilePathName, cchMax, szDesktop);
	        	_wcscat_c(ms_FilePathName, cchMax, (szDesktop[cchDirLen-1] != L'\\') ? L"\\ConEmuLogs" : L"ConEmuLogs");
				CreateDirectory(ms_FilePathName, NULL);
				_wcscat_c(ms_FilePathName, cchMax, L"\\");
	    		_wcscat_c(ms_FilePathName, cchMax, ms_FileName);

	    		mh_LogFile = CreateFileW(ms_FilePathName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	    		// Нет прав на запись в текущую папку?
				if (mh_LogFile == INVALID_HANDLE_VALUE)
				{
					dwErr = GetLastError();
					mh_LogFile = NULL;
				}
			}
		}
	}
	else
	{
		if (!ms_FilePathName || !*ms_FilePathName)
			return -1;

		mh_LogFile = CreateFileW(ms_FilePathName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (mh_LogFile == INVALID_HANDLE_VALUE)
		{
			mh_LogFile = NULL;
			dwErr = GetLastError();
		}
	}

	if (!mh_LogFile || (mh_LogFile == INVALID_HANDLE_VALUE))
	{
		mh_LogFile = NULL;
		return (dwErr ? dwErr : -1);
	}

	LogString(szConEmu, true);

	return 0; // OK
}

LPCWSTR MFileLog::GetLogFileName()
{
	if (!this)
		return L"<NULL>";

	return (ms_FilePathName ? ms_FilePathName : L"<NullFileName>");
}

void MFileLog::LogString(LPCSTR asText, bool abWriteTime /*= true*/, LPCSTR asThreadName /*= NULL*/, bool abNewLine /*= true*/, UINT anCP /*= CP_ACP*/)
{
	if (!this)
		return;

	if (mh_LogFile == INVALID_HANDLE_VALUE || mh_LogFile == NULL)
		return;

	wchar_t szInfo[460]; szInfo[0] = 0;
	wchar_t* pszBuf = szInfo;
	wchar_t szThread[32]; szThread[0] = 0;

	if (asText)
	{
		UINT nLen = lstrlenA(asText);
		if (nLen < countof(szInfo))
		{
			pszBuf = szInfo;
		}
		else
		{
			//Too large, need more memory
			pszBuf = (wchar_t*)malloc((nLen+1)*sizeof(*pszBuf));
		}

		if (pszBuf)
		{
			MultiByteToWideChar(anCP, 0, asText, -1, pszBuf, nLen);
			pszBuf[nLen] = 0;
		}
	}

	if (asThreadName)
	{
		MultiByteToWideChar(anCP, 0, asThreadName, -1, szThread, countof(szThread));
		szThread[countof(szThread)-1] = 0;
	}

	LogString(pszBuf, abWriteTime, szThread, abNewLine);

	if (pszBuf && (pszBuf != szInfo))
	{
		SafeFree(pszBuf);
	}
}

void MFileLog::LogString(LPCWSTR asText, bool abWriteTime /*= true*/, LPCWSTR asThreadName /*= NULL*/, bool abNewLine /*= true*/)
{
	if (!this) return;

	if (!asText) return;

	//DEBUGTEST(abWriteTime = false);

	size_t cchTextLen = asText ? lstrlen(asText) : 0;
	size_t cchThrdLen = asThreadName ? lstrlen(asThreadName) : 0;
	size_t cchMax = (cchTextLen + cchThrdLen)*3 + 32;
	char szBuffer[1024];
	char *pszBuffer, *pszTemp = NULL;
	if (cchMax < countof(szBuffer))
	{
		pszBuffer = szBuffer;
	}
	else
	{
		pszTemp = pszBuffer = (char*)malloc(cchMax);
	}
	if (!pszBuffer)
		return;

	size_t cchCur = 0;

	if (abWriteTime)
	{
		SYSTEMTIME st; GetLocalTime(&st);
		char szTime[32];
		_wsprintfA(szTime, SKIPLEN(countof(szTime)) "%i:%02i:%02i.%03i ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		INT_PTR dwLen = lstrlenA(szTime);
		memmove(pszBuffer+cchCur, szTime, dwLen);
		cchCur += dwLen;
	}

	if (asThreadName && *asThreadName)
	{
		int nLen = WideCharToMultiByte(CP_UTF8, 0, asThreadName, -1, pszBuffer+cchCur, (int)cchThrdLen*3+1, NULL, NULL);
		if (nLen > 1) // including terminating null char
			cchCur += (nLen-1);
		pszBuffer[cchCur++] = L' ';
	}

	if (asText && *asText)
	{
		int nLen = WideCharToMultiByte(CP_UTF8, 0, asText, -1, pszBuffer+cchCur, (int)cchTextLen*3+1, NULL, NULL);
		if (nLen > 1) // including terminating null char
			cchCur += (nLen-1);
	}

	if (abNewLine)
	{
		memmove(pszBuffer+cchCur, "\r\n", 2);
		cchCur += 2;
	}

	pszBuffer[cchCur] = 0;

	if (mh_LogFile)
	{
		MSectionLockSimple lock; lock.Lock(mpcs_Lock);
		DWORD dwLen = (DWORD)cchCur;
		WriteFile(mh_LogFile, pszBuffer, dwLen, &dwLen, 0);
		FlushFileBuffers(mh_LogFile);
	}
	else
	{
		#ifdef _DEBUG
		DEBUGSTRLOG(asText);
		#endif
	}

	SafeFree(pszTemp);

#if 0
	if (!this)
		return;

	if (mh_LogFile == INVALID_HANDLE_VALUE || mh_LogFile == NULL)
		return;

	wchar_t szInfo[512]; szInfo[0] = 0;
	SYSTEMTIME st; GetLocalTime(&st);
	_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"%i:%02i:%02i.%03i ",
	          st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	int nCur = lstrlenW(szInfo);

	if (asThreadName && *asThreadName)
	{
		lstrcpynW(szInfo+nCur, asThreadName, 32); //-V112
		nCur += lstrlenW(szInfo+nCur);
	}

	if (asText && *asText)
	{
		lstrcpynW(szInfo+nCur, asText, 508-nCur);
		nCur += lstrlenW(szInfo+nCur);
	}

	wcscpy_add(nCur, szInfo, L"\r\n");
	nCur += 2;
	DWORD dwLen = 0;
	WriteFile(mh_LogFile, szInfo, nCur*2, &dwLen, 0);
	FlushFileBuffers(mh_LogFile);
#endif
}

void MFileLog::LogStartEnv(CEStartupEnv* apStartEnv)
{
	if (!apStartEnv || (apStartEnv->cbSize < sizeof(*apStartEnv)))
	{
		LogString(L"LogStartEnv failed, invalid apStartEnv");
		return;
	}

	// Пишем инфу
	wchar_t szSI[MAX_PATH*4], szDesktop[128] = L"", szTitle[128] = L"";
	lstrcpyn(szDesktop, apStartEnv->si.lpDesktop ? apStartEnv->si.lpDesktop : L"", countof(szDesktop));
	lstrcpyn(szTitle, apStartEnv->si.lpTitle ? apStartEnv->si.lpTitle : L"", countof(szTitle));

	BOOL bWin64 = IsWindows64();

	#pragma warning(disable: 4996)
	OSVERSIONINFOEXW osv = {sizeof(osv)};
	GetVersionEx((OSVERSIONINFOW*)&osv);
	#pragma warning(default: 4996)

	LPCWSTR pszReactOS = osv.szCSDVersion + lstrlen(osv.szCSDVersion) + 1;
	if (!*pszReactOS)
		pszReactOS++;

	HWND hConWnd = GetConsoleWindow();

	_wsprintf(szSI, SKIPLEN(countof(szSI)) L"Startup info\r\n"
		L"\tOsVer: %u.%u.%u.x%u, Product: %u, SP: %u.%u, Suite: 0x%X, SM_SERVERR2: %u\r\n"
		L"\tCSDVersion: %s, ReactOS: %u (%s), Rsrv: %u\r\n"
		L"\tDBCS: %u, WINE: %u, PE: %u, Remote: %u, ACP: %u, OEMCP: %u\r\n"
		L"\tDesktop: %s; BPP: %u\r\n\tTitle: %s\r\n\tSize: {%u,%u},{%u,%u}\r\n"
		L"\tFlags: 0x%08X, ShowWindow: %u, ConHWnd: 0x%08X\r\n"
		L"\tHandles: 0x%08X, 0x%08X, 0x%08X"
		,
		osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber, bWin64 ? 64 : 32,
		osv.wProductType, osv.wServicePackMajor, osv.wServicePackMinor, osv.wSuiteMask, GetSystemMetrics(89/*SM_SERVERR2*/),
		osv.szCSDVersion, apStartEnv->bIsReactOS, pszReactOS, osv.wReserved,
		apStartEnv->bIsDbcs, apStartEnv->bIsWine, apStartEnv->bIsWinPE, apStartEnv->bIsRemote,
		apStartEnv->nAnsiCP, apStartEnv->nOEMCP,
		szDesktop, apStartEnv->nPixels, szTitle,
		apStartEnv->si.dwX, apStartEnv->si.dwY, apStartEnv->si.dwXSize, apStartEnv->si.dwYSize,
		apStartEnv->si.dwFlags, (DWORD)apStartEnv->si.wShowWindow, (DWORD)hConWnd,
		(DWORD)apStartEnv->si.hStdInput, (DWORD)apStartEnv->si.hStdOutput, (DWORD)apStartEnv->si.hStdError
		);
	LogString(szSI, true);

	if (hConWnd)
	{
		szSI[0] = 0;
		HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
		typedef BOOL (__stdcall *FGetConsoleKeyboardLayoutName)(wchar_t*);
		FGetConsoleKeyboardLayoutName pfnGetConsoleKeyboardLayoutName = hKernel ? (FGetConsoleKeyboardLayoutName)GetProcAddress(hKernel, "GetConsoleKeyboardLayoutNameW") : NULL;
		if (pfnGetConsoleKeyboardLayoutName)
		{
			ZeroStruct(szTitle);
			if (pfnGetConsoleKeyboardLayoutName(szTitle))
				_wsprintf(szSI, SKIPLEN(countof(szSI)) L"Active console layout name: '%s'", szTitle);
		}
		if (!*szSI)
			_wsprintf(szSI, SKIPLEN(countof(szSI)) L"Active console layout: Unknown, code=%u", GetLastError());
		LogString(szSI, false);
	}

	// Текущий HKL (он может отличаться от GetConsoleKeyboardLayoutNameW
	HKL hkl[32] = {NULL};
	hkl[0] = GetKeyboardLayout(0);
	_wsprintf(szSI, SKIPLEN(countof(szSI)) L"Active HKL: " WIN3264TEST(L"0x%08X",L"0x%08X%08X"), WIN3264WSPRINT((DWORD_PTR)hkl[0]));
	LogString(szSI, false);
	// Установленные в системе HKL
	UINT nHkl = GetKeyboardLayoutList(countof(hkl), hkl);
	if (!nHkl || (nHkl > countof(hkl)))
	{
		_wsprintf(szSI, SKIPLEN(countof(szSI)) L"GetKeyboardLayoutList failed, code=%u", GetLastError());
		LogString(szSI, false);
	}
	else
	{
		wcscpy_c(szSI, L"GetKeyboardLayoutList:");
		size_t iLen = lstrlen(szSI);
		_ASSERTE((iLen + 1 + nHkl*17)<countof(szSI));

		for (UINT i = 0; i < nHkl; i++)
		{
			_wsprintf(szSI+iLen, SKIPLEN(18) WIN3264TEST(L" 0x%08X",L" 0x%08X%08X"), WIN3264WSPRINT((DWORD_PTR)hkl[i]));
			iLen += lstrlen(szSI+iLen);
		}
		LogString(szSI, false);
	}

	LogString("CmdLine: ", false, NULL, false);
	LogString(apStartEnv->pszCmdLine ? apStartEnv->pszCmdLine : L"<NULL>", false, NULL, true);
	LogString("ExecMod: ", false, NULL, false);
	LogString(apStartEnv->pszExecMod ? apStartEnv->pszExecMod : L"<NULL>", false, NULL, true);
	LogString("WorkDir: ", false, NULL, false);
	LogString(apStartEnv->pszWorkDir ? apStartEnv->pszWorkDir : L"<NULL>", false, NULL, true);
	LogString("PathEnv: ", false, NULL, false);
	LogString(apStartEnv->pszPathEnv ? apStartEnv->pszPathEnv : L"<NULL>", false, NULL, true);
	LogString("ConFont: ", false, NULL, false);
	LogString(apStartEnv->pszRegConFonts ? apStartEnv->pszRegConFonts : L"<NULL>", false, NULL, true);

	// szSI уже не используется, можно

	HWND hFore = GetForegroundWindow();
	RECT rcFore = {0}; if (hFore) GetWindowRect(hFore, &rcFore);
	if (hFore) GetClassName(hFore, szDesktop, countof(szDesktop)-1); else szDesktop[0] = 0;
	if (hFore) GetWindowText(hFore, szTitle, countof(szTitle)-1); else szTitle[0] = 0;
	_wsprintf(szSI, SKIPLEN(countof(szSI)) L"Foreground: x%08X {%i,%i}-{%i,%i} '%s' - %s", (DWORD)(DWORD_PTR)hFore, rcFore.left, rcFore.top, rcFore.right, rcFore.bottom, szDesktop, szTitle);
	LogString(szSI, false, NULL, true);

	POINT ptCur = {0}; GetCursorPos(&ptCur);
	_wsprintf(szSI, SKIPLEN(countof(szSI)) L"Cursor: {%i,%i}", ptCur.x, ptCur.y);
	LogString(szSI, false, NULL, true);

	HDC hdcScreen = GetDC(NULL);
	int nBits = GetDeviceCaps(hdcScreen,BITSPIXEL);
	int nPlanes = GetDeviceCaps(hdcScreen,PLANES);
	int nAlignment = GetDeviceCaps(hdcScreen,BLTALIGNMENT);
	int nVRefr = GetDeviceCaps(hdcScreen,VREFRESH);
	int nShadeCaps = GetDeviceCaps(hdcScreen,SHADEBLENDCAPS);
	int nDevCaps = GetDeviceCaps(hdcScreen,RASTERCAPS);
	int nDpiX = GetDeviceCaps(hdcScreen, LOGPIXELSX);
	int nDpiY = GetDeviceCaps(hdcScreen, LOGPIXELSY);
	_wsprintf(szSI, SKIPLEN(countof(szSI))
		L"Display: bpp=%i, planes=%i, align=%i, vrefr=%i, shade=x%08X, rast=x%08X, dpi=%ix%i, per-mon-dpi=%u",
		nBits, nPlanes, nAlignment, nVRefr, nShadeCaps, nDevCaps, nDpiX, nDpiY, apStartEnv->bIsPerMonitorDpi);
	ReleaseDC(NULL, hdcScreen);
	LogString(szSI, false, NULL, true);

	LogString("Monitors:", false, NULL, true);
	for (size_t i = 0; i < apStartEnv->nMonitorsCount; i++)
	{
		CEStartupEnv::MyMonitorInfo* p = (apStartEnv->Monitors + i);
		szDesktop[0] = 0;
		for (size_t j = 0; j < countof(p->dpis); j++)
		{
			if (p->dpis[j].x || p->dpis[j].y)
			{
				wchar_t szDpi[32];
				_wsprintf(szDpi, SKIPLEN(countof(szDpi))
					szDesktop[0] ? L";{%i,%i}" : L"{%i,%i}",
					p->dpis[j].x, p->dpis[j].y);
				wcscat_c(szDesktop, szDpi);
			}
		}
		_wsprintf(szSI, SKIPLEN(countof(szSI))
			L"  %08X: {%i,%i}-{%i,%i} (%ix%i), Working: {%i,%i}-{%i,%i} (%ix%i), dpi: %s `%s`%s",
			(DWORD)(DWORD_PTR)p->hMon,
			p->rcMonitor.left, p->rcMonitor.top, p->rcMonitor.right, p->rcMonitor.bottom, p->rcMonitor.right-p->rcMonitor.left, p->rcMonitor.bottom-p->rcMonitor.top,
			p->rcWork.left, p->rcWork.top, p->rcWork.right, p->rcWork.bottom, p->rcWork.right-p->rcWork.left, p->rcWork.bottom-p->rcWork.top,
			szDesktop, p->szDevice,
			(p->dwFlags & MONITORINFOF_PRIMARY) ? L" <<== Primary" : L"");
		LogString(szSI, false, NULL, true);
	}

	LogString("Modules:", false, NULL, true);
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	MODULEENTRY32W mi = {sizeof(mi)};
	if (h && (h != INVALID_HANDLE_VALUE))
	{
		if (Module32First(h, &mi))
		{
			do
			{
				DWORD_PTR ptrStart = (DWORD_PTR)mi.modBaseAddr;
				DWORD_PTR ptrEnd = (DWORD_PTR)mi.modBaseAddr + (DWORD_PTR)(mi.modBaseSize ? (mi.modBaseSize-1) : 0);
				_wsprintf(szSI, SKIPLEN(countof(szSI))
					L"  " WIN3264TEST(L"%08X-%08X",L"%08X%08X-%08X%08X") L" %8X %s",
					WIN3264WSPRINT(ptrStart), WIN3264WSPRINT(ptrEnd), mi.modBaseSize, mi.szExePath);
				LogString(szSI, false, NULL, true);
			} while (Module32Next(h, &mi));
		}
		CloseHandle(h);
	}
}
