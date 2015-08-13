
/*
Copyright (c) 2012-2015 Maximus5
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

#include "Header.h"
#include <Tlhelp32.h>
#include "DefaultTerm.h"
#include "ConEmu.h"
#include "Options.h"
#include "OptionsClass.h"
#include "Status.h"
#include "TrayIcon.h"
#include "../ConEmuCD/ExitCodes.h"

#define StartupValueName L"ConEmuDefaultTerminal"

CDefaultTerminal::CDefaultTerminal()
	: CDefTermBase(true)
{
	mb_ExplorerHookAllowed = true;
}

CDefaultTerminal::~CDefaultTerminal()
{
}

bool CDefaultTerminal::isDefaultTerminalAllowed(bool bDontCheckName /*= false*/)
{
	if (gpConEmu->DisableSetDefTerm || !gpSet->isSetDefaultTerminal)
		return false;
	return true;
}

bool CDefaultTerminal::IsRegisteredOsStartup(wchar_t* rsValue, DWORD cchMax, bool* pbLeaveInTSA)
{
	LPCWSTR ValueName = StartupValueName /* L"ConEmuDefaultTerminal" */;
	bool bCurState = false, bLeaveTSA = gpSet->isRegisterOnOsStartupTSA;
	bool brsValueAllocated = false;
	HKEY hk;
	DWORD nSize, nType = 0;
	LONG lRc;
	if (0 == (lRc = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hk)))
	{
		if (!rsValue)
		{
			cchMax = MAX_PATH*3;
			rsValue = (wchar_t*)calloc(cchMax+1,sizeof(*rsValue));
			brsValueAllocated = true;
		}

		nSize = cchMax*sizeof(*rsValue);
		if ((0 == (lRc = RegQueryValueEx(hk, ValueName, NULL, &nType, (LPBYTE)rsValue, &nSize)) && (nType == REG_SZ) && (nSize > sizeof(wchar_t))))
		{
			bCurState = true;
			if (rsValue)
			{
				bLeaveTSA = (StrStrI(rsValue, L"/Exit") == NULL);
			}
		}
		RegCloseKey(hk);
	}

	if (pbLeaveInTSA)
		*pbLeaveInTSA = bLeaveTSA;
	if (brsValueAllocated)
		SafeFree(rsValue);
	return bCurState;
}

void CDefaultTerminal::ApplyAndSave(bool bApply, bool bSaveToReg)
{
	// Get new values from gpSet
	if (bApply)
		ReloadSettings();

	// And save to [HKCU\Software\ConEmu]
	if (bSaveToReg)
		m_Opt.Serialize(true);
}

void CDefaultTerminal::CheckRegisterOsStartup()
{
	LPCWSTR ValueName = StartupValueName;
	bool bCurState = false;
	bool bNeedState = gpSet->isSetDefaultTerminal && gpSet->isRegisterOnOsStartup;
	HKEY hk;
	DWORD nSize; //, nType = 0;
	wchar_t szCurValue[MAX_PATH*3] = {};
	wchar_t szNeedValue[MAX_PATH*2+80] = {};
	LONG lRc;
	bool bPrevTSA = false;

	LPCWSTR pszConfigName = gpSetCls->GetConfigName();
	if (pszConfigName && !*pszConfigName)
		pszConfigName = NULL;

	_wsprintf(szNeedValue, SKIPLEN(countof(szNeedValue)) L"\"%s\" %s%s%s/SetDefTerm /Detached /MinTSA%s",
		gpConEmu->ms_ConEmuExe,
		pszConfigName ? L"/Config \"" : L"", pszConfigName ? pszConfigName : L"", pszConfigName ? L"\" " : L"",
		gpSet->isRegisterOnOsStartupTSA ? L"" : L" /Exit");

	if (IsRegisteredOsStartup(szCurValue, countof(szCurValue)-1, &bPrevTSA) && *szCurValue)
	{
		bCurState = true;
	}

	if ((bCurState != bNeedState)
		|| (bNeedState && (lstrcmpi(szNeedValue, szCurValue) != 0)))
	{
		if (0 == (lRc = RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hk, NULL)))
		{
			if (bNeedState)
			{
				nSize = ((DWORD)_tcslen(szNeedValue)+1) * sizeof(szNeedValue[0]);
				if (0 != (lRc = RegSetValueEx(hk, ValueName, 0, REG_SZ, (LPBYTE)szNeedValue, nSize)))
				{
					DisplayLastError(L"Failed to set ConEmuDefaultTerminal value in HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", lRc);
				}
			}
			else
			{
				if (0 != (lRc = RegDeleteValue(hk, ValueName)))
				{
					DisplayLastError(L"Failed to remove ConEmuDefaultTerminal value from HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", lRc);
				}
			}
			RegCloseKey(hk);
		}
		else
		{
			DisplayLastError(L"Failed to open HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run for write", lRc);
		}
	}
}

