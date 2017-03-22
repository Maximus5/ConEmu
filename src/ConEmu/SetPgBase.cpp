
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
#include "DlgItemHelper.h"
#include "DynDialog.h"
#include "LngRc.h"
#include "Options.h"
#include "OptionsClass.h"
#include "RealConsole.h"
#include "SearchCtrl.h"
#include "SetDlgColors.h"
#include "SetPgApps.h"
#include "SetPgBase.h"
#include "SetPgDebug.h"
#include "SetPgFonts.h"
#include "SetPgKeys.h"

bool CSetPgBase::mb_IgnoreEditChanged = false;

CSetPgBase::CSetPgBase()
	: mh_Dlg(NULL)
	, mh_Parent(NULL)
	, mb_SkipSelChange(false)
	, mb_DpiChanged(false)
	, mn_ActivateTabMsg(WM_APP)
	, mp_DpiAware(NULL)
	, mp_DynDialog(NULL)
	, mp_ParentDpi(NULL)
	, mp_InfoPtr(NULL)
{
	mb_IgnoreEditChanged = false;
}

CSetPgBase::~CSetPgBase()
{
	if (mp_DpiAware)
	{
		mp_DpiAware->Detach();
		SafeDelete(mp_DpiAware);
	}
	SafeDelete(mp_DynDialog);
}

HWND CSetPgBase::CreatePage(ConEmuSetupPages* p, HWND ahParent, UINT anActivateTabMsg, const CDpiForDialog* apParentDpi)
{
	wchar_t szLog[80]; _wsprintf(szLog, SKIPCOUNT(szLog) L"Creating child dialog ID=%u", p->DialogID);
	LogString(szLog);

	SafeDelete(p->pPage);

	p->pPage = p->CreateObj();
	p->pPage->InitObject(ahParent, anActivateTabMsg, apParentDpi, p);

	p->pPage->mp_DynDialog = CDynDialog::ShowDialog(p->DialogID, ahParent, pageOpProc, (LPARAM)p->pPage);
	p->hPage = p->pPage->Dlg();

	return p->hPage;
}

void CSetPgBase::InitObject(HWND ahParent, UINT anActivateTabMsg, const CDpiForDialog* apParentDpi, ConEmuSetupPages* apInfoPtr)
{
	mh_Parent = ahParent;
	mn_ActivateTabMsg = anActivateTabMsg;
	mp_ParentDpi = apParentDpi;
	mp_InfoPtr = apInfoPtr;

	if (apParentDpi)
	{
		if (!mp_DpiAware)
			mp_DpiAware = new CDpiForDialog();
		mb_DpiChanged = false;
	}
	else
	{
		SafeDelete(mp_DpiAware);
	}
}

void CSetPgBase::OnDpiChanged(const CDpiForDialog* apDpi)
{
	if (!this || !mh_Dlg || !mp_DpiAware || !apDpi)
		return;

	mb_DpiChanged = true;

	if (IsWindowVisible(mh_Dlg))
	{
		ProcessDpiChange(apDpi);
	}
}

void CSetPgBase::ProcessDpiChange(const CDpiForDialog* apDpi)
{
	if (!this || !mh_Dlg || !mp_DpiAware || !apDpi)
		return;

	HWND hPlace = GetDlgItem(mh_Parent, tSetupPagePlace);
	RECT rcClient; GetWindowRect(hPlace, &rcClient);
	MapWindowPoints(NULL, mh_Parent, (LPPOINT)&rcClient, 2);

	mb_DpiChanged = false;
	mp_DpiAware->SetDialogDPI(apDpi->GetCurDpi(), &rcClient);

}

INT_PTR CSetPgBase::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	_ASSERTE(FALSE && "ComboBox was not processed!");
	return 0;
}

