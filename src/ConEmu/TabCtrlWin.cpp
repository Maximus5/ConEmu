﻿
/*
Copyright (c) 2013-present Maximus5
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

#define DEBUGSTRTABS(l,i,s) // { wchar_t szLbl[80]; swprintf_c(szLbl, L"CTabPanelWin(%s,%i): ",l,i+1); wchar_t* pszDbg = lstrmerge(szLbl,s); DEBUGSTR(pszDbg); SafeFree(pszDbg); }
#define DEBUGSTRSEL(s) //DEBUGSTR(s)
#define DEBUGSTRSIZE(s) //DEBUGSTR(s)
#define DEBUGSTRPANE(s) //DEBUGSTR(s)

#include "../common/defines.h"
#include "Header.h"
#include "TabBar.h"
#include "TabCtrlBase.h"
#include "TabCtrlWin.h"
#include "Options.h"
#include "OptionsClass.h"
#include "ConEmu.h"
#include "FindPanel.h"
#include "VirtualConsole.h"
#include "ToolImg.h"
#include "TrayIcon.h"
#include "VConChild.h"
#include "VConGroup.h"
#include "Status.h"
#include "Menu.h"
#include "../common/WUser.h"

//WNDPROC CTabPanelWin::_defaultTabProc = NULL;
//WNDPROC CTabPanelWin::_defaultToolProc = NULL;
//WNDPROC CTabPanelWin::_defaultReBarProc = NULL;

enum ReBarIndex
{
	rbi_TabBar  = 1,
	rbi_ToolBar = 2,
	rbi_FindBar = 3,
};

// не используем size, т.к. приходит "новый" размер из висты и в XP обламываемся
#define REBARBANDINFO_SIZE 80

// methods
CTabPanelWin::CTabPanelWin(CTabBarClass* ap_Owner)
	: CTabPanelBase(ap_Owner)
{
	mh_Toolbar = NULL; mh_Tabbar = NULL; mh_Rebar = NULL;
	mp_Find = NULL;
	mb_ChangeAllowed = false;
	mn_LastToolbarWidth = 0;
	mn_TabHeight = 0;
	mp_ToolImg = new CToolImg();
}

CTabPanelWin::~CTabPanelWin()
{
	SafeDelete(mp_ToolImg);
	SafeDelete(mp_Find);

	_ASSERTE(!mh_Toolbar || !IsWindow(mh_Toolbar));
	_ASSERTE(!mh_Tabbar || !IsWindow(mh_Tabbar));
	_ASSERTE(!mh_Rebar || !IsWindow(mh_Rebar));
}


/* Window procedures */

LRESULT CALLBACK CTabPanelWin::_ReBarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc;
	TabPanelWinMap map = {NULL};
	if (GetObj(hwnd, &map, (uMsg == WM_DESTROY)))
		lRc = ((CTabPanelWin*)map.object)->ReBarProc(hwnd, uMsg, wParam, lParam, map.defaultProc);
	else
		lRc = ::DefWindowProc(hwnd, uMsg, wParam, lParam);
	return lRc;
}

