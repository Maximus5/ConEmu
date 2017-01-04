
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

#include "../common/DefTermBase.h"
#include "../common/CmdLine.h"

class MFileLog;
class CDefTermHk;

extern CDefTermHk* gpDefTerm;

bool InitDefTerm();
bool isDefTermEnabled();
void DefTermLogString(LPCSTR asMessage, LPCWSTR asLabel = NULL);
void DefTermLogString(LPCWSTR asMessage, LPCWSTR asLabel = NULL);

class CDefTermHk : public CDefTermBase
{
public:
	CDefTermHk();
	virtual ~CDefTermHk();

	void StartDefTerm();

	static DWORD WINAPI InitDefTermContinue(LPVOID ahPrevHooks);
	DWORD mn_InitDefTermContinueTID;
	HANDLE mh_InitDefTermContinueFrom;

	// Запустить сервер, и подцепиться к его консоли
	HWND AllocHiddenConsole(bool bTempForVS);
	// Вызывается из хуков после успешного AllocConsole (Win2k only? а смысл?)
	void OnAllocConsoleFinished(HWND hNewConWnd);

	virtual bool isDefaultTerminalAllowed(bool bDontCheckName = false) override; // !(gpConEmu->DisableSetDefTerm || !gpSet->isSetDefaultTerminal)
	virtual void StopHookers() override;
	virtual void ReloadSettings() override; // Copy from gpSet or load from [HKCU]

	size_t GetSrvAddArgs(bool bGuiArgs, CEStr& rsArgs, CEStr& rsNewCon);

protected:
	HANDLE  mh_StopEvent;
	wchar_t ms_ExeName[MAX_PATH];
	DWORD   mn_LastCheck;

	DWORD   StartConsoleServer(DWORD nAttachPID, bool bNewConWnd, PHANDLE phSrvProcess);

protected:
	virtual CDefTermBase* GetInterface() override;
	virtual int  DisplayLastError(LPCWSTR asLabel, DWORD dwError=0, DWORD dwMsgFlags=0, LPCWSTR asTitle=NULL, HWND hParent=NULL) override;
	virtual void ShowTrayIconError(LPCWSTR asErrText) override; // Icon.ShowTrayIcon(asErrText, tsa_Default_Term);
	virtual void PostCreateThreadFinished() override;

protected:
	MFileLog* mp_FileLog;
	void LogInit();
	virtual void LogHookingStatus(DWORD nForePID, LPCWSTR sMessage) override;
protected:
	friend bool InitDefTerm();
	friend void DefTermLogInit();
	friend void DefTermLogString(LPCSTR asMessage, LPCWSTR asLabel /*= NULL*/);
	friend void DefTermLogString(LPCWSTR asMessage, LPCWSTR asLabel /*= NULL*/);
};
