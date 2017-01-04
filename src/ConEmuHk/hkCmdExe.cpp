
/*
Copyright (c) 2015-2017 Maximus5
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

#define HIDE_USE_EXCEPTION_INFO

#ifdef _DEBUG
	#define DebugString(x) //OutputDebugString(x)
#else
	#define DebugString(x) //OutputDebugString(x)
#endif

#include "../common/Common.h"

#include "hkCmdExe.h"
#include "hlpProcess.h"

/* **************** */

static wchar_t* gszClinkCmdLine = NULL;

/* **************** */

static bool InitializeClink()
{
	if (gnCmdInitialized)
		return true;
	gnCmdInitialized = 1; // Single

	//if (!gnAllowClinkUsage)
	//	return false;

	CESERVER_CONSOLE_MAPPING_HDR* pConMap = GetConMap();
	if (!pConMap /*|| !(pConMap->Flags & CECF_UseClink_Any)*/)
	{
		//gnAllowClinkUsage = 0;
		gnCmdInitialized = -1;
		return false;
	}

	// Запомнить режим
	//gnAllowClinkUsage =
	//	(pConMap->Flags & CECF_UseClink_2) ? 2 :
	//	(pConMap->Flags & CECF_UseClink_1) ? 1 :
	//	CECF_Empty;
	gbAllowClinkUsage = ((pConMap->Flags & CECF_UseClink_Any) != 0);
	gbAllowUncPaths = (pConMap->ComSpec.isAllowUncPaths != FALSE);

	if (gbAllowClinkUsage)
	{
		wchar_t szClinkBat[MAX_PATH+32];
		wcscpy_c(szClinkBat, pConMap->ComSpec.ConEmuBaseDir);
		wcscat_c(szClinkBat, L"\\clink\\clink.bat");
		if (!FileExists(szClinkBat))
		{
			gbAllowClinkUsage = false;
		}
		else
		{
			int iLen = lstrlen(szClinkBat) + 16;
			gszClinkCmdLine = (wchar_t*)malloc(iLen*sizeof(*gszClinkCmdLine));
			if (gszClinkCmdLine)
			{
				*gszClinkCmdLine = L'"';
				_wcscpy_c(gszClinkCmdLine+1, iLen-1, szClinkBat);
				_wcscat_c(gszClinkCmdLine, iLen, L"\" inject");
			}
		}
	}

	return true;

	//BOOL bRunRc = FALSE;
	//DWORD nErrCode = 0;

	//if (gnAllowClinkUsage == 2)
	//{
	//	// New style. TODO
	//	wchar_t szClinkDir[MAX_PATH+32], szClinkArgs[MAX_PATH+64];

	//	wcscpy_c(szClinkDir, pConMap->ComSpec.ConEmuBaseDir);
	//	wcscat_c(szClinkDir, L"\\clink");

	//	wcscpy_c(szClinkArgs, L"\"");
	//	wcscat_c(szClinkArgs, szClinkDir);
	//	wcscat_c(szClinkArgs, WIN3264TEST(L"\\clink_x86.exe",L"\\clink_x64.exe"));
	//	wcscat_c(szClinkArgs, L"\" inject");

	//	STARTUPINFO si = {sizeof(si)};
	//	PROCESS_INFORMATION pi = {};
	//	bRunRc = CreateProcess(NULL, szClinkArgs, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, szClinkDir, &si, &pi);
	//
	//	if (bRunRc)
	//	{
	//		WaitForSingleObject(pi.hProcess, INFINITE);
	//		CloseHandle(pi.hProcess);
	//		CloseHandle(pi.hThread);
	//	}
	//	else
	//	{
	//		nErrCode = GetLastError();
	//		_ASSERTEX(FALSE && "Clink loader failed");
	//		UNREFERENCED_PARAMETER(nErrCode);
	//		UNREFERENCED_PARAMETER(bRunRc);
	//	}
	//}
	//else if (gnAllowClinkUsage == 1)
	//{
	//	if (!ghClinkDll)
	//	{
	//		wchar_t szClinkModule[MAX_PATH+30];
	//		_wsprintf(szClinkModule, SKIPLEN(countof(szClinkModule)) L"%s\\clink\\%s",
	//			pConMap->ComSpec.ConEmuBaseDir, WIN3264TEST(L"clink_dll_x86.dll",L"clink_dll_x64.dll"));
	//
	//		ghClinkDll = LoadLibrary(szClinkModule);
	//		if (!ghClinkDll)
	//			return false;
	//	}

	//	if (!gpfnClinkReadLine)
	//	{
	//		gpfnClinkReadLine = (call_readline_t)GetProcAddress(ghClinkDll, "call_readline");
	//		_ASSERTEX(gpfnClinkReadLine!=NULL);
	//	}
	//}

	//return (gpfnClinkReadLine != NULL);
}

static bool IsInteractive()
{
	const wchar_t* const cmdLine = ::GetCommandLineW();
	if (!cmdLine)
	{
		return true;	// can't know - assume it is
	}

	const wchar_t* pos = cmdLine;
	while ((pos = wcschr(pos, L'/')) != NULL)
	{
		switch (pos[1])
		{
		case L'K': case L'k':
			return true;	// /k - execute and remain working
		case L'C': case L'c':
			return false;	// /c - execute and exit
		}
		++pos;
	}

	return true;
}

