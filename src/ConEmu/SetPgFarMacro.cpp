
/*
Copyright (c) 2016-present Maximus5
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

#include "../ConEmuPlugin/FarDefaultMacros.h"
#include "SetDlgLists.h"
#include "SetPgFarMacro.h"

CSetPgFarMacro::CSetPgFarMacro()
{
}

CSetPgFarMacro::~CSetPgFarMacro()
{
}

LRESULT CSetPgFarMacro::OnInitDialog(HWND hDlg, bool abInitial)
{
	_ASSERTE(gpSet->isRClickSendKey==0 || gpSet->isRClickSendKey==1 || gpSet->isRClickSendKey==2);

	checkDlgButton(hDlg, cbRClick, gpSet->isRClickSendKey);
	checkDlgButton(hDlg, cbSafeFarClose, gpSet->isSafeFarClose);

	struct MacroItem {
		int nDlgItem;
		LPCWSTR pszEditor;
		LPCWSTR pszVariants[6];
	} Macros[] = {
		{tRClickMacro, gpSet->RClickMacro(fmv_Default),
			{FarRClickMacroDefault2, FarRClickMacroDefault3, NULL}},
		{tSafeFarCloseMacro, gpSet->SafeFarCloseMacro(fmv_Default),
		{FarSafeCloseMacroDefault2, FarSafeCloseMacroDefault3, FarSafeCloseMacroDefaultD2, FarSafeCloseMacroDefaultD3, L"#Close(0)", NULL}},
		{tCloseTabMacro, gpSet->TabCloseMacro(fmv_Default),
			{FarTabCloseMacroDefault2, FarTabCloseMacroDefault3, NULL}},
		{tSaveAllMacro, gpSet->SaveAllMacro(fmv_Default),
			{FarSaveAllMacroDefault2, FarSaveAllMacroDefault3, NULL}},
		{0}
	};

	for (MacroItem* p = Macros; p->nDlgItem; p++)
	{
		HWND hCombo = GetDlgItem(hDlg, p->nDlgItem);

		CSetDlgLists::FillCBList(hCombo, abInitial, p->pszVariants, p->pszEditor);
	}

	return 0;
}

INT_PTR CSetPgFarMacro::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	switch (nCtrlId)
	{
	case tRClickMacro:
	case tSafeFarCloseMacro:
	case tCloseTabMacro:
	case tSaveAllMacro:
	{
		if ((code == CBN_EDITCHANGE) || (code == CBN_SELCHANGE))
		{
			wchar_t** ppszMacro = NULL;
			LPCWSTR pszDefaultMacro = NULL;
			switch (nCtrlId)
			{
			case tRClickMacro:
				ppszMacro = &gpSet->sRClickMacro; pszDefaultMacro = gpSet->RClickMacroDefault(fmv_Default);
				break;
			case tSafeFarCloseMacro:
				ppszMacro = &gpSet->sSafeFarCloseMacro; pszDefaultMacro = gpSet->SafeFarCloseMacroDefault(fmv_Default);
				break;
			case tCloseTabMacro:
				ppszMacro = &gpSet->sTabCloseMacro; pszDefaultMacro = gpSet->TabCloseMacroDefault(fmv_Default);
				break;
			case tSaveAllMacro:
				ppszMacro = &gpSet->sSaveAllMacro; pszDefaultMacro = gpSet->SaveAllMacroDefault(fmv_Default);
				break;
			default:
				_ASSERTE(FALSE && "ComboBox was not processed!");
				return 0;
			}

			GetString(hDlg, nCtrlId, ppszMacro, pszDefaultMacro, false);
		}

		break;
	} // case tRClickMacro, tSafeFarCloseMacro, tCloseTabMacro, tSaveAllMacro

	default:
		_ASSERTE(FALSE && "ComboBox was not processed!");
		return 0;
	}

	return 0;
}