LRESULT CTabPanelWin::ReBarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC defaultProc)
{
	int nOverTabHit = -1;

	switch (uMsg)
	{
		case WM_DESTROY:
		{
			_ASSERTE(!mh_Rebar || mh_Rebar == hwnd);
			mh_Rebar = NULL;
			break;
		}
		case WM_WINDOWPOSCHANGING:
		{
			LPWINDOWPOS pos = (LPWINDOWPOS)lParam;
			RECT rcRebar = gpConEmu->RebarRect();
			pos->x = rcRebar.left;
			pos->y = rcRebar.top;
			pos->cx = RectWidth(rcRebar);
			pos->cy = RectHeight(rcRebar);
			return 0;
			//if (mn_TabHeight)
			//{
			//	pos->cy = mn_TabHeight;
			//	return 0;
			//}
			//break;
		}
		case WM_WINDOWPOSCHANGED:
		{
			#ifdef _DEBUG
			LPWINDOWPOS pos = (LPWINDOWPOS)lParam;
			#endif
			break;
		}
		case WM_SETFOCUS:
		{
			gpConEmu->setFocus();
			return 0;
		}
		case WM_SETCURSOR:
		{
			if (OnSetCursorRebar())
				return TRUE;
			break;
		}
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
		{
			if (gpSet->isTabs)
			{
				if (TabHitTest(true, &nOverTabHit) == HTCAPTION)
				{
					OnMouseRebar(uMsg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				}
				else if (nOverTabHit >= 0)
				{
					POINT ptTab = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
					if (uMsg != WM_MOUSEWHEEL && uMsg != WM_MOUSEHWHEEL)
						MapWindowPoints(mh_Rebar, mh_Tabbar, &ptTab, 1);
					OnMouseTabbar(uMsg, nOverTabHit, ptTab.x, ptTab.y);
				}
			}

			return 0;
		} // End of mouse messages
	}

	return CallWindowProc(defaultProc, hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CTabPanelWin::_TabProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc;
	TabPanelWinMap map = {NULL};
	if (GetObj(hwnd, &map, (uMsg == WM_DESTROY)))
		lRc = ((CTabPanelWin*)map.object)->TabProc(hwnd, uMsg, wParam, lParam, map.defaultProc);
	else
		lRc = ::DefWindowProc(hwnd, uMsg, wParam, lParam);
	return lRc;
}

LRESULT CTabPanelWin::TabProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC defaultProc)
{
	switch (uMsg)
	{
		case WM_DESTROY:
		{
			_ASSERTE(!mh_Tabbar || mh_Tabbar == hwnd);
			mh_Tabbar = NULL;
			break;
		}
		case WM_WINDOWPOSCHANGING:
		{
			if (mh_Rebar)
			{
				LPWINDOWPOS pos = (LPWINDOWPOS)lParam;

				_ASSERTE(mn_TabHeight > 0);

				//if (mp_Owner->mb_ThemingEnabled) {
				if (gpSetCls->CheckTheming())
				{
					pos->y = 2; // иначе в Win7 он смещается в {0x0} и снизу видна некрасивая полоса
					pos->cy = mn_TabHeight -3; // на всякий случай
				}
				else
				{
					pos->y = 1;
					pos->cy = mn_TabHeight + 1;
				}

				return 0;
			}

			break;
		}
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_LBUTTONDBLCLK:
		{
			OnMouseTabbar(uMsg, -1, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		}
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
		{
			LRESULT lDefRc = CallWindowProc(defaultProc, hwnd, uMsg, wParam, lParam);
			OnMouseTabbar(uMsg, -1, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return lDefRc;
		}
		case WM_SETCURSOR:
		{
			if (OnMouseTabbar(uMsg, -1, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
				return TRUE;
			break;
		}
		case WM_SETFOCUS:
		{
			gpConEmu->setFocus();
			return 0;
		}
	}

	return CallWindowProc(defaultProc, hwnd, uMsg, wParam, lParam);
}

// Window procedure for Toolbar (Multiconsole & BufferHeight)
LRESULT CALLBACK CTabPanelWin::_ToolProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc;
	TabPanelWinMap map = {NULL};
	if (GetObj(hwnd, &map, (uMsg == WM_DESTROY)))
		lRc = ((CTabPanelWin*)map.object)->ToolProc(hwnd, uMsg, wParam, lParam, map.defaultProc);
	else
		lRc = ::DefWindowProc(hwnd, uMsg, wParam, lParam);
	return lRc;
}

// Window procedure for Toolbar (Multiconsole, BufferHeight, Min/Max, etc.)
LRESULT CTabPanelWin::ToolProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC defaultProc)
{
	switch (uMsg)
	{
		case WM_DESTROY:
		{
			_ASSERTE(!mh_Toolbar || mh_Toolbar == hwnd);
			mh_Toolbar = NULL;
			break;
		}
		case WM_WINDOWPOSCHANGING:
		{
			LPWINDOWPOS pos = (LPWINDOWPOS)lParam;
			pos->y = gpConEmu->IsThemed() ? 2 : 1;

			if (gpSet->isHideCaptionAlways() && gpSet->nToolbarAddSpace > 0)
			{
				SIZE sz = {0,0};

				if (CallWindowProc(defaultProc, hwnd, TB_GETIDEALSIZE, 0, (LPARAM)&sz))
				{
					pos->cx = sz.cx + gpSet->nToolbarAddSpace;
					RECT rcParent; GetClientRect(GetParent(hwnd), &rcParent);
					pos->x = rcParent.right - pos->cx;
				}
			}

			return 0;
		}
		case TB_GETMAXSIZE:
		case TB_GETIDEALSIZE:
		{
			SIZE *psz = (SIZE*)lParam;

			if (!lParam) return 0;

			if (CallWindowProc(defaultProc, hwnd, uMsg, wParam, lParam))
			{
				psz->cx += (gpSet->isHideCaptionAlways() ? gpSet->nToolbarAddSpace : 0);
				return 1;
			}
		} break;
		case WM_RBUTTONUP:
		{
			TBBUTTON tb = {};
			POINT pt = {(int)(short)LOWORD(lParam),(int)(short)HIWORD(lParam)};
			int nIdx = SendMessage(hwnd, TB_HITTEST, 0, (LPARAM)&pt);
			// If the return value is zero or a positive value, it is
			// the zero-based index of the nonseparator item in which the point lies.
			if ((nIdx >= 0) && SendMessage(hwnd, TB_GETBUTTON, nIdx, (LPARAM)&tb))
			{
				OnMouseToolbar(WM_RBUTTONUP, tb.idCommand, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			}
			break;
		}
		case WM_SETFOCUS:
		{
			gpConEmu->setFocus();
			return 0;
		}
	}

	return CallWindowProc(defaultProc, hwnd, uMsg, wParam, lParam);
}


/* !!!Virtual!!! */

bool CTabPanelWin::IsSearchShownInt(bool bFilled)
{
	if (!mp_Find || !gpSet->isMultiShowSearch)
		return false;
	INT_PTR nPaneIndex = SendMessage(mh_Rebar, RB_IDTOINDEX, rbi_FindBar, 0);
	if (nPaneIndex < 0)
		return false;
	return bFilled ? mp_Find->IsAvailable(bFilled) : true;
}

HWND CTabPanelWin::ActivateSearchPaneInt(bool bActivate)
{
	if (!IsSearchShownInt(false))
		return NULL;
	return mp_Find->Activate(bActivate);
}

RECT CTabPanelWin::GetRect()
{
	RECT rcWnd = {-32000, -32000, -32000+300, -32000+100};
	if (ghWnd)
	{
		rcWnd = gpConEmu->RebarRect();
		if (IsRectEmpty(&rcWnd))
		{
			_ASSERTE(!IsRectEmpty(&rcWnd));
			gpConEmu->RequestRecalc();
			rcWnd = gpConEmu->RebarRect();
			if (IsRectEmpty(&rcWnd))
			{
				rcWnd = gpConEmu->ClientRect();
			}
		}
	}
	else
	{
		_ASSERTE(ghWnd!=NULL); // вроде ReBar для теста не создается.
	}
	return rcWnd;
}

void CTabPanelWin::CreateRebar()
{
	RECT rcWnd = GetRect();

	gpSetCls->CheckTheming();

	if ((mh_Rebar != NULL) && IsWindow(mh_Rebar))
	{
		_ASSERTE((mh_Rebar==NULL) && "Rebar was already created!");
		return;
	}

	mh_Rebar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
								(ghWnd ? (WS_VISIBLE | WS_CHILD) : 0)
								|WS_CLIPSIBLINGS|WS_CLIPCHILDREN
								|/*CCS_NORESIZE|*/CCS_NOPARENTALIGN
								|RBS_FIXEDORDER|RBS_AUTOSIZE|/*RBS_VARHEIGHT|*/CCS_NODIVIDER,
								rcWnd.left, rcWnd.top, RectWidth(rcWnd), RectHeight(rcWnd), ghWnd, NULL, g_hInstance, NULL);
	if (mh_Rebar == NULL)
	{
		return;
	}

	#if !defined(__GNUC__)
	#pragma warning (disable : 4312)
	#endif
	// Надо
	WNDPROC defaultProc = (WNDPROC)SetWindowLongPtr(mh_Rebar, GWLP_WNDPROC, (LONG_PTR)_ReBarProc);
	SetObj(mh_Rebar, this, defaultProc);


	REBARINFO     rbi = {sizeof(REBARINFO)};

	if (!SendMessage(mh_Rebar, RB_SETBARINFO, 0, (LPARAM)&rbi))
	{
		DisplayLastError(_T("Can't initialize rebar!"));
		DestroyWindow(mh_Rebar);
		mh_Rebar = NULL;
		return;
	}

	COLORREF clrBack = GetSysColor(COLOR_BTNFACE);
	SendMessage(mh_Rebar, RB_SETBKCOLOR, 0, clrBack);
	SendMessage(mh_Rebar, RB_SETWINDOWTHEME, 0, (LPARAM)L" ");

	// Пока табы есть всегда
	ShowTabsPane(true);

	if (gpSet->isMultiShowButtons)
		ShowToolsPane(true);

	if (gpSet->isMultiShowSearch)
		ShowSearchPane(true);

	RepositionInt();

	#ifdef _DEBUG
	RECT rc;
	GetWindowRect(mh_Rebar, &rc);
	#endif
}

void CTabPanelWin::DestroyRebar()
{
	SafeDelete(mp_Find);
	HWND* pWnd[] = {&mh_Toolbar, &mh_Tabbar, &mh_Rebar};
	for (INT_PTR i = 0; i < countof(pWnd); i++)
	{
		HWND h = *(pWnd[i]);
		*(pWnd[i]) = NULL;
		if (h) DestroyWindow(h);
	}
}

HWND CTabPanelWin::CreateTabbar()
{
	gpSetCls->CheckTheming();

	if (!mh_Rebar)
		return NULL; // создаётся только как Band в ReBar

	if (mh_Tabbar)
		return mh_Tabbar; // Уже создали

	RECT rcClient = GetRect();

	int nHeightExpected = QueryTabbarHeight();

	DWORD nPlacement = TCS_SINGLELINE|WS_VISIBLE|WS_CHILD;

	mh_Tabbar = CreateWindow(WC_TABCONTROL, NULL, nPlacement | WS_CLIPSIBLINGS | TCS_FOCUSNEVER,
							0, 0, RectWidth(rcClient), RectHeight(rcClient), mh_Rebar, NULL, g_hInstance, NULL);

	if (mh_Tabbar == NULL)
	{
		return NULL;
	}


	// Subclass it
	WNDPROC defaultProc = (WNDPROC)SetWindowLongPtr(mh_Tabbar, GWLP_WNDPROC, (LONG_PTR)_TabProc);
	SetObj(mh_Tabbar, this, defaultProc);

	// It may be NULL
	SendMessage(mh_Tabbar, TCM_SETIMAGELIST, 0, (LPARAM)mp_Owner->GetTabIcons(nHeightExpected));

	if (!mh_TabTip || !IsWindow(mh_TabTip))
		InitTooltips(mh_Tabbar);
	TabCtrl_SetToolTips(mh_Tabbar, mh_TabTip);

	UpdateTabFontInt();

	// Force tab item heights?
	TabCtrl_SetItemSize(mh_Tabbar, 80/*doesn't matter with variable width*/, nHeightExpected-3);

	if (!mh_Balloon || !IsWindow(mh_Balloon))
	{
		InitTooltips(mh_Tabbar);
	}

	// Add new of refresh existing tab label
	AddTabInt(gpConEmu->GetDefaultTabLabel(), 0, gpConEmu->mb_IsUacAdmin ? fwt_Elevated : fwt_Any, -1);

	// Retrieve created tabbar size (height)
	int iHeight = QueryTabbarHeight();
	_ASSERTE(nHeightExpected == iHeight);

	// And log it
	wchar_t szInfo[100];
	swprintf_c(szInfo, L"Created tab height=%i", iHeight);
	LogString(szInfo);
	DEBUGSTRSIZE(szInfo);

	return mh_Tabbar;
}

HWND CTabPanelWin::CreateToolbar()
{
	gpSetCls->CheckTheming();

	if (!mh_Rebar || !gpSet->isMultiShowButtons)
		return NULL; // создаётся только как Band в ReBar

	if (mh_Toolbar)
		return mh_Toolbar; // Уже создали

	mh_Toolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
	                            WS_CHILD|WS_VISIBLE|
	                            TBSTYLE_FLAT|CCS_NOPARENTALIGN|CCS_NORESIZE|CCS_NODIVIDER|
	                            TBSTYLE_TOOLTIPS|TBSTYLE_TRANSPARENT,
	                            0, 0, 0, 0, mh_Rebar,
	                            NULL, NULL, NULL);

	WNDPROC defaultProc = (WNDPROC)SetWindowLongPtr(mh_Toolbar, GWLP_WNDPROC, (LONG_PTR)_ToolProc);
	SetObj(mh_Toolbar, this, defaultProc);

	DWORD lExStyle = ((DWORD)SendMessage(mh_Toolbar, TB_GETEXTENDEDSTYLE, 0, 0)) | TBSTYLE_EX_DRAWDDARROWS;
	SendMessage(mh_Toolbar, TB_SETEXTENDEDSTYLE, 0, lExStyle);
	SendMessage(mh_Toolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);

	// Original size buttons was painted (no other resources yet)
	const int nOriginalSize = 14;
	// Preferred size of button?
	_ASSERTE(mn_TabHeight > 0);
	int nPrefSize = mn_TabHeight - 10;
	// Use it but not less than 14 pix
	int nBtnSize = std::max(nOriginalSize, nPrefSize);

	// But use fixed size for some ranges...
	int nSize125 = nOriginalSize * 125 / 100;
	int nSize150 = nOriginalSize * 150 / 100;
	int nSize200 = nOriginalSize * 2;
	if ((nBtnSize > nOriginalSize) && (nBtnSize < nSize125))
		nBtnSize = nOriginalSize;

	wchar_t szLog[100];
	swprintf_c(szLog, L"Creating toolbar for size %i px", nBtnSize);
	gpConEmu->LogString(szLog);
	DEBUGSTRSIZE(szLog);

	SendMessage(mh_Toolbar, TB_SETBITMAPSIZE, 0, MAKELONG(nBtnSize,nBtnSize));

	int iCreated = 0;
	if (mp_ToolImg->Create(nBtnSize, nBtnSize, BID_TOOLBAR_LAST_IDX+1, GetSysColor(COLOR_BTNFACE)))
	{
		iCreated += mp_ToolImg->AddButtonsMapped(g_hInstance, IDB_MAIN_TOOLBAR, 38, 1, 0, GetSysColor(COLOR_BTNFACE));
		iCreated += mp_ToolImg->AddButtonsMapped(g_hInstance, IDB_SCROLL, 1, 1, 0xC0C0C0, GetSysColor(COLOR_BTNFACE));
		_ASSERTE(iCreated == (BID_TOOLBAR_LAST_IDX+1));
	}

	TBADDBITMAP bmp = {NULL, (UINT_PTR)mp_ToolImg->GetBitmap()};
	int nFirst = SendMessage(mh_Toolbar, TB_ADDBITMAP, iCreated, (LPARAM)&bmp);
	_ASSERTE(BID_TOOLBAR_LAST_IDX==38);
	int nScrollBmp = BID_TOOLBAR_LAST_IDX;

	//buttons
	TBBUTTON btn = {0, 0, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP};
	TBBUTTON sep = {0, TID_MINIMIZE_SEP+1, TBSTATE_ENABLED, TBSTYLE_SEP};
	int nActiveCon = gpConEmu->ActiveConNum()+1;

	// New console
	btn.iBitmap = nFirst + BID_NEWCON_IDX;
	btn.idCommand = TID_CREATE_CON;
	btn.fsStyle = BTNS_DROPDOWN;
	btn.fsState = TBSTATE_ENABLED;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);

	// Console numbers
	btn.iBitmap = ((nActiveCon >= 0) ? (nFirst + BID_FIST_CON) : BID_DUMMYBTN_IDX);
	btn.idCommand = TID_ACTIVE_NUMBER;
	btn.fsStyle = BTNS_DROPDOWN;
	btn.fsState = TBSTATE_ENABLED;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);

	btn.fsStyle = BTNS_BUTTON;

	//// Show copying state
	//btn.iBitmap = nCopyBmp; btn.idCommand = TID_COPYING;
	//SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);
	//SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&sep); sep.idCommand++;

	// Buffer height mode
	btn.iBitmap = nFirst + BID_ALTERNATIVE_IDX; btn.idCommand = TID_ALTERNATIVE; btn.fsState = TBSTATE_ENABLED;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);
	// Show scroll state
	btn.iBitmap = nScrollBmp; btn.idCommand = TID_SCROLL;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);
	// Button for system menu
	btn.iBitmap = nFirst + BID_SYSMENU_IDX; btn.idCommand = TID_SYSMENU; btn.fsState = TBSTATE_ENABLED;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);
	// Separator before min/max/close
	BYTE bHideBtn = (!gpConEmu->isCaptionHidden() || gpConEmu->isInside()) ? TBSTATE_HIDDEN : 0;
	sep.fsState |= bHideBtn; sep.idCommand = TID_MINIMIZE_SEP;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&sep);
	// Min,Max,Close
	btn.iBitmap = nFirst + BID_MINIMIZE_IDX; btn.idCommand = TID_MINIMIZE; btn.fsState = TBSTATE_ENABLED|bHideBtn;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);
	btn.iBitmap = nFirst + ((gpConEmu->GetWindowMode() != wmNormal) ? BID_MAXIMIZE_IDX : BID_RESTORE_IDX); btn.idCommand = TID_MAXIMIZE;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);
	btn.iBitmap = nFirst + BID_APPCLOSE_IDX; btn.idCommand = TID_APPCLOSE;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);
	SendMessage(mh_Toolbar, TB_AUTOSIZE, 0, 0);
	#ifdef _DEBUG
	SIZE sz = {}; SendMessage(mh_Toolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz);
	#endif
	return mh_Toolbar;
}

