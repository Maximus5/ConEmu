
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
#include "TabCtrlWin.h"
#include "Options.h"
#include "ConEmu.h"
#include "VirtualConsole.h"
#include "TrayIcon.h"
#include "VConChild.h"
#include "VConGroup.h"
#include "Status.h"
#include "Menu.h"
#include "../common/MMap.h"

//WNDPROC CTabPanelWin::_defaultTabProc = NULL;
//WNDPROC CTabPanelWin::_defaultToolProc = NULL;
//WNDPROC CTabPanelWin::_defaultReBarProc = NULL;

struct TabPanelWinMap { CTabPanelWin* object; HWND hWnd; WNDPROC defaultProc; };
static MMap<HWND,TabPanelWinMap>* gp_TabPanelWinMap = NULL;

typedef BOOL (WINAPI* FAppThemed)();

CTabPanelWin::CTabPanelWin(CTabBarClass* ap_Owner)
	: CTabPanelBase(ap_Owner)
{
	mh_Toolbar = NULL; mh_Tabbar = NULL; mh_Rebar = NULL;
	mb_ChangeAllowed = false;
	mn_LastToolbarWidth = 0;
	mn_ThemeHeightDiff = 0;
	gp_TabPanelWinMap = (MMap<HWND,TabPanelWinMap>*)calloc(1,sizeof(*gp_TabPanelWinMap));
	gp_TabPanelWinMap->Init(8);
	mn_TabHeight = 0;
}

CTabPanelWin::~CTabPanelWin()
{
	SafeFree(gp_TabPanelWinMap);
}


/* Window procedures */

LRESULT CALLBACK CTabPanelWin::_ReBarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc;
	TabPanelWinMap map = {NULL};
	if (gp_TabPanelWinMap && gp_TabPanelWinMap->Get(hwnd, &map))
		lRc = map.object->ReBarProc(hwnd, uMsg, wParam, lParam, map.defaultProc);
	else
		lRc = ::DefWindowProc(hwnd, uMsg, wParam, lParam);
	return lRc;
}

