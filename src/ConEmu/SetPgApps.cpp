
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
#include "DpiAware.h"
#include "DynDialog.h"
#include "OptionsClass.h"
#include "SetDlgLists.h"
#include "SetPgApps.h"
#include "SetPgCursor.h"
#include "VConGroup.h"
#include "VirtualConsole.h"

static struct StrDistinctControls
{
	DWORD nOverrideID;
	DWORD nCtrls[32];
} DistinctControls[] = {
	{0, {rbAppDistinctElevatedOn, rbAppDistinctElevatedOff, rbAppDistinctElevatedIgnore, stAppDistinctName, tAppDistinctName}},
	{cbExtendFontsOverride, {cbExtendFonts, stExtendFontBoldIdx, lbExtendFontBoldIdx, stExtendFontItalicIdx, lbExtendFontItalicIdx, stExtendFontNormalIdx, lbExtendFontNormalIdx}},
	{cbCursorOverride, {
		rCursorV, rCursorH, rCursorB/**/, rCursorR/**/, cbCursorColor, cbCursorBlink, cbCursorIgnoreSize, tCursorFixedSize, stCursorFixedSize, tCursorMinSize, stCursorMinSize,
		cbInactiveCursor/**/, rInactiveCursorV/**/, rInactiveCursorH/**/, rInactiveCursorB/**/, rInactiveCursorR/**/, cbInactiveCursorColor/**/, cbInactiveCursorBlink/**/, cbInactiveCursorIgnoreSize/**/, tInactiveCursorFixedSize/**/, stInactiveCursorFixedSize, tInactiveCursorMinSize, stInactiveCursorMinSize,
	}},
	{cbColorsOverride, {lbColorsOverride}},
	{cbClipboardOverride, {
		gbCopyingOverride, cbCTSDetectLineEnd, cbCTSBashMargin, cbCTSTrimTrailing, stCTSEOL, lbCTSEOL,
		gbSelectingOverride, cbCTSShiftArrowStartSel,
		gbPasteM1, rPasteM1MultiLine, rPasteM1SingleLine, rPasteM1FirstLine, rPasteM1Nothing,
		gbPasteM2, rPasteM2MultiLine, rPasteM2SingleLine, rPasteM2FirstLine, rPasteM2Nothing,
		gbPromptOverride, cbCTSClickPromptPosition, cbCTSDeleteLeftWord}},
};

CSetPgApps::CSetPgApps()
	: mb_SkipEditChange(false)
	, mb_SkipEditSet(false)
	, mp_DlgDistinct2(NULL)
	, mp_DpiDistinct2(NULL)
	, mh_Child(NULL)
	, mb_Redraw(false)
	, mb_Refill(false)
{
}

CSetPgApps::~CSetPgApps()
{
	if (mp_DpiDistinct2)
		mp_DpiDistinct2->Detach();
	SafeDelete(mp_DpiDistinct2);
	SafeDelete(mp_DlgDistinct2);
}

LRESULT CSetPgApps::OnInitDialog(HWND hDlg, bool abInitial)
{
	//mn_AppsEnableControlsMsg = RegisterWindowMessage(L"ConEmu::AppsEnableControls");

	if (!mh_Child)
	{
		if (abInitial)
		{
			_ASSERTE(mh_Dlg == hDlg);
			CreateChildDlg();
		}

		if (!mh_Child)
		{
			_ASSERTE(mh_Child != NULL); // Must be created already!
			return 0;
		}
	}

	if (abInitial)
	{
		int nTab[2] = {4*4, 7*4}; // represent the number of quarters of the average character width for the font
		SendDlgItemMessage(hDlg, lbAppDistinct, LB_SETTABSTOPS, countof(nTab), (LPARAM)nTab);

		LONG_PTR nStyles = GetWindowLongPtr(GetDlgItem(hDlg, lbAppDistinct), GWL_STYLE);
		if (!(nStyles & LBS_NOTIFY))
			SetWindowLongPtr(GetDlgItem(hDlg, lbAppDistinct), GWL_STYLE, nStyles|LBS_NOTIFY);
	}

	MSetter lockSelChange(&mb_SkipSelChange);

	if (abInitial)
	{
		SendDlgItemMessage(mh_Child, lbColorsOverride, CB_RESETCONTENT, 0, 0);
		int iCurPalette = 0;
		const ColorPalette* pPal;
		for (int i = 0; (pPal = gpSet->PaletteGet(i)) != NULL; i++)
		{
			SendDlgItemMessage(mh_Child, lbColorsOverride, CB_ADDSTRING, 0, (LPARAM)pPal->pszName);
			if ((!iCurPalette) && (lstrcmp(pPal->pszName, gsDefaultColorScheme) == 0))
				iCurPalette = i;
		}
		SendDlgItemMessage(mh_Child, lbColorsOverride, CB_SETCURSEL, iCurPalette, 0);

		CSetDlgLists::FillListBox(mh_Child, lbExtendFontBoldIdx, CSetDlgLists::eColorIdx);
		CSetDlgLists::FillListBox(mh_Child, lbExtendFontItalicIdx, CSetDlgLists::eColorIdx);
		CSetDlgLists::FillListBox(mh_Child, lbExtendFontNormalIdx, CSetDlgLists::eColorIdx);
	}

	// Сброс ранее загруженного списка (ListBox: lbAppDistinct)
	SendDlgItemMessage(hDlg, lbAppDistinct, LB_RESETCONTENT, 0,0);

	int nApp = 0;
	const AppSettings* pApp = NULL;
	while ((pApp = gpSet->GetAppSettings(nApp)) && pApp->AppNames)
	{
		nApp++;
		SetListAppName(pApp, nApp);
	}

	lockSelChange.Unlock();

	OnAppSelectionChanged();

	return 0;
}

