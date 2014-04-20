
/*
Copyright (c) 2009-2014 Maximus5
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
#include <commctrl.h>
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

WARNING("!!! Запустили far, открыли edit, перешли в панель, открыли второй edit, ESC, ни одна вкладка не активна");
// Более того, если есть еще одна консоль - активной станет первая вкладка следующей НЕАКТИВНОЙ консоли
WARNING("не меняются табы при переключении на другую консоль");

TODO("Для WinXP можно поиграться стилем WS_EX_COMPOSITED");

WARNING("isTabFrame на удаление");

#ifndef TBN_GETINFOTIP
#define TBN_GETINFOTIP TBN_GETINFOTIPW
#endif

#ifndef RB_SETWINDOWTHEME
#define CCM_SETWINDOWTHEME      (CCM_FIRST + 0xb)
#define RB_SETWINDOWTHEME       CCM_SETWINDOWTHEME
#endif

WARNING("TB_GETIDEALSIZE - awailable on XP only, use insted TB_GETMAXSIZE");
#ifndef TB_GETIDEALSIZE
#define TB_GETIDEALSIZE         (WM_USER + 99)
#endif

//enum ToolbarMainBitmapIdx
//{
//	BID_FIST_CON = 0,
//	BID_LAST_CON = (MAX_CONSOLE_COUNT-1),
//	BID_NEWCON_IDX,
//	BID_ALTERNATIVE_IDX,
//	BID_MINIMIZE_IDX,
//	BID_MAXIMIZE_IDX,
//	BID_RESTORE_IDX,
//	BID_APPCLOSE_IDX,
//	BID_DUMMYBTN_IDX,
//	BID_TOOLBAR_LAST_IDX,
//};

//typedef long (WINAPI* ThemeFunction_t)();

CTabBarClass::CTabBarClass()
{
	_active = false;
	_visible = false;
	_tabHeight = 0;
	mn_CurSelTab = 0;
	mb_ForceRecalcHeight = false;
	mb_DisableRedraw = FALSE;
	//memset(&m_Margins, 0, sizeof(m_Margins));
	//m_Margins = gpSet->rcTabMargins; // !! Значения отступов
	//_titleShouldChange = false;
	//mb_Enabled = TRUE;
	mb_PostUpdateCalled = FALSE;
	mb_PostUpdateRequested = FALSE;
	mn_PostUpdateTick = 0;
	//mn_MsgUpdateTabs = RegisterWindowMessage(CONEMUMSG_UPDATETABS);
	memset(&m_Tab4Tip, 0, sizeof(m_Tab4Tip));
	mb_InKeySwitching = FALSE;
	ms_TmpTabText[0] = 0;
	mn_InUpdate = 0;

	mp_Rebar = new CTabPanelWin(this);
}

CTabBarClass::~CTabBarClass()
{
	SafeDelete(mp_Rebar);

	// Освободить все табы
	m_Tabs.ReleaseTabs(false);

	//m_TabStack.ReleaseTabs(false);
	_ASSERTE(gpConEmu->isMainThread());
	for (INT_PTR i = 0; i < m_TabStack.size(); i++)
	{
		m_TabStack[i]->Release();
	}
	m_TabStack.clear();
}

void CTabBarClass::RePaint()
{
	mp_Rebar->RePaintInt();
}

//void CTabBarClass::Refresh(BOOL abFarActive)
//{
//    Enable(abFarActive);
//}

void CTabBarClass::Reset()
{
	if (!_active)
	{
		return;
	}

	/*ConEmuTab tab; memset(&tab, 0, sizeof(tab));
	tab.Pos=0;
	tab.Current=1;
	tab.Type = 1;*/
	//gpConEmu->mp_TabBar->Update(&tab, 1);
	Update();
}

void CTabBarClass::Retrieve()
{
	if (gpSet->isTabs == 0)
		return; // если табов нет ВООБЩЕ - и читать ничего не нужно

	if (!CVConGroup::isFar())
	{
		Reset();
		return;
	}

	TODO("Retrieve() может нужно выполнить в RCon?");
	//CConEmuPipe pipe;
	//if (pipe.Init(_T("CTabBarClass::Retrieve"), TRUE))
	//{
	//  DWORD cbWritten=0;
	//  if (pipe.Execute(CMD_REQTABS))
	//  {
	//      gpConEmu->DebugStep(_T("Tabs: Checking for plugin (1 sec)"));
	//      // Подождем немножко, проверим что плагин живой
	//      cbWritten = WaitForSingleObject(pipe.hEventAlive, CONEMUALIVETIMEOUT);
	//      if (cbWritten!=WAIT_OBJECT_0) {
	//          TCHAR szErr[MAX_PATH];
	//          _wsprintf(szErr, countof(szErr), _T("ConEmu plugin is not active!\r\nProcessID=%i"), pipe.nPID);
	//          MBoxA(szErr);
	//      } else {
	//          gpConEmu->DebugStep(_T("Tabs: Waiting for result (10 sec)"));
	//          cbWritten = WaitForSingleObject(pipe.hEventReady, CONEMUREADYTIMEOUT);
	//          if (cbWritten!=WAIT_OBJECT_0) {
	//              TCHAR szErr[MAX_PATH];
	//              _wsprintf(szErr, countof(szErr), _T("Command waiting time exceeds!\r\nConEmu plugin is locked?\r\nProcessID=%i"), pipe.nPID);
	//              MBoxA(szErr);
	//          } else {
	//              gpConEmu->DebugStep(_T("Tabs: Recieving data"));
	//              DWORD cbBytesRead=0;
	//              int nTabCount=0;
	//              pipe.Read(&nTabCount, sizeof(nTabCount), &cbBytesRead);
	//              if (nTabCount<=0) {
	//                  gpConEmu->DebugStep(_T("Tabs: data empty"));
	//                  this->Reset();
	//              } else {
	//                  COPYDATASTRUCT cds = {0};
	//
	//                  cds.dwData = nTabCount;
	//                  cds.lpData = pipe.GetPtr(); // хвост
	//                  gpConEmu->OnCopyData(&cds);
	//                  gpConEmu->DebugStep(NULL);
	//              }
	//          }
	//      }
	//  }
	//}
}

int CTabBarClass::CreateTabIcon(LPCWSTR asIconDescr, bool bAdmin, LPCWSTR asWorkDir)
{
	if (!gpSet->isTabIcons)
		return -1;
	return m_TabIcons.CreateTabIcon(asIconDescr, bAdmin, asWorkDir);
}

