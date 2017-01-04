
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

	CSetDlgLists::EnableDlgItems(hDlg, CSetDlgLists::eImgCtrls, (gpSet->isShowBgImage != 0));

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

LRESULT CSetPgBackgr::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case tBgImage:
	{
		wchar_t temp[MAX_PATH];
		GetDlgItemText(hDlg, tBgImage, temp, countof(temp));

		if (wcscmp(temp, gpSet->sBgImage))
		{
			if (gpSetCls->LoadBackgroundFile(temp, true))
			{
				wcscpy_c(gpSet->sBgImage, temp);
				gpSetCls->NeedBackgroundUpdate();
				gpConEmu->Update(true);
			}
		}
		break;
	} // case tBgImage:

	case tBgImageColors:
	{
		wchar_t temp[128] = {0};
		GetDlgItemText(hDlg, tBgImageColors, temp, countof(temp)-1);
		DWORD newBgColors = 0;

		for (wchar_t* pc = temp; *pc; pc++)
		{
			if (*pc == L'*')
			{
				newBgColors = (DWORD)-1;
				break;
			}

			if (*pc == L'#')
			{
				if (isDigit(pc[1]))
				{
					pc++;
					// Получить индекс цвета (0..15)
					int nIdx = *pc - L'0';

					if (nIdx == 1 && isDigit(pc[1]))
					{
						pc++;
						nIdx = nIdx*10 + (*pc - L'0');
					}

					if (nIdx >= 0 && nIdx <= 15)
					{
						newBgColors |= (1 << nIdx);
					}
				}
			}
		}

		// Если таки изменился - обновим
		if (newBgColors && gpSet->nBgImageColors != newBgColors)
		{
			gpSet->nBgImageColors = newBgColors;
			gpSetCls->NeedBackgroundUpdate();
			gpConEmu->Update(true);
		}

		break;
	} // case tBgImageColors:

	case tDarker:
	{
		DWORD newV;
		TCHAR tmp[10];
		GetDlgItemText(hDlg, tDarker, tmp, countof(tmp));
		newV = _wtoi(tmp);

		if (newV < 256 && newV != gpSet->bgImageDarker)
		{
			gpSetCls->SetBgImageDarker(newV, true);

			//gpSet->bgImageDarker = newV;
			//SendDlgItemMessage(hWnd2, slDarker, TBM_SETPOS, (WPARAM) true, (LPARAM) gpSet->bgImageDarker);

			//// Картинку может установить и плагин
			//if (gpSet->isShowBgImage && gpSet->sBgImage[0])
			//	LoadBackgroundFile(gpSet->sBgImage);
			//else
			//	NeedBackgroundUpdate();

			//gpConEmu->Update(true);
		}
		break;
	} //case tDarker:

	default:
		_ASSERTE(FALSE && "EditBox was not processed");
	}

	return 0;
}
