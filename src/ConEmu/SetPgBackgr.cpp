
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

#include "ConEmu.h"
#include "OptionsClass.h"
#include "SetDlgColors.h"
#include "SetDlgLists.h"
#include "SetPgBackgr.h"

CSetPgBackgr::CSetPgBackgr()
{
}

CSetPgBackgr::~CSetPgBackgr()
{
}

LRESULT CSetPgBackgr::OnInitDialog(HWND hDlg, bool abInitial)
{
	TCHAR tmp[255];

	SetDlgItemText(hDlg, tBgImage, gpSet->sBgImage);
	//checkDlgButton(hDlg, rBgSimple, BST_CHECKED);

	CSetDlgColors::FillBgImageColors(hDlg);

	checkDlgButton(hDlg, cbBgImage, BST(gpSet->isShowBgImage));

	_wsprintf(tmp, SKIPLEN(countof(tmp)) L"%i", gpSet->bgImageDarker);
	SendDlgItemMessage(hDlg, tDarker, EM_SETLIMITTEXT, 3, 0);
	SetDlgItemText(hDlg, tDarker, tmp);
	SendDlgItemMessage(hDlg, slDarker, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(0, 255));
	SendDlgItemMessage(hDlg, slDarker, TBM_SETPOS  , (WPARAM) true, (LPARAM) gpSet->bgImageDarker);

	//checkDlgButton(hDlg, rBgUpLeft+(UINT)gpSet->bgOperation, BST_CHECKED);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbBgPlacement), CSetDlgLists::eBgOper, gpSet->bgOperation, false);

	checkDlgButton(hDlg, cbBgAllowPlugin, BST(gpSet->isBgPluginAllowed));

	CSetDlgLists::EnableDlgItems(hDlg, CSetDlgLists::eImgCtrls, gpSet->isShowBgImage);

	return 0;
}

INT_PTR CSetPgBackgr::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	switch (nCtrlId)
	{
	case lbBgPlacement:
	{
		BYTE bg = 0;
		CSetDlgLists::GetListBoxItem(hDlg, lbBgPlacement, CSetDlgLists::eBgOper, bg);
		gpSet->bgOperation = bg;
		gpSetCls->LoadBackgroundFile(gpSet->sBgImage, true);
		gpSetCls->NeedBackgroundUpdate();
		gpConEmu->Update(true);
		break;
	}

	default:
		_ASSERTE(FALSE && "ListBox was not processed");
	}

	return 0;
}
