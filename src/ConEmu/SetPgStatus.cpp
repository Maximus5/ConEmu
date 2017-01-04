
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
#include "OptionsClass.h"
#include "SetDlgColors.h"
#include "SetDlgLists.h"
#include "SetDlgFonts.h"
#include "SetpgFonts.h"
#include "SetPgStatus.h"
#include "Status.h"

CSetPgStatus::CSetPgStatus()
{
}

CSetPgStatus::~CSetPgStatus()
{
}

LRESULT CSetPgStatus::OnInitDialog(HWND hDlg, bool abInitial)
{
	SetDlgItemText(hDlg, tStatusFontFace, gpSet->sStatusFontFace);

	if (CSetDlgFonts::EnumFontsFinished())  // Если шрифты уже считаны
	{
		CSetPgFonts* pFonts = NULL;
		if (gpSetCls->GetPageObj(pFonts))
		{
			pFonts->CopyFontsTo(hDlg, tStatusFontFace, 0); // можно скопировать список с вкладки [thi_Fonts]
		}
	}

	UINT nVal = gpSet->nStatusFontHeight;
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, tStatusFontHeight), CSetDlgLists::eFSizesSmall, nVal, true);

	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, tStatusFontCharset), CSetDlgLists::eCharSets, gpSet->nStatusFontCharSet, false);

	// Colors
	for (uint c = c35; c <= c37; c++)
		CSetDlgColors::ColorSetEdit(hDlg, c);

	checkDlgButton(hDlg, cbStatusVertSep, (gpSet->isStatusBarFlags & csf_VertDelim) ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hDlg, cbStatusHorzSep, (gpSet->isStatusBarFlags & csf_HorzDelim) ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hDlg, cbStatusVertPad, (gpSet->isStatusBarFlags & csf_NoVerticalPad)==0 ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hDlg, cbStatusSystemColors, (gpSet->isStatusBarFlags & csf_SystemColors) ? BST_CHECKED : BST_UNCHECKED);

	CSetDlgLists::EnableDlgItems(hDlg, CSetDlgLists::eStatusColorIds, !(gpSet->isStatusBarFlags & csf_SystemColors));

	checkDlgButton(hDlg, cbShowStatusBar, gpSet->isStatusBarShow);

	//for (size_t i = 0; i < countof(SettingsNS::StatusItems); i++)
	//{
	//	checkDlgButton(hDlg, SettingsNS::StatusItems[i].nDlgID, !gpSet->isStatusColumnHidden[SettingsNS::StatusItems[i].stItem]);
	//}

	UpdateStatusItems(hDlg);

	return 0;
}

void CSetPgStatus::UpdateStatusItems(HWND hDlg)
{
	HWND hAvail = GetDlgItem(hDlg, lbStatusAvailable); _ASSERTE(hAvail!=NULL);
	INT_PTR iMaxAvail = -1, iCurAvail = SendMessage(hAvail, LB_GETCURSEL, 0, 0);
	DEBUGTEST(INT_PTR iCountAvail = SendMessage(hAvail, LB_GETCOUNT, 0, 0));
	HWND hSeltd = GetDlgItem(hDlg, lbStatusSelected); _ASSERTE(hSeltd!=NULL);
	INT_PTR iMaxSeltd = -1, iCurSeltd = SendMessage(hSeltd, LB_GETCURSEL, 0, 0);
	DEBUGTEST(INT_PTR iCountSeltd = SendMessage(hSeltd, LB_GETCOUNT, 0, 0));

	SendMessage(hAvail, LB_RESETCONTENT, 0, 0);
	SendMessage(hSeltd, LB_RESETCONTENT, 0, 0);

	StatusColInfo* pColumns = NULL;
	size_t nCount = CStatus::GetAllStatusCols(&pColumns);
	_ASSERTE(pColumns!=NULL);

	for (size_t i = 0; i < nCount; i++)
	{
		CEStatusItems nID = pColumns[i].nID;
		if ((nID == csi_Info) || (pColumns[i].sSettingName == NULL))
			continue;

		if (gpSet->isStatusColumnHidden[nID])
		{
			iMaxAvail = SendMessage(hAvail, LB_ADDSTRING, 0, (LPARAM)pColumns[i].sName);
			if (iMaxAvail >= 0)
				SendMessage(hAvail, LB_SETITEMDATA, iMaxAvail, nID);
		}
		else
		{
			iMaxSeltd = SendMessage(hSeltd, LB_ADDSTRING, 0, (LPARAM)pColumns[i].sName);
			if (iMaxSeltd >= 0)
				SendMessage(hSeltd, LB_SETITEMDATA, iMaxSeltd, nID);
		}
	}

	if (iCurAvail >= 0 && iMaxAvail >= 0)
		SendMessage(hAvail, LB_SETCURSEL, (iCurAvail <= iMaxAvail) ? iCurAvail : iMaxAvail, 0);
	if (iCurSeltd >= 0 && iMaxSeltd >= 0)
		SendMessage(hSeltd, LB_SETCURSEL, (iCurSeltd <= iMaxSeltd) ? iCurSeltd : iMaxSeltd, 0);
}

INT_PTR CSetPgStatus::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	switch (nCtrlId)
	{
	case tStatusFontFace:
	case tStatusFontHeight:
	case tStatusFontCharset:
	{
		if (code == CBN_EDITCHANGE)
		{
			switch (nCtrlId)
			{
			case tStatusFontFace:
				GetDlgItemText(hDlg, nCtrlId, gpSet->sStatusFontFace, countof(gpSet->sStatusFontFace)); break;
			case tStatusFontHeight:
				gpSet->nStatusFontHeight = GetNumber(hDlg, nCtrlId); break;
			}
		}
		else if (code == CBN_SELCHANGE)
		{
			UINT val;
			INT_PTR nSel = SendDlgItemMessage(hDlg, nCtrlId, CB_GETCURSEL, 0, 0);

			switch (nCtrlId)
			{
			case tStatusFontFace:
				SendDlgItemMessage(hDlg, nCtrlId, CB_GETLBTEXT, nSel, (LPARAM)gpSet->sStatusFontFace);
				break;
			case tStatusFontHeight:
				if (CSetDlgLists::GetListBoxItem(hDlg, nCtrlId, CSetDlgLists::eFSizesSmall, val))
					gpSet->nStatusFontHeight = val;
				break;
			case tStatusFontCharset:
				if (CSetDlgLists::GetListBoxItem(hDlg, nCtrlId, CSetDlgLists::eCharSets, val))
					gpSet->nStatusFontCharSet = val;
				else
					gpSet->nStatusFontCharSet = DEFAULT_CHARSET;
			}
		}
		gpConEmu->RecreateControls(false, true, true);
		break;
	} // tStatusFontFace, tStatusFontHeight, tStatusFontCharset

	case lbStatusAvailable:
	case lbStatusSelected:
		break;

	default:
		_ASSERTE(FALSE && "ListBox was not processed");
	}

	return 0;
}

LRESULT CSetPgStatus::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	COLORREF color = 0;

	if ((nCtrlId >= tc35) && (nCtrlId <= tc37)
		&& CSetDlgColors::GetColorById(nCtrlId - (tc0-c0), &color))
	{
		if (CSetDlgColors::GetColorRef(hDlg, nCtrlId, &color))
		{
			if (CSetDlgColors::SetColorById(nCtrlId - (tc0-c0), color))
			{
				InvalidateCtrl(GetDlgItem(hDlg, nCtrlId - (tc0-c0)), TRUE);
			}
		}
	}
	else
	{
		_ASSERTE(FALSE && "EditBox was not processed");
	}

	return 0;
}
