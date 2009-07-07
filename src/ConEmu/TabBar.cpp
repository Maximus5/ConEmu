
#include <windows.h>
#include <commctrl.h>
#include "header.h"

WARNING("!!! Запустили far, открыли edit, перешли в панель, открыли второй edit, ESC, ни одна вкладка не активна");
// Более того, если есть еще одна консоль - активной станет первая вкладка следующей НЕАКТИВНОЙ консоли
WARNING("не меняются табы при переключении на другую консоль");

TODO("Для WinXP можно поиграться стилем WS_EX_COMPOSITED");

TabBarClass TabBar;
const int TAB_FONT_HEIGTH = 16;
wchar_t TAB_FONT_FACE[] = L"Tahoma";
WNDPROC TabBarClass::_defaultTabProc = NULL;
WNDPROC TabBarClass::_defaultBarProc = NULL;
typedef BOOL (WINAPI* FAppThemed)();

#ifndef TBN_GETINFOTIP
#define TBN_GETINFOTIP TBN_GETINFOTIPW
#endif
#ifndef RB_SETWINDOWTHEME
#define CCM_SETWINDOWTHEME      (CCM_FIRST + 0xb)
#define RB_SETWINDOWTHEME       CCM_SETWINDOWTHEME
#endif


TabBarClass::TabBarClass()
{
    _active = false;
    _tabHeight = 0;
	memset(&m_Margins, 0, sizeof(m_Margins));
    _titleShouldChange = false;
    _prevTab = -1;
    mb_ChangeAllowed = FALSE;
    mb_Enabled = TRUE;
    mh_Toolbar = NULL; mh_Tabbar = NULL; mh_Rebar = NULL; mn_LastToolbarWidth = 0;
    mb_PostUpdateCalled = FALSE;
    mn_MsgUpdateTabs = RegisterWindowMessage(CONEMUMSG_UPDATETABS);
    memset(&m_Tab4Tip, 0, sizeof(m_Tab4Tip));
    mb_InKeySwitching = FALSE;
    ms_TmpTabText[0] = 0;
	mn_CurSelTab = 0;
	mn_ThemeHeightDiff = 0;
}

void TabBarClass::Enable(BOOL abEnabled)
{
    if (mh_Tabbar && mb_Enabled!=abEnabled)
    {
        //EnableWindow(mh_Tabbar, abEnabled);
        mb_Enabled = abEnabled;
    }
}

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

void TabBarClass::Refresh(BOOL abFarActive)
{
    Enable(abFarActive);
}

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
    //TabBar.Update(&tab, 1);
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
    //      gConEmu.DnDstep(_T("Tabs: Checking for plugin (1 sec)"));
    //      // Подождем немножко, проверим что плагин живой
    //      cbWritten = WaitForSingleObject(pipe.hEventAlive, CONEMUALIVETIMEOUT);
    //      if (cbWritten!=WAIT_OBJECT_0) {
    //          TCHAR szErr[MAX_PATH];
    //          wsprintf(szErr, _T("ConEmu plugin is not active!\r\nProcessID=%i"), pipe.nPID);
    //          MBoxA(szErr);
    //      } else {
    //          gConEmu.DnDstep(_T("Tabs: Waiting for result (10 sec)"));
    //          cbWritten = WaitForSingleObject(pipe.hEventReady, CONEMUREADYTIMEOUT);
    //          if (cbWritten!=WAIT_OBJECT_0) {
    //              TCHAR szErr[MAX_PATH];
    //              wsprintf(szErr, _T("Command waiting time exceeds!\r\nConEmu plugin is locked?\r\nProcessID=%i"), pipe.nPID);
    //              MBoxA(szErr);
    //          } else {
    //              gConEmu.DnDstep(_T("Tabs: Recieving data"));
    //              DWORD cbBytesRead=0;
    //              int nTabCount=0;
    //              pipe.Read(&nTabCount, sizeof(nTabCount), &cbBytesRead);

    //              if (nTabCount<=0) {
    //                  gConEmu.DnDstep(_T("Tabs: data empty"));
    //                  this->Reset();
    //              } else {
    //                  COPYDATASTRUCT cds = {0};
    //                  
    //                  cds.dwData = nTabCount;
    //                  cds.lpData = pipe.GetPtr(); // хвост

    //                  gConEmu.OnCopyData(&cds);
    //                  gConEmu.DnDstep(NULL);
    //              }
    //          }
    //      }
    //  }
    //}
}