INT_PTR CSetPgBase::OnCtlColorStatic(HWND hDlg, HDC hdc, HWND hCtrl, WORD nCtrlId)
{
	if ((nCtrlId >= c0 && nCtrlId <= CSetDlgColors::MAX_COLOR_EDT_ID) || (nCtrlId >= c32 && nCtrlId <= c38))
	{
		return CSetDlgColors::ColorCtlStatic(hDlg, nCtrlId, hCtrl);
	}
	else if (CDlgItemHelper::isHyperlinkCtrl(nCtrlId))
	{
		_ASSERTE(hCtrl!=NULL);
		// Check appropriate flags
		DWORD nStyle = GetWindowLong(hCtrl, GWL_STYLE);
		if (!(nStyle & SS_NOTIFY))
			SetWindowLong(hCtrl, GWL_STYLE, nStyle|SS_NOTIFY);
		// And the colors
		SetTextColor(hdc, GetSysColor(COLOR_HOTLIGHT));
		SetBkMode(hdc, TRANSPARENT);
		HBRUSH hBrush = GetSysColorBrush(COLOR_3DFACE);
		return (INT_PTR)hBrush;
	}

	/*
	SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
	HBRUSH hBrush = GetSysColorBrush(COLOR_3DFACE);
	SetBkMode((HDC)wParam, TRANSPARENT);
	return (INT_PTR)hBrush;
	*/

	return FALSE;
}

INT_PTR CSetPgBase::OnSetCursor(HWND hDlg, HWND hCtrl, WORD nCtrlId, WORD nHitTest, WORD nMouseMsg)
{
	if (nHitTest != HTCLIENT)
		return FALSE;

	#ifdef _DEBUG
	LONG_PTR wId = GetWindowLongPtr(hCtrl, GWLP_ID);
	#endif
	if (CDlgItemHelper::isHyperlinkCtrl(nCtrlId))
	{
		SetCursor(LoadCursor(NULL, IDC_HAND));
		SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
		return TRUE;
	}

	return FALSE;
}

INT_PTR CSetPgBase::OnButtonClicked(HWND hDlg, HWND hBtn, WORD nCtrlId)
{
	return gpSetCls->OnButtonClicked(hDlg, MAKELONG(nCtrlId, BN_CLICKED), (LPARAM)hBtn);
}