// Прямоугольник в клиентских координатах ghWnd!
bool CTabPanelWin::GetRebarClientRect(RECT* rc)
{
	if (!mp_Owner->IsTabsActive())
		return false;

	HWND hWnd = mh_Rebar ? mh_Rebar : mh_Tabbar;
	GetWindowRect(hWnd, rc);
	MapWindowPoints(NULL, ghWnd, (LPPOINT)rc, 2);

	return true;
}

bool CTabPanelWin::IsCreated()
{
	return (this && mh_Rebar);
}

bool CTabPanelWin::IsTabbarCreated()
{
	return (this && mh_Tabbar);
}

bool CTabPanelWin::IsToolbarCreated()
{
	return (this && mh_Toolbar);
}

void CTabPanelWin::SetTabbarFont(HFONT hFont)
{
	if (!IsTabbarCreated())
	{
		_ASSERTE(IsTabbarCreated());
		return;
	}
	SendMessage(mh_Tabbar, WM_SETFONT, (WPARAM)hFont, TRUE);
}

// Screen-coordinates!
bool CTabPanelWin::GetTabRect(int nTabIdx, RECT* rcTab)
{
	if (!IsTabbarCreated() || !mp_Owner->IsTabsActive())
	{
		_ASSERTE(IsTabbarCreated());
		_ASSERTE(mp_Owner->IsTabsActive());
		return false;
	}

	if (nTabIdx < 0)
		nTabIdx = TabCtrl_GetCurSel(mh_Tabbar);

	TabCtrl_GetItemRect(mh_Tabbar, nTabIdx, rcTab);
	MapWindowPoints(mh_Tabbar, NULL, (LPPOINT)rcTab, 2);
	RECT rcBar = {}; GetWindowRect(mh_Rebar, &rcBar);
	rcTab->bottom = std::min(rcTab->bottom, rcBar.bottom);
	rcTab->left = std::max(rcTab->left-2, rcBar.left);

	return true;
}

