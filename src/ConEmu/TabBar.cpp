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
    _tabHeight = NULL; memset(&m_Margins, 0, sizeof(m_Margins));
    _titleShouldChange = false;
    _prevTab = -1;
    mb_ChangeAllowed = FALSE;
    mb_Enabled = TRUE;
    mh_ConmanToolbar = NULL; mh_Tabbar = NULL; mh_Rebar = NULL; mn_LastToolbarWidth = 0;
    mb_PostUpdateCalled = FALSE;
    //GetConsolesTitles = NULL;
    //ActivateConsole = NULL;
    //ConMan_KeyAction = NULL;
    mn_MsgUpdateTabs = RegisterWindowMessage(CONEMUMSG_UPDATETABS);
    memset(&m_Tab4Tip, 0, sizeof(m_Tab4Tip));
    mb_InKeySwitching = FALSE;
    ms_TmpTabText[0] = 0;
	mn_CurSelTab = 0;
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


bool TabBarClass::IsActive()
{
    return _active;
}

bool TabBarClass::IsShown()
{
    return _active && IsWindowVisible(mh_Tabbar);
}

/*int TabBarClass::Height()
{
    return _tabHeight;
}*/

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
}

void TabBarClass::Deactivate()
{
    if (!_active /*|| !_tabHeight*/)
        return;

    _tabHeight = 0; memset(&m_Margins, 0, sizeof(m_Margins));
    UpdatePosition();
    _active = false;
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
    VConTabs vct = {NULL};

    // Выполняться должно только в основной нити, так что CriticalSection не нужна
    m_Tab2VCon.clear();
    
    for (V = 0; V < MAX_CONSOLE_COUNT; V++) {
        CVirtualConsole* pVCon = gConEmu.GetVCon(V);
        if (!pVCon) continue;
        
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

    // удалить лишние закладки
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

    
    if (_tabHeight && tabIdx==1 && tab.Type == 1/*WTYPE_PANELS*/ && gSet.isTabs==2) {
        // Автоскрытие табов (все редакторы/вьюверы закрыты)
        Deactivate();
    } else
    if (mh_Rebar) {
        RECT rcWnd; GetWindowRect(mh_Rebar, &rcWnd);
        m_Margins.top = rcWnd.bottom - rcWnd.top;
        m_Margins.left = 0;
        m_Margins.right = 0;
        m_Margins.bottom = 0;
    } else
    if (_tabHeight == NULL && mh_Tabbar)
    {
        RECT rcClient, rcWnd;
        GetClientRect(ghWnd, &rcClient); 
        rcWnd = rcClient;
        TabCtrl_AdjustRect(mh_Tabbar, FALSE, &rcClient);
        _tabHeight = rcClient.top;

        m_Margins.top = rcClient.top;
        gSet.UpdateMargins(m_Margins);

        UpdatePosition();
    }
}


RECT TabBarClass::GetMargins()
{
    return m_Margins;
}

void TabBarClass::UpdatePosition()
{
    if (!_active)
    {
        return;
    }

    gConEmu.ReSize();

    RECT client; //, self;
    GetClientRect(ghWnd, &client);
    //GetWindowRect(mh_Tabbar, &self);
    
    if (_tabHeight>0) {
        if (mh_Rebar) {
            if (!IsWindowVisible(mh_Rebar))
                ShowWindow(mh_Rebar, SW_SHOW);
            MoveWindow(mh_Rebar, 0, 0, client.right, _tabHeight, 1);
        } else {
            if (!IsWindowVisible(mh_Tabbar))
                ShowWindow(mh_Tabbar, SW_SHOW);
            if (gSet.isTabFrame)
                MoveWindow(mh_Tabbar, 0, 0, client.right, client.bottom, 1);
            else
                MoveWindow(mh_Tabbar, 0, 0, client.right, _tabHeight, 1);
        }
    } else {
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
        MoveWindow(mh_Rebar, 0, 0, client.right, _tabHeight, 1);
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
    if (mh_ConmanToolbar) {
        SIZE sz; 
        SendMessage(mh_ConmanToolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz);
        if (mh_Rebar) {
            if (sz.cx > mn_LastToolbarWidth)
            {
                REBARBANDINFO rbBand={80}; // не используем size, т.к. приходит "новый" размер из висты и в XP обламываемся
                rbBand.fMask  = RBBIM_SIZE | RBBIM_CHILDSIZE;
                // Set values unique to the band with the toolbar.
                rbBand.cx = rbBand.cxMinChild = rbBand.cxIdeal = mn_LastToolbarWidth = sz.cx;
                rbBand.cyMinChild = sz.cy;

                // Add the band that has the toolbar.
                if (SendMessage(mh_Rebar, RB_SETBANDINFO, 1, (LPARAM)&rbBand)) {
                }
            }
        } else {
            RECT rcClient;
            GetWindowRect(mh_Tabbar, &rcClient);
            MapWindowPoints(NULL, ghWnd, (LPPOINT)&rcClient, 2);
            //int nbWidth = SendMessage(mh_ConmanToolbar, TB_GETBUTTONSIZE, 0, 0) & 0xFFFF;
            //int nHidden = 10;
            /*SetWindowPos(mh_ConmanToolbarParent, HWND_TOP, 
               rcClient.right - sz.cx - 2, 0,
               sz.cx, sz.cy, 0);*/
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

    if (nmhdr->code == TBN_GETINFOTIP /*&& nmhdr->hwndFrom == mh_ConmanToolbar*/)
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
	        BOOL lbPressed = (SendMessage(mh_ConmanToolbar, TB_GETSTATE, pDisp->iItem, 0) & TBSTATE_CHECKED) == TBSTATE_CHECKED;
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
    if (mh_ConmanToolbar != (HWND)lParam)
        return;
    //if (!gConEmu.isConman() || !gConEmu.mh_ConMan || gConEmu.mh_ConMan==INVALID_HANDLE_VALUE)
    //  return;
    if (!gSet.isMulti)
        return;

    if (wParam>=1 && wParam<=MAX_CONSOLE_COUNT) {
        gConEmu.ConmanAction(wParam-1);
    } else if (wParam==13) {
        gConEmu.ConmanAction(CONMAN_NEWCONSOLE);
    } else if (wParam==14) {
        //gConEmu.ConmanAction(CONMAN_ALTCONSOLE); // Только информационно!
		SendMessage(mh_ConmanToolbar, TB_CHECKBUTTON, 14, gConEmu.ActiveCon()->RCon()->isBufferHeight());
    }

    //if (!TabBar.GetConsolesTitles)
    //  TabBar.GetConsolesTitles =
    //      (GetConsolesTitles_t*)GetProcAddress( gConEmu.mh_Infis, "GetConsolesTitles" );
    //if (!TabBar.ActivateConsole)
    //  TabBar.ActivateConsole =
    //      (ActivateConsole_t*)GetProcAddress( gConEmu.mh_Infis, "ActivateConsole" );
    /*if (!TabBar.ConMan_KeyAction)
        TabBar.ConMan_KeyAction = (ConMan_KeyAction_t)GetProcAddress( gConEmu.mh_ConMan, "_KeyAction_" );
    if (!TabBar.ConMan_KeyAction)
        return;

    RegShortcut cmd; memset(&cmd, 0, sizeof(cmd));
    if (wParam>=1 && wParam<=MAX_CONSOLE_COUNT)
    {
        // активировать консоль №
        cmd.action = wParam - 1;
        //gConEmu.mb_IgnoreSizeChange = true;
        //CVirtualConsole* pCon = gConEmu.ActiveCon();
        //if (pCon) {
        //  COORD sz = {pCon->TextWidth, pCon->TextHeight};
            TabBar.ConMan_KeyAction ( &cmd );
            //// Установить размер консоли!
            //gConEmu.SetConsoleWindowSize(sz, false);
            //gConEmu.mb_IgnoreSizeChange = false;
        //}
    } else
    if (wParam==13)
    {
        // Создать новую консоль
        cmd.action = 20;
        TabBar.ConMan_KeyAction ( &cmd );
    } else
    if (wParam==14)
    {
        // переключение между альтернативной консолью
        cmd.action = 15;
        TabBar.ConMan_KeyAction ( &cmd );
    }*/
}

void TabBarClass::OnMouse(int message, int x, int y)
{
    if (!_active)
    {
        return;
    }

    //SetFocus(ghWnd);
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
				pVCon->RCon()->OnKeyboard(ghWnd, WM_KEYDOWN, VK_F10, 0);
				pVCon->RCon()->OnKeyboard(ghWnd, WM_KEYUP, VK_F10, (LPARAM)(3<<30));
			}
        }
    }
}

void TabBarClass::Invalidate()
{
    if (TabBar.IsActive())
        InvalidateRect(mh_Rebar, NULL, TRUE);
}

void TabBarClass::OnConsoleActivated(int nConNumber/*, BOOL bAlternative*/)
{
    if (!mh_ConmanToolbar) return;

    BOOL bPresent[MAX_CONSOLE_COUNT]; memset(bPresent, 0, sizeof(bPresent));
    MCHKHEAP
    for (int i=1; i<=MAX_CONSOLE_COUNT; i++) {
        bPresent[i-1] = gConEmu.GetTitle(i-1) != NULL;
    }

    SendMessage(mh_ConmanToolbar, WM_SETREDRAW, 0, 0);
    for (int i=1; i<=MAX_CONSOLE_COUNT; i++) {
        SendMessage(mh_ConmanToolbar, TB_HIDEBUTTON, i, !bPresent[i-1]);
    }

    UpdateToolbarPos();
    SendMessage(mh_ConmanToolbar, WM_SETREDRAW, 1, 0);

    //nConNumber = gConEmu.ActiveConNum()+1; -- сюда пришел уже правильный номер!
    
    if (nConNumber>=1 && nConNumber<=MAX_CONSOLE_COUNT) {
        SendMessage(mh_ConmanToolbar, TB_CHECKBUTTON, nConNumber, 1);
    } else {
        for (int i=1; i<=MAX_CONSOLE_COUNT; i++)
            SendMessage(mh_ConmanToolbar, TB_CHECKBUTTON, i, 0);
    }
}

void TabBarClass::OnBufferHeight(BOOL abBufferHeight)
{
	if (!mh_ConmanToolbar) return;

    SendMessage(mh_ConmanToolbar, TB_CHECKBUTTON, 14, abBufferHeight);
}

HWND TabBarClass::CreateToolbar()
{
    if (!mh_Rebar || !gSet.isMulti)
        return NULL; // нет табов - нет и тулбара
    if (mh_ConmanToolbar)
        return mh_ConmanToolbar; // Уже создали

    /*mh_ConmanToolbarP = CreateWindow(_T("VirtualConsoleClassBar"), _T(""), 
            WS_VISIBLE|WS_CHILD, 0,0,340,22, ghWnd, 0, 0, 0);
    if (!mh_ConmanToolbarP) return NULL;*/

    
// Create a toolbar. 
    TBBUTTON buttons[16] = {
        {0, 1, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP},
        {1, 2, TBSTATE_ENABLED|TBSTATE_HIDDEN, TBSTYLE_CHECKGROUP},
        {2, 3, TBSTATE_ENABLED|TBSTATE_HIDDEN, TBSTYLE_CHECKGROUP/*|TBSTYLE_TRANSPARENT*/},
        {3, 4, TBSTATE_ENABLED|TBSTATE_HIDDEN, TBSTYLE_CHECKGROUP/*|TBSTYLE_TRANSPARENT*/},
        {4, 5, TBSTATE_ENABLED|TBSTATE_HIDDEN, TBSTYLE_CHECKGROUP/*|TBSTYLE_TRANSPARENT*/},
        {5, 6, TBSTATE_ENABLED|TBSTATE_HIDDEN, TBSTYLE_CHECKGROUP/*|TBSTYLE_TRANSPARENT*/},
        {6, 7, TBSTATE_ENABLED|TBSTATE_HIDDEN, TBSTYLE_CHECKGROUP/*|TBSTYLE_TRANSPARENT*/},
        {7, 8, TBSTATE_ENABLED|TBSTATE_HIDDEN, TBSTYLE_CHECKGROUP/*|TBSTYLE_TRANSPARENT*/},
        {8, 9, TBSTATE_ENABLED|TBSTATE_HIDDEN, TBSTYLE_CHECKGROUP/*|TBSTYLE_TRANSPARENT*/},
        {9, 10, TBSTATE_ENABLED|TBSTATE_HIDDEN, TBSTYLE_CHECKGROUP/*|TBSTYLE_TRANSPARENT*/},
        {10, 11, TBSTATE_ENABLED|TBSTATE_HIDDEN, TBSTYLE_CHECKGROUP/*|TBSTYLE_TRANSPARENT*/},
        {11, 12, TBSTATE_ENABLED|TBSTATE_HIDDEN, TBSTYLE_CHECKGROUP/*|TBSTYLE_TRANSPARENT*/},
        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
        {12, 13, TBSTATE_ENABLED, BTNS_BUTTON},
        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
        {13, 14, TBSTATE_ENABLED, TBSTYLE_CHECK}
        };

    mh_ConmanToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, 
        WS_CHILD|WS_VISIBLE|TBSTYLE_FLAT|CCS_NOPARENTALIGN|CCS_NORESIZE|CCS_NODIVIDER|TBSTYLE_TOOLTIPS|TBSTYLE_TRANSPARENT, 0, 0, 0, 0, mh_Rebar, 
        NULL, NULL, NULL); 
 
   SendMessage(mh_ConmanToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0); 
   SendMessage(mh_ConmanToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(16,16)); 
   TBADDBITMAP bmp = {g_hInstance,IDB_CONMAN1};
   int nFirst = SendMessage(mh_ConmanToolbar, TB_ADDBITMAP, 14, (LPARAM)&bmp);
   //buttons
   SendMessage(mh_ConmanToolbar, TB_ADDBUTTONS, 16, (LPARAM)&buttons);

   SendMessage(mh_ConmanToolbar, TB_AUTOSIZE, 0, 0); 
   SIZE sz; 
   SendMessage(mh_ConmanToolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz);
   /*RECT rcClient;
   GetWindowRect(mh_Tabbar, &rcClient);
   MapWindowPoints(NULL, ghWnd, (LPPOINT)&rcClient, 2);
   //int nbWidth = SendMessage(mh_ConmanToolbar, TB_GETBUTTONSIZE, 0, 0) & 0xFFFF;
   //int nHidden = 10;
   SetWindowPos(mh_ConmanToolbarParent, HWND_TOP, 
       rcClient.right - sz.cx - 2, 0,
       sz.cx, sz.cy, 0);*/

   /*if (gConEmu.ConMan_ProcessCommand)
       OnConman(0, FALSE);*/

   //ShowWindow(hwndTB, SW_SHOW); 
   return mh_ConmanToolbar;
}