// Общая DlgProc на _все_ вкладки
INT_PTR CSetPgBase::pageOpProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	TabHwndIndex pgId = thi_Last;
	CSetPgBase* pObj = NULL;

	if (messg != WM_INITDIALOG)
	{
		ConEmuSetupPages* pPage = NULL;
		pgId = gpSetCls->GetPageId(hDlg, &pPage);

		if ((pgId == thi_Last) || !pPage || !pPage->pPage)
		{
			_ASSERTE(FALSE && "Page was not created properly yet");
			return TRUE;
		}

		pObj = pPage->pPage;
		_ASSERTE(pObj && (pObj->mn_ActivateTabMsg != WM_APP));
		if (!pObj)
		{
			return TRUE;
		}

		if (pObj && pObj->mp_DpiAware && pObj->mp_DpiAware->ProcessDpiMessages(hDlg, messg, wParam, lParam))
		{
			return TRUE;
		}
	}

	if ((messg == WM_INITDIALOG) || (pObj && (messg == pObj->mn_ActivateTabMsg)))
	{
		bool bInitial = (messg == WM_INITDIALOG);

		if (bInitial)
		{
			if (!lParam)
			{
				_ASSERTE(lParam != 0);
				return 0;
			}
			pObj = (CSetPgBase*)lParam;
		}

		if (!pObj || (pObj->GetPageType() < thi_Fonts) || (pObj->GetPageType() >= thi_Last))
		{
			_ASSERTE(pObj && (pObj->GetPageType() >= thi_Fonts && pObj->GetPageType() < thi_Last));
			return 0;
		}
		_ASSERTE(pObj->Dlg() == NULL || pObj->Dlg() == hDlg);

		pgId = pObj->GetPageType();

		if (bInitial)
		{
			_ASSERTE(pObj->mp_InfoPtr && pObj->mp_InfoPtr->PageIndex >= 0 && pObj->mp_InfoPtr->hPage == NULL);
			pObj->mp_InfoPtr->hPage = hDlg;
			pObj->mh_Dlg = hDlg;

			CDynDialog* pDynDialog = CDynDialog::GetDlgClass(hDlg);
			_ASSERTE(pObj->mp_DynDialog==NULL || pObj->mp_DynDialog==pDynDialog);

			#ifdef _DEBUG
			// pObj->mp_DynDialog is NULL on first WM_INIT
			if (pObj->mp_DynDialog)
			{
				_ASSERTE(pObj->mp_DynDialog->mh_Dlg == hDlg);
			}
			#endif

			HWND hPlace = GetDlgItem(pObj->mh_Parent, tSetupPagePlace);
			RECT rcClient; GetWindowRect(hPlace, &rcClient);
			MapWindowPoints(NULL, pObj->mh_Parent, (LPPOINT)&rcClient, 2);
			if (pObj->mp_DpiAware)
				pObj->mp_DpiAware->Attach(hDlg, pObj->mh_Parent, pDynDialog);
			MoveWindowRect(hDlg, rcClient);
		}
		else
		{
			_ASSERTE(pObj->mp_InfoPtr->PageIndex >= 0 && pObj->mp_InfoPtr->hPage == hDlg);
		}

		MSetter lockSelChange(&pObj->mb_SkipSelChange);

		pObj->OnInitDialog(hDlg, bInitial);

		if (bInitial)
		{
			EditIconHint_Subclass(hDlg, pObj->mh_Parent);
			gpSetCls->RegisterTipsFor(hDlg);
		}

		pObj->OnPostLocalize(hDlg);

	}
	else if ((messg == WM_HELP) || (messg == HELP_WM_HELP))
	{
		_ASSERTE(messg == WM_HELP);
		return gpSetCls->wndOpProc(hDlg, messg, wParam, lParam);
	}
	else if (pgId == thi_Apps)
	{
		// Страничка "App distinct" в некотором смысле особенная.
		// У многих контролов ИД дублируются с другими вкладками.
		CSetPgApps* pAppsPg;
		if (gpSetCls->GetPageObj(pAppsPg))
		{
			return pAppsPg->PageDlgProc(hDlg, messg, wParam, lParam);
		}
		else
		{
			_ASSERTE(pAppsPg!=NULL);
			return 0;
		}
	}
	else if (pgId == thi_Integr)
	{
		return pObj->PageDlgProc(hDlg, messg, wParam, lParam);
	}
	else if (pgId == thi_Startup)
	{
		return pObj->PageDlgProc(hDlg, messg, wParam, lParam);
	}
	else
	// All other messages
	switch (messg)
	{
		#ifdef _DEBUG
		case WM_INITDIALOG:
			// Должно быть обработано выше
			_ASSERTE(messg != WM_INITDIALOG);
			break;
		#endif

		case WM_COMMAND:
		{
			switch (HIWORD(wParam))
			{
			case BN_CLICKED:
				return CSetDlgButtons::OnButtonClicked(hDlg, wParam, lParam);

			case EN_CHANGE:
				// TODO: Remove duplicate condition!
				if (!pObj->mb_SkipSelChange && !pObj->mb_IgnoreEditChanged)
					pObj->OnEditChanged(hDlg, LOWORD(wParam));
				return 0;

			case CBN_EDITCHANGE:
			case CBN_SELCHANGE/*LBN_SELCHANGE*/:
				if (!pObj->mb_SkipSelChange)
					pObj->OnComboBox(hDlg, LOWORD(wParam), HIWORD(wParam));
				return 0;

			case LBN_DBLCLK:
				gpSetCls->OnListBoxDblClk(hDlg, wParam, lParam);
				return 0;

			case CBN_KILLFOCUS:
				if (gpSetCls->mn_LastChangingFontCtrlId && (LOWORD(wParam) == gpSetCls->mn_LastChangingFontCtrlId))
				{
					_ASSERTE(pgId == thi_Fonts);
					PostMessage(hDlg, gpSetCls->mn_MsgRecreateFont, gpSetCls->mn_LastChangingFontCtrlId, 0);
					gpSetCls->mn_LastChangingFontCtrlId = 0;
					return 0;
				}
				break;

			default:
				if (HIWORD(wParam) == 0xFFFF && LOWORD(wParam) == lbConEmuHotKeys)
				{
					dynamic_cast<CSetPgKeys*>(pObj)->OnHotkeysNotify(hDlg, wParam, 0);
				}
			} // switch (HIWORD(wParam))
		} // WM_COMMAND
		break;

		case WM_MEASUREITEM:
			return gpSetCls->OnMeasureFontItem(hDlg, messg, wParam, lParam);
		case WM_DRAWITEM:
			return gpSetCls->OnDrawFontItem(hDlg, messg, wParam, lParam);

		case WM_CTLCOLORSTATIC:
			return pObj->OnCtlColorStatic(hDlg, (HDC)wParam, (HWND)lParam, GetDlgCtrlID((HWND)lParam));

		case WM_SETCURSOR:
			return pObj->OnSetCursor(hDlg, (HWND)wParam, GetDlgCtrlID((HWND)wParam), LOWORD(lParam), HIWORD(lParam));

		case WM_HSCROLL:
		{
			if ((pgId == thi_Backgr) && (HWND)lParam == GetDlgItem(hDlg, slDarker))
			{
				int newV = SendDlgItemMessage(hDlg, slDarker, TBM_GETPOS, 0, 0);

				if (newV != gpSet->bgImageDarker)
				{
					gpSetCls->SetBgImageDarker(newV, true);

					//gpSet->bgImageDarker = newV;
					//TCHAR tmp[10];
					//_wsprintf(tmp, SKIPLEN(countof(tmp)) L"%i", gpSet->bgImageDarker);
					//SetDlgItemText(hDlg, tDarker, tmp);

					//// Картинку может установить и плагин
					//if (gpSet->isShowBgImage && gpSet->sBgImage[0])
					//	gpSetCls->LoadBackgroundFile(gpSet->sBgImage);
					//else
					//	gpSetCls->NeedBackgroundUpdate();

					//gpConEmu->Update(true);
				}
			}
			else if ((pgId == thi_Transparent) && (HWND)lParam == GetDlgItem(hDlg, slTransparent))
			{
				int newV = SendDlgItemMessage(hDlg, slTransparent, TBM_GETPOS, 0, 0);

				if (newV != gpSet->nTransparent)
				{
					CSettings::checkDlgButton(hDlg, cbTransparent, (newV != MAX_ALPHA_VALUE) ? BST_CHECKED : BST_UNCHECKED);
					gpSet->nTransparent = newV;
					if (!gpSet->isTransparentSeparate)
						SendDlgItemMessage(hDlg, slTransparentInactive, TBM_SETPOS, (WPARAM)true, (LPARAM)gpSet->nTransparent);
					gpConEmu->OnTransparent();
				}
			}
			else if ((pgId == thi_Transparent) && (HWND)lParam == GetDlgItem(hDlg, slTransparentInactive))
			{
				int newV = SendDlgItemMessage(hDlg, slTransparentInactive, TBM_GETPOS, 0, 0);

				if (gpSet->isTransparentSeparate && (newV != gpSet->nTransparentInactive))
				{
					//checkDlgButton(hDlg, cbTransparentInactive, (newV!=MAX_ALPHA_VALUE) ? BST_CHECKED : BST_UNCHECKED);
					gpSet->nTransparentInactive = newV;
					gpConEmu->OnTransparent();
				}
			}
		} // WM_HSCROLL
		break;

		case WM_NOTIFY:
		{
			if (((NMHDR*)lParam)->code == TTN_GETDISPINFO)
			{
				return gpSetCls->ProcessTipHelp(hDlg, messg, wParam, lParam);
			}
			else switch (((NMHDR*)lParam)->idFrom)
			{
			case lbActivityLog:
				if (!pObj->mb_SkipSelChange)
					return ((CSetPgDebug*)pObj)->OnActivityLogNotify(wParam, lParam);
				break;
			case lbConEmuHotKeys:
				if (!pObj->mb_SkipSelChange)
					return dynamic_cast<CSetPgKeys*>(pObj)->OnHotkeysNotify(hDlg, wParam, lParam);
				break;
			}
			return 0;
		} // WM_NOTIFY
		break;

		case WM_TIMER:

			if (wParam == BALLOON_MSG_TIMERID)
			{
				KillTimer(hDlg, BALLOON_MSG_TIMERID);
				SendMessage(gpSetCls->hwndBalloon, TTM_TRACKACTIVATE, FALSE, (LPARAM)&gpSetCls->tiBalloon);
				SendMessage(gpSetCls->hwndTip, TTM_ACTIVATE, TRUE, 0);
			}
			break;

		default:
		{
			if (messg == gpSetCls->mn_MsgRecreateFont)
			{
				gpSetCls->RecreateFont(LOWORD(wParam));
			}
			else if (messg == gpSetCls->mn_MsgLoadFontFromMain)
			{
				CSetPgFonts* pFonts = NULL;
				if (gpSetCls->GetPageObj(pFonts))
				{
					if (pgId == thi_Views)
						pFonts->CopyFontsTo(hDlg, tThumbsFontName, tTilesFontName, 0);
					else if (pgId == thi_Tabs)
						pFonts->CopyFontsTo(hDlg, tTabFontFace, 0);
					else if (pgId == thi_Status)
						pFonts->CopyFontsTo(hDlg, tStatusFontFace, 0);
				}
			}
			else if (messg == gpSetCls->mn_MsgUpdateCounter)
			{
				gpSetCls->PostUpdateCounters(true);
			}
			else if (messg == DBGMSG_LOG_ID && pgId == thi_Debug)
			{
				MSetter lockSelChange(&pObj->mb_SkipSelChange);
				if (wParam == DBGMSG_LOG_SHELL_MAGIC)
				{
					CSetPgDebug::DebugLogShellActivity *pShl = (CSetPgDebug::DebugLogShellActivity*)lParam;
					((CSetPgDebug*)pObj)->debugLogShell(pShl);
				} // DBGMSG_LOG_SHELL_MAGIC
				else if (wParam == DBGMSG_LOG_INPUT_MAGIC)
				{
					CESERVER_REQ_PEEKREADINFO* pInfo = (CESERVER_REQ_PEEKREADINFO*)lParam;
					((CSetPgDebug*)pObj)->debugLogInfo(pInfo);
				} // DBGMSG_LOG_INPUT_MAGIC
				else if (wParam == DBGMSG_LOG_CMD_MAGIC)
				{
					CSetPgDebug::LogCommandsData* pCmd = (CSetPgDebug::LogCommandsData*)lParam;
					((CSetPgDebug*)pObj)->debugLogCommand(pCmd);
				} // DBGMSG_LOG_CMD_MAGIC
			} // if (messg == DBGMSG_LOG_ID && hDlg == gpSetCls->hDebug)
			else
			{
				pObj->PageDlgProc(hDlg, messg, wParam, lParam);
			}
		} // default:
	} //switch (messg)

	return 0;
}