INT_PTR CSetPgApps::SetListAppName(const AppSettings* pApp, int nAppIndex/*1-based*/, INT_PTR iBefore/*0-based*/)
{
	INT_PTR iCount, iIndex;
	wchar_t szItem[1024];

	_wsprintf(szItem, SKIPLEN(countof(szItem)) L"%i\t%s\t", nAppIndex,
		(pApp->Elevated == 1) ? L"E" : (pApp->Elevated == 2) ? L"N" : L"*");
	int nPrefix = lstrlen(szItem);
	if (pApp->AppNames)
		lstrcpyn(szItem+nPrefix, pApp->AppNames, countof(szItem)-nPrefix);

	iIndex = SendDlgItemMessage(mh_Dlg, lbAppDistinct, LB_INSERTSTRING, iBefore, (LPARAM)szItem);
	if ((iBefore >= 0) && (iIndex >= 0))
	{
		_ASSERTE(iIndex == iBefore);

		iCount = SendDlgItemMessage(mh_Dlg, lbAppDistinct, LB_GETCOUNT, 0, 0);
		if (iCount <= (iIndex+1))
		{
			_ASSERTE(iCount > (iIndex+1));
		}
		else
		{
			SendDlgItemMessage(mh_Dlg, lbAppDistinct, LB_DELETESTRING, iIndex+1, 0);
			SendDlgItemMessage(mh_Dlg, lbAppDistinct, LB_SETCURSEL, iIndex,0);
		}
	}

	return iIndex;
}

void CSetPgApps::NotifyVCon()
{
	// Tell all RCon-s that application settings were changed
	struct impl
	{
		static bool ResetAppId(CVirtualConsole* pVCon, LPARAM lParam)
		{
			pVCon->RCon()->ResetActiveAppSettingsId();
			return true;
		};
	};
	CVConGroup::EnumVCon(evf_All, impl::ResetAppId, 0);

	// And update VCon-s. We need to update consoles if only visible settings were changes...
	gpConEmu->Update(true);
}

void CSetPgApps::ProcessDpiChange(const CDpiForDialog* apDpi)
{
	if (!this || !mh_Dlg || !mp_DpiAware || !apDpi)
		return;

	CSetPgBase::ProcessDpiChange(apDpi);

	if (mp_DpiDistinct2)
	{
		HWND hHolder = GetDlgItem(mh_Dlg, tAppDistinctHolder);
		RECT rcPos = {}; GetWindowRect(hHolder, &rcPos);
		MapWindowPoints(NULL, mh_Dlg, (LPPOINT)&rcPos, 2);

		mp_DpiDistinct2->SetDialogDPI(apDpi->GetCurDpi(), &rcPos);
	}
}

