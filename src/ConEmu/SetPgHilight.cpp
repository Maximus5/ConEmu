
/*
Copyright (c) 2016-2017 Maximus5
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
#define SHOWDEBUGSTR

#include "Header.h"

#include "SetDlgLists.h"
#include "SetPgHilight.h"

CSetPgHilight::CSetPgHilight()
{
}

CSetPgHilight::~CSetPgHilight()
{
}

LRESULT CSetPgHilight::OnInitDialog(HWND hDlg, bool abInitial)
{
	// Hyperlinks & compiler errors
	checkDlgButton(hDlg, cbFarGotoEditor, gpSet->isFarGotoEditor);
	UINT VkMod = gpSet->GetHotkeyById(vkFarGotoEditorVk);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbFarGotoEditorVk), CSetDlgLists::eKeysAct, VkMod, false);

	LPCWSTR ppszDefEditors[] = {
		HI_GOTO_EDITOR_FAR,    // Far Manager
		HI_GOTO_EDITOR_SCITE,  // SciTE (Scintilla)
		HI_GOTO_EDITOR_NPADP,  // Notepad++
		HI_GOTO_EDITOR_VIMW,   // Vim, official, using Windows paths
		HI_GOTO_EDITOR_SUBLM,  // Sublime text
		HI_GOTO_EDITOR_CMD,    // Just ‘start’ highlighted file via cmd.exe
		HI_GOTO_EDITOR_CMD_NC, // Just ‘start’ highlighted file via cmd.exe, same as prev. but without close confirmation
		NULL};
	CSetDlgLists::FillCBList(GetDlgItem(hDlg, lbGotoEditorCmd), abInitial, ppszDefEditors, gpSet->sFarGotoEditor);

	// Highlight full row/column under mouse cursor
	checkDlgButton(hDlg, cbHighlightMouseRow, gpSet->isHighlightMouseRow);
	checkDlgButton(hDlg, cbHighlightMouseCol, gpSet->isHighlightMouseCol);

	// This modifier (lbFarGotoEditorVk) is not checked
	// CheckSelectionModifiers(hDlg);

	return 0;
}

INT_PTR CSetPgHilight::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	switch (nCtrlId)
	{
	case lbGotoEditorCmd:
	{
		if ((code == CBN_EDITCHANGE) || (code == CBN_SELCHANGE))
		{
			GetString(hDlg, lbGotoEditorCmd, &gpSet->sFarGotoEditor, NULL, (code == CBN_SELCHANGE));
		}
		break;
	} // lbGotoEditorCmd

	case lbFarGotoEditorVk:
	{
		if (code == CBN_SELCHANGE)
		{
			BYTE VkMod = 0;
			CSetDlgLists::GetListBoxItem(hDlg, lbFarGotoEditorVk, CSetDlgLists::eKeysAct, VkMod);
			gpSet->SetHotkeyById(vkFarGotoEditorVk, VkMod);
		}
		break;
	} // lbFarGotoEditorVk

	default:
		_ASSERTE(FALSE && "ListBox was not processed");
	}

	return 0;
}