// Добавляет закладку, или меняет (при необходимости) заголовок существующей
void CTabBarClass::AddTab(LPCWSTR text, int i, bool bAdmin, int iTabIcon)
{
	if (mh_Tabbar)
	{
		TCITEM tie;
		// иконку обновляем всегда. она может измениться для таба
		tie.mask = TCIF_TEXT | (m_TabIcons.IsInitialized() ? TCIF_IMAGE : 0);
		tie.pszText = (LPWSTR)text ;
		tie.iImage = (iTabIcon > 0) ? iTabIcon : m_TabIcons.GetTabIcon(bAdmin); // Пока иконка только одна - для табов со щитом
		int nCurCount = GetItemCount();

		if (i>=nCurCount)
		{
			TabCtrl_InsertItem(mh_Tabbar, i, &tie);
		}
		else
		{
			// Проверим, изменился ли текст
			if (!wcscmp(GetTabText(i), text))
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
}

HIMAGELIST CTabBarClass::GetTabIcons()
{
	if (!m_TabIcons.IsInitialized())
		return NULL;
	return (HIMAGELIST)m_TabIcons;
}

int CTabBarClass::GetTabIcon(bool bAdmin)
{
	return m_TabIcons.GetTabIcon(bAdmin);
}

void CTabBarClass::SelectTab(int i)
{
	mn_CurSelTab = mp_Rebar->SelectTabInt(i); // Меняем выделение, только если оно реально меняется
}

int CTabBarClass::GetCurSel()
{
	mn_CurSelTab = mp_Rebar->GetCurSelInt();
	return mn_CurSelTab;
}

int CTabBarClass::GetItemCount()
{
	int nCurCount = 0;

	TODO("Здесь вообще не нужно обращаться к mp_Rebar, табы должны храниться здесь");
	if (mp_Rebar->IsTabbarCreated())
		nCurCount = mp_Rebar->GetItemCountInt();
	else
		nCurCount = m_Tabs.GetCount();

	return nCurCount;
}

int CTabBarClass::CountActiveTabs(int nMax /*= 0*/)
{
	int  nTabs = 0;
	bool bHideInactiveConsoleTabs = gpSet->bHideInactiveConsoleTabs;

	for (int V = 0; V < MAX_CONSOLE_COUNT; V++)
	{
		CVConGuard guard;
		if (!CVConGroup::GetVCon(V, &guard))
			continue;
		CVirtualConsole* pVCon = guard.VCon();

		if (bHideInactiveConsoleTabs)
		{
			if (!gpConEmu->isActive(pVCon))
				continue;
		}

		nTabs += pVCon->RCon()->GetTabCount(TRUE);

		if ((nMax > 0) && (nTabs >= nMax))
			break;
	}

	return nTabs;
}

void CTabBarClass::DeleteItem(int I)
{
	if (mp_Rebar->IsTabbarCreated())
	{
		mp_Rebar->DeleteItemInt(I);
	}
}


/*char CTabBarClass::FarTabShortcut(int tabIndex)
{
    return tabIndex < 10 ? '0' + tabIndex : 'A' + tabIndex - 10;
}*/

bool CTabBarClass::NeedPostUpdate()
{
	return (mb_PostUpdateCalled || mb_PostUpdateRequested);
}

void CTabBarClass::RequestPostUpdate()
{
	if (mb_PostUpdateCalled)
	{
		DWORD nDelta = GetTickCount() - mn_PostUpdateTick;

		// Может так получиться, что флажок остался, а Post не вызвался
		if (nDelta <= POST_UPDATE_TIMEOUT)
			return; // Уже
	}

	if (mn_InUpdate > 0)
	{
		mb_PostUpdateRequested = TRUE;
		DEBUGSTRTABS(L"   PostRequesting CTabBarClass::Update\n");
	}
	else
	{
		mb_PostUpdateCalled = TRUE;
		DEBUGSTRTABS(L"   Posting CTabBarClass::Update\n");
		//PostMessage(ghWnd, mn_MsgUpdateTabs, 0, 0);
		gpConEmu->RequestPostUpdateTabs();
		mn_PostUpdateTick = GetTickCount();
	}
}

bool CTabBarClass::GetVConFromTab(int nTabIdx, CVConGuard* rpVCon, DWORD* rpWndIndex)
{
	bool lbRc = false;
	CTab tab;
	CVConGuard VCon;
	DWORD wndIndex = 0;

	if (m_Tabs.GetTabByIndex(nTabIdx, tab))
	{
		wndIndex = tab->Info.nFarWindowID;

		if (!gpConEmu->isValid((CVirtualConsole*)tab->Info.pVCon))
		{
			RequestPostUpdate();
		}
		else
		{
			VCon = (CVirtualConsole*)tab->Info.pVCon;
			lbRc = true;
		}
	}

	if (rpVCon)
		rpVCon->Attach(VCon.VCon());

	if (rpWndIndex)
		*rpWndIndex = lbRc ? wndIndex : 0;

	return lbRc;
}

CVirtualConsole* CTabBarClass::FarSendChangeTab(int tabIndex)
{
	CVirtualConsole *pVCon = NULL;
	DWORD wndIndex = 0;
	BOOL  bNeedActivate = FALSE, bChangeOk = FALSE;
	ShowTabError(NULL);

	if (!GetVConFromTab(tabIndex, &pVCon, &wndIndex))
	{
		if (mb_InKeySwitching) Update();  // показать реальное положение дел

		return NULL;
	}

	if (!gpConEmu->isActive(pVCon, false))
		bNeedActivate = TRUE;

	DWORD nCallStart = TimeGetTime(), nCallEnd = 0;

	bChangeOk = pVCon->RCon()->ActivateFarWindow(wndIndex);

	if (!bChangeOk)
	{
		// Всплыть тултип с руганью - не смогли активировать
		ShowTabError(L"This tab can't be activated now!", tabIndex);
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
			if (mb_InKeySwitching) Update();  // показать реальное положение дел

			TODO("А текущий таб не слетит, если активировать не удалось?");
			return NULL;
		}
	}

	if (!bChangeOk)
	{
		pVCon = NULL;

		if (mb_InKeySwitching) Update();  // показать реальное положение дел
	}

	UNREFERENCED_PARAMETER(nCallStart);
	UNREFERENCED_PARAMETER(nCallEnd);

	return pVCon;
}

LRESULT CTabBarClass::TabHitTest(bool abForce /*= false*/, int* pnOverTabHit /*= NULL*/)
{
	if (pnOverTabHit)
		*pnOverTabHit = -1; // reset, mean "not clicked overtab"

	if (gpSet->isTabs && (abForce || gpSet->isCaptionHidden()))
	{
		if (gpConEmu->mp_TabBar->IsTabsShown())
		{
			TCHITTESTINFO tch = {{0,0}};
			GetCursorPos(&tch.pt); // Screen coordinates

			HWND hTabBar = gpConEmu->mp_TabBar->mh_Tabbar;
			HWND hReBar = gpConEmu->mp_TabBar->mh_Rebar;
			RECT rcWnd; GetWindowRect(hTabBar, &rcWnd);
			RECT rcBar; GetWindowRect(hReBar, &rcBar);

			// 130911 - Let allow switching tabs by clicking over tabs headers (may be useful with fullscreen)
			RECT rcFull = {rcWnd.left, rcBar.top, rcWnd.right, rcWnd.bottom};
			if (PtInRect(&rcFull, tch.pt))
			{
				tch.pt.x -= rcWnd.left;
				tch.pt.y -= rcWnd.top;

				LRESULT nTest = SendMessage(hTabBar, TCM_HITTEST, 0, (LPARAM)&tch);

				if (nTest == -1)
				{
					// Clicked over tab?
					tch.pt.y = (rcWnd.bottom - rcWnd.top)/2;
					nTest = SendMessage(hTabBar, TCM_HITTEST, 0, (LPARAM)&tch);
					if ((nTest >= 0) && pnOverTabHit)
					{
						*pnOverTabHit = (int)nTest;
					}
				}

				if (nTest == -1)
					return HTCAPTION;
			}
		}
	}

	return HTNOWHERE;
}

LRESULT CALLBACK CTabBarClass::ReBarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int nOverTabHit = -1;

	switch(uMsg)
	{
		case WM_WINDOWPOSCHANGING:
		{
			LPWINDOWPOS pos = (LPWINDOWPOS)lParam;
			if (gpConEmu->mp_TabBar->_tabHeight)
			{
				pos->cy = gpConEmu->mp_TabBar->_tabHeight;
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
		case WM_LBUTTONDOWN: case WM_LBUTTONUP:
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
						int lnCurTab = gpConEmu->mp_TabBar->GetCurSel();
						if (lnCurTab != nOverTabHit)
						{
							gpConEmu->mp_TabBar->FarSendChangeTab(nOverTabHit);
							//gpConEmu->mp_TabBar->SelectTab(nOverTabHit);
						}
					}
					else if (uMsg == WM_RBUTTONUP)
					{
						gpConEmu->mp_TabBar->OnMouse(uMsg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), true);
					}
				}
			}

			break;

		case WM_MBUTTONUP:
			if (gpSet->isCaptionHidden() && gpSet->isTabs)
			{
				if ((TabHitTest(true, &nOverTabHit) != HTCAPTION) && (nOverTabHit >= 0))
				{
					gpConEmu->mp_TabBar->OnMouse(uMsg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), true);
					break;
				}
			}
			gpConEmu->RecreateAction(cra_CreateTab/*FALSE*/, gpSet->isMultiNewConfirm || isPressed(VK_SHIFT));
			break;
	}

	return CallWindowProc(_defaultReBarProc, hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CTabBarClass::TabProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_WINDOWPOSCHANGING:
		{
			if (gpConEmu->mp_TabBar->mh_Rebar)
			{
				LPWINDOWPOS pos = (LPWINDOWPOS)lParam;

				//if (gpConEmu->mp_TabBar->mb_ThemingEnabled) {
				if (gpSetCls->CheckTheming())
				{
					pos->y = 2; // иначе в Win7 он смещается в {0x0} и снизу видна некрасивая полоса
					pos->cy = gpConEmu->mp_TabBar->_tabHeight -3; // на всякий случай
				}
				else
				{
					pos->y = 1;
					pos->cy = gpConEmu->mp_TabBar->_tabHeight + 1;
				}

				return 0;
			}

			break;
		}
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		{
			gpConEmu->mp_TabBar->OnMouse(uMsg, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		}
		case WM_SETFOCUS:
		{
			gpConEmu->setFocus();
			return 0;
		}
	}

	return CallWindowProc(_defaultTabProc, hwnd, uMsg, wParam, lParam);
}