INT_PTR CSetPgApps::pageOpProc_AppsChild(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	static int nLastScrollPos = 0;

	CSetPgApps* pObj;
	if (!gpSetCls->GetPageObj(pObj))
	{
		_ASSERTE(FALSE && "CSetPgApps was not created yet");
		return FALSE;
	}

	switch (messg)
	{
	case WM_INITDIALOG:
		nLastScrollPos = 0;
		gpSetCls->RegisterTipsFor(hDlg);
		return FALSE;

	case WM_NOTIFY:
		{
			LPNMHDR phdr = (LPNMHDR)lParam;

			if (phdr->code == TTN_GETDISPINFO)
			{
				return gpSetCls->ProcessTipHelp(hDlg, messg, wParam, lParam);
			}

			break;
		}

	case WM_MOUSEWHEEL:
		{
			SHORT nDir = (SHORT)HIWORD(wParam);
			if (nDir)
			{
				pageOpProc_AppsChild(hDlg, WM_VSCROLL, (nDir > 0) ? SB_PAGEUP : SB_PAGEDOWN, 0);
			}
		}
		return 0;

	case WM_VSCROLL:
		{
			int dx = 0, dy = 0;

			SCROLLINFO si = {sizeof(si)};
			si.fMask = SIF_POS|SIF_RANGE|SIF_PAGE;
			GetScrollInfo(hDlg, SB_VERT, &si);

			int nPos = 0;
			SHORT nCode = (SHORT)LOWORD(wParam);
			if ((nCode == SB_THUMBPOSITION) || (nCode == SB_THUMBTRACK))
			{
				nPos = (SHORT)HIWORD(wParam);
			}
			else
			{
				nPos = si.nPos;
				int nDelta = 16; // Высота CheckBox'a
				RECT rcChild = {};
				if (GetWindowRect(GetDlgItem(hDlg, cbExtendFontsOverride), &rcChild))
					nDelta = rcChild.bottom - rcChild.top;

				switch (nCode)
				{
				case SB_LINEDOWN:
				case SB_PAGEDOWN:
					nPos = min(si.nMax,si.nPos+nDelta);
					break;
				//case SB_PAGEDOWN:
				//	nPos = min(si.nMax,si.nPos+si.nPage);
				//	break;
				case SB_LINEUP:
				case SB_PAGEUP:
					nPos = max(si.nMin,si.nPos-nDelta);
					break;
				//case SB_PAGEUP:
				//	nPos = max(si.nMin,si.nPos-si.nPage);
				//	break;
				}
			}

			dy = nLastScrollPos - nPos;
			nLastScrollPos = nPos;

			si.fMask = SIF_POS;
			si.nPos = nPos;
			SetScrollInfo(hDlg, SB_VERT, &si, TRUE);

			if (dy)
			{
				ScrollWindowEx(hDlg, dx, dy, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN|SW_INVALIDATE|SW_ERASE);
			}
		}
		return FALSE;

	default:
		if (pObj->mp_DpiDistinct2 && pObj->mp_DpiDistinct2->ProcessDpiMessages(hDlg, messg, wParam, lParam))
		{
			return TRUE;
		}
	}

	HWND hParent = GetParent(hDlg);
	return pObj->PageDlgProc(hParent, messg, wParam, lParam);
}

INT_PTR CSetPgApps::PageDlgProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR iRc = 0;

	if (!mh_Child && ((messg == WM_INITDIALOG) || (messg == mn_ActivateTabMsg)))
	{
		if (!CreateChildDlg())
		{
			return 0;
		}
	}

	if ((messg == WM_INITDIALOG) || (messg == mn_ActivateTabMsg))
	{
		// Return here! Do not pass to mb_Redraw/mb_Refill routines!
		return OnInitDialog(hDlg, (messg == WM_INITDIALOG));
	}
	else switch (messg)
	{
	case WM_NOTIFY:
		{
			LPNMHDR phdr = (LPNMHDR)lParam;

			if (phdr->code == TTN_GETDISPINFO)
			{
				return gpSetCls->ProcessTipHelp(hDlg, messg, wParam, lParam);
			}

			break;
		}

	case WM_COMMAND:
		{
			_ASSERTE(mh_Child && IsWindow(mh_Child));
			_ASSERTE(hDlg == mh_Dlg);

			if (HIWORD(wParam) == BN_CLICKED)
			{
				OnButtonClicked(hDlg, (HWND)lParam, LOWORD(wParam));
			}
			else if (HIWORD(wParam) == EN_CHANGE)
			{
				OnEditChanged(hDlg, LOWORD(wParam));
			}
			else if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				OnComboBox(hDlg, LOWORD(wParam), HIWORD(wParam));
			}

		} // WM_COMMAND
		break;
	} // switch (messg)


	if (mb_Redraw)
	{
		mb_Redraw = false;
		NotifyVCon();
	}


	if (mb_Refill)
	{
		mb_Refill = false;

		MSetter lockSelChange(&mb_SkipSelChange);
		MSetter lockEditSet(&mb_SkipEditSet);

		INT_PTR iSel = SendDlgItemMessage(hDlg, lbAppDistinct, LB_GETCURSEL, 0,0);
		OnInitDialog(hDlg, false);
		SendDlgItemMessage(hDlg, lbAppDistinct, LB_SETCURSEL, iSel,0);

	} // mb_Refill


	// To be sure
	mb_Refill = false;
	mb_Redraw = false;

	return iRc;
}