// Добавляет закладку, или меняет (при необходимости) заголовок существующей
void TabBarClass::AddTab(LPCWSTR text, int i)
{
	if (mh_Tabbar) {
		TCITEM tie;
		tie.mask = TCIF_TEXT;
		tie.iImage = -1; 
		tie.pszText = (LPWSTR)text ;

		int nCurCount = GetItemCount();
		if (i>=nCurCount) {
			TabCtrl_InsertItem(mh_Tabbar, i, &tie);
		} else {
			if (wcscmp(GetTabText(i), text)) // "меняем" только если он реально меняется
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

BOOL TabBarClass::GetVConFromTab(int nTabIdx, CVirtualConsole** rpVCon, DWORD* rpWndIndex)
{
    BOOL lbRc = FALSE;
    CVirtualConsole *pVCon = NULL;
    DWORD wndIndex = 0;

    if (nTabIdx >= 0 && (UINT)nTabIdx < m_Tab2VCon.size()) {
        pVCon = m_Tab2VCon[nTabIdx].pVCon;
        wndIndex = m_Tab2VCon[nTabIdx].nFarWindowId;

        if (!gConEmu.isValid(pVCon)) {
            if (!mb_PostUpdateCalled)
            {
                mb_PostUpdateCalled = TRUE;
                PostMessage(ghWnd, mn_MsgUpdateTabs, 0, 0);
            }
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
    //SetWindowLong(ghWndDC, GWL_TABINDEX, tabIndex);
    //PostMessage(ghConWnd, WM_KEYDOWN, VK_F14, 0);
    //PostMessage(ghConWnd, WM_KEYUP, VK_F14, 0);
    //PostMessage(ghConWnd, WM_KEYDOWN, FarTabShortcut(tabIndex), 0);
    //PostMessage(ghConWnd, WM_KEYUP, FarTabShortcut(tabIndex), 0);

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

LRESULT CALLBACK TabBarClass::TabProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        {
            TabBar.OnMouse(uMsg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
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

LRESULT CALLBACK TabBarClass::BarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_WINDOWPOSCHANGING:
        {
        	LPWINDOWPOS pos = (LPWINDOWPOS)lParam;
            pos->y = (TabBar.mn_ThemeHeightDiff == 0) ? 1 : 0;
            return 0;
        }
    }
    return CallWindowProc(_defaultBarProc, hwnd, uMsg, wParam, lParam);
}


bool TabBarClass::IsActive()
{
    return _active;
}

bool TabBarClass::IsShown()
{
    return _active && IsWindowVisible(mh_Tabbar);
}

BOOL TabBarClass::IsAllowed()
{
    BOOL lbTabsAllowed = TRUE;
    TODO("пока убрал");
    //if (gConEmu.BufferHeight) {
        CVirtualConsole* pCon = gConEmu.ActiveCon();
        if (!pCon) 
            lbTabsAllowed = FALSE;
        else
            lbTabsAllowed = !pCon->RCon()->isBufferHeight();
    //}
    return lbTabsAllowed;
}

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
    /*if (!_active)
    {
        return;
    }*/ // Теперь - ВСЕГДА! т.к. сами управляем мультиконсолью
    
    if (!gConEmu.isMainThread()) {
        if (mb_PostUpdateCalled) return;
        mb_PostUpdateCalled = TRUE;
        PostMessage(ghWnd, mn_MsgUpdateTabs, 0, 0);
        return;
    }
    
    mb_PostUpdateCalled = FALSE;

    ConEmuTab tab = {0};
    
    int V, I, tabIdx = 0, nCurTab = -1;
	CVirtualConsole* pVCon = NULL;
    VConTabs vct = {{NULL}};

    // Выполняться должно только в основной нити, так что CriticalSection не нужна
    m_Tab2VCon.clear();

	if (!TabBar.IsActive() && gSet.isTabs) {
		int nTabs = 0;
		for (V = 0; V < MAX_CONSOLE_COUNT && nTabs < 2; V++) {
			if (!(pVCon = gConEmu.GetVCon(V))) continue;
			nTabs += pVCon->RCon()->GetTabCount();
		}
		if (nTabs > 1)
			Activate();
	} else if (TabBar.IsActive() && gSet.isTabs==2) {
		int nTabs = 0;
		for (V = 0; V < MAX_CONSOLE_COUNT && nTabs < 2; V++) {
			if (!(pVCon = gConEmu.GetVCon(V))) continue;
			nTabs += pVCon->RCon()->GetTabCount();
		}
		if (nTabs <= 1)
			Deactivate();
	}
    
    for (V = 0; V < MAX_CONSOLE_COUNT; V++) {
        if (!(pVCon = gConEmu.GetVCon(V))) continue;
        
        BOOL lbActive = gConEmu.isActive(pVCon);
        
        for (I = 0; pVCon->RCon()->GetTab(I, &tab); I++) {
            PrepareTab(&tab);
            
            vct.pVCon = pVCon;
            vct.nFarWindowId = I;
            m_Tab2VCon.push_back(vct);
            // Добавляет закладку, или меняет (при необходимости) заголовок существующей
            AddTab(tab.Name, tabIdx);
            
            if (lbActive && tab.Current)
                nCurTab = tabIdx;
            
            tabIdx++;
        }
    }
    if (tabIdx == 0) // хотя бы "Console" покажем
    {
        PrepareTab(&tab);
        
        vct.pVCon = NULL;
        vct.nFarWindowId = 0;
		m_Tab2VCon.push_back(vct); //2009-06-14. Не было!

        // Добавляет закладку, или меняет (при необходимости) заголовок существующей
        AddTab(tab.Name, tabIdx);
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
    for (I = tabIdx; I < nCurCount; I++)
        DeleteItem(I);

    if (mb_InKeySwitching) {
	    if (mn_CurSelTab >= nCurCount) // Если выбранный таб вылез за границы
		    mb_InKeySwitching = FALSE;
    }
        
    if (!mb_InKeySwitching && nCurTab != -1) {
        SelectTab(nCurTab);
    }
}


RECT TabBarClass::GetMargins()
{
    return m_Margins;
}

void TabBarClass::UpdatePosition()
{
    if (!mh_Rebar)
        return;

	if (IsIconic(ghWnd))
		return; // иначе расчет размеров будет некорректным!

    RECT client;
    GetClientRect(ghWnd, &client); // нас интересует ширина окна
    
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
        if (pDisp->iItem==13) {
            lstrcpyn(pDisp->pszText, _T("Create new console"), pDisp->cchTextMax);
        } else
        if (pDisp->iItem==14) {
	        BOOL lbPressed = (SendMessage(mh_Toolbar, TB_GETSTATE, pDisp->iItem, 0) & TBSTATE_CHECKED) == TBSTATE_CHECKED;
            lstrcpyn(pDisp->pszText, 
	            lbPressed ? L"BufferHeight mode is ON" : L"BufferHeight mode is off",
	            pDisp->cchTextMax);
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
    if (mh_Toolbar != (HWND)lParam)
        return;

	if (!gSet.isMulti)
        return;

    if (wParam>=1 && wParam<=MAX_CONSOLE_COUNT) {
        gConEmu.ConmanAction(wParam-1);
    } else if (wParam==13) {
        gConEmu.Recreate ( FALSE, gSet.isMultiNewConfirm ); //ConmanAction(CONMAN_NEWCONSOLE);
    } else if (wParam==14) {
		SendMessage(mh_Toolbar, TB_CHECKBUTTON, 14, gConEmu.ActiveCon()->RCon()->isBufferHeight());
    }
}

void TabBarClass::OnMouse(int message, int x, int y)
{
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
				gConEmu.PostMacro(gSet.sTabCloseMacro ? gSet.sTabCloseMacro : L"F10");
			}
        }
    }
}

void TabBarClass::Invalidate()
{
    if (TabBar.IsActive())
        InvalidateRect(mh_Rebar, NULL, TRUE);
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

    SendMessage(mh_Toolbar, TB_CHECKBUTTON, 14, abBufferHeight);
}

HWND TabBarClass::CreateToolbar()
{
    if (!mh_Rebar || !gSet.isMulti)
        return NULL; // нет табов - нет и тулбара
    if (mh_Toolbar)
        return mh_Toolbar; // Уже создали


    mh_Toolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, 
        WS_CHILD|WS_VISIBLE|TBSTYLE_FLAT|CCS_NOPARENTALIGN|CCS_NORESIZE|CCS_NODIVIDER|TBSTYLE_TOOLTIPS|TBSTYLE_TRANSPARENT, 0, 0, 0, 0, mh_Rebar, 
        NULL, NULL, NULL); 
        
   _defaultBarProc = (WNDPROC)SetWindowLongPtr(mh_Toolbar, GWLP_WNDPROC, (LONG_PTR)BarProc);

 
   SendMessage(mh_Toolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0); 
   SendMessage(mh_Toolbar, TB_SETBITMAPSIZE, 0, MAKELONG(16,16)); 
   TBADDBITMAP bmp = {g_hInstance,IDB_CONMAN1};
   int nFirst = SendMessage(mh_Toolbar, TB_ADDBITMAP, 14, (LPARAM)&bmp);

   //buttons
   TBBUTTON btn = {0, 1, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP}, sep = {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP};
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
   SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&sep);

   // New console
   btn.fsStyle = BTNS_BUTTON; btn.iBitmap = nFirst + 12; btn.idCommand = 13; btn.fsState = TBSTATE_ENABLED;
   SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);
   SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&sep);

   // Buffer height mode
   btn.iBitmap = nFirst + 13; btn.idCommand = 14; btn.fsState = TBSTATE_ENABLED;
   SendMessage(mh_Toolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);


   SendMessage(mh_Toolbar, TB_AUTOSIZE, 0, 0);
   SIZE sz; 
   SendMessage(mh_Toolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz);


   return mh_Toolbar;
}

