
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

//#ifdef _DEBUG
//#define USE_LOCK_SECTION
//#endif

#define HIDE_USE_EXCEPTION_INFO
#include "Common.h"
#include <Sddl.h> // ConvertSidToStringSid
#include "CmdLine.h"
#include "MModule.h"
#include "MProcess.h"
#include "MStrDup.h"
#include "WObjects.h"
#include "WUser.h"

bool apiSetForegroundWindow(HWND ahWnd)
{
#ifdef _DEBUG
	wchar_t szLastFore[1024]; getWindowInfo(GetForegroundWindow(), szLastFore);
	wchar_t szWnd[1024]; getWindowInfo(ahWnd, szWnd);
#endif
	bool lbRc = ::SetForegroundWindow(ahWnd) ? true : false;
	return lbRc;
}

// If the window was previously visible, the return value is nonzero.
// If the window was previously hidden, the return value is zero.
bool apiShowWindow(HWND ahWnd, int anCmdShow)
{
	#ifdef _DEBUG
	wchar_t szLastFore[1024]; getWindowInfo(GetForegroundWindow(), szLastFore);
	wchar_t szWnd[1024]; getWindowInfo(ahWnd, szWnd);
	#endif

	bool lbRc = ::ShowWindow(ahWnd, anCmdShow) ? true : false;

	return lbRc;
}

bool apiShowWindowAsync(HWND ahWnd, int anCmdShow)
{
	#ifdef _DEBUG
	wchar_t szLastFore[1024]; getWindowInfo(GetForegroundWindow(), szLastFore);
	wchar_t szWnd[1024]; getWindowInfo(ahWnd, szWnd);
	#endif

	bool lbRc = ::ShowWindowAsync(ahWnd, anCmdShow) ? true : false;

	return lbRc;
}

void getWindowInfo(HWND ahWnd, wchar_t (&rsInfo)[1024], bool bProcessName /*= false*/, LPDWORD pnPID /*= NULL*/)
{
	DWORD nPID = 0;

	if (!ahWnd)
	{
		wcscpy_c(rsInfo, L"<NULL>");
	}
	else if (!IsWindow(ahWnd))
	{
		msprintf(rsInfo, countof(rsInfo), L"0x%08X: Invalid window handle", LODWORD(ahWnd));
	}
	else
	{
		wchar_t szClass[256], szTitle[512];
		wchar_t szProc[120] = L"";

		if (!GetClassName(ahWnd, szClass, 256)) wcscpy_c(szClass, L"<GetClassName failed>");
		if (!GetWindowText(ahWnd, szTitle, 512)) szTitle[0] = 0;

		if (bProcessName || pnPID)
		{
			if (GetWindowThreadProcessId(ahWnd, &nPID))
			{
				PROCESSENTRY32 pi = {};
				if (bProcessName && GetProcessInfo(nPID, &pi))
				{
					pi.szExeFile[100] = 0;
					msprintf(szProc, countof(szProc), L" - %s [%u]", pi.szExeFile, nPID);
				}
			}
		}

		msprintf(rsInfo, countof(rsInfo), L"0x%08X: %s - '%s'%s", LODWORD(ahWnd), szClass, szTitle, szProc);
	}

	if (pnPID)
		*pnPID = nPID;
}

bool IsUserAdmin()
{
	// No need to show any "Shield" on XP or 2k
	_ASSERTE(_WIN32_WINNT_VISTA==0x600);
	OSVERSIONINFOEXW osvi = {sizeof(osvi), HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA)};
	DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);
	if (!VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask))
		return false;

	BOOL b;
	SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
	PSID AdministratorsGroup;
	b = AllocateAndInitializeSid(
			&NtAuthority,
			2,
			SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_ADMINS,
			0, 0, 0, 0, 0, 0,
			&AdministratorsGroup);

	if (b)
	{
		if (!CheckTokenMembership(NULL, AdministratorsGroup, &b))
		{
			b = FALSE;
		}

		FreeSid(AdministratorsGroup);
	}

	return (b ? true : false);
}

