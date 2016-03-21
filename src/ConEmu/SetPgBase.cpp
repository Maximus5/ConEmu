
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

#include "ConEmu.h"
#include "DynDialog.h"
#include "Options.h"
#include "OptionsClass.h"
#include "RealConsole.h"
#include "SetDlgColors.h"
#include "SetPgBase.h"

CSetPgBase::CSetPgBase()
{
}

CSetPgBase::~CSetPgBase()
{
}

HWND CSetPgBase::CreatePage(ConEmuSetupPages* p)
{
	if (gpSetCls->mp_DpiAware)
	{
		if (!p->pDpiAware)
			p->pDpiAware = new CDpiForDialog();
		p->DpiChanged = false;
	}
	wchar_t szLog[80]; _wsprintf(szLog, SKIPCOUNT(szLog) L"Creating child dialog ID=%u", p->DialogID);
	LogString(szLog);
	SafeDelete(p->pDialog);

	p->pDialog = CDynDialog::ShowDialog(p->DialogID, ghOpWnd, pageOpProc, (LPARAM)p);
	p->hPage = p->pDialog ? p->pDialog->mh_Dlg : NULL;

	return p->hPage;
}

// Общая DlgProc на _все_ вкладки
INT_PTR CSetPgBase::pageOpProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	static bool bSkipSelChange = false;

	TabHwndIndex pgId = thi_Last;

	if (messg != WM_INITDIALOG)
	{
		ConEmuSetupPages* pPage = NULL;
		pgId = gpSetCls->GetPageId(hDlg, &pPage);

		if (pPage && pPage->pDpiAware && pPage->pDpiAware->ProcessDpiMessages(hDlg, messg, wParam, lParam))
		{
			return TRUE;
		}
	}

	if ((messg == WM_INITDIALOG) || (messg == gpSetCls->mn_ActivateTabMsg))
	{
		if (!lParam)
		{
			_ASSERTE(lParam != 0);
			return 0;
		}

		ConEmuSetupPages* p = (ConEmuSetupPages*)lParam;
		_ASSERTE(p && (p->PageIndex >= thi_Fonts && p->PageIndex < thi_Last));
		_ASSERTE(p->hPage == NULL || p->hPage == hDlg);

		pgId = p->PageIndex;
		//p->hPage = hDlg; -- later, in bInitial

		bool bInitial = (messg == WM_INITDIALOG);

		if (bInitial)
		{
			_ASSERTE(p->PageIndex >= 0 && p->hPage == NULL);
			p->hPage = hDlg;

			HWND hPlace = GetDlgItem(ghOpWnd, tSetupPagePlace);
			RECT rcClient; GetWindowRect(hPlace, &rcClient);
			MapWindowPoints(NULL, ghOpWnd, (LPPOINT)&rcClient, 2);
			if (p->pDpiAware)
				p->pDpiAware->Attach(hDlg, ghOpWnd, p->pDialog);
			MoveWindowRect(hDlg, rcClient);
		}
		else
		{
			_ASSERTE(p->PageIndex >= 0 && p->hPage == hDlg);
			// обновить контролы страничек при активации вкладки
		}

		switch (p->DialogID)
		{
		case IDD_SPG_FONTS:
			gpSetCls->OnInitDialog_Main(hDlg, bInitial);
			break;
		case IDD_SPG_SIZEPOS:
		{
			bool lbOld = bSkipSelChange; bSkipSelChange = true;
			gpSetCls->OnInitDialog_WndSizePos(hDlg, bInitial);
			bSkipSelChange = lbOld;
		}
		break;
		case IDD_SPG_BACKGR:
			gpSetCls->OnInitDialog_Background(hDlg, bInitial);
			break;
		case IDD_SPG_APPEAR:
			gpSetCls->OnInitDialog_Show(hDlg, bInitial);
			break;
		case IDD_SPG_CONFIRM:
			gpSetCls->OnInitDialog_Confirm(hDlg, bInitial);
			break;
		case IDD_SPG_TASKBAR:
			gpSetCls->OnInitDialog_Taskbar(hDlg, bInitial);
			break;
		case IDD_SPG_CURSOR:
			gpSetCls->OnInitDialog_Cursor(hDlg, bInitial);
			break;
		case IDD_SPG_STARTUP:
			gpSetCls->OnInitDialog_Startup(hDlg, bInitial);
			break;
		case IDD_SPG_FEATURE:
			gpSetCls->OnInitDialog_Ext(hDlg, bInitial);
			break;
		case IDD_SPG_COMSPEC:
			gpSetCls->OnInitDialog_Comspec(hDlg, bInitial);
			break;
		case IDD_SPG_ENVIRONMENT:
			gpSetCls->OnInitDialog_Environment(hDlg, bInitial);
			break;
		case IDD_SPG_MARKCOPY:
			gpSetCls->OnInitDialog_MarkCopy(hDlg, bInitial);
			break;
		case IDD_SPG_PASTE:
			gpSetCls->OnInitDialog_Paste(hDlg, bInitial);
			break;
		case IDD_SPG_HIGHLIGHT:
			gpSetCls->OnInitDialog_Hilight(hDlg, bInitial);
			break;
		case IDD_SPG_FEATURE_FAR:
			gpSetCls->OnInitDialog_Far(hDlg, bInitial);
			break;
		case IDD_SPG_FARMACRO:
			gpSetCls->OnInitDialog_FarMacro(hDlg, bInitial);
			break;
		case IDD_SPG_KEYS:
		{
			bool lbOld = bSkipSelChange; bSkipSelChange = true;
			gpSetCls->OnInitDialog_Keys(hDlg, bInitial);
			bSkipSelChange = lbOld;
		}
		break;
		case IDD_SPG_CONTROL:
			gpSetCls->OnInitDialog_Control(hDlg, bInitial);
			break;
		case IDD_SPG_TABS:
			gpSetCls->OnInitDialog_Tabs(hDlg, bInitial);
			break;
		case IDD_SPG_STATUSBAR:
			gpSetCls->OnInitDialog_Status(hDlg, bInitial);
			break;
		case IDD_SPG_COLORS:
			gpSetCls->OnInitDialog_Color(hDlg, bInitial);
			break;
		case IDD_SPG_TRANSPARENT:
			if (bInitial)
				gpSetCls->OnInitDialog_Transparent(hDlg);
			break;
		case IDD_SPG_TASKS:
		{
			bool lbOld = bSkipSelChange; bSkipSelChange = true;
			gpSetCls->OnInitDialog_Tasks(hDlg, bInitial);
			bSkipSelChange = lbOld;
		}
		break;
		case IDD_SPG_APPDISTINCT:
			gpSetCls->OnInitDialog_Apps(hDlg, bInitial);
			break;
		case IDD_SPG_INTEGRATION:
			gpSetCls->OnInitDialog_Integr(hDlg, bInitial);
			break;
		case IDD_SPG_DEFTERM:
		{
			bool lbOld = bSkipSelChange; bSkipSelChange = true;
			gpSetCls->OnInitDialog_DefTerm(hDlg, bInitial);
			bSkipSelChange = lbOld;
		}
		break;
		case IDD_SPG_VIEWS:
			if (bInitial)
				gpSetCls->OnInitDialog_Views(hDlg);
			break;
		case IDD_SPG_DEBUG:
			if (bInitial)
				gpSetCls->OnInitDialog_Debug(hDlg);
			break;
		case IDD_SPG_UPDATE:
			gpSetCls->OnInitDialog_Update(hDlg, bInitial);
			break;
		case IDD_SPG_INFO:
			if (bInitial)
				gpSetCls->OnInitDialog_Info(hDlg);
			break;

		default:
			// Чтобы не забыть добавить вызов инициализации
			_ASSERTE(FALSE && "Unhandled DialogID!");
		} // switch (((ConEmuSetupPages*)lParam)->DialogID)

		if (bInitial)
		{
			gpSetCls->RegisterTipsFor(hDlg);
		}
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
		return gpSetCls->pageOpProc_Apps(hDlg, NULL, messg, wParam, lParam);
	}
	else if (pgId == thi_Integr)
	{
		return gpSetCls->pageOpProc_Integr(hDlg, messg, wParam, lParam);
	}
	else if (pgId == thi_Startup)
	{
		return gpSetCls->pageOpProc_Start(hDlg, messg, wParam, lParam);
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
				if (!bSkipSelChange)
					gpSetCls->OnEditChanged(hDlg, wParam, lParam);
				return 0;

			case CBN_EDITCHANGE:
			case CBN_SELCHANGE/*LBN_SELCHANGE*/:
				if (!bSkipSelChange)
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
					gpSetCls->OnHotkeysNotify(hDlg, wParam, 0);
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
				return gpSetCls->ColorCtlStatic(hDlg, wID, (HWND)lParam);

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
				if (!bSkipSelChange)
					return gpSetCls->OnActivityLogNotify(hDlg, wParam, lParam);
				break;
			case lbConEmuHotKeys:
				if (!bSkipSelChange)
					return gpSetCls->OnHotkeysNotify(hDlg, wParam, lParam);
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
				if (pgId == thi_Views)
					gpSetCls->OnInitDialog_CopyFonts(hDlg, tThumbsFontName, tTilesFontName, 0);
				else if (pgId == thi_Tabs)
					gpSetCls->OnInitDialog_CopyFonts(hDlg, tTabFontFace, 0);
				else if (pgId == thi_Status)
					gpSetCls->OnInitDialog_CopyFonts(hDlg, tStatusFontFace, 0);

			}
			else if (messg == gpSetCls->mn_MsgUpdateCounter)
			{
				gpSetCls->PostUpdateCounters(true);
			}
			else if (messg == DBGMSG_LOG_ID && pgId == thi_Debug)
			{
				if (wParam == DBGMSG_LOG_SHELL_MAGIC)
				{
					bool lbOld = bSkipSelChange; bSkipSelChange = true;
					DebugLogShellActivity *pShl = (DebugLogShellActivity*)lParam;
					gpSetCls->debugLogShell(hDlg, pShl);
					bSkipSelChange = lbOld;
				} // DBGMSG_LOG_SHELL_MAGIC
				else if (wParam == DBGMSG_LOG_INPUT_MAGIC)
				{
					bool lbOld = bSkipSelChange; bSkipSelChange = true;
					CESERVER_REQ_PEEKREADINFO* pInfo = (CESERVER_REQ_PEEKREADINFO*)lParam;
					gpSetCls->debugLogInfo(hDlg, pInfo);
					bSkipSelChange = lbOld;
				} // DBGMSG_LOG_INPUT_MAGIC
				else if (wParam == DBGMSG_LOG_CMD_MAGIC)
				{
					bool lbOld = bSkipSelChange; bSkipSelChange = true;
					CSettings::LogCommandsData* pCmd = (CSettings::LogCommandsData*)lParam;
					gpSetCls->debugLogCommand(hDlg, pCmd);
					bSkipSelChange = lbOld;
				} // DBGMSG_LOG_CMD_MAGIC
			} // if (messg == DBGMSG_LOG_ID && hDlg == gpSetCls->hDebug)
		} // default:
	} //switch (messg)

	return 0;
}