HWND TabBarClass::CreateTabbar()
{
    if (!mh_Rebar)
        return NULL; // нет табов - нет и тулбара
    if (mh_Tabbar)
        return mh_Tabbar; // Уже создали
    
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

        // Надо
        #pragma warning (disable : 4312)
        _defaultTabProc = (WNDPROC)SetWindowLongPtr(mh_Tabbar, GWL_WNDPROC, (LONG_PTR)TabProc);

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
 
// Create a toolbar. 
    /*
    TBBUTTON buttons[3] = {
        {0, 1001, TBSTATE_ENABLED|TBSTATE_CHECKED|TBSTATE_MARKED, TBSTYLE_CHECKGROUP},
        {0, 1002, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP},
        {0, 0, TBSTATE_ENABLED, BTNS_SEP}
        };

    mh_Tabbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, 
        WS_CHILD|WS_VISIBLE|CCS_NOPARENTALIGN|CCS_NORESIZE|CCS_NODIVIDER|TBSTYLE_TOOLTIPS|TBSTYLE_LIST, 0, 0, 0, 0, mh_Rebar, 
        NULL, NULL, NULL); 
 
    HFONT hFont = CreateFont(TAB_FONT_HEIGTH, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, 
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TAB_FONT_FACE);
    SendMessage(mh_Tabbar, WM_SETFONT, WPARAM (hFont), TRUE);

   SendMessage(mh_Tabbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0); 
   SendMessage(mh_Tabbar, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_HIDECLIPPEDBUTTONS);

   SendMessage(mh_Tabbar, TB_SETBITMAPSIZE, 0, MAKELONG(16,16)); 
   SendMessage(mh_Tabbar, TB_SETBUTTONSIZE, 0, MAKELONG(200,16));
   //SendMessage(mh_Tabbar, TB_SETPADDING, 0, MAKELONG(4,4));
   //SendMessage(mh_Tabbar, TB_SETINDENT, 4, 0);
   SendMessage(mh_Tabbar, TB_SETBUTTONWIDTH, 0, MAKELONG(100,200));
   DWORD nStyle = DT_NOPREFIX|DT_CENTER|DT_VCENTER|DT_PATH_ELLIPSIS;
   SendMessage(mh_Tabbar, TB_SETDRAWTEXTFLAGS, DT_CENTER, DT_CENTER);
   TBMETRICS tbm = {sizeof(TBMETRICS)};
   tbm.dwMask = TBMF_PAD|TBMF_BUTTONSPACING;
   tbm.cxPad = 4; tbm.cyPad = 4;
   tbm.cxButtonSpacing = 6; tbm.cyButtonSpacing = 4;
   SendMessage(mh_Tabbar, TB_SETMETRICS, 0, (LPARAM)&tbm);



   

   buttons[0].iString = (INT_PTR)L"Console";
   buttons[0].fsStyle = BTNS_CHECKGROUP|BTNS_AUTOSIZE|BTNS_NOPREFIX|BTNS_SHOWTEXT;
   buttons[0].iBitmap = I_IMAGENONE;
   buttons[1].iString = (INT_PTR)L"Panels";
   buttons[1].fsStyle = BTNS_CHECKGROUP|BTNS_AUTOSIZE|BTNS_NOPREFIX|BTNS_SHOWTEXT;
   buttons[1].iBitmap = I_IMAGENONE;

   SendMessage(mh_Tabbar, TB_ADDBUTTONS, 2, (LPARAM)&buttons);

   SendMessage(mh_Tabbar, TB_AUTOSIZE, 0, 0); */

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
    rbBand.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDSIZE;
    rbBand.clrBack = GetSysColor(COLOR_BTNFACE);
    rbBand.clrFore = GetSysColor(COLOR_BTNTEXT);


    SendMessage(mh_Rebar, RB_SETBKCOLOR, 0, GetSysColor(COLOR_BTNFACE));
    SendMessage(mh_Rebar, RB_SETWINDOWTHEME, 0, (LPARAM)L" ");


    CreateTabbar();
    CreateToolbar();

    SIZE sz = {0,0};
    if (mh_ConmanToolbar) {
        SendMessage(mh_ConmanToolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz);
    } else {
        RECT rcClient;
        GetClientRect(ghWnd, &rcClient); 
        TabCtrl_AdjustRect(mh_Tabbar, FALSE, &rcClient);
        sz.cy = rcClient.top - 3;
    }


    if (mh_Tabbar)
    {
        // Set values unique to the band with the toolbar.
        rbBand.wID          = 1;
        rbBand.hwndChild  = mh_Tabbar;
        rbBand.cxMinChild = 100;
        rbBand.cx = rbBand.cxIdeal = rcWnd.right - sz.cx - 80;
        rbBand.cyMinChild = sz.cy;

        if (!SendMessage(mh_Rebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand)) {
            DisplayLastError(_T("Can't initialize rebar (tabbar)"));
        }
    }


    if (mh_ConmanToolbar)
    {
        // Set values unique to the band with the toolbar.
        rbBand.wID        = 2;
        rbBand.hwndChild  = mh_ConmanToolbar;
        rbBand.cx = rbBand.cxMinChild = rbBand.cxIdeal = mn_LastToolbarWidth = sz.cx;
        rbBand.cyMinChild = sz.cy;

        // Add the band that has the toolbar.
        if (!SendMessage(mh_Rebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand)) {
            DisplayLastError(_T("Can't initialize rebar (toolbar)"));
        }
    }

    RECT rc;
    GetWindowRect(mh_Rebar, &rc);


    //GetWindowRect(mh_Rebar, &rc);
    m_Margins = MakeRect(0,rc.bottom-rc.top,0,0);
    gSet.UpdateMargins(m_Margins);

    //_hwndTab = mh_Rebar; // пока...
}

