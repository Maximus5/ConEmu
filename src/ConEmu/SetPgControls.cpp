
/*
Copyright (c) 2016 Maximus5
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

#include "OptionsClass.h"
#include "SetPgControls.h"
#include "SetDlgLists.h"

CSetPgControls::CSetPgControls()
{
}

CSetPgControls::~CSetPgControls()
{
}

LRESULT CSetPgControls::OnInitDialog(HWND hDlg, bool abInitial)
{
	checkDlgButton(hDlg, cbEnableMouse, !gpSet->isDisableMouse);
	checkDlgButton(hDlg, cbSkipActivation, gpSet->isMouseSkipActivation);
	checkDlgButton(hDlg, cbSkipMove, gpSet->isMouseSkipMoving);
	checkDlgButton(hDlg, cbActivateSplitMouseOver, gpSet->bActivateSplitMouseOver);

	checkDlgButton(hDlg, cbInstallKeybHooks,
	               (gpSet->m_isKeyboardHooks == 1) ? BST_CHECKED :
	               ((gpSet->m_isKeyboardHooks == 0) ? BST_INDETERMINATE : BST_UNCHECKED));

	setHotkeyCheckbox(hDlg, cbUseWinNumber, vkConsole_1, L"+1", L"+Numbers", gpSet->isUseWinNumber);
	setHotkeyCheckbox(hDlg, cbUseWinArrows, vkWinLeft, L"+Left", L"+Arrows", gpSet->isUseWinArrows);

	checkDlgButton(hDlg, cbUseWinTab, gpSet->isUseWinTab);

	checkDlgButton(hDlg, cbSendAltTab, gpSet->isSendAltTab);
	checkDlgButton(hDlg, cbSendAltEsc, gpSet->isSendAltEsc);
	checkDlgButton(hDlg, cbSendAltPrintScrn, gpSet->isSendAltPrintScrn);
	checkDlgButton(hDlg, cbSendPrintScrn, gpSet->isSendPrintScrn);
	checkDlgButton(hDlg, cbSendCtrlEsc, gpSet->isSendCtrlEsc);

	checkDlgButton(hDlg, cbFixAltOnAltTab, gpSet->isFixAltOnAltTab);

	checkDlgButton(hDlg, cbSkipFocusEvents, gpSet->isSkipFocusEvents);

	// Prompt click
	checkDlgButton(hDlg, cbCTSClickPromptPosition, gpSet->AppStd.isCTSClickPromptPosition);
	UINT VkMod = gpSet->GetHotkeyById(vkCTSVkPromptClk);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbCTSClickPromptPosition), CSetDlgLists::eKeysAct, VkMod, false);

	// Ctrl+BS - del left word
	setHotkeyCheckbox(hDlg, cbCTSDeleteLeftWord, vkDeleteLeftWord, NULL, NULL, gpSet->AppStd.isCTSDeleteLeftWord);

	checkDlgButton(hDlg, (gpSet->isCTSActMode==1)?rbCTSActAlways:rbCTSActBufferOnly, BST_CHECKED);
	VkMod = gpSet->GetHotkeyById(vkCTSVkAct);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbCTSActAlways), CSetDlgLists::eKeysAct, VkMod, false);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbCTSRBtnAction), CSetDlgLists::eClipAct, gpSet->isCTSRBtnAction, false);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbCTSMBtnAction), CSetDlgLists::eClipAct, gpSet->isCTSMBtnAction, false);

	gpSetCls->CheckSelectionModifiers(hDlg);

	return 0;
}

INT_PTR CSetPgControls::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	if (code == CBN_SELCHANGE)
	{
		switch (nCtrlId)
		{
		case lbCTSClickPromptPosition:
			{
				BYTE VkMod = 0;
				CSetDlgLists::GetListBoxItem(hDlg, lbCTSClickPromptPosition, CSetDlgLists::eKeysAct, VkMod);
				gpSet->SetHotkeyById(vkCTSVkPromptClk, VkMod);
				CheckSelectionModifiers(hDlg);
			} break;
		case lbCTSActAlways:
			{
				BYTE VkMod = 0;
				CSetDlgLists::GetListBoxItem(hDlg, lbCTSActAlways, CSetDlgLists::eKeysAct, VkMod);
				gpSet->SetHotkeyById(vkCTSVkAct, VkMod);
			} break;
		case lbCTSRBtnAction:
			{
				CSetDlgLists::GetListBoxItem(hDlg, lbCTSRBtnAction, CSetDlgLists::eClipAct, gpSet->isCTSRBtnAction);
			} break;
		case lbCTSMBtnAction:
			{
				CSetDlgLists::GetListBoxItem(hDlg, lbCTSMBtnAction, CSetDlgLists::eClipAct, gpSet->isCTSMBtnAction);
			} break;
		default:
			_ASSERTE(FALSE && "ListBox was not processed");
		}
	} // if (HIWORD(wParam) == CBN_SELCHANGE)

	break;
}
