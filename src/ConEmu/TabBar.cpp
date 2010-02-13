
/*
Copyright (c) 2009-2010 Maximus5
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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


#define SHOWDEBUGSTR

#define DEBUGSTRTABS(s) //DEBUGSTR(s)

#include <windows.h>
#include <commctrl.h>
#include "header.h"

WARNING("!!! Запустили far, открыли edit, перешли в панель, открыли второй edit, ESC, ни одна вкладка не активна");
// Более того, если есть еще одна консоль - активной станет первая вкладка следующей НЕАКТИВНОЙ консоли
WARNING("не меняются табы при переключении на другую консоль");

TODO("Для WinXP можно поиграться стилем WS_EX_COMPOSITED");

//TabBarClass TabBar;
//const int TAB_FONT_HEIGTH = 16;
//wchar_t TAB_FONT_FACE[] = L"Tahoma";
WNDPROC TabBarClass::_defaultTabProc = NULL;
WNDPROC TabBarClass::_defaultToolProc = NULL;
WNDPROC TabBarClass::_defaultReBarProc = NULL;
typedef BOOL (WINAPI* FAppThemed)();

#ifndef TBN_GETINFOTIP
#define TBN_GETINFOTIP TBN_GETINFOTIPW
#endif
#ifndef RB_SETWINDOWTHEME
#define CCM_SETWINDOWTHEME      (CCM_FIRST + 0xb)
#define RB_SETWINDOWTHEME       CCM_SETWINDOWTHEME
#endif

#define TID_CREATE_CON   13
#define TID_BUFFERHEIGHT 14
#define TID_MINIMIZE 15
#define TID_MAXIMIZE 16
#define TID_APPCLOSE 17
#define TID_MINIMIZE_SEP 110

#define BID_NEWCON_IDX 12
#define BID_BUFHEIGHT_IDX 13
#define BID_MINIMIZE_IDX 14
#define BID_MAXIMIZE_IDX 15
#define BID_RESTORE_IDX 16
#define BID_APPCLOSE_IDX 17

#define POST_UPDATE_TIMEOUT 2000

//typedef long (WINAPI* ThemeFunction_t)();

TabBarClass::TabBarClass()
{
    _active = false;
    _tabHeight = 0;
	mb_DisableRedraw = FALSE;
	//memset(&m_Margins, 0, sizeof(m_Margins));
	m_Margins = gSet.rcTabMargins; // !! Значения отступов
    _titleShouldChange = false;
    _prevTab = -1;
    mb_ChangeAllowed = FALSE;
    //mb_Enabled = TRUE;
    mh_Toolbar = NULL; mh_Tabbar = NULL; mh_Rebar = NULL; mh_TabIcons = NULL; mn_LastToolbarWidth = 0;
	mn_AdminIcon = 0;
    mb_PostUpdateCalled = FALSE;
	mb_PostUpdateRequested = FALSE;
	mn_PostUpdateTick = 0;
    mn_MsgUpdateTabs = RegisterWindowMessage(CONEMUMSG_UPDATETABS);
    memset(&m_Tab4Tip, 0, sizeof(m_Tab4Tip));
    mb_InKeySwitching = FALSE;
    ms_TmpTabText[0] = 0;
	mn_CurSelTab = 0;
	mn_ThemeHeightDiff = 0;
	mn_InUpdate = 0;
	//mb_ThemingEnabled = FALSE;
	mh_TabTip = NULL;
}

//void TabBarClass::CheckTheming()
//{
//	static bool bChecked = false;
//	if (bChecked) return;
//	bChecked = true;
//
//	if (gOSVer.dwMajorVersion >= 6 || (gOSVer.dwMajorVersion == 5 && gOSVer.dwMinorVersion >= 1)) {
//		ThemeFunction_t fIsAppThemed = NULL;
//		ThemeFunction_t fIsThemeActive = NULL;
//		HMODULE hUxTheme = GetModuleHandle ( L"UxTheme.dll" );
//		if (hUxTheme)
//		{
//			fIsAppThemed = (ThemeFunction_t)GetProcAddress(hUxTheme, "IsAppThemed");
//			fIsThemeActive = (ThemeFunction_t)GetProcAddress(hUxTheme, "IsThemeActive");
//			if (fIsAppThemed && fIsThemeActive)
//			{
//				long llThemed = fIsAppThemed();
//				long llActive = fIsThemeActive();
//				if (llThemed && llActive)
//					mb_ThemingEnabled = TRUE;
//			}
//		}
//	}
//}

//void TabBarClass::Enable(BOOL abEnabled)
//{
//    if (mh_Tabbar && mb_Enabled!=abEnabled)
//    {
//        //EnableWindow(mh_Tabbar, abEnabled);
//        mb_Enabled = abEnabled;
//    }
//}

void TabBarClass::RePaint()
{
    if (!mh_Rebar)
        return;

    RECT client, self;
    GetClientRect(ghWnd, &client);
    GetWindowRect(mh_Rebar, &self);
    if (client.right != (self.right - self.left)) {
        MoveWindow(mh_Rebar, 0, 0, client.right, self.bottom-self.top, 1);
    }
    
    UpdateWindow(mh_Rebar);
}

//void TabBarClass::Refresh(BOOL abFarActive)
//{
//    Enable(abFarActive);
//}

void TabBarClass::Reset()
{
    if (!_active)
    {
        return;
    }

    /*ConEmuTab tab; memset(&tab, 0, sizeof(tab));
    tab.Pos=0;
    tab.Current=1;
    tab.Type = 1;*/
    //gConEmu.mp_TabBar->Update(&tab, 1);
    Update();
}

void TabBarClass::Retrieve()
{
    if (gSet.isTabs == 0)
        return; // если табов нет ВООБЩЕ - и читать ничего не нужно

    if (!gConEmu.isFar()) {
        Reset();
        return;
    }

    TODO("Retrieve() может нужно выполнить в RCon?");

    //CConEmuPipe pipe;
    //if (pipe.Init(_T("TabBarClass::Retrieve"), TRUE))
    //{
    //  DWORD cbWritten=0;
    //  if (pipe.Execute(CMD_REQTABS))
    //  {
    //      gConEmu.DebugStep(_T("Tabs: Checking for plugin (1 sec)"));
    //      // Подождем немножко, проверим что плагин живой
    //      cbWritten = WaitForSingleObject(pipe.hEventAlive, CONEMUALIVETIMEOUT);
    //      if (cbWritten!=WAIT_OBJECT_0) {
    //          TCHAR szErr[MAX_PATH];
    //          wsprintf(szErr, _T("ConEmu plugin is not active!\r\nProcessID=%i"), pipe.nPID);
    //          MBoxA(szErr);
    //      } else {
    //          gConEmu.DebugStep(_T("Tabs: Waiting for result (10 sec)"));
    //          cbWritten = WaitForSingleObject(pipe.hEventReady, CONEMUREADYTIMEOUT);
    //          if (cbWritten!=WAIT_OBJECT_0) {
    //              TCHAR szErr[MAX_PATH];
    //              wsprintf(szErr, _T("Command waiting time exceeds!\r\nConEmu plugin is locked?\r\nProcessID=%i"), pipe.nPID);
    //              MBoxA(szErr);
    //          } else {
    //              gConEmu.DebugStep(_T("Tabs: Recieving data"));
    //              DWORD cbBytesRead=0;
    //              int nTabCount=0;
    //              pipe.Read(&nTabCount, sizeof(nTabCount), &cbBytesRead);

    //              if (nTabCount<=0) {
    //                  gConEmu.DebugStep(_T("Tabs: data empty"));
    //                  this->Reset();
    //              } else {
    //                  COPYDATASTRUCT cds = {0};
    //                  
    //                  cds.dwData = nTabCount;
    //                  cds.lpData = pipe.GetPtr(); // хвост

    //                  gConEmu.OnCopyData(&cds);
    //                  gConEmu.DebugStep(NULL);
    //              }
    //          }
    //      }
    //  }
    //}
}