HWND TabBarClass::CreateTabbar()
{
    if (!mh_Rebar)
        return NULL; // нет табов - нет и тулбара
    if (mh_Tabbar)
        return mh_Tabbar; // Уже создали

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
        HFONT hFont = CreateFont(TAB_FONT_HEIGTH, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, 
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TAB_FONT_FACE);
        SendMessage(mh_Tabbar, WM_SETFONT, WPARAM (hFont), TRUE);
        

        // Добавляет закладку, или меняет (при необходимости) заголовок существующей
        //AddTab(gConEmu.isFar() ? gSet.szTabPanels : gSet.pszTabConsole, 0);
        AddTab(gConEmu.GetTitle(), 0);

		GetClientRect(ghWnd, &rcClient); 
		TabCtrl_AdjustRect(mh_Tabbar, FALSE, &rcClient);
		_tabHeight = rcClient.top - mn_ThemeHeightDiff;



   return mh_Tabbar;
}

void TabBarClass::CreateRebar()
{
    RECT rcWnd; GetClientRect(ghWnd, &rcWnd);


    if (NULL == (mh_Rebar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
                WS_VISIBLE |WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
                /*CCS_NORESIZE|*/CCS_NOPARENTALIGN|
                RBS_FIXEDORDER|RBS_AUTOSIZE|/*RBS_VARHEIGHT|*/CCS_NODIVIDER,
                0,0,rcWnd.right,16, ghWnd, NULL, g_hInstance, NULL)))
        return;

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
	_ASSERTE(this == &TabBar);
    // get file name
    TCHAR dummy[MAX_PATH*2];
    TCHAR fileName[MAX_PATH+4];
    TCHAR szFormat[32];
    TCHAR szEllip[MAX_PATH+1];
    wchar_t *tFileName=NULL, *pszNo=NULL, *pszTitle=NULL; //--Maximus
    int nSplit = 0;
    int nMaxLen = 0; //gSet.nTabLenMax - _tcslen(szFormat) + 2/* %s */;
    int origLength = 0; //_tcslen(tFileName);
    if (pTab->Name[0]==0 || pTab->Type == 1/*WTYPE_PANELS*/) {
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
        if (origLength>6) {
	        // Чтобы в заголовке было что-то вроде "{C:\Program Fil...- Far"
	        //                              вместо "{C:\Program F...} - Far"
	        if (lstrcmp(tFileName + origLength - 6, L" - Far") == 0)
		        nSplit = nMaxLen - 6;
	    }
	        
    } else {
        GetFullPathName(pTab->Name, MAX_PATH*2, dummy, &tFileName);
        if (!tFileName)
            tFileName = pTab->Name;

        if (pTab->Type == 3/*WTYPE_EDITOR*/) {
            if (pTab->Modified)
                _tcscpy(szFormat, gSet.szTabEditorModified);
            else
                _tcscpy(szFormat, gSet.szTabEditor);
        } 
        else if (pTab->Type == 2/*WTYPE_VIEWER*/)
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
        if (!nSplit)
	        nSplit = nMaxLen*2/3;
        
        _tcsncpy(szEllip, tFileName, nSplit); szEllip[nSplit]=0;
        _tcscat(szEllip, L"\x2026" /*"…"*/);
        _tcscat(szEllip, tFileName + origLength - (nMaxLen - nSplit));
        
        tFileName = szEllip;
    }
    pszNo = wcsstr(szFormat, L"%i");
    pszTitle = wcsstr(szFormat, L"%s");
    if (pszNo == NULL)
        wsprintf(fileName, szFormat, tFileName);
    else if (pszNo < pszTitle || pszTitle == NULL)
        wsprintf(fileName, szFormat, pTab->Pos, tFileName);
    else
        wsprintf(fileName, szFormat, tFileName, pTab->Pos);

    wcscpy(pTab->Name, fileName);
}



