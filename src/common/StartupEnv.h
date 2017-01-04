
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

#include "StartupEnvDef.h"
#include "ConEmuCheck.h"

class LoadStartupEnv
{
protected:
	typedef BOOL (WINAPI* GetConsoleHistoryInfo_t)(CE_CONSOLE_HISTORY_INFO* lpConsoleHistoryInfo);

	static CEStartupEnv* AllocEnv(size_t cchTotal)
	{
		return (CEStartupEnv*)calloc(cchTotal,1);
	}

	static LPCWSTR GetReactOsName()
	{
		return L"ReactOS";
	}

	static bool Load(size_t cbExtraSize, CEStartupEnv*& pEnv, LPBYTE& ptrEnd)
	{
		pEnv = NULL;
		ptrEnd = NULL;

		STARTUPINFOW si = {sizeof(si)};
		GetStartupInfoW(&si);

		OSVERSIONINFOEXW os = {sizeof(os)};
		GetOsVersionInformational((OSVERSIONINFOW*)&os);

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

		size_t cchTotal = sizeof(*pEnv) + cbExtraSize;

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

		pEnv = AllocEnv(cchTotal);

		if (pEnv)
		{
			pEnv->cbSize = cchTotal;

			pEnv->OsBits = IsWindows64() ? 64 : 32;
			pEnv->hConWnd = myGetConsoleWindow();

			pEnv->si = si;
			pEnv->os = os;

			pEnv->hStartMon = (si.dwFlags & STARTF_USESTDHANDLES) ? NULL : (HMONITOR)si.hStdOutput;

			// Информационно. К физической консоли потом могут и через RDP подключиться...
			pEnv->bIsRemote = GetSystemMetrics(0x1000/*SM_REMOTESESSION*/);

			pEnv->bIsDbcs = IsDbcs();

			// Don't use them in ConEmuHk
			pEnv->bIsWine = 2;
			pEnv->bIsWinPE = 2;
			pEnv->bIsAdmin = 2;

			LPCWSTR pszReactCompare = GetReactOsName();
			int nCmdLen = lstrlen(pszReactCompare);
			wchar_t* pszReactOS = os.szCSDVersion + lstrlen(os.szCSDVersion) + 1;
			if (*pszReactOS && ((pszReactOS+nCmdLen+1) < (os.szCSDVersion + countof(os.szCSDVersion))))
			{
				pszReactOS[nCmdLen] = 0;
				pEnv->bIsReactOS = (lstrcmpi(pszReactOS, pszReactCompare) == 0);
			}

			pEnv->nAnsiCP = GetACP();
			pEnv->nOEMCP = GetOEMCP();

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

			ptrEnd = (LPBYTE)psz;
		}

		SafeFree(pszEnvPathStore);
		SafeFree(pszExecMod);
		SafeFree(pszWorkDir);

		return (pEnv != NULL && ptrEnd != NULL);
	}

public:
	static CEStartupEnv* Create()
	{
		CEStartupEnv* pEnv = NULL;
		LPBYTE ptrEnd = NULL;
		Load(0, pEnv, ptrEnd);
		return pEnv;
	}
};