LRESULT CTabBarClass::OnTimer(WPARAM wParam)
{
	if (wParam == TIMER_FAILED_TABBAR_ID)
	{
		gpConEmu->SetKillTimer(false, TIMER_FAILED_TABBAR_ID, 0);
		SendMessage(gpConEmu->mp_TabBar->mh_Balloon, TTM_TRACKACTIVATE, FALSE, (LPARAM)&gpConEmu->mp_TabBar->tiBalloon);
		SendMessage(gpConEmu->mp_TabBar->mh_TabTip, TTM_ACTIVATE, TRUE, 0);
	}
	return 0;
}

// Window procedure for Toolbar (Multiconsole & BufferHeight)
LRESULT CALLBACK CTabBarClass::ToolProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_WINDOWPOSCHANGING:
		{
			LPWINDOWPOS pos = (LPWINDOWPOS)lParam;
			pos->y = (gpConEmu->mp_TabBar->mn_ThemeHeightDiff == 0) ? 2 : 1;

			if (gpSet->isHideCaptionAlways() && gpSet->nToolbarAddSpace > 0)
			{
				SIZE sz = {0,0};

				if (CallWindowProc(_defaultToolProc, hwnd, TB_GETIDEALSIZE, 0, (LPARAM)&sz))
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

			if (CallWindowProc(_defaultToolProc, hwnd, uMsg, wParam, lParam))
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

	return CallWindowProc(_defaultToolProc, hwnd, uMsg, wParam, lParam);
}


bool CTabBarClass::IsTabsActive()
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return false;
	}
	return _active;
}

bool CTabBarClass::IsTabsShown()
{
	if (!this)
	{
		_ASSERTE(this!=NULL);
		return false;
	}

	// Было "IsWindowVisible(mh_Tabbar)", что некорректно, т.к. оно учитывает состояние ghWnd

	// Надо использовать внутренние статусы!
	return _active && mp_Rebar->IsTabbarCreated() && _visible;
}

void CTabBarClass::Activate(BOOL abPreSyncConsole/*=FALSE*/)
{
	if (!mp_Rebar->IsCreated())
	{
		if (!m_TabIcons.IsInitialized())
		{
			m_TabIcons.Initialize();
		}
		// Создать
		mp_Rebar->CreateRebar();
		// Узнать высоту созданного
		_tabHeight = mp_Rebar->QueryTabbarHeight();
		// Передернуть
		OnCaptionHidden();
	}

	_active = true;
	if (abPreSyncConsole && (gpConEmu->WindowMode == wmNormal))
	{
		RECT rcIdeal = gpConEmu->GetIdealRect();
		CVConGroup::SyncConsoleToWindow(&rcIdeal, TRUE);
	}
	gpConEmu->OnTabbarActivated(true);
	UpdatePosition();
}

void CTabBarClass::Deactivate(BOOL abPreSyncConsole/*=FALSE*/)
{
	if (!_active)
		return;

	_active = false;
	if (abPreSyncConsole && !(gpConEmu->isZoomed() || gpConEmu->isFullScreen()))
	{
		RECT rcIdeal = gpConEmu->GetIdealRect();
		CVConGroup::SyncConsoleToWindow(&rcIdeal, true);
	}
	gpConEmu->OnTabbarActivated(false);
	UpdatePosition();
}

