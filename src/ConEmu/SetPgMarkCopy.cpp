
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
#include "Font.h"
#include "FontMgr.h"
#include "OptionsClass.h"
#include "SetDlgLists.h"
#include "SetPgMarkCopy.h"

CSetPgMarkCopy::CSetPgMarkCopy()
	: mf_MarkCopyPreviewProc(NULL)
{
}

CSetPgMarkCopy::~CSetPgMarkCopy()
{
}

LRESULT CSetPgMarkCopy::OnInitDialog(HWND hDlg, bool abInitial)
{
	checkDlgButton(hDlg, cbCTSIntelligent, gpSet->isCTSIntelligent);
	wchar_t* pszExcept = gpSet->GetIntelligentExceptions();
	SetDlgItemText(hDlg, tCTSIntelligentExceptions, pszExcept ? pszExcept : L"");
	SafeFree(pszExcept);

	checkDlgButton(hDlg, cbCTSAutoCopy, gpSet->isCTSAutoCopy);
	checkDlgButton(hDlg, cbCTSResetOnRelease, (gpSet->isCTSResetOnRelease && gpSet->isCTSAutoCopy));
	enableDlgItem(hDlg, cbCTSResetOnRelease, gpSet->isCTSAutoCopy);

	checkDlgButton(hDlg, cbCTSIBeam, gpSet->isCTSIBeam);

	checkDlgButton(hDlg, cbCTSEndOnTyping, (gpSet->isCTSEndOnTyping != 0));
	checkDlgButton(hDlg, cbCTSEndOnKeyPress, (gpSet->isCTSEndOnTyping != 0) && gpSet->isCTSEndOnKeyPress);
	checkDlgButton(hDlg, cbCTSEndCopyBefore, (gpSet->isCTSEndOnTyping == 1));
	checkDlgButton(hDlg, cbCTSEraseBeforeReset, gpSet->isCTSEraseBeforeReset);
	enableDlgItem(hDlg, cbCTSEndOnKeyPress, gpSet->isCTSEndOnTyping!=0);
	enableDlgItem(hDlg, cbCTSEndCopyBefore, gpSet->isCTSEndOnTyping!=0);
	enableDlgItem(hDlg, cbCTSEraseBeforeReset, gpSet->isCTSEndOnTyping!=0);

	checkDlgButton(hDlg, cbCTSFreezeBeforeSelect, gpSet->isCTSFreezeBeforeSelect);
	checkDlgButton(hDlg, cbCTSBlockSelection, gpSet->isCTSSelectBlock);
	UINT VkMod = gpSet->GetHotkeyById(vkCTSVkBlock);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbCTSBlockSelection), CSetDlgLists::eKeysAct, VkMod, true);
	checkDlgButton(hDlg, cbCTSTextSelection, gpSet->isCTSSelectText);
	VkMod = gpSet->GetHotkeyById(vkCTSVkText);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbCTSTextSelection), CSetDlgLists::eKeysAct, VkMod, true);
	VkMod = gpSet->GetHotkeyById(vkCTSVkAct);

	UINT idxBack = CONBACKCOLOR(gpSet->isCTSColorIndex);
	UINT idxFore = CONFORECOLOR(gpSet->isCTSColorIndex);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbCTSForeIdx), CSetDlgLists::eColorIdx16, idxFore, false);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbCTSBackIdx), CSetDlgLists::eColorIdx16, idxBack, false);

	if (abInitial)
	{
		mf_MarkCopyPreviewProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hDlg, stCTSPreview), GWLP_WNDPROC, (LONG_PTR)MarkCopyPreviewProc);
	}

	checkDlgButton(hDlg, cbCTSDetectLineEnd, gpSet->AppStd.isCTSDetectLineEnd);
	checkDlgButton(hDlg, cbCTSBashMargin, gpSet->AppStd.isCTSBashMargin);
	checkDlgButton(hDlg, cbCTSTrimTrailing, gpSet->AppStd.isCTSTrimTrailing);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbCTSEOL), CSetDlgLists::eCRLF, gpSet->AppStd.isCTSEOL, false);

	checkDlgButton(hDlg, cbCTSShiftArrowStartSel, gpSet->AppStd.isCTSShiftArrowStart);

	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbCopyFormat), CSetDlgLists::eCopyFormat, gpSet->isCTSHtmlFormat, false);

	gpSetCls->CheckSelectionModifiers(hDlg);

	return 0;
}

