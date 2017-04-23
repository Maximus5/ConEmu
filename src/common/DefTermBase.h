
/*
Copyright (c) 2012-2017 Maximus5
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

#if !defined(DEBUGSTRDEFTERM)
#define DEBUGSTRDEFTERM(s)
#undef USEDEBUGSTRDEFTERM
#else
#define USEDEBUGSTRDEFTERM
#endif

#include <Windows.h>
#include "Common.h"
#include "MArray.h"
#include "MSectionSimple.h"
#include "ConEmuCheck.h"
#include "MProcess.h"
#include "MProcessBits.h"
#include "WObjects.h"
#include "WThreads.h"
#include "../ConEmuCD/ExitCodes.h"
#include <TlHelp32.h>

#define DEF_TERM_ALIVE_CHECK_TIMEOUT 750
#define DEF_TERM_ALIVE_RECHECK_TIMEOUT 1500
#define DEF_TERM_INSTALL_TIMEOUT 30000

/* *********** Default terminal settings *********** */
struct CEDefTermOpt
{
public:
	BOOL     bUseDefaultTerminal;
	BOOL     bAgressive;
	BOOL     bNoInjects; // " /NOINJECT"
	BOOL     bNewWindow; // " /GHWND=NEW"
	BOOL     bDebugLog;
	BOOL     nDefaultTerminalConfirmClose; // "Press Enter to close console". 0 - Auto, 1 - Always, 2 - Never | " /CONFIRM" | " /NOCONFIRM"
	ConEmuConsoleFlags nConsoleFlags; // Used for populating m_SrvMapping in ShellProcessor
	wchar_t* pszConEmuExe; // Полный путь к ConEmu.exe
	wchar_t* pszConEmuBaseDir; // %ConEmuBaseDir%
	wchar_t* pszCfgFile; // " /LoadCfgFile "...""
	wchar_t* pszConfigName; // " /CONFIG "...""
	wchar_t* pszzHookedApps; // ASCIIZZ
	bool     bExternalPointers;
	// Last checking time
	FILETIME ftLastCheck;
public:
	CEDefTermOpt()
	{
		bUseDefaultTerminal = FALSE;
		bAgressive = FALSE;
		bNoInjects = FALSE;
		bNewWindow = FALSE;
		bDebugLog = FALSE;
		nDefaultTerminalConfirmClose = 0;
		nConsoleFlags = CECF_Empty;
		pszConEmuExe = NULL;
		pszConEmuBaseDir = NULL;
		pszCfgFile = NULL;
		pszConfigName = NULL;
		pszzHookedApps = NULL;
		bExternalPointers = false;
		ZeroStruct(ftLastCheck);
	};