// Добавляет закладку, или меняет (при необходимости) заголовок существующей
void TabBarClass::AddTab(LPCWSTR text, int i, bool bAdmin)
{
	if (mh_Tabbar) {
		TCITEM tie;
		// иконку обновляем всегда. она может измениться для таба
		tie.mask = TCIF_TEXT | (mh_TabIcons ? TCIF_IMAGE : 0);
		tie.iImage = -1; 
		tie.pszText = (LPWSTR)text ;
		tie.iImage = bAdmin ? mn_AdminIcon : -1; // Пока иконка только одна - для табов со щитом

		int nCurCount = GetItemCount();
		if (i>=nCurCount) {
			TabCtrl_InsertItem(mh_Tabbar, i, &tie);
		} else {
			// Проверим, изменился ли текст
			if (!wcscmp(GetTabText(i), text))
				tie.mask &= ~TCIF_TEXT;
			// Изменилась ли иконка
			if (tie.mask & TCIF_IMAGE) {
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
}

void TabBarClass::SelectTab(int i)
{
    mb_ChangeAllowed = TRUE;
	if (i != GetCurSel()) { // Меняем выделение, только если оно реально меняется
		mn_CurSelTab = i;
		if (mh_Tabbar)
			TabCtrl_SetCurSel(mh_Tabbar, i);
	}
    mb_ChangeAllowed = FALSE;
}

int TabBarClass::GetCurSel()
{
	if (mh_Tabbar) {
		// Если табы реально есть - обновим
		mn_CurSelTab = TabCtrl_GetCurSel(mh_Tabbar);
	}
	int nCurSel = mn_CurSelTab;
	return nCurSel;
}

int TabBarClass::GetItemCount()
{
	int nCurCount = 0;
	if (mh_Tabbar)
		nCurCount = TabCtrl_GetItemCount(mh_Tabbar);
	else
		nCurCount = m_Tab2VCon.size();
	return nCurCount;
}

void TabBarClass::DeleteItem(int I)
{
	if (!mh_Tabbar) return;
	TabCtrl_DeleteItem(mh_Tabbar, I);
}


/*char TabBarClass::FarTabShortcut(int tabIndex)
{
    return tabIndex < 10 ? '0' + tabIndex : 'A' + tabIndex - 10;
}*/

BOOL TabBarClass::NeedPostUpdate()
{
	return (mb_PostUpdateCalled || mb_PostUpdateRequested);
}

void TabBarClass::RequestPostUpdate()
{
	if (mb_PostUpdateCalled) {
		DWORD nDelta = GetTickCount() - mn_PostUpdateTick;
		// Может так получиться, что флажок остался, а Post не вызвался
		if (nDelta <= POST_UPDATE_TIMEOUT)
			return; // Уже
	}

	if (mn_InUpdate > 0) {
		mb_PostUpdateRequested = TRUE;
		DEBUGSTRTABS(L"   PostRequesting TabBarClass::Update\n");
	} else {
		mb_PostUpdateCalled = TRUE;
		DEBUGSTRTABS(L"   Posting TabBarClass::Update\n");
		PostMessage(ghWnd, mn_MsgUpdateTabs, 0, 0);
		mn_PostUpdateTick = GetTickCount();
	}
}

BOOL TabBarClass::GetVConFromTab(int nTabIdx, CVirtualConsole** rpVCon, DWORD* rpWndIndex)
{
    BOOL lbRc = FALSE;
    CVirtualConsole *pVCon = NULL;
    DWORD wndIndex = 0;

    if (nTabIdx >= 0 && (UINT)nTabIdx < m_Tab2VCon.size()) {
        pVCon = m_Tab2VCon[nTabIdx].pVCon;
        wndIndex = m_Tab2VCon[nTabIdx].nFarWindowId;

        if (!gConEmu.isValid(pVCon)) {
			RequestPostUpdate();
            //if (!mb_PostUpdateCalled)
            //{
            //    mb_PostUpdateCalled = TRUE;
            //    PostMessage(ghWnd, mn_Msg UpdateTabs, 0, 0);
            //}
        } else {
            lbRc = TRUE;
        }
    }

    if (rpVCon) *rpVCon = lbRc ? pVCon : NULL;
    if (rpWndIndex) *rpWndIndex = lbRc ? wndIndex : 0;

    return lbRc;
}

CVirtualConsole* TabBarClass::FarSendChangeTab(int tabIndex)
{
    CVirtualConsole *pVCon = NULL;
    DWORD wndIndex = 0;
    BOOL  bNeedActivate = FALSE, bChangeOk = FALSE;

    if (!GetVConFromTab(tabIndex, &pVCon, &wndIndex)) {
        if (mb_InKeySwitching) Update(); // показать реальное положение дел
        return NULL;
    }
    
    if (!gConEmu.isActive(pVCon))
        bNeedActivate = TRUE;
        

    bChangeOk = pVCon->RCon()->ActivateFarWindow(wndIndex);
    
    // Чтобы лишнее не мелькало - активируем консоль 
    // ТОЛЬКО после смены таба (успешной или неудачной - неважно)
    if (bNeedActivate) {
        if (!gConEmu.Activate(pVCon)) {
            if (mb_InKeySwitching) Update(); // показать реальное положение дел
            
            TODO("А текущий таб не слетит, если активировать не удалось?");
            return NULL;
        }
    }
    
    if (!bChangeOk) {
        pVCon = NULL;
        if (mb_InKeySwitching) Update(); // показать реальное положение дел
    }

    return pVCon;
}

LRESULT TabBarClass::TabHitTest()
{
	if ((gSet.isHideCaptionAlways || gSet.isFullScreen || (gConEmu.isZoomed() && gSet.isHideCaption))
		&& gSet.isTabs)
	{
		if (gConEmu.mp_TabBar->IsShown()) {
			TCHITTESTINFO tch = {{0,0}};
			HWND hTabBar = gConEmu.mp_TabBar->mh_Tabbar;
			RECT rcWnd; GetWindowRect(hTabBar, &rcWnd);
			GetCursorPos(&tch.pt); // Screen coordinates
			if (PtInRect(&rcWnd, tch.pt)) {
				tch.pt.x -= rcWnd.left; tch.pt.y -= rcWnd.top;
				LRESULT nTest = SendMessage(hTabBar, TCM_HITTEST, 0, (LPARAM)&tch);
				if (nTest == -1)
					return HTCAPTION;
			}
		}
	}
	return HTNOWHERE;
}

LRESULT CALLBACK TabBarClass::ReBarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_WINDOWPOSCHANGING:
		{
			if (gConEmu.mp_TabBar->_tabHeight) {
				LPWINDOWPOS pos = (LPWINDOWPOS)lParam;
				pos->cy = gConEmu.mp_TabBar->_tabHeight;
				return 0;
			}
		}
	case WM_SETFOCUS:
		{
			SetFocus(ghWnd);
			return 0;
		}
	case WM_SETCURSOR:
		if (gSet.isHideCaptionAlways && gSet.isTabs && !gSet.isFullScreen && !gConEmu.isZoomed()) {
			if (TabHitTest()==HTCAPTION) {
				SetCursor(gConEmu.mh_CursorMove);
				return TRUE;
			}
		}
		break;

	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
	/*case WM_RBUTTONDOWN:*/ case WM_RBUTTONUP: //case WM_RBUTTONDBLCLK:
		if ((gSet.isHideCaptionAlways || gSet.isFullScreen || (gConEmu.isZoomed() && gSet.isHideCaption))
			&& gSet.isTabs)
		{
			if (TabHitTest()==HTCAPTION) {
				POINT ptScr; GetCursorPos(&ptScr);
				lParam = MAKELONG(ptScr.x,ptScr.y);
				LRESULT lRc = 0;
				if (uMsg == WM_LBUTTONDBLCLK) {
					// Чтобы клик случайно не провалился в консоль
					gConEmu.mouse.state |= MOUSE_SIZING_DBLCKL;
					// Аналог AltF9
					//gConEmu.SetWindowMode((gConEmu.isZoomed()||(gSet.isFullScreen&&gConEmu.isWndNotFSMaximized)) ? rNormal : rMaximized);
					gConEmu.OnAltF9(TRUE);
				} else if (uMsg == WM_RBUTTONUP) {
					gConEmu.ShowSysmenu(ghWnd, ptScr.x, ptScr.y/*-32000*/);
				} else if (!gSet.isFullScreen && !gConEmu.isZoomed()) {
					lRc = gConEmu.WndProc(ghWnd, uMsg-(WM_MOUSEMOVE-WM_NCMOUSEMOVE), HTCAPTION, lParam);
				}
				return lRc;
			}
		}
		break;
	}
	return CallWindowProc(_defaultReBarProc, hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK TabBarClass::TabProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_WINDOWPOSCHANGING:
        {
        	if (gConEmu.mp_TabBar->mh_Rebar) {
	        	LPWINDOWPOS pos = (LPWINDOWPOS)lParam;
				//if (gConEmu.mp_TabBar->mb_ThemingEnabled) {
				if (gSet.CheckTheming()) {
		            pos->y = 2; // иначе в Win7 он смещается в {0x0} и снизу видна некрасивая полоса
					pos->cy = gConEmu.mp_TabBar->_tabHeight -3; // на всякий случай
				} else {
					pos->y = 1;
					pos->cy = gConEmu.mp_TabBar->_tabHeight + 1;
				}
	            return 0;
            }
            break;
        }
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        {
            gConEmu.mp_TabBar->OnMouse(uMsg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }
    case WM_SETFOCUS:
        {
            SetFocus(ghWnd);
            return 0;
        }
    }
    return CallWindowProc(_defaultTabProc, hwnd, uMsg, wParam, lParam);
}

// Window procedure for Toolbar (Multiconsole & BufferHeight)
LRESULT CALLBACK TabBarClass::ToolProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_WINDOWPOSCHANGING:
        {
        	LPWINDOWPOS pos = (LPWINDOWPOS)lParam;
            pos->y = (gConEmu.mp_TabBar->mn_ThemeHeightDiff == 0) ? 2 : 1;
            if (gSet.isHideCaptionAlways && gSet.nToolbarAddSpace > 0) {
            	SIZE sz;
            	if (CallWindowProc(_defaultToolProc, hwnd, TB_GETIDEALSIZE, 0, (LPARAM)&sz)) {
            		pos->cx = sz.cx + gSet.nToolbarAddSpace;
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
			if (CallWindowProc(_defaultToolProc, hwnd, uMsg, wParam, lParam)) {
				psz->cx += (gSet.isHideCaptionAlways ? gSet.nToolbarAddSpace : 0);
				return 1;
			}
		} break;
    }
    return CallWindowProc(_defaultToolProc, hwnd, uMsg, wParam, lParam);
}


bool TabBarClass::IsActive()
{
    return _active;
}

bool TabBarClass::IsShown()
{
    return _active && IsWindowVisible(mh_Tabbar);
}

//BOOL TabBarClass::IsAllowed()
//{
//    BOOL lbTabsAllowed = TRUE;
//    TODO("пока убрал");
//    //if (gConEmu.BufferHeight) {
//        CVirtualConsole* pCon = gConEmu.ActiveCon();
//        if (!pCon) 
//            lbTabsAllowed = FALSE;
//        else
//            lbTabsAllowed = !pCon->RCon()->isBufferHeight();
//    //}
//    return lbTabsAllowed;
//}

void TabBarClass::Activate()
{
    if (!mh_Rebar) {
        CreateRebar();
    }

    _active = true;

	UpdatePosition();
}

void TabBarClass::Deactivate()
{
    if (!_active)
        return;

	_active = false;
    UpdatePosition();
}

void TabBarClass::Update(BOOL abPosted/*=FALSE*/)
{
	#ifdef _DEBUG
	if (this != gConEmu.mp_TabBar) {
		_ASSERTE(this == gConEmu.mp_TabBar);
	}
	#endif

	MCHKHEAP 
    /*if (!_active)
    {
        return;
    }*/ // Теперь - ВСЕГДА! т.к. сами управляем мультиконсолью

	if (mb_DisableRedraw)
		return;
    
    if (!gConEmu.isMainThread()) {
		RequestPostUpdate();
        return;
    }
    
    mb_PostUpdateCalled = FALSE;

	#ifdef _DEBUG
	_ASSERTE(mn_InUpdate >= 0);
	if (mn_InUpdate > 0) {
		_ASSERTE(mn_InUpdate == 0);
	}
	#endif
	mn_InUpdate ++;

    ConEmuTab tab = {0};
    
	MCHKHEAP
    int V, I, tabIdx = 0, nCurTab = -1;
	CVirtualConsole* pVCon = NULL;
    VConTabs vct = {NULL};

    // Выполняться должно только в основной нити, так что CriticalSection не нужна
    m_Tab2VCon.clear();
	_ASSERTE(m_Tab2VCon.size()==0);

	#ifdef _DEBUG
	if (this != gConEmu.mp_TabBar) {
		_ASSERTE(this == gConEmu.mp_TabBar);
	}
	#endif

	MCHKHEAP
	if (!gConEmu.mp_TabBar->IsActive() && gSet.isTabs) {
		int nTabs = 0;
		for (V = 0; V < MAX_CONSOLE_COUNT && nTabs < 2; V++) {
			_ASSERTE(m_Tab2VCon.size()==0);
			if (!(pVCon = gConEmu.GetVCon(V))) continue;
			_ASSERTE(m_Tab2VCon.size()==0);
			nTabs += pVCon->RCon()->GetTabCount();
			_ASSERTE(m_Tab2VCon.size()==0);
		}
		if (nTabs > 1) {
			_ASSERTE(m_Tab2VCon.size()==0);
			Activate();
			_ASSERTE(m_Tab2VCon.size()==0);
		}
	} else if (gConEmu.mp_TabBar->IsActive() && gSet.isTabs==2) {
		int nTabs = 0;
		for (V = 0; V < MAX_CONSOLE_COUNT && nTabs < 2; V++) {
			_ASSERTE(m_Tab2VCon.size()==0);
			if (!(pVCon = gConEmu.GetVCon(V))) continue;
			_ASSERTE(m_Tab2VCon.size()==0);
			nTabs += pVCon->RCon()->GetTabCount();
			_ASSERTE(m_Tab2VCon.size()==0);
		}
		if (nTabs <= 1) {
			_ASSERTE(m_Tab2VCon.size()==0);
			Deactivate();
			_ASSERTE(m_Tab2VCon.size()==0);
		}
	}

	#ifdef _DEBUG
	if (this != gConEmu.mp_TabBar) {
		_ASSERTE(this == gConEmu.mp_TabBar);
	}
	#endif

	MCHKHEAP
	_ASSERTE(m_Tab2VCon.size()==0);

    for (V = 0; V < MAX_CONSOLE_COUNT; V++) {
        if (!(pVCon = gConEmu.GetVCon(V))) continue;
		CRealConsole *pRCon = pVCon->RCon();
        
        BOOL lbActive = gConEmu.isActive(pVCon);
        
        for (I = 0; TRUE; I++) {
			#ifdef _DEBUG
			if (!I && !V) {
				_ASSERTE(m_Tab2VCon.size()==0);
			}

			if (this != gConEmu.mp_TabBar) {
				_ASSERTE(this == gConEmu.mp_TabBar);
			}
			MCHKHEAP;
			#endif

			if (!pRCon->GetTab(I, &tab))
				break;

			#ifdef _DEBUG
			if (this != gConEmu.mp_TabBar) {
				_ASSERTE(this == gConEmu.mp_TabBar);
			}
			MCHKHEAP;
			#endif

            PrepareTab(&tab);
            
            vct.pVCon = pVCon;
            vct.nFarWindowId = I;

			#ifdef _DEBUG
			if (!I && !V) {
				_ASSERTE(m_Tab2VCon.size()==0);
			}
			#endif

            AddTab2VCon(vct);
            // Добавляет закладку, или меняет (при необходимости) заголовок существующей
            AddTab(tab.Name, tabIdx, (tab.Type & 0x100)==0x100);
            
            if (lbActive && tab.Current)
                nCurTab = tabIdx;
            
            tabIdx++;

			#ifdef _DEBUG
			if (this != gConEmu.mp_TabBar) {
				_ASSERTE(this == gConEmu.mp_TabBar);
			}
			#endif
        }
    }
	MCHKHEAP
    if (tabIdx == 0) // хотя бы "Console" покажем
    {
        PrepareTab(&tab);
        
        vct.pVCon = NULL;
        vct.nFarWindowId = 0;
		AddTab2VCon(vct); //2009-06-14. Не было!

        // Добавляет закладку, или меняет (при необходимости) заголовок существующей
        AddTab(tab.Name, tabIdx, (tab.Type & 0x100)==0x100);
        nCurTab = tabIdx;
        tabIdx++;
    }

	// Update последних выбранных
	if (nCurTab >= 0 && (UINT)nCurTab < m_Tab2VCon.size())
		AddStack(m_Tab2VCon[nCurTab]);
	else
		CheckStack(); // иначе просто проверим стек

    // удалить лишние закладки (визуально)
    int nCurCount = GetItemCount();
	#ifdef _DEBUG
	wchar_t szDbg[128];
	wsprintf(szDbg, L"TabBarClass::Update.  ItemCount=%i, PrevItemCount=%i\n", tabIdx, nCurCount);
	DEBUGSTRTABS(szDbg);
	#endif
	for (I = tabIdx; I < nCurCount; I++) {
		#ifdef _DEBUG
		wsprintf(szDbg, L"   Deleting tab=%i\n", I+1);
		DEBUGSTRTABS(szDbg);
		#endif
        DeleteItem(tabIdx);
	}

	MCHKHEAP
    if (mb_InKeySwitching) {
	    if (mn_CurSelTab >= nCurCount) // Если выбранный таб вылез за границы
		    mb_InKeySwitching = FALSE;
    }
        
    if (!mb_InKeySwitching && nCurTab != -1) {
        SelectTab(nCurTab);
    }

	mn_InUpdate --;
	if (mb_PostUpdateRequested) {
		mb_PostUpdateCalled = FALSE;
		mb_PostUpdateRequested = FALSE;
		RequestPostUpdate();
	}
	MCHKHEAP
}

void TabBarClass::AddTab2VCon(VConTabs& vct)
{
	#ifdef _DEBUG
	std::vector<VConTabs>::iterator i = m_Tab2VCon.begin();
	while (i != m_Tab2VCon.end()) {
		_ASSERTE(i->pVCon!=vct.pVCon || i->nFarWindowId!=vct.nFarWindowId);
		i++;
	}
	#endif
	m_Tab2VCon.push_back(vct);
}

RECT TabBarClass::GetMargins()
{
    return m_Margins;
}

void TabBarClass::UpdatePosition()
{
    if (!mh_Rebar)
        return;

	if (gConEmu.isIconic())
		return; // иначе расчет размеров будет некорректным!

    RECT client;
    GetClientRect(ghWnd, &client); // нас интересует ширина окна

	DEBUGSTRTABS(_active ? L"TabBarClass::UpdatePosition(activate)\n" : L"TabBarClass::UpdatePosition(DEactivate)\n");
    
	#ifdef _DEBUG
	DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
	#endif

    if (_active) {
        if (mh_Rebar) {
            if (!IsWindowVisible(mh_Rebar))
                ShowWindow(mh_Rebar, SW_SHOW);
            //MoveWindow(mh_Rebar, 0, 0, client.right, _tabHeight, 1);
        } else {
            if (!IsWindowVisible(mh_Tabbar))
                ShowWindow(mh_Tabbar, SW_SHOW);
            if (gSet.isTabFrame)
                MoveWindow(mh_Tabbar, 0, 0, client.right, client.bottom, 1);
            else
                MoveWindow(mh_Tabbar, 0, 0, client.right, _tabHeight, 1);

        }

		#ifdef _DEBUG
		dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
		#endif

		//gConEmu.SyncConsoleToWindow(); -- 2009.07.04 Sync должен быть выполнен в самом ReSize
		gConEmu.ReSize(TRUE);
    } else {
		//gConEmu.SyncConsoleToWindow(); -- 2009.07.04 Sync должен быть выполнен в самом ReSize
		gConEmu.ReSize(TRUE);
		// _active уже сбросили, поэтому реально спрятать можно и позже
        if (mh_Rebar) {
            if (IsWindowVisible(mh_Rebar))
                ShowWindow(mh_Rebar, SW_HIDE);
        } else {
            if (IsWindowVisible(mh_Tabbar))
                ShowWindow(mh_Tabbar, SW_HIDE);
        }
    }
}

void TabBarClass::UpdateWidth()
{
    if (!_active)
    {
        return;
    }
    RECT client, self;
    GetClientRect(ghWnd, &client);
    GetWindowRect(mh_Tabbar, &self);
    if (mh_Rebar) {
		SIZE sz = {0,0};
		int nBarIndex = -1;
		BOOL lbNeedShow = FALSE, lbWideEnough = FALSE;
		if (mh_Toolbar) {
			nBarIndex = SendMessage(mh_Rebar, RB_IDTOINDEX, 2, 0);
			SendMessage(mh_Toolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz);
			sz.cx += (gSet.isHideCaptionAlways ? gSet.nToolbarAddSpace : 0);
			lbWideEnough = (sz.cx + 150) <= client.right;
			if (!lbWideEnough) {
				if (IsWindowVisible(mh_Toolbar))
					SendMessage(mh_Rebar, RB_SHOWBAND, nBarIndex, 0);
			} else {
				if (!IsWindowVisible(mh_Toolbar))
					lbNeedShow = TRUE;
			}
		}
        MoveWindow(mh_Rebar, 0, 0, client.right, _tabHeight, 1);
		if (lbWideEnough && nBarIndex != 1) {
			SendMessage(mh_Rebar, RB_MOVEBAND, nBarIndex, 1);
			nBarIndex = SendMessage(mh_Rebar, RB_IDTOINDEX, 2, 0);
			_ASSERTE(nBarIndex == 1);
		}
		if (lbNeedShow) {
			SendMessage(mh_Rebar, RB_SHOWBAND, nBarIndex, 1);
		}
    } else
    if (gSet.isTabFrame) {
        MoveWindow(mh_Tabbar, 0, 0, client.right, client.bottom, 1);
    } else {
        MoveWindow(mh_Tabbar, 0, 0, client.right, _tabHeight, 1);
    }

    UpdateToolbarPos();
}

void TabBarClass::UpdateToolbarPos()
{
    if (mh_Toolbar) {
        SIZE sz; 
        SendMessage(mh_Toolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz);
        if (mh_Rebar) {
            if (sz.cx != mn_LastToolbarWidth)
            {
                REBARBANDINFO rbBand={80}; // не используем size, т.к. приходит "новый" размер из висты и в XP обламываемся
                rbBand.fMask  = RBBIM_SIZE | RBBIM_CHILDSIZE;
                // Set values unique to the band with the toolbar.
                rbBand.cx = rbBand.cxMinChild = rbBand.cxIdeal = mn_LastToolbarWidth = sz.cx;
                rbBand.cyMinChild = sz.cy;

                // Add the band that has the toolbar.
                SendMessage(mh_Rebar, RB_SETBANDINFO, 1, (LPARAM)&rbBand);
            }
        } else {
            RECT rcClient;
            GetWindowRect(mh_Tabbar, &rcClient);
            MapWindowPoints(NULL, ghWnd, (LPPOINT)&rcClient, 2);
        }
    }
}

bool TabBarClass::OnNotify(LPNMHDR nmhdr)
{
	if (!this)
		return false;
    if (!_active)
    {
        return false;
    }

    //SetFocus(ghWnd); // 02.04.2009 Maks - ?
    if (nmhdr->code == TCN_SELCHANGING)
    {
        //if (mb_ChangeAllowed) {
        //  return FALSE;
        //}
        _prevTab = GetCurSel(); 
        return FALSE; // разрешаем
    }

    if (nmhdr->code == TCN_SELCHANGE)
    {
        int lnNewTab = GetCurSel();
        //_tcscpy(_lastTitle, gConEmu.Title);
        
        if (_prevTab>=0) {
            SelectTab(_prevTab);
            _prevTab = -1;
        }
        
        if (mb_ChangeAllowed) {
            return FALSE;
        }
        
        FarSendChangeTab(lnNewTab);
        // start waiting for title to change
        _titleShouldChange = true;
        return true;
    }

    if (nmhdr->code == TBN_GETINFOTIP /*&& nmhdr->hwndFrom == mh_Toolbar*/)
    {
        if (!gSet.isMulti)
            return 0;
        LPNMTBGETINFOTIP pDisp = (LPNMTBGETINFOTIP)nmhdr;
        if (pDisp->iItem>=1 && pDisp->iItem<=MAX_CONSOLE_COUNT) {
            if (!pDisp->pszText || !pDisp->cchTextMax) return false;
            LPCWSTR pszTitle = gConEmu.GetVCon(pDisp->iItem-1)->RCon()->GetTitle();
            if (pszTitle) {
                lstrcpyn(pDisp->pszText, pszTitle, pDisp->cchTextMax);
            } else {
                pDisp->pszText[0] = 0;
            }
        } else
        if (pDisp->iItem == TID_CREATE_CON) {
            lstrcpyn(pDisp->pszText, _T("Create new console"), pDisp->cchTextMax);
        } else
        if (pDisp->iItem == TID_BUFFERHEIGHT) {
	        BOOL lbPressed = (SendMessage(mh_Toolbar, TB_GETSTATE, pDisp->iItem, 0) & TBSTATE_CHECKED) == TBSTATE_CHECKED;
            lstrcpyn(pDisp->pszText, 
	            lbPressed ? L"BufferHeight mode is ON" : L"BufferHeight mode is off",
	            pDisp->cchTextMax);
        } else
        if (pDisp->iItem == TID_MINIMIZE) {
        	lstrcpyn(pDisp->pszText, _T("Minimize window"), pDisp->cchTextMax);
        } else 
        if (pDisp->iItem == TID_MAXIMIZE) {
			lstrcpyn(pDisp->pszText, _T("Maximize window"), pDisp->cchTextMax);
        } else
        if (pDisp->iItem == TID_APPCLOSE) {
        	lstrcpyn(pDisp->pszText, _T("Close ALL consoles"), pDisp->cchTextMax);
        }
        return true;
    }

    if (nmhdr->code == TTN_GETDISPINFO 
		&& (mh_Tabbar && (nmhdr->hwndFrom == mh_Tabbar || nmhdr->hwndFrom == mh_TabTip)))
    {
        LPNMTTDISPINFO pDisp = (LPNMTTDISPINFO)nmhdr;
        CVirtualConsole *pVCon = NULL;
        DWORD wndIndex = 0;
        TCHITTESTINFO htInfo;
        
        pDisp->hinst = NULL;
        pDisp->szText[0] = 0;
        pDisp->lpszText = NULL;
        
        GetCursorPos(&htInfo.pt);
        MapWindowPoints(NULL, mh_Tabbar, &htInfo.pt, 1);
        
        int iPage = TabCtrl_HitTest(mh_Tabbar, &htInfo);
        
        if (iPage >= 0) {
            // Если в табе нет "…" - тип не нужен
            if (!wcschr(GetTabText(iPage), L'\x2026' /*"…"*/))
                return 0;
        
            if (!GetVConFromTab(iPage, &pVCon, &wndIndex))
                return 0;
            if (!pVCon->RCon()->GetTab(wndIndex, &m_Tab4Tip))
                return 0;
        
            pDisp->lpszText = m_Tab4Tip.Name;
        }
        
        return true;
    }

    return false;
}

LPCWSTR TabBarClass::GetTabText(int nTabIdx)
{
    TCITEM item = {TCIF_TEXT};
    item.pszText = ms_TmpTabText; item.cchTextMax = sizeof(ms_TmpTabText)/sizeof(ms_TmpTabText[0]);
    ms_TmpTabText[0] = 0;
    
    if (!TabCtrl_GetItem(mh_Tabbar, nTabIdx, &item))
        return L"";
        
    return ms_TmpTabText;
}

void TabBarClass::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (!this)
		return;
    if (mh_Toolbar != (HWND)lParam)
        return;

	if (!gSet.isMulti)
        return;

    if (wParam>=1 && wParam<=MAX_CONSOLE_COUNT) {
        gConEmu.ConActivate(wParam-1);
    } else if (wParam == TID_CREATE_CON) {
        gConEmu.Recreate ( FALSE, gSet.isMultiNewConfirm );
    } else if (wParam == TID_BUFFERHEIGHT) {
		SendMessage(mh_Toolbar, TB_CHECKBUTTON, TID_BUFFERHEIGHT, gConEmu.ActiveCon()->RCon()->isBufferHeight());
		gConEmu.AskChangeBufferHeight();
    } else
    if (wParam == TID_MINIMIZE) {
    	PostMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
    } else 
    if (wParam == TID_MAXIMIZE) {
		// Чтобы клик случайно не провалился в консоль
		gConEmu.mouse.state |= MOUSE_SIZING_DBLCKL;
		// Аналог AltF9
		gConEmu.OnAltF9(TRUE);
    } else
    if (wParam == TID_APPCLOSE) {
    	PostMessage(ghWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    }
}

void TabBarClass::OnMouse(int message, int x, int y)
{
	if (!this)
		return;
    if (!_active)
    {
        return;
    }

    if (message == WM_MBUTTONUP || message == WM_RBUTTONUP)
    {
        TCHITTESTINFO htInfo;
        htInfo.pt.x = x;
        htInfo.pt.y = y;
        int iPage = TabCtrl_HitTest(mh_Tabbar, &htInfo);
        if (iPage != -1)
        {
            CVirtualConsole* pVCon = NULL;

			pVCon = FarSendChangeTab(iPage);

			if (pVCon) {
				if (pVCon->RCon()->GetFarPID()) {
					gConEmu.PostMacro(gSet.sTabCloseMacro ? gSet.sTabCloseMacro : L"F10");
				} else {
					gConEmu.Recreate ( TRUE, TRUE );
				}
			}
        }
    }
}

void TabBarClass::Invalidate()
{
    if (gConEmu.mp_TabBar->IsActive())
        InvalidateRect(mh_Rebar, NULL, TRUE);
}

void TabBarClass::OnCaptionHidden()
{
	if (!this) return;
	if (mh_Toolbar)
	{
		BOOL lbHide = !(gSet.isHideCaptionAlways 
			|| gSet.isFullScreen 
			|| (gConEmu.isZoomed() && gSet.isHideCaption));
		SendMessage(mh_Toolbar, TB_HIDEBUTTON, TID_MINIMIZE_SEP, lbHide);
		SendMessage(mh_Toolbar, TB_HIDEBUTTON, TID_MINIMIZE, lbHide);
		SendMessage(mh_Toolbar, TB_HIDEBUTTON, TID_MAXIMIZE, lbHide);
		SendMessage(mh_Toolbar, TB_HIDEBUTTON, TID_APPCLOSE, lbHide);
		
		SendMessage(mh_Toolbar, TB_AUTOSIZE, 0, 0);
		
		UpdateToolbarPos();
	}
}

void TabBarClass::OnWindowStateChanged()
{
	if (!this) return;
	if (mh_Toolbar)
	{
		TBBUTTONINFO tbi = {sizeof(TBBUTTONINFO), TBIF_IMAGE};
		tbi.iImage = (gSet.isFullScreen || gConEmu.isZoomed()) ? BID_MAXIMIZE_IDX : BID_RESTORE_IDX;
		SendMessage(mh_Toolbar, TB_SETBUTTONINFO, TID_MAXIMIZE, (LPARAM)&tbi);

		OnCaptionHidden();
	}
}

// nConNumber - 1based
void TabBarClass::OnConsoleActivated(int nConNumber/*, BOOL bAlternative*/)
{
    if (!mh_Toolbar) return;

    BOOL bPresent[MAX_CONSOLE_COUNT]; memset(bPresent, 0, sizeof(bPresent));
    MCHKHEAP
    for (int i=1; i<=MAX_CONSOLE_COUNT; i++) {
        bPresent[i-1] = gConEmu.GetTitle(i-1) != NULL;
    }

    SendMessage(mh_Toolbar, WM_SETREDRAW, 0, 0);
    for (int i=1; i<=MAX_CONSOLE_COUNT; i++) {
        SendMessage(mh_Toolbar, TB_HIDEBUTTON, i, !bPresent[i-1]);
    }

    UpdateToolbarPos();
    SendMessage(mh_Toolbar, WM_SETREDRAW, 1, 0);

    //nConNumber = gConEmu.ActiveConNum()+1; -- сюда пришел уже правильный номер!
    
    if (nConNumber>=1 && nConNumber<=MAX_CONSOLE_COUNT) {
        SendMessage(mh_Toolbar, TB_CHECKBUTTON, nConNumber, 1);
    } else {
        for (int i=1; i<=MAX_CONSOLE_COUNT; i++)
            SendMessage(mh_Toolbar, TB_CHECKBUTTON, i, 0);
    }
}

void TabBarClass::OnBufferHeight(BOOL abBufferHeight)
{
	if (!mh_Toolbar) return;

    SendMessage(mh_Toolbar, TB_CHECKBUTTON, TID_BUFFERHEIGHT, abBufferHeight);
}

HWND TabBarClass::CreateToolbar()
{
	gSet.CheckTheming();

	if (!mh_Rebar || !gSet.isMulti)
	    return NULL; // нет табов - нет и тулбара
	if (mh_Toolbar)
	    return mh_Toolbar; // Уже создали


	mh_Toolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, 
	    WS_CHILD|WS_VISIBLE|TBSTYLE_FLAT|CCS_NOPARENTALIGN|CCS_NORESIZE|CCS_NODIVIDER|TBSTYLE_TOOLTIPS|TBSTYLE_TRANSPARENT, 0, 0, 0, 0, mh_Rebar, 
	    NULL, NULL, NULL); 
	    
	_defaultToolProc = (WNDPROC)SetWindowLongPtr(mh_Toolbar, GWLP_WNDPROC, (LONG_PTR)ToolProc);


	SendMessage(mh_Toolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0); 
	SendMessage(mh_Toolbar, TB_SETBITMAPSIZE, 0, MAKELONG(14,14)); 
	TBADDBITMAP bmp = {g_hInstance,IDB_MAIN_TOOLBAR};
	int nFirst = SendMessage(mh_Toolbar, TB_ADDBITMAP, TID_BUFFERHEIGHT, (LPARAM)&bmp);

	//buttons
	TBBUTTON btn = {0, 1, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP};
	TBBUTTON sep = {0, 100, TBSTATE_ENABLED, TBSTYLE_SEP};
	int nActiveCon = gConEmu.ActiveConNum()+1;
	// Console numbers
	for (int i = 1; i <= 12; i++) {
	   btn.iBitmap = nFirst + i-1;
	   btn.idCommand = i;
	   btn.fsState = TBSTATE_ENABLED
		   | ((gConEmu.GetTitle(i-1) == NULL) ? TBSTATE_HIDDEN : 0)
		   | ((i == nActiveCon) ? TBSTATE_CHECKED : 0);
	   SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);
	}
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&sep); sep.idCommand++;

	// New console
	btn.fsStyle = BTNS_BUTTON; btn.idCommand = TID_CREATE_CON; btn.fsState = TBSTATE_ENABLED;
	btn.iBitmap = nFirst + BID_NEWCON_IDX;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);

	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&sep); sep.idCommand++;

	// Buffer height mode
	btn.iBitmap = nFirst + BID_BUFHEIGHT_IDX; btn.idCommand = TID_BUFFERHEIGHT; btn.fsState = TBSTATE_ENABLED;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);

	sep.fsState |= TBSTATE_HIDDEN; sep.idCommand = TID_MINIMIZE_SEP;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&sep);

	// Min,Max,Close
	btn.iBitmap = nFirst + BID_MINIMIZE_IDX; btn.idCommand = TID_MINIMIZE; btn.fsState = TBSTATE_ENABLED|TBSTATE_HIDDEN;
	SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);
	btn.iBitmap = nFirst + (gConEmu.isZoomed() ? BID_MAXIMIZE_IDX : BID_RESTORE_IDX); btn.idCommand = TID_MAXIMIZE;
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