// *ppszSID - must be LocalFree'd
bool GetLogonSID (HANDLE hToken, wchar_t **ppszSID)
{
	bool bSuccess = false;
	//DWORD dwIndex;
	DWORD dwLength = 0;
	TOKEN_USER user;
	PTOKEN_USER ptu = &user;
	BOOL bFreeToken = FALSE;

	// Verify the parameter passed in is not NULL.
	if (NULL == ppszSID)
		goto Cleanup;
	*ppszSID = NULL;

	if (!hToken)
		bFreeToken = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);

	// Get required buffer size and allocate the TOKEN_GROUPS buffer.
	if (!GetTokenInformation(
		hToken,         // handle to the access token
		TokenUser,      // get information about the token's user account
		(LPVOID) ptu,   // pointer to TOKEN_USER buffer
		0,              // size of buffer
		&dwLength       // receives required buffer size
	))
	{
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			goto Cleanup;

		ptu = (PTOKEN_USER)calloc(dwLength,1);

		if (ptu == NULL)
			goto Cleanup;
	}

	// Get the token group information from the access token.

	if (!GetTokenInformation(
		hToken,         // handle to the access token
		TokenUser,      // get information about the token's user account
		(LPVOID) ptu,   // pointer to TOKEN_USER buffer
		dwLength,       // size of buffer
		&dwLength       // receives required buffer size
	))
	{
		goto Cleanup;
	}

	if (!ConvertSidToStringSid(ptu->User.Sid, ppszSID) || (*ppszSID == NULL))
		goto Cleanup;

	bSuccess = true;

Cleanup:

	// Free the buffer for the token groups.
	if ((ptu != NULL) && (ptu != &user))
		free(ptu);
	if (bFreeToken && hToken)
		CloseHandle(hToken);

	return bSuccess;
}

HANDLE DuplicateProcessHandle(DWORD anTargetPID)
{
	HANDLE hDup = NULL;

	if (anTargetPID == 0)
	{
		_ASSERTEX(anTargetPID != 0);
	}
	else
	{
		HANDLE hTarget = OpenProcess(PROCESS_DUP_HANDLE, FALSE, anTargetPID);
		if (hTarget)
		{
			DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), hTarget, &hDup, MY_PROCESS_ALL_ACCESS, FALSE, 0);

			CloseHandle(hTarget);
		}
	}

	return hDup;
}

