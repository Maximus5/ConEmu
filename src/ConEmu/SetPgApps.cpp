
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

CSetPgApps::CSetPgApps()
	: mb_SkipEditChange(false)
	, mb_SkipEditSet(false)
	, mp_DlgDistinct2(NULL)
	, mp_DpiDistinct2(NULL)
{
}

CSetPgApps::~CSetPgApps()
{
	if (mp_DpiDistinct2)
		mp_DpiDistinct2->Detach();
	SafeDelete(mp_DpiDistinct2);
	SafeDelete(mp_DlgDistinct2);
}

LRESULT CSetPgApps::OnInitDialog(HWND hDlg, bool abForceReload)
{
	//mn_AppsEnableControlsMsg = RegisterWindowMessage(L"ConEmu::AppsEnableControls");

	if (abForceReload)
	{
		int nTab[2] = {4*4, 7*4}; // represent the number of quarters of the average character width for the font
		SendDlgItemMessage(hDlg, lbAppDistinct, LB_SETTABSTOPS, countof(nTab), (LPARAM)nTab);

		LONG_PTR nStyles = GetWindowLongPtr(GetDlgItem(hDlg, lbAppDistinct), GWL_STYLE);
		if (!(nStyles & LBS_NOTIFY))
			SetWindowLongPtr(GetDlgItem(hDlg, lbAppDistinct), GWL_STYLE, nStyles|LBS_NOTIFY);
	}

	PageDlgProc(hDlg, abForceReload ? WM_INITDIALOG : mn_ActivateTabMsg, 0, 0);

	return 0;
}