void CSetPgApps::DoReloadApps()
{
	if (MsgBox(L"Warning! All unsaved changes will be lost!\n\nReload Apps from settings?",
			MB_YESNO|MB_ICONEXCLAMATION, gpConEmu->GetDefaultTitle(), ghOpWnd) != IDYES)
		return;

	// Reload App distinct
	gpSet->LoadAppsSettings(NULL, true);

	// Refresh on screen
	OnInitDialog(mh_Dlg, true);
}

void CSetPgApps::DoAppAdd()
{
	int iCount = (int)SendDlgItemMessage(mh_Dlg, lbAppDistinct, LB_GETCOUNT, 0,0);
	AppSettings* pNew = gpSet->GetAppSettingsPtr(iCount, TRUE);
	UNREFERENCED_PARAMETER(pNew);

	MSetter lockSelChange(&mb_SkipSelChange);

	OnInitDialog(mh_Dlg, false);

	SendDlgItemMessage(mh_Dlg, lbAppDistinct, LB_SETCURSEL, iCount, 0);

	lockSelChange.Unlock();

	OnAppSelectionChanged();
}

void CSetPgApps::DoAppDel()
{
	int iCur = (int)SendDlgItemMessage(mh_Dlg, lbAppDistinct, LB_GETCURSEL, 0,0);
	if (iCur < 0)
		return;

	const AppSettings* p = gpSet->GetAppSettingsPtr(iCur);
	if (!p)
		return;

	if (MsgBox(L"Delete application?", MB_YESNO|MB_ICONQUESTION, p->AppNames, ghOpWnd) != IDYES)
		return;

	gpSet->AppSettingsDelete(iCur);

	MSetter lockSelChange(&mb_SkipSelChange);

	OnInitDialog(mh_Dlg, false);

	int iCount = (int)SendDlgItemMessage(mh_Dlg, lbAppDistinct, LB_GETCOUNT, 0,0);

	SendDlgItemMessage(mh_Dlg, lbAppDistinct, LB_SETCURSEL, min(iCur,(iCount-1)), 0);

	lockSelChange.Unlock();

	OnAppSelectionChanged();
}

void CSetPgApps::DoAppMove(bool bUpward)
{
	int iCur, iChg;
	iCur = (int)SendDlgItemMessage(mh_Dlg, lbAppDistinct, LB_GETCURSEL, 0,0);
	if (iCur < 0)
		return;
	if (bUpward)
	{
		if (!iCur)
			return;
		iChg = iCur - 1;
	}
	else
	{
		iChg = iCur + 1;
		if (iChg >= (int)SendDlgItemMessage(mh_Dlg, lbAppDistinct, LB_GETCOUNT, 0,0))
			return;
	}

	if (!gpSet->AppSettingsXch(iCur, iChg))
		return;

	MSetter lockSelChange(&mb_SkipSelChange);

	OnInitDialog(mh_Dlg, false);

	SendDlgItemMessage(mh_Dlg, lbAppDistinct, LB_SETCURSEL, iChg, 0);

	lockSelChange.Unlock();

	OnAppSelectionChanged();
}