void CTabBarClass::Update(BOOL abPosted/*=FALSE*/)
{
	#ifdef _DEBUG
	if (this != gpConEmu->mp_TabBar)
	{
		_ASSERTE(this == gpConEmu->mp_TabBar);
	}
	#endif

	MCHKHEAP
	/*if (!_active)
	{
	    return;
	}*/ // Теперь - ВСЕГДА! т.к. сами управляем мультиконсолью

	if (mb_DisableRedraw)
	{
		_ASSERTE(mb_DisableRedraw); // Надо?
		return;
	}

	if (!gpConEmu->isMainThread())
	{
		RequestPostUpdate();
		return;
	}

	gpConEmu->mp_Status->UpdateStatusBar();

	mb_PostUpdateCalled = FALSE;

	#ifdef _DEBUG
	_ASSERTE(mn_InUpdate >= 0);
	if (mn_InUpdate > 0)
	{
		_ASSERTE(mn_InUpdate == 0);
	}
	#endif

	mn_InUpdate ++;

	//ConEmuTab tab = {0};
	MCHKHEAP
	int V, I, tabIdx = 0, nCurTab = -1, rFrom, rFound;
	BOOL bShowFarWindows = gpSet->bShowFarWindows;
	CVirtualConsole* pVCon = NULL;
	VConTabs vct = {NULL};
	// Выполняться должно только в основной нити, так что CriticalSection не нужна
	m_Tab2VCon.clear();
	_ASSERTE(m_Tab2VCon.size()==0);

	#ifdef _DEBUG
	if (this != gpConEmu->mp_TabBar)
	{
		_ASSERTE(this == gpConEmu->mp_TabBar);
	}
	#endif

	TODO("Обработка gpSet->bHideInactiveConsoleTabs для новых табов");
	MCHKHEAP


	// Check if we need to AutoSHOW or AutoHIDE tab bar
	if (!IsTabsActive() && gpSet->isTabs)
	{
		int nTabs = CountActiveTabs(2);
		if (nTabs > 1)
		{
			_ASSERTE(m_Tab2VCon.size()==0);
			Activate();
			_ASSERTE(m_Tab2VCon.size()==0);
		}
	}
	else if (IsTabsActive() && gpSet->isTabs==2)
	{
		int nTabs = CountActiveTabs(2);
		if (nTabs <= 1)
		{
			_ASSERTE(m_Tab2VCon.size()==0);
			Deactivate();
			_ASSERTE(m_Tab2VCon.size()==0);
		}
	}


    // Validation?
	#ifdef _DEBUG
	if (this != gpConEmu->mp_TabBar)
	{
		_ASSERTE(this == gpConEmu->mp_TabBar);
	}
	#endif

	MCHKHEAP
	_ASSERTE(m_Tab2VCon.size()==0);






	/* ********************* */
	/*          Go           */
	/* ********************* */
	{
		MMap<CVConGroup*,CVirtualConsole*> Groups; Groups.Init(MAX_CONSOLE_COUNT, true);

		for (V = 0; V < MAX_CONSOLE_COUNT; V++)
		{
			//if (!(pVCon = gpConEmu->GetVCon(V))) continue;
			CVConGuard guard;
			if (!CVConGroup::GetVCon(V, &guard))
				continue;
			pVCon = guard.VCon();

			BOOL lbActive = gpConEmu->isActive(pVCon, false);

			if (gpSet->bHideInactiveConsoleTabs)
			{
				if (!lbActive) continue;
			}

			if (gpSet->isOneTabPerGroup)
			{
				CVConGroup *pGr;
				CVConGuard VGrActive;
				if (CVConGroup::isGroup(pVCon, &pGr, &VGrActive))
				{
					CVirtualConsole* pGrVCon;

					if (Groups.Get(pGr, &pGrVCon))
						continue; // эта группа уже есть

					pGrVCon = VGrActive.VCon();
					Groups.Set(pGr, pGrVCon);

					// И показывать таб нужно от "активной" консоли, а не от первой в группе
					if (pVCon != pGrVCon)
					{
						guard = pGrVCon;
						pVCon = pGrVCon;
					}

					if (!lbActive)
					{
						lbActive = gpConEmu->isActive(pVCon, true);
					}
				}
			}

			CRealConsole *pRCon = pVCon->RCon();
			if (!pRCon)
			{
				_ASSERTE(pRCon!=NULL);
				continue;
			}

			// (Panels=1, Viewer=2, Editor=3) |(Elevated=0x100) |(NotElevated=0x200) |(Modal=0x400)
			bool bAllWindows = (bShowFarWindows && !(pRCon->GetActiveTabType() & fwt_ModalFarWnd));
			rFrom = bAllWindows ? 0 : pRCon->GetActiveTab();
			rFound = 0;

			for (I = rFrom; bAllWindows || !rFound; I++)
			{
				#ifdef _DEBUG
					if (!I && !V)
					{
						_ASSERTE(m_Tab2VCon.size()==0);
					}
					if (this != gpConEmu->mp_TabBar)
					{
						_ASSERTE(this == gpConEmu->mp_TabBar);
					}
					MCHKHEAP;
				#endif


				// bShowFarWindows проверяем, чтобы не проколоться с недоступностью единственного таба
				if (gpSet->bHideDisabledTabs && bShowFarWindows)
				{
					if (!pRCon->CanActivateFarWindow(I))
						continue;
				}

				if (!pRCon->GetTab(I, &tab))
					break;


				#ifdef _DEBUG
					if (this != gpConEmu->mp_TabBar)
					{
						_ASSERTE(this == gpConEmu->mp_TabBar);
					}
					MCHKHEAP;
				#endif


				PrepareTab(&tab, pVCon);
				vct.pVCon = pVCon;
				vct.nFarWindowId = I;


				#ifdef _DEBUG
					if (!I && !V)
					{
						_ASSERTE(m_Tab2VCon.size()==0);
					}
				#endif


				AddTab2VCon(vct);
				// Добавляет закладку, или меняет (при необходимости) заголовок существующей
				mp_Rebar->AddTabInt(tab.Name, tabIdx, (tab.Type & fwt_Elevated)==fwt_Elevated);

				if (lbActive && tab.Current)
					nCurTab = tabIdx;

				rFound++;
				tabIdx++;


				#ifdef _DEBUG
					if (this != gpConEmu->mp_TabBar)
					{
						_ASSERTE(this == gpConEmu->mp_TabBar);
					}
				#endif
			}
		}

		Groups.Release();
	}

	MCHKHEAP

	// Must be at least one tab!
	if (tabIdx == 0)
	{
		ZeroStruct(tab);
		PrepareTab(&tab, NULL);
		wcscpy_c(tab.Name, gpConEmu->GetDefaultTitle());
		vct.pVCon = NULL;
		vct.nFarWindowId = 0;
		AddTab2VCon(vct); //2009-06-14. Не было!
		// Добавляет закладку, или меняет (при необходимости) заголовок существующей
		mp_Rebar->AddTabInt(tab.Name, tabIdx, (tab.Type & fwt_Elevated)==fwt_Elevated);
		nCurTab = tabIdx;
		tabIdx++;
	}

	// Update последних выбранных
	if ((nCurTab >= 0) && (nCurTab < m_Tab2VCon.size()))
		AddStack(m_Tab2VCon[nCurTab]);
	else
		CheckStack(); // иначе просто проверим стек

	// удалить лишние закладки (визуально)
	int nCurCount = GetItemCount();

	#ifdef _DEBUG
	wchar_t szDbg[128];
	_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"CTabBarClass::Update.  ItemCount=%i, PrevItemCount=%i\n", tabIdx, nCurCount);
	DEBUGSTRTABS(szDbg);
	#endif

	for (I = tabIdx; I < nCurCount; I++)
	{
		#ifdef _DEBUG
		_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"   Deleting tab=%i\n", I+1);
		DEBUGSTRTABS(szDbg);
		#endif

		DeleteItem(tabIdx);
	}

	MCHKHEAP

	if (mb_InKeySwitching)
	{
		if (mn_CurSelTab >= nCurCount)  // Если выбранный таб вылез за границы
			mb_InKeySwitching = FALSE;
	}

	if (!mb_InKeySwitching && nCurTab != -1)
	{
		SelectTab(nCurTab);
	}

	UpdateToolConsoles();

	//if (gpSet->isTabsInCaption)
	//{
	//	SendMessage(ghWnd, WM_NCPAINT, 0, 0);
	//}

	mn_InUpdate --;

	if (mb_PostUpdateRequested)
	{
		mb_PostUpdateCalled = FALSE;
		mb_PostUpdateRequested = FALSE;
		RequestPostUpdate();
	}

	MCHKHEAP
}

void CTabBarClass::AddTab2VCon(VConTabs& vct)
{
	m_Tab2VCon.push_back(vct);
}

RECT CTabBarClass::GetMargins()
{
	_ASSERTE(this);
	RECT rcNewMargins = {0,0};

	if (_active || (gpSet->isTabs == 1))
	{
		if (!_tabHeight)
		{
			// Вычислить высоту
			GetTabbarHeight();
		}

		if (_tabHeight /*&& IsTabsShown()*/)
		{
			_ASSERTE(_tabHeight!=0 && "Height must be evaluated already!");

			if (gpSet->nTabsLocation == 1)
				rcNewMargins = MakeRect(0,0,0,_tabHeight);
			else
				rcNewMargins = MakeRect(0,_tabHeight,0,0);

			//if (memcmp(&rcNewMargins, &m_Margins, sizeof(m_Margins)) != 0)
			//{
			//	m_Margins = rcNewMargins;
			//	gpSet->UpdateMargins(m_Margins);
			//}
		}
	}
	//return m_Margins;

	return rcNewMargins;
}

void CTabBarClass::UpdatePosition()
{
	if (!mp_Rebar->IsCreated())
		return;

	if (gpConEmu->isIconic())
		return; // иначе расчет размеров будет некорректным!

	DEBUGSTRTABS(_active ? L"CTabBarClass::UpdatePosition(activate)\n" : L"CTabBarClass::UpdatePosition(DEactivate)\n");

	#ifdef _DEBUG
	DWORD_PTR dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
	#endif

	if (_active)
	{
		_visible = true;

		mp_Rebar->ShowBar(true);

		#ifdef _DEBUG
		dwStyle = GetWindowLongPtr(ghWnd, GWL_STYLE);
		#endif

		if (!gpConEmu->InCreateWindow())
		{
			//gpConEmu->Sync ConsoleToWindow(); -- 2009.07.04 Sync должен быть выполнен в самом ReSize
			gpConEmu->ReSize(TRUE);
		}
	}
	else
	{
		_visible = false;

		//gpConEmu->Sync ConsoleToWindow(); -- 2009.07.04 Sync должен быть выполнен в самом ReSize
		gpConEmu->ReSize(TRUE);

		// _active уже сбросили, поэтому реально спрятать можно и позже
		mp_Rebar->ShowBar(false);
	}
}

void CTabBarClass::Reposition()
{
	if (!_active)
	{
		return;
	}

	mp_Rebar->RepositionInt();
}

LPCWSTR CTabBarClass::GetTabText(int nTabIdx)
{
	if (!mp_Rebar->GetTabText(nTabIdx, ms_TmpTabText, countof(ms_TmpTabText)))
		return L"";
	return ms_TmpTabText;
}