// используется в GUI при загрузке настроек
void FindComspec(ConEmuComspec* pOpt, bool bCmdAlso /*= true*/)
{
	if (!pOpt)
		return;

	pOpt->Comspec32[0] = 0;
	pOpt->Comspec64[0] = 0;

	// Ищем tcc.exe
	if (pOpt->csType == cst_AutoTccCmd)
	{
		HKEY hk;
		BOOL bWin64 = IsWindows64();
		wchar_t szPath[MAX_PATH+1];

		// If tcc.exe can be found near to ConEmu location
		LPCWSTR ppszPredefined[] = {
			L"%ConEmuBaseDir%\\tcc.exe",
			L"%ConEmuDir%\\tcc.exe",
			// Sort of PortableApps locations
			L"%ConEmuDir%\\..\\tcc\\tcc.exe",
			L"%ConEmuDir%\\..\\..\\tcc\\tcc.exe",
			// End of predefined list
			NULL};
		for (INT_PTR i = 0; ppszPredefined[i]; i++)
		{
			DWORD nExpand = ExpandEnvironmentStrings(ppszPredefined[i], szPath, countof(szPath));
			if (nExpand && (nExpand < countof(szPath)))
			{
				if (FileExists(szPath))
				{
					wcscpy_c(pOpt->Comspec32, szPath);
					wcscpy_c(pOpt->Comspec64, szPath);
					break;
				}
			}
		}

		// On this step - check "Take Command"!
		if (!*pOpt->Comspec32 || !*pOpt->Comspec64)
		{
			// [HKEY_LOCAL_MACHINE\SOFTWARE\JP Software\Take Command 13.0]
			// @="\"C:\\Program Files\\JPSoft\\TCMD13\\tcmd.exe\""
			for (int b = 0; b <= 1; b++)
			{
				// b==0 - 32bit, b==1 - 64bit
				if (b && !bWin64)
					continue;
				bool bFound = false;
				DWORD nOpt = (b == 0) ? (bWin64 ? KEY_WOW64_32KEY : 0) : (bWin64 ? KEY_WOW64_64KEY : 0);
				if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\JP Software", 0, KEY_READ|nOpt, &hk))
				{
					wchar_t szName[MAX_PATH+1]; DWORD nLen;
					for (DWORD k = 0; !bFound && !RegEnumKeyEx(hk, k, szName, &(nLen = countof(szName)-1), 0,0,0,0); k++)
					{
						HKEY hk2;
						if (!RegOpenKeyEx(hk, szName, 0, KEY_READ|nOpt, &hk2))
						{
							// Just in case, check "Path" too
							LPCWSTR rsNames[] = {NULL, L"Path"};

							for (size_t n = 0; n < countof(rsNames); n++)
							{
								ZeroStruct(szPath); DWORD nSize = (countof(szPath)-1)*sizeof(szPath[0]);
								if (!RegQueryValueExW(hk2, rsNames[n], NULL, NULL, (LPBYTE)szPath, &nSize) && *szPath)
								{
									wchar_t* psz, *pszEnd;
									psz = (wchar_t*)Unquote(szPath, true);
									pszEnd = wcsrchr(psz, L'\\');
									if (!pszEnd || lstrcmpi(pszEnd, L"\\tcmd.exe") || !FileExists(psz))
										continue;
									lstrcpyn(pszEnd+1, L"tcc.exe", 8);
									if (FileExists(psz))
									{
										bFound = true;
										if (b == 0)
											wcscpy_c(pOpt->Comspec32, psz);
										else
											wcscpy_c(pOpt->Comspec64, psz);
									}
								}
							} // for (size_t n = 0; n < countof(rsNames); n++)
							RegCloseKey(hk2);
						}
					} //  for, подключи
					RegCloseKey(hk);
				} // L"SOFTWARE\\JP Software"
			} // for (int b = 0; b <= 1; b++)

			// Если установлен TCMD - предпочтительно использовать именно его, независимо от битности
			if (*pOpt->Comspec32 && !*pOpt->Comspec64)
				wcscpy_c(pOpt->Comspec64, pOpt->Comspec32);
			else if (*pOpt->Comspec64 && !*pOpt->Comspec32)
				wcscpy_c(pOpt->Comspec32, pOpt->Comspec64);
		}

		// If "Take Command" not installed - try "TCC/LE"
		if (!*pOpt->Comspec32 || !*pOpt->Comspec64)
		{
			// [HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{16A21882-4138-4ADA-A390-F62DC27E4504}]
			// "DisplayVersion"="13.04.60"
			// "Publisher"="JP Software"
			// "DisplayName"="Take Command 13.0"
			// или
			// "DisplayName"="TCC/LE 13.0"
			// и наконец
			// "InstallLocation"="C:\\Program Files\\JPSoft\\TCMD13\\"
			for (int b = 0; b <= 1; b++)
			{
				// b==0 - 32bit, b==1 - 64bit
				if (b && !bWin64)
					continue;
				if (((b == 0) ? *pOpt->Comspec32 : *pOpt->Comspec64))
					continue; // этот уже нашелся в TCMD

				bool bFound = false;
				DWORD nOpt = (b == 0) ? (bWin64 ? KEY_WOW64_32KEY : 0) : (bWin64 ? KEY_WOW64_64KEY : 0);
				if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0, KEY_READ|nOpt, &hk))
				{
					wchar_t szName[MAX_PATH+1]; DWORD nLen;
					for (DWORD n = 0; !bFound && !RegEnumKeyEx(hk, n, szName, &(nLen = countof(szName)-1), 0,0,0,0); n++)
					{
						if (*szName != L'{')
							continue;
						HKEY hk2;
						if (!RegOpenKeyEx(hk, szName, 0, KEY_READ|nOpt, &hk2))
						{
							ZeroStruct(szPath); DWORD nSize = (countof(szPath) - 1)*sizeof(szPath[0]);
							if (!RegQueryValueExW(hk2, L"Publisher", NULL, NULL, (LPBYTE)szPath, &nSize)
								&& !lstrcmpi(szPath, L"JP Software"))
							{
								nSize = (countof(szPath)-12)*sizeof(szPath[0]);
								if (!RegQueryValueExW(hk2, L"InstallLocation", NULL, NULL, (LPBYTE)szPath, &nSize)
									&& *szPath)
								{
									wchar_t* psz, *pszEnd;
									if (szPath[0] == L'"')
									{
										psz = szPath + 1;
										pszEnd = wcschr(psz, L'"');
										if (pszEnd)
											*pszEnd = 0;
									}
									else
									{
										psz = szPath;
									}
									if (*psz)
									{
										pszEnd = psz+lstrlen(psz);
										if (*(pszEnd-1) != L'\\')
											*(pszEnd++) = L'\\';
										lstrcpyn(pszEnd, L"tcc.exe", 8);
										if (FileExists(psz))
										{
											bFound = true;
											if (b == 0)
												wcscpy_c(pOpt->Comspec32, psz);
											else
												wcscpy_c(pOpt->Comspec64, psz);
										}
									}
								}
							}
							RegCloseKey(hk2);
						}
					} // for, подключи
					RegCloseKey(hk);
				} // L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
			} // for (int b = 0; b <= 1; b++)
		}

		// Попытаться "в лоб" из "Program Files"
		if (!*pOpt->Comspec32 && !*pOpt->Comspec64)
		{

			const wchar_t* pszTcmd  = L"C:\\Program Files\\JPSoft\\TCMD13\\tcc.exe";
			const wchar_t* pszTccLe = L"C:\\Program Files\\JPSoft\\TCCLE13\\tcc.exe";
			if (FileExists(pszTcmd))
				wcscpy_c(pOpt->Comspec32, pszTcmd);
			else if (FileExists(pszTccLe))
				wcscpy_c(pOpt->Comspec32, pszTccLe);
		}

		if (*pOpt->Comspec32 && !*pOpt->Comspec64)
			wcscpy_c(pOpt->Comspec64, pOpt->Comspec32);
		else if (*pOpt->Comspec64 && !*pOpt->Comspec32)
			wcscpy_c(pOpt->Comspec32, pOpt->Comspec64);
	} // if (pOpt->csType == cst_AutoTccCmd)

	// С поиском tcc закончили. Теперь, если pOpt->Comspec32/pOpt->Comspec64 остались не заполнены
	// нужно сначала попытаться обработать переменную окружения ComSpec, а потом - просто "cmd.exe"
	if (!*pOpt->Comspec32)
		GetComspecFromEnvVar(pOpt->Comspec32, countof(pOpt->Comspec32), csb_x32);
	if (!*pOpt->Comspec64)
		GetComspecFromEnvVar(pOpt->Comspec64, countof(pOpt->Comspec64), csb_x64);
}

