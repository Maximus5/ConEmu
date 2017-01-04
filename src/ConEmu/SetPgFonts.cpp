
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
#include "FontMgr.h"
#include "OptionsClass.h"
#include "SetDlgLists.h"
#include "SetPgFonts.h"

CSetPgFonts::CSetPgFonts()
{
}

CSetPgFonts::~CSetPgFonts()
{
}

LRESULT CSetPgFonts::OnInitDialog(HWND hDlg, bool abInitial)
{
	SetDlgItemText(hDlg, tFontFace, gpFontMgr->FontFaceName());
	SetDlgItemText(hDlg, tFontFace2, gpFontMgr->BorderFontFaceName());

	if (abInitial)
	{
		// Добавить шрифты рисованные ConEmu
		for (INT_PTR j = 0; j < gpFontMgr->m_RegFonts.size(); ++j)
		{
			const RegFont* iter = &(gpFontMgr->m_RegFonts[j]);

			if (iter->pCustom)
			{
				BOOL bMono = iter->pCustom->GetFont(0,0,0,0)->IsMonospace();

				int nIdx = SendDlgItemMessage(hDlg, tFontFace, CB_ADDSTRING, 0, (LPARAM)iter->szFontName);
				SendDlgItemMessage(hDlg, tFontFace, CB_SETITEMDATA, nIdx, bMono ? 1 : 0);

				nIdx = SendDlgItemMessage(hDlg, tFontFace2, CB_ADDSTRING, 0, (LPARAM)iter->szFontName);
				SendDlgItemMessage(hDlg, tFontFace2, CB_SETITEMDATA, nIdx, bMono ? 1 : 0);
			}
		}

		CSetDlgFonts::StartEnumFontsThread();

		CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, tFontSizeY), CSetDlgLists::eFSizesY, gpSet->FontSizeY, true);

		CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, tFontSizeX), CSetDlgLists::eFSizesX, gpSet->FontSizeX, true);
		CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, tFontSizeX2), CSetDlgLists::eFSizesX, gpSet->FontSizeX2, true);
		CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, tFontSizeX3), CSetDlgLists::eFSizesX, gpSet->FontSizeX3, true);

		CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbExtendFontBoldIdx), CSetDlgLists::eColorIdx, gpSet->AppStd.nFontBoldColor, false);
		CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbExtendFontItalicIdx), CSetDlgLists::eColorIdx, gpSet->AppStd.nFontItalicColor, false);
		CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbExtendFontNormalIdx), CSetDlgLists::eColorIdx, gpSet->AppStd.nFontNormalColor, false);

		CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, tFontCharset), CSetDlgLists::eCharSets, gpFontMgr->LogFont.lfCharSet, false);
	}
	else
	{
		//TODO: Обновить значения в списках?
	}

	checkDlgButton(hDlg, cbExtendFonts, gpSet->AppStd.isExtendFonts);
	CSetDlgLists::EnableDlgItems(hDlg, CSetDlgLists::eExtendFonts, gpSet->AppStd.isExtendFonts);

	checkDlgButton(hDlg, cbFontAuto, gpSet->isFontAutoSize);

	MCHKHEAP

	checkRadioButton(hDlg, rNoneAA, rCTAA,
		(gpFontMgr->LogFont.lfQuality == CLEARTYPE_NATURAL_QUALITY) ? rCTAA :
		(gpFontMgr->LogFont.lfQuality == ANTIALIASED_QUALITY) ? rStandardAA : rNoneAA);

	// 3d state - force center symbols in cells
	checkDlgButton(hDlg, cbMonospace, BST(gpSet->isMonospace));

	checkDlgButton(hDlg, cbBold, (gpFontMgr->LogFont.lfWeight == FW_BOLD) ? BST_CHECKED : BST_UNCHECKED);

	checkDlgButton(hDlg, cbItalic, gpFontMgr->LogFont.lfItalic ? BST_CHECKED : BST_UNCHECKED);

	/* Alternative font, initially created for prettifying Far Manager borders */
	{
		checkDlgButton(hDlg, cbFixFarBorders, BST(gpSet->isFixFarBorders));

		checkDlgButton(hDlg, cbFont2AA, gpSet->isAntiAlias2 ? BST_CHECKED : BST_UNCHECKED);

		LPCWSTR cszFontRanges[] = {
			L"Far Manager borders: 2500-25C4;",
			L"Dashes and Borders: 2013-2015;2500-25C4;",
			L"Pseudographics: 2013-25C4;",
			L"CJK: 2E80-9FC3;AC00-D7A3;F900-FAFF;FE30-FE4F;FF01-FF60;FFE0-FFE6;",
			NULL
		};
		CEStr szCharRanges(gpSet->CreateCharRanges(gpSet->mpc_CharAltFontRanges));
		LPCWSTR pszCurrentRange = szCharRanges.ms_Val;
		bool bExist = false;

		HWND hCombo = GetDlgItem(hDlg, tUnicodeRanges);
		SendDlgItemMessage(hDlg, tUnicodeRanges, CB_RESETCONTENT, 0,0);

		// Fill our drop down with font ranges
		for (INT_PTR i = 0; cszFontRanges[i]; i++)
		{
			LPCWSTR pszRange = wcsstr(cszFontRanges[i], L": ");
			if (!pszRange) { _ASSERTE(pszRange); continue; }

			SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)cszFontRanges[i]);

			if (!bExist && (lstrcmpi(pszRange+2, pszCurrentRange) == 0))
			{
				pszCurrentRange = cszFontRanges[i];
				bExist = true;
			}
		}
		if (pszCurrentRange && *pszCurrentRange && !bExist)
			SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)pszCurrentRange);
		// And show current value
		SetWindowText(hCombo, pszCurrentRange ? pszCurrentRange : L"");
	}
	/* Alternative font ends */

	checkDlgButton(hDlg, cbFontMonitorDpi, gpSet->FontUseDpi ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hDlg, cbFontAsDeviceUnits, gpSet->FontUseUnits ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hDlg, cbCompressLongStrings, gpSet->isCompressLongStrings ? BST_CHECKED : BST_UNCHECKED);

	gpSetCls->mn_LastChangingFontCtrlId = 0;
	return 0;
}