bool IsClinkLoaded()
{
	// Check processes supported
	if (!gbIsCmdProcess && !gbIsPowerShellProcess)
		return false;
	// Check, if clink library is loaded
	HMODULE hClink = GetModuleHandle(WIN3264TEST(L"clink_dll_x86.dll", L"clink_dll_x64.dll"));
	return (hClink != NULL);
}


/* **************** */

// cmd.exe only!
LONG WINAPI OnRegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
	//typedef LONG (WINAPI* OnRegQueryValueExW_t)(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
	ORIGINAL_EX(RegQueryValueExW);
	LONG lRc = -1;

	if (gbIsCmdProcess && hKey && lpValueName)
	{
		// Allow `CD` to network paths
		if (lstrcmpi(lpValueName, L"DisableUNCCheck") == 0)
		{
			if (lpData)
			{
				if (lpcbData && *lpcbData >= sizeof(DWORD))
					*((LPDWORD)lpData) = gbAllowUncPaths;
				else
					*lpData = gbAllowUncPaths;
			}
			if (lpType)
				*lpType = REG_DWORD;
			if (lpcbData)
				*lpcbData = sizeof(DWORD);
			lRc = 0;
			goto wrap;
		}

		if (gbIsCmdProcess && hKey && lpValueName
			&& (lstrcmpi(lpValueName, L"AutoRun") == 0)
			&& InitializeClink())
		{
			if (gbAllowClinkUsage && gszClinkCmdLine
				&& IsInteractive())
			{
				// Is already loaded?
				if (!IsClinkLoaded()
					&& !gbClinkInjectRequested)
				{
					// Do this once, to avoid multiple initializations
					gbClinkInjectRequested = true;
					// May be it is set up itself?
					typedef LONG (WINAPI* RegOpenKeyEx_t)(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
					typedef LONG (WINAPI* RegCloseKey_t)(HKEY hKey);
					HMODULE hAdvApi = LoadLibrary(L"AdvApi32.dll");
					if (hAdvApi)
					{
						RegOpenKeyEx_t _RegOpenKeyEx = (RegOpenKeyEx_t)GetProcAddress(hAdvApi, "RegOpenKeyExW");
						RegCloseKey_t  _RegCloseKey  = (RegCloseKey_t)GetProcAddress(hAdvApi, "RegCloseKey");
						if (_RegOpenKeyEx && _RegCloseKey)
						{
							const DWORD cchMax = 0x3FF0;
							const DWORD cbMax = cchMax*2;
							wchar_t* pszCmd = (wchar_t*)malloc(cbMax);
							if (pszCmd)
							{
								DWORD cbSize;
								bool bClinkInstalled = false;
								for (int i = 0; i <= 1 && !bClinkInstalled; i++)
								{
									HKEY hk;
									if (_RegOpenKeyEx(i?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER, L"Software\\Microsoft\\Command Processor", 0, KEY_READ, &hk))
										continue;
									if (!F(RegQueryValueExW)(hk, lpValueName, NULL, NULL, (LPBYTE)pszCmd, &(cbSize = cbMax))
										&& (cbSize+2) < cbMax)
									{
										cbSize /= 2; pszCmd[cbSize] = 0;
										CharLowerBuffW(pszCmd, cbSize);
										if (wcsstr(pszCmd, L"\\clink.bat"))
											bClinkInstalled = true;
									}
									_RegCloseKey(hk);
								}
								// Not installed via "Autorun"
								if (!bClinkInstalled)
								{
									int iLen = lstrlen(gszClinkCmdLine);
									_wcscpy_c(pszCmd, cchMax, gszClinkCmdLine);
									_wcscpy_c(pszCmd+iLen, cchMax-iLen, L" & "); // conveyer next command indifferent to %errorlevel%

									cbSize = cbMax - (iLen + 3)*sizeof(*pszCmd);
									if (F(RegQueryValueExW)(hKey, lpValueName, NULL, NULL, (LPBYTE)(pszCmd + iLen + 3), &cbSize)
										|| (pszCmd[iLen+3] == 0))
									{
										pszCmd[iLen] = 0; // There is no self value in registry
									}
									cbSize = (lstrlen(pszCmd)+1)*sizeof(*pszCmd);

									// Return
									lRc = 0;
									if (lpData && lpcbData)
									{
										if (*lpcbData < cbSize)
											lRc = ERROR_MORE_DATA;
										else
											_wcscpy_c((wchar_t*)lpData, (*lpcbData)/2, pszCmd);
									}
									if (lpcbData)
										*lpcbData = cbSize;
									free(pszCmd);
									FreeLibrary(hAdvApi);
									goto wrap;
								}
								free(pszCmd);
							}
						}
						FreeLibrary(hAdvApi);
					}
				}
			}
		}
	}

	if (F(RegQueryValueExW))
		lRc = F(RegQueryValueExW)(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);

wrap:
	return lRc;
}

/* **************** */
