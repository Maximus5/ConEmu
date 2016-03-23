
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
#include "DynDialog.h"
#include "LngRc.h"
#include "Options.h"
#include "OptionsClass.h"
#include "RealConsole.h"
#include "SetDlgColors.h"
#include "SetPgApps.h"
#include "SetPgBase.h"
#include "SetPgFonts.h"
#include "SetPgKeys.h"

CSetPgBase::CSetPgBase()
	: mh_Dlg(NULL)
	, mb_SkipSelChange(false)
	, mn_ActivateTabMsg(WM_APP)
	, mp_ParentDpi(NULL)
{
}

CSetPgBase::~CSetPgBase()
{
}

HWND CSetPgBase::CreatePage(ConEmuSetupPages* p, UINT nActivateTabMsg, const CDpiForDialog* apParentDpi)
{
	if (apParentDpi)
	{
		if (!p->pDpiAware)
			p->pDpiAware = new CDpiForDialog();
		p->DpiChanged = false;
	}

	wchar_t szLog[80]; _wsprintf(szLog, SKIPCOUNT(szLog) L"Creating child dialog ID=%u", p->DialogID);
	LogString(szLog);
	SafeDelete(p->pDialog);
	SafeDelete(p->pPage);

	p->pPage = p->CreateObj();

	p->pPage->mn_ActivateTabMsg = nActivateTabMsg;
	p->pPage->mp_ParentDpi = apParentDpi;

	p->pDialog = CDynDialog::ShowDialog(p->DialogID, ghOpWnd, pageOpProc, (LPARAM)p);
	p->hPage = p->pDialog ? p->pDialog->mh_Dlg : NULL;

	return p->hPage;
}

