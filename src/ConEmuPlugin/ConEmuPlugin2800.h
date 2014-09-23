
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

#include <windows.h>
#include "../common/common.hpp"
#include "ConEmuPluginBase.h"

class CPluginW2800 : public CPluginBase
{
protected:
	
public:
	CPluginW2800() {};
	virtual ~CPluginW2800() {};

public:
	virtual BOOL    CheckBufferEnabled() override;
	virtual BOOL    ExecuteSynchro() override;
	virtual void    ExitFAR(void) override;
	virtual void    FillUpdateBackground(struct PaintBackgroundArg* pFar) override;
	virtual int     GetActiveWindowType() override;
	virtual DWORD   GetEditorModifiedState() override;
	virtual int     GetMacroArea() override;
	virtual LPCWSTR GetMsg(int aiMsg, wchar_t* psMsg = NULL, size_t cchMsgMax = 0) override;
	virtual LPWSTR  GetPanelDir(GetPanelDirFlags Flags) override;
	virtual void    GetPluginInfo(void* piv) override; // PluginInfo* versioned
	virtual LPCWSTR GetWindowTypeName(int WindowType) override;
	virtual void    GuiMacroDlg() override;
	virtual bool    InputBox(LPCWSTR Title, LPCWSTR SubTitle, LPCWSTR HistoryName, LPCWSTR SrcText, wchar_t*& DestText);
	virtual bool    IsMacroActive() override;
	virtual void    LoadPanelDirs() override;
	#if 0
	virtual bool    LoadPlugin(wchar_t* pszPluginPath) override;
	#endif
	virtual HANDLE  Open(const void* apInfo) override;
	virtual bool    OpenEditor(LPCWSTR asFileName, bool abView, bool abDeleteTempFile, bool abDetectCP = false, int anStartLine = 0, int anStartChar = 1) override;
	virtual void    PostMacroApi(const wchar_t* asMacro, INPUT_RECORD* apRec) override;
	virtual int     ProcessDialogEvent(void* p) override;
	virtual void    ProcessDragFrom() override;
	virtual void    ProcessDragTo() override;
	virtual int     ProcessEditorEvent(int Event, void *Param) override;
	virtual int     ProcessEditorInput(LPCVOID Rec) override;
	virtual int     ProcessSynchroEvent(void* p) override;
	virtual int     ProcessViewerEvent(int Event, void *Param) override;
	virtual void    RedrawAll() override;
	virtual BOOL    ReloadFarInfo() override;
	virtual void    SetStartupInfo(void *aInfo) override;
	virtual void    SetWindow(int nTab) override;
	virtual int     ShowMessage(int aiMsg, int aiButtons) override;
	virtual int     ShowMessage(LPCWSTR asMsg, int aiButtons, bool bWarning) override;
	virtual int     ShowPluginMenu(ConEmuPluginMenuItem* apItems, int Count) override;
	virtual void    ShowUserScreen(bool bUserScreen) override;
	virtual void    StopWaitEndSynchro() override;
	virtual void    WaitEndSynchro() override;
};