INT_PTR CSetPgApps::OnButtonClicked(HWND hDlg, HWND hBtn, WORD nCtrlId)
{
	// First we process "Main" buttons, controlling application list
	switch (nCtrlId)
	{
	case cbAppDistinctReload:
		DoReloadApps();
		return 0;
	case cbAppDistinctAdd:
		DoAppAdd();
		return 0;
	case cbAppDistinctDel:
		DoAppDel();
		return 0;
	case cbAppDistinctUp:
	case cbAppDistinctDown:
		DoAppMove(nCtrlId == cbAppDistinctUp);
		return 0;
	}

	BOOL bChecked;
	int iCur = mb_SkipSelChange ? -1 : (int)SendDlgItemMessage(hDlg, lbAppDistinct, LB_GETCURSEL, 0,0);
	AppSettings* pApp = (iCur < 0) ? NULL : gpSet->GetAppSettingsPtr(iCur);
	_ASSERTE((iCur<0) || (pApp && pApp->AppNames));

	if (!pApp)
	{
		_ASSERTE(pApp!=NULL);
		return 0;
	}

	switch (nCtrlId)
	{
	case rbAppDistinctElevatedOn:
	case rbAppDistinctElevatedOff:
	case rbAppDistinctElevatedIgnore:
		pApp->Elevated = isChecked(hDlg, rbAppDistinctElevatedOn) ? 1
			: isChecked(hDlg, rbAppDistinctElevatedOff) ? 2
			: 0;
		mb_Refill = true;
		mb_Redraw = true;
		break;


	case cbColorsOverride:
		bChecked = isChecked(mh_Child, nCtrlId);
		DoEnableControls(cbColorsOverride);
		pApp->OverridePalette = _bool(bChecked);
		mb_Redraw = true;
		// Обновить палитры
		gpSet->PaletteSetStdIndexes();
		// Обновить консоли (считаем, что меняется только TextAttr, Popup не трогать
		gpSetCls->UpdateTextColorSettings(TRUE, FALSE, pApp);
		break;

	case cbCursorOverride:
		bChecked = isChecked(mh_Child, nCtrlId);
		DoEnableControls(cbCursorOverride);
		pApp->OverrideCursor = _bool(bChecked);
		mb_Redraw = true;
		break;

	case rCursorH:
	case rCursorV:
	case rCursorB:
	case rCursorR:
	case cbCursorColor:
	case cbCursorBlink:
	case cbCursorIgnoreSize:
	case cbInactiveCursor:
	case rInactiveCursorH:
	case rInactiveCursorV:
	case rInactiveCursorB:
	case rInactiveCursorR:
	case cbInactiveCursorColor:
	case cbInactiveCursorBlink:
	case cbInactiveCursorIgnoreSize:
		OnButtonClicked_Cursor(mh_Child, nCtrlId, isChecked(hDlg, nCtrlId), pApp);
		mb_Redraw = true;
		break;

	case cbExtendFontsOverride:
		bChecked = isChecked(mh_Child, nCtrlId);
		DoEnableControls(cbExtendFontsOverride);
		pApp->OverrideExtendFonts = isChecked2(mh_Child, nCtrlId);
		mb_Redraw = true;
		break;
	case cbExtendFonts:
		pApp->isExtendFonts = isChecked2(mh_Child, nCtrlId);
		mb_Redraw = true;
		break;

	case cbClipboardOverride:
		bChecked = isChecked(mh_Child, nCtrlId);
		DoEnableControls(cbClipboardOverride);
		pApp->OverrideClipboard = isChecked2(mh_Child, nCtrlId);
		break;

	case rPasteM1MultiLine:
	case rPasteM1FirstLine:
	case rPasteM1SingleLine:
	case rPasteM1Nothing:
		switch (nCtrlId)
		{
		case rPasteM1MultiLine:
			pApp->isPasteAllLines = plm_Default; break;
		case rPasteM1FirstLine:
			pApp->isPasteAllLines = plm_FirstLine; break;
		case rPasteM1SingleLine:
			pApp->isPasteAllLines = plm_SingleLine; break;
		case rPasteM1Nothing:
			pApp->isPasteAllLines = plm_Nothing; break;
		}
		break;
	case rPasteM2MultiLine:
	case rPasteM2FirstLine:
	case rPasteM2SingleLine:
	case rPasteM2Nothing:
		switch (nCtrlId)
		{
		case rPasteM2MultiLine:
			gpSet->AppStd.isPasteFirstLine = plm_MultiLine; break;
		case rPasteM2FirstLine:
			gpSet->AppStd.isPasteFirstLine = plm_FirstLine; break;
		case rPasteM2SingleLine:
			gpSet->AppStd.isPasteFirstLine = plm_Default; break;
		case rPasteM2Nothing:
			gpSet->AppStd.isPasteFirstLine = plm_Nothing; break;
		}
		break;

	case cbCTSDetectLineEnd:
		pApp->isCTSDetectLineEnd = isChecked2(mh_Child, nCtrlId);
		break;
	case cbCTSBashMargin:
		pApp->isCTSBashMargin = isChecked2(mh_Child, nCtrlId);
		break;
	case cbCTSTrimTrailing:
		pApp->isCTSTrimTrailing = isChecked(mh_Child, nCtrlId);
		break;
	case cbCTSClickPromptPosition:
		pApp->isCTSClickPromptPosition = isChecked(mh_Child, nCtrlId);
		break;
	case cbCTSDeleteLeftWord:
		pApp->isCTSDeleteLeftWord = isChecked(mh_Child, nCtrlId);
		break;

	}

	return 0;
}