void UpdateComspec(ConEmuComspec* pOpt, bool DontModifyPath /*= false*/)
{
	if (!pOpt)
	{
		_ASSERTE(pOpt!=NULL);
		return;
	}

	if (pOpt->isUpdateEnv && (pOpt->csType != cst_EnvVar))
	{
		//if (pOpt->csType == cst_AutoTccCmd) -- always, if isUpdateEnv
		{
			LPCWSTR pszNew = NULL;
			switch (pOpt->csBits)
			{
			case csb_SameOS:
				pszNew = IsWindows64() ? pOpt->Comspec64 : pOpt->Comspec32;
				break;
			case csb_SameApp:
				pszNew = WIN3264TEST(pOpt->Comspec32,pOpt->Comspec64);
				break;
			case csb_x32:
				pszNew = pOpt->Comspec32;
				break;
			default:
				_ASSERTE(pOpt->csBits==csb_SameOS || pOpt->csBits==csb_SameApp || pOpt->csBits==csb_x32);
				pszNew = NULL;
			}
			if (pszNew && *pszNew)
			{
				#ifdef SHOW_COMSPEC_CHANGE
				wchar_t szCurrent[MAX_PATH]; GetEnvironmentVariable(L"ComSpec", szCurrent, countof(szCurrent));
				if (lstrcmpi(szCurrent, pszNew))
				{
					wchar_t szMsg[MAX_PATH*4], szProc[MAX_PATH] = {}, szPid[MAX_PATH];
					GetModuleFileName(NULL, szProc, countof(szProc));
					_wsprintf(szPid, SKIPLEN(countof(szPid))
						L"PID=%u, '%s'", GetCurrentProcessId(), PointToName(szProc));
					_wsprintf(szMsg, SKIPLEN(countof(szMsg))
						L"Changing %%ComSpec%% in %s\nCur=%s\nNew=%s",
						szPid , szCurrent, pszNew);
					MessageBox(NULL, szMsg, szPid, MB_SYSTEMMODAL);
				}
				#endif

				_ASSERTE(wcschr(pszNew, L'%')==NULL);
				SetEnvVarExpanded(L"ComSpec", pszNew);
			}
		}
	}

	if (pOpt->AddConEmu2Path && !DontModifyPath)
	{
		if ((pOpt->ConEmuBaseDir[0] == 0) || (pOpt->ConEmuExeDir[0] == 0))
		{
			_ASSERTE(pOpt->ConEmuBaseDir[0] != 0);
			_ASSERTE(pOpt->ConEmuExeDir[0] != 0);
		}
		else
		{
			wchar_t* pszCur = GetEnvVar(L"PATH");

			if (!pszCur)
				pszCur = lstrdup(L"");

			DWORD n = lstrlen(pszCur);
			wchar_t* pszUpr = lstrdup(pszCur);
			wchar_t* pszDirUpr = (wchar_t*)malloc(MAX_PATH*sizeof(*pszCur));

			MCHKHEAP;

			if (!pszUpr || !pszDirUpr)
			{
				_ASSERTE(pszUpr && pszDirUpr);
			}
			else
			{
				bool bChanged = false;
				wchar_t* pszAdd = NULL;

				CharUpperBuff(pszUpr, n);

				for (int i = 0; i <= 1; i++)
				{
					// Put '%ConEmuExeDir' on first place
					switch (i)
					{
					case 1:
						if (!(pOpt->AddConEmu2Path & CEAP_AddConEmuExeDir))
							continue;
						pszAdd = pOpt->ConEmuExeDir;
						break;
					case 0:
						if (!(pOpt->AddConEmu2Path & CEAP_AddConEmuBaseDir))
							continue;
						if (lstrcmp(pOpt->ConEmuExeDir, pOpt->ConEmuBaseDir) == 0)
							continue; // второй раз ту же директорию не добавляем
						pszAdd = pOpt->ConEmuBaseDir;
						break;
					}

					int nDirLen = lstrlen(pszAdd);
					lstrcpyn(pszDirUpr, pszAdd, MAX_PATH);
					CharUpperBuff(pszDirUpr, nDirLen);

					MCHKHEAP;

					// Need to find exact match!
					bool bFound = false;

					LPCWSTR pszFind = wcsstr(pszUpr, pszDirUpr);
					while (pszFind)
					{
						if (pszFind[nDirLen] == L';' || pszFind[nDirLen] == 0)
						{
							// OK, found
							bFound = true;
							break;
						}
						// Next try (may be partial match of subdirs...)
						pszFind = wcsstr(pszFind+nDirLen, pszDirUpr);
					}

					if (!bFound)
					{
						wchar_t* pszNew = lstrmerge(pszAdd, L";", pszCur);
						if (!pszNew)
						{
							_ASSERTE(pszNew && "Failed to reallocate PATH variable");
							break;
						}
						MCHKHEAP;
						SafeFree(pszCur);
						pszCur = pszNew;
						bChanged = true; // Set flag, check next dir
					}
				}

				MCHKHEAP;

				if (bChanged)
				{
					SetEnvironmentVariable(L"PATH", pszCur);
				}
			}

			MCHKHEAP;

			SafeFree(pszUpr);
			SafeFree(pszDirUpr);

			MCHKHEAP;
			SafeFree(pszCur);
		}
	}
}

void SetEnvVarExpanded(LPCWSTR asName, LPCWSTR asValue)
{
	if (!asName || !*asName || !asValue || !*asValue)
	{
		_ASSERTE(asName && *asName && asValue && *asValue);
		return;
	}

	DWORD cchMax = MAX_PATH;
	wchar_t *pszTemp = (wchar_t*)malloc(cchMax*sizeof(*pszTemp));
	if (wcschr(asValue, L'%'))
	{
		DWORD n = ExpandEnvironmentStrings(asValue, pszTemp, cchMax);
		if (n > cchMax)
		{
			cchMax = n+1;
			free(pszTemp);
			pszTemp = (wchar_t*)malloc(cchMax*sizeof(*pszTemp));
			n = ExpandEnvironmentStrings(asValue, pszTemp, cchMax);
		}
		if (n && n <= cchMax)
			asValue = pszTemp;
	}

	SetEnvironmentVariable(asName, asValue);

	SafeFree(pszTemp);
}
