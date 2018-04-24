
/*
Copyright (c) 2009-present Maximus5
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

#include "../common/Common.h"
#include "ConEmuPluginBase.h"

class CPluginW2800 : public CPluginBase
{
protected:
	
public:
	CPluginW2800();
	virtual ~CPluginW2800() {};

public:
	virtual bool    CheckPanelExist() override;
	virtual bool    ExecuteSynchroApi() override;
	virtual void    ExitFar() override;
	virtual int     GetActiveWindowType() override;
	virtual DWORD   GetEditorModifiedState() override;
	virtual bool    GetFarRect(SMALL_RECT& rcFar) override;
	virtual int     GetMacroArea() override;
	virtual LPCWSTR GetMsg(int aiMsg, wchar_t* psMsg = NULL, size_t cchMsgMax = 0) override;
	virtual LPWSTR  GetPanelDir(GetPanelDirFlags Flags, wchar_t* pszBuffer = NULL, int cchBufferMax = 0) override;
	virtual bool    GetPanelInfo(GetPanelDirFlags Flags, CEPanelInfo* pBk) override;
	virtual bool    GetPanelItemInfo(const CEPanelInfo& PnlInfo, bool bSelected, INT_PTR iIndex, WIN32_FIND_DATAW& Info, wchar_t** ppszFullPathName) override;
	virtual void    GetPluginInfoPtr(void* piv) override; // PluginInfo* versioned
	virtual int     GetWindowCount() override;
	virtual LPCWSTR GetWindowTypeName(int WindowType) override;
	virtual void    GuiMacroDlg() override;
	virtual bool    InputBox(LPCWSTR Title, LPCWSTR SubTitle, LPCWSTR HistoryName, LPCWSTR SrcText, wchar_t*& DestText);
	virtual bool    IsMacroActive() override;
	virtual void    LoadFarColors(BYTE (&nFarColors)[col_LastIndex]) override;
	virtual void    LoadFarSettings(CEFarInterfaceSettings* pInterface, CEFarPanelSettings* pPanel) override;
	//virtual void    LoadPanelDirs() override;
	//virtual void    LoadPanelTabsSettings() override;
	#if 0
	virtual bool    LoadPlugin(wchar_t* pszPluginPath) override;
	#endif
	virtual HANDLE  Open(const void* apInfo) override;
	virtual bool    OpenEditor(LPCWSTR asFileName, bool abView, bool abDeleteTempFile, bool abDetectCP = false, int anStartLine = 0, int anStartChar = 1) override;
	virtual INT_PTR PanelControlApi(HANDLE hPanel, int Command, INT_PTR Param1, void* Param2);
	virtual void    PostMacroApi(const wchar_t* asMacro, INPUT_RECORD* apRec, bool abShowParseErrors) override;
	virtual int     ProcessEditorEventPtr(void* p) override;
	virtual int     ProcessEditorInputPtr(LPCVOID Rec) override;
	virtual int     ProcessSynchroEventPtr(void* p) override;
	virtual int     ProcessViewerEventPtr(void* p) override;
	virtual void    RedrawAll() override;
	virtual void    SetStartupInfoPtr(void *aInfo) override;
	virtual void    SetWindow(int nTab) override;
	virtual int     ShowMessage(LPCWSTR asMsg, int aiButtons, bool bWarning) override;
	virtual int     ShowPluginMenu(ConEmuPluginMenuItem* apItems, int Count, int TitleMsgId = CEPluginName) override;
	virtual void    ShowUserScreen(bool bUserScreen) override;
	virtual void    StopWaitEndSynchro() override;
	virtual bool    UpdateConEmuTabsApi(int windowCount) override;
	virtual void    WaitEndSynchro() override;
};