bool CSetPgApps::CreateChildDlg()
{
	if (mh_Child && !IsWindow(mh_Child))
	{
		_ASSERTE(FALSE && "Invalid mh_Child");
		mh_Child = NULL;
	}

	if (!mh_Child)
	{
		#if defined(_DEBUG)
		HWND hChild = GetDlgItem(mh_Dlg, IDD_SPG_APPDISTINCT2);
		_ASSERTE(hChild == NULL);
		#endif

		LogString(L"Creating child dialog IDD_SPG_APPDISTINCT2");
		SafeDelete(mp_DlgDistinct2); // JIC

		mp_DlgDistinct2 = CDynDialog::ShowDialog(IDD_SPG_APPDISTINCT2, mh_Dlg, pageOpProc_AppsChild, 0/*dwInitParam*/);
		mh_Child = mp_DlgDistinct2 ? mp_DlgDistinct2->mh_Dlg : NULL;

		if (!mh_Child)
		{
			EnableWindow(mh_Dlg, FALSE);
			MBoxAssert(mh_Child && "CreateDialogParam(IDD_SPG_APPDISTINCT2) failed");
			return 0;
		}
		SetWindowLongPtr(mh_Child, GWLP_ID, IDD_SPG_APPDISTINCT2);

		if (!mp_DpiDistinct2 && mp_ParentDpi)
		{
			mp_DpiDistinct2 = new CDpiForDialog();
			mp_DpiDistinct2->Attach(mh_Child, mh_Dlg, mp_DlgDistinct2);
		}

		HWND hHolder = GetDlgItem(mh_Dlg, tAppDistinctHolder);
		RECT rcPos = {}; GetWindowRect(hHolder, &rcPos);
		MapWindowPoints(NULL, mh_Dlg, (LPPOINT)&rcPos, 2);
		POINT ptScroll = {};
		HWND hEnd = GetDlgItem(mh_Child,stAppDistinctBottom);
		MapWindowPoints(hEnd, mh_Child, &ptScroll, 1);
		ShowWindow(hEnd, SW_HIDE);

		SCROLLINFO si = {sizeof(si), SIF_ALL};
		si.nMax = ptScroll.y - (rcPos.bottom - rcPos.top);
		RECT rcChild = {}; GetWindowRect(GetDlgItem(mh_Child, DistinctControls[1].nOverrideID), &rcChild);
		si.nPage = rcChild.bottom - rcChild.top;
		SetScrollInfo(mh_Child, SB_VERT, &si, FALSE);

		MoveWindowRect(mh_Child, rcPos);

		ShowWindow(hHolder, SW_HIDE);
		ShowWindow(mh_Child, SW_SHOW);

		//_ASSERTE(mh_Child && IsWindow(mh_Child));
	}

	return (mh_Child != NULL);
}

void CSetPgApps::DoFillControls(const AppSettings* pApp)
{
	if (!pApp)
	{
		_ASSERTE(pApp);
		return;
	}

	checkRadioButton(mh_Dlg, rbAppDistinctElevatedOn, rbAppDistinctElevatedIgnore,
		(pApp->Elevated == 1) ? rbAppDistinctElevatedOn :
		(pApp->Elevated == 2) ? rbAppDistinctElevatedOff : rbAppDistinctElevatedIgnore);

	BYTE b;

	checkDlgButton(mh_Child, cbExtendFontsOverride, pApp->OverrideExtendFonts);
	checkDlgButton(mh_Child, cbExtendFonts, pApp->isExtendFonts);

	b = pApp->nFontBoldColor;
	CSetDlgLists::FillListBoxItems(GetDlgItem(mh_Child, lbExtendFontBoldIdx), CSetDlgLists::eColorIdx, b, false);
	b = pApp->nFontItalicColor;
	CSetDlgLists::FillListBoxItems(GetDlgItem(mh_Child, lbExtendFontItalicIdx), CSetDlgLists::eColorIdx, b, false);
	b = pApp->nFontNormalColor;
	CSetDlgLists::FillListBoxItems(GetDlgItem(mh_Child, lbExtendFontNormalIdx), CSetDlgLists::eColorIdx, b, false);

	checkDlgButton(mh_Child, cbCursorOverride, pApp->OverrideCursor);
	CSetPgCursor::InitCursorCtrls(mh_Child, pApp);

	checkDlgButton(mh_Child, cbColorsOverride, pApp->OverridePalette);
	// Don't add unknown palettes in the list!
	int nIdx = SendDlgItemMessage(mh_Child, lbColorsOverride, CB_FINDSTRINGEXACT, -1, (LPARAM)pApp->szPaletteName);
	SendDlgItemMessage(mh_Child, lbColorsOverride, CB_SETCURSEL, klMax(nIdx, 0), 0);

	checkDlgButton(mh_Child, cbClipboardOverride, pApp->OverrideClipboard);
	//
	checkDlgButton(mh_Child, cbCTSDetectLineEnd, pApp->isCTSDetectLineEnd);
	checkDlgButton(mh_Child, cbCTSBashMargin, pApp->isCTSBashMargin);
	checkDlgButton(mh_Child, cbCTSTrimTrailing, pApp->isCTSTrimTrailing);
	b = pApp->isCTSEOL;
	CSetDlgLists::FillListBoxItems(GetDlgItem(mh_Child, lbCTSEOL), CSetDlgLists::eCRLF, b, false);

	//
	PasteLinesMode mode;
	mode = pApp->isPasteAllLines;
	checkRadioButton(mh_Child, rPasteM1MultiLine, rPasteM1Nothing,
		(mode == plm_FirstLine) ? rPasteM1FirstLine
		: (mode == plm_SingleLine) ? rPasteM1SingleLine
		: (mode == plm_Nothing) ? rPasteM1Nothing
		: rPasteM1MultiLine);
	mode = pApp->isPasteFirstLine;
	checkRadioButton(mh_Child, rPasteM2MultiLine, rPasteM2Nothing,
		(mode == plm_MultiLine) ? rPasteM2MultiLine
		: (mode == plm_FirstLine) ? rPasteM2FirstLine
		: (mode == plm_Nothing) ? rPasteM2Nothing
		: rPasteM2SingleLine);
	//
	checkDlgButton(mh_Child, cbCTSClickPromptPosition, pApp->isCTSClickPromptPosition);
	//
	checkDlgButton(mh_Child, cbCTSDeleteLeftWord, pApp->isCTSDeleteLeftWord);

}

