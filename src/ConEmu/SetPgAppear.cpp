
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
#include "SetPgAppear.h"

CSetPgAppear::CSetPgAppear()
{
}

CSetPgAppear::~CSetPgAppear()
{
}

LRESULT CSetPgAppear::OnInitDialog(HWND hDlg, bool abInitial)
{
	checkDlgButton(hDlg, cbHideCaption, gpSet->isHideCaption);

	checkDlgButton(hDlg, cbHideCaptionAlways, gpSet->isHideCaptionAlways());
	EnableWindow(GetDlgItem(hDlg, cbHideCaptionAlways), !gpSet->isForcedHideCaptionAlways());

	// Quake style
	checkDlgButton(hDlg, cbQuakeStyle, gpSet->isQuakeStyle ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hDlg, cbQuakeAutoHide, (gpSet->isQuakeStyle == 2) ? BST_CHECKED : BST_UNCHECKED);
	// Show/Hide/Slide timeout
	SetDlgItemInt(hDlg, tQuakeAnimation, gpSet->nQuakeAnimation, FALSE);

	EnableWindow(GetDlgItem(hDlg, cbQuakeAutoHide), gpSet->isQuakeStyle);

	// Скрытие рамки
	SetDlgItemInt(hDlg, tHideCaptionAlwaysFrame, gpSet->HideCaptionAlwaysFrame(), TRUE);
	SetDlgItemInt(hDlg, tHideCaptionAlwaysDelay, gpSet->nHideCaptionAlwaysDelay, FALSE);
	SetDlgItemInt(hDlg, tHideCaptionAlwaysDissapear, gpSet->nHideCaptionAlwaysDisappear, FALSE);

	// Child GUI applications
	checkDlgButton(hDlg, cbHideChildCaption, gpSet->isHideChildCaption);

	checkDlgButton(hDlg, cbEnhanceGraphics, gpSet->isEnhanceGraphics);

	//checkDlgButton(hDlg, cbEnhanceButtons, gpSet->isEnhanceButtons);

	//checkDlgButton(hDlg, cbAlwaysShowScrollbar, gpSet->isAlwaysShowScrollbar);
	checkRadioButton(hDlg, rbScrollbarHide, rbScrollbarAuto, (gpSet->isAlwaysShowScrollbar==0) ? rbScrollbarHide : (gpSet->isAlwaysShowScrollbar==1) ? rbScrollbarShow : rbScrollbarAuto);
	SetDlgItemInt(hDlg, tScrollAppearDelay, gpSet->nScrollBarAppearDelay, FALSE);
	SetDlgItemInt(hDlg, tScrollDisappearDelay, gpSet->nScrollBarDisappearDelay, FALSE);

	checkDlgButton(hDlg, cbAlwaysOnTop, gpSet->isAlwaysOnTop);

	#ifdef _DEBUG
	checkDlgButton(hDlg, cbTabsInCaption, gpSet->isTabsInCaption);
	#else
	ShowWindow(GetDlgItem(hDlg, cbTabsInCaption), SW_HIDE);
	#endif

	checkDlgButton(hDlg, cbNumberInCaption, gpSet->isNumberInCaption);

	checkDlgButton(hDlg, cbMultiCon, gpSet->mb_isMulti);
	checkDlgButton(hDlg, cbMultiShowButtons, gpSet->isMultiShowButtons);
	checkDlgButton(hDlg, cbMultiShowSearch, gpSet->isMultiShowSearch);

	checkDlgButton(hDlg, cbSingleInstance, gpSetCls->IsSingleInstanceArg());
	enableDlgItem(hDlg, cbSingleInstance, (gpSet->isQuakeStyle == 0));

	checkDlgButton(hDlg, cbShowHelpTooltips, gpSet->isShowHelpTooltips);

	return 0;
}
