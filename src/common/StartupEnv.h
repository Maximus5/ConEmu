
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

// Some parts of code skips execution when FULL_STARTUP_ENV not defined

typedef BOOL (WINAPI* GetConsoleHistoryInfo_t)(CE_CONSOLE_HISTORY_INFO* lpConsoleHistoryInfo);


#ifdef FULL_STARTUP_ENV
static BOOL CALLBACK LoadStartupEnv_FillMonitors(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	CEStartupEnv* p = (CEStartupEnv*)dwData;
	if (p->nMonitorsCount >= countof(p->Monitors))
		return FALSE;

	MONITORINFOEX mi;
	ZeroStruct(mi);
	mi.cbSize = sizeof(mi);
	if (GetMonitorInfo(hMonitor, &mi))
	{
		size_t i = p->nMonitorsCount++;
		p->Monitors[i].hMon = hMonitor;
		p->Monitors[i].rcMonitor = mi.rcMonitor;
		p->Monitors[i].rcWork = mi.rcWork;
		p->Monitors[i].dwFlags = mi.dwFlags;
		wcscpy_c(p->Monitors[i].szDevice, mi.szDevice);

		HDC hdc = CreateDC(mi.szDevice, mi.szDevice, NULL, NULL);
		if (hdc)
		{
			p->Monitors[i].dpis[0].x = GetDeviceCaps(hdc, LOGPIXELSX);
			p->Monitors[i].dpis[0].y = GetDeviceCaps(hdc, LOGPIXELSY);
			DeleteDC(hdc);
		}
		else
		{
			p->Monitors[i].dpis[0].x = p->Monitors[i].dpis[0].y = -1;
		}

		if (p->nMonitorsCount >= countof(p->Monitors))
			return FALSE;
	}

	return TRUE;
}
#endif


