
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

#define SHOWDEBUGSTR

#define DEBUGSTRDEFTERM(s) DEBUGSTR(s)

#include "../common/DefTermBase.h"

class CDefaultTerminal : public CDefTermBase
{
public:
	CDefaultTerminal();
	virtual ~CDefaultTerminal();

	void StartGuiDefTerm(bool bManual, bool bNoThreading = false);
	void OnTaskbarCreated();

	void CheckRegisterOsStartup();
	void ApplyAndSave(bool bApply, bool bSaveToReg);
	bool IsRegisteredOsStartup(CEStr* rszData, bool* pbLeaveInTSA);

	virtual bool isDefaultTerminalAllowed(bool bDontCheckName = false) override; // !(gpConEmu->DisableSetDefTerm || !gpSet->isSetDefaultTerminal)

protected:
	virtual CDefTermBase* GetInterface() override;
	virtual int  DisplayLastError(LPCWSTR asLabel, DWORD dwError=0, DWORD dwMsgFlags=0, LPCWSTR asTitle=NULL, HWND hParent=NULL) override;
	virtual void ShowTrayIconError(LPCWSTR asErrText) override; // Icon.ShowTrayIcon(asErrText, tsa_Default_Term);
	virtual void ReloadSettings() override; // Copy from gpSet or load from [HKCU]
	virtual void PreCreateThread() override;
	virtual void PostCreateThreadFinished() override;
	virtual void AutoClearThreads() override;
	virtual void ConhostLocker(bool bLock, bool& bWasLocked) override;
	// nPID = 0 when hooking is done (remove status bar notification)
	// sName is executable name or window class name
	virtual bool NotifyHookingStatus(DWORD nPID, LPCWSTR sName) override;
public:
	virtual void LogHookingStatus(DWORD nForePID, LPCWSTR sMessage) override;
	virtual bool isLogging() override;
};
