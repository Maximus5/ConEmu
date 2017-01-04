
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

#include "OptionsClass.h"
#include "SetPgKeyboard.h"
#include "SetDlgLists.h"

CSetPgKeyboard::CSetPgKeyboard()
{
}

CSetPgKeyboard::~CSetPgKeyboard()
{
}

LRESULT CSetPgKeyboard::OnInitDialog(HWND hDlg, bool abInitial)
{
	checkDlgButton(hDlg, cbInstallKeybHooks,
	               (gpSet->m_isKeyboardHooks == 1) ? BST_CHECKED :
	               ((gpSet->m_isKeyboardHooks == 0) ? BST_INDETERMINATE : BST_UNCHECKED));

	setHotkeyCheckbox(hDlg, cbUseWinNumber, vkConsole_1, L"+1", L"+Numbers", gpSet->isUseWinNumber);
	setHotkeyCheckbox(hDlg, cbUseWinArrows, vkWinLeft, L"+Left", L"+Arrows", gpSet->isUseWinArrows);

	checkDlgButton(hDlg, cbUseWinTab, gpSet->isUseWinTab);

	checkDlgButton(hDlg, cbUseAltGrayPlus, gpSet->isUseAltGrayPlus);

	checkDlgButton(hDlg, cbSendAltTab, gpSet->isSendAltTab);
	checkDlgButton(hDlg, cbSendAltEsc, gpSet->isSendAltEsc);
	checkDlgButton(hDlg, cbSendAltPrintScrn, gpSet->isSendAltPrintScrn);
	checkDlgButton(hDlg, cbSendPrintScrn, gpSet->isSendPrintScrn);
	checkDlgButton(hDlg, cbSendCtrlEsc, gpSet->isSendCtrlEsc);

	checkDlgButton(hDlg, cbFixAltOnAltTab, gpSet->isFixAltOnAltTab);

	// Ctrl+BS - del left word
	setHotkeyCheckbox(hDlg, cbCTSDeleteLeftWord, vkDeleteLeftWord, NULL, NULL, gpSet->AppStd.isCTSDeleteLeftWord);

	// !!!
	gpSetCls->CheckSelectionModifiers(hDlg);

	return 0;
}