void CSetPgApps::DoEnableControls(WORD nGroupCtrlId)
{
	bool bAllowed = false;

	const AppSettings* pApp = NULL;
	int iCur = (int)SendDlgItemMessage(mh_Dlg, lbAppDistinct, LB_GETCURSEL, 0,0);
	if (iCur >= 0)
		pApp = gpSet->GetAppSettings(iCur);
	if (pApp && pApp->AppNames)
	{
		bAllowed = true;
	}

	for (size_t i = 0; i < countof(DistinctControls); i++)
	{
		if (nGroupCtrlId && (nGroupCtrlId != DistinctControls[i].nOverrideID))
			continue;

		bool bEnabled = bAllowed
			? (DistinctControls[i].nOverrideID ? isChecked2(mh_Child, DistinctControls[i].nOverrideID) : true)
			: FALSE;

		HWND hDlgCtrl = DistinctControls[i].nOverrideID ? mh_Child : mh_Dlg;

		if (DistinctControls[i].nOverrideID)
		{
			enableDlgItem(hDlgCtrl, DistinctControls[i].nOverrideID, bAllowed);
			if (!bAllowed)
				checkDlgButton(hDlgCtrl, DistinctControls[i].nOverrideID, BST_UNCHECKED);
		}

		_ASSERTE(DistinctControls[i].nCtrls[countof(DistinctControls[i].nCtrls)-1]==0 && "Overflow check of nCtrls[]")

		CSetDlgLists::EnableDlgItems(hDlgCtrl, DistinctControls[i].nCtrls, countof(DistinctControls[i].nCtrls), bEnabled);
	}

	InvalidateCtrl(mh_Child, FALSE);
}

LRESULT CSetPgApps::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	int iCur = mb_SkipSelChange ? -1 : (int)SendDlgItemMessage(hDlg, lbAppDistinct, LB_GETCURSEL, 0,0);
	AppSettings* pApp = (iCur < 0) ? NULL : gpSet->GetAppSettingsPtr(iCur);
	_ASSERTE((iCur<0) || (pApp && pApp->AppNames));

	if (pApp)
	{
		switch (nCtrlId)
		{
		case tAppDistinctName:
			if (!mb_SkipEditChange)
			{
				_ASSERTE(pApp && pApp->AppNames);
				wchar_t* pszApps = NULL;
				if (GetString(hDlg, nCtrlId, &pszApps))
				{
					pApp->SetNames(pszApps);
					MSetter lockSelChange(&mb_SkipSelChange);
					SetListAppName(pApp, iCur+1, iCur);
				}
				SafeFree(pszApps);
			} // tAppDistinctName
			break;

		case tCursorFixedSize:
		case tInactiveCursorFixedSize:
		case tCursorMinSize:
		case tInactiveCursorMinSize:
			if (pApp)
			{
				mb_Redraw = CSetPgCursor::OnEditChangedCursor(mh_Child, nCtrlId, pApp);
			} //case tCursorFixedSize, tInactiveCursorFixedSize, tCursorMinSize, tInactiveCursorMinSize:
			break;

		}
	}

	return 0;
}