void CDefaultTerminal::StartGuiDefTerm(bool bManual, bool bNoThreading /*= false*/)
{
	// Do not need to run init procedure, if feature is disabled
	if (!isDefaultTerminalAllowed())
	{
		return;
	}

	// if bManual - this was called from Settings (user interaction) and ApplyAndSave must be called already
	// if !bManual - this was called from ConEmu window startup
	if (!bManual)
	{
		// Refresh settings in the registry
		_ASSERTE(gpConEmu->mn_StartupFinished == gpConEmu->ss_PostCreate2Called);
		ApplyAndSave(true, true);
	}

	// Will call ReloadSettings(), PreCreateThread(), PostCreateThreadFinished()
	Initialize(bManual/*bWaitForReady*/, bManual/*bShowErrors*/, bNoThreading);
}

void CDefaultTerminal::OnTaskbarCreated()
{
	if (!isDefaultTerminalAllowed(true))
		return;

	CheckShellWindow();
}

void CDefaultTerminal::PreCreateThread()
{
	// Write to [HKCU\Software\Microsoft\Windows\CurrentVersion\Run]
	CheckRegisterOsStartup();

	// Write to [HKCU\Software\ConEmu]
	m_Opt.Serialize(true);
}

void CDefaultTerminal::PostCreateThreadFinished()
{
}

int CDefaultTerminal::DisplayLastError(LPCWSTR asLabel, DWORD dwError/*=0*/, DWORD dwMsgFlags/*=0*/, LPCWSTR asTitle/*=NULL*/, HWND hParent/*=NULL*/)
{
	return ::DisplayLastError(asLabel, dwError, dwMsgFlags, asTitle, hParent);
}

void CDefaultTerminal::ShowTrayIconError(LPCWSTR asErrText)
{
	Icon.ShowTrayIcon(asErrText, tsa_Default_Term);
}

void CDefaultTerminal::ReloadSettings()
{
	m_Opt.bUseDefaultTerminal = gpSet->isSetDefaultTerminal;

	m_Opt.bAgressive = gpSet->isRegisterAgressive;
	m_Opt.bNoInjects = gpSet->isDefaultTerminalNoInjects;
	m_Opt.bNewWindow = gpSet->isDefaultTerminalNewWindow;
	m_Opt.bDebugLog  = gpSet->isDefaultTerminalDebugLog;
	m_Opt.nDefaultTerminalConfirmClose = gpSet->nDefaultTerminalConfirmClose;

	ConEmuGuiMapping GuiInfo = {};
	gpConEmu->GetGuiInfo(GuiInfo);
	m_Opt.nConsoleFlags = GuiInfo.Flags;

	m_Opt.bExternalPointers = true;
	m_Opt.pszConEmuExe = gpConEmu->ms_ConEmuExe;
	m_Opt.pszConEmuBaseDir = gpConEmu->ms_ConEmuBaseDir;
	m_Opt.pszConfigName = (wchar_t*)gpSetCls->GetConfigName();
	m_Opt.pszzHookedApps = (wchar_t*)gpSet->GetDefaultTerminalAppsMSZ(); // ASCIIZZ
}

void CDefaultTerminal::AutoClearThreads()
{
	if (!isMainThread())
		return;

	// Clear finished threads
	ClearThreads(false);
}

void CDefaultTerminal::ConhostLocker(bool bLock, bool& bWasLocked)
{
	if (bLock)
	{
		bWasLocked = gpConEmu->LockConhostStart();
	}
	else if (bWasLocked)
	{
		gpConEmu->UnlockConhostStart();
	}
}

// nPID = 0 when hooking is done (remove status bar notification)
// sName is executable name or window class name
bool CDefaultTerminal::NotifyHookingStatus(DWORD nPID, LPCWSTR sName)
{
	wchar_t szInfo[200] = L"";

	if (nPID)
	{
		msprintf(szInfo, countof(szInfo), L"DefTerm setup: PID=%u", nPID);
		if (sName && *sName)
		{
			wcscat_c(szInfo, L", ");
			int nLen = lstrlen(szInfo);
			lstrcpyn(szInfo+nLen, sName, countof(szInfo)-nLen);
		}
	}

	gpConEmu->mp_Status->SetStatus(szInfo);
	// descendant must return true if status bar was changed
	return true;
}

void CDefaultTerminal::LogHookingStatus(LPCWSTR sMessage)
{
	gpConEmu->LogString(sMessage);
}