HWND TabBarClass::GetTabbar()
{
	if (!this) return NULL;
	return mh_Tabbar;
}

int TabBarClass::GetTabbarHeight()
{
	if (!this) return NULL;
	return _tabHeight;
}

HWND TabBarClass::CreateTabbar()
{
	gSet.CheckTheming();

    if (!mh_Rebar)
        return NULL; // нет табов - нет и тулбара
    if (mh_Tabbar)
        return mh_Tabbar; // Уже создали
        
    if (!mh_TabIcons) {
    	mh_TabIcons = ImageList_LoadImage(g_hInstance, MAKEINTRESOURCE(IDB_SHIELD), 14, 1, RGB(128,0,0), IMAGE_BITMAP, LR_CREATEDIBSECTION);
		mn_AdminIcon = (gConEmu.m_osv.dwMajorVersion >= 6) ? 0 : 1;
    }

	// Важно проверку делать после создания главного окна, иначе IsAppThemed будет возвращать FALSE
    BOOL bAppThemed = FALSE, bThemeActive = FALSE;
    FAppThemed pfnThemed = NULL;
    HMODULE hUxTheme = LoadLibrary ( L"UxTheme.dll" );
    if (hUxTheme) {
    	pfnThemed = (FAppThemed)GetProcAddress( hUxTheme, "IsAppThemed" );
    	if (pfnThemed) bAppThemed = pfnThemed();
    	pfnThemed = (FAppThemed)GetProcAddress( hUxTheme, "IsThemeActive" );
    	if (pfnThemed) bThemeActive = pfnThemed();
    	FreeLibrary ( hUxTheme ); hUxTheme = NULL;
    }
    if (!bAppThemed || !bThemeActive)
    	mn_ThemeHeightDiff = 2;

    
    /*mh_TabbarP = CreateWindow(_T("VirtualConsoleClassBar"), _T(""), 
            WS_VISIBLE|WS_CHILD, 0,0,340,22, ghWnd, 0, 0, 0);
    if (!mh_TabbarP) return NULL;*/

        RECT rcClient;
        GetClientRect(ghWnd, &rcClient); 
        DWORD nPlacement = TCS_SINGLELINE|WS_VISIBLE/*|TCS_BUTTONS*//*|TCS_TOOLTIPS*/;
        mh_Tabbar = CreateWindow(WC_TABCONTROL, NULL, nPlacement | WS_CHILD | WS_CLIPSIBLINGS | TCS_FOCUSNEVER, 0, 0, 
            rcClient.right, 0, mh_Rebar, NULL, g_hInstance, NULL);
        if (mh_Tabbar == NULL)
        { 
            return NULL; 
        }

        #if !defined(__GNUC__)
        #pragma warning (disable : 4312)
        #endif
        // Надо
        _defaultTabProc = (WNDPROC)SetWindowLongPtr(mh_Tabbar, GWLP_WNDPROC, (LONG_PTR)TabProc);
        
        SendMessage(mh_Tabbar, TCM_SETIMAGELIST, 0, (LPARAM)mh_TabIcons);

        if (!mh_TabTip || !IsWindow(mh_TabTip)) {
            mh_TabTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
                                  WS_POPUP | TTS_ALWAYSTIP /*| TTS_BALLOON*/ | TTS_NOPREFIX,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  mh_Tabbar, NULL, 
                                  g_hInstance, NULL);
            SetWindowPos(mh_TabTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
            TabCtrl_SetToolTips ( mh_Tabbar, mh_TabTip );
        }
        HFONT hFont = CreateFont(gSet.nTabFontHeight, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, gSet.nTabFontCharSet, OUT_DEFAULT_PRECIS, 
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, gSet.sTabFontFace);
        SendMessage(mh_Tabbar, WM_SETFONT, WPARAM (hFont), TRUE);
        

        // Добавляет закладку, или меняет (при необходимости) заголовок существующей
        //AddTab(gConEmu.isFar() ? gSet.szTabPanels : gSet.pszTabConsole, 0);
        AddTab(gConEmu.GetTitle(), 0, false);

		GetClientRect(ghWnd, &rcClient); 
		TabCtrl_AdjustRect(mh_Tabbar, FALSE, &rcClient);
		_tabHeight = rcClient.top - mn_ThemeHeightDiff;



   return mh_Tabbar;
}