INT_PTR CSetPgApps::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	if (nCtrlId == lbAppDistinct)
	{
		if (!mb_SkipSelChange)
		{
			OnAppSelectionChanged();
		}

	} // if (CB == lbAppDistinct)
	else
	{
		int iCur = mb_SkipSelChange ? -1 : (int)SendDlgItemMessage(hDlg, lbAppDistinct, LB_GETCURSEL, 0,0);
		AppSettings* pApp = (iCur < 0) ? NULL : gpSet->GetAppSettingsPtr(iCur);
		_ASSERTE((iCur<0) || (pApp && pApp->AppNames));

		if (!pApp)
		{
			_ASSERTE(pApp);
			return 0;
		}

		switch (nCtrlId)
		{
		case lbExtendFontBoldIdx:
			pApp->nFontBoldColor = GetNumber(mh_Child, nCtrlId);
			if (pApp->isExtendFonts)
				mb_Redraw = true;
			break;
		case lbExtendFontItalicIdx:
			pApp->nFontItalicColor = GetNumber(mh_Child, nCtrlId);
			if (pApp->isExtendFonts)
				mb_Redraw = true;
			break;
		case lbExtendFontNormalIdx:
			pApp->nFontNormalColor = GetNumber(mh_Child, nCtrlId);
			if (pApp->isExtendFonts)
				mb_Redraw = true;
			break;

		case lbColorsOverride:
			{
				HWND hList = GetDlgItem(mh_Child, nCtrlId);
				INT_PTR nIdx = SendMessage(hList, CB_GETCURSEL, 0, 0);
				if (nIdx >= 0)
				{
					INT_PTR nLen = SendMessage(hList, CB_GETLBTEXTLEN, nIdx, 0);
					wchar_t* pszText = (nLen > 0) ? (wchar_t*)calloc((nLen+1),sizeof(wchar_t)) : NULL;
					if (pszText)
					{
						SendMessage(hList, CB_GETLBTEXT, nIdx, (LPARAM)pszText);
						int iPal = (nIdx == 0) ? -1 : gpSet->PaletteGetIndex(pszText);
						if ((nIdx == 0) || (iPal >= 0))
						{
							pApp->SetPaletteName((nIdx == 0) ? L"" : pszText);

							_ASSERTE(iCur>=0 && iCur<gpSet->AppCount /*&& gpSet->AppColors*/);
							const ColorPalette* pPal = gpSet->PaletteGet(iPal);
							if (pPal)
							{
								//memmove(gpSet->AppColors[iCur]->Colors, pPal->Colors, sizeof(pPal->Colors));
								//gpSet->AppColors[iCur]->FadeInitialized = false;

								BOOL bTextAttr = (pApp->nTextColorIdx != pPal->nTextColorIdx) || (pApp->nBackColorIdx != pPal->nBackColorIdx);
								pApp->nTextColorIdx = pPal->nTextColorIdx;
								pApp->nBackColorIdx = pPal->nBackColorIdx;
								BOOL bPopupAttr = (pApp->nPopTextColorIdx != pPal->nPopTextColorIdx) || (pApp->nPopBackColorIdx != pPal->nPopBackColorIdx);
								pApp->nPopTextColorIdx = pPal->nPopTextColorIdx;
								pApp->nPopBackColorIdx = pPal->nPopBackColorIdx;
								pApp->isExtendColors = pPal->isExtendColors;
								pApp->nExtendColorIdx = pPal->nExtendColorIdx;
								if (bTextAttr || bPopupAttr)
								{
									gpSetCls->UpdateTextColorSettings(bTextAttr, bPopupAttr, pApp);
								}
								mb_Redraw = true;
							}
							else
							{
								_ASSERTE(pPal!=NULL);
							}
						}
					}
				}
			} // lbColorsOverride
			break;

		case lbCTSEOL:
			{
				BYTE eol = 0;
				CSetDlgLists::GetListBoxItem(mh_Child, lbCTSEOL, CSetDlgLists::eCRLF, eol);
				pApp->isCTSEOL = eol;
			} // lbCTSEOL
			break;

		}
	}

	return 0;
}

void CSetPgApps::OnAppSelectionChanged()
{
	const AppSettings* pApp = NULL;
	//while ((pApp = gpSet->GetAppSettings(nApp)) && pApp->AppNames)
	int iCur = (int)SendDlgItemMessage(mh_Dlg, lbAppDistinct, LB_GETCURSEL, 0,0);
	if (iCur >= 0)
		pApp = gpSet->GetAppSettings(iCur);
	if (pApp && pApp->AppNames)
	{
		if (!mb_SkipEditSet)
		{
			MSetter lockEditChange(&mb_SkipEditChange);
			SetDlgItemText(mh_Dlg, tAppDistinctName, pApp->AppNames);
		}

		DoFillControls(pApp);
	}
	else
	{
		SetDlgItemText(mh_Dlg, tAppDistinctName, L"");
		checkRadioButton(mh_Dlg, rbAppDistinctElevatedOn, rbAppDistinctElevatedIgnore, rbAppDistinctElevatedIgnore);
	}

	DoEnableControls(0);
}