bool CTabPanelWin::GetTabText(int nTabIdx, wchar_t* pszText, int cchTextMax)
{
	bool bRc = false;
	TCITEM item = {TCIF_TEXT};
	item.pszText = pszText;
	item.cchTextMax = cchTextMax;

	pszText[0] = 0;

	if (TabCtrl_GetItem(mh_Tabbar, nTabIdx, &item))
		bRc = true;

	return bRc;
}

// Screen coordinates, if bScreen==true
// -2 - out of control, -1 - out of tab-labels, 0+ - tab index 0-based
int CTabPanelWin::GetTabFromPoint(POINT ptCur, bool bScreen /*= true*/, bool bOverTabHitTest /*= true*/)
{
	int iTabIndex = -2; // Вообще вне контрола

	if (IsTabbarCreated())
	{
		RECT rcWnd = {}; GetWindowRect(mh_Tabbar, &rcWnd);
		RECT rcBar = {}; GetWindowRect(mh_Rebar, &rcBar);
		// 130911 - Let allow switching tabs by clicking over tabs headers (may be useful with fullscreen)
		RECT rcFull = {0, rcBar.top - rcWnd.top, rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top};

		TCHITTESTINFO tch = {ptCur};
		if (bScreen)
		{
			tch.pt.x -= rcWnd.left;
			tch.pt.y -= rcWnd.top;
		}

		if (PtInRect(&rcFull, tch.pt))
		{
			LRESULT nTest = TabCtrl_HitTest(mh_Tabbar, &tch);

			iTabIndex = (nTest <= -1) ? -1 : (int)nTest;

			if (iTabIndex == -1 && bOverTabHitTest)
			{
				// Clicked over tab?
				tch.pt.y = (rcWnd.bottom - rcWnd.top)/2;
				nTest = SendMessage(mh_Tabbar, TCM_HITTEST, 0, (LPARAM)&tch);
				if (nTest >= 0)
				{
					iTabIndex = (int)nTest;
				}
			}

		}
	}

	return iTabIndex;
}

// Screen(!) coordinates!
bool CTabPanelWin::GetToolBtnRect(int nCmd, RECT* rcBtnRect)
{
	if (!mp_Owner->IsTabsActive())
	{
		return false;
	}
	if (!mh_Toolbar)
	{
		_ASSERTE(mh_Toolbar);
		return false;
	}

	SendMessage(mh_Toolbar, TB_GETRECT, nCmd/*например TID_CREATE_CON*/, (LPARAM)rcBtnRect);
	MapWindowPoints(mh_Toolbar, NULL, (LPPOINT)rcBtnRect, 2);

	return true;
}