void TabBarClass::PrepareTab(ConEmuTab* pTab)
{
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

int TabBarClass::GetNextTab(BOOL abForward)
{
    int nCurSel = GetCurSel();
    int nCurCount = GetItemCount();
    
    int i, nNewSel = -1;

    TODO("Добавить возможность переключаться а'ля RecentScreens");
    if (abForward) {    
        if (!gSet.isTabRecent) {
            for (i = nCurSel+1; nNewSel == -1 && i < nCurCount; i++)
                if (CanActivateTab(i)) nNewSel = i;
            
            for (i = 0; nNewSel == -1 && i < nCurSel; i++)
                if (CanActivateTab(i)) nNewSel = i;
        } else {
            TODO("...");
        }
    } else {
        if (!gSet.isTabRecent) {
            for (i = nCurSel-1; nNewSel == -1 && i >= 0; i++)
                if (CanActivateTab(i)) nNewSel = i;
            
            for (i = nCurCount-1; nNewSel == -1 && i > nCurSel; i++)
                if (CanActivateTab(i)) nNewSel = i;
        } else {
            TODO("...");
        }
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

    if (messg == WM_KEYDOWN && wParam == VK_TAB) {
        if (!isPressed(VK_SHIFT))
            SwitchNext();
        else
            SwitchPrev();
    }
        
    return TRUE;
}