void CSetPgBase::setHotkeyCheckbox(HWND hDlg, WORD nCtrlId, int iHotkeyId, LPCWSTR pszFrom, LPCWSTR pszTo, UINT uChecked)
{
	wchar_t szKeyFull[128] = L"";
	gpSet->GetHotkeyNameById(iHotkeyId, szKeyFull, false);
	if (szKeyFull[0] == 0)
	{
		EnableWindow(GetDlgItem(hDlg, nCtrlId), FALSE);
		checkDlgButton(hDlg, nCtrlId, BST_UNCHECKED);
	}
	else
	{
		if (pszFrom)
		{
			wchar_t* ptr = (wchar_t*)wcsstr(szKeyFull, pszFrom);
			if (ptr)
			{
				*ptr = 0;
				if (pszTo)
				{
					wcscat_c(szKeyFull, pszTo);
				}
			}
		}

		CEStr lsText(GetDlgItemTextPtr(hDlg, nCtrlId));
		LPCWSTR pszTail = lsText.IsEmpty() ? NULL : wcsstr(lsText, L" - ");
		if (pszTail)
		{
			CEStr lsNew(szKeyFull, pszTail);
			SetDlgItemText(hDlg, nCtrlId, lsNew);
		}

		checkDlgButton(hDlg, nCtrlId, uChecked);
	}
}

