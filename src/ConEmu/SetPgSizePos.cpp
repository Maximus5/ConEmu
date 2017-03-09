
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
#include "../common/MSetter.h"

#include "ConEmu.h"
#include "LngRc.h"
#include "OptionsClass.h"
#include "SetDlgLists.h"
#include "SetPgSizePos.h"

CSetPgSizePos::CSetPgSizePos()
{
}

CSetPgSizePos::~CSetPgSizePos()
{
}

// IDD_SPG_SIZEPOS / thi_SizePos
LRESULT CSetPgSizePos::OnInitDialog(HWND hDlg, bool abInitial)
{
	_ASSERTE(gpSetCls->GetPage(thi_SizePos) == hDlg);

	checkDlgButton(hDlg, cbAutoSaveSizePos, gpSet->isAutoSaveSizePos);

	checkDlgButton(hDlg, cbUseCurrentSizePos, gpSet->isUseCurrentSizePos);

	ConEmuWindowMode wMode;
	if (gpSet->isQuakeStyle || !gpSet->isUseCurrentSizePos)
		wMode = (ConEmuWindowMode)gpSet->_WindowMode;
	else if (gpConEmu->isFullScreen())
		wMode = wmFullScreen;
	else if (gpConEmu->isZoomed())
		wMode = wmMaximized;
	else
		wMode = wmNormal;
	checkRadioButton(hDlg, rNormal, rFullScreen,
		(wMode == wmFullScreen) ? rFullScreen
		: (wMode == wmMaximized) ? rMaximized
		: rNormal);

	SendDlgItemMessage(hDlg, tWndWidth, EM_SETLIMITTEXT, 6, 0);
	SendDlgItemMessage(hDlg, tWndHeight, EM_SETLIMITTEXT, 6, 0);

	gpSetCls->UpdateSize(gpConEmu->WndWidth, gpConEmu->WndHeight);

	EnableWindow(GetDlgItem(hDlg, cbApplyPos), FALSE);
	SendDlgItemMessage(hDlg, tWndX, EM_SETLIMITTEXT, 6, 0);
	SendDlgItemMessage(hDlg, tWndY, EM_SETLIMITTEXT, 6, 0);
	EnablePosSizeControls(hDlg);
	MCHKHEAP

	gpSetCls->UpdatePos(gpConEmu->WndPos.x, gpConEmu->WndPos.y, true);

	checkRadioButton(hDlg, rCascade, rFixed, gpSet->wndCascade ? rCascade : rFixed);
	if (!abInitial)
		OnPostLocalize(hDlg);

	checkDlgButton(hDlg, cbLongOutput, gpSet->AutoBufferHeight);
	TODO("Надо бы увеличить, но нужно сервер допиливать");
	SendDlgItemMessage(hDlg, tLongOutputHeight, EM_SETLIMITTEXT, 5, 0);
	SetDlgItemInt(hDlg, tLongOutputHeight, gpSet->DefaultBufferHeight, FALSE);
	//EnableWindow(GetDlgItem(hDlg, tLongOutputHeight), gpSet->AutoBufferHeight);


	// 16bit Height
	if (abInitial)
	{
		SendDlgItemMessage(hDlg, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"Auto");
		SendDlgItemMessage(hDlg, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"25 lines");
		SendDlgItemMessage(hDlg, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"28 lines");
		SendDlgItemMessage(hDlg, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"43 lines");
		SendDlgItemMessage(hDlg, lbNtvdmHeight, CB_ADDSTRING, 0, (LPARAM) L"50 lines");
	}
	SendDlgItemMessage(hDlg, lbNtvdmHeight, CB_SETCURSEL, !gpSet->ntvdmHeight ? 0 :
	                   ((gpSet->ntvdmHeight == 25) ? 1 : ((gpSet->ntvdmHeight == 28) ? 2 : ((gpSet->ntvdmHeight == 43) ? 3 : 4))), 0); //-V112


	checkDlgButton(hDlg, cbTryToCenter, gpSet->isTryToCenter);
	SetDlgItemInt(hDlg, tPadSize, gpSet->nCenterConsolePad, FALSE);

	checkDlgButton(hDlg, cbIntegralSize, !gpSet->mb_IntegralSize);

	checkDlgButton(hDlg, cbRestore2ActiveMonitor, gpSet->isRestore2ActiveMon);

	checkDlgButton(hDlg, cbSnapToDesktopEdges, gpSet->isSnapToDesktopEdges);

	return 0;
}

void CSetPgSizePos::FillSizeControls(HWND hDlg)
{
	{
		MSetter lIgnoreEdit(&mb_IgnoreEditChanged);
		SetDlgItemText(hDlg, tWndWidth, gpSet->isUseCurrentSizePos ? gpConEmu->WndWidth.AsString() : gpSet->wndWidth.AsString());
		SetDlgItemText(hDlg, tWndHeight, gpSet->isUseCurrentSizePos ? gpConEmu->WndHeight.AsString() : gpSet->wndHeight.AsString());
	}

	// Во избежание недоразумений - запретим элементы размера для Max/Fullscreen
	bool bNormalChecked = isChecked2(hDlg, rNormal);
	//for (size_t i = 0; i < countof(SettingsNS::nSizeCtrlId); i++)
	//{
	//	EnableWindow(GetDlgItem(hDlg, SettingsNS::nSizeCtrlId[i]), bNormalChecked);
	//}
	CSetDlgLists::EnableDlgItems(hDlg, CSetDlgLists::eSizeCtrlId, bNormalChecked);
}