void CSetPgApps::ProcessDpiChange(ConEmuSetupPages* p, CDpiForDialog* apDpi)
{
	if (!this || !p || !p->hPage || !p->pDpiAware || !apDpi)
		return;

	CSetPgBase::ProcessDpiChange(p, apDpi);

	if (mp_DpiDistinct2)
	{
		HWND hHolder = GetDlgItem(p->hPage, tAppDistinctHolder);
		RECT rcPos = {}; GetWindowRect(hHolder, &rcPos);
		MapWindowPoints(NULL, p->hPage, (LPPOINT)&rcPos, 2);

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
	bool bRedraw = false, bRefill = false;

	#define UM_DISTINCT_ENABLE (WM_APP+32)
	#define UM_FILL_CONTROLS (WM_APP+33)

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
			gbPastingOverride, cbClipShiftIns, cbClipCtrlV,
			gbPromptOverride, cbCTSClickPromptPosition, cbCTSDeleteLeftWord}},
		{cbBgImageOverride, {cbBgImage, tBgImage, bBgImage, lbBgPlacement}},
	};

	HWND hChild = GetDlgItem(hDlg, IDD_SPG_APPDISTINCT2);

	if (!hChild)
	{
		if ((messg == WM_INITDIALOG) || (messg == mn_ActivateTabMsg))
		{
			LogString(L"Creating child dialog IDD_SPG_APPDISTINCT2");
			SafeDelete(mp_DlgDistinct2); // JIC

			mp_DlgDistinct2 = CDynDialog::ShowDialog(IDD_SPG_APPDISTINCT2, hDlg, pageOpProc_AppsChild, 0/*dwInitParam*/);
			HWND hChild = mp_DlgDistinct2 ? mp_DlgDistinct2->mh_Dlg : NULL;

			if (!hChild)
			{
				EnableWindow(hDlg, FALSE);
				MBoxAssert(hChild && "CreateDialogParam(IDD_SPG_APPDISTINCT2) failed");
				return 0;
			}
			SetWindowLongPtr(hChild, GWLP_ID, IDD_SPG_APPDISTINCT2);

			if (!mp_DpiDistinct2 && mp_ParentDpi)
			{
				mp_DpiDistinct2 = new CDpiForDialog();
				mp_DpiDistinct2->Attach(hChild, hDlg, mp_DlgDistinct2);
			}

			HWND hHolder = GetDlgItem(hDlg, tAppDistinctHolder);
			RECT rcPos = {}; GetWindowRect(hHolder, &rcPos);
			MapWindowPoints(NULL, hDlg, (LPPOINT)&rcPos, 2);
			POINT ptScroll = {};
			HWND hEnd = GetDlgItem(hChild,stAppDistinctBottom);
			MapWindowPoints(hEnd, hChild, &ptScroll, 1);
			ShowWindow(hEnd, SW_HIDE);

			SCROLLINFO si = {sizeof(si), SIF_ALL};
			si.nMax = ptScroll.y - (rcPos.bottom - rcPos.top);
			RECT rcChild = {}; GetWindowRect(GetDlgItem(hChild, DistinctControls[1].nOverrideID), &rcChild);
			si.nPage = rcChild.bottom - rcChild.top;
			SetScrollInfo(hChild, SB_VERT, &si, FALSE);

			MoveWindowRect(hChild, rcPos);

			ShowWindow(hHolder, SW_HIDE);
			ShowWindow(hChild, SW_SHOW);
		}

		//_ASSERTE(hChild && IsWindow(hChild));
	}



	if ((messg == WM_INITDIALOG) || (messg == mn_ActivateTabMsg))
	{
		MSetter lockSelChange(&mb_SkipSelChange);
		bool bForceReload = (messg == WM_INITDIALOG);

		if (bForceReload || !mb_SkipEditSet)
		{
			const ColorPalette* pPal;

			SendMessage(GetDlgItem(hChild, lbColorsOverride), CB_RESETCONTENT, 0, 0);
			//LPCWSTR pszCurPal = CLngRc::getRsrc(lng_CurClrScheme/*"<Current color scheme>"*/, countof(szCurrentScheme));
			//SendDlgItemMessage(hChild, lbColorsOverride, CB_ADDSTRING, 0, (LPARAM)pszCurPal);

			int iCurPalette = 0;
			for (int i = 0; (pPal = gpSet->PaletteGet(i)) != NULL; i++)
			{
				SendDlgItemMessage(hChild, lbColorsOverride, CB_ADDSTRING, 0, (LPARAM)pPal->pszName);
				if ((!iCurPalette) && (lstrcmp(pPal->pszName, gsDefaultColorScheme) == 0))
					iCurPalette = i;
			}

			SendDlgItemMessage(hChild, lbColorsOverride, CB_SETCURSEL, iCurPalette, 0);

			CSetDlgLists::FillListBox(hChild, lbExtendFontBoldIdx, CSetDlgLists::eColorIdx);
			CSetDlgLists::FillListBox(hChild, lbExtendFontItalicIdx, CSetDlgLists::eColorIdx);
			CSetDlgLists::FillListBox(hChild, lbExtendFontNormalIdx, CSetDlgLists::eColorIdx);
		}

		// Сброс ранее загруженного списка (ListBox: lbAppDistinct)
		SendDlgItemMessage(hDlg, lbAppDistinct, LB_RESETCONTENT, 0,0);

		//if (abForceReload)
		//{
		//	// Обновить группы команд
		//	gpSet->LoadCmdTasks(NULL, true);
		//}

		int nApp = 0;
		wchar_t szItem[1024];
		const AppSettings* pApp = NULL;
		while ((pApp = gpSet->GetAppSettings(nApp)) && pApp->AppNames)
		{
			_wsprintf(szItem, SKIPLEN(countof(szItem)) L"%i\t%s\t", nApp+1,
				(pApp->Elevated == 1) ? L"E" : (pApp->Elevated == 2) ? L"N" : L"*");
			int nPrefix = lstrlen(szItem);
			lstrcpyn(szItem+nPrefix, pApp->AppNames, countof(szItem)-nPrefix);

			INT_PTR iIndex = SendDlgItemMessage(hDlg, lbAppDistinct, LB_ADDSTRING, 0, (LPARAM)szItem);
			UNREFERENCED_PARAMETER(iIndex);
			//SendDlgItemMessage(hDlg, lbAppDistinct, LB_SETITEMDATA, iIndex, (LPARAM)pApp);

			nApp++;
		}

		lockSelChange.Unlock();

		if (!mb_SkipSelChange)
		{
			PageDlgProc(hDlg, WM_COMMAND, MAKELONG(lbAppDistinct,LBN_SELCHANGE), 0);
		}

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
	case UM_FILL_CONTROLS:
		if ((((HWND)wParam) == hDlg) && lParam) // arg check
		{
			const AppSettings* pApp = (const AppSettings*)lParam;

			checkRadioButton(hDlg, rbAppDistinctElevatedOn, rbAppDistinctElevatedIgnore,
				(pApp->Elevated == 1) ? rbAppDistinctElevatedOn :
				(pApp->Elevated == 2) ? rbAppDistinctElevatedOff : rbAppDistinctElevatedIgnore);

			BYTE b;
			wchar_t temp[MAX_PATH];

			checkDlgButton(hChild, cbExtendFontsOverride, pApp->OverrideExtendFonts);
			checkDlgButton(hChild, cbExtendFonts, pApp->isExtendFonts);
			_wsprintf(temp, SKIPLEN(countof(temp))(pApp->nFontBoldColor<16) ? L"%2i" : L"None", pApp->nFontBoldColor);
			CSetDlgLists::SelectStringExact(hChild, lbExtendFontBoldIdx, temp);
			_wsprintf(temp, SKIPLEN(countof(temp))(pApp->nFontItalicColor<16) ? L"%2i" : L"None", pApp->nFontItalicColor);
			CSetDlgLists::SelectStringExact(hChild, lbExtendFontItalicIdx, temp);
			_wsprintf(temp, SKIPLEN(countof(temp))(pApp->nFontNormalColor<16) ? L"%2i" : L"None", pApp->nFontNormalColor);
			CSetDlgLists::SelectStringExact(hChild, lbExtendFontNormalIdx, temp);

			checkDlgButton(hChild, cbCursorOverride, pApp->OverrideCursor);
			CSetPgCursor::InitCursorCtrls(hChild, pApp);

			checkDlgButton(hChild, cbColorsOverride, pApp->OverridePalette);
			CSetDlgLists::SelectStringExact(hChild, lbColorsOverride, pApp->szPaletteName);

			checkDlgButton(hChild, cbClipboardOverride, pApp->OverrideClipboard);
			//
			checkDlgButton(hChild, cbCTSDetectLineEnd, pApp->isCTSDetectLineEnd);
			checkDlgButton(hChild, cbCTSBashMargin, pApp->isCTSBashMargin);
			checkDlgButton(hChild, cbCTSTrimTrailing, pApp->isCTSTrimTrailing);
			b = pApp->isCTSEOL;
			CSetDlgLists::GetListBoxItem(hChild, lbCTSEOL, CSetDlgLists::eCRLF, b);
			//
			checkDlgButton(hChild, cbClipShiftIns, pApp->isPasteAllLines);
			checkDlgButton(hChild, cbClipCtrlV, pApp->isPasteFirstLine);
			//
			checkDlgButton(hChild, cbCTSClickPromptPosition, pApp->isCTSClickPromptPosition);
			//
			checkDlgButton(hChild, cbCTSDeleteLeftWord, pApp->isCTSDeleteLeftWord);


			checkDlgButton(hChild, cbBgImageOverride, pApp->OverrideBgImage);
			checkDlgButton(hChild, cbBgImage, BST(pApp->isShowBgImage));
			SetDlgItemText(hChild, tBgImage, pApp->sBgImage);
			b = pApp->nBgOperation;
			CSetDlgLists::GetListBoxItem(hChild, lbBgPlacement, CSetDlgLists::eBgOper, b);

		} // UM_FILL_CONTROLS
		break;
	case UM_DISTINCT_ENABLE:
		if (((HWND)wParam) == hDlg) // arg check
		{
			_ASSERTE(hChild && IsWindow(hChild));

			WORD nID = (WORD)(lParam & 0xFFFF);
			bool bAllowed = false;

			const AppSettings* pApp = NULL;
			int iCur = (int)SendDlgItemMessage(hDlg, lbAppDistinct, LB_GETCURSEL, 0,0);
			if (iCur >= 0)
				pApp = gpSet->GetAppSettings(iCur);
			if (pApp && pApp->AppNames)
			{
				bAllowed = true;
			}

			for (size_t i = 0; i < countof(DistinctControls); i++)
			{
				if (nID && (nID != DistinctControls[i].nOverrideID))
					continue;

				BOOL bEnabled = bAllowed
					? (DistinctControls[i].nOverrideID ? isChecked(hChild, DistinctControls[i].nOverrideID) : TRUE)
					: FALSE;

				HWND hDlg = DistinctControls[i].nOverrideID ? hChild : hDlg;

				if (DistinctControls[i].nOverrideID)
				{
					EnableWindow(GetDlgItem(hDlg, DistinctControls[i].nOverrideID), bAllowed);
					if (!bAllowed)
						checkDlgButton(hDlg, DistinctControls[i].nOverrideID, BST_UNCHECKED);
				}

				_ASSERTE(DistinctControls[i].nCtrls[countof(DistinctControls[i].nCtrls)-1]==0 && "Overflow check of nCtrls[]")

				CSetDlgLists::EnableDlgItems(hDlg, DistinctControls[i].nCtrls, countof(DistinctControls[i].nCtrls), bEnabled);
			}

			InvalidateCtrl(hChild, FALSE);
		} // UM_DISTINCT_ENABLE
		break;
	case WM_COMMAND:
		{
			_ASSERTE(hChild && IsWindow(hChild));

			if (HIWORD(wParam) == BN_CLICKED)
			{
				BOOL bChecked;
				WORD CB = LOWORD(wParam);

				int iCur = mb_SkipSelChange ? -1 : (int)SendDlgItemMessage(hDlg, lbAppDistinct, LB_GETCURSEL, 0,0);
				AppSettings* pApp = (iCur < 0) ? NULL : gpSet->GetAppSettingsPtr(iCur);
				_ASSERTE((iCur<0) || (pApp && pApp->AppNames));

				switch (CB)
				{
				case cbAppDistinctReload:
					{
						if (MsgBox(L"Warning! All unsaved changes will be lost!\n\nReload Apps from settings?",
								MB_YESNO|MB_ICONEXCLAMATION, gpConEmu->GetDefaultTitle(), ghOpWnd) != IDYES)
							break;

						// Перезагрузить App distinct
						gpSet->LoadAppsSettings(NULL, true);

						// Обновить список на экране
						OnInitDialog(hDlg, true);

					} // cbAppDistinctReload
					break;
				case cbAppDistinctAdd:
					{
						int iCount = (int)SendDlgItemMessage(hDlg, lbAppDistinct, LB_GETCOUNT, 0,0);
						AppSettings* pNew = gpSet->GetAppSettingsPtr(iCount, TRUE);
						UNREFERENCED_PARAMETER(pNew);

						MSetter lockSelChange(&mb_SkipSelChange);

						OnInitDialog(hDlg, false);

						SendDlgItemMessage(hDlg, lbAppDistinct, LB_SETCURSEL, iCount, 0);

						lockSelChange.Unlock();

						PageDlgProc(hDlg, WM_COMMAND, MAKELONG(lbAppDistinct,LBN_SELCHANGE), 0);
					} // cbAppDistinctAdd
					break;

				case cbAppDistinctDel:
					{
						int iCur = (int)SendDlgItemMessage(hDlg, lbAppDistinct, LB_GETCURSEL, 0,0);
						if (iCur < 0)
							break;

						const AppSettings* p = gpSet->GetAppSettingsPtr(iCur);
						if (!p)
							break;

						if (MsgBox(L"Delete application?", MB_YESNO|MB_ICONQUESTION, p->AppNames, ghOpWnd) != IDYES)
							break;

						gpSet->AppSettingsDelete(iCur);

						MSetter lockSelChange(&mb_SkipSelChange);

						OnInitDialog(hDlg, false);

						int iCount = (int)SendDlgItemMessage(hDlg, lbAppDistinct, LB_GETCOUNT, 0,0);

						lockSelChange.Unlock();

						SendDlgItemMessage(hDlg, lbAppDistinct, LB_SETCURSEL, min(iCur,(iCount-1)), 0);
						PageDlgProc(hDlg, WM_COMMAND, MAKELONG(lbAppDistinct,LBN_SELCHANGE), 0);

					} // cbAppDistinctDel
					break;

				case cbAppDistinctUp:
				case cbAppDistinctDown:
					{
						int iCur, iChg;
						iCur = (int)SendDlgItemMessage(hDlg, lbAppDistinct, LB_GETCURSEL, 0,0);
						if (iCur < 0)
							break;
						if (CB == cbAppDistinctUp)
						{
							if (!iCur)
								break;
							iChg = iCur - 1;
						}
						else
						{
							iChg = iCur + 1;
							if (iChg >= (int)SendDlgItemMessage(hDlg, lbAppDistinct, LB_GETCOUNT, 0,0))
								break;
						}

						if (!gpSet->AppSettingsXch(iCur, iChg))
							break;

						MSetter lockSelChange(&mb_SkipSelChange);

						OnInitDialog(hDlg, false);

						lockSelChange.Unlock();

						SendDlgItemMessage(hDlg, lbAppDistinct, LB_SETCURSEL, iChg, 0);
					} // cbAppDistinctUp, cbAppDistinctDown
					break;

				case rbAppDistinctElevatedOn:
				case rbAppDistinctElevatedOff:
				case rbAppDistinctElevatedIgnore:
					if (pApp)
					{
						pApp->Elevated = isChecked(hDlg, rbAppDistinctElevatedOn) ? 1
							: isChecked(hDlg, rbAppDistinctElevatedOff) ? 2
							: 0;
						bRefill = bRedraw = true;
					}
					break;


				case cbColorsOverride:
					bChecked = isChecked(hChild, CB);
					//EnableWindow(GetDlgItem(hChild, lbColorsOverride), bChecked);
					PageDlgProc(hDlg, UM_DISTINCT_ENABLE, (WPARAM)hDlg, cbColorsOverride);
					if (pApp)
					{
						pApp->OverridePalette = bChecked;
						bRedraw = true;
						// Обновить палитры
						gpSet->PaletteSetStdIndexes();
						// Обновить консоли (считаем, что меняется только TextAttr, Popup не трогать
						gpSetCls->UpdateTextColorSettings(TRUE, FALSE, pApp);
					}
					break;

				case cbCursorOverride:
					bChecked = isChecked(hChild, CB);
					//EnableWindow(GetDlgItem(hDlg, rCursorV), bChecked);
					//EnableWindow(GetDlgItem(hDlg, rCursorH), bChecked);
					//EnableWindow(GetDlgItem(hDlg, cbCursorColor), bChecked);
					//EnableWindow(GetDlgItem(hDlg, cbCursorBlink), bChecked);
					//EnableWindow(GetDlgItem(hDlg, cbBlockInactiveCursor), bChecked);
					//EnableWindow(GetDlgItem(hDlg, cbCursorIgnoreSize), bChecked);
					PageDlgProc(hDlg, UM_DISTINCT_ENABLE, (WPARAM)hDlg, cbCursorOverride);
					if (pApp)
					{
						pApp->OverrideCursor = bChecked;
						bRedraw = true;
					}
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
					OnButtonClicked_Cursor(hChild, wParam, lParam, pApp);
					bRedraw = true;
					break;
				//case rCursorV:
				//case rCursorH:
				//case rCursorB:
				//	if (pApp)
				//	{
				//		pApp->isCursorType = (CB - rCursorH); // isChecked(hChild, rCursorV);
				//		bRedraw = true;
				//	}
				//	break;
				//case cbCursorColor:
				//	if (pApp)
				//	{
				//		pApp->isCursorColor = isChecked(hChild, CB);
				//		bRedraw = true;
				//	}
				//	break;
				//case cbCursorBlink:
				//	if (pApp)
				//	{
				//		pApp->isCursorBlink = isChecked(hChild, CB);
				//		//if (!gpSet->AppStd.isCursorBlink) // если мигание отключается - то курсор может "замереть" в погашенном состоянии.
				//		//	gpConEmu->ActiveCon()->Invalidate();
				//		bRedraw = true;
				//	}
				//	break;
				//case cbBlockInactiveCursor:
				//	if (pApp)
				//	{
				//		pApp->isCursorBlockInactive = isChecked(hChild, CB);
				//		bRedraw = true;
				//	}
				//	break;
				//case cbCursorIgnoreSize:
				//	if (pApp)
				//	{
				//		pApp->isCursorIgnoreSize = isChecked(hChild, CB);
				//		bRedraw = true;
				//	}
				//	break;

				case cbExtendFontsOverride:
					bChecked = isChecked(hChild, CB);
					//EnableWindow(GetDlgItem(hDlg, cbExtendFonts), bChecked);
					//EnableWindow(GetDlgItem(hDlg, lbExtendFontBoldIdx), bChecked);
					//EnableWindow(GetDlgItem(hDlg, lbExtendFontItalicIdx), bChecked);
					//EnableWindow(GetDlgItem(hDlg, lbExtendFontNormalIdx), bChecked);
					PageDlgProc(hDlg, UM_DISTINCT_ENABLE, (WPARAM)hDlg, cbExtendFontsOverride);
					if (!mb_SkipSelChange)
					{
						pApp->OverrideExtendFonts = isChecked(hChild, CB);
						bRedraw = true;
					}
					break;
				case cbExtendFonts:
					if (pApp)
					{
						pApp->isExtendFonts = isChecked(hChild, CB);
						bRedraw = true;
					}
					break;


				case cbClipboardOverride:
					bChecked = isChecked(hChild, CB);
					//EnableWindow(GetDlgItem(hDlg, cbClipShiftIns), bChecked);
					//EnableWindow(GetDlgItem(hDlg, cbClipCtrlV), bChecked);
					PageDlgProc(hDlg, UM_DISTINCT_ENABLE, (WPARAM)hDlg, cbClipboardOverride);
					if (!mb_SkipSelChange)
					{
						pApp->OverrideClipboard = isChecked(hChild, CB);
					}
					break;
				case cbClipShiftIns:
					if (pApp)
					{
						pApp->isPasteAllLines = isChecked(hChild, CB);
					}
					break;
				case cbClipCtrlV:
					if (pApp)
					{
						pApp->isPasteFirstLine = isChecked(hChild, CB);
					}
					break;
				case cbCTSDetectLineEnd:
					if (pApp)
					{
						pApp->isCTSDetectLineEnd = isChecked(hChild, CB);
					}
					break;
				case cbCTSBashMargin:
					if (pApp)
					{
						pApp->isCTSBashMargin = isChecked(hChild, CB);
					}
					break;
				case cbCTSTrimTrailing:
					if (pApp)
					{
						pApp->isCTSTrimTrailing = isChecked(hChild, CB);
					}
					break;
				case cbCTSClickPromptPosition:
					if (pApp)
					{
						pApp->isCTSClickPromptPosition = isChecked(hChild, CB);
					}
					break;
				case cbCTSDeleteLeftWord:
					if (pApp)
					{
						pApp->isCTSDeleteLeftWord = isChecked(hChild, CB);
					}
					break;

				case cbBgImageOverride:
					bChecked = isChecked(hChild, CB);
					PageDlgProc(hDlg, UM_DISTINCT_ENABLE, (WPARAM)hDlg, cbBgImageOverride);
					if (!mb_SkipSelChange)
					{
						//pApp->OverrideBackground = isChecked(hChild, CB);
					}
					break;
				case cbBgImage:
					if (pApp)
					{
						pApp->isShowBgImage = isChecked(hChild, CB);
					}
					break;
				case bBgImage:
					if (pApp)
					{
						wchar_t temp[MAX_PATH], edt[MAX_PATH];
						if (!GetDlgItemText(hChild, tBgImage, edt, countof(edt)))
							edt[0] = 0;
						ExpandEnvironmentStrings(edt, temp, countof(temp));
						OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
						ofn.lStructSize=sizeof(ofn);
						ofn.hwndOwner = ghOpWnd;
						ofn.lpstrFilter = L"All images (*.bmp,*.jpg,*.png)\0*.bmp;*.jpg;*.jpe;*.jpeg;*.png\0Bitmap images (*.bmp)\0*.bmp\0JPEG images (*.jpg)\0*.jpg;*.jpe;*.jpeg\0PNG images (*.png)\0*.png\0\0";
						ofn.nFilterIndex = 1;
						ofn.lpstrFile = temp;
						ofn.nMaxFile = countof(temp);
						ofn.lpstrTitle = L"Choose background image";
						ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
									| OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;

						if (GetOpenFileName(&ofn))
						{
							TODO("LoadBackgroundFile");
							//if (LoadBackgroundFile(temp, true))
							{
								bool bUseEnvVar = false;
								size_t nEnvLen = _tcslen(gpConEmu->ms_ConEmuExeDir);
								if (_tcslen(temp) > nEnvLen && temp[nEnvLen] == L'\\')
								{
									temp[nEnvLen] = 0;
									if (lstrcmpi(temp, gpConEmu->ms_ConEmuExeDir) == 0)
										bUseEnvVar = true;
									temp[nEnvLen] = L'\\';
								}
								if (bUseEnvVar)
								{
									wcscpy_c(pApp->sBgImage, L"%ConEmuDir%");
									wcscat_c(pApp->sBgImage, temp + _tcslen(gpConEmu->ms_ConEmuExeDir));
								}
								else
								{
									wcscpy_c(pApp->sBgImage, temp);
								}
								SetDlgItemText(hChild, tBgImage, pApp->sBgImage);
								gpConEmu->Update(true);
							}
						}
					} // bBgImage
					break;
				}
			} // if (HIWORD(wParam) == BN_CLICKED)
			else if (HIWORD(wParam) == EN_CHANGE)
			{
				WORD ID = LOWORD(wParam);
				int iCur = mb_SkipSelChange ? -1 : (int)SendDlgItemMessage(hDlg, lbAppDistinct, LB_GETCURSEL, 0,0);
				AppSettings* pApp = (iCur < 0) ? NULL : gpSet->GetAppSettingsPtr(iCur);
				_ASSERTE((iCur<0) || (pApp && pApp->AppNames));

				if (pApp)
				{
					switch (ID)
					{
					case tAppDistinctName:
						if (!mb_SkipEditChange)
						{
							AppSettings* pApp = gpSet->GetAppSettingsPtr(iCur);
							if (!pApp || !pApp->AppNames)
							{
								_ASSERTE(pApp && pApp->AppNames);
							}
							else
							{
								wchar_t* pszApps = NULL;
								if (GetString(hDlg, ID, &pszApps))
								{
									pApp->SetNames(pszApps);
									bRefill = bRedraw = true;
								}
								SafeFree(pszApps);
							}
						} // tAppDistinctName
						break;

					case tCursorFixedSize:
					case tInactiveCursorFixedSize:
					case tCursorMinSize:
					case tInactiveCursorMinSize:
						if (pApp)
						{
							bRedraw = CSetPgCursor::OnEditChanged(hChild, wParam, lParam, pApp);
						} //case tCursorFixedSize, tInactiveCursorFixedSize, tCursorMinSize, tInactiveCursorMinSize:
						break;

					case tBgImage:
						if (pApp)
						{
							wchar_t temp[MAX_PATH];
							GetDlgItemText(hChild, tBgImage, temp, countof(temp));

							if (wcscmp(temp, pApp->sBgImage))
							{
								TODO("LoadBackgroundFile");
								//if (LoadBackgroundFile(temp, true))
								{
									wcscpy_c(pApp->sBgImage, temp);
									gpConEmu->Update(true);
								}
							}
						} // tBgImage
						break;
					}
				}
			} // if (HIWORD(wParam) == EN_CHANGE)
			else if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				WORD CB = LOWORD(wParam);

				if (CB == lbAppDistinct)
				{
					if (mb_SkipSelChange)
						break; // WM_COMMAND

					const AppSettings* pApp = NULL;
					//while ((pApp = gpSet->GetAppSettings(nApp)) && pApp->AppNames)
					int iCur = (int)SendDlgItemMessage(hDlg, lbAppDistinct, LB_GETCURSEL, 0,0);
					if (iCur >= 0)
						pApp = gpSet->GetAppSettings(iCur);
					if (pApp && pApp->AppNames)
					{
						if (!mb_SkipEditSet)
						{
							MSetter lockEditChange(&mb_SkipEditChange);
							SetDlgItemText(hDlg, tAppDistinctName, pApp->AppNames);
						}

						PageDlgProc(hDlg, UM_FILL_CONTROLS, (WPARAM)hDlg, (LPARAM)pApp);
					}
					else
					{
						SetDlgItemText(hDlg, tAppDistinctName, L"");
						checkRadioButton(hDlg, rbAppDistinctElevatedOn, rbAppDistinctElevatedIgnore, rbAppDistinctElevatedIgnore);
					}

					MSetter lockSelChange(&mb_SkipSelChange);

					PageDlgProc(hDlg, UM_DISTINCT_ENABLE, (WPARAM)hDlg, 0);

				} // if (CB == lbAppDistinct)
				else
				{
					int iCur = mb_SkipSelChange ? -1 : (int)SendDlgItemMessage(hDlg, lbAppDistinct, LB_GETCURSEL, 0,0);
					AppSettings* pApp = (iCur < 0) ? NULL : gpSet->GetAppSettingsPtr(iCur);
					_ASSERTE((iCur<0) || (pApp && pApp->AppNames));

					if (pApp)
					{
						switch (CB)
						{
						case lbExtendFontBoldIdx:
						case lbExtendFontItalicIdx:
						case lbExtendFontNormalIdx:
							{
								if (CB == lbExtendFontNormalIdx)
									pApp->nFontNormalColor = GetNumber(hChild, CB);
								else if (CB == lbExtendFontBoldIdx)
									pApp->nFontBoldColor = GetNumber(hChild, CB);
								else if (CB == lbExtendFontItalicIdx)
									pApp->nFontItalicColor = GetNumber(hChild, CB);

								if (pApp->isExtendFonts)
									bRedraw = true;
							} // lbExtendFontBoldIdx, lbExtendFontItalicIdx, lbExtendFontNormalIdx
							break;

						case lbColorsOverride:
							{
								HWND hList = GetDlgItem(hChild, CB);
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
												bRedraw = true;
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
								CSetDlgLists::GetListBoxItem(hChild, lbCTSEOL, CSetDlgLists::eCRLF, eol);
								pApp->isCTSEOL = eol;
							} // lbCTSEOL
							break;

						case lbBgPlacement:
							{
								BYTE bg = 0;
								CSetDlgLists::GetListBoxItem(hChild, lbBgPlacement, CSetDlgLists::eBgOper, bg);
								pApp->nBgOperation = bg;
								TODO("LoadBackgroundFile");
								//gpSetCls->LoadBackgroundFile(gpSet->sBgImage, true);
								gpConEmu->Update(true);
							} // lbBgPlacement
							break;
						}
					}
				}
			} // if (HIWORD(wParam) == LBN_SELCHANGE)
		} // WM_COMMAND
		break;
	} // switch (messg)

	if (bRedraw)
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

	if (bRefill)
	{
		MSetter lockSelChange(&mb_SkipSelChange);
		MSetter lockEditSet(&mb_SkipEditSet);

		INT_PTR iSel = SendDlgItemMessage(hDlg, lbAppDistinct, LB_GETCURSEL, 0,0);
		OnInitDialog(hDlg, false);
		SendDlgItemMessage(hDlg, lbAppDistinct, LB_SETCURSEL, iSel,0);

		bRefill = false;
	}

	return iRc;
}