/// Update GroupBox title, replacing text *between* pszFrom and pszTo with current HotKey
/// Examples:
///    gbPasteM1 has title "Paste mode #1 (Shift+Ins)", pszFrom=L"(", pszTo=L")"
///    cbMouseDragWindow has title "Ctrl+Alt - drag ConEmu window", pszFrom=NULL, pszTo=L" - "
/// Than `Shift+Ins` would be replaced with current user defined hotkey value
void CSetPgBase::setCtrlTitleByHotkey(HWND hDlg, WORD nCtrlId, int iHotkeyId, LPCWSTR pszFrom, LPCWSTR pszTo)
{
	if (!hDlg || !nCtrlId || (nCtrlId == (WORD)IDC_STATIC) || (iHotkeyId <= 0))
	{
		_ASSERTE(FALSE && "Invalid identifiers");
		return;
	}

	wchar_t szKeyFull[128] = L"";
	gpSet->GetHotkeyNameById(iHotkeyId, szKeyFull, true);
	if (szKeyFull[0] == 0)
	{
		_ASSERTE(FALSE && "Failed to acquire HotKey");
		wcscpy_c(szKeyFull, L"???");
	}

	CEStr lsLoc;
	if (!CLngRc::getControl(nCtrlId, lsLoc))
	{
		if (GetString(hDlg, nCtrlId, &lsLoc.ms_Val) <= 0)
		{
			_ASSERTE(FALSE && "No title?");
			return;
		}
	}

	if (pszFrom && *pszFrom)
	{
		// "Paste mode #1 (Shift+Ins)"
		LPCWSTR ptr1, ptr2 = NULL;
		ptr1 = wcsstr(lsLoc, pszFrom);
		if (!ptr1)
		{
			_ASSERTE(ptr1 && "pszFrom not found");
			return;
		}
		ptr1 += wcslen(pszFrom);
		if (pszTo && *pszTo)
		{
			ptr2 = wcsstr(ptr1, pszTo);
		}
		if (!ptr2 || (ptr2 == ptr1))
		{
			_ASSERTE(ptr2 && (ptr2 != ptr1) && "Invalid source string");
			return;
		}

		// Trim to hotkey beginning
		lsLoc.ms_Val[(ptr1 - lsLoc.ms_Val)] = 0;
		// And create new title
		CEStr lsNew(lsLoc.ms_Val, szKeyFull, ptr2);
		// Update control text
		SetDlgItemText(hDlg, nCtrlId, lsNew);
	}
	else if (pszTo && *pszTo)
	{
		// "Ctrl+Alt - drag ConEmu window"
		LPCWSTR ptr;
		ptr = wcsstr(lsLoc, pszTo);
		if (!ptr)
		{
			_ASSERTE(ptr && "pszTo not found");
			return;
		}

		// Create new title
		CEStr lsNew(szKeyFull, ptr);
		// Update control text
		SetDlgItemText(hDlg, nCtrlId, lsNew);
	}
	else
	{
		_ASSERTE(FALSE && "Invalid anchors");
	}
}
