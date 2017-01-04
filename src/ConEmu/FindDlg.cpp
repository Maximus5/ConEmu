
/*
Copyright (c) 2013-2017 Maximus5
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
#include <commctrl.h>
#pragma warning(disable: 4091)
#include <shlobj.h>
#pragma warning(default: 4091)

#include "ConEmu.h"
#include "DlgItemHelper.h"
#include "DpiAware.h"
#include "DynDialog.h"
#include "FindDlg.h"
#include "FontMgr.h"
#include "Options.h"
#include "RealConsole.h"
#include "TabBar.h"
#include "VConChild.h"
#include "VConGroup.h"
#include "VirtualConsole.h"


//#define DEBUGSTRFONT(s) DEBUGSTR(s)

CEFindDlg::CEFindDlg()
{
	mh_FindDlg = NULL;
	mp_Dlg = NULL;
	mp_DpiAware = NULL;
}

void CEFindDlg::FindTextDialog()
{
	if (mh_FindDlg && IsWindow(mh_FindDlg))
	{
		SetForegroundWindow(mh_FindDlg);
		return;
	}

	CVConGuard VCon;
	CRealConsole* pRCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : NULL;

	// Создаем диалог поиска только для консольных приложений
	if (!pRCon || (pRCon->GuiWnd() && !pRCon->isBufferHeight()) || !pRCon->GetView())
	{
		//DisplayLastError(L"No RealConsole, nothing to find");
		return;
	}

	if (gpConEmu->mp_TabBar && gpConEmu->mp_TabBar->ActivateSearchPane(true))
	{
		// Контрол поиска встроен и показан в панели инструментов
		return;
	}

	gpConEmu->SkipOneAppsRelease(true);

	if (!mp_DpiAware)
		mp_DpiAware = new CDpiForDialog();

	// (CreateDialog)
	mp_Dlg = CDynDialog::ShowDialog(IDD_FIND, ghWnd, findTextProc, 0/*Param*/);
	mh_FindDlg = mp_Dlg ? mp_Dlg->mh_Dlg : NULL;
	if (!mh_FindDlg)
	{
		DisplayLastError(L"Can't create Find text dialog", GetLastError());
	}
}

void CEFindDlg::UpdateFindDlgAlpha(bool abForce)
{
	if (!mh_FindDlg)
		return;

	int nAlpha = 255;

	if (gpSet->FindOptions.bTransparent)
	{
		POINT ptCur = {}; GetCursorPos(&ptCur);
		RECT rcWnd = {}; GetWindowRect(mh_FindDlg, &rcWnd);

		nAlpha = PtInRect(&rcWnd, ptCur) ? 254 : DEFAULT_FINDDLG_ALPHA;
	}

	static int nWasAlpha;
	if (!abForce && (nWasAlpha == nAlpha))
		return;

	nWasAlpha = nAlpha;
	gpConEmu->SetTransparent(mh_FindDlg, nAlpha);
}