LRESULT CTabPanelWin::ReBarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC defaultProc)
{
	int nOverTabHit = -1;

	switch(uMsg)
	{
		case WM_WINDOWPOSCHANGING:
		{
			LPWINDOWPOS pos = (LPWINDOWPOS)lParam;
			if (mn_TabHeight)
			{
				pos->cy = mn_TabHeight;
				return 0;
			}
			break;
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

			if (gpSet->isTabs && !gpSet->isQuakeStyle
				&& (gpConEmu->GetWindowMode() == wmNormal)
				&& gpSet->isCaptionHidden())
			{
				if (TabHitTest() == HTCAPTION)
				{
					SetCursor(/*gpSet->isQuakeStyle ? gpConEmu->mh_CursorArrow :*/ gpConEmu->mh_CursorMove);
					return TRUE;
				}
			}

			break;
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		/*case WM_RBUTTONDOWN:*/ case WM_RBUTTONUP: //case WM_RBUTTONDBLCLK:

			if (((uMsg == WM_RBUTTONUP)
					|| ((uMsg == WM_LBUTTONDBLCLK) && gpSet->nTabBarDblClickAction)
					|| gpSet->isCaptionHidden())
				&& gpSet->isTabs)
			{
				if (TabHitTest(true, &nOverTabHit)==HTCAPTION)
				{
					POINT ptScr; GetCursorPos(&ptScr);
					lParam = MAKELONG(ptScr.x,ptScr.y);
					LRESULT lRc = 0;

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
						gpConEmu->mp_Menu->ShowSysmenu(ptScr.x, ptScr.y/*-32000*/);
					}
					else if (gpConEmu->WindowMode == wmNormal)
					{
						lRc = gpConEmu->WndProc(ghWnd, uMsg-(WM_MOUSEMOVE-WM_NCMOUSEMOVE), HTCAPTION, lParam);
					}

					return lRc;
				}
				else if ((nOverTabHit >= 0) && ((uMsg == WM_LBUTTONDOWN) || (uMsg == WM_LBUTTONUP) || (uMsg == WM_RBUTTONDOWN) || (uMsg == WM_RBUTTONUP)))
				{
					if (uMsg == WM_LBUTTONDOWN)
					{
						int lnCurTab = GetCurSelInt();
						if (lnCurTab != nOverTabHit)
						{
							FarSendChangeTab(nOverTabHit);
							//mp_Owner->SelectTab(nOverTabHit);
						}
					}
					else if (uMsg == WM_RBUTTONUP)
					{
						mp_Owner->OnMouse(uMsg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
					}
				}
			}

			break;

		case WM_MBUTTONUP:
			if (gpSet->isCaptionHidden() && gpSet->isTabs)
			{
				if ((TabHitTest(true, &nOverTabHit) != HTCAPTION) && (nOverTabHit >= 0))
				{
					mp_Owner->OnMouse(uMsg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
					break;
				}
			}
			gpConEmu->RecreateAction(cra_CreateTab/*FALSE*/, gpSet->isMultiNewConfirm || isPressed(VK_SHIFT));
			break;
	}

	return CallWindowProc(defaultProc, hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CTabPanelWin::_TabProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc;
	TabPanelWinMap map = {NULL};
	if (gp_TabPanelWinMap && gp_TabPanelWinMap->Get(hwnd, &map))
		lRc = map.object->TabProc(hwnd, uMsg, wParam, lParam, map.defaultProc);
	else
		lRc = ::DefWindowProc(hwnd, uMsg, wParam, lParam);
	return lRc;
}

LRESULT CTabPanelWin::TabProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC defaultProc)
{
	switch(uMsg)
	{
		case WM_WINDOWPOSCHANGING:
		{
			if (mh_Rebar)
			{
				LPWINDOWPOS pos = (LPWINDOWPOS)lParam;

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
			mp_Owner->OnMouse(uMsg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
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
	if (gp_TabPanelWinMap && gp_TabPanelWinMap->Get(hwnd, &map))
		lRc = map.object->ToolProc(hwnd, uMsg, wParam, lParam, map.defaultProc);
	else
		lRc = ::DefWindowProc(hwnd, uMsg, wParam, lParam);
	return lRc;
}

// Window procedure for Toolbar (Multiconsole, BufferHeight, Min/Max, etc.)
LRESULT CTabPanelWin::ToolProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC defaultProc)
{
	switch(uMsg)
	{
		case WM_WINDOWPOSCHANGING:
		{
			LPWINDOWPOS pos = (LPWINDOWPOS)lParam;
			pos->y = (mn_ThemeHeightDiff == 0) ? 2 : 1;

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
			POINT pt = {(int)(short)LOWORD(lParam),(int)(short)HIWORD(lParam)};
			int nIdx = SendMessage(hwnd, TB_HITTEST, 0, (LPARAM)&pt);

			// If the return value is zero or a positive value, it is
			// the zero-based index of the nonseparator item in which the point lies.
			//if (nIdx >= 0 && nIdx < MAX_CONSOLE_COUNT)
			_ASSERTE(TID_ACTIVE_NUMBER==1);
			if (nIdx == (TID_ACTIVE_NUMBER-1))
			{
				CVConGuard VCon;
				CVirtualConsole* pVCon = (gpConEmu->GetActiveVCon(&VCon) >= 0) ? VCon.VCon() : NULL;

				if (!gpConEmu->isActive(pVCon, false))
				{
					if (!gpConEmu->ConActivate(nIdx))
					{
						if (!gpConEmu->isActive(pVCon, false))
						{
							return 0;
						}
					}
				}

				//ClientToScreen(hwnd, &pt);
				RECT rcBtnRect = {0};
				SendMessage(hwnd, TB_GETRECT, TID_ACTIVE_NUMBER, (LPARAM)&rcBtnRect);
				MapWindowPoints(hwnd, NULL, (LPPOINT)&rcBtnRect, 2);
				POINT pt = {rcBtnRect.right,rcBtnRect.bottom};

				gpConEmu->mp_Menu->ShowPopupMenu(pVCon, pt, TPM_RIGHTALIGN|TPM_TOPALIGN);
			}
			else
			{
				LRESULT nTestIdxMin = SendMessage(hwnd, TB_COMMANDTOINDEX, TID_MINIMIZE, 0);
				LRESULT nTestIdxClose = SendMessage(hwnd, TB_COMMANDTOINDEX, TID_APPCLOSE, 0);
				if ((nIdx == nTestIdxMin) || (nIdx == nTestIdxClose))
				{
					Icon.HideWindowToTray();
					return 0;
				}

				LRESULT nTestIdxMax = SendMessage(hwnd, TB_COMMANDTOINDEX, TID_MAXIMIZE, 0);
				if (nIdx == nTestIdxMax)
				{
					gpConEmu->DoFullScreen();
					return 0;
				}

				LRESULT nTestIdx = SendMessage(hwnd, TB_COMMANDTOINDEX, TID_CREATE_CON, 0);
				if (nIdx == nTestIdx)
				{
					gpConEmu->RecreateAction(cra_CreateTab/*FALSE*/, TRUE);
				}
			}

			return 0;
		}
	}

	return CallWindowProc(defaultProc, hwnd, uMsg, wParam, lParam);
}


/* !!!Virtual!!! */

void CTabPanelWin::CreateRebar()
{
	RECT rcWnd = {-32000, -32000, -32000+300, -32000+100};
	if (ghWnd)
	{
		rcWnd = gpConEmu->GetGuiClientRect();
	}
	else
	{
		_ASSERTE(ghWnd!=NULL); // вроде ReBar для теста не создается.
	}

	gpSetCls->CheckTheming();

	if (NULL == (mh_Rebar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
								(ghWnd ? (WS_VISIBLE | WS_CHILD) : 0)
								|WS_CLIPSIBLINGS|WS_CLIPCHILDREN
	                            |/*CCS_NORESIZE|*/CCS_NOPARENTALIGN
	                            |RBS_FIXEDORDER|RBS_AUTOSIZE|/*RBS_VARHEIGHT|*/CCS_NODIVIDER,
	                            0,0,rcWnd.right,16, ghWnd, NULL, g_hInstance, NULL)))
		return;

#if !defined(__GNUC__)
#pragma warning (disable : 4312)
#endif
	// Надо
	TabPanelWinMap map = {this}; //{ CTabPanelWin* object; HWND hWnd; WNDPROC defaultProc; };
	map.defaultProc = (WNDPROC)SetWindowLongPtr(mh_Rebar, GWLP_WNDPROC, (LONG_PTR)_ReBarProc);
	map.hWnd = mh_Rebar;
	gp_TabPanelWinMap->Set(mh_Rebar, map);


	REBARINFO     rbi = {sizeof(REBARINFO)};
	REBARBANDINFO rbBand = {80}; // не используем size, т.к. приходит "новый" размер из висты и в XP обламываемся

	if (!SendMessage(mh_Rebar, RB_SETBARINFO, 0, (LPARAM)&rbi))
	{
		DisplayLastError(_T("Can't initialize rebar!"));
		DestroyWindow(mh_Rebar);
		mh_Rebar = NULL;
		return;
	}

	rbBand.fMask  = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | RBBIM_STYLE | RBBIM_COLORS;
	rbBand.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDSIZE | RBBS_VARIABLEHEIGHT;
	rbBand.clrBack = GetSysColor(COLOR_BTNFACE);
	rbBand.clrFore = GetSysColor(COLOR_BTNTEXT);
	SendMessage(mh_Rebar, RB_SETBKCOLOR, 0, GetSysColor(COLOR_BTNFACE));
	SendMessage(mh_Rebar, RB_SETWINDOWTHEME, 0, (LPARAM)L" ");
	CreateTabbar();
	CreateToolbar();
	SIZE sz = {0,0};

	if (mh_Toolbar)
	{
		SendMessage(mh_Toolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz);
	}
	else
	{
		RECT rcClient = gpConEmu->GetGuiClientRect();
		TabCtrl_AdjustRect(mh_Tabbar, FALSE, &rcClient);
		sz.cy = rcClient.top - 3 - mn_ThemeHeightDiff;
	}

	if (mh_Tabbar)
	{
		// Set values unique to the band with the toolbar.
		rbBand.wID          = 1;
		rbBand.hwndChild  = mh_Tabbar;
		rbBand.cxMinChild = 100;
		rbBand.cx = rbBand.cxIdeal = rcWnd.right - sz.cx - 80;
		rbBand.cyChild = rbBand.cyMinChild = rbBand.cyMaxChild = mn_TabHeight; // sz.cy;

		if (!SendMessage(mh_Rebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand))
		{
			DisplayLastError(_T("Can't initialize rebar (tabbar)"));
		}
	}

	if (mh_Toolbar)
	{
		// Set values unique to the band with the toolbar.
		rbBand.wID        = 2;
		rbBand.hwndChild  = mh_Toolbar;
		rbBand.cx = rbBand.cxMinChild = rbBand.cxIdeal = mn_LastToolbarWidth = sz.cx;
		rbBand.cyChild = rbBand.cyMinChild = rbBand.cyMaxChild = sz.cy + mn_ThemeHeightDiff;

		// Add the band that has the toolbar.
		if (!SendMessage(mh_Rebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand))
		{
			DisplayLastError(_T("Can't initialize rebar (toolbar)"));
		}

		//if (mn_ThemeHeightDiff) {
		//	POINT pt = {0,0};
		//	MapWindowPoints(mh_Toolbar, mh_Rebar, &pt, 1);
		//	pt.y = 0;
		//	SetWindowPos(mh_Toolbar, NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
		//}
	}

#ifdef _DEBUG
	RECT rc;
	GetWindowRect(mh_Rebar, &rc);
#endif
	//GetWindowRect(mh_Rebar, &rc);
	//mn_TabHeight = rc.bottom - rc.top;
	//if (gpSet->nTabsLocation == 1)
	//	m_Margins = MakeRect(0,0,0,mn_TabHeight);
	//else
	//	m_Margins = MakeRect(0,mn_TabHeight,0,0);
	//gpSet->UpdateMargins(m_Margins);
	//_hwndTab = mh_Rebar; // пока...
}

HWND CTabPanelWin::CreateTabbar()
{
	gpSetCls->CheckTheming();

	if (!mh_Rebar)
		return NULL; // Табы создаются только как Band в ReBar

	if (mh_Tabbar)
		return mh_Tabbar; // Уже создали

	//if (!m_TabIcons.IsInitialized())
	//{
	//	InitIconList();
	//}

	// Важно проверку делать после создания главного окна, иначе IsAppThemed будет возвращать FALSE
	BOOL bAppThemed = FALSE, bThemeActive = FALSE;
	FAppThemed pfnThemed = NULL;
	HMODULE hUxTheme = LoadLibrary(L"UxTheme.dll");

	if (hUxTheme)
	{
		pfnThemed = (FAppThemed)GetProcAddress(hUxTheme, "IsAppThemed");

		if (pfnThemed) bAppThemed = pfnThemed();

		pfnThemed = (FAppThemed)GetProcAddress(hUxTheme, "IsThemeActive");

		if (pfnThemed) bThemeActive = pfnThemed();

		FreeLibrary(hUxTheme); hUxTheme = NULL;
	}

	if (!bAppThemed || !bThemeActive)
		mn_ThemeHeightDiff = 2;

	/*mh_TabbarP = CreateWindow(_T("VirtualConsoleClassBar"), _T(""),
	        WS_VISIBLE|WS_CHILD, 0,0,340,22, ghWnd, 0, 0, 0);
	if (!mh_TabbarP) return NULL;*/
	RECT rcClient = {-32000, -32000, -32000+300, -32000+100};
	if (ghWnd)
	{
		rcClient = gpConEmu->GetGuiClientRect();
	}
	else
	{
		_ASSERTE(ghWnd!=NULL && "ConEmu main window must be created already!");
	}
	
	DWORD nPlacement = TCS_SINGLELINE|WS_VISIBLE|WS_CHILD/*|TCS_BUTTONS*//*|TCS_TOOLTIPS*/;
			//|((gpSet->nTabsLocation == 1) ? TCS_BOTTOM : 0);

	mh_Tabbar = CreateWindow(WC_TABCONTROL, NULL, nPlacement | WS_CLIPSIBLINGS | TCS_FOCUSNEVER, 0, 0,
	                         rcClient.right, 0, mh_Rebar, NULL, g_hInstance, NULL);

	if (mh_Tabbar == NULL)
	{
		return NULL;
	}


	//#if !defined(__GNUC__)
	//#pragma warning (disable : 4312)
	//#endif

	// Надо
	TabPanelWinMap map = {this}; //{ CTabPanelWin* object; HWND hWnd; WNDPROC defaultProc; };
	map.defaultProc = (WNDPROC)SetWindowLongPtr(mh_Tabbar, GWLP_WNDPROC, (LONG_PTR)_TabProc);
	map.hWnd = mh_Tabbar;
	gp_TabPanelWinMap->Set(mh_Tabbar, map);


	SendMessage(mh_Tabbar, TCM_SETIMAGELIST, 0, (LPARAM)mp_Owner->GetTabIcons());

	if (!mh_TabTip || !IsWindow(mh_TabTip))
		InitTooltips(mh_Tabbar);
	TabCtrl_SetToolTips(mh_Tabbar, mh_TabTip);

	UpdateTabFontInt();
	//HFONT hFont = CreateFont(gpSet->nTabFontHeight, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, gpSet->nTabFontCharSet, OUT_DEFAULT_PRECIS,
	//                         CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, gpSet->sTabFontFace);
	//SendMessage(mh_Tabbar, WM_SETFONT, WPARAM(hFont), TRUE);

	if (!mh_Balloon || !IsWindow(mh_Balloon))
	{
		InitTooltips(mh_Tabbar);
	}

	// Добавляет закладку, или меняет (при необходимости) заголовок существующей
	//AddTab(gpConEmu->isFar() ? gpSet->szTabPanels : gpSet->pszTabConsole, 0);
	AddTabInt(gpConEmu->GetLastTitle(), 0, gpConEmu->mb_IsUacAdmin, -1);
	// нас интересует смещение клиентской области. Т.е. начало - из 0. Остальное не важно
	rcClient = MakeRect(600, 400);
	//rcClient = gpConEmu->GetGuiClientRect();
	TabCtrl_AdjustRect(mh_Tabbar, FALSE, &rcClient);
	mn_TabHeight = rcClient.top - mn_ThemeHeightDiff;
	return mh_Tabbar;
}

HWND CTabPanelWin::CreateToolbar()
{
	gpSetCls->CheckTheming();

	if (!mh_Rebar || !gpSet->isMultiShowButtons)
		return NULL; // нет табов - нет и тулбара

	if (mh_Toolbar)
		return mh_Toolbar; // Уже создали

	mh_Toolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
	                            WS_CHILD|WS_VISIBLE|
	                            TBSTYLE_FLAT|CCS_NOPARENTALIGN|CCS_NORESIZE|CCS_NODIVIDER|
	                            TBSTYLE_TOOLTIPS|TBSTYLE_TRANSPARENT,
	                            0, 0, 0, 0, mh_Rebar,
	                            NULL, NULL, NULL);
	
	TabPanelWinMap map = {this}; //{ CTabPanelWin* object; HWND hWnd; WNDPROC defaultProc; };
	map.defaultProc = (WNDPROC)SetWindowLongPtr(mh_Toolbar, GWLP_WNDPROC, (LONG_PTR)_ToolProc);
	map.hWnd = mh_Toolbar;
	gp_TabPanelWinMap->Set(mh_Toolbar, map);

	DWORD lExStyle = ((DWORD)SendMessage(mh_Toolbar, TB_GETEXTENDEDSTYLE, 0, 0)) | TBSTYLE_EX_DRAWDDARROWS;
	SendMessage(mh_Toolbar, TB_SETEXTENDEDSTYLE, 0, lExStyle);
	SendMessage(mh_Toolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
	SendMessage(mh_Toolbar, TB_SETBITMAPSIZE, 0, MAKELONG(14,14));

	TBADDBITMAP bmp = {g_hInstance,IDB_MAIN_TOOLBAR};
	int nFirst = SendMessage(mh_Toolbar, TB_ADDBITMAP, BID_TOOLBAR_LAST_IDX, (LPARAM)&bmp);
	_ASSERTE(BID_TOOLBAR_LAST_IDX==37);
	
	//DWORD nLoadErr = 0;
	if (gnOsVer >= 0x600)
	{
		bmp.hInst = g_hInstance;
		bmp.nID = IDB_COPY24;
	}
	else
	{
		bmp.hInst = NULL;
		COLORMAP colorMap = {RGB(255,0,0),GetSysColor(COLOR_BTNFACE)};
		bmp.nID = (UINT_PTR)CreateMappedBitmap(g_hInstance, IDB_COPY4, 0, &colorMap, 1);
		//bmp.nID = (UINT_PTR)LoadImage(g_hInstance, MAKEINTRESOURCE(IDB_COPY24), IMAGE_BITMAP, 0,0, LR_LOADTRANSPARENT|LR_LOADMAP3DCOLORS);
		//nLoadErr = GetLastError();
	}

	int nCopyBmp = SendMessage(mh_Toolbar, TB_ADDBITMAP, 1, (LPARAM)&bmp);
	// Должен 37 возвращать
	_ASSERTE(nCopyBmp == BID_TOOLBAR_LAST_IDX);
	if (nCopyBmp < BID_TOOLBAR_LAST_IDX)
		nCopyBmp = BID_TOOLBAR_LAST_IDX;

	{
		bmp.hInst = NULL;
		COLORMAP colorMap = {0xC0C0C0,GetSysColor(COLOR_BTNFACE)};
		bmp.nID = (UINT_PTR)CreateMappedBitmap(g_hInstance, IDB_SCROLL, 0, &colorMap, 1);
	}
	int nScrollBmp = SendMessage(mh_Toolbar, TB_ADDBITMAP, 1, (LPARAM)&bmp);
	// Должен 38 возвращать
	_ASSERTE(nScrollBmp == (BID_TOOLBAR_LAST_IDX+1));
	if (nScrollBmp < (BID_TOOLBAR_LAST_IDX+1))
		nScrollBmp = BID_TOOLBAR_LAST_IDX+1;
		

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
	//SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&sep); sep.idCommand++;
#if 0 //defined(_DEBUG)
	// Show copying state
	btn.iBitmap = nCopyBmp; btn.idCommand = TID_COPYING;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&sep); sep.idCommand++;
#endif
	// Buffer height mode
	btn.iBitmap = nFirst + BID_ALTERNATIVE_IDX; btn.idCommand = TID_ALTERNATIVE; btn.fsState = TBSTATE_ENABLED;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);
	// Show copying state
	btn.iBitmap = nScrollBmp; btn.idCommand = TID_SCROLL;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);
	// Separator before min/max/close
	sep.fsState |= TBSTATE_HIDDEN; sep.idCommand = TID_MINIMIZE_SEP;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&sep);
	// Min,Max,Close
	btn.iBitmap = nFirst + BID_MINIMIZE_IDX; btn.idCommand = TID_MINIMIZE; btn.fsState = TBSTATE_ENABLED|TBSTATE_HIDDEN;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);
	btn.iBitmap = nFirst + ((gpConEmu->GetWindowMode() != wmNormal) ? BID_MAXIMIZE_IDX : BID_RESTORE_IDX); btn.idCommand = TID_MAXIMIZE;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);
	btn.iBitmap = nFirst + BID_APPCLOSE_IDX; btn.idCommand = TID_APPCLOSE;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);
	SendMessage(mh_Toolbar, TB_AUTOSIZE, 0, 0);