void TabBarClass::CreateRebar()
{
    RECT rcWnd; GetClientRect(ghWnd, &rcWnd);

	gSet.CheckTheming();

    if (NULL == (mh_Rebar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
                WS_VISIBLE |WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
                /*CCS_NORESIZE|*/CCS_NOPARENTALIGN|
                RBS_FIXEDORDER|RBS_AUTOSIZE|/*RBS_VARHEIGHT|*/CCS_NODIVIDER,
                0,0,rcWnd.right,16, ghWnd, NULL, g_hInstance, NULL)))
        return;

	#if !defined(__GNUC__)
	#pragma warning (disable : 4312)
	#endif
	// Надо
	_defaultReBarProc = (WNDPROC)SetWindowLongPtr(mh_Rebar, GWLP_WNDPROC, (LONG_PTR)ReBarProc);

    REBARINFO     rbi={sizeof(REBARINFO)};
    REBARBANDINFO rbBand={80}; // не используем size, т.к. приходит "новый" размер из висты и в XP обламываемся

    if(!SendMessage(mh_Rebar, RB_SETBARINFO, 0, (LPARAM)&rbi)) {
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
    if (mh_Toolbar) {
        SendMessage(mh_Toolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz);
    } else {
        RECT rcClient;
        GetClientRect(ghWnd, &rcClient); 
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
		rbBand.cyChild = rbBand.cyMinChild = rbBand.cyMaxChild = _tabHeight; // sz.cy;

        if (!SendMessage(mh_Rebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand)) {
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
        if (!SendMessage(mh_Rebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand)) {
            DisplayLastError(_T("Can't initialize rebar (toolbar)"));
        }
        
        //if (mn_ThemeHeightDiff) {
        //	POINT pt = {0,0};
        //	MapWindowPoints(mh_Toolbar, mh_Rebar, &pt, 1);
        //	pt.y = 0;
        //	SetWindowPos(mh_Toolbar, NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
       	//}
    }

    RECT rc;
    GetWindowRect(mh_Rebar, &rc);


    //GetWindowRect(mh_Rebar, &rc);
	//_tabHeight = rc.bottom - rc.top;
    m_Margins = MakeRect(0,_tabHeight,0,0);
    gSet.UpdateMargins(m_Margins);

    //_hwndTab = mh_Rebar; // пока...
}

void TabBarClass::PrepareTab(ConEmuTab* pTab)
{
	#ifdef _DEBUG
	if (this != gConEmu.mp_TabBar) {
		_ASSERTE(this == gConEmu.mp_TabBar);
	}
	#endif

	MCHKHEAP
    // get file name
    TCHAR dummy[MAX_PATH*2];
    TCHAR fileName[MAX_PATH+4];
    TCHAR szFormat[32];
    TCHAR szEllip[MAX_PATH+1];
    wchar_t *tFileName=NULL, *pszNo=NULL, *pszTitle=NULL; //--Maximus
    int nSplit = 0;
    int nMaxLen = 0; //gSet.nTabLenMax - _tcslen(szFormat) + 2/* %s */;
    int origLength = 0; //_tcslen(tFileName);
    if (pTab->Name[0]==0 || (pTab->Type & 0xFF) == 1/*WTYPE_PANELS*/) {
	    //_tcscpy(szFormat, _T("%s"));
	    _tcscpy(szFormat, gSet.szTabConsole);
	    nMaxLen = gSet.nTabLenMax - _tcslen(szFormat) + 2/* %s */;
	    
        if (pTab->Name[0] == 0) {
            #ifdef _DEBUG
            // Это должно случаться ТОЛЬКО при инициализации GUI
            int nTabCount = GetItemCount();
            if (nTabCount>0 && gConEmu.ActiveCon()!=NULL) {
                //_ASSERTE(pTab->Name[0] != 0);
                nTabCount = nTabCount;
            }
            #endif
            _tcscpy(pTab->Name, gConEmu.GetTitle()); //isFar() ? gSet.szTabPanels : gSet.pszTabConsole);
        }
        tFileName = pTab->Name;
        origLength = _tcslen(tFileName);
        //if (origLength>6) {
	    //    // Чтобы в заголовке было что-то вроде "{C:\Program Fil...- Far"
	    //    //                              вместо "{C:\Program F...} - Far"
		//	После добавления суффиков к заголовку фара - оно уже влезать не будет в любом случае... Так что если панели - '...' строго ставить в конце
	    //    if (lstrcmp(tFileName + origLength - 6, L" - Far") == 0)
		//        nSplit = nMaxLen - 6;
	    //}
	        
    } else {
        GetFullPathName(pTab->Name, MAX_PATH*2, dummy, &tFileName);
        if (!tFileName)
            tFileName = pTab->Name;

        if ((pTab->Type & 0xFF) == 3/*WTYPE_EDITOR*/) {
            if (pTab->Modified)
                _tcscpy(szFormat, gSet.szTabEditorModified);
            else
                _tcscpy(szFormat, gSet.szTabEditor);
        } 
        else if ((pTab->Type & 0xFF) == 2/*WTYPE_VIEWER*/)
            _tcscpy(szFormat, gSet.szTabViewer);
    }
    // restrict length
    if (!nMaxLen)
	    nMaxLen = gSet.nTabLenMax - _tcslen(szFormat) + 2/* %s */;
	if (!origLength)
	    origLength = _tcslen(tFileName);
    if (nMaxLen<15) nMaxLen=15; else
        if (nMaxLen>=MAX_PATH) nMaxLen=MAX_PATH-1;
    if (origLength > nMaxLen)
    {
        /*_tcsnset(fileName, _T('\0'), MAX_PATH);
        _tcsncat(fileName, tFileName, 10);
        _tcsncat(fileName, _T("..."), 3);
        _tcsncat(fileName, tFileName + origLength - 10, 10);*/
        //if (!nSplit)
	    //    nSplit = nMaxLen*2/3;
		//// 2009-09-20 Если в заголовке нет расширения (отсутствует точка)
		//const wchar_t* pszAdmin = gSet.szAdminTitleSuffix;
		//const wchar_t* pszFrom = tFileName + origLength - (nMaxLen - nSplit);
		//if (!wcschr(pszFrom, L'.') && (*pszAdmin && !wcsstr(tFileName, pszAdmin)))
		//{
		//	// то троеточие ставить в конец, а не середину
		//	nSplit = nMaxLen;
		//}
		
		// "{C:\Program Files} - Far 2.1283 Administrator x64"
		// После добавления суффиков к заголовку фара - оно уже влезать не будет в любом случае... Так что если панели - '...' строго ставить в конце
		nSplit = nMaxLen;
        
        _tcsncpy(szEllip, tFileName, nSplit); szEllip[nSplit]=0;
        szEllip[nSplit] = L'\x2026' /*"…"*/;
        szEllip[nSplit+1] = 0;
        //_tcscat(szEllip, L"\x2026" /*"…"*/);
        //_tcscat(szEllip, tFileName + origLength - (nMaxLen - nSplit));
        
        tFileName = szEllip;
    }
    // szFormat различается для Panel/Viewer(*)/Editor(*)
    // Пример: "%i-[%s] *"
    pszNo = wcsstr(szFormat, L"%i");
    pszTitle = wcsstr(szFormat, L"%s");
    if (pszNo == NULL)
        wsprintf(fileName, szFormat, tFileName);
    else if (pszNo < pszTitle || pszTitle == NULL)
        wsprintf(fileName, szFormat, pTab->Pos, tFileName);
    else
        wsprintf(fileName, szFormat, tFileName, pTab->Pos);

    wcscpy(pTab->Name, fileName);
	MCHKHEAP
}



// Переключение табов

int TabBarClass::GetIndexByTab(VConTabs tab)
{
	int nIdx = -1;
	std::vector<VConTabs>::iterator iter = m_Tab2VCon.begin();
	while (iter != m_Tab2VCon.end()) {
		nIdx ++;
		if (*iter == tab)
			return nIdx;
		iter ++;
	}
	return -1;
}

int TabBarClass::GetNextTab(BOOL abForward, BOOL abAltStyle/*=FALSE*/)
{
    BOOL lbRecentMode = (gSet.isTabs != 0) &&
		(((abAltStyle == FALSE) ? gSet.isTabRecent : !gSet.isTabRecent));
    int nCurSel = GetCurSel();
    int nCurCount = GetItemCount();
	VConTabs cur = {NULL};
    
	#ifdef _DEBUG
	if (nCurCount != m_Tab2VCon.size()) {
		_ASSERTE(nCurCount == m_Tab2VCon.size());
	}
	#endif
    if (nCurCount < 1)
    	return 0; // хотя такого и не должно быть
    
    if (lbRecentMode && nCurSel >= 0 && (UINT)nCurSel < m_Tab2VCon.size())
        cur = m_Tab2VCon[nCurSel];
    
    
    int i, nNewSel = -1;

    TODO("Добавить возможность переключаться а'ля RecentScreens");
    if (abForward) {
    	if (lbRecentMode) {
        	std::vector<VConTabs>::iterator iter = m_TabStack.begin();
        	while (iter != m_TabStack.end()) {
				VConTabs Item = *iter;
        		// Найти в стеке выделенный таб
        		if (Item == cur) {
        			// Определить следующий таб, который мы можем активировать
        			do {
	        			iter ++; // Если дошли до конца (сейчас выделен последний таб) вернуть первый
    	    			if (iter == m_TabStack.end()) iter = m_TabStack.begin();
    	    			// Определить индекс в m_Tab2VCon
    	    			i = GetIndexByTab ( *iter );
    	    			if (CanActivateTab(i)) {
    	    				return i;
        				}
        			} while (*iter != cur);
        			break;
        		}
				iter ++;
        	}
    	} // Если не смогли в стиле Recent - идем простым путем
    	
    
        for (i = nCurSel+1; nNewSel == -1 && i < nCurCount; i++)
            if (CanActivateTab(i)) nNewSel = i;
        
        for (i = 0; nNewSel == -1 && i < nCurSel; i++)
            if (CanActivateTab(i)) nNewSel = i;

    } else {
    
    	if (lbRecentMode) {
        	std::vector<VConTabs>::reverse_iterator iter = m_TabStack.rbegin();
        	while (iter != m_TabStack.rend()) {
				VConTabs Item = *iter;
        		// Найти в стеке выделенный таб
        		if (Item == cur) {
        			// Определить следующий таб, который мы можем активировать
        			do {
	        			iter ++; // Если дошли до конца (сейчас выделен последний таб) вернуть первый
    	    			if (iter == m_TabStack.rend()) iter = m_TabStack.rbegin();
    	    			// Определить индекс в m_Tab2VCon
    	    			i = GetIndexByTab ( *iter );
    	    			if (CanActivateTab(i)) {
    	    				return i;
        				}
        			} while (*iter != cur);
        			break;
        		}
				iter++;
        	}
    	} // Если не смогли в стиле Recent - идем простым путем
    
        for (i = nCurSel-1; nNewSel == -1 && i >= 0; i++)
            if (CanActivateTab(i)) nNewSel = i;
        
        for (i = nCurCount-1; nNewSel == -1 && i > nCurSel; i++)
            if (CanActivateTab(i)) nNewSel = i;

    }

    return nNewSel;
}

void TabBarClass::SwitchNext(BOOL abAltStyle/*=FALSE*/)
{
	Switch(TRUE, abAltStyle);
}

void TabBarClass::SwitchPrev(BOOL abAltStyle/*=FALSE*/)
{
	Switch(FALSE, abAltStyle);
}

void TabBarClass::Switch(BOOL abForward, BOOL abAltStyle/*=FALSE*/)
{
    int nNewSel = GetNextTab ( abForward, abAltStyle );
    
    if (nNewSel != -1) {
		// mh_Tabbar может быть и создан, Но отключен пользователем!
        if (gSet.isTabLazy && mh_Tabbar && gSet.isTabs) {
            mb_InKeySwitching = TRUE;
            // Пока Ctrl не отпущен - только подсвечиваем таб, а не переключаем реально
            SelectTab ( nNewSel );
        } else {
            FarSendChangeTab ( nNewSel );
			mb_InKeySwitching = FALSE;
        }
    }
}

BOOL TabBarClass::IsInSwitch()
{
	return mb_InKeySwitching;
}

void TabBarClass::SwitchCommit()
{
    if (!mb_InKeySwitching) return;
    
    int nCurSel = GetCurSel();
    
    FarSendChangeTab(nCurSel);
    
    mb_InKeySwitching = FALSE;
}

void TabBarClass::SwitchRollback()
{
	if (mb_InKeySwitching) {
		mb_InKeySwitching = FALSE;
		Update();
	}
}

// Убьет из стека старые, и добавит новые
void TabBarClass::CheckStack()
{
	_ASSERTE(gConEmu.isMainThread());

	std::vector<VConTabs>::iterator i, j;
	BOOL lbExist = FALSE;

	j = m_TabStack.begin();
	while (j != m_TabStack.end()) {
		lbExist = FALSE;
		for (i = m_Tab2VCon.begin(); i != m_Tab2VCon.end(); i++) {
			if (*i == *j) {
				lbExist = TRUE; break;
			}
		}
		if (lbExist)
			j++;
		else
			j = m_TabStack.erase(j);
	}

	for (i = m_Tab2VCon.begin(); i != m_Tab2VCon.end(); i++) {
		lbExist = FALSE;
		for (j = m_TabStack.begin(); j != m_TabStack.end(); j++) {
			if (*i == *j) {
				lbExist = TRUE; break;
			}
		}
		if (!lbExist)
			m_TabStack.push_back(*i);
	}
}

// Убьет из стека отсутствующих и поместит tab на верх стека
void TabBarClass::AddStack(VConTabs tab)
{
	_ASSERTE(gConEmu.isMainThread());
	
	BOOL lbExist = FALSE;
	if (!m_TabStack.empty()) {
		//VConTabs tmp;
		std::vector<VConTabs>::iterator iter = m_TabStack.begin();
		while (iter != m_TabStack.end()) {
			if (*iter == tab) {
				if (iter == m_TabStack.begin()) {
					lbExist = TRUE;
				} else {
					m_TabStack.erase(iter);
				}
				break;
			}
			iter ++;
		}
	}
	if (!lbExist) // поместить наверх стека
		m_TabStack.insert(m_TabStack.begin(), tab);

	CheckStack();
}

BOOL TabBarClass::CanActivateTab(int nTabIdx)
{
    CVirtualConsole *pVCon = NULL;
    DWORD wndIndex = 0;

    if (!GetVConFromTab(nTabIdx, &pVCon, &wndIndex))
        return FALSE;

    if (!pVCon->RCon()->CanActivateFarWindow(wndIndex))
        return FALSE;
        
    return TRUE;
}

BOOL TabBarClass::OnKeyboard(UINT messg, WPARAM wParam, LPARAM lParam)
{
    //if (!IsShown()) return FALSE; -- всегда. Табы теперь есть в памяти
    BOOL lbAltPressed = isPressed(VK_MENU);

    if (messg == WM_KEYDOWN && wParam == VK_TAB)
    {
        if (!isPressed(VK_SHIFT))
            SwitchNext(lbAltPressed);
        else
            SwitchPrev(lbAltPressed);
    } else if (mb_InKeySwitching && messg == WM_KEYDOWN && !lbAltPressed
        && (wParam == VK_UP || wParam == VK_DOWN || wParam == VK_LEFT || wParam == VK_RIGHT))
    {
    	bool bRecent = gSet.isTabRecent;
		gSet.isTabRecent = false;
    	BOOL bForward = (wParam == VK_RIGHT || wParam == VK_DOWN);
    	Switch(bForward);
    	gSet.isTabRecent = bRecent;
    }
        
    return TRUE;
}

void TabBarClass::SetRedraw(BOOL abEnableRedraw)
{
	mb_DisableRedraw = !abEnableRedraw;
}