void CSetPgBase::ProcessDpiChange(ConEmuSetupPages* p, CDpiForDialog* apDpi)
{
	if (!this || !p || !p->hPage || !p->pDpiAware || !apDpi)
		return;

	HWND hPlace = GetDlgItem(ghOpWnd, tSetupPagePlace);
	RECT rcClient; GetWindowRect(hPlace, &rcClient);
	MapWindowPoints(NULL, ghOpWnd, (LPPOINT)&rcClient, 2);

	p->DpiChanged = false;
	p->pDpiAware->SetDialogDPI(apDpi->GetCurDpi(), &rcClient);

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

		if (pPage && pPage->pDpiAware && pPage->pDpiAware->ProcessDpiMessages(hDlg, messg, wParam, lParam))
		{
			return TRUE;
		}
	}

	if ((messg == WM_INITDIALOG) || (pObj && (messg == pObj->mn_ActivateTabMsg)))
	{
		if (!lParam)
		{
			_ASSERTE(lParam != 0);
			return 0;
		}

		ConEmuSetupPages* p = (ConEmuSetupPages*)lParam;
		_ASSERTE(p && (p->PageIndex >= thi_Fonts && p->PageIndex < thi_Last));
		_ASSERTE(p && p->pPage);
		_ASSERTE(p->hPage == NULL || p->hPage == hDlg);

		pgId = p->PageIndex;
		pObj = p->pPage;
		//p->hPage = hDlg; -- later, in bInitial

		bool bInitial = (messg == WM_INITDIALOG);

		if (bInitial)
		{
			_ASSERTE(p->PageIndex >= 0 && p->hPage == NULL);
			p->hPage = hDlg;
			p->pPage->mh_Dlg = hDlg;

			#ifdef _DEBUG
			// p->pDialog is NULL on first WM_INIT
			if (p->pDialog)
			{
				_ASSERTE(p->pDialog->mh_Dlg == hDlg);
			}
			#endif

			HWND hPlace = GetDlgItem(ghOpWnd, tSetupPagePlace);
			RECT rcClient; GetWindowRect(hPlace, &rcClient);
			MapWindowPoints(NULL, ghOpWnd, (LPPOINT)&rcClient, 2);
			if (p->pDpiAware)
				p->pDpiAware->Attach(hDlg, ghOpWnd, CDynDialog::GetDlgClass(hDlg));
			MoveWindowRect(hDlg, rcClient);
		}
		else
		{
			_ASSERTE(p->PageIndex >= 0 && p->hPage == hDlg);
			// обновить контролы страничек при активации вкладки
		}

		MSetter lockSelChange(&pObj->mb_SkipSelChange);

		pObj->OnInitDialog(hDlg, bInitial);

		if (bInitial)
		{
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
				gpSetCls->OnButtonClicked(hDlg, wParam, lParam);
				return 0;

			case EN_CHANGE:
				if (!pObj->mb_SkipSelChange)
					gpSetCls->OnEditChanged(hDlg, wParam, lParam);
				return 0;

			case CBN_EDITCHANGE:
			case CBN_SELCHANGE/*LBN_SELCHANGE*/:
				if (!pObj->mb_SkipSelChange)
					gpSetCls->OnComboBox(hDlg, wParam, lParam);
				return 0;

			case LBN_DBLCLK:
				gpSetCls->OnListBoxDblClk(hDlg, wParam, lParam);
				return 0;

			case CBN_KILLFOCUS:
				if (gpSetCls->mn_LastChangingFontCtrlId && LOWORD(wParam) == gpSetCls->mn_LastChangingFontCtrlId)
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
		{
			WORD wID = GetDlgCtrlID((HWND)lParam);

			if ((wID >= c0 && wID <= CSetDlgColors::MAX_COLOR_EDT_ID) || (wID >= c32 && wID <= c38))
				return CSetDlgColors::ColorCtlStatic(hDlg, wID, (HWND)lParam);

			return 0;
		} // WM_CTLCOLORSTATIC
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
					return gpSetCls->OnActivityLogNotify(hDlg, wParam, lParam);
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
				gpSetCls->RecreateFont(wParam);
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
					DebugLogShellActivity *pShl = (DebugLogShellActivity*)lParam;
					gpSetCls->debugLogShell(hDlg, pShl);
				} // DBGMSG_LOG_SHELL_MAGIC
				else if (wParam == DBGMSG_LOG_INPUT_MAGIC)
				{
					CESERVER_REQ_PEEKREADINFO* pInfo = (CESERVER_REQ_PEEKREADINFO*)lParam;
					gpSetCls->debugLogInfo(hDlg, pInfo);
				} // DBGMSG_LOG_INPUT_MAGIC
				else if (wParam == DBGMSG_LOG_CMD_MAGIC)
				{
					CSettings::LogCommandsData* pCmd = (CSettings::LogCommandsData*)lParam;
					gpSetCls->debugLogCommand(hDlg, pCmd);
				} // DBGMSG_LOG_CMD_MAGIC
			} // if (messg == DBGMSG_LOG_ID && hDlg == gpSetCls->hDebug)
		} // default:
	} //switch (messg)

	return 0;
}

void CSetPgBase::setHotkeyCheckbox(HWND hDlg, WORD nCtrlId, int iHotkeyId, LPCWSTR pszFrom, LPCWSTR pszTo, bool bChecked)
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

		checkDlgButton(hDlg, nCtrlId, bChecked);
	}
}

/// Update GroupBox title, replacing text *between* pszFrom and pszTo with current HotKey
/// For example, gbPasteM1 has title "Paste mode #1 (Shift+Ins)", pszFrom=L"(", pszTo=L")"
/// Than `Shift+Ins` would be replaced with current user defined hotkey value
void CSetPgBase::setCtrlTitleByHotkey(HWND hDlg, WORD nCtrlId, int iHotkeyId, LPCWSTR pszFrom, LPCWSTR pszTo)
{
	if (!hDlg || !nCtrlId || (nCtrlId == (WORD)IDC_STATIC) || (iHotkeyId <= 0))
	{
		_ASSERTE(FALSE && "Invalid identifiers");
		return;
	}
	if (!pszFrom || !*pszFrom)
	{
		_ASSERTE(FALSE && "Invalid anchors");
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

	LPCWSTR ptr1, ptr2;
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
	if (ptr2 == ptr1)
	{
		_ASSERTE(ptr2 != ptr1 && "Invalid source string");
		return;
	}

	// Trim to hotkey beginning
	lsLoc.ms_Val[(ptr1 - lsLoc.ms_Val)] = 0;
	// And create new title
	CEStr lsNew(lsLoc.ms_Val, szKeyFull, ptr2);

	// Update control text
	SetDlgItemText(hDlg, nCtrlId, lsNew);
}