	~CEDefTermOpt()
	{
		if (!bExternalPointers)
		{
			if (pszConEmuExe)
				free(pszConEmuExe);
			if (pszConEmuBaseDir)
				free(pszConEmuBaseDir);
			if (pszCfgFile)
				free(pszCfgFile);
			if (pszConfigName)
				free(pszConfigName);
			if (pszzHookedApps)
				free(pszzHookedApps);
		}
	};

public:
	bool Serialize(bool bSave=false)
	{
		if (!this)
			return false;

		// use dynamic link, because code is using in ConEmuHk too
		typedef LONG (WINAPI* RegCreateKeyEx_t)(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
		typedef LONG (WINAPI* RegCloseKey_t)(HKEY hKey);
		typedef LONG (WINAPI* RegQueryValueEx_t)(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
		typedef LONG (WINAPI* RegSetValueEx_t)(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE *lpData, DWORD cbData);
		typedef LONG (WINAPI* RegQueryInfoKey_t)(HKEY hKey, LPWSTR lpClass, LPDWORD lpcClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcMaxSubKeyLen, LPDWORD lpcMaxClassLen, LPDWORD lpcValues, LPDWORD lpcMaxValueNameLen, LPDWORD lpcMaxValueLen, LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime);
		static HMODULE hAdvApi = NULL;
		static RegCreateKeyEx_t regCreateKeyEx = NULL;
		static RegCloseKey_t regCloseKey = NULL;
		static RegQueryValueEx_t regQueryValueEx = NULL;
		static RegSetValueEx_t regSetValueEx = NULL;
		static RegQueryInfoKey_t regQueryInfoKey = NULL;
		if (!hAdvApi)
		{
			hAdvApi = LoadLibrary(L"advapi32.dll");
			if (!hAdvApi)
				return false;
		}
		if (!regCreateKeyEx || !regCloseKey || !regQueryValueEx || !regSetValueEx || !regQueryInfoKey)
		{
			regCreateKeyEx = (RegCreateKeyEx_t)GetProcAddress(hAdvApi, "RegCreateKeyExW");
			regCloseKey = (RegCloseKey_t)GetProcAddress(hAdvApi, "RegCloseKey");
			regQueryValueEx  = (RegQueryValueEx_t)GetProcAddress(hAdvApi, "RegQueryValueExW");
			regSetValueEx = (RegSetValueEx_t)GetProcAddress(hAdvApi, "RegSetValueExW");
			regQueryInfoKey = (RegQueryInfoKey_t)GetProcAddress(hAdvApi, "RegQueryInfoKeyW");
			if (!regCreateKeyEx || !regCloseKey || !regQueryValueEx || !regSetValueEx || !regQueryInfoKey)
				return false;
		}

		const LPCWSTR sSubKey = /*[HKCU]*/ L"Software\\ConEmu";
		HKEY hk = NULL;
		DWORD samDesired = bSave ? KEY_WRITE : KEY_READ;
		DWORD dwValue = 0, dwSize, dwType;
		LONG lRc = regCreateKeyEx(HKEY_CURRENT_USER, sSubKey, 0, NULL, 0, samDesired, NULL, &hk, &dwValue);
		if ((lRc != 0) || (hk == NULL))
		{
			return false;
		}

		if (!bSave)
		{
			FILETIME ftKey = {};
			lRc = regQueryInfoKey(hk, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &ftKey);
			if ((lRc == 0)
				&& (ftLastCheck.dwHighDateTime || ftLastCheck.dwLowDateTime)
				&& (memcmp(&ftKey, &ftLastCheck, sizeof(ftLastCheck)) == 0))
			{
				// Ничего не менялось, перечитывать не нужно
				regCloseKey(hk);
				return (bUseDefaultTerminal != 0);
			}
			ftLastCheck = ftKey;
		}

		_ASSERTE(sizeof(nConsoleFlags)==sizeof(DWORD) && sizeof(bUseDefaultTerminal)==sizeof(DWORD));

		struct strOpt {
			LPCWSTR Name; void* ptr; DWORD Size; DWORD Type;
		} Opt[] = {
			{L"DefTerm-Enabled",   &bUseDefaultTerminal, sizeof(bUseDefaultTerminal), REG_DWORD},
			{L"DefTerm-Agressive", &bAgressive, sizeof(bAgressive), REG_DWORD},
			{L"DefTerm-NoInjects", &bNoInjects, sizeof(bNoInjects), REG_DWORD},
			{L"DefTerm-NewWindow", &bNewWindow, sizeof(bNewWindow), REG_DWORD},
			{L"DefTerm-DebugLog",  &bDebugLog,  sizeof(bDebugLog),  REG_DWORD},
			{L"DefTerm-Confirm",   &nDefaultTerminalConfirmClose, sizeof(nDefaultTerminalConfirmClose), REG_DWORD},
			{L"DefTerm-Flags",     &nConsoleFlags, sizeof(nConsoleFlags), REG_DWORD},
			{L"DefTerm-ConEmuExe", &pszConEmuExe, 0, REG_SZ},
			{L"DefTerm-BaseDir",   &pszConEmuBaseDir, 0, REG_SZ},
			{L"DefTerm-CfgFile",   &pszCfgFile, 0, REG_SZ},
			{L"DefTerm-Config",    &pszConfigName, 0, REG_SZ},
			{L"DefTerm-AppList",   &pszzHookedApps, 0, REG_MULTI_SZ},
		};

		bool bRc = false;

		if (!bSave)
		{
			// Load
			for (INT_PTR i = 0; i < countof(Opt); i++)
			{
				if (Opt[i].Size)
				{
					// BYTE or BOOL
					dwSize = sizeof(dwValue);
					lRc = regQueryValueEx(hk, Opt[i].Name, NULL, &dwType, (LPBYTE)&dwValue, &dwSize);
					if (lRc != 0) dwValue = 0;
					if (Opt[i].Size == sizeof(DWORD))
						*((LPDWORD)Opt[i].ptr) = dwValue;
					else if (Opt[i].Size == sizeof(BYTE))
						*((LPBYTE)Opt[i].ptr) = LOBYTE(dwValue);
				}
				else // String or MSZ
				{
					wchar_t* pszOld = *((wchar_t**)Opt[i].ptr);
					wchar_t* pszNew = NULL;

					dwSize = 0;
					lRc = regQueryValueEx(hk, Opt[i].Name, NULL, &dwType, NULL, &dwSize);
					if ((lRc == 0) && dwSize && (dwType == REG_SZ || dwType == REG_MULTI_SZ))
					{
						pszNew = (wchar_t*)calloc(dwSize+2*sizeof(wchar_t),1);
						if (pszNew)
						{
							lRc = regQueryValueEx(hk, Opt[i].Name, NULL, &dwType, (LPBYTE)pszNew, &dwSize);
							// Do not need to store empty strings
							if (!*pszNew)
							{
								free(pszNew);
								pszNew = NULL;
							}
						}
					}

					*((wchar_t**)Opt[i].ptr) = pszNew;
					if (pszOld) free(pszOld);
				}
			}

			bRc = (bUseDefaultTerminal != 0);
		}
		#ifndef CONEMU_MINIMAL
		else
		{
			// Save
			bRc = true;

			for (INT_PTR i = 0; i < countof(Opt); i++)
			{
				if (Opt[i].Size)
				{
					// BYTE or BOOL
					dwSize = Opt[i].Size;
					lRc = regSetValueEx(hk, Opt[i].Name, NULL, Opt[i].Type, (LPBYTE)Opt[i].ptr, dwSize);
				}
				else if (Opt[i].Type == REG_SZ)
				{
					LPCWSTR pSZ = *((wchar_t**)Opt[i].ptr);
					if (!pSZ) pSZ = L"";
					dwSize = (lstrlen(pSZ)+1) * sizeof(wchar_t);
					lRc = regSetValueEx(hk, Opt[i].Name, NULL, REG_SZ, (LPBYTE)pSZ, dwSize);
				}
				else if (Opt[i].Type == REG_MULTI_SZ)
				{
					LPCWSTR pMSZ = *((wchar_t**)Opt[i].ptr);
					if (!pMSZ) pMSZ = L"\0";
					dwSize = sizeof(wchar_t);
					LPCWSTR ptr = pMSZ;
					while (*ptr)
					{
						int nLen = lstrlen(ptr);
						if (nLen < 1)
							break;
						dwSize += (nLen+1)*sizeof(wchar_t);
						ptr += (nLen+1);
					}
					lRc = regSetValueEx(hk, Opt[i].Name, NULL, REG_MULTI_SZ, (LPBYTE)pMSZ, dwSize);
				}
				if (lRc != 0)
					bRc = false;
			}
		}
		#endif

		regCloseKey(hk);
		return bRc;
	}
};
/* *********** Default terminal settings *********** */

class CDefTermBase
{
protected:
	struct ProcessInfo
	{
		HWND      hWnd;
		HANDLE    hProcess;
		DWORD     nPID;
		DWORD     nHookTick; // informational field
		DWORD_PTR nHookResult; // _PTR for alignment
	};
	struct ThreadArg
	{
		CDefTermBase* pTerm;
		HWND hFore;
		DWORD nForePID;
	};
	struct ThreadHandle
	{
		HANDLE hThread;
		DWORD nThreadId, nCreatorThreadId;
	};

private:
	bool   mb_ConEmuGui;
	bool   mb_ReadyToHook;
	bool   mb_PostCreatedThread;
	bool   mb_Initialized;
	HWND   mh_LastWnd;
	HWND   mh_LastIgnoredWnd;
	HWND   mh_LastCall;
	MArray<ThreadHandle> m_Threads;
	MArray<ProcessInfo> m_Processed;
protected:
	HANDLE mh_PostThread;
	DWORD  mn_PostThreadId;
	MSectionSimple* mcs;
	bool   mb_Termination;
protected:
	// Shell may not like injecting immediately after start-up
	// If so, set (mb_ExplorerHookAllowed = false) in constructor
	bool   mb_ExplorerHookAllowed;

protected:
	CEDefTermOpt m_Opt;
public:
	const CEDefTermOpt* GetOpt()
	{
		return &m_Opt;
	};

protected:
	virtual CDefTermBase* GetInterface() = 0;
	virtual bool isDefaultTerminalAllowed(bool bDontCheckName = false) = 0; // !(gpConEmu->DisableSetDefTerm || !gpSet->isSetDefaultTerminal)
	virtual int  DisplayLastError(LPCWSTR asLabel, DWORD dwError=0, DWORD dwMsgFlags=0, LPCWSTR asTitle=NULL, HWND hParent=NULL) = 0;
	virtual void ShowTrayIconError(LPCWSTR asErrText) = 0; // Icon.ShowTrayIcon(asErrText, tsa_Default_Term);
	virtual void ReloadSettings() = 0; // Copy from gpSet or load from [HKCU]

protected:
	virtual void StopHookers()
	{
		mb_Termination = true;

		ClearThreads(true);

		ClearProcessed(true);

		SafeDelete(mcs);
	}

public:
	CDefTermBase(bool bConEmuGui)
	{
		mb_ConEmuGui = bConEmuGui;
		mb_ReadyToHook = false;
		mh_LastWnd = mh_LastIgnoredWnd = mh_LastCall = NULL;
		mh_PostThread = NULL;
		mn_PostThreadId = 0;
		mb_PostCreatedThread = false;
		mb_Initialized = false;
		mb_ExplorerHookAllowed = true;
		mb_Termination = false;
		mcs = new MSectionSimple(true);
	};

	virtual ~CDefTermBase()
	{
		// StopHookers(); -- Have to be called in the ancestor destructor!
		_ASSERTE(mcs==NULL);
	};

public:
	bool IsReady()
	{
		return (this && mb_Initialized && mb_ReadyToHook);
	};

	static bool IsShellTrayWindowClass(LPCWSTR asClassName)
	{
		bool bShellTrayWnd = (asClassName != NULL)
			&& ((lstrcmp(asClassName, L"Shell_TrayWnd") == 0)
				);
		return bShellTrayWnd;
	};

	static bool IsExplorerWindowClass(LPCWSTR asClassName)
	{
		bool bShellWnd = (asClassName != NULL)
			&& ((lstrcmp(asClassName, L"CabinetWClass") == 0)
				|| (lstrcmp(asClassName, L"ExploreWClass") == 0)
				);
		return bShellWnd;
	};

	static bool IsWindowResponsive(HWND hFore)
	{
		LRESULT lMsgRc;
		DWORD_PTR dwResult = 0;

		// To be sure that application is responsive (started successfully and is not hunged)
		// However, that does not guarantee that explorer.exe will be hooked properly
		// During several restarts of explorer.exe from TaskMgr - sometimes it hungs...
		// So, that is the last change but not sufficient (must be handled in GUI too)
		// -- SMTO_ERRORONEXIT exist in Vista only
		lMsgRc = SendMessageTimeout(hFore, WM_NULL, 0,0, SMTO_NORMAL, DEF_TERM_ALIVE_CHECK_TIMEOUT, &dwResult);
		return (lMsgRc != 0);
	}

	bool IsAppMonitored(HWND hFore, DWORD nForePID, bool bSkipShell = true, PROCESSENTRY32* pPrc = NULL, LPWSTR pszClass/*[MAX_PATH]*/ = NULL)
	{
		// Должен выполнять все проверки, запоминать успех/не успех,
		// чтобы при вызове CheckForeground не выполнять проверки повторно
		// Т.к. оно должно вызываться из CConEmuMain::RecheckForegroundWindow

		bool lbConHostLocked = false;
		wchar_t szClass[MAX_PATH]; szClass[0] = 0;
		PROCESSENTRY32 prc = {};
		bool bMonitored = false;
		bool bShellTrayWnd = false; // task bar
		bool bShellWnd = false; // one of explorer windows (folder browsers)
		#ifdef USEDEBUGSTRDEFTERM
		wchar_t szInfo[MAX_PATH+80];
		#endif

		// Check window class
		if (GetClassName(hFore, szClass, countof(szClass)))
		{
			bShellTrayWnd = IsShellTrayWindowClass(szClass);
			bShellWnd = IsExplorerWindowClass(szClass);

			#ifdef USEDEBUGSTRDEFTERM
			_wsprintf(szInfo, SKIPCOUNT(szInfo) L"DefTerm::CheckForeground x%08X PID=%u %u %u '%s'", (DWORD)(DWORD_PTR)hFore, nForePID, (UINT)bShellTrayWnd, (UINT)bShellWnd, szClass);
			DEBUGSTRDEFTERM(szInfo);
			#endif

			if ((lstrcmp(szClass, VirtualConsoleClass) == 0)
				//|| (lstrcmp(szClass, L"#32770") == 0) // Ignore dialogs // -- Process dialogs too (Application may be dialog-based)
				|| ((bSkipShell || !mb_ExplorerHookAllowed) && bShellTrayWnd) // Do not hook explorer windows?
				|| isConsoleClass(szClass))
			{
				#ifdef USEDEBUGSTRDEFTERM
				_wsprintf(szInfo, SKIPCOUNT(szInfo) L"DefTerm::CheckForeground x%08X PID=%u ignored by class", (DWORD)(DWORD_PTR)hFore, nForePID);
				DEBUGSTRDEFTERM(szInfo);
				#endif
				goto wrap;
			}
		}

		// May be hooked by class name? e.g. "TaskManagerWindow"
		bMonitored = IsAppMonitored(szClass);
		if (!bMonitored)
		{
			// Then check by process exe name
			if (!GetProcessInfo(nForePID, &prc))
			{
				#ifdef USEDEBUGSTRDEFTERM
				_wsprintf(szInfo, SKIPCOUNT(szInfo) L"DefTerm::CheckForeground x%08X PID=%u skipped, can't get process name", (DWORD)(DWORD_PTR)hFore, nForePID);
				DEBUGSTRDEFTERM(szInfo);
				#endif
				goto wrap;
			}

			CharLowerBuff(prc.szExeFile, lstrlen(prc.szExeFile));

			if (lstrcmp(prc.szExeFile, L"csrss.exe") == 0)
			{
				// This is "System" process and may not be hooked
				#ifdef USEDEBUGSTRDEFTERM
				_wsprintf(szInfo, SKIPCOUNT(szInfo) L"DefTerm::CheckForeground x%08X PID=%u skipped, csrss.exe", (DWORD)(DWORD_PTR)hFore, nForePID);
				DEBUGSTRDEFTERM(szInfo);
				#endif
				goto wrap;
			}

			// Is it in monitored applications?
			bMonitored = IsAppMonitored(prc.szExeFile);

			// And how it is?
			if (!bMonitored)
			{
				mh_LastIgnoredWnd = hFore;
				#ifdef USEDEBUGSTRDEFTERM
				_wsprintf(szInfo, SKIPCOUNT(szInfo) L"DefTerm::CheckForeground x%08X PID=%u skipped, app is not monitored", (DWORD)(DWORD_PTR)hFore, nForePID);
				DEBUGSTRDEFTERM(szInfo);
				#endif
				goto wrap;
			}
		}
	wrap:
		if (pPrc)
			memmove(pPrc, &prc, sizeof(*pPrc));
		if (pszClass)
			lstrcpyn(pszClass, szClass, MAX_PATH);
		return bMonitored;
	}

	bool CheckForeground(HWND hFore, DWORD nForePID, bool bRunInThread = true, bool bSkipShell = true)
	{
		if (mb_Termination)
		{
			return false;
		}

		if (!isDefaultTerminalAllowed())
		{
			#ifdef USEDEBUGSTRDEFTERM
			static bool bNotified = false;
			if (!bNotified)
			{
				bNotified = true;
				DEBUGSTRDEFTERM(L"!!! DefTerm::CheckForeground skipped, !isDefaultTerminalAllowed !!!");
			}
			#endif
			return false;
		}

		// Если еще не были инициализированы?
		if (!mb_ReadyToHook
			// Или ошибка в параметрах
			|| (!hFore || !nForePID)
			// Или это вобще окно этого процесса
			|| (nForePID == GetCurrentProcessId()))
		{
			// Сразу выходим
			DEBUGSTRDEFTERM(L"!!! DefTerm::CheckForeground skipped, uninitialized (mb_ReadyToHook, hFore, nForePID) !!!");
			_ASSERTE(mb_ReadyToHook && "Must be initialized");
			_ASSERTE(hFore && nForePID);
			return false;
		}

		if (hFore == mh_LastWnd || hFore == mh_LastIgnoredWnd)
		{
			DEBUGSTRDEFTERM(L"DefTerm::CheckForeground skipped, hFore was already checked");
			// Это окно уже проверялось
			return (hFore == mh_LastWnd);
		}

		// GO

		bool lbRc = false;
		int  iHookerRc = -1;
		MSectionLockSimple CS;
		bool lbConHostLocked = false;
		DWORD nResult = 0;
		wchar_t szClass[MAX_PATH]; szClass[0] = 0;
		PROCESSENTRY32 prc = {};
		bool bMonitored = false;
		bool bNotified = false;
		HANDLE hProcess = NULL;
		DWORD nErrCode = 0;
		wchar_t szInfo[MAX_PATH+80];


		if (bRunInThread && (hFore == mh_LastCall))
		{
			// Просто выйти. Это проверка на частые фоновые вызовы.
			DEBUGSTRDEFTERM(L"DefTerm::CheckForeground skipped, already bRunInThread");
			goto wrap;
		}
		mh_LastCall = hFore;

		if (bRunInThread)
		{
			AutoClearThreads();

			HANDLE hPostThread = NULL; DWORD nThreadId = 0;
			ThreadArg* pArg = (ThreadArg*)malloc(sizeof(ThreadArg));
			if (!pArg)
			{
				_ASSERTE(pArg);
				goto wrap;
			}
			pArg->pTerm = GetInterface();
			pArg->hFore = hFore;
			pArg->nForePID = nForePID;

			DEBUGSTRDEFTERM(L"DefTerm::CheckForeground creating background thread PostCheckThread");

			hPostThread = apiCreateThread(PostCheckThread, pArg, &nThreadId, "DefTerm::PostCheckThread");
			_ASSERTE(hPostThread!=NULL);
			if (hPostThread)
			{
				ThreadHandle th = {hPostThread, nThreadId, GetCurrentThreadId()};
				m_Threads.push_back(th);

				#ifdef USEDEBUGSTRDEFTERM
				_wsprintf(szInfo, SKIPCOUNT(szInfo) L"DefTerm::CheckForeground created TID=%u", nThreadId);
				DEBUGSTRDEFTERM(szInfo);
				#endif
			}

			lbRc = (hPostThread != NULL); // вернуть OK?
			goto wrap;
		}

		if (mb_Termination)
			goto wrap;

		// To be sure that application is responsive (started successfully and is not hunged)
		// However, that does not guarantee that explorer.exe will be hooked properly
		// During several restarts of explorer.exe from TaskMgr - sometimes it hungs...
		// So, that is the last change but not sufficient (must be handled in GUI too)
		// -- SMTO_ERRORONEXIT exist in Vista only
		if (!IsWindowResponsive(hFore))
		{
			// That app is not ready for hooking
			if (mh_LastCall == hFore)
				mh_LastCall = NULL;
			#ifdef USEDEBUGSTRDEFTERM
			_wsprintf(szInfo, SKIPCOUNT(szInfo) L"!!! DefTerm::CheckForeground skipped, x%08X is non-responsive !!!", (DWORD)(DWORD_PTR)hFore);
			DEBUGSTRDEFTERM(szInfo);
			#endif
			goto wrap;
		}

		CS.Lock(mcs);

		// Clear dead processes and windows
		ClearProcessed(false);

		if (mb_Termination)
			goto wrap;

		// Check if app is monitored by window class and process name
		bMonitored = IsAppMonitored(hFore, nForePID, bSkipShell, &prc, szClass);

		if (!bMonitored)
		{
			mh_LastIgnoredWnd = hFore;
			goto wrap;
		}

		// That is hooked executable/classname
		// But may be it was processed already?
		for (INT_PTR i = m_Processed.size(); i-- && !mb_Termination;)
		{
			if (m_Processed[i].nPID == nForePID)
			{
				bMonitored = false;
				#ifdef USEDEBUGSTRDEFTERM
				_wsprintf(szInfo, SKIPCOUNT(szInfo) L"DefTerm::CheckForeground x%08X PID=%u skipped, already processed", (DWORD)(DWORD_PTR)hFore, nForePID);
				DEBUGSTRDEFTERM(szInfo);
				#endif
				break; // already hooked/processed
			}
		}

		// May be hooked already?
		if (!bMonitored || mb_Termination)
		{
			mh_LastWnd = hFore;
			lbRc = true;
			goto wrap;
		}

		_ASSERTE(isDefaultTerminalAllowed());

		_wsprintf(szInfo, SKIPCOUNT(szInfo) L"CheckForeground x%08X <<== trying to set hooks", LODWORD(hFore));
		LogHookingStatus(nForePID, szInfo);

		bNotified = NotifyHookingStatus(nForePID, prc.szExeFile[0] ? prc.szExeFile : szClass);

		ConhostLocker(true, lbConHostLocked);

		_ASSERTE(hProcess == NULL);
		iHookerRc = StartDefTermHooker(nForePID, hProcess, nResult, m_Opt.pszConEmuBaseDir, nErrCode);

		ConhostLocker(false, lbConHostLocked);

		_wsprintf(szInfo, SKIPCOUNT(szInfo) L"CheckForeground x%08X <<== nResult=%i iHookerRc=%i", LODWORD(hFore), nResult, iHookerRc);
		LogHookingStatus(nForePID, szInfo);

		if (iHookerRc != 0)
		{
			mh_LastIgnoredWnd = hFore;
			if (iHookerRc == -3)
			{
				DisplayLastError(L"Failed to start hooking application!\nDefault terminal feature will not be available!", nErrCode);
			}
			goto wrap;
		}

		// Always store to m_Processed
		{
			ProcessInfo inf = {};
			inf.hWnd = hFore;
			inf.hProcess = hProcess;
			hProcess = NULL; // его закрывать (если есть) НЕ нужно, сохранен в массиве
			inf.nPID = nForePID;
			inf.nHookTick = GetTickCount();
			inf.nHookResult = nResult;
			m_Processed.push_back(inf);
		}

		// And what?
		if ((nResult == (UINT)CERR_HOOKS_WAS_SET) || (nResult == (UINT)CERR_HOOKS_WAS_ALREADY_SET))
		{
			mh_LastWnd = hFore;
			lbRc = true;
			goto wrap;
		}
		// Failed, remember this
		CloseHandle(hProcess);
		mh_LastIgnoredWnd = hFore;
		_ASSERTE(lbRc == false);
	wrap:
		CS.Unlock();
		if (bNotified)
		{
			NotifyHookingStatus(0, NULL);
			LogHookingStatus(nForePID, L"CheckForeground finished");
		}
		return lbRc;
	};

public:
	void OnHookedListChanged()
	{
		if (!mb_Initialized)
			return;
		if (!isDefaultTerminalAllowed(true))
			return;

		MSectionLockSimple CS;
		CS.Lock(mcs);
		ClearProcessed(true);
		CS.Unlock();

		// Если проводника в списке НЕ было, а сейчас появился...
		// Проверить процесс шелла
		CheckShellWindow();
	};

public:
	int StartDefTermHooker(DWORD nPID, HANDLE hDefProcess = NULL)
	{
		int iRc;
		HANDLE hProcess = NULL;
		DWORD nResult = 0, nErrCode = 0;
		MSectionLockSimple CS;
		CS.Lock(mcs);

		// Сюда мы попадаем после CreateProcess (ConEmuHk) или из VisualStudio для "*.vshost.exe"
		// Так что теоретически и дескриптор открыть должны уметь и хуки поставить

		if (hDefProcess)
		{
			// This handle may come from CreateProcess (ConEmuHk), need to make a copy of it
			DuplicateHandle(GetCurrentProcess(), hDefProcess, GetCurrentProcess(), &hProcess, 0, FALSE, DUPLICATE_SAME_ACCESS);
		}

		// Always store to m_Processed
		{
			ProcessInfo inf = {};
			inf.hProcess = hProcess; // hProcess закрывать НЕ нужно, сохранен в массиве
			inf.nPID = nPID;
			inf.nHookTick = GetTickCount();
			inf.nHookResult = STILL_ACTIVE;
			m_Processed.push_back(inf);
		}

		// Because we must unlock the section before StartDefTermHooker to avoid dead locks in VS
		CS.Unlock();

		// Do the job
		iRc = StartDefTermHooker(nPID, hProcess, nResult, m_Opt.pszConEmuBaseDir, nErrCode);

		return iRc;
	}

private:
	int StartDefTermHooker(DWORD nForePID, HANDLE& hProcess, DWORD& nResult, LPCWSTR asConEmuBaseDir, DWORD& nErrCode)
	{
		if (mb_Termination)
			return -1;

		int iRc = -1;
		wchar_t szCmdLine[MAX_PATH*2];
		wchar_t szConTitle[32] = L"ConEmu";
		LPCWSTR pszSrvExe = NULL;
		int nBits;
		PROCESS_INFORMATION pi = {};
		STARTUPINFO si = {sizeof(si)};
		BOOL bStarted = FALSE;
		HANDLE hMutex = NULL;
		wchar_t szMutexName[64];
		DWORD nMutexCreated = 0;
		wchar_t szInfo[120];
		DWORD nWaitStartTick = (DWORD)-1, nWaitDelta = (DWORD)-1;

		nErrCode = 0;

		msprintf(szMutexName, countof(szMutexName), CEDEFAULTTERMBEGIN, nForePID);
		hMutex = CreateMutex(LocalSecurity(), TRUE, szMutexName);
		nMutexCreated = hMutex ? GetLastError() : 0;

		// If was not opened yet
		if (hProcess == NULL)
		{
			hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|SYNCHRONIZE, FALSE, nForePID);
		}

		if (!hProcess)
		{
			// Failed to hook
			nErrCode = GetLastError();
			_wsprintf(szInfo, SKIPCOUNT(szInfo) L"OpenProcess fails, code=%u", nErrCode);
			LogHookingStatus(nForePID, szInfo);

			if (nErrCode == ERROR_ACCESS_DENIED)
			{
				TODO("Let's try to run ConEmuC[64] as admin");
			}
			goto wrap;
		}

		if (nMutexCreated == ERROR_ALREADY_EXISTS)
		{
			// Hooking was started from another process (most probably in agressive mode)
			iRc = 0;
			nResult = CERR_HOOKS_WAS_ALREADY_SET;
			_wsprintf(szInfo, SKIPCOUNT(szInfo) L"Mutex already exists");
			LogHookingStatus(nForePID, szInfo);
			goto wrap;
		}

		// Need to be hooked
		nBits = GetProcessBits(nForePID, hProcess);
		switch (nBits)
		{
		case 32:
			pszSrvExe = L"ConEmuC.exe";
			break;
		case 64:
			pszSrvExe = L"ConEmuC64.exe";
			break;
		default:
			pszSrvExe = NULL;
		}
		if (!pszSrvExe)
		{
			// Unsupported bitness?
			CloseHandle(hProcess);
			iRc = -2;
			_wsprintf(szInfo, SKIPCOUNT(szInfo) L"Unsupported process bitness (%i)", nBits);
			LogHookingStatus(nForePID, szInfo);
			goto wrap;
		}
		msprintf(szCmdLine, countof(szCmdLine), L"\"%s\\%s\"%s /DEFTRM=%u",
			asConEmuBaseDir, pszSrvExe,
			isLogging() ? L" /LOG" : L"",
			nForePID);

		if (mb_Termination)
		{
			iRc = -1;
			goto wrap;
		}

		// Run hooker
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.lpTitle = szConTitle;
		LogHookingStatus(nForePID, szCmdLine);
		bStarted = CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
		if (!bStarted)
		{
			//DisplayLastError(L"Failed to start hooking application!\nDefault terminal feature will not be available!");
			nErrCode = GetLastError();
			CloseHandle(hProcess);
			iRc = -3;
			_wsprintf(szInfo, SKIPCOUNT(szInfo) L"Fails to start service process, code=%u", nErrCode);
			LogHookingStatus(nForePID, szInfo);
			goto wrap;
		}
		CloseHandle(pi.hThread);

		_wsprintf(szInfo, SKIPCOUNT(szInfo) L"Service process PID=%u started", pi.dwProcessId);
		LogHookingStatus(nForePID, szInfo);

		// Waiting for result, to avoid multiple hooks processed (due to closed mutex)
		// Within VisualStudio and others don't do INFINITE wait to avoid dead locks
		nWaitStartTick = GetTickCount();
		while (!mb_Termination)
		{
			if (WaitForSingleObject(pi.hProcess, 100) == WAIT_OBJECT_0)
				break;
			nWaitDelta = GetTickCount() - nWaitStartTick;
			if (!mb_ConEmuGui && (nWaitDelta >= DEF_TERM_INSTALL_TIMEOUT))
				break;
		}
		if (!GetExitCodeProcess(pi.hProcess, &nResult))
			nResult = (DWORD)-1;
		_wsprintf(szInfo, SKIPCOUNT(szInfo) L"Service process PID=%u finished with code=%u", pi.dwProcessId, nResult);
		LogHookingStatus(nForePID, szInfo);

		CloseHandle(pi.hProcess);

		iRc = 0;
	wrap:
		if (hMutex)
		{
			CloseHandle(hMutex);
		}
		return iRc;
	}

protected:
	void Initialize(bool bWaitForReady /*= false*/, bool bShowErrors /*= false*/, bool bNoThreading /*= false*/)
	{
		// Once
		if (!mb_Initialized)
		{
			mb_Initialized = true;
			ReloadSettings();
		}

		if (!isDefaultTerminalAllowed())
		{
			_ASSERTE(bWaitForReady == false);
			if (bShowErrors && m_Opt.bUseDefaultTerminal)
			{
				DisplayLastError(L"Default terminal feature was blocked\n"
					L"with '/NoDefTerm' ConEmu command line argument!", -1);
			}
			return;
		}

		#ifdef _DEBUG
		DWORD nDbgWait = mh_PostThread ? WaitForSingleObject(mh_PostThread,0) : WAIT_OBJECT_0;
		DWORD nDbgExitCode = (DWORD)-1; if (mh_PostThread) GetExitCodeThread(mh_PostThread, &nDbgExitCode);
		_ASSERTE(nDbgWait==WAIT_OBJECT_0);
		#endif

		if (mb_PostCreatedThread)
		{
			if (!bShowErrors)
				return;
			ClearThreads(false);
			if (m_Threads.size() > 0)
			{
				ShowTrayIconError(L"Previous Default Terminal setup cycle was not finished yet");
				return;
			}
		}

		PreCreateThread();

		mb_ReadyToHook = true;

		// Even if it is not a ConEmu.exe we need to start thread,
		// aggressive mode may be turned on by user in any moment
		// We need just slow down our thred as possible...

		// Этот процесс занимает некоторое время, чтобы не блокировать основной поток - запускаем фоновый
		mb_PostCreatedThread = true;
		DWORD  nWait = WAIT_FAILED;
		HANDLE hPostThread = apiCreateThread(PostCreatedThread, GetInterface(), &mn_PostThreadId, "DefTerm::PostCreatedThread");

		if (hPostThread)
		{
			if (bWaitForReady)
			{
				// Wait for 30 seconds or infinite if bNoThreading specifed (used for ConEmu.exe /SetDefTerm /Exit)
				DWORD nWaitTimeout = bNoThreading ? INFINITE : DEF_TERM_INSTALL_TIMEOUT;
				// Do wait
				SetCursor(LoadCursor(NULL, IDC_WAIT));
				DWORD nStart = GetTickCount();
				nWait = WaitForSingleObject(hPostThread, nWaitTimeout);
				SetCursor(LoadCursor(NULL, IDC_ARROW));
				DWORD nDuration = GetTickCount() - nStart;
				if (nWait == WAIT_OBJECT_0)
				{
					CloseHandle(hPostThread);
					return;
				}
				else
				{
					//_ASSERTE(nWait == WAIT_OBJECT_0);
					DisplayLastError(L"PostCreatedThread was not finished in 30 seconds");
					UNREFERENCED_PARAMETER(nDuration);
				}
			}
			else
			{
				mh_PostThread = hPostThread;
			}

			ThreadHandle th = {hPostThread, mn_PostThreadId, GetCurrentThreadId()};
			m_Threads.push_back(th);
		}
		else
		{
			if (bShowErrors)
			{
				DisplayLastError(L"Failed to start PostCreatedThread");
			}
			_ASSERTE(hPostThread!=NULL);
			mb_PostCreatedThread = false;
		}
	};

protected:
	virtual void PreCreateThread()
	{
	};

protected:
	virtual void PostCreateThreadFinished()
	{
	};

protected:
	// nPID = 0 when hooking is done (remove status bar notification)
	// sName is executable name or window class name
	virtual bool NotifyHookingStatus(DWORD nPID, LPCWSTR sName)
	{
		// descendant must return true if status bar was changed
		return false;
	};
public:
	// Messages to be placed in log
	virtual void LogHookingStatus(DWORD nForePID, LPCWSTR sMessage)
	{
		DEBUGSTRDEFTERM(sMessage);
	};
	virtual bool isLogging()
	{
		return false;
	};

protected:
	virtual void AutoClearThreads()
	{
		//if (!isMainThread())
		//	return;

		// Clear finished threads
		ClearThreads(false);
	};

protected:
	virtual void ConhostLocker(bool bLock, bool& bWasLocked)
	{
	};

protected:
	void ClearProcessed(bool bForceAll)
	{
		//_ASSERTE(isMainThread());

		for (INT_PTR i = m_Processed.size(); i--;)
		{
			bool bTerm = false;
			HANDLE hProcess = m_Processed[i].hProcess;
			HWND hWnd = m_Processed[i].hWnd;

			if (bForceAll)
			{
				bTerm = true;
			}
			else if (hWnd)
			{
				if (!IsWindow(hWnd))
				{
					bTerm = true;
				}
			}
			else if (hProcess)
			{
				if (!bTerm)
				{
					DWORD nWait = WaitForSingleObject(hProcess, 0);
					if (nWait == WAIT_OBJECT_0)
					{
						bTerm = true;
					}
				}
			}

			if (bTerm)
			{
				if (hProcess)
					CloseHandle(hProcess);
				m_Processed.erase(i);
			}
		}

		if (mh_LastWnd && (bForceAll || !IsWindow(mh_LastWnd)))
		{
			if (mh_LastIgnoredWnd == mh_LastWnd)
				mh_LastIgnoredWnd = NULL;
			mh_LastWnd = NULL;
		}
		if (mh_LastIgnoredWnd && (bForceAll || !IsWindow(mh_LastIgnoredWnd)))
		{
			mh_LastIgnoredWnd = NULL;
		}
	};

protected:
	void ClearThreads(bool bForceTerminate)
	{
		_ASSERTE((!bForceTerminate || mb_Termination) && "Termination state is expected here!");

		for (INT_PTR i = m_Threads.size(); i--;)
		{
			const ThreadHandle& th = m_Threads[i];
			HANDLE hThread = th.hThread;
			// mb_Termination: The thread may be was not properly started yet, what for a while...
			if (WaitForSingleObject(hThread, bForceTerminate ? 2500 : 0) != WAIT_OBJECT_0)
			{
				if (bForceTerminate)
				{
					#ifdef _DEBUG
					SuspendThread(hThread);
					_ASSERTE(FALSE && "Terminating DefTermBase hooker thread");
					ResumeThread(hThread); // superfluous?
					#endif
					apiTerminateThread(hThread, 100);
				}
				else
				{
					continue; // Эта нить еще не закончила
				}
			}
			SafeCloseHandle(hThread);

			m_Threads.erase(i);
		}
	};

protected:
	static DWORD WINAPI PostCreatedThread(LPVOID lpParameter)
	{
		CDefTermBase *pTerm = (CDefTermBase*)lpParameter;

		if (pTerm->mb_ConEmuGui || pTerm->m_Opt.bAgressive)
		{
			// Проверит Shell (Taskbar) и активное окно (GetForegroundWindow)
			pTerm->CheckShellWindow();
		}

		pTerm->PostCreateThreadFinished();

		// Done
		pTerm->mb_PostCreatedThread = false;

		return 0;
	};

protected:
	// Чтобы избежать возможного зависания ConEmu.exe в процессе установки
	// хука в процесс шелла (explorer.exe) при старте ConEmu и загрузке ConEmuHk
	static DWORD WINAPI PostCheckThread(LPVOID lpParameter)
	{
		bool bRc = false;
		ThreadArg* pArg = (ThreadArg*)lpParameter;
		if (pArg && !pArg->pTerm->mb_Termination)
		{
			bRc = pArg->pTerm->CheckForeground(pArg->hFore, pArg->nForePID, false, !pArg->pTerm->mb_ExplorerHookAllowed);
			free(pArg);
		}
		return bRc;
	};

protected:
	// exe-шник в списке обрабатываемых?
	bool IsAppMonitored(LPCWSTR szExeFile)
	{
		bool bMonitored = false;
		LPCWSTR pszMonitored = m_Opt.pszzHookedApps;

		if (pszMonitored)
		{
			// All strings are lower case
			const wchar_t* psz = pszMonitored;
			while (*psz)
			{
				if (lstrcmpi(psz, szExeFile) == 0)
				{
					bMonitored = true;
					break;
				}
				psz += lstrlen(psz)+1;
			}
		}

		return bMonitored;
	};

protected:
	// Поставить хук в процесс шелла (explorer.exe)
	bool CheckShellWindow()
	{
		if (mb_Termination)
			return false;

		bool bHooked = false;
		HWND hFore = GetForegroundWindow();
		HWND hDesktop = GetDesktopWindow(); //csrss.exe on Windows 8
		HWND hShell = GetShellWindow();
		HWND hTrayWnd = FindWindowEx(NULL, NULL, L"Shell_TrayWnd", NULL);
		DWORD nDesktopPID = 0, nShellPID = 0, nTrayPID = 0, nForePID = 0;

		if (!bHooked && hShell)
		{
			if (GetWindowThreadProcessId(hShell, &nShellPID) && nShellPID)
			{
				bHooked = CheckForeground(hShell, nShellPID, false, !mb_ExplorerHookAllowed);
			}
		}

		if (!bHooked && hTrayWnd)
		{
			if (GetWindowThreadProcessId(hTrayWnd, &nTrayPID) && nTrayPID
				&& (nTrayPID != nShellPID))
			{
				bHooked = CheckForeground(hTrayWnd, nTrayPID, false, !mb_ExplorerHookAllowed);
			}
		}

		if (!bHooked && hDesktop)
		{
			if (GetWindowThreadProcessId(hDesktop, &nDesktopPID) && nDesktopPID
				&& (nDesktopPID != nTrayPID) && (nDesktopPID != nShellPID))
			{
				bHooked = CheckForeground(hDesktop, nDesktopPID, false, !mb_ExplorerHookAllowed);
			}
		}

		// Поскольку это выполняется на старте, то ConEmu могли запустить специально
		// для установки перехвата терминала. Поэтому нужно проверить и ForegroundWindow!
		if (hFore)
		{
			if (GetWindowThreadProcessId(hFore, &nForePID)
				&& (nForePID != GetCurrentProcessId())
				&& (nForePID != nShellPID) && (nForePID != nDesktopPID) && (nForePID != nTrayPID))
			{
				CheckForeground(hFore, nForePID, false);
			}
		}

		return bHooked;
	};
};