CEStartupEnv* LoadStartupEnv()
{
	CEStartupEnv* pEnv = NULL;

	STARTUPINFOW si = {sizeof(si)};
	GetStartupInfoW(&si);

	OSVERSIONINFOEXW os = {sizeof(os)};
	GetVersionEx((OSVERSIONINFOW*)&os);

	wchar_t* pszEnvPathStore = (wchar_t*)malloc(1024*sizeof(*pszEnvPathStore));
	if (pszEnvPathStore)
	{
		DWORD cbPathMax = 1024;
		DWORD cbPathSize = GetEnvironmentVariable(L"PATH", pszEnvPathStore, cbPathMax);
		if (cbPathSize >= 1024)
		{
			cbPathMax = cbPathSize+1;
			pszEnvPathStore = (wchar_t*)realloc(pszEnvPathStore, cbPathMax*sizeof(*pszEnvPathStore));
			if (pszEnvPathStore)
			{
				cbPathSize = GetEnvironmentVariable(L"PATH", pszEnvPathStore, (cbPathSize+1));
			}
		}

		if (pszEnvPathStore)
		{
			pszEnvPathStore[min(cbPathSize,cbPathMax-1)] = 0;
		}
	}

	LPCWSTR pszCmdLine = GetCommandLine();

	wchar_t* pszExecMod = (wchar_t*)calloc(MAX_PATH*2,sizeof(*pszExecMod));
	if (pszExecMod)
		GetModuleFileName(NULL, pszExecMod, MAX_PATH*2);

	wchar_t* pszWorkDir = (wchar_t*)calloc(MAX_PATH*2,sizeof(*pszWorkDir));
	if (pszWorkDir)
		GetCurrentDirectory(MAX_PATH*2, pszWorkDir);

	// Enum registered Console Fonts
	// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Console\TrueTypeFont
	wchar_t* pszFonts;
#ifndef FULL_STARTUP_ENV
	pszFonts = NULL;
#else
	int nLenMax = 2048;
	pszFonts = (wchar_t*)calloc(nLenMax,sizeof(*pszFonts));
	wchar_t szName[64], szValue[64];
	if (pszFonts)
	{
		HKEY hk;
		DWORD nRights = KEY_READ|WIN3264TEST((IsWindows64() ? KEY_WOW64_64KEY : 0),0);
		if (0 == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont", 0, nRights, &hk))
		{
			bool bIsDBCS = (GetSystemMetrics(SM_DBCSENABLED) != 0);
			DWORD idx = 0, cchName = countof(szName), cchValue = sizeof(szValue)-2, dwType;
			LONG iRc;
			wchar_t* psz = pszFonts;
			while ((iRc = RegEnumValue(hk, idx++, szName, &cchName, NULL, &dwType, (LPBYTE)szValue, &cchValue)) == 0)
			{
				szName[min(countof(szName)-1,cchName)] = 0; szValue[min(countof(szValue)-1,cchValue/2)] = 0;
				int nNameLen = lstrlen(szName);
				int nValLen = lstrlen(szValue);
				int nLen = nNameLen+nValLen+3;

				wchar_t* pszEndPtr = NULL;
				int cp = wcstol(szName, &pszEndPtr, 10);
				// Standard console fonts are stored with named "0", "00", "000", etc.
				// But some hieroglyph fonts can be stored for special codepages ("950", "936", etc)
				if (cp)
				{
					// Is codepage supported by our console host?
					// No way to check that...
					// All function reporting "Valid CP"
					// Checked: GetCPInfo, GetCPInfoEx, IsValidCodePage, WideCharToMultiByte
					if (!bIsDBCS)
						cp = -1; // No need to store this font
				}

				if ((cp != -1) && (nLen < nLenMax))
				{
					if (*pszFonts) *(psz++) = L'\t';
					lstrcpy(psz, szName); psz+= nNameLen;
					*(psz++) = L'\t';
					lstrcpy(psz, szValue); psz+= nValLen;
				}

				cchName = countof(szName); cchValue = sizeof(szValue)-2;
			}
			RegCloseKey(hk);
		}
	}
#endif

	size_t cchTotal = sizeof(*pEnv);

	size_t cchEnv = pszEnvPathStore ? (lstrlen(pszEnvPathStore)+1) : 0;
	cchTotal += cchEnv*sizeof(wchar_t);

	size_t cchExe = pszExecMod ? (lstrlen(pszExecMod)+1) : 0;
	cchTotal += cchExe*sizeof(wchar_t);

	size_t cchCmd = pszCmdLine ? (lstrlen(pszCmdLine)+1) : 0;
	cchTotal += cchCmd*sizeof(wchar_t);

	size_t cchDir = pszWorkDir ? (lstrlen(pszWorkDir)+1) : 0;
	cchTotal += cchDir*sizeof(wchar_t);

	size_t cchTtl = si.lpTitle ? (lstrlen(si.lpTitle)+1) : 0;
	cchTotal += cchTtl*sizeof(wchar_t);

	size_t cchFnt = pszFonts ? (lstrlen(pszFonts)+1) : 0;
	cchTotal += cchFnt*sizeof(wchar_t);

	pEnv = (CEStartupEnv*)calloc(cchTotal,1);
	if (pEnv)
	{
		pEnv->cbSize = cchTotal;

		pEnv->OsBits = IsWindows64() ? 64 : 32;
		pEnv->hConWnd = GetConsoleWindow();

		pEnv->si = si;
		pEnv->os = os;

		pEnv->bIsDbcs = IsDbcs();

		#ifdef FULL_STARTUP_ENV
		// These functions call AdvApi32 functions
		pEnv->bIsWine = IsWine() ? 1 : 0;
		pEnv->bIsWinPE = IsWinPE() ? 1 : 0;
		#else
		// Don't use them in ConEmuHk
		pEnv->bIsWine = 2;
		pEnv->bIsWinPE = 2;
		#endif

		LPCWSTR pszReactCompare = L"ReactOS";
		int nCmdLen = lstrlen(pszReactCompare);
		wchar_t* pszReactOS = os.szCSDVersion + lstrlen(os.szCSDVersion) + 1;
		if (*pszReactOS && ((pszReactOS+nCmdLen+1) < (os.szCSDVersion + countof(os.szCSDVersion))))
		{
			pszReactOS[nCmdLen] = 0;
			pEnv->bIsReactOS = (lstrcmpi(pszReactOS, pszReactCompare) == 0);
		}
		if (!pEnv->bIsReactOS && (os.dwMajorVersion == 5))
		{
			#ifdef FULL_STARTUP_ENV
			//if (os.dwMajorVersion == 5 && os.dwMinorVersion == 2
			//pEnv->bIsReactOS = ;
			HKEY hk;
			if (0 == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hk))
			{
				DWORD nSize = sizeof(szValue);
				if (0 == RegQueryValueEx(hk, L"ProductName", NULL, NULL, (LPBYTE)szValue, &nSize))
				{
					szValue[nCmdLen] = 0;
					pEnv->bIsReactOS = (lstrcmpi(szValue, pszReactCompare) == 0);
				}
				RegCloseKey(hk);
			}
			#endif
		}
		pEnv->nAnsiCP = GetACP();
		pEnv->nOEMCP = GetOEMCP();

		pEnv->nMonitorsCount = 0;
		#ifdef FULL_STARTUP_ENV
		EnumDisplayMonitors(NULL, NULL, LoadStartupEnv_FillMonitors, (LPARAM)pEnv);
		#endif

		HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
		if (hKernel)
		{
			// Функция есть только в Vista+
			GetConsoleHistoryInfo_t _GetConsoleHistoryInfo = (GetConsoleHistoryInfo_t)GetProcAddress(hKernel, "GetConsoleHistoryInfo");
			if (_GetConsoleHistoryInfo)
			{
				pEnv->hi.cbSize = sizeof(pEnv->hi);
				if (!_GetConsoleHistoryInfo(&pEnv->hi))
				{
					pEnv->hi.cbSize = GetLastError() | 0x80000000;
				}
			}
		}

		struct { CE_HANDLE_INFO* p; DWORD nId; }
			lHandles[] = {{&pEnv->hIn, STD_INPUT_HANDLE}, {&pEnv->hOut, STD_OUTPUT_HANDLE}, {&pEnv->hErr, STD_ERROR_HANDLE}};
		for (size_t i = 0; i < countof(lHandles); i++)
		{
			lHandles[i].p->hStd = GetStdHandle(lHandles[i].nId);
			if (!GetConsoleMode(lHandles[i].p->hStd, &lHandles[i].p->nMode))
				lHandles[i].p->nMode = (DWORD)-1;
		}

		wchar_t* psz = (wchar_t*)(pEnv+1);

		if (pszCmdLine)
		{
			_wcscpy_c(psz, cchCmd, pszCmdLine);
			pEnv->pszCmdLine = psz;
			psz += cchCmd;
		}

		if (pszExecMod)
		{
			_wcscpy_c(psz, cchExe, pszExecMod);
			pEnv->pszExecMod = psz;
			psz += cchExe;
		}

		if (pszWorkDir)
		{
			_wcscpy_c(psz, cchDir, pszWorkDir);
			pEnv->pszWorkDir = psz;
			psz += cchDir;
		}

		if (pszEnvPathStore)
		{
			_wcscpy_c(psz, cchEnv, pszEnvPathStore);
			pEnv->pszPathEnv = psz;
			pEnv->cchPathLen = cchEnv;
			psz += cchEnv;
		}

		if (si.lpTitle)
		{
			_wcscpy_c(psz, cchTtl, si.lpTitle);
			pEnv->si.lpTitle = psz;
			psz += cchTtl;
		}

		if (pszFonts)
		{
			_wcscpy_c(psz, cchFnt, pszFonts);
			pEnv->pszRegConFonts = psz;
			psz += cchFnt;
		}
	}

	SafeFree(pszEnvPathStore);
	SafeFree(pszExecMod);
	SafeFree(pszFonts);
	return pEnv;
}