// Добавляет закладку, или меняет (при необходимости) заголовок существующей
void CTabPanelWin::AddTabInt(LPCWSTR text, int i, CEFarWindowType Flags, int iTabIcon)
{
	if (!IsTabbarCreated())
		return;

	bool bAdmin = ((Flags & fwt_Elevated)==fwt_Elevated);
	bool bHight = ((Flags & fwt_Highlighted)==fwt_Highlighted);
	int iIconIdx = mp_Owner->GetTabIcon(bAdmin);

	_ASSERTE(text && *text);

	TCITEM tie;
	// иконку обновляем всегда. она может измениться для таба
	tie.mask = TCIF_TEXT | (gpSet->isTabIcons ? TCIF_IMAGE : 0) | TCIF_STATE;
	tie.dwState = bHight ? TCIS_HIGHLIGHTED : 0;
	tie.dwStateMask = TCIS_HIGHLIGHTED;
	tie.pszText = (LPWSTR)text ;
	tie.iImage = (iTabIcon > 0) ? iTabIcon : mp_Owner->GetTabIcon(bAdmin); // Пока иконка только одна - для табов со щитом
	int nCurCount = GetItemCountInt();

	if (i>=nCurCount)
	{
		DEBUGSTRTABS(L"Add", i, text);
		TabCtrl_InsertItem(mh_Tabbar, i, &tie);
		if (bHight)
		{
			tie.mask = TCIF_STATE;
			TabCtrl_SetItem(mh_Tabbar, i, &tie);
		}
	}
	else
	{
		// Проверим, изменился ли текст
		wchar_t sOldTabName[CONEMUTABMAX];
		if (!GetTabText(i, sOldTabName, countof(sOldTabName)) || !wcscmp(sOldTabName, text))
			tie.mask &= ~TCIF_TEXT;
		#ifdef _DEBUG
		else
			DEBUGSTRTABS(L"Set", i, text);
		#endif

		// Изменилась ли иконка
		if (tie.mask & (TCIF_IMAGE|TCIF_STATE))
		{
			TCITEM told;
			told.mask = TCIF_IMAGE|TCIF_STATE;
			told.dwStateMask = TCIS_HIGHLIGHTED;
			TabCtrl_GetItem(mh_Tabbar, i, &told);

			if (told.iImage == tie.iImage)
				tie.mask &= ~TCIF_IMAGE;
			if ((told.dwState & TCIS_HIGHLIGHTED) == (tie.dwState & TCIS_HIGHLIGHTED))
				tie.mask &= ~TCIF_STATE;
		}

		// "меняем" только если он реально меняется
		if (tie.mask)
			TabCtrl_SetItem(mh_Tabbar, i, &tie);
	}
}

int CTabPanelWin::SelectTabInt(int i)
{
	int iCurSel = GetCurSelInt();
	mb_ChangeAllowed = true;

	wchar_t szInfo[120];
	swprintf_c(szInfo, L"SelectTabInt Tab=%i CurTab=%i", i+1, iCurSel+1);
	if (gpSet->isLogging()) { LogString(szInfo); } else { DEBUGSTRSEL(szInfo); }

	if (i != iCurSel)    // Меняем выделение, только если оно реально меняется
	{
		iCurSel = i;

		if (mh_Tabbar)
			TabCtrl_SetCurSel(mh_Tabbar, i);
	}

	mb_ChangeAllowed = false;

	return iCurSel;
}

int CTabPanelWin::GetCurSelInt()
{
	int nCurSel = mh_Tabbar ? TabCtrl_GetCurSel(mh_Tabbar) : 0;
	return nCurSel;
}

int CTabPanelWin::GetItemCountInt()
{
	_ASSERTE(IsTabbarCreated());
	int nCurCount = TabCtrl_GetItemCount(mh_Tabbar);
	return nCurCount;
}

void CTabPanelWin::DeleteItemInt(int I)
{
	if (!mh_Tabbar) return;

	TabCtrl_DeleteItem(mh_Tabbar, I);
}

void CTabPanelWin::ShowBar(bool bShow)
{
	int nShow = bShow ? SW_SHOW : SW_HIDE;

	if (mh_Rebar)
	{
		if (!IsWindowVisible(mh_Rebar))
			apiShowWindow(mh_Rebar, nShow);
	}
	else
	{
		_ASSERTE((mh_Rebar!=NULL) && "ReBar was not created!");
	}
}

void CTabPanelWin::RepositionInt()
{
	RECT rebarRect = gpConEmu->RebarRect();

	if (mh_Rebar)
	{
		wchar_t szLog[120];
		// Rebar location (top/bottom)
		msprintf(szLog, countof(szLog), L"New ReBar position {%i,%i}-{%i,%i} {%i*%i}", LOGRECTCOORDS(rebarRect), LOGRECTSIZE(rebarRect));
		LogString(szLog); DEBUGSTRSIZE(szLog);
		MoveWindow(mh_Rebar, rebarRect.left, rebarRect.top, RectWidth(rebarRect), RectHeight(rebarRect), TRUE);

		// Visible panes
		bool bRebarChanged = false;
		int iAllWidth = 0;
		// Desired panes position
		struct {
			ReBarIndex iPaneID;
			HWND hChild;
			int iPaneMinWidth;
			bool bNeedShow;
		} Panes[3] = {
			{rbi_TabBar}, {rbi_FindBar}, {rbi_ToolBar}
		};

		// Default (minimal) widths
		for (size_t i = 0; i < countof(Panes); i++)
		{
			switch (Panes[i].iPaneID)
			{
			case rbi_TabBar:
				Panes[i].hChild = mh_Tabbar;
				_ASSERTE(mn_TabHeight > 0);
				Panes[i].iPaneMinWidth = std::max(150,mn_TabHeight*5);
				break;
			case rbi_FindBar:
				if (mp_Find && ((Panes[i].hChild = mp_Find->GetHWND()) != NULL))
				{
					if (gpSet->isMultiShowSearch)
						Panes[i].iPaneMinWidth = mp_Find->GetMinWidth();
				}
				break;
			case rbi_ToolBar:
				if ((Panes[i].hChild = mh_Toolbar) != NULL)
				{
					SIZE sz = {0,0};
					SendMessage(mh_Toolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz);
					sz.cx += (gpSet->isHideCaptionAlways() ? gpSet->nToolbarAddSpace : 0);
					if (gpSet->isMultiShowButtons)
						Panes[i].iPaneMinWidth = sz.cx;
				}
				break;
			}
			iAllWidth += Panes[i].iPaneMinWidth;
		}

		// Check what panes are fit
		if (iAllWidth > RectWidth(rebarRect))
		{
			// Firstly we hide find-pane
			if (Panes[1].iPaneMinWidth > 0)
			{
				iAllWidth -= Panes[1].iPaneMinWidth;
				Panes[1].iPaneMinWidth = 0;
			}
			// If that is not enough - hide toolbar
			if (iAllWidth > RectWidth(rebarRect))
			{
				Panes[2].iPaneMinWidth = 0;
			}
		}

		// Log desired widths
		for (size_t i = 0; i < countof(Panes); i++)
		{
			msprintf(szLog, countof(szLog), L"Req pane width: Pane=%i width=%i needshow=%i", Panes[i].iPaneID, Panes[i].iPaneMinWidth, Panes[i].bNeedShow);
			LogString(szLog); DEBUGSTRPANE(szLog);
		}

		// Run through and hide don't fit panes
		for (size_t i = 1; i < countof(Panes); i++)
		{
			if (!Panes[i].hChild)
				continue;

			INT_PTR nPaneIndex = SendMessage(mh_Rebar, RB_IDTOINDEX, Panes[i].iPaneID, 0);
			if (nPaneIndex < 0)
				continue;

			if (IsWindowVisible(Panes[i].hChild))
			{
				if (Panes[i].iPaneMinWidth == 0)
				{
					msprintf(szLog, countof(szLog), L"Pane=%i is hidden", Panes[i].iPaneID);
					LogString(szLog); DEBUGSTRPANE(szLog);

					SendMessage(mh_Rebar, RB_SHOWBAND, nPaneIndex, 0);
					bRebarChanged = true;
				}
			}
			else if (Panes[i].iPaneMinWidth > 0)
			{
				msprintf(szLog, countof(szLog), L"Pane=%i is showing", Panes[i].iPaneID);
				LogString(szLog); DEBUGSTRPANE(szLog);

				Panes[i].bNeedShow = true;
			}
		}

		// Fix probably tangled order
		INT_PTR iAfter = 0;
		for (size_t i = 0; i < countof(Panes); i++)
		{
			if (Panes[i].iPaneMinWidth <= 0)
				continue;
			INT_PTR nIndex = SendMessage(mh_Rebar, RB_IDTOINDEX, Panes[i].iPaneID, 0);
			if (nIndex < 0)
				continue;
			if (nIndex > iAfter)
			{
				SendMessage(mh_Rebar, RB_MOVEBAND, nIndex, iAfter);
				bRebarChanged = true;
			}
			iAfter++;
		}

		// Show panes was hidden
		for (size_t i = 0; i < countof(Panes); i++)
		{
			if (!Panes[i].bNeedShow)
				continue;
			INT_PTR nIndex = SendMessage(mh_Rebar, RB_IDTOINDEX, Panes[i].iPaneID, 0);
			SendMessage(mh_Rebar, RB_SHOWBAND, nIndex, TRUE);
			bRebarChanged = true;
		}

		// If the toolbar is visible
		if (Panes[2].iPaneMinWidth > 0)
		{
			UpdateToolbarPos();
		}
	}
	else
	{
		_ASSERTE((mh_Rebar!=NULL) && "ReBar was not created!");
	}
}