// tThumbsFontName, tTilesFontName, 0
void CSetPgFonts::CopyFontsTo(HWND hDlgTo, int nList1, ...)
{
	DWORD bAlmostMonospace;
	int nIdx, nCount, i;
	wchar_t szFontName[128]; // FontFaceName is not expected to be longer than 32 chars

	HWND hMainPg = Dlg();
	nCount = SendDlgItemMessage(hMainPg, tFontFace, CB_GETCOUNT, 0, 0);

#ifdef _DEBUG
	GetDlgItemText(hDlgTo, nList1, szFontName, countof(szFontName));
#endif

	int nCtrls = 1;
	int nCtrlIds[10] = {nList1};
	va_list argptr;
	va_start(argptr, nList1);
	int nNext = va_arg( argptr, int );
	while (nNext)
	{
		nCtrlIds[nCtrls++] = nNext;
		nNext = va_arg( argptr, int );
	}
	va_end(argptr);


	for (i = 0; i < nCount; i++)
	{
		// Взять список шрифтов с главной страницы
		if (SendDlgItemMessage(hMainPg, tFontFace, CB_GETLBTEXT, i, (LPARAM) szFontName) > 0)
		{
			// Показывать [Raster Fonts WxH] смысла нет
			if (szFontName[0] == L'[' && !wcsncmp(szFontName+1, CFontMgr::RASTER_FONTS_NAME, _tcslen(CFontMgr::RASTER_FONTS_NAME)))
				continue;
			// В Thumbs & Tiles [bdf] пока не поддерживается
			int iLen = lstrlen(szFontName);
			if ((iLen > CE_BDF_SUFFIX_LEN) && !wcscmp(szFontName+iLen-CE_BDF_SUFFIX_LEN, CE_BDF_SUFFIX))
				continue;

			bAlmostMonospace = (DWORD)SendDlgItemMessage(hMainPg, tFontFace, CB_GETITEMDATA, i, 0);

			// Теперь создаем новые строки
			for (int j = 0; j < nCtrls; j++)
			{
				nIdx = SendDlgItemMessage(hDlgTo, nCtrlIds[j], CB_ADDSTRING, 0, (LPARAM) szFontName);
				SendDlgItemMessage(hDlgTo, nCtrlIds[j], CB_SETITEMDATA, nIdx, bAlmostMonospace);
			}
		}
	}

	for (int j = 0; j < nCtrls; j++)
	{
		GetDlgItemText(hDlgTo, nCtrlIds[j], szFontName, countof(szFontName));
		CSetDlgLists::SelectString(hDlgTo, nCtrlIds[j], szFontName);
	}
}

INT_PTR CSetPgFonts::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	switch (nCtrlId)
	{
	case tFontCharset:
		gpSet->mb_CharSetWasSet = TRUE;
		PostMessage(hDlg, gpSetCls->mn_MsgRecreateFont, nCtrlId, 0);
		break;

	case tFontFace:
	case tFontFace2:
	case tFontSizeY:
	case tFontSizeX:
	case tFontSizeX2:
	case tFontSizeX3:
		if (code == CBN_SELCHANGE)
			PostMessage(hDlg, gpSetCls->mn_MsgRecreateFont, nCtrlId, 0);
		else
			gpSetCls->mn_LastChangingFontCtrlId = nCtrlId;
		break;

	case tUnicodeRanges:
		// Do not required actually, the button "Apply" is enabled by default
		enableDlgItem(hDlg, cbUnicodeRangesApply, true);
		if (code == CBN_SELCHANGE)
			PostMessage(hDlg, WM_COMMAND, cbUnicodeRangesApply, 0);
		break;

	case lbExtendFontNormalIdx:
	case lbExtendFontBoldIdx:
	case lbExtendFontItalicIdx:
	{
		if (nCtrlId == lbExtendFontNormalIdx)
			gpSet->AppStd.nFontNormalColor = GetNumber(hDlg, nCtrlId);
		else if (nCtrlId == lbExtendFontBoldIdx)
			gpSet->AppStd.nFontBoldColor = GetNumber(hDlg, nCtrlId);
		else if (nCtrlId == lbExtendFontItalicIdx)
			gpSet->AppStd.nFontItalicColor = GetNumber(hDlg, nCtrlId);

		if (gpSet->AppStd.isExtendFonts)
			gpConEmu->Update(true);
		break;
	}

	default:
		_ASSERTE(FALSE && "ListBox was not processed");
	}

	return 0;
}
