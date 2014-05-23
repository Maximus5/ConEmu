
/*
Copyright (c) 2013-2014 Maximus5
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

#define DEBUGSTRTABS(s) //DEBUGSTR(s)

#include <windows.h>
#include "header.h"
#include "TabBar.h"
#include "TabCtrlBase.h"
#include "Options.h"
#include "ConEmu.h"
#include "VirtualConsole.h"
#include "TrayIcon.h"
#include "VConChild.h"
#include "VConGroup.h"
#include "Status.h"
#include "Menu.h"


CTabPanelBase::CTabPanelBase(CTabBarClass* ap_Owner)
{
	mp_Owner = ap_Owner;
	mh_TabFont = NULL;
	mh_TabTip = mh_Balloon = NULL;
	ms_TabErrText[0] = 0;
	memset(&tiBalloon,0,sizeof(tiBalloon));
	mn_prevTab = -1;
}

CTabPanelBase::~CTabPanelBase()
{
	if (mh_TabFont)
	{
		DeleteObject(mh_TabFont);
		mh_TabFont = NULL;
	}
}

void CTabPanelBase::UpdateTabFontInt()
{
	if (!IsTabbarCreated())
		return;

	// CreateFont
	HFONT hFont = CreateFont(gpSet->nTabFontHeight, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, gpSet->nTabFontCharSet, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, gpSet->sTabFontFace);

	// virtual
	SetTabbarFont(hFont);

	// Store new Font in member variable
	if (mh_TabFont)
		DeleteObject(mh_TabFont);
	mh_TabFont = hFont;
}

void CTabPanelBase::ShowTabErrorInt(LPCTSTR asInfo, int tabIndex)
{
	if (!asInfo)
		ms_TabErrText[0] = 0;
	else if (asInfo != ms_TabErrText)
		lstrcpyn(ms_TabErrText, asInfo, countof(ms_TabErrText));

	tiBalloon.lpszText = ms_TabErrText;

	if (ms_TabErrText[0])
	{
		SendMessage(mh_TabTip, TTM_ACTIVATE, FALSE, 0);
		SendMessage(mh_Balloon, TTM_UPDATETIPTEXT, 0, (LPARAM)&tiBalloon);

		//RECT rcControl; GetWindowRect(GetDlgItem(hMain, tFontFace), &rcControl);
		RECT rcTab = {0}; GetTabRect(tabIndex, &rcTab);

		//int ptx = 0; //rcControl.right - 10;
		//int pty = 0; //(rcControl.top + rcControl.bottom) / 2;
		SendMessage(mh_Balloon, TTM_TRACKPOSITION, 0, MAKELONG((rcTab.left+rcTab.right)>>1,rcTab.bottom));
		SendMessage(mh_Balloon, TTM_TRACKACTIVATE, TRUE, (LPARAM)&tiBalloon);
		gpConEmu->SetKillTimer(true, TIMER_FAILED_TABBAR_ID, TIMER_FAILED_TABBAR_ELAPSE);
	}
	else
	{
		SendMessage(mh_Balloon, TTM_TRACKACTIVATE, FALSE, (LPARAM)&tiBalloon);
		SendMessage(mh_TabTip, TTM_ACTIVATE, TRUE, 0);
	}
}

//int CTabPanelBase::GetTabIcon(bool bAdmin)
//{
//	return m_TabIcons.GetTabIcon(bAdmin);
//}

LRESULT CTabPanelBase::OnTimerInt(WPARAM wParam)
{
	if (wParam == TIMER_FAILED_TABBAR_ID)
	{
		gpConEmu->SetKillTimer(false, TIMER_FAILED_TABBAR_ID, 0);
		SendMessage(mh_Balloon, TTM_TRACKACTIVATE, FALSE, (LPARAM)&tiBalloon);
		SendMessage(mh_TabTip, TTM_ACTIVATE, TRUE, 0);
	}
	return 0;
}

//void CTabPanelBase::InitIconList()
//{
//	if (!m_TabIcons.IsInitialized())
//	{
//		m_TabIcons.Initialize();
//	}
//}

void CTabPanelBase::InitTooltips(HWND hParent)
{
	if (!mh_TabTip || !IsWindow(mh_TabTip))
	{
		mh_TabTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
									WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
									CW_USEDEFAULT, CW_USEDEFAULT,
									CW_USEDEFAULT, CW_USEDEFAULT,
									hParent, NULL,
									g_hInstance, NULL);
		SetWindowPos(mh_TabTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
	}

	if (!mh_Balloon || !IsWindow(mh_Balloon))
	{
		mh_Balloon = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
		                            gpSetCls->BalloonStyle() | WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
		                            CW_USEDEFAULT, CW_USEDEFAULT,
		                            CW_USEDEFAULT, CW_USEDEFAULT,
		                            hParent, NULL,
		                            g_hInstance, NULL);
		SetWindowPos(mh_Balloon, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
		// Set up tool information.
		// In this case, the "tool" is the entire parent window.
		tiBalloon.cbSize = 44; // был sizeof(TOOLINFO);
		tiBalloon.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
		tiBalloon.hwnd = hParent;
		tiBalloon.hinst = g_hInstance;
		static wchar_t szAsterisk[] = L"*"; // eliminate GCC warning
		tiBalloon.lpszText = szAsterisk;
		tiBalloon.uId = (UINT_PTR)TTF_IDISHWND;
		GetTabBarClientRect(&tiBalloon.rect);
		// Associate the ToolTip with the tool window.
		SendMessage(mh_Balloon, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO) &tiBalloon);
		// Allow multiline
		SendMessage(mh_Balloon, TTM_SETMAXTIPWIDTH, 0, (LPARAM)300);
	}
}

CVirtualConsole* CTabPanelBase::FarSendChangeTab(int tabIndex)
{
	CVConGuard VCon;
	DWORD wndIndex = 0;
	BOOL  bNeedActivate = FALSE, bChangeOk = FALSE;
	ShowTabErrorInt(NULL, 0);

	if (!mp_Owner->GetVConFromTab(tabIndex, &VCon, &wndIndex))
	{
		if (mp_Owner->IsInSwitch())
			mp_Owner->Update();  // показать реальное положение дел

		return NULL;
	}

	CVirtualConsole *pVCon = VCon.VCon();

	if (!gpConEmu->isActive(pVCon, false))
		bNeedActivate = TRUE;

	DWORD nCallStart = TimeGetTime(), nCallEnd = 0;

	bChangeOk = pVCon->RCon()->ActivateFarWindow(wndIndex);

	if (!bChangeOk)
	{
		// Всплыть тултип с руганью - не смогли активировать
		ShowTabErrorInt(L"This tab can't be activated now!", tabIndex);
	}
	else
	{
		nCallEnd = TimeGetTime();
		_ASSERTE((nCallEnd - nCallStart) < ACTIVATE_TAB_CRITICAL);
	}

	// Чтобы лишнее не мелькало - активируем консоль
	// ТОЛЬКО после смены таба (успешной или неудачной - неважно)
	TODO("Не срабатывает. Новые данные из консоли получаются с задержкой и мелькает старое содержимое активируемой консоли");

	if (bNeedActivate)
	{
		if (!gpConEmu->Activate(pVCon))
		{
			if (mp_Owner->IsInSwitch())
				mp_Owner->Update();  // показать реальное положение дел

			TODO("А текущий таб не слетит, если активировать не удалось?");
			return NULL;
		}
	}

	if (!bChangeOk)
	{
		pVCon = NULL;

		if (mp_Owner->IsInSwitch())
			mp_Owner->Update();  // показать реальное положение дел
	}

	UNREFERENCED_PARAMETER(nCallStart);
	UNREFERENCED_PARAMETER(nCallEnd);

	return pVCon;
}

LRESULT CTabPanelBase::OnMouseRebar(UINT uMsg, int x, int y)
{
	if (!gpSet->isTabs)
		return 0;

	if (uMsg == WM_LBUTTONDBLCLK)
	{
		if ((gpSet->nTabBarDblClickAction == 2)
			|| ((gpSet->nTabBarDblClickAction == 1) && gpSet->isCaptionHidden()))
		{
			// Чтобы клик случайно не провалился в консоль
			gpConEmu->mouse.state |= MOUSE_SIZING_DBLCKL;
			// Аналог AltF9
			gpConEmu->DoMaximizeRestore();
		}
		else if ((gpSet->nTabBarDblClickAction == 3)
			|| ((gpSet->nTabBarDblClickAction == 1) && !gpSet->isCaptionHidden()))
		{
			gpConEmu->RecreateAction(cra_CreateTab/*FALSE*/, gpSet->isMultiNewConfirm || isPressed(VK_SHIFT));
		}
	}
	else if (uMsg == WM_RBUTTONUP)
	{
		POINT ptScr; GetCursorPos(&ptScr);
		gpConEmu->mp_Menu->ShowSysmenu(ptScr.x, ptScr.y/*-32000*/);
	}
	else if ((gpConEmu->WindowMode == wmNormal) && gpSet->isCaptionHidden())
	{
		// WM_NC* messages needs screen coords in lParam
		POINT ptScr; GetCursorPos(&ptScr);
		LPARAM lParamMain = MAKELONG(ptScr.x,ptScr.y);
		gpConEmu->WndProc(ghWnd, uMsg-(WM_MOUSEMOVE-WM_NCMOUSEMOVE), HTCAPTION, lParamMain);
	}
	else if (uMsg == WM_MBUTTONUP)
	{
		gpConEmu->RecreateAction(cra_CreateTab/*FALSE*/, gpSet->isMultiNewConfirm || isPressed(VK_SHIFT));
	}

	return 0;
}