void CTabPanelWin::UpdateToolbarPos()
{
	if (mh_Toolbar && gpSet->isMultiShowButtons && mp_Owner->IsTabsActive())
	{
		SIZE sz;
		SendMessage(mh_Toolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz);
		// В Win2k имеет место быть глюк вычисления размера (разделители)
		if ((gOSVer.dwMajorVersion == 5) && (gOSVer.dwMinorVersion == 0) && !gbIsWine)
			sz.cx += 26;

		if (mh_Rebar)
		{
			if (sz.cx != mn_LastToolbarWidth)
			{
				INT_PTR nBarIndex = SendMessage(mh_Rebar, RB_IDTOINDEX, rbi_ToolBar, 0);
				if (nBarIndex >= 0)
				{
					wchar_t szLog[120];
					msprintf(szLog, countof(szLog), L"Ajust ToolBar size to {%i,%i} Last width was %i", sz.cx, sz.cy, mn_LastToolbarWidth);
					LogString(szLog); DEBUGSTRSIZE(szLog);

					REBARBANDINFO rbBand= {REBARBANDINFO_SIZE}; // не используем size, т.к. приходит "новый" размер из висты и в XP обламываемся
					rbBand.fMask  = RBBIM_SIZE | RBBIM_CHILDSIZE;
					// Update band options.
					rbBand.cx = rbBand.cxMinChild = rbBand.cxIdeal = mn_LastToolbarWidth = sz.cx;
					rbBand.cyMinChild = sz.cy;
					SendMessage(mh_Rebar, RB_SETBANDINFO, nBarIndex, (LPARAM)&rbBand);
				}
			}
		}
		else
		{
			_ASSERTE((mh_Rebar!=NULL) && "ReBar was not created!");
		}
	}
}

bool CTabPanelWin::OnNotifyInt(LPNMHDR nmhdr, LRESULT& lResult)
{
	//gpConEmu->setFocus(); // 02.04.2009 Maks - ?
	if (nmhdr->code == TCN_SELCHANGING)
	{
		// Returns TRUE to prevent the selection from changing, or FALSE to allow the selection to change.
		mn_prevTab = GetCurSelInt();
		lResult = FALSE; // Allow tab change
		return true; // Processed
	}

	if (nmhdr->code == TCN_SELCHANGE)
	{
		InterlockedIncrement(&mn_InSelChange);
		int lnNewTab = GetCurSelInt();

		wchar_t szInfo[120];
		swprintf_c(szInfo, L"WinApi tab was changed: NewTab=%i", lnNewTab+1);
		if (gpSet->isLogging()) { LogString(szInfo); } else { DEBUGSTRSEL(szInfo); }

		bool bRollbackTabs = false, bRefreshTabs = false;

		if (!mb_ChangeAllowed)
		{
			if (!FarSendChangeTab(lnNewTab))
			{
				bRollbackTabs = bRefreshTabs = true;
			}
			// start waiting for title to change
			//_titleShouldChange = true;
		}
		else
		{
			// This message was intended to be received from user interactions only
			_ASSERTE(FALSE && "Must not be triggered from internal calls?");
			bRollbackTabs = true;
		}

		if (bRollbackTabs)
		{
			if (mn_prevTab>=0)
			{
				SelectTabInt(mn_prevTab);
				mn_prevTab = -1;
			}
			if (bRefreshTabs)
			{
				mp_Owner->Update();
			}
		}

		if (gpSet->isLogging()) { LogString(L"WinApi tab change was finished"); } else { DEBUGSTRSEL(L"WinApi tab change was finished"); }
		mn_LastChangeTick = GetTickCount();
		InterlockedDecrement(&mn_InSelChange);
		lResult = FALSE; // Value ignored actually
		return true; // Processed
	}

	return false; // Continue processing...
}

bool CTabPanelWin::GetToolBtnChecked(ToolbarCommandIdx iCmd)
{
	LRESULT lState = SendMessage(mh_Toolbar, TB_GETSTATE, iCmd, 0);
	bool lbChecked = (lState & TBSTATE_CHECKED) == TBSTATE_CHECKED;
	return lbChecked;
}

bool CTabPanelWin::IsTabbarNotify(LPNMHDR nmhdr)
{
	return (mh_Tabbar && (nmhdr->hwndFrom == mh_Tabbar || nmhdr->hwndFrom == mh_TabTip));
}

bool CTabPanelWin::IsToolbarNotify(LPNMHDR nmhdr)
{
	return (mh_Toolbar && (nmhdr->hwndFrom == mh_Toolbar));
}

bool CTabPanelWin::IsToolbarCommand(WPARAM wParam, LPARAM lParam)
{
	return (mh_Toolbar && (mh_Toolbar == (HWND)lParam));
}