LRESULT CTabBarClass::OnNotify(LPNMHDR nmhdr)
{
	if (!this)
		return false;

	if (!_active)
	{
		return false;
	}

	LRESULT lResult = 0;

	if (mp_Rebar->OnNotifyInt(nmhdr, lResult))
		return lResult;


	if (nmhdr->code == TBN_GETINFOTIP && mp_Rebar->IsToolbarNotify(nmhdr))
	{
		if (!gpSet->isMultiShowButtons)
			return 0;

		LPNMTBGETINFOTIP pDisp = (LPNMTBGETINFOTIP)nmhdr;

		//if (pDisp->iItem>=1 && pDisp->iItem<=MAX_CONSOLE_COUNT)
		if (pDisp->iItem == TID_ACTIVE_NUMBER)
		{
			if (!pDisp->pszText || !pDisp->cchTextMax)
				return false;

			CVConGuard VCon;
			CVirtualConsole* pVCon = (gpConEmu->GetActiveVCon(&VCon) >= 0) ? VCon.VCon() : NULL;
			LPCWSTR pszTitle = pVCon ? pVCon->RCon()->GetTitle() : NULL;

			if (pszTitle)
			{
				lstrcpyn(pDisp->pszText, pszTitle, pDisp->cchTextMax);
			}
			else
			{
				pDisp->pszText[0] = 0;
			}
		}
		else if (pDisp->iItem == TID_CREATE_CON)
		{
			lstrcpyn(pDisp->pszText, _T("Create new console"), pDisp->cchTextMax);
		}
		else if (pDisp->iItem == TID_ALTERNATIVE)
		{
			bool lbChecked = mp_Rebar->GetToolBtnChecked(TID_ALTERNATIVE);
			lstrcpyn(pDisp->pszText,
			         lbChecked ? L"Alternative mode is ON (console freezed)" : L"Alternative mode is off",
			         pDisp->cchTextMax);
		}
		else if (pDisp->iItem == TID_SCROLL)
		{
			bool lbChecked = mp_Rebar->GetToolBtnChecked(TID_SCROLL);
			lstrcpyn(pDisp->pszText,
			         lbChecked ? L"BufferHeight mode is ON (scrolling enabled)" : L"BufferHeight mode is off",
			         pDisp->cchTextMax);
		}
		else if (pDisp->iItem == TID_MINIMIZE)
		{
			lstrcpyn(pDisp->pszText, _T("Minimize window"), pDisp->cchTextMax);
		}
		else if (pDisp->iItem == TID_MAXIMIZE)
		{
			lstrcpyn(pDisp->pszText, _T("Maximize window"), pDisp->cchTextMax);
		}
		else if (pDisp->iItem == TID_APPCLOSE)
		{
			lstrcpyn(pDisp->pszText, _T("Close ALL consoles"), pDisp->cchTextMax);
		}
		else if (pDisp->iItem == TID_COPYING)
		{
			lstrcpyn(pDisp->pszText, _T("Show copying queue"), pDisp->cchTextMax);
		}

		return true;
	}

	if (nmhdr->code == TBN_DROPDOWN && mp_Rebar->IsToolbarNotify(nmhdr))
	{
		LPNMTOOLBAR pBtn = (LPNMTOOLBAR)nmhdr;
		switch (pBtn->iItem)
		{
		case TID_ACTIVE_NUMBER:
			OnChooseTabPopup();
			break;
		case TID_CREATE_CON:
			gpConEmu->mp_Menu->OnNewConPopupMenu();
			break;
		}
		return TBDDRET_DEFAULT;
	}

	if (nmhdr->code == TTN_GETDISPINFO && mp_Rebar->IsTabbarNotify(nmhdr))
	{
		LPNMTTDISPINFO pDisp = (LPNMTTDISPINFO)nmhdr;
		CVirtualConsole *pVCon = NULL;
		DWORD wndIndex = 0;
		pDisp->hinst = NULL;
		pDisp->szText[0] = 0;
		pDisp->lpszText = NULL;
		POINT ptScr = {}; GetCursorPos(&ptScr);
		int iPage = mp_Rebar->GetTabFromPoint(ptScr);

		if (iPage >= 0)
		{
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

void CTabBarClass::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (!this)
		return;

	if (!mp_Rebar->IsToolbarCommand(wParam, lParam))
		return;

	if (!gpSet->isMultiShowButtons)
	{
		_ASSERTE(gpSet->isMultiShowButtons);
		return;
	}

	if (wParam == TID_ACTIVE_NUMBER)
	{
		//gpConEmu->ConActivate(wParam-1);
		OnChooseTabPopup();
	}
	else if (wParam == TID_CREATE_CON)
	{
		if (gpConEmu->IsGesturesEnabled())
			gpConEmu->mp_Menu->OnNewConPopupMenu();
		else
			gpConEmu->RecreateAction(gpSetCls->GetDefaultCreateAction(), gpSet->isMultiNewConfirm || isPressed(VK_SHIFT));
	}
	else if (wParam == TID_ALTERNATIVE)
	{
		CVConGuard VCon;
		CVirtualConsole* pVCon = (gpConEmu->GetActiveVCon(&VCon) >= 0) ? VCon.VCon() : NULL;
		// Вернуть на тулбар _текущее_ состояние режима
		mp_Rebar->SetToolBtnChecked(TID_ALTERNATIVE, pVCon ? pVCon->RCon()->isAlternative() : false);
		// И собственно Action
		gpConEmu->AskChangeAlternative();
	}
	else if (wParam == TID_SCROLL)
	{
		CVConGuard VCon;
		CVirtualConsole* pVCon = (gpConEmu->GetActiveVCon(&VCon) >= 0) ? VCon.VCon() : NULL;
		// Вернуть на тулбар _текущее_ состояние режима
		mp_Rebar->SetToolBtnChecked(TID_SCROLL, pVCon ? pVCon->RCon()->isBufferHeight() : false);
		// И собственно Action
		gpConEmu->AskChangeBufferHeight();
	}
	else if (wParam == TID_MINIMIZE)
	{
		PostMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	}
	else if (wParam == TID_MAXIMIZE)
	{
		// Чтобы клик случайно не провалился в консоль
		gpConEmu->mouse.state |= MOUSE_SIZING_DBLCKL;
		// Аналог AltF9
		gpConEmu->DoMaximizeRestore();
	}
	else if (wParam == TID_APPCLOSE)
	{
		gpConEmu->PostScClose();
	}
	else if (wParam == TID_COPYING)
	{
		gpConEmu->OnCopyingState();
	}
}

void CTabBarClass::OnMouse(int message, int x, int y, bool yCenter /*= false*/)
{
	if (!this)
		return;

	if (!_active)
	{
		return;
	}

	// After clicks above exact tab (Fullscreen or caption hidden)
	if (yCenter)
	{
		RECT rcClient = {}; GetWindowRect(mh_Tabbar, &rcClient);
		y = (rcClient.top + rcClient.bottom) >> 1;
	}

	if ((message == WM_MBUTTONUP)
		|| (message == WM_RBUTTONUP)
		|| ((message == WM_LBUTTONDBLCLK) && gpSet->nTabBtnDblClickAction))
	{
		TCHITTESTINFO htInfo = {{x,y}};
		int iPage = mp_Rebar->GetTabFromPoint(htInfo.pt, false);

		if (iPage >= 0)
		{
			CVirtualConsole* pVCon = NULL;
			// для меню нужны экранные координаты, получим их сразу, чтобы менюшка вплывала на клике
			// а то вдруг мышка уедет, во время активации таба...
			POINT ptCur = {0,0}; GetCursorPos(&ptCur);
			pVCon = mp_Rebar->FarSendChangeTab(iPage);

			if (pVCon)
			{
				CVConGuard guard(pVCon);
				BOOL lbCtrlPressed = isPressed(VK_CONTROL);

				if (message == WM_LBUTTONDBLCLK)
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
				else if (message == WM_RBUTTONUP && !lbCtrlPressed)
				{
					gpConEmu->mp_Menu->ShowPopupMenu(guard.VCon(), ptCur);
				}
				else
				{
					guard->RCon()->CloseTab();
					//if (pVCon->RCon()->GetFarPID())
					//{
					//	pVCon->RCon()->PostMacro(gpSet->sTabCloseMacro ? gpSet->sTabCloseMacro : L"F10");
					//}
					//else
					//{
					//	// Если запущен CMD, PowerShell, и т.п.
					//	pVCon->RCon()->CloseTab();
					//	//// показать ДИАЛОГ пересоздания консоли, там есть кнопки Terminate & Recreate
					//	//gpConEmu->Recreate(TRUE, TRUE);
					//}
				}

				// борьба с оптимизатором в релизе
				gpConEmu->isValid(pVCon);
			}
		}
	}
	else if (message == WM_LBUTTONUP)
	{
		TODO("Обработать клик НАД табами");
		TCHITTESTINFO htInfo;
		htInfo.pt.x = x;
		htInfo.pt.y = y;
		int iPage = TabCtrl_HitTest(mh_Tabbar, &htInfo);

		int iDbg = 0;
	}
}

void CTabBarClass::Invalidate()
{
	if (gpConEmu->mp_TabBar->IsTabsActive())
		mp_Rebar->InvalidateBar();
}

void CTabBarClass::OnCaptionHidden()
{
	if (!this) return;

	mp_Rebar->OnCaptionHiddenChanged(gpSet->isCaptionHidden());
}

void CTabBarClass::OnWindowStateChanged()
{
	if (!this) return;

	mp_Rebar->OnWindowStateChanged(gpConEmu->GetWindowMode());
}

// Обновить наличие кнопок консолей на тулбаре
void CTabBarClass::UpdateToolConsoles(bool abForcePos/*=false*/)
{
	if (!mp_Rebar->IsToolbarCreated())
		return;

	int nNewActiveIdx = gpConEmu->ActiveConNum(); // 0-based

	OnConsoleActivated(nNewActiveIdx);
}

// nConNumber - 0-based
void CTabBarClass::OnConsoleActivated(int nConNumber)
{
	if (!mp_Rebar->IsToolbarCreated())
		return;

	mp_Rebar->OnConsoleActivatedInt(nConNumber);
}

void CTabBarClass::OnBufferHeight(BOOL abBufferHeight)
{
	if (!mp_Rebar->IsToolbarCreated())
		return;

	mp_Rebar->SetToolBtnChecked(TID_SCROLL, (abBufferHeight!=FALSE));
}

void CTabBarClass::OnAlternative(BOOL abAlternative)
{
	if (!mp_Rebar->IsToolbarCreated())
		return;

	mp_Rebar->SetToolBtnChecked(TID_ALTERNATIVE, (abAlternative!=FALSE));
}

void CTabBarClass::OnShowButtonsChanged()
{
	if (gpSet->isMultiShowButtons)
	{
		mp_Rebar->ShowToolbar(true);
	}

	Reposition();
}

int CTabBarClass::GetTabbarHeight()
{
	if (!this) return 0;

	_ASSERTE(gpSet->isTabs!=0);

	if (mb_ForceRecalcHeight || (_tabHeight == 0))
	{
		// Узнать высоту созданного
		_tabHeight = mp_Rebar->QueryTabbarHeight();
	}

	return _tabHeight;
}

void CTabBarClass::PrepareTab(ConEmuTab* pTab, CVirtualConsole *apVCon)
{
#ifdef _DEBUG

	if (this != gpConEmu->mp_TabBar)
	{
		_ASSERTE(this == gpConEmu->mp_TabBar);
	}

#endif
	MCHKHEAP
	// get file name
	TCHAR dummy[MAX_PATH*2];
	TCHAR fileName[MAX_PATH+4]; fileName[0] = 0;
	TCHAR szFormat[32];
	TCHAR szEllip[MAX_PATH+1];
	//wchar_t /**tFileName=NULL,*/ *pszNo=NULL, *pszTitle=NULL;
	int nSplit = 0;
	int nMaxLen = 0; //gpSet->nTabLenMax - _tcslen(szFormat) + 2/* %s */;
	int origLength = 0; //_tcslen(tFileName);

	CRealConsole* pRCon = apVCon ? apVCon->RCon() : NULL;
	bool bIsFar = pRCon ? pRCon->isFar() : false;


	if (pTab->Name[0]==0 || (pTab->Type & 0xFF) == 1/*WTYPE_PANELS*/)
	{
		//_tcscpy(szFormat, _T("%s"));
		lstrcpyn(szFormat, bIsFar ? gpSet->szTabPanels : gpSet->szTabConsole, countof(szFormat));
		nMaxLen = gpSet->nTabLenMax - _tcslen(szFormat) + 2/* %s */;

		if (apVCon && gpConEmu->mb_IsUacAdmin)
			pTab->Type |= fwt_Elevated;

		if (pTab->Name[0] == 0)
		{
			// Это может случаться при инициализации GUI или закрытии консоли
			#if 0
			_ASSERTE(!bIsFar);

			int nTabCount = GetItemCount();

			if (nTabCount>0 && gpConEmu->ActiveCon()!=NULL)
			{
				//_ASSERTE(pTab->Name[0] != 0);
				nTabCount = nTabCount;
			}
			#endif

			//100930 - нельзя. GetLastTitle() вернет текущую консоль, а pTab может быть из любой консоли!
			// -- _tcscpy(pTab->Name, gpConEmu->GetLastTitle()); //isFar() ? gpSet->szTabPanels : gpSet->pszTabConsole);
			_tcscpy(pTab->Name, gpConEmu->GetDefaultTitle());
		}

		lstrcpyn(fileName, pTab->Name, countof(fileName));
		if (gpSet->szTabSkipWords[0])
		{
			StripWords(fileName, gpSet->pszTabSkipWords);
		}
		origLength = _tcslen(fileName);
		//if (origLength>6) {
		//    // Чтобы в заголовке было что-то вроде "{C:\Program Fil...- Far"
		//    //                              вместо "{C:\Program F...} - Far"
		//	После добавления суффиков к заголовку фара - оно уже влезать не будет в любом случае... Так что если панели - '...' строго ставить в конце
		//    if (lstrcmp(tFileName + origLength - 6, L" - Far") == 0)
		//        nSplit = nMaxLen - 6;
		//}
	}
	else
	{
		LPTSTR tFileName = NULL;
		if (GetFullPathName(pTab->Name, countof(dummy), dummy, &tFileName) && tFileName && *tFileName)
			lstrcpyn(fileName, tFileName, countof(fileName));
		else
			lstrcpyn(fileName, pTab->Name, countof(fileName));

		if ((pTab->Type & 0xFF) == 3/*WTYPE_EDITOR*/)
		{
			if (pTab->Modified)
				lstrcpyn(szFormat, gpSet->szTabEditorModified, countof(szFormat));
			else
				lstrcpyn(szFormat, gpSet->szTabEditor, countof(szFormat));
		}
		else if ((pTab->Type & 0xFF) == 2/*WTYPE_VIEWER*/)
		{
			lstrcpyn(szFormat, gpSet->szTabViewer, countof(szFormat));
		}
		else
		{
			_ASSERTE(FALSE && "Must be processed in previous branch");
			lstrcpyn(szFormat, bIsFar ? gpSet->szTabPanels : gpSet->szTabConsole, countof(szFormat));
		}
	}

	// restrict length
	if (!nMaxLen)
		nMaxLen = gpSet->nTabLenMax - _tcslen(szFormat) + 2/* %s */;

	if (!origLength)
		origLength = _tcslen(fileName);
	if (nMaxLen<15) nMaxLen=15; else if (nMaxLen>=MAX_PATH) nMaxLen=MAX_PATH-1;

	if (origLength > nMaxLen)
	{
		/*_tcsnset(fileName, _T('\0'), MAX_PATH);
		_tcsncat(fileName, tFileName, 10);
		_tcsncat(fileName, _T("..."), 3);
		_tcsncat(fileName, tFileName + origLength - 10, 10);*/
		//if (!nSplit)
		//    nSplit = nMaxLen*2/3;
		//// 2009-09-20 Если в заголовке нет расширения (отсутствует точка)
		//const wchar_t* pszAdmin = gpSet->szAdminTitleSuffix;
		//const wchar_t* pszFrom = tFileName + origLength - (nMaxLen - nSplit);
		//if (!wcschr(pszFrom, L'.') && (*pszAdmin && !wcsstr(tFileName, pszAdmin)))
		//{
		//	// то троеточие ставить в конец, а не середину
		//	nSplit = nMaxLen;
		//}
		// "{C:\Program Files} - Far 2.1283 Administrator x64"
		// После добавления суффиков к заголовку фара - оно уже влезать не будет в любом случае... Так что если панели - '...' строго ставить в конце
		nSplit = nMaxLen;
		_tcsncpy(szEllip, fileName, nSplit); szEllip[nSplit]=0;
		szEllip[nSplit] = L'\x2026' /*"…"*/;
		szEllip[nSplit+1] = 0;
		//_tcscat(szEllip, L"\x2026" /*"…"*/);
		//_tcscat(szEllip, tFileName + origLength - (nMaxLen - nSplit));
		//tFileName = szEllip;
		lstrcpyn(fileName, szEllip, countof(fileName));
	}

	// szFormat различается для Panel/Viewer(*)/Editor(*)
	// Пример: "%i-[%s] *"
	////pszNo = wcsstr(szFormat, L"%i");
	////pszTitle = wcsstr(szFormat, L"%s");
	////if (pszNo == NULL)
	////	_wsprintf(fileName, SKIPLEN(countof(fileName)) szFormat, tFileName);
	////else if (pszNo < pszTitle || pszTitle == NULL)
	////	_wsprintf(fileName, SKIPLEN(countof(fileName)) szFormat, pTab->Pos, tFileName);
	////else
	////	_wsprintf(fileName, SKIPLEN(countof(fileName)) szFormat, tFileName, pTab->Pos);
	//wcscpy(pTab->Name, fileName);
	const TCHAR* pszFmt = szFormat;
	TCHAR* pszDst = pTab->Name;
	TCHAR* pszStart = pszDst;
	TCHAR* pszEnd = pTab->Name + countof(pTab->Name) - 1; // в конце еще нужно зарезервировать место для '\0'

	if (!pszFmt || !*pszFmt)
	{
		pszFmt = _T("%s");
	}
	*pszDst = 0;

	bool bRenamedTab = false;
	if (pTab->Type & fwt_Renamed)
	{
		if (wcsstr(pszFmt, L"%s") == NULL)
		{
			if (wcsstr(pszFmt, L"%n") != NULL)
				bRenamedTab = true;
			else
				pszFmt = _T("%s");
		}
	}

	TCHAR szTmp[64];
	bool  bAppendAdmin = gpSet->isAdminSuffix() && (pTab->Type & fwt_Elevated);

	while (*pszFmt && pszDst < pszEnd)
	{
		if (*pszFmt == _T('%'))
		{
			pszFmt++;
			LPCTSTR pszText = NULL;
			switch (*pszFmt)
			{
				case _T('s'): case _T('S'):
					pszText = fileName;
					break;
				case _T('i'): case _T('I'):
					_wsprintf(szTmp, SKIPLEN(countof(szTmp)) _T("%i"), pTab->Pos);
					pszText = szTmp;
					break;
				case _T('p'): case _T('P'):
					if (!apVCon || !apVCon->RCon())
					{
						wcscpy_c(szTmp, _T("?"));
					}
					else
					{
						_wsprintf(szTmp, SKIPLEN(countof(szTmp)) _T("%u"), apVCon->RCon()->GetActivePID());
					}
					pszText = szTmp;
					break;
				case _T('c'): case _T('C'):
					{
						int iCon = gpConEmu->isVConValid(apVCon);
						if (iCon > 0)
							_wsprintf(szTmp, SKIPLEN(countof(szTmp)) _T("%u"), iCon);
						else
							wcscpy_c(szTmp, _T("?"));
						pszText = szTmp;
					}
					break;
				case _T('n'): case _T('N'):
					{
						pszText = bRenamedTab ? fileName : pRCon ? pRCon->GetActiveProcessName() : NULL;
						wcscpy_c(szTmp, (pszText && *pszText) ? pszText : L"?");
						pszText = szTmp;
					}
					break;
				case _T('a'): case _T('A'):
					pszText = bAppendAdmin ? gpSet->szAdminTitleSuffix : NULL;
					bAppendAdmin = false;
					break;
				case _T('%'):
					pszText = L"%";
					break;
				case 0:
					pszFmt--;
					break;
			}
			pszFmt++;
			if (pszText)
			{
				if ((*(pszDst-1) == L' ') && (*pszText == L' '))
					pszText = SkipNonPrintable(pszText);
				while (*pszText && pszDst < pszEnd)
				{
					*(pszDst++) = *(pszText++);
				}
			}
		}
		else if ((pszDst > pszStart) && (*(pszDst-1) == L' ') && (*pszFmt == L' '))
		{
			pszFmt++; // Avoid adding sequential spaces (e.g. if some macros was empty)
		}
		else
		{
			*(pszDst++) = *(pszFmt++);
		}
	}

	// Fin. Append smth else?
	if (bAppendAdmin)
	{
		LPCTSTR pszText = gpSet->szAdminTitleSuffix;
		if (pszText)
		{
			while (*pszText && pszDst < pszEnd)
			{
				*(pszDst++) = *(pszText++);
			}
		}
	}

	*pszDst = 0;

	MCHKHEAP
}



// Переключение табов

int CTabBarClass::GetIndexByTab(VConTabs tab)
{
	//int nIdx = -1;
	int nCount = m_Tab2VCon.size();

	for (int i = 0; i < nCount; i++)
	{
		if (m_Tab2VCon[i] == tab)
		{
			return i;
		}
	}

	return -1;
}

int CTabBarClass::GetNextTab(BOOL abForward, BOOL abAltStyle/*=FALSE*/)
{
	BOOL lbRecentMode = (gpSet->isTabs != 0) &&
	                    (((abAltStyle == FALSE) ? gpSet->isTabRecent : !gpSet->isTabRecent));
	int nCurSel = GetCurSel();
	int nCurCount = GetItemCount();
	VConTabs cur = {NULL};

	#ifdef _DEBUG
	if (nCurCount != m_Tab2VCon.size())
	{
		_ASSERTE(nCurCount == m_Tab2VCon.size());
	}
	#endif

	if (nCurCount < 1)
		return 0; // хотя такого и не должно быть

	if (lbRecentMode && (nCurSel >= 0) && (nCurSel < m_Tab2VCon.size()))
	{
		cur = m_Tab2VCon[nCurSel];
	}

	int i, nNewSel = -1;
	TODO("Добавить возможность переключаться а'ля RecentScreens");

	if (abForward)
	{
		if (lbRecentMode)
		{
			int iter = 0;

			while (iter < m_TabStack.GetCount())
			{
				VConTabs Item = m_TabStack[iter]; // *iter;

				// Найти в стеке выделенный таб
				if (Item == cur)
				{
					int iCur = iter;

					// Определить следующий таб, который мы можем активировать
					do
					{
						++iter;

                        // Если дошли до конца (сейчас выделен последний таб) вернуть первый
						if (iter >= m_TabStack.size())
						{
							iter = 0;
						}

						// Определить индекс в m_Tab2VCon
						i = GetIndexByTab(m_TabStack[iter]);

						if (CanActivateTab(i))
						{
							return i;
						}
					}
					while (iter != iCur);

					break;
				}

				++iter;
			}
		} // Если не смогли в стиле Recent - идем простым путем

		for (i = nCurSel+1; nNewSel == -1 && i < nCurCount; i++)
		{
			if (CanActivateTab(i))
				nNewSel = i;
		}

		for (i = 0; nNewSel == -1 && i < nCurSel; i++)
		{
			if (CanActivateTab(i))
				nNewSel = i;
		}
	}
	else
	{
		if (lbRecentMode)
		{
			int iter = m_TabStack.size()-1;

			while (iter >= 0)
			{
				VConTabs Item = m_TabStack[iter]; // *iter;

				// Найти в стеке выделенный таб
				if (Item == cur)
				{
					int iCur = iter;

					// Определить следующий таб, который мы можем активировать
					do
					{
						iter--;

                        // Если дошли до конца (сейчас выделен последний таб) вернуть первый
						if (iter < 0)
						{
							iter = m_TabStack.size()-1;
						}

						// Определить индекс в m_Tab2VCon
						i = GetIndexByTab(m_TabStack[iter]);

						if (CanActivateTab(i))
						{
							return i;
						}
					}
					while (iter != iCur);

					break;
				}

				iter--;
			}
		} // Если не смогли в стиле Recent - идем простым путем

		for (i = nCurSel-1; nNewSel == -1 && i >= 0; i++)
		{
			if (CanActivateTab(i))
				nNewSel = i;
		}

		for (i = nCurCount-1; nNewSel == -1 && i > nCurSel; i++)
		{
			if (CanActivateTab(i))
				nNewSel = i;
		}
	}

	return nNewSel;
}

void CTabBarClass::SwitchNext(BOOL abAltStyle/*=FALSE*/)
{
	Switch(TRUE, abAltStyle);
}

void CTabBarClass::SwitchPrev(BOOL abAltStyle/*=FALSE*/)
{
	Switch(FALSE, abAltStyle);
}

void CTabBarClass::Switch(BOOL abForward, BOOL abAltStyle/*=FALSE*/)
{
	int nNewSel = GetNextTab(abForward, abAltStyle);

	if (nNewSel != -1)
	{
		// mh_Tabbar может быть и создан, Но отключен пользователем!
		if (gpSet->isTabLazy && mp_Rebar->IsTabbarCreated() && gpSet->isTabs)
		{
			mb_InKeySwitching = TRUE;
			// Пока Ctrl не отпущен - только подсвечиваем таб, а не переключаем реально
			SelectTab(nNewSel);
		}
		else
		{
			mp_Rebar->FarSendChangeTab(nNewSel);
			mb_InKeySwitching = FALSE;
		}
	}
}

BOOL CTabBarClass::IsInSwitch()
{
	return mb_InKeySwitching;
}

void CTabBarClass::SwitchCommit()
{
	if (!mb_InKeySwitching) return;

	int nCurSel = GetCurSel();
	mb_InKeySwitching = FALSE;
	CVirtualConsole* pVCon = mp_Rebar->FarSendChangeTab(nCurSel);
	UNREFERENCED_PARAMETER(pVCon);
}

void CTabBarClass::SwitchRollback()
{
	if (mb_InKeySwitching)
	{
		mb_InKeySwitching = FALSE;
		Update();
	}
}

// Убьет из стека старые, и добавит новые
void CTabBarClass::CheckStack()
{
	_ASSERTE(gpConEmu->isMainThread());
	INT_PTR i, j;

	BOOL lbExist = FALSE;
	j = 0;

	// Remove all absent items
	while (j < m_TabStack.size())
	{
		// If refcount was decreased to 1, that means that CTabID is left only in m_TabStack
		// All other references was eliminated
		if (m_TabStack[j]->RefCount() <= 1)
		{
			m_TabStack[j]->Release();
			m_TabStack.erase(j);
		}
		else
		{
			j++;
		}
	}

	CTab tab;

	for (i = 0; m_Tabs.GetTabByIndex(i, tab); ++i)
	{
		lbExist = FALSE;

		for (j = 0; j < m_TabStack.size(); ++j)
		{
			if (tab == m_TabStack[j])
			{
				lbExist = TRUE; break;
			}
		}

		if (!lbExist)
		{
			m_TabStack.push_back(tab.AddRef());
		}
	}
}

// Убьет из стека отсутствующих и поместит tab на верх стека
void CTabBarClass::AddStack(CTab& tab)
{
	_ASSERTE(gpConEmu->isMainThread());
	BOOL lbExist = FALSE;

	CTabID* pTab = NULL;

	if (!m_TabStack.empty())
	{
		int iter = 0;

		while (iter < m_TabStack.size())
		{
			if (m_TabStack[iter] == tab)
			{
				if (iter == 0)
				{
					// Already first
					lbExist = TRUE;
				}
				else
				{
					pTab = m_TabStack[iter];
					m_TabStack.erase(iter);
				}

				break;
			}

			++iter;
		}
	}

	// поместить наверх стека
	if (!lbExist)
	{
		if (!pTab)
			pTab = tab.AddRef();
		m_TabStack.insert(0, pTab);
	}

	CheckStack();
}

BOOL CTabBarClass::CanActivateTab(int nTabIdx)
{
	CVirtualConsole *pVCon = NULL;
	DWORD wndIndex = 0;

	if (!GetVConFromTab(nTabIdx, &pVCon, &wndIndex))
		return FALSE;

	if (!pVCon->RCon()->CanActivateFarWindow(wndIndex))
		return FALSE;

	return TRUE;
}

BOOL CTabBarClass::OnKeyboard(UINT messg, WPARAM wParam, LPARAM lParam)
{
	//if (!IsShown()) return FALSE; -- всегда. Табы теперь есть в памяти
	BOOL lbAltPressed = isPressed(VK_MENU);

	if (messg == WM_KEYDOWN && wParam == VK_TAB)
	{
		if (!isPressed(VK_SHIFT))
			SwitchNext(lbAltPressed);
		else
			SwitchPrev(lbAltPressed);

		return TRUE;
	}
	else if (mb_InKeySwitching && messg == WM_KEYDOWN && !lbAltPressed
	        && (wParam == VK_UP || wParam == VK_DOWN || wParam == VK_LEFT || wParam == VK_RIGHT))
	{
		bool bRecent = gpSet->isTabRecent;
		gpSet->isTabRecent = false;
		BOOL bForward = (wParam == VK_RIGHT || wParam == VK_DOWN);
		Switch(bForward);
		gpSet->isTabRecent = bRecent;

		return TRUE;
	}

	return FALSE;
}

void CTabBarClass::SetRedraw(BOOL abEnableRedraw)
{
	mb_DisableRedraw = !abEnableRedraw;
}

void CTabBarClass::ShowTabError(LPCTSTR asInfo, int tabIndex)
{
	mp_Rebar->ShowTabErrorInt(asInfo, tabIndex);
}

void CTabBarClass::OnChooseTabPopup()
{
	RECT rcBtnRect = {0};
	mp_Rebar->GetToolBtnRect(TID_ACTIVE_NUMBER, &rcBtnRect);
	MapWindowPoints(ghWnd, NULL, (LPPOINT)&rcBtnRect, 2);
	POINT pt = {rcBtnRect.right,rcBtnRect.bottom};

	gpConEmu->ChooseTabFromMenu(FALSE, pt, TPM_RIGHTALIGN|TPM_TOPALIGN);
}

int CTabBarClass::ActiveTabByName(int anType, LPCWSTR asName, CVirtualConsole** ppVCon)
{
	int nTab = -1;
	CVirtualConsole *pVCon = NULL;
	if (ppVCon)
		*ppVCon = NULL;

	TODO("TabBarClass::ActiveTabByName - найти таб по имени");

	INT_PTR V, I;
	int tabIdx = 0;
	ConEmuTab tab = {0};
	for (V = 0; V < MAX_CONSOLE_COUNT && nTab == -1; V++)
	{
		if (!(pVCon = gpConEmu->GetVCon(V))) continue;

		#ifdef _DEBUG
		BOOL lbActive = gpConEmu->isActive(pVCon, false);
		#endif

		//111120 - Эту опцию игнорируем. Если редактор открыт в другой консоли - активируем ее потом
		//if (gpSet->bHideInactiveConsoleTabs)
		//{
		//	if (!lbActive) continue;
		//}

		CRealConsole *pRCon = pVCon->RCon();

		for (I = 0; TRUE; I++)
		{
			if (!pRCon->GetTab(I, &tab))
				break;
			if (tab.Type == (anType & fwt_TypeMask))
			{
				LPCWSTR pszName = PointToName(tab.Name);
				if ((pszName && (lstrcmpi(pszName, asName) == 0)) || (lstrcmpi(tab.Name, asName) == 0))
				{
					nTab = tabIdx;
					break;
				}
			}

			tabIdx++;
		}
	}


	if (nTab >= 0)
	{
		if (!CanActivateTab(nTab))
		{
			nTab = -2;
		}
		else
		{
			pVCon = mp_Rebar->FarSendChangeTab(nTab);
			if (!pVCon)
				nTab = -2;
		}
	}

	if (ppVCon)
		*ppVCon = pVCon;

	return nTab;
}

void CTabBarClass::UpdateTabFont()
{
	mp_Rebar->UpdateTabFontInt();
}

// Прямоугольник в клиентских координатах ghWnd!
bool CTabBarClass::GetRebarClientRect(RECT* rc)
{
	return mp_Rebar->GetRebarClientRect(rc);
}

bool CTabBarClass::GetActiveTabRect(RECT* rcTab)
{
	bool bSet = false;

	if (!IsTabsShown())
	{
		_ASSERTE(IsTabsShown());
		memset(rcTab, 0, sizeof(*rcTab));
	}
	else
	{
		bSet = mp_Rebar->GetTabRect(-1/*Active*/, rcTab);
	}

	return bSet;
}

bool CTabBarClass::Toolbar_GetBtnRect(int nCmd, RECT* rcBtnRect)
{
	if (!IsTabsShown())
	{
		return false;
	}
	return mp_Rebar->GetToolBtnRect(nCmd, rcBtnRect);
}

LRESULT CTabBarClass::OnTimer(WPARAM wParam)
{
	return mp_Rebar->OnTimerInt(wParam);
}