// Переключение табов

int TabBarClass::GetIndexByTab(VConTabs tab)
{
	int nIdx = -1;
	std::vector<VConTabs>::iterator iter = m_Tab2VCon.begin();
	while (iter != m_Tab2VCon.end()) {
		nIdx ++;
		if (iter->ID == tab.ID)
			return nIdx;
		iter ++;
	}
	return -1;
}

int TabBarClass::GetNextTab(BOOL abForward)
{
    int nCurSel = GetCurSel();
    int nCurCount = GetItemCount();
    VConTabs cur; cur.ID = 0;
    
    _ASSERTE(nCurCount == m_Tab2VCon.size());
    if (nCurCount < 1)
    	return 0; // хотя такого и не должно быть
    
    if (gSet.isTabRecent && nCurSel >= 0 && (UINT)nCurSel < m_Tab2VCon.size())
        cur = m_Tab2VCon[nCurSel];
    
    
    int i, nNewSel = -1;

    TODO("Добавить возможность переключаться а'ля RecentScreens");
    if (abForward) {
    	if (gSet.isTabRecent) {
        	std::vector<VConTabs>::iterator iter = m_TabStack.begin();
        	while (iter != m_TabStack.end()) {
        		// Найти в стеке выделенный таб
        		if (iter->ID == cur.ID) {
        			// Определить следующий таб, который мы можем активировать
        			do {
	        			iter ++; // Если дошли до конца (сейчас выделен последний таб) вернуть первый
    	    			if (iter == m_TabStack.end()) iter = m_TabStack.begin();
    	    			// Определить индекс в m_Tab2VCon
    	    			i = GetIndexByTab ( *iter );
    	    			if (CanActivateTab(i)) {
    	    				return i;
        				}
        			} while (iter->ID != cur.ID);
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
    
    	if (gSet.isTabRecent) {
        	std::vector<VConTabs>::reverse_iterator iter = m_TabStack.rbegin();
        	while (iter != m_TabStack.rend()) {
        		// Найти в стеке выделенный таб
        		if (iter->ID == cur.ID) {
        			// Определить следующий таб, который мы можем активировать
        			do {
	        			iter ++; // Если дошли до конца (сейчас выделен последний таб) вернуть первый
    	    			if (iter == m_TabStack.rend()) iter = m_TabStack.rbegin();
    	    			// Определить индекс в m_Tab2VCon
    	    			i = GetIndexByTab ( *iter );
    	    			if (CanActivateTab(i)) {
    	    				return i;
        				}
        			} while (iter->ID != cur.ID);
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

void TabBarClass::SwitchNext()
{
	Switch(TRUE);
}

void TabBarClass::SwitchPrev()
{
	Switch(FALSE);
}

void TabBarClass::Switch(BOOL abForward)
{
    int nNewSel = GetNextTab ( abForward );
    
    if (nNewSel != -1) {
        if (gSet.isTabLazy && mh_Tabbar) {
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
			if (i->ID == j->ID) {
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
			if (i->ID == j->ID) {
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
			if (iter->ID == tab.ID) {
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

    if (messg == WM_KEYDOWN && wParam == VK_TAB)
    {
        if (!isPressed(VK_SHIFT))
            SwitchNext();
        else
            SwitchPrev();
    } else if (mb_InKeySwitching && messg == WM_KEYDOWN
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