LRESULT CSetPgMarkCopy::MarkCopyPreviewProc(HWND hCtrl, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc = 0;
	PAINTSTRUCT ps = {};
	HBRUSH hbr;
	HDC hdc;

	uint idxCon = gpSet->AppStd.nBackColorIdx;
	if (idxCon > 15)
		idxCon = 0;
	uint idxBack = CONBACKCOLOR(gpSet->isCTSColorIndex);
	uint idxFore = CONFORECOLOR(gpSet->isCTSColorIndex);
	RECT rcClient = {};
	CSetPgMarkCopy* pPage = NULL;

	switch (uMsg)
	{
	case WM_ERASEBKGND:
	case WM_PAINT:
		hbr = CreateSolidBrush(gpSet->Colors[idxCon]);
		GetClientRect(hCtrl, &rcClient);
		if (uMsg == WM_ERASEBKGND)
			hdc = (HDC)wParam;
		else
			hdc = BeginPaint(hCtrl, &ps);
		if (!hdc)
			goto wrap;
		FillRect(hdc, &rcClient, hbr);
		if (uMsg == WM_PAINT)
		{
			HFONT hOld = NULL;
			HFONT hNew = NULL;
			CFontPtr pFont;
			if (gpFontMgr->QueryFont(fnt_Normal, NULL, pFont) && (pFont->iType == CEFONT_GDI))
				hNew = pFont->hFont;
			else
				hNew = (HFONT)SendMessage(GetParent(hCtrl), WM_GETFONT, 0, 0);
			if (hNew)
				hOld = (HFONT)SelectObject(hdc, hNew);
			SetTextColor(hdc, gpSet->Colors[idxFore]);
			SetBkColor(hdc, gpSet->Colors[idxBack]);
			wchar_t szText[] = L" Selected text ";
			SIZE sz = {};
			GetTextExtentPoint32(hdc, szText, lstrlen(szText), &sz);
			RECT rcText = {sz.cx, sz.cy};
			OffsetRect(&rcText, max(0,(rcClient.right-rcClient.left-sz.cx)/2), max(0,(rcClient.bottom-rcClient.top-sz.cy)/2));
			DrawText(hdc, szText, -1, &rcText, DT_VCENTER|DT_CENTER|DT_SINGLELINE);
			if (hOld)
				SelectObject(hdc, hOld);
		}
		DeleteObject(hbr);
		if (uMsg == WM_PAINT)
			EndPaint(hCtrl, &ps);
		goto wrap;
	}

	if (gpSetCls->GetPageObj(pPage) && pPage->mf_MarkCopyPreviewProc)
		lRc = ::CallWindowProc(pPage->mf_MarkCopyPreviewProc, hCtrl, uMsg, wParam, lParam);
	else
		lRc = ::DefWindowProc(hCtrl, uMsg, wParam, lParam);
wrap:
	return lRc;

}

INT_PTR CSetPgMarkCopy::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	if (code == CBN_SELCHANGE)
	{
		switch (nCtrlId)
		{
		case lbCTSBlockSelection:
			{
				BYTE VkMod = 0;
				CSetDlgLists::GetListBoxItem(hDlg, lbCTSBlockSelection, CSetDlgLists::eKeysAct, VkMod);
				gpSet->SetHotkeyById(vkCTSVkBlock, VkMod);
				gpSetCls->CheckSelectionModifiers(hDlg);
			} break;
		case lbCTSTextSelection:
			{
				BYTE VkMod = 0;
				CSetDlgLists::GetListBoxItem(hDlg, lbCTSTextSelection, CSetDlgLists::eKeysAct, VkMod);
				gpSet->SetHotkeyById(vkCTSVkText, VkMod);
				gpSetCls->CheckSelectionModifiers(hDlg);
			} break;
		case lbCTSEOL:
			{
				BYTE eol = 0;
				CSetDlgLists::GetListBoxItem(hDlg, lbCTSEOL, CSetDlgLists::eCRLF, eol);
				gpSet->AppStd.isCTSEOL = eol;
			} // lbCTSEOL
			break;
		case lbCopyFormat:
			{
				BYTE CopyFormat = 0;
				CSetDlgLists::GetListBoxItem(hDlg, lbCopyFormat, CSetDlgLists::eCopyFormat, CopyFormat);
				gpSet->isCTSHtmlFormat = CopyFormat;
			} // lbCopyFormat
			break;
		case lbCTSForeIdx:
			{
				UINT nFore = 0;
				CSetDlgLists::GetListBoxItem(hDlg, lbCTSForeIdx, CSetDlgLists::eColorIdx16, nFore);
				gpSet->isCTSColorIndex = MAKECONCOLOR((nFore & 0xF), CONBACKCOLOR(gpSet->isCTSColorIndex));
				InvalidateRect(GetDlgItem(hDlg, stCTSPreview), NULL, FALSE);
				gpConEmu->Update(true);
			} break;
		case lbCTSBackIdx:
			{
				UINT nBack = 0;
				CSetDlgLists::GetListBoxItem(hDlg, lbCTSBackIdx, CSetDlgLists::eColorIdx16, nBack);
				gpSet->isCTSColorIndex = MAKECONCOLOR(CONFORECOLOR(gpSet->isCTSColorIndex), (nBack & 0xF));
				InvalidateRect(GetDlgItem(hDlg, stCTSPreview), NULL, FALSE);
				gpConEmu->Update(true);
			} break;
		default:
			_ASSERTE(FALSE && "ListBox was not processed");
		}
	} // if (HIWORD(wParam) == CBN_SELCHANGE)

	return 0;
}

LRESULT CSetPgMarkCopy::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case tCTSIntelligentExceptions:
		// *** Console text selections - intelligent exclusions ***
		{
			wchar_t* pszApps = GetDlgItemTextPtr(hDlg, tCTSIntelligentExceptions);
			gpSet->SetIntelligentExceptions(pszApps);
			SafeFree(pszApps);
		}
		break;

	default:
		_ASSERTE(FALSE && "EditBox was not processed");
	}

	return 0;
}
