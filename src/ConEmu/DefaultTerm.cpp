
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

#define HIDE_USE_EXCEPTION_INFO

#include "Header.h"
#include <Tlhelp32.h>
#include "DefaultTerm.h"
#include "ConEmu.h"
#include "ConEmuStart.h"
#include "Options.h"
#include "OptionsClass.h"
#include "Status.h"
#include "TrayIcon.h"
#include "../ConEmuCD/ExitCodes.h"
#include "../common/WRegistry.h"

#define StartupValueName L"ConEmuDefaultTerminal"

CDefaultTerminal::CDefaultTerminal()
	: CDefTermBase(true)
{
	mb_ExplorerHookAllowed = true;
}

CDefaultTerminal::~CDefaultTerminal()
{
	StopHookers();
}

bool CDefaultTerminal::isDefaultTerminalAllowed(bool bDontCheckName /*= false*/)
{
	if (gpConEmu->DisableSetDefTerm || !gpSet->isSetDefaultTerminal)
		return false;
	return true;
}

bool CDefaultTerminal::IsRegisteredOsStartup(CEStr* rszData, bool* pbLeaveInTSA)
{
	bool bCurState = false, bLeaveTSA = gpSet->isRegisterOnOsStartupTSA;
	LPCWSTR ValueName = StartupValueName /* L"ConEmuDefaultTerminal" */;
	CEStr lsData;
	int iLen;

	iLen = RegGetStringValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", ValueName, lsData);
	if (iLen > 0)
	{
		bCurState = true;
		bLeaveTSA = (StrStrI(lsData.c_str(L""), L"/Exit") == NULL);
	}

	if (rszData)
		rszData->Set(lsData.c_str());
	if (pbLeaveInTSA)
		*pbLeaveInTSA = bLeaveTSA;
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
	LONG lRc;
	bool bPrevTSA = false;

	// Need to acquire proper startup (config+xml at least) arguments
	CEStr szAddArgs;
	LPCWSTR pszAddArgs = gpConEmu->MakeConEmuStartArgs(szAddArgs);

	// Allocate memory
	CEStr szNeedValue(
		L"\"", gpConEmu->ms_ConEmuExe, L"\" ",
		pszAddArgs, // -config, -loadcfgfile and others...
		L"-SetDefTerm -Detached -MinTSA",
		gpSet->isRegisterOnOsStartupTSA ? NULL : L" -Exit");

	// Compare with current
	CEStr szCurValue;
	if (IsRegisteredOsStartup(&szCurValue, &bPrevTSA) && !szCurValue.IsEmpty())
	{
		bCurState = true;
	}

	if ((bCurState != bNeedState)
		|| (bNeedState && (lstrcmpi(szNeedValue, szCurValue) != 0)))
	{
		if (bNeedState)
		{
			if (0 != (lRc = RegSetStringValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", ValueName, szNeedValue)))
			{
				DisplayLastError(L"Failed to set ConEmuDefaultTerminal value in HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", lRc);
			}
		}
		else
		{
			if (0 != (lRc = RegSetStringValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", ValueName, NULL)))
			{
				DisplayLastError(L"Failed to remove ConEmuDefaultTerminal value from HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", lRc);
			}
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
		_ASSERTE(gpConEmu->mn_StartupFinished == CConEmuMain::ss_VConAreCreated || gpConEmu->mn_StartupFinished == CConEmuMain::ss_CreateQueueReady);
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

CDefTermBase* CDefaultTerminal::GetInterface()
{
	return this;
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
	m_Opt.pszCfgFile = gpConEmu->opt.LoadCfgFile.Exists ? (wchar_t*)gpConEmu->opt.LoadCfgFile.GetStr() : NULL;
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
		msprintf(szInfo, countof(szInfo), L"DefTerm[%u]: Setup", nPID);
		if (sName && *sName)
		{
			wcscat_c(szInfo, L" ");
			int nLen = lstrlen(szInfo);
			lstrcpyn(szInfo+nLen, sName, countof(szInfo)-nLen);
		}
	}

	gpConEmu->mp_Status->SetStatus(szInfo);
	// descendant must return true if status bar was changed
	return true;
}

void CDefaultTerminal::LogHookingStatus(DWORD nForePID, LPCWSTR sMessage)
{
	wchar_t szPID[16];
	CEStr lsLog(L"DefTerm[", _ultow(nForePID, szPID, 10), L"]: ", sMessage);
	gpConEmu->LogString(lsLog);
}

bool CDefaultTerminal::isLogging()
{
	return (gpSet->isLogging() != 0);
}