void CTabPanelWin::SetToolBtnChecked(ToolbarCommandIdx iCmd, bool bChecked)
{
	if (!mh_Toolbar)
		return;
	SendMessage(mh_Toolbar, TB_CHECKBUTTON, iCmd, bChecked);
}

void CTabPanelWin::OnCaptionHiddenChanged(bool bCaptionHidden)
{
	if (!mh_Toolbar)
		return;

	bool lbHideBtn = !bCaptionHidden || gpConEmu->isInside();

	#ifdef _DEBUG
	SIZE sz1 = {}; SendMessage(mh_Toolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz1);
	#endif

	OnWindowStateChanged(gpConEmu->GetWindowMode());
	SendMessage(mh_Toolbar, TB_HIDEBUTTON, TID_MINIMIZE_SEP, lbHideBtn);
	SendMessage(mh_Toolbar, TB_HIDEBUTTON, TID_MINIMIZE, lbHideBtn);
	SendMessage(mh_Toolbar, TB_HIDEBUTTON, TID_MAXIMIZE, lbHideBtn);
	SendMessage(mh_Toolbar, TB_HIDEBUTTON, TID_APPCLOSE, lbHideBtn);
	SendMessage(mh_Toolbar, TB_AUTOSIZE, 0, 0);

	#ifdef _DEBUG
	SIZE sz2 = {}; SendMessage(mh_Toolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz2);
	#endif

	RepositionInt();
}

void CTabPanelWin::OnWindowStateChanged(ConEmuWindowMode newMode)
{
	if (mh_Toolbar)
	{
		TBBUTTONINFO tbi = {sizeof(TBBUTTONINFO), TBIF_IMAGE};
		tbi.iImage = (newMode != wmNormal) ? BID_MAXIMIZE_IDX : BID_RESTORE_IDX;
		SendMessage(mh_Toolbar, TB_SETBUTTONINFO, TID_MAXIMIZE, (LPARAM)&tbi);
		//OnCaptionHidden();
	}
}

// nConNumber - 0-based
void CTabPanelWin::OnConsoleActivatedInt(int nConNumber)
{
	if (!mh_Toolbar)
		return;

	TBBUTTONINFO tbi = {sizeof(TBBUTTONINFO), TBIF_IMAGE};
	SendMessage(mh_Toolbar, TB_GETBUTTONINFO, TID_ACTIVE_NUMBER, (LPARAM)&tbi);

	if (tbi.iImage != nConNumber)
	{
		bool bNeedShow = false, bWasHidden = false;

		if ((nConNumber >= BID_FIST_CON) && (nConNumber <= BID_LAST_CON))
		{
			bNeedShow = (tbi.iImage == BID_DUMMYBTN_IDX);
			tbi.iImage = nConNumber;
		}
		else
		{
			if (tbi.iImage != BID_DUMMYBTN_IDX)
			{
				SendMessage(mh_Toolbar, TB_HIDEBUTTON, TID_ACTIVE_NUMBER, TRUE);
				bWasHidden = true;
			}
			tbi.iImage = BID_DUMMYBTN_IDX;
		}

		SendMessage(mh_Toolbar, TB_SETBUTTONINFO, TID_ACTIVE_NUMBER, (LPARAM)&tbi);

		if (bNeedShow)
		{
			SendMessage(mh_Toolbar, TB_HIDEBUTTON, TID_ACTIVE_NUMBER, FALSE);
		}

		if (bNeedShow || bWasHidden)
		{
			SendMessage(mh_Toolbar, TB_AUTOSIZE, 0, 0);
			UpdateToolbarPos();
		}
	}
}

void CTabPanelWin::ShowTabsPane(bool bShow)
{
	if (bShow)
	{
		_ASSERTE(isMainThread());
		if (CreateTabbar())
		{
			RECT rcWnd = GetRect();

			REBARBANDINFO rbBand = {REBARBANDINFO_SIZE}; // не используем size, т.к. приходит "новый" размер из висты и в XP обламываемся

			rbBand.fMask  = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | RBBIM_STYLE;
			rbBand.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDSIZE | RBBS_VARIABLEHEIGHT;

			#if 0
			rbBand.fMask |= RBBIM_COLORS;
			rbBand.clrFore = RGB(255,255,255);
			rbBand.clrBack = RGB(0,0,0);
			#endif

			_ASSERTE(mn_TabHeight > 0);

			rbBand.wID        = rbi_TabBar;
			rbBand.hwndChild  = mh_Tabbar;
			rbBand.cxMinChild = 100;
			rbBand.cx = /*rbBand.cxIdeal =*/ rcWnd.right - 20; // - sz.cx - 80 - iFindPanelWidth;
			rbBand.cyChild = rbBand.cyMinChild = rbBand.cyMaxChild = mn_TabHeight; // sz.cy;

			// Tabs are always on the first place
			if (!SendMessage(mh_Rebar, RB_INSERTBAND, (WPARAM)0, (LPARAM)&rbBand))
			{
				DisplayLastError(_T("Can't initialize rebar (tabbar)"));
			}
		}
	}
	else
	{
		_ASSERTE(FALSE && "Deletion is not supported yet");
	}
}

// Returns previous state
bool CTabPanelWin::ShowSearchPane(bool bShow, bool bCtrlOnly /*= false*/)
{
	bool bWasShown = false;

	if (bCtrlOnly)
	{
		HWND hPane = mp_Find ? mp_Find->GetHWND() : NULL;
		bWasShown = IsSearchShownInt(false);
		if (hPane)
			::ShowWindow(hPane, bShow ? SW_SHOWNORMAL : SW_HIDE);
		goto wrap;
	}

	if (bShow && gpSet->isMultiShowSearch)
	{
		if (!mh_Rebar)
			goto wrap; // created only as band in ReBar

		_ASSERTE(isMainThread());
		if (!IsSearchShownInt(false))
		{
			REBARBANDINFO rbBand = {REBARBANDINFO_SIZE}; // не используем size, т.к. приходит "новый" размер из висты и в XP обламываемся

			HWND hFindPane = NULL;
			if (!mp_Find)
				mp_Find = new CFindPanel(gpConEmu);

			int iPaneHeight;
			SIZE sz = {0,0};
			_ASSERTE(mn_TabHeight > 0);
			if (mn_TabHeight > 0)
			{
				iPaneHeight = mn_TabHeight;
			}
			else if (mh_Toolbar)
			{
				SendMessage(mh_Toolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz);
				iPaneHeight = sz.cy;
			}
			else
			{
				iPaneHeight = QueryTabbarHeight() - 4;
			}

			hFindPane = mp_Find->CreatePane(mh_Rebar, iPaneHeight);

			if (hFindPane)
			{
				rbBand.fMask  = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | RBBIM_STYLE;
				rbBand.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDSIZE /*| RBBS_VARIABLEHEIGHT*/;

				#if 0
				rbBand.fMask |= RBBIM_COLORS;
				rbBand.clrFore = RGB(255,255,255);
				rbBand.clrBack = RGB(0,255,0);
				#endif

				rbBand.wID        = rbi_FindBar;
				rbBand.hwndChild  = hFindPane;
				rbBand.cx = rbBand.cxMinChild = rbBand.cxIdeal = mp_Find->GetMinWidth();
				rbBand.cyChild = rbBand.cyMinChild = rbBand.cyMaxChild = iPaneHeight;
			}

			// Insert before toolbar
			INT_PTR nPaneIndex = SendMessage(mh_Rebar, RB_IDTOINDEX, rbi_ToolBar, 0);
			if (nPaneIndex < 0) nPaneIndex = -1;

			if (!hFindPane
				|| !SendMessage(mh_Rebar, RB_INSERTBAND, nPaneIndex, (LPARAM)&rbBand)
				|| !mp_Find->OnCreateFinished())
			{
				DisplayLastError(_T("Can't initialize rebar (searchbar)"));
				bShow = false;
			}
		}
		else
		{
			bWasShown = true;
		}
	}

	// Delete band?
	if (!bShow)
	{
		_ASSERTEX(!bShow);
		INT_PTR nPaneIndex = SendMessage(mh_Rebar, RB_IDTOINDEX, rbi_FindBar, 0);
		if (nPaneIndex >= 0)
		{
			SendMessage(mh_Rebar, RB_DELETEBAND, nPaneIndex, 0);
			SafeDelete(mp_Find);
			bWasShown = true;
		}
	}

wrap:
	return bWasShown;
}