LRESULT CTabPanelBase::OnMouseTabbar(UINT uMsg, int nTabIdx, int x, int y)
{
	if (nTabIdx == -1)
		nTabIdx = GetTabFromPoint(MakePoint(x,y), false);

	if (uMsg == WM_LBUTTONDOWN)
	{
		// Приходит, например, из ReBar по клику НАД табами
		int lnCurTab = GetCurSelInt();
		if (lnCurTab != nTabIdx)
		{
			FarSendChangeTab(nTabIdx);
		}
	}
	else if ((uMsg == WM_MBUTTONUP)
		|| (uMsg == WM_RBUTTONUP)
		|| ((uMsg == WM_LBUTTONDBLCLK) && gpSet->nTabBtnDblClickAction))
	{
		if (nTabIdx >= 0)
		{
			CVirtualConsole* pVCon = NULL;
			// для меню нужны экранные координаты, получим их сразу, чтобы менюшка вплывала на клике
			// а то вдруг мышка уедет, во время активации таба...
			POINT ptCur = {0,0}; GetCursorPos(&ptCur);
			pVCon = FarSendChangeTab(nTabIdx);

			if (pVCon)
			{
				CVConGuard guard(pVCon);
				BOOL lbCtrlPressed = isPressed(VK_CONTROL);

				if (uMsg == WM_LBUTTONDBLCLK)
				{
					switch (gpSet->nTabBtnDblClickAction)
					{
					case 1:
						// Чтобы клик случайно не провалился в консоль
						gpConEmu->mouse.state |= MOUSE_SIZING_DBLCKL;
						// Аналог AltF9
						gpConEmu->DoMaximizeRestore();
						break;
					case 2:
						guard->RCon()->CloseTab();
						break;
					case 3:
						gpConEmu->mp_Menu->ExecPopupMenuCmd(tmp_None, guard.VCon(), IDM_RESTART);
						break;
					case 4:
						gpConEmu->mp_Menu->ExecPopupMenuCmd(tmp_None, guard.VCon(), IDM_DUPLICATE);
						break;
					}
				}
				else if (uMsg == WM_RBUTTONUP && !lbCtrlPressed)
				{
					gpConEmu->mp_Menu->ShowPopupMenu(guard.VCon(), ptCur);
				}
				else
				{
					guard->RCon()->CloseTab();
				}

				// борьба с оптимизатором в релизе
				gpConEmu->isValid(pVCon);
			}
		}
	}

	return 0;
}

LRESULT CTabPanelBase::OnMouseToolbar(UINT uMsg, int nCmdId, int x, int y)
{
	//TODO:
	return 0;
}

bool CTabPanelBase::OnSetCursorRebar()
{
	if (gpSet->isTabs && !gpSet->isQuakeStyle
		&& (gpConEmu->GetWindowMode() == wmNormal)
		&& gpSet->isCaptionHidden())
	{
		POINT ptCur = {};
		GetCursorPos(&ptCur);
		int nHitTest = GetTabFromPoint(ptCur);

		if (nHitTest == -1)
		{
			SetCursor(/*gpSet->isQuakeStyle ? gpConEmu->mh_CursorArrow :*/ gpConEmu->mh_CursorMove);
			return true;
		}
	}

	return false;
}