#ifdef _DEBUG
	SIZE sz;
	SendMessage(mh_Toolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz);
#endif
	return mh_Toolbar;
}

// Прямоугольник в клиентских координатах ghWnd!
bool CTabPanelWin::GetRebarClientRect(RECT* rc)
{
	if (!mp_Owner->IsTabsShown())
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
	if (!IsTabbarCreated() || !mp_Owner->IsTabsShown())
	{
		_ASSERTE(IsTabbarCreated());
		_ASSERTE(mp_Owner->IsTabsShown());
		return false;
	}

	if (nTabIdx < 0)
		nTabIdx = TabCtrl_GetCurSel(mh_Tabbar);

	TabCtrl_GetItemRect(mh_Tabbar, nTabIdx, rcTab);
	MapWindowPoints(mh_Tabbar, NULL, (LPPOINT)rcTab, 2);
	RECT rcBar = {}; GetWindowRect(mh_Rebar, &rcBar);
	rcTab->bottom = min(rcTab->bottom, rcBar.bottom);
	rcTab->left = max(rcTab->left-2, rcBar.left);

	return true;
}

bool CTabPanelWin::GetTabText(int nTabIdx, wchar_t* pszText, int cchTextMax)
{
	bool bRc = false;
	TCITEM item = {TCIF_TEXT};
	item.pszText = pszText;
	item.cchTextMax = cchTextMax;

	if (TabCtrl_GetItem(mh_Tabbar, nTabIdx, &item))
		bRc = true;
	else
		pszText[0] = 0;

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

		if (PtInRect(&rcWnd, tch.pt))
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

bool CTabPanelWin::GetToolBtnRect(int nCmd, RECT* rcBtnRect)
{
	if (!mp_Owner->IsTabsShown())
	{
		return false;
	}
	if (!mh_Toolbar)
	{
		_ASSERTE(mh_Toolbar);
		return false;
	}
	
	SendMessage(mh_Toolbar, TB_GETRECT, nCmd/*например TID_CREATE_CON*/, (LPARAM)rcBtnRect);
	MapWindowPoints(mh_Toolbar, ghWnd, (LPPOINT)rcBtnRect, 2);

	return true;
}

// Добавляет закладку, или меняет (при необходимости) заголовок существующей
void CTabPanelWin::AddTabInt(LPCWSTR text, int i, bool bAdmin, int iTabIcon)
{
	if (!IsTabbarCreated())
		return;

	int iIconIdx = mp_Owner->GetTabIcon(bAdmin);

	TCITEM tie;
	// иконку обновляем всегда. она может измениться для таба
	tie.mask = TCIF_TEXT | (mp_Owner->GetTabIcons() ? TCIF_IMAGE : 0);
	tie.pszText = (LPWSTR)text ;
	tie.iImage = (iTabIcon > 0) ? iTabIcon : mp_Owner->GetTabIcon(bAdmin); // Пока иконка только одна - для табов со щитом
	int nCurCount = GetItemCountInt();

	if (i>=nCurCount)
	{
		TabCtrl_InsertItem(mh_Tabbar, i, &tie);
	}
	else
	{
		// Проверим, изменился ли текст
		wchar_t sOldTabName[CONEMUTABMAX];
		if (!GetTabText(i, sOldTabName, countof(sOldTabName)) || !wcscmp(sOldTabName, text))
			tie.mask &= ~TCIF_TEXT;

		// Изменилась ли иконка
		if (tie.mask & TCIF_IMAGE)
		{
			TCITEM told;
			told.mask = TCIF_IMAGE;
			TabCtrl_GetItem(mh_Tabbar, i, &told);

			if (told.iImage == tie.iImage)
				tie.mask &= ~TCIF_IMAGE;
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
	RECT client = {};
	if (bShow)
		client = gpConEmu->GetGuiClientRect(); // нас интересует ширина окна
	int nShow = bShow ? SW_SHOW : SW_HIDE;

	if (mh_Rebar)
	{
		if (!IsWindowVisible(mh_Rebar))
			apiShowWindow(mh_Rebar, nShow);

		//MoveWindow(mh_Rebar, 0, 0, client.right, mn_TabHeight, 1);
	}
	else
	{
		if (!IsWindowVisible(mh_Tabbar))
			apiShowWindow(mh_Tabbar, nShow);

		if (bShow)
		{
			//if (gpSet->isTabFrame)
			//	MoveWindow(mh_Tabbar, 0, 0, client.right, client.bottom, 1);
			//else
			MoveWindow(mh_Tabbar, 0, 0, client.right, mn_TabHeight, 1);
		}
	}
}

void CTabPanelWin::RepositionInt()
{
	RECT client, self;
	client = gpConEmu->GetGuiClientRect();
	GetWindowRect(mh_Tabbar, &self);

	if (mh_Rebar)
	{
		SIZE sz = {0,0};
		int nBarIndex = -1;
		BOOL lbNeedShow = FALSE, lbWideEnough = FALSE;

		if (mh_Toolbar)
		{
			nBarIndex = SendMessage(mh_Rebar, RB_IDTOINDEX, 2, 0);

			if (gpSet->isMultiShowButtons)
			{
				SendMessage(mh_Toolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz);
				sz.cx += (gpSet->isHideCaptionAlways() ? gpSet->nToolbarAddSpace : 0);
				lbWideEnough = (sz.cx + 150) <= client.right;

				if (!lbWideEnough)
				{
					if (IsWindowVisible(mh_Toolbar))
						SendMessage(mh_Rebar, RB_SHOWBAND, nBarIndex, 0);
				}
				else
				{
					if (!IsWindowVisible(mh_Toolbar))
						lbNeedShow = TRUE;
				}
			}
			else
			{
				SendMessage(mh_Rebar, RB_SHOWBAND, nBarIndex, 0);
			}
		}

		if (gpSet->nTabsLocation == 1)
		{
			int nStatusHeight = gpSet->isStatusBarShow ? gpSet->StatusBarHeight() : 0;
			MoveWindow(mh_Rebar, 0, client.bottom-nStatusHeight-mn_TabHeight, client.right, mn_TabHeight, 1);
		}
		else
		{
			MoveWindow(mh_Rebar, 0, 0, client.right, mn_TabHeight, 1);
		}

		// Не будем пока трогать. Некрасиво табы рисуются. Нужно на OwnerDraw переходить.
		#if 0
		DWORD nCurStyle = GetWindowLong(mh_Tabbar, GWL_STYLE);
		DWORD nNeedStyle = (gpSet->nTabsLocation == 1) ? (nCurStyle | TCS_BOTTOM) : (nCurStyle & ~TCS_BOTTOM);
		if (nNeedStyle != nCurStyle)
		{
			SetWindowLong(mh_Tabbar, GWL_STYLE, nNeedStyle);

			//_ASSERTE(!gpSet->isTabFrame);
			if (gpSet->nTabsLocation == 1)
			{
				RECT rcTab = client;
				GetWindowRect(mh_Tabbar, &rcTab);
				SetWindowPos(mh_Tabbar, NULL, 0, client.bottom - (rcTab.bottom-rcTab.top), 0,0, SWP_NOSIZE);
			}
			else
			{
				SetWindowPos(mh_Tabbar, NULL, 0,0, 0,0, SWP_NOSIZE);
			}
		}
		#endif

		//if (gpSet->nTabsLocation == 1)
		//	m_Margins = MakeRect(0,0,0,mn_TabHeight);
		//else
		//	m_Margins = MakeRect(0,mn_TabHeight,0,0);
		//gpSet->UpdateMargins(m_Margins);


		if (lbWideEnough && nBarIndex != 1)
		{
			SendMessage(mh_Rebar, RB_MOVEBAND, nBarIndex, 1);
			nBarIndex = SendMessage(mh_Rebar, RB_IDTOINDEX, 2, 0);
			_ASSERTE(nBarIndex == 1);
		}

		if (lbNeedShow)
		{
			SendMessage(mh_Rebar, RB_SHOWBAND, nBarIndex, 1);
		}
	}
	//else if (gpSet->isTabFrame)
	//{
	//	MoveWindow(mh_Tabbar, 0, 0, client.right, client.bottom, 1);
	//}
	else
	{
		MoveWindow(mh_Tabbar, 0, 0, client.right, mn_TabHeight, 1);
	}

	UpdateToolbarPos();
}

void CTabPanelWin::UpdateToolbarPos()
{
	if (mh_Toolbar && gpSet->isMultiShowButtons)
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
				REBARBANDINFO rbBand= {80}; // не используем size, т.к. приходит "новый" размер из висты и в XP обламываемся
				rbBand.fMask  = RBBIM_SIZE | RBBIM_CHILDSIZE;
				// Set values unique to the band with the toolbar.
				rbBand.cx = rbBand.cxMinChild = rbBand.cxIdeal = mn_LastToolbarWidth = sz.cx;
				rbBand.cyMinChild = sz.cy;
				// Add the band that has the toolbar.
				SendMessage(mh_Rebar, RB_SETBANDINFO, 1, (LPARAM)&rbBand);
			}
		}
		else
		{
			RECT rcClient;
			GetWindowRect(mh_Tabbar, &rcClient);
			MapWindowPoints(NULL, ghWnd, (LPPOINT)&rcClient, 2);
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
		int lnNewTab = GetCurSelInt();

		if (mn_prevTab>=0)
		{
			SelectTabInt(mn_prevTab);
			mn_prevTab = -1;
		}

		if (!mb_ChangeAllowed)
		{
			FarSendChangeTab(lnNewTab);
			// start waiting for title to change
			//_titleShouldChange = true;
		}

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

	bool lbHideBtn = !bCaptionHidden;

	OnWindowStateChanged(gpConEmu->GetWindowMode());
	SendMessage(mh_Toolbar, TB_HIDEBUTTON, TID_MINIMIZE_SEP, lbHideBtn);
	SendMessage(mh_Toolbar, TB_HIDEBUTTON, TID_MINIMIZE, lbHideBtn);
	SendMessage(mh_Toolbar, TB_HIDEBUTTON, TID_MAXIMIZE, lbHideBtn);
	SendMessage(mh_Toolbar, TB_HIDEBUTTON, TID_APPCLOSE, lbHideBtn);
	SendMessage(mh_Toolbar, TB_AUTOSIZE, 0, 0);

	//if (abUpdatePos)
	{
		//-- было 			//UpdateToolbarPos();
		RepositionInt();
	}
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

		//if (!bRedraw)
		//{
		//	bRedraw = true;
		//	SendMessage(mh_Toolbar, WM_SETREDRAW, FALSE, 0);
		//}

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

	//UpdateToolConsoles(true);
	////nConNumber = gpConEmu->ActiveConNum()+1; -- сюда пришел уже правильный номер!
	//if (nConNumber>=1 && nConNumber<=MAX_CONSOLE_COUNT)
	//{
	//	SendMessage(mh_Toolbar, TB_CHECKBUTTON, nConNumber, 1);
	//}
	//else
	//{
	//	for (int i = 1; i <= MAX_CONSOLE_COUNT; i++)
	//		SendMessage(mh_Toolbar, TB_CHECKBUTTON, i, 0);
	//}
}

void CTabPanelWin::ShowToolbar(bool bShow)
{
	if (bShow)
	{
		if (!IsToolbarCreated() && (CreateToolbar() != NULL))
		{
			REBARBANDINFO rbBand = {80}; // не используем size, т.к. приходит "новый" размер из висты и в XP обламываемся

			rbBand.fMask  = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | RBBIM_STYLE | RBBIM_COLORS;
			rbBand.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDSIZE | RBBS_VARIABLEHEIGHT;
			rbBand.clrBack = GetSysColor(COLOR_BTNFACE);
			rbBand.clrFore = GetSysColor(COLOR_BTNTEXT);

			SIZE sz = {0,0};
			SendMessage(mh_Toolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz);

			// Set values unique to the band with the toolbar.
			rbBand.wID        = 2;
			rbBand.hwndChild  = mh_Toolbar;
			rbBand.cx = rbBand.cxMinChild = rbBand.cxIdeal = mn_LastToolbarWidth = sz.cx;
			rbBand.cyChild = rbBand.cyMinChild = rbBand.cyMaxChild = sz.cy + mn_ThemeHeightDiff;

			// Add the band that has the toolbar.
			if (!SendMessage(mh_Rebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand))
			{
				DisplayLastError(_T("Can't initialize rebar (toolbar)"));
			}
		}
	}
	else
	{
		_ASSERTEX(bShow);
	}
}

int CTabPanelWin::QueryTabbarHeight()
{
	if (!this) return 0;

	// Нужно пересчитать высоту таба

	//bool bDummyCreate = (hTabs == NULL);
	//
	//if (bDummyCreate)
	//{
	//	hTabs = CreateTabbar(true);
	//}

	if (mh_Tabbar)
	{
		// нас интересует смещение клиентской области. Т.е. начало - из 0. Остальное не важно
		RECT rcClient = MakeRect(600, 400);
		//rcClient = gpConEmu->GetGuiClientRect();
		TabCtrl_AdjustRect(mh_Tabbar, FALSE, &rcClient);
		mn_TabHeight = rcClient.top - mn_ThemeHeightDiff;
	}
	else
	{
		// Не будем создавать TabBar. Все равно вне окно ConEmu оценка получается неточной
		//_ASSERTE((hTabs!=NULL) && "Creating of a dummy tab control failed");
		mn_TabHeight = gpSet->nTabFontHeight + 9;
	}

	//if (bDummyCreate && hTabs)
	//{
	//	DestroyWindow(hTabs);
	//	mh_Tabbar = NULL;
	//}

	return mn_TabHeight;
}

void CTabPanelWin::InvalidateBar()
{
	if (this && mh_Rebar)
	{
		InvalidateRect(mh_Rebar, NULL, TRUE);
	}
}

void CTabPanelWin::RePaintInt()
{
	if (!mh_Rebar)
		return;

	RECT client, self;
	client = gpConEmu->GetGuiClientRect();
	GetWindowRect(mh_Rebar, &self);
	MapWindowPoints(NULL, ghWnd, (LPPOINT)&self, 2);

	int nNewY;
	if (gpSet->nTabsLocation == 1)
	{
		int nStatusHeight = gpSet->isStatusBarShow ? gpSet->StatusBarHeight() : 0;
		nNewY = client.bottom-nStatusHeight-(self.bottom-self.top);
	}
	else
	{
		nNewY = 0;
	}

	if ((client.right != (self.right - self.left))
		|| (nNewY != self.top))
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
	if (gpSet->isTabs && (abForce || gpSet->isCaptionHidden()))
	{
		if (mp_Owner->IsTabsShown())
		{
			POINT ptCur = {}; GetCursorPos(&ptCur);
			int nTabIdx = GetTabFromPoint(ptCur);
			// -2 - out of control, -1 - out of tab-labels, 0+ - tab index 0-based
			if (nTabIdx == -1)
			{
				return HTCAPTION;
			}
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