void CTabPanelWin::ShowToolsPane(bool bShow)
{
	if (bShow)
	{
		_ASSERTE(isMainThread());
		if (!IsToolbarCreated() && (CreateToolbar() != NULL))
		{
			REBARBANDINFO rbBand = {REBARBANDINFO_SIZE}; // не используем size, т.к. приходит "новый" размер из висты и в XP обламываемся

			rbBand.fMask  = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | RBBIM_STYLE;
			rbBand.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDSIZE | RBBS_VARIABLEHEIGHT;

			#if 0
			rbBand.fMask |= RBBIM_COLORS;
			rbBand.clrFore = RGB(255,255,255);
			rbBand.clrBack = RGB(0,0,255);
			#endif

			SIZE sz = {0,0};
			SendMessage(mh_Toolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz);

			wchar_t szLog[120];
			msprintf(szLog, countof(szLog), L"Create ToolBar with size {%i,%i} Last width was %i", sz.cx, sz.cy, mn_LastToolbarWidth);
			LogString(szLog); DEBUGSTRSIZE(szLog);

			rbBand.wID        = rbi_ToolBar;
			rbBand.hwndChild  = mh_Toolbar;
			rbBand.cx = rbBand.cxMinChild = rbBand.cxIdeal = mn_LastToolbarWidth = sz.cx;
			rbBand.cyChild = rbBand.cyMinChild = rbBand.cyMaxChild = sz.cy + (gpConEmu->IsThemed() ? 0 : 2);

			if (!SendMessage(mh_Rebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand))
			{
				DisplayLastError(_T("Can't initialize rebar (toolbar)"));
			}
		}
	}
	else
	{
		_ASSERTEX(!bShow);
		INT_PTR nPaneIndex = SendMessage(mh_Rebar, RB_IDTOINDEX, rbi_ToolBar, 0);
		if (nPaneIndex >= 0)
		{
			SendMessage(mh_Rebar, RB_DELETEBAND, nPaneIndex, 0);
			DestroyWindow(mh_Toolbar);
			mh_Toolbar = NULL;
		}
	}
}

// #SIZE_TODO Eliminate the function? Or rename to GetDefaultTabbarHeight?
int CTabPanelWin::QueryTabbarHeight()
{
	if (!this) return 0;

	// Размеры таба через TabCtrl_AdjustRect считаются криво при прыжках по мониторам
	//_ASSERTE((hTabs!=NULL) && "Creating of a dummy tab control failed");
	int newHeight = gpConEmu->GetDefaultTabbarHeight();
	if (mn_TabHeight != newHeight)
	{
		wchar_t szInfo[100];
		msprintf(szInfo, countof(szInfo), L"CTabPanelWin::mn_TabHeight changed from %i to %i", mn_TabHeight, newHeight);
		LogString(szInfo);
		DEBUGSTRSIZE(szInfo);
		mn_TabHeight = newHeight;
	}

	return mn_TabHeight;
}

void CTabPanelWin::InvalidateBar()
{
	if (this && mh_Rebar)
	{
		InvalidateRect(mh_Rebar, NULL, TRUE);
		if (mp_Find)
			mp_Find->Invalidate();
	}
}

void CTabPanelWin::RePaintInt()
{
	if (!mh_Rebar)
		return;

	RECT client, self;
	client = gpConEmu->RebarRect();
	GetWindowRect(mh_Rebar, &self);
	MapWindowPoints(NULL, ghWnd, (LPPOINT)&self, 2);

	if (client != self)
	{
		RepositionInt();

		#if 0
		MoveWindow(mh_Rebar, 0, nNewY, client.right, self.bottom-self.top, 1);

		if (gpSet->nTabsLocation == 1)
			m_Margins = MakeRect(0,0,0,mn_TabHeight);
		else
			m_Margins = MakeRect(0,mn_TabHeight,0,0);
		gpSet->UpdateMargins(m_Margins);
		#endif
	}

	UpdateWindow(mh_Rebar);
}

LRESULT CTabPanelWin::TabHitTest(bool abForce /*= false*/, int* pnOverTabHit /*= NULL*/)
{
	if (pnOverTabHit)
		*pnOverTabHit = -1;

	if (gpSet->isTabs && (abForce || gpConEmu->isCaptionHidden()))
	{
		if (mp_Owner->IsTabsActive())
		{
			POINT ptCur = {}; GetCursorPos(&ptCur);
			int nTabIdx = GetTabFromPoint(ptCur);
			// -2 - out of control, -1 - out of tab-labels, 0+ - tab index 0-based
			if (nTabIdx == -1)
				return HTCAPTION;
			if (pnOverTabHit)
				*pnOverTabHit = nTabIdx;
		}
	}

	return HTNOWHERE;
}

bool CTabPanelWin::GetTabBarClientRect(RECT* rcTab)
{
	if (!mh_Tabbar)
		return false;
	GetClientRect(mh_Tabbar, rcTab);
	return true;
}

void CTabPanelWin::HighlightTab(int iTab, bool bHighlight)
{
	TCITEM tie;
	tie.mask = TCIF_STATE;
	tie.dwState = bHighlight ? TCIS_HIGHLIGHTED : 0;
	tie.dwStateMask = TCIS_HIGHLIGHTED;
	TabCtrl_SetItem(mh_Tabbar, iTab, &tie);
}
