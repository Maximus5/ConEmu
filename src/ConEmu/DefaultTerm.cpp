
/*
Copyright (c) 2012-present Maximus5
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
#include "DefaultTerm.h"
#include "ConEmu.h"
#include "ConEmuStart.h"
#include "Inside.h"
#include "Options.h"
#include "OptionsClass.h"
#include "Status.h"
#include "TrayIcon.h"
#include "../common/WRegistry.h"

#define StartupValueName L"ConEmuDefaultTerminal"

CDefaultTerminal::CDefaultTerminal()
	: CDefTermBase(true)
{
	mb_ExplorerHookAllowed = true;
}

CDefaultTerminal::~CDefaultTerminal()
{
	CDefTermBase::StopHookers();
}

bool CDefaultTerminal::isDefaultTerminalAllowed(const bool bDontCheckName /*= false*/)
{
	if (gpConEmu->DisableSetDefTerm || !gpSet->isSetDefaultTerminal)
		return false;
	return true;
}

bool CDefaultTerminal::IsRegisteredOsStartup(CEStr* rszData, bool* pbLeaveInTSA)
{
	bool bCurState = false, bLeaveTSA = gpSet->isRegisterOnOsStartupTSA;
	const auto* valueName = StartupValueName /* L"ConEmuDefaultTerminal" */;
	CEStr lsData;

	const int iLen = RegGetStringValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", valueName, lsData);
	if (iLen > 0)
	{
		bCurState = true;
		bLeaveTSA = (StrStrI(lsData.c_str(L""), L"/Exit") == nullptr);
	}

	if (rszData)
		rszData->Set(lsData.c_str());
	if (pbLeaveInTSA)
		*pbLeaveInTSA = bLeaveTSA;
	return bCurState;
}

void CDefaultTerminal::ApplyAndSave(const bool bApply, const bool bSaveToReg)
{
	// Get new values from gpSet
	if (bApply)
		ReloadSettings();

	// And save to [HKCU\Software\ConEmu]
	if (bSaveToReg)
		m_Opt.Serialize(CEDefTermOpt::SerializeMode::Save);
}

void CDefaultTerminal::CheckRegisterOsStartup()
{
	const auto* ValueName = StartupValueName;
	bool bCurState = false;
	const bool bNeedState = gpSet->isSetDefaultTerminal && gpSet->isRegisterOnOsStartup;
	LONG lRc;
	bool bPrevTSA = false;

	// Need to acquire proper startup (config+xml at least) arguments
	CEStr szAddArgs;
	const auto* pszAddArgs = gpConEmu->MakeConEmuStartArgs(szAddArgs);

	// Allocate memory
	const CEStr szNeedValue(
		L"\"", gpConEmu->ms_ConEmuExe, L"\" ",
		pszAddArgs, // -config, -loadcfgfile and others...
		L"-SetDefTerm -Detached -MinTSA",
		gpSet->isRegisterOnOsStartupTSA ? nullptr : L" -Exit");

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
			if (0 != (lRc = RegSetStringValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", ValueName, nullptr)))
			{
				DisplayLastError(L"Failed to remove ConEmuDefaultTerminal value from HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", lRc);
			}
		}
	}
}

void CDefaultTerminal::StartGuiDefTerm(const bool bManual, const bool bNoThreading /*= false*/)
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
		_ASSERTE(gpConEmu->GetStartupStage() == CConEmuMain::ss_VConAreCreated || gpConEmu->GetStartupStage() == CConEmuMain::ss_CreateQueueReady);
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
	m_Opt.Serialize(CEDefTermOpt::SerializeMode::Save);
}

void CDefaultTerminal::PostCreateThreadFinished()
{
}

CDefTermBase* CDefaultTerminal::GetInterface()
{
	return this;
}

int CDefaultTerminal::DisplayLastError(LPCWSTR asLabel, const DWORD dwError/*=0*/, const DWORD dwMsgFlags/*=0*/, LPCWSTR asTitle/*=nullptr*/, HWND hParent/*=nullptr*/)
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

	m_Opt.bAggressive = gpSet->isRegisterAggressive;
	m_Opt.bNoInjects = gpSet->isDefaultTerminalNoInjects;
	m_Opt.bNewWindow = gpSet->isDefaultTerminalNewWindow;
	m_Opt.bDebugLog  = gpSet->isDefaultTerminalDebugLog;
	m_Opt.nDefaultTerminalConfirmClose = gpSet->nDefaultTerminalConfirmClose;

	const auto& guiInfo = gpConEmu->GetGuiInfo();
	m_Opt.nConsoleFlags = guiInfo.Flags;

	m_Opt.bExternalPointers = true;
	// in "external pointers" mode m_Opt does not release memory
	m_Opt.pszConEmuExe = gpConEmu->ms_ConEmuExe;
	m_Opt.pszConEmuBaseDir = gpConEmu->ms_ConEmuBaseDir;
	m_Opt.pszCfgFile = gpConEmu->opt.LoadCfgFile.Exists ? const_cast<wchar_t*>(gpConEmu->opt.LoadCfgFile.GetStr()) : nullptr;
	m_Opt.pszConfigName = const_cast<wchar_t*>(gpSetCls->GetConfigName());
	m_Opt.pszzHookedApps = const_cast<wchar_t*>(gpSet->GetDefaultTerminalAppsMSZ()); // ASCIIZZ

	if (gpConEmu->mp_Inside)
	{
		gpConEmu->mp_Inside->UpdateDefTermMapping();
	}
}

void CDefaultTerminal::AutoClearThreads()
{
	if (!isMainThread())
		return;

	// Clear finished threads
	ClearThreads(false);
}

void CDefaultTerminal::ConhostLocker(const bool bLock, bool& bWasLocked)
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

bool CDefaultTerminal::NotifyHookingStatus(const DWORD processId, LPCWSTR sName)
{
	wchar_t szInfo[200] = L"";

	if (processId)
	{
		msprintf(szInfo, countof(szInfo), L"DefTerm[%u]: Setup", processId);
		if (sName && *sName)
		{
			wcscat_c(szInfo, L" ");
			const int nLen = lstrlen(szInfo);
			lstrcpyn(szInfo + nLen, sName, countof(szInfo) - nLen);
		}
	}

	gpConEmu->mp_Status->SetStatus(szInfo);
	// descendant must return true if status bar was changed
	return true;
}

bool CDefaultTerminal::IsAppAllowed(HWND hFore, DWORD processId)
{
	return false;
}

void CDefaultTerminal::LogHookingStatus(const DWORD nForePID, LPCWSTR sMessage)
{
	wchar_t szPID[16] = L"";
	const CEStr lsLog(L"DefTerm[", ultow_s(nForePID, szPID, 10), L"]: ", sMessage);
	gpConEmu->LogString(lsLog);
}

bool CDefaultTerminal::isLogging()
{
	return (gpSet->isLogging() != 0);
}