INT_PTR CEFindDlg::findTextProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (messg)
	{
		case WM_INITDIALOG:
		{
			gpConEmu->OnOurDialogOpened();
			gpConEmu->mp_Find->mh_FindDlg = hWnd2;

			SendMessage(hWnd2, WM_SETICON, ICON_BIG, (LPARAM)hClassIcon);
			SendMessage(hWnd2, WM_SETICON, ICON_SMALL, (LPARAM)hClassIconSm);

			CDynDialog::LocalizeDialog(hWnd2);

			if (gpConEmu->mp_Find->mp_DpiAware)
			{
				gpConEmu->mp_Find->mp_DpiAware->Attach(hWnd2, ghWnd, gpConEmu->mp_Find->mp_Dlg);
			}

			#if 0
			//if (IsDebuggerPresent())
			if (!gpSet->isAlwaysOnTop)
				SetWindowPos(hWnd2, HWND_NOTOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
			#endif

			CVConGuard VCon;
			CRealConsole* pRCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : NULL;
			RECT rcWnd = {}; GetWindowRect(pRCon->GetView(), &rcWnd);
			RECT rcDlg = {}; GetWindowRect(hWnd2, &rcDlg);
			int nShift = max(gpFontMgr->FontWidth(),gpFontMgr->FontHeight());
			int nWidth = rcDlg.right - rcDlg.left;
			SetWindowPos(hWnd2, gpSet->isAlwaysOnTop ? HWND_TOPMOST : HWND_TOP,
				max(rcWnd.left,(rcWnd.right-nShift-nWidth)), (rcWnd.top+nShift), 0,0,
				SWP_NOSIZE);

			gpConEmu->mp_Find->UpdateFindDlgAlpha(true);
			SetTimer(hWnd2, 101, 1000, NULL);

			SetClassLongPtr(hWnd2, GCLP_HICON, (LONG_PTR)hClassIcon);
			SetDlgItemText(hWnd2, tFindText, gpSet->FindOptions.pszText ? gpSet->FindOptions.pszText : L"");
			CDlgItemHelper::checkDlgButton(hWnd2, cbFindMatchCase, gpSet->FindOptions.bMatchCase);
			CDlgItemHelper::checkDlgButton(hWnd2, cbFindWholeWords, gpSet->FindOptions.bMatchWholeWords);
			CDlgItemHelper::checkDlgButton(hWnd2, cbFindFreezeConsole, gpSet->FindOptions.bFreezeConsole);
			#if 0
			CDlgItemHelper::checkDlgButton(hWnd2, cbFindHighlightAll, gpSet->FindOptions.bHighlightAll);
			#endif
			CDlgItemHelper::checkDlgButton(hWnd2, cbFindTransparent, gpSet->FindOptions.bTransparent);

			if (gpSet->FindOptions.pszText && *gpSet->FindOptions.pszText)
				SendDlgItemMessage(hWnd2, tFindText, EM_SETSEL, 0, lstrlen(gpSet->FindOptions.pszText));

			// Зовем всегда, чтобы инициализировать буфер для поиска как минимум
			gpConEmu->DoFindText(0);
			break;
		}

		//case WM_SYSCOMMAND:
		//	if (LOWORD(wParam) == ID_ALWAYSONTOP)
		//	{
		//		BOOL lbOnTopNow = GetWindowLong(ghOpWnd, GWL_EXSTYLE) & WS_EX_TOPMOST;
		//		SetWindowPos(ghOpWnd, lbOnTopNow ? HWND_NOTOPMOST : HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
		//		CheckMenuItem(GetSystemMenu(ghOpWnd, FALSE), ID_ALWAYSONTOP, MF_BYCOMMAND |
		//		              (lbOnTopNow ? MF_UNCHECKED : MF_CHECKED));
		//		SetWindowLongPtr(hWnd2, DWLP_MSGRESULT, 0);
		//		return 1;
		//	}
		//	break;

		case WM_MOUSEMOVE:
		case WM_NCMOUSEMOVE:
		case WM_TIMER:
			gpConEmu->mp_Find->UpdateFindDlgAlpha();
			break;

		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				int nDirection = 0;

				switch (LOWORD(wParam))
				{
				case IDCLOSE:
				case IDCANCEL:
					DestroyWindow(hWnd2);
					return 0;
				case cbFindMatchCase:
					gpSet->FindOptions.bMatchCase = CDlgItemHelper::isChecked2(hWnd2, cbFindMatchCase);
					break;
				case cbFindWholeWords:
					gpSet->FindOptions.bMatchWholeWords = CDlgItemHelper::isChecked2(hWnd2, cbFindWholeWords);
					break;
				case cbFindFreezeConsole:
					gpSet->FindOptions.bFreezeConsole = CDlgItemHelper::isChecked2(hWnd2, cbFindFreezeConsole);
					break;
				case cbFindHighlightAll:
					gpSet->FindOptions.bHighlightAll = CDlgItemHelper::isChecked2(hWnd2, cbFindHighlightAll);
					break;
				case cbFindTransparent:
					gpSet->FindOptions.bTransparent = CDlgItemHelper::isChecked2(hWnd2, cbFindTransparent);
					gpConEmu->mp_Find->UpdateFindDlgAlpha(true);
					return 0;
				case cbFindNext:
					nDirection = 1;
					break;
				case cbFindPrev:
					nDirection = -1;
					break;
				default:
					return 0;
				}

				if (gpSet->FindOptions.pszText && *gpSet->FindOptions.pszText)
					gpConEmu->DoFindText(nDirection);
			}
			else if (HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
			{
				MyGetDlgItemText(hWnd2, tFindText, gpSet->FindOptions.cchTextMax, gpSet->FindOptions.pszText);
				if (gpSet->FindOptions.pszText && *gpSet->FindOptions.pszText)
					gpConEmu->DoFindText(0);
			}
			break;

		case WM_CLOSE:
			gpConEmu->OnOurDialogClosed();
			DestroyWindow(hWnd2);
			break;

		case WM_DESTROY:
			KillTimer(hWnd2, 101);
			gpConEmu->mp_Find->mh_FindDlg = NULL;
			gpSet->SaveFindOptions();
			gpConEmu->SkipOneAppsRelease(false);
			gpConEmu->DoEndFindText();
			if (gpConEmu->mp_Find->mp_DpiAware)
				gpConEmu->mp_Find->mp_DpiAware->Detach();
			SafeDelete(gpConEmu->mp_Find->mp_Dlg);
			break;

		default:
			if (gpConEmu->mp_Find->mp_DpiAware && gpConEmu->mp_Find->mp_DpiAware->ProcessDpiMessages(hWnd2, messg, wParam, lParam))
			{
				return TRUE;
			}
	}

	return 0;
}