void CSetPgSizePos::EnablePosSizeControls(HWND hDlg)
{
	enableDlgItem(hDlg, tWndX, !gpConEmu->mp_Inside && !(gpSet->isQuakeStyle && gpSet->wndCascade));
	enableDlgItem(hDlg, tWndY, !(gpSet->isQuakeStyle || gpConEmu->mp_Inside));
	enableDlgItem(hDlg, tWndWidth, !gpConEmu->mp_Inside);
	enableDlgItem(hDlg, tWndHeight, !gpConEmu->mp_Inside);
}

INT_PTR CSetPgSizePos::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	switch (nCtrlId)
	{
	case lbNtvdmHeight:
	{
		INT_PTR num = SendDlgItemMessage(hDlg, nCtrlId, CB_GETCURSEL, 0, 0);
		gpSet->ntvdmHeight = (num == 1) ? 25 : ((num == 2) ? 28 : ((num == 3) ? 43 : ((num == 4) ? 50 : 0))); //-V112
		break;
	}

	default:
		_ASSERTE(FALSE && "ListBox was not processed");
	}

	return 0;
}

void CSetPgSizePos::OnPostLocalize(HWND hDlg)
{
	CEStr lsRsrc;
	SetDlgItemText(hDlg, rFixed, gpSet->isQuakeStyle
		? CLngRc::getRsrc(lng_SizeRadioFree/*"Free"*/)
		: CLngRc::getControl(rFixed, lsRsrc, L"Fixed"));
	SetDlgItemText(hDlg, rCascade, gpSet->isQuakeStyle
		? CLngRc::getRsrc(lng_SizeRadioCentered/*"Centered"*/)
		: CLngRc::getControl(rCascade, lsRsrc, L"Cascade"));
}

LRESULT CSetPgSizePos::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case tWndX:
	case tWndY:
		if (isChecked(hDlg, rNormal) == BST_CHECKED)
		{
			wchar_t *pVal = GetDlgItemTextPtr(hDlg, nCtrlId);
			bool bValid = (pVal && (isDigit(pVal[0]) || (pVal[0]==L'-' && isDigit(pVal[1]))));
			enableDlgItem(hDlg, cbApplyPos, bValid);
			SafeFree(pVal);
		}
		break; // case tWndX: case tWndY:
	case tWndWidth:
	case tWndHeight:
		if (isChecked(hDlg, rNormal) == BST_CHECKED)
		{
			CESize sz = {0};
			wchar_t *pVal = GetDlgItemTextPtr(hDlg, nCtrlId);
			bool bValid = (pVal && sz.SetFromString(false, pVal));
			enableDlgItem(hDlg, cbApplyPos, bValid);
			SafeFree(pVal);
		}
		break; // case tWndWidth: case tWndHeight:

	case tPadSize:
	{
		BOOL bPadOk = FALSE;
		UINT nNewPad = GetDlgItemInt(hDlg, nCtrlId, &bPadOk, FALSE);

		if (nNewPad >= CENTERCONSOLEPAD_MIN && nNewPad <= CENTERCONSOLEPAD_MAX)
			gpSet->nCenterConsolePad = nNewPad;
		else if (nNewPad > CENTERCONSOLEPAD_MAX)
			SetDlgItemInt(hDlg, nCtrlId, CENTERCONSOLEPAD_MAX, FALSE);
		// Если юзер ставит "бордюр" то нужно сразу включить опцию, чтобы он работал
		if (gpSet->nCenterConsolePad && !isChecked(hDlg, cbTryToCenter))
		{
			gpSet->isTryToCenter = true;
			checkDlgButton(hDlg, cbTryToCenter, BST_CHECKED);
		}
		// Update window/console size
		if (gpSet->isTryToCenter)
			gpConEmu->OnSize();
		break;
	}

	case tLongOutputHeight:
	{
		BOOL lbOk = FALSE;
		wchar_t szTemp[16];
		UINT nNewVal = GetDlgItemInt(hDlg, nCtrlId, &lbOk, FALSE);

		if (lbOk)
		{
			if (nNewVal >= LONGOUTPUTHEIGHT_MIN && nNewVal <= LONGOUTPUTHEIGHT_MAX)
				gpSet->DefaultBufferHeight = nNewVal;
			else if (nNewVal > LONGOUTPUTHEIGHT_MAX)
				SetDlgItemInt(hDlg, nCtrlId, gpSet->DefaultBufferHeight, FALSE);
		}
		else
		{
			SetDlgItemText(hDlg, nCtrlId, _ltow(gpSet->DefaultBufferHeight, szTemp, 10));
		}
		break;
	} //case tLongOutputHeight:

	default:
		_ASSERTE(FALSE && "EditBox was not processed");
	}

	return 0;
}
