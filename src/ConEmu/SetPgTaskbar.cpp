
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

#include "ConEmu.h"
#include "SetPgTaskbar.h"

CSetPgTaskbar::CSetPgTaskbar()
{
}

CSetPgTaskbar::~CSetPgTaskbar()
{
}

LRESULT CSetPgTaskbar::OnInitDialog(HWND hDlg, bool abInitial)
{
	checkDlgButton(hDlg, cbMinToTray, gpSet->mb_MinToTray);
	EnableWindow(GetDlgItem(hDlg, cbMinToTray), !gpConEmu->ForceMinimizeToTray);

	checkDlgButton(hDlg, cbAlwaysShowTrayIcon, gpSet->isAlwaysShowTrayIcon());

	Settings::TabsOnTaskbar tabsOnTaskbar = gpSet->GetRawTabsOnTaskBar();
	checkRadioButton(hDlg, rbTaskbarBtnActive, rbTaskbarBtnHidden,
		(tabsOnTaskbar == Settings::tot_DontShow) ? rbTaskbarBtnHidden :
		(tabsOnTaskbar == Settings::tot_AllTabsWin7) ? rbTaskbarBtnWin7 :
		(tabsOnTaskbar == Settings::tot_AllTabsAllOS) ? rbTaskbarBtnAll
		: rbTaskbarBtnActive);
	checkDlgButton(hDlg, cbTaskbarOverlay, gpSet->isTaskbarOverlay);
	checkDlgButton(hDlg, cbTaskbarProgress, gpSet->isTaskbarProgress);

	//
	checkDlgButton(hDlg, cbCloseConEmuWithLastTab, gpSet->isCloseOnLastTabClose() ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hDlg, cbCloseConEmuOnCrossClicking, gpSet->isCloseOnCrossClick() ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hDlg, cbMinimizeOnLastTabClose, gpSet->isMinOnLastTabClose() ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hDlg, cbHideOnLastTabClose, gpSet->isHideOnLastTabClose() ? BST_CHECKED : BST_UNCHECKED);
	//
	enableDlgItem(hDlg, cbCloseConEmuOnCrossClicking, !gpSet->isCloseOnLastTabClose());
	enableDlgItem(hDlg, cbMinimizeOnLastTabClose, !gpSet->isCloseOnLastTabClose());
	enableDlgItem(hDlg, cbHideOnLastTabClose, !gpSet->isCloseOnLastTabClose() && gpSet->isMinOnLastTabClose());


	checkDlgButton(hDlg, cbMinimizeOnLoseFocus, gpSet->mb_MinimizeOnLoseFocus);
	EnableWindow(GetDlgItem(hDlg, cbMinimizeOnLoseFocus), (gpSet->isQuakeStyle == 0));


	checkRadioButton(hDlg, rbMinByEscAlways, rbMinByEscNever,
		(gpSet->isMultiMinByEsc == Settings::mbe_NoConsoles) ? rbMinByEscEmpty : gpSet->isMultiMinByEsc ? rbMinByEscAlways : rbMinByEscNever);
	checkDlgButton(hDlg, cbMapShiftEscToEsc, gpSet->isMapShiftEscToEsc);
	EnableWindow(GetDlgItem(hDlg, cbMapShiftEscToEsc), (gpSet->isMultiMinByEsc == Settings::mbe_Always));

	checkDlgButton(hDlg, cbCmdTaskbarTasks, gpSet->isStoreTaskbarkTasks);
	checkDlgButton(hDlg, cbCmdTaskbarCommands, gpSet->isStoreTaskbarCommands);
	checkDlgButton(hDlg, cbJumpListAutoUpdate, gpSet->isJumpListAutoUpdate);
	EnableWindow(GetDlgItem(hDlg, cbCmdTaskbarUpdate), (gnOsVer >= 0x601));

	return 0;
}
