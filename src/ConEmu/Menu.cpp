
/*
Copyright (c) 2012 Maximus5
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

#ifndef HIDE_USE_EXCEPTION_INFO
#define HIDE_USE_EXCEPTION_INFO
#endif

#ifndef SHOWDEBUGSTR
#define SHOWDEBUGSTR
#endif

#include <windows.h>
//
#include "header.h"
//
#include "Attach.h"
#include "ConEmu.h"
#include "FindDlg.h"
#include "Menu.h"
#include "Menu.h"
#include "Options.h"
#include "RealConsole.h"
#include "Status.h"
#include "TabBar.h"
#include "TrayIcon.h"
#include "Update.h"
#include "VConChild.h"
#include "VConGroup.h"
#include "VirtualConsole.h"


#define DEBUGSTRSIZE(s) //DEBUGSTR(s)
#define DEBUGSTRSYS(s) //DEBUGSTR(s)



CConEmuMenu::CConEmuMenu()
{
	mb_InNewConPopup = false;
	mb_InNewConPopup = mb_InNewConRPopup = false;
	mn_FirstTaskID = mn_LastTaskID = 0;
	mb_PassSysCommand = false;
	mb_InScMinimize = false;
	mb_InRestoreFromMinimized = false;
	mn_SysMenuOpenTick = mn_SysMenuCloseTick = 0;
	mn_TrackMenuPlace = tmp_None;

	HMENU* hMenu[] = {
		&mh_SysDebugPopup, &mh_SysEditPopup, &mh_ActiveVConPopup, &mh_TerminateVConPopup, &mh_VConListPopup, &mh_HelpPopup, // Popup's для SystemMenu
		&mh_InsideSysMenu,
		&mh_PopupMenu, &mh_TerminatePopup, &mh_VConDebugPopup, &mh_VConEditPopup, // А это из VirtualConsole
		NULL // end
	};
	mn_MenusCount = countof(hMenu);
	mph_Menus = (HMENU**)malloc(sizeof(hMenu));
	memmove(mph_Menus, hMenu, sizeof(hMenu));
	for (size_t i = 0; mph_Menus[i]; i++)
	{
		*mph_Menus[i] = NULL;
	}
}

CConEmuMenu::~CConEmuMenu()
{
	for (size_t i = 0; mph_Menus[i]; i++)
	{
		if (*mph_Menus[i])
		{
			DestroyMenu(*mph_Menus[i]);
			*mph_Menus[i] = NULL;
		}
	}
}

// Returns previous value
bool CConEmuMenu::SetPassSysCommand(bool abPass /*= true*/)
{
	bool b = mb_PassSysCommand;
	mb_PassSysCommand = abPass;
	return b;
}

bool CConEmuMenu::GetPassSysCommand()
{
	return mb_PassSysCommand;
}

// Returns previous value
bool CConEmuMenu::SetInScMinimize(bool abInScMinimize)
{
	bool b = mb_InScMinimize;
	mb_InScMinimize = abInScMinimize;
	return b;
}

bool CConEmuMenu::GetInScMinimize()
{
	return mb_InScMinimize;
}

bool CConEmuMenu::SetRestoreFromMinimized(bool abInRestore)
{
	bool b = mb_InRestoreFromMinimized;
	mb_InRestoreFromMinimized = abInRestore;
	return b;
}

bool CConEmuMenu::GetRestoreFromMinimized()
{
	return mb_InRestoreFromMinimized;
}

TrackMenuPlace CConEmuMenu::SetTrackMenuPlace(TrackMenuPlace tmpPlace)
{
	TrackMenuPlace t = mn_TrackMenuPlace;
	mn_TrackMenuPlace = tmpPlace;
	return t;
}

TrackMenuPlace CConEmuMenu::GetTrackMenuPlace()
{
	return mn_TrackMenuPlace;
}

void CConEmuMenu::OnNewConPopupMenu(POINT* ptWhere /*= NULL*/, DWORD nFlags /*= 0*/)
{
	HMENU hPopup = CreatePopupMenu();
	LPCWSTR pszCurCmd = NULL;
	bool lbReverse = (nFlags & TPM_BOTTOMALIGN) == TPM_BOTTOMALIGN;

	CVConGuard VCon;

	if ((gpConEmu->GetActiveVCon(&VCon) >= 0) && VCon->RCon())
		pszCurCmd = VCon->RCon()->GetCmd();

	LPCWSTR pszHistory = gpSet->psCmdHistory;
	int nFirstID = 0, nLastID = 0, nFirstGroupID = 0, nLastGroupID = 0;
	int nCreateID = 0, nSetupID = 0, nResetID = 0;
	bool bWasHistory = false;

	memset(m_CmdPopupMenu, 0, sizeof(m_CmdPopupMenu));

	//// Обновить группы команд
	//gpSet->LoadCmdTasks(NULL);

	int nGroup = 0;
	const Settings::CommandTasks* pGrp = NULL;
	while ((nGroup < MAX_CMD_GROUP_SHOW) && (pGrp = gpSet->CmdTaskGet(nGroup)))
	{
		m_CmdPopupMenu[nLastID].nCmd = nLastID+1;
		m_CmdPopupMenu[nLastID].pszCmd = NULL; // pGrp->pszCommands; - don't show hint, there is SubMenu on RClick

		if (nGroup < 9)
			_wsprintf(m_CmdPopupMenu[nLastID].szShort, SKIPLEN(countof(m_CmdPopupMenu[nLastID].szShort)) L"&%i: ", nGroup+1);
		else if (nGroup == 9)
			wcscpy_c(m_CmdPopupMenu[nLastID].szShort, L"1&0: ");
		else
			m_CmdPopupMenu[nLastID].szShort[0] = 0;

		int nCurLen = _tcslen(m_CmdPopupMenu[nLastID].szShort);

		int nMaxShort = countof(m_CmdPopupMenu[nLastID].szShort)-2; // wchar_t szShort[32];
		int nLen = _tcslen(pGrp->pszName);
		if ((nLen+nCurLen) < nMaxShort)
		{
			lstrcpyn(m_CmdPopupMenu[nLastID].szShort+nCurLen, pGrp->pszName, countof(m_CmdPopupMenu[nLastID].szShort)-nCurLen);
			nLen = lstrlen(m_CmdPopupMenu[nLastID].szShort);
		}
		else
		{
			lstrcpyn(m_CmdPopupMenu[nLastID].szShort+nCurLen, pGrp->pszName, countof(m_CmdPopupMenu[nLastID].szShort)-1-nCurLen);
			m_CmdPopupMenu[nLastID].szShort[nMaxShort-2] = /*…*/L'\x2026';
			nLen = nMaxShort-1;
		}
		m_CmdPopupMenu[nLastID].szShort[nLen] = L'\t';
		m_CmdPopupMenu[nLastID].szShort[nLen+1] = 0xBB /* RightArrow/Quotes */;
		m_CmdPopupMenu[nLastID].szShort[nLen+2] = 0;

		InsertMenu(hPopup, lbReverse ? 0 : -1, MF_BYPOSITION | MF_STRING | MF_ENABLED, m_CmdPopupMenu[nLastID].nCmd, m_CmdPopupMenu[nLastID].szShort);
		nLastID++; nGroup++;
		nLastGroupID = nLastID;
		if (!nFirstGroupID)
			nFirstGroupID = nLastID;
	}

	nSetupID = ++nLastID;
	InsertMenu(hPopup, lbReverse ? 0 : -1, MF_BYPOSITION | MF_STRING | MF_ENABLED, nSetupID, L"Setup tasks...");

	nFirstID = nLastID+1;

	if (pszHistory && *pszHistory)
	{
		bool bSeparator = false;
		int nCount = 0;
		while (*pszHistory && (nCount < MAX_CMD_HISTORY_SHOW))
		{
			// Текущий - будет первым
			if (!pszCurCmd || lstrcmp(pszCurCmd, pszHistory))
			{
				m_CmdPopupMenu[nLastID].nCmd = nLastID+1;
				m_CmdPopupMenu[nLastID].pszCmd = pszHistory;
				int nLen = _tcslen(pszHistory);
				int nMaxShort = countof(m_CmdPopupMenu[nLastID].szShort);
				if (nLen >= nMaxShort)
				{
					m_CmdPopupMenu[nLastID].szShort[0] = /*…*/L'\x2026';
					_wcscpyn_c(m_CmdPopupMenu[nLastID].szShort+1, nMaxShort-1, pszHistory+nLen-nMaxShort+2, nMaxShort-1);
					m_CmdPopupMenu[nLastID].szShort[nMaxShort-1] = 0;
				}
				else
				{
					_wcscpyn_c(m_CmdPopupMenu[nLastID].szShort, nMaxShort, pszHistory, nMaxShort);
				}

				if (!bSeparator)
				{
					bSeparator = true;
					InsertMenu(hPopup, lbReverse ? 0 : -1, MF_BYPOSITION | MF_SEPARATOR, -1, NULL);
				}

				InsertMenu(hPopup, lbReverse ? 0 : -1, MF_BYPOSITION | MF_STRING | MF_ENABLED, m_CmdPopupMenu[nLastID].nCmd, m_CmdPopupMenu[nLastID].szShort);
				nLastID++; nCount++; bWasHistory = true;
			}

			pszHistory += _tcslen(pszHistory)+1;
		}
	}

	if (pszCurCmd && *pszCurCmd)
	{
		m_CmdPopupMenu[nLastID].nCmd = nLastID+1;
		m_CmdPopupMenu[nLastID].pszCmd = pszCurCmd;
		int nLen = _tcslen(pszCurCmd);
		int nMaxShort = countof(m_CmdPopupMenu[nLastID].szShort);
		if (nLen >= nMaxShort)
		{
			m_CmdPopupMenu[nLastID].szShort[0] = /*…*/L'\x2026';
			_wcscpyn_c(m_CmdPopupMenu[nLastID].szShort+1, nMaxShort-1, pszCurCmd+nLen-nMaxShort+2, nMaxShort-1);
			m_CmdPopupMenu[nLastID].szShort[nMaxShort-1] = 0;
		}
		else
		{
			_wcscpyn_c(m_CmdPopupMenu[nLastID].szShort, nMaxShort, pszCurCmd, nMaxShort);
		}
		InsertMenu(hPopup, lbReverse ? -1 : 0, MF_BYPOSITION|MF_SEPARATOR, 0, 0);
		InsertMenu(hPopup, lbReverse ? -1 : 0, MF_BYPOSITION|MF_ENABLED|MF_STRING, m_CmdPopupMenu[nLastID].nCmd, m_CmdPopupMenu[nLastID].szShort);
		nLastID++;
		nCreateID = ++nLastID;
		InsertMenu(hPopup, lbReverse ? -1 : 0, MF_BYPOSITION|MF_ENABLED|MF_STRING, nCreateID, L"New console dialog...");
	}

	if (bWasHistory)
	{
		nResetID = ++nLastID;
		InsertMenu(hPopup, lbReverse ? 0 : -1, MF_BYPOSITION | MF_STRING | MF_ENABLED, nResetID, L"Clear history...");
	}

	RECT rcBtnRect = {0};
	LPRECT lpExcl = NULL;
	DWORD nAlign = TPM_RIGHTALIGN|TPM_TOPALIGN;

	if (ptWhere)
	{
		rcBtnRect.right = ptWhere->x;
		rcBtnRect.bottom = ptWhere->y;
		if (nFlags)
			nAlign = nFlags;
	}
	else if (gpConEmu->mp_TabBar && gpConEmu->mp_TabBar->IsTabsShown())
	{
		gpConEmu->mp_TabBar->Toolbar_GetBtnRect(TID_CREATE_CON, &rcBtnRect);
		//SendMessage(mh_Toolbar, TB_GETRECT, TID_CREATE_CON, (LPARAM)&rcBtnRect);
		//MapWindowPoints(mh_Toolbar, NULL, (LPPOINT)&rcBtnRect, 2);
		MapWindowPoints(ghWnd, NULL, (LPPOINT)&rcBtnRect, 2);
		lpExcl = &rcBtnRect;
	}
	else
	{
		GetClientRect(ghWnd, &rcBtnRect);
		MapWindowPoints(ghWnd, NULL, (LPPOINT)&rcBtnRect, 2);
		rcBtnRect.left = rcBtnRect.right;
		rcBtnRect.bottom = rcBtnRect.top;
	}

	mn_FirstTaskID = nFirstGroupID; mn_LastTaskID = nLastGroupID;

	mb_InNewConRPopup = false; // JIC

	mb_InNewConPopup = true;
	int nId = trackPopupMenu(tmp_Cmd, hPopup, nAlign|TPM_RETURNCMD/*|TPM_NONOTIFY*/,
	                         rcBtnRect.right,rcBtnRect.bottom, ghWnd, lpExcl);
	mb_InNewConPopup = mb_InNewConRPopup = false;
	//gpConEmu->mp_Tip->HideTip();
	
	if (nCreateID && (nId == nCreateID))
	{
		gpConEmu->RecreateAction(gpSet->GetDefaultCreateAction(), TRUE);
	}
	else if (nSetupID && (nId == nSetupID))
	{
		CSettings::Dialog(IDD_SPG_CMDTASKS);
	}
	else if (nResetID && (nId == nResetID))
	{
		gpSetCls->ResetCmdHistory();
	}
	else if (nId >= 1 && nId <= nLastID)
	{
		RConStartArgs con;
		if (nLastGroupID > 0 && nId <= nLastGroupID)
		{
			const Settings::CommandTasks* pGrp = gpSet->CmdTaskGet(nId-nFirstGroupID);
			if (pGrp)
			{
				con.pszSpecialCmd = lstrdup(pGrp->pszName);
				_ASSERTE(con.pszSpecialCmd && *con.pszSpecialCmd==TaskBracketLeft && con.pszSpecialCmd[lstrlen(con.pszSpecialCmd)-1]==TaskBracketRight);
			}
			else
			{
				MBoxAssert(pGrp!=NULL);
				goto wrap;
			}
		}
		else if (nId >= nFirstID)
		{
			con.pszSpecialCmd = lstrdup(m_CmdPopupMenu[nId-1].pszCmd);
		}

		if (isPressed(VK_SHIFT))
		{
			int nRc = gpConEmu->RecreateDlg(&con);

			if (nRc != IDC_START)
				return;

			CVConGroup::Redraw();
		}
		else
		{
			gpSet->HistoryAdd(con.pszSpecialCmd);
		}

		//Собственно, запуск
		if (gpSet->isMulti)
			gpConEmu->CreateCon(&con, true);
		else
			gpConEmu->CreateWnd(&con);
	}

wrap:
	DestroyMenu(hPopup);
}

void CConEmuMenu::OnNewConPopupMenuRClick(HMENU hMenu, UINT nItemPos)
{
	if (!mb_InNewConPopup || mb_InNewConRPopup)
		return;

	MENUITEMINFO mi = {sizeof(mi)};
	mi.fMask = MIIM_ID;
	if (!GetMenuItemInfo(hMenu, nItemPos, TRUE, &mi))
		return;

	wchar_t szClass[128] = {};
	POINT ptCur = {}; GetCursorPos(&ptCur);
	HWND hMenuWnd = WindowFromPoint(ptCur);
	GetClassName(hMenuWnd, szClass, countof(szClass));
	if (lstrcmp(szClass, L"#32768") != 0)
		hMenuWnd = NULL;

	UINT nId = mi.wID;

	if (mn_LastTaskID < 0 || nId > (UINT)mn_LastTaskID)
		return;

	const Settings::CommandTasks* pGrp = gpSet->CmdTaskGet(nId-mn_FirstTaskID);
	if (!pGrp || !pGrp->pszCommands || !*pGrp->pszCommands)
		return;

	// Поехали
	HMENU hPopup = CreatePopupMenu();
	wchar_t *pszDataW = lstrdup(pGrp->pszCommands);
	wchar_t *pszLine = pszDataW;
	wchar_t *pszNewLine = wcschr(pszLine, L'\n');
	const UINT nStartID = 1000;
	UINT nLastID = 0;
	LPCWSTR pszLines[MAX_CONSOLE_COUNT];

	while (*pszLine && (nLastID < MAX_CONSOLE_COUNT))
	{
		if (pszNewLine)
		{
			*pszNewLine = 0;
			if ((pszNewLine > pszDataW) && (*(pszNewLine-1) == L'\r'))
				*(pszNewLine-1) = 0;
		}

		while (*pszLine == L' ') pszLine++;

		if (*pszLine)
		{
			pszLines[nLastID] = pszLine;
			InsertMenu(hPopup, -1, MF_BYPOSITION | MF_STRING | MF_ENABLED, (++nLastID) + nStartID, pszLine);
		}

		if (!pszNewLine) break;

		pszLine = pszNewLine+1;

		if (!*pszLine) break;

		while ((*pszLine == L'\r') || (*pszLine == L'\n'))
			pszLine++; // пропустить все переводы строк

		pszNewLine = wcschr(pszLine, L'\n');
	}

	int nRetID = -1;
	
	if (nLastID > 0)
	{
		RECT rcMenuItem = {};
		GetMenuItemRect(ghWnd, hMenu, nItemPos, &rcMenuItem);

		mb_InNewConRPopup = true;
		nRetID = trackPopupMenu(tmp_CmdPopup, hPopup, TPM_RETURNCMD|TPM_NONOTIFY|TPM_RECURSE,
	                         rcMenuItem.right,rcMenuItem.top, ghWnd, &rcMenuItem);
		mb_InNewConRPopup = false;
	}

	DestroyMenu(hPopup);

	if (nRetID >= (int)(nStartID+1) && nRetID <= (int)(nStartID+nLastID))
	{
		// Need to close parentmenu
		if (hMenuWnd)
		{
			PostMessage(hMenuWnd, WM_CLOSE, 0, 0);
		}

		bool bRunAs = false;
		LPCWSTR pszCmd = gpConEmu->ParseScriptLineOptions(pszLines[nRetID-nStartID-1], &bRunAs, NULL);

		// Well, start selected line from Task
		RConStartArgs con;
		con.pszSpecialCmd = lstrdup(pszCmd);
		if (!con.pszSpecialCmd)
		{
			_ASSERTE(con.pszSpecialCmd!=NULL);
		}
		else
		{
			con.bRunAsAdministrator = bRunAs; // May be set in style ">*powershell"

			// May be directory was set in task properties?
			pGrp->ParseGuiArgs(&con.pszStartupDir, NULL);

			con.ProcessNewConArg();
			
			if (isPressed(VK_SHIFT))
			{
				int nRc = gpConEmu->RecreateDlg(&con);

				if (nRc != IDC_START)
					return;

				CVConGroup::Redraw();
			}
			else
			{
				gpSet->HistoryAdd(con.pszSpecialCmd);
			}

			//Собственно, запуск
			if (gpSet->isMulti)
				gpConEmu->CreateCon(&con, true);
			else
				gpConEmu->CreateWnd(&con);
		}
	}

	SafeFree(pszDataW);
}

bool CConEmuMenu::OnMenuSelected_NewCon(HMENU hMenu, WORD nID, WORD nFlags)
{
	if (!mb_InNewConPopup)
	{
		_ASSERTE(mb_InNewConPopup);
		return false;
	}

	if (nID >= 1 && nID <= countof(m_CmdPopupMenu) && m_CmdPopupMenu[nID-1].pszCmd)
	{
		LPCWSTR pszCmd = m_CmdPopupMenu[nID-1].pszCmd;
		if (m_CmdPopupMenu[nID-1].szShort[0] && pszCmd && lstrcmp(pszCmd, m_CmdPopupMenu[nID-1].szShort))
		{
			POINT pt; GetCursorPos(&pt);
			RECT rcMenuItem = {};
			BOOL lbMenuItemPos = FALSE;
			if (nFlags & MF_POPUP)
			{
				lbMenuItemPos = GetMenuItemRect(ghWnd, hMenu, nID, &rcMenuItem);
			}
			else
			{
				for (int i = 0; i < 100; i++)
				{
					if (GetMenuItemID(hMenu, i) == nID)
					{
						lbMenuItemPos = GetMenuItemRect(ghWnd, hMenu, i, &rcMenuItem);
						break;
					}
				}
			}
			if (lbMenuItemPos)
			{
				pt.x = rcMenuItem.left + (rcMenuItem.bottom - rcMenuItem.top)*2; //(rcMenuItem.left + rcMenuItem.right) >> 1;
				pt.y = rcMenuItem.bottom;
			}
			
			gpConEmu->mp_Tip->ShowTip(ghWnd, ghWnd, pszCmd, TRUE, pt, g_hInstance);
		}
		else
		{
			gpConEmu->mp_Tip->HideTip();
		}
	}
	else
	{
		gpConEmu->mp_Tip->HideTip();
	}
	return true;
}


bool CConEmuMenu::OnMenuSelected(HMENU hMenu, WORD nID, WORD nFlags)
{
	bool bRc = true;

	switch (mn_TrackMenuPlace)
	{
	case tmp_Cmd:
		bRc = OnMenuSelected_NewCon(hMenu, nID, nFlags);
		break;
	case tmp_System:
	case tmp_VCon:
		ShowMenuHint(hMenu, nID, nFlags);
		break;
	case tmp_KeyBar:
		ShowKeyBarHint(hMenu, nID, nFlags);
		break;
	case tmp_StatusBarCols:
		gpConEmu->mp_Status->ProcessMenuHighlight(hMenu, nID, nFlags);
		break;
	case tmp_None:
		bRc = false;
		break;
	default:
		// Unknown menu type?
		bRc = false; // Take no action
	}

	return bRc;
}


void CConEmuMenu::OnMenuRClick(HMENU hMenu, UINT nItemPos)
{
	switch (mn_TrackMenuPlace)
	{
	case tmp_Cmd:
		OnNewConPopupMenuRClick(hMenu, nItemPos);
		break;
	default:
		; // Unsupported in this menu type?
	}
}

// Показать контекстное меню для ТЕКУЩЕЙ закладки консоли
// ptCur - экранные координаты
void CConEmuMenu::ShowPopupMenu(CVirtualConsole* apVCon, POINT ptCur, DWORD Align /* = TPM_LEFTALIGN */)
{
	CVConGuard guard(apVCon);
	BOOL lbNeedCreate = (mh_PopupMenu == NULL);

	if (!Align)
		Align = TPM_LEFTALIGN;

	// Создать или обновить enable/disable
	mh_PopupMenu = CreateVConPopupMenu(apVCon, mh_PopupMenu, TRUE, mh_TerminatePopup);
	if (!mh_PopupMenu)
	{
		MBoxAssert(mh_PopupMenu!=NULL);
		return;
	}

	if (lbNeedCreate)
	{
		AppendMenu(mh_PopupMenu, MF_BYPOSITION, MF_SEPARATOR, 0);

		mh_VConEditPopup = CreateEditMenuPopup(apVCon);
		AppendMenu(mh_PopupMenu, MF_BYPOSITION | MF_POPUP | MF_ENABLED, (UINT_PTR)mh_VConEditPopup, _T("Ed&it"));

		mh_VConDebugPopup = CreateDebugMenuPopup();
		AppendMenu(mh_PopupMenu, MF_BYPOSITION | MF_POPUP | MF_ENABLED, (UINT_PTR)mh_VConDebugPopup, _T("&Debug"));
	}
	else
	{
		// обновить enable/disable пунктов меню
		CreateEditMenuPopup(apVCon, mh_VConEditPopup);
	}

	// Некузяво. Может вслыть тултип под меню
	//ptCur.x++; ptCur.y++; // чтобы меню можно было сразу закрыть левым кликом.
	
	
	// -- перенесено в CreateVConPopupMenu
	//bool lbIsFar = mp_RCon->isFar(TRUE/* abPluginRequired */)!=FALSE;
	//bool lbIsPanels = lbIsFar && mp_RCon->isFilePanel(false/* abPluginAllowed */)!=FALSE;
	//bool lbIsEditorModified = lbIsFar && mp_RCon->isEditorModified()!=FALSE;
	//bool lbHaveModified = lbIsFar && mp_RCon->GetModifiedEditors()!=FALSE;
	//bool lbCanCloseTab = mp_RCon->CanCloseTab();
	//
	//if (lbHaveModified)
	//{
	//	if (!gpSet->sSaveAllMacro || !*gpSet->sSaveAllMacro)
	//		lbHaveModified = false;
	//}
	//
	//EnableMenuItem(mh_PopupMenu, IDM_CLOSE, MF_BYCOMMAND | (lbCanCloseTab ? MF_ENABLED : MF_GRAYED));
	//EnableMenuItem(mh_PopupMenu, IDM_ADMIN_DUPLICATE, MF_BYCOMMAND | (lbIsPanels ? MF_ENABLED : MF_GRAYED));
	//EnableMenuItem(mh_PopupMenu, IDM_SAVE, MF_BYCOMMAND | (lbIsEditorModified ? MF_ENABLED : MF_GRAYED));
	//EnableMenuItem(mh_PopupMenu, IDM_SAVEALL, MF_BYCOMMAND | (lbHaveModified ? MF_ENABLED : MF_GRAYED));
	
	int nCmd = trackPopupMenu(tmp_VCon, mh_PopupMenu,
	                          Align | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
	                          ptCur.x, ptCur.y, ghWnd);

	if (!nCmd)
		return; // отмена

	ExecPopupMenuCmd(apVCon, nCmd);
}

void CConEmuMenu::ExecPopupMenuCmd(CVirtualConsole* apVCon, int nCmd)
{
	if (!apVCon)
		return;

	CVConGuard guard(apVCon);

	switch (nCmd)
	{
		case IDM_CLOSE:
			apVCon->RCon()->CloseTab();
			break;
		case IDM_DETACH:
			apVCon->RCon()->Detach();
			//if (apVCon->RCon()->Detach())
			//	gpConEmu->OnVConClosed(apVCon);
			break;
		case IDM_RENAMETAB:
			apVCon->RCon()->DoRenameTab();
			break;
		case IDM_DUPLICATE:
		case IDM_ADMIN_DUPLICATE:
			if ((nCmd == IDM_ADMIN_DUPLICATE) || isPressed(VK_SHIFT))
				apVCon->RCon()->AdminDuplicate();
			else
				apVCon->RCon()->DuplicateRoot();
			break;
		case IDM_CHILDSYSMENU:
			apVCon->RCon()->ChildSystemMenu();
			break;
		case IDM_TERMINATEPRC:
			apVCon->RCon()->CloseConsole(true, false);
			break;
		case IDM_TERMINATECON:
			//apVCon->RCon()->CloseConsoleWindow();
			apVCon->RCon()->CloseConsole(false, true);
			break;
		case IDM_TERMINATEGROUP:
			//apVCon->RCon()->CloseConsoleWindow();
			CVConGroup::CloseGroup(apVCon);
			break;
		case IDM_RESTART:
		case IDM_RESTARTAS:
		case IDM_RESTARTDLG:

			if (gpConEmu->isActive(apVCon))
			{
				gpConEmu->RecreateAction(cra_RecreateTab/*TRUE*/, (nCmd==IDM_RESTARTDLG) || isPressed(VK_SHIFT), (nCmd==IDM_RESTARTAS));
			}
			else
			{
				MBoxAssert(gpConEmu->isActive(apVCon));
			}

			break;
		case ID_NEWCONSOLE:
			gpConEmu->RecreateAction(gpSet->GetDefaultCreateAction(), gpSet->isMultiNewConfirm || isPressed(VK_SHIFT));
			break;
		case IDM_ATTACHTO:
			OnSysCommand(ghWnd, IDM_ATTACHTO, 0);
			break;
		case IDM_SAVE:
			apVCon->RCon()->PostMacro(L"F2");
			break;
		case IDM_SAVEALL:
			apVCon->RCon()->PostMacro(gpSet->sSaveAllMacro);
			break;
		default:
			if (nCmd >= 0xAB00)
			{
				// "Системные" команды, обрабатываемые в CConEmu
				OnSysCommand(ghWnd, nCmd, 0);
			}
	}
}

HMENU CConEmuMenu::GetSysMenu(BOOL abInitial /*= FALSE*/)
{
	HMENU hwndMain = NULL;
	//MENUITEMINFO mi = {sizeof(mi)};
	//wchar_t szText[255];

	
	if (gpConEmu->mp_Inside || gpSet->isQuakeStyle)
	{
		if (!mh_InsideSysMenu || abInitial)
		{
			if (mh_InsideSysMenu)
				DestroyMenu(mh_InsideSysMenu);

			mh_InsideSysMenu = CreatePopupMenu();
			AppendMenu(mh_InsideSysMenu, MF_STRING | MF_ENABLED, SC_CLOSE, MenuAccel(vkCloseConEmu,L"&Close ConEmu"));
		}
		hwndMain = mh_InsideSysMenu;
	}
	else
	{
		hwndMain = ::GetSystemMenu(ghWnd, FALSE);

		//// "Alt+F4" для пункта "Close" смысла не имеет
		//mi.fMask = MIIM_STRING; mi.dwTypeData = szText; mi.cch = countof(szText);
		//if (GetMenuItemInfo(hwndMain, SC_CLOSE, FALSE, &mi))
		//{
		//	wchar_t* psz = wcschr(szText, L'\t');
		//	if (psz)
		//	{
		//		*psz = 0;
		//		SetMenuItemInfo(hwndMain, SC_CLOSE, FALSE, &mi);
		//	}
		//}
	}

	UpdateSysMenu(hwndMain);

	return hwndMain;
}

void CConEmuMenu::UpdateSysMenu(HMENU hSysMenu)
{
	MENUITEMINFO mi = {sizeof(mi)};
	wchar_t szText[255];

	// "Alt+F4" для пункта "Close" смысла не имеет
	mi.fMask = MIIM_STRING; mi.dwTypeData = szText; mi.cch = countof(szText);
	if (GetMenuItemInfo(hSysMenu, SC_CLOSE, FALSE, &mi))
	{
		wchar_t* psz = wcschr(szText, L'\t');
		if (psz)
		{
			*psz = 0;
			//SetMenuItemInfo(hSysMenu, SC_CLOSE, FALSE, &mi);
		}
		mi.dwTypeData = (LPWSTR)MenuAccel(vkCloseConEmu,szText);
		if (lstrcmp(mi.dwTypeData, szText) != 0)
		{
			SetMenuItemInfo(hSysMenu, SC_CLOSE, FALSE, &mi);
		}
	}

	// В результате работы некоторых недобросовествных программ может сбиваться настроенное системное меню
	mi.fMask = MIIM_STRING; mi.dwTypeData = szText; mi.cch = countof(szText);
	if (!GetMenuItemInfo(hSysMenu, ID_NEWCONSOLE, FALSE, &mi))
	{
		if (!gpConEmu->mp_Inside)
		{
			if (!gpSet->isQuakeStyle)
				InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_TOMONITOR, _T("Bring &here"));
			InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_TOTRAY, TRAY_ITEM_HIDE_NAME/* L"Hide to &TSA" */);
		}
		InsertMenu(hSysMenu, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
		
		//InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_ABOUT, _T("&About / Help"));
		if (mh_HelpPopup) DestroyMenu(mh_HelpPopup);
		mh_HelpPopup = CreateHelpMenuPopup();
		InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_POPUP | MF_ENABLED, (UINT_PTR)mh_HelpPopup, _T("Hel&p"));
		//if (ms_ConEmuChm[0])  //Показывать пункт только если есть conemu.chm
		//	InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_HELP, _T("&Help"));

		// --------------------
		InsertMenu(hSysMenu, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
		
		if (mh_SysDebugPopup) DestroyMenu(mh_SysDebugPopup);
		mh_SysDebugPopup = CreateDebugMenuPopup();
		InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_POPUP | MF_ENABLED, (UINT_PTR)mh_SysDebugPopup, _T("&Debug"));
		
		if (mh_SysEditPopup) DestroyMenu(mh_SysEditPopup);
		mh_SysEditPopup = CreateEditMenuPopup(NULL);
		InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_POPUP | MF_ENABLED, (UINT_PTR)mh_SysEditPopup, _T("Ed&it"));
		
		// --------------------
		InsertMenu(hSysMenu, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
		
		if (mh_VConListPopup) DestroyMenu(mh_VConListPopup);
		mh_VConListPopup = CreateVConListPopupMenu(mh_VConListPopup, TRUE/*abFirstTabOnly*/);
		InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_POPUP | MF_ENABLED, (UINT_PTR)mh_VConListPopup, _T("Console &list"));
		
		if (mh_ActiveVConPopup) DestroyMenu(mh_ActiveVConPopup);
		if (mh_TerminateVConPopup) { DestroyMenu(mh_TerminateVConPopup); mh_TerminateVConPopup = NULL; }
		mh_ActiveVConPopup = CreateVConPopupMenu(NULL, NULL, FALSE, mh_TerminateVConPopup);
		InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_POPUP | MF_ENABLED, (UINT_PTR)mh_ActiveVConPopup, _T("Acti&ve console"));
		
		// --------------------
		InsertMenu(hSysMenu, 0, MF_BYPOSITION, MF_SEPARATOR, 0);
		if (!gpConEmu->mp_Inside)
		{
			InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED | (gpSet->isAlwaysOnTop ? MF_CHECKED : 0),
				ID_ALWAYSONTOP, MenuAccel(vkAlwaysOnTop,L"Al&ways on top"));
		}
		#ifdef SHOW_AUTOSCROLL
		InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED | (gpSetCls->AutoScroll ? MF_CHECKED : 0),
			ID_AUTOSCROLL, _T("Auto scro&ll"));
		#endif
		InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_SETTINGS, MenuAccel(vkWinAltP,L"S&ettings..."));
		InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, IDM_ATTACHTO, MenuAccel(vkMultiNewAttach,L"Attach t&o..."));
		InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED, ID_NEWCONSOLE, MenuAccel(vkMultiNew,L"New console..."));
	}
}

POINT CConEmuMenu::CalcTabMenuPos(CVirtualConsole* apVCon)
{
	POINT ptCur = {};
	if (apVCon)
	{
		RECT rcWnd;
		if (gpConEmu->mp_TabBar && gpConEmu->mp_TabBar->IsTabsShown())
		{
			gpConEmu->mp_TabBar->GetActiveTabRect(&rcWnd);
			ptCur.x = rcWnd.left;
			ptCur.y = rcWnd.bottom;
		}
		else
		{
			CVConGuard VCon;
			if (gpConEmu->GetActiveVCon(&VCon) >= 0)
			{
				GetWindowRect(VCon->GetView(), &rcWnd);
			}
			else
			{
				_ASSERTE(FALSE && "No Active VCon");
				GetWindowRect(ghWnd, &rcWnd);
			}

			ptCur.x = rcWnd.left;
			ptCur.y = rcWnd.top;
		}
	}
	return ptCur;
}

LRESULT CConEmuMenu::OnInitMenuPopup(HWND hWnd, HMENU hMenu, LPARAM lParam)
{
	// Уже должен быть выставлен тип меню, иначе не будут всплывать подсказки для пунктов меню
	_ASSERTE(mn_TrackMenuPlace != tmp_None);

	DefWindowProc(hWnd, WM_INITMENUPOPUP, (WPARAM)hMenu, lParam);

	MENUITEMINFO mi = {sizeof(mi)};
	wchar_t szText[255];
	mi.fMask = MIIM_STRING; mi.dwTypeData = szText; mi.cch = countof(szText);
	BOOL bIsSysMenu = GetMenuItemInfo(hMenu, SC_CLOSE, FALSE, &mi);


	if (HIWORD(lParam))
	{
		_ASSERTE(mn_TrackMenuPlace == tmp_System);

		// при всплытии "Help/Debug/..." submenu сюда мы тоже попадаем

		if (bIsSysMenu)
		{
			UpdateSysMenu(hMenu);			

			//BOOL bSelectionExist = FALSE;

			CVConGuard VCon;
			CVirtualConsole* pVCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon.VCon() : NULL;
			//if (pVCon && pVCon->RCon())
			//	bSelectionExist = pVCon->RCon()->isSelectionPresent();

			//EnableMenuItem(hMenu, ID_CON_COPY, MF_BYCOMMAND | (bSelectionExist?MF_ENABLED:MF_GRAYED));
			if (mh_SysEditPopup)
			{
				TODO("Проверить, сработает ли, если mh_EditPopup уже был вставлен в SystemMenu?");
				CreateEditMenuPopup(pVCon, mh_SysEditPopup);
			}
			else
			{
				_ASSERTE(mh_SysEditPopup!=NULL);
			}
			
			if (mh_VConListPopup)
			{
				CreateVConListPopupMenu(mh_VConListPopup, TRUE/*abFirstTabOnly*/);
			}
			else
			{
				_ASSERTE(mh_VConListPopup!=NULL);
			}
			
			if (mh_ActiveVConPopup)
			{
				CreateVConPopupMenu(NULL, mh_ActiveVConPopup, FALSE, mh_TerminateVConPopup);
			}
			else
			{
				_ASSERTE(mh_ActiveVConPopup!=NULL);
			}
			
			
			CheckMenuItem(hMenu, ID_DEBUG_SHOWRECTS, MF_BYCOMMAND|(gbDebugShowRects ? MF_CHECKED : MF_UNCHECKED));
			//#ifdef _DEBUG
			//		wchar_t szText[128];
			//		MENUITEMINFO mi = {sizeof(MENUITEMINFO)};
			//		mi.fMask = MIIM_STRING|MIIM_STATE;
			//		bool bLogged = false, bAllowed = false;
			//		CRealConsole* pRCon = mp_ VActive ? mp_ VActive->RCon() : NULL;
			//
			//		if (pRCon)
			//		{
			//			bLogged = pRCon->IsLogShellStarted();
			//			bAllowed = (pRCon->GetFarPID(TRUE) != 0);
			//		}
			//
			//		lstrcpy(szText, bLogged ? _T("Disable &shell log") : _T("Enable &shell log..."));
			//		mi.dwTypeData = szText;
			//		mi.fState = bAllowed ? MFS_ENABLED : MFS_GRAYED;
			//		SetMenuItemInfo(hMenu, ID_MONITOR_SHELLACTIVITY, FALSE, &mi);
			//#endif
		}
	}

	return 0;
}

int CConEmuMenu::trackPopupMenu(TrackMenuPlace place, HMENU hMenu, UINT uFlags, int x, int y, HWND hWnd, RECT *prcRect /* = NULL*/)
{
	gpConEmu->mp_Tip->HideTip();
	TrackMenuPlace prevPlace = mn_TrackMenuPlace;
	if (prevPlace == place)
	{
		_ASSERTE(prevPlace==tmp_System);
		prevPlace = tmp_None; 
	}
	_ASSERTE(prevPlace==tmp_None || prevPlace==tmp_Cmd);

	mn_TrackMenuPlace = place;

	TPMPARAMS ex = {sizeof(ex)};
	if (prcRect)
		ex.rcExclude = *prcRect;
	else
		ex.rcExclude = MakeRect(x-1,y-1,x+1,y+1);

	if (!(uFlags & (TPM_HORIZONTAL|TPM_VERTICAL)))
		uFlags |= TPM_HORIZONTAL;

	int cmd = TrackPopupMenuEx(hMenu, uFlags, x, y, hWnd, &ex);

	mn_TrackMenuPlace = prevPlace;

	gpConEmu->mp_Tip->HideTip();

	return cmd;
}

void CConEmuMenu::ShowMenuHint(HMENU hMenu, WORD nID, WORD nFlags)
{
	if (nID && (nID != MF_SEPARATOR) && !(nFlags & MF_POPUP))
	{
		//POINT pt; GetCursorPos(&pt);
		RECT rcMenuItem = {};
		BOOL lbMenuItemPos = FALSE;
		UINT nMenuID = 0;
		for (int i = 0; i < 100; i++)
		{
			nMenuID = GetMenuItemID(hMenu, i);
			if (nMenuID == nID)
			{
				lbMenuItemPos = GetMenuItemRect(ghWnd, hMenu, i, &rcMenuItem);
				break;
			}
		}
		if (lbMenuItemPos)
		{
			POINT pt = {rcMenuItem.left + (rcMenuItem.bottom - rcMenuItem.top)*2, rcMenuItem.bottom};
			//pt.x = rcMenuItem.left; //(rcMenuItem.left + rcMenuItem.right) >> 1;
			//pt.y = rcMenuItem.bottom;
			TCHAR szText[0x200];
			if (LoadString(g_hInstance, nMenuID, szText, countof(szText)))
			{
				gpConEmu->mp_Tip->ShowTip(ghWnd, ghWnd, szText, TRUE, pt, g_hInstance);
				return;
			}
		}
	}

	gpConEmu->mp_Tip->HideTip();
}

void CConEmuMenu::ShowKeyBarHint(HMENU hMenu, WORD nID, WORD nFlags)
{
	if (nID && (nID != MF_SEPARATOR) && !(nFlags & MF_POPUP))
	{
		CVConGuard VCon;
		CVirtualConsole* pVCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon.VCon() : NULL;
		if (pVCon && pVCon->RCon())
			pVCon->RCon()->ShowKeyBarHint(nID);
	}
}

void CConEmuMenu::OnNcIconLClick()
{
	DWORD nCurTick = GetTickCount();
	DWORD nOpenDelay = nCurTick - mn_SysMenuOpenTick;
	DWORD nCloseDelay = nCurTick - mn_SysMenuCloseTick;
	DWORD nDoubleTime = GetDoubleClickTime();

	if (mn_SysMenuOpenTick && (nOpenDelay < nDoubleTime))
	{
		gpConEmu->PostScClose();
	}
	else if (mn_SysMenuCloseTick && (nCloseDelay < (nDoubleTime/2)))
	{
		// Пропустить - кликом закрыли меню
		#ifdef _DEBUG
		int nDbg = 0;
		#endif
	}
	// When Alt is hold down - sysmenu will be immediately closed
	else if (!isPressed(VK_MENU))
	{
		ShowSysmenu();
	}
}

void CConEmuMenu::ShowSysmenu(int x, int y, bool bAlignUp /*= false*/)
{
	//if (!Wnd)
	//	Wnd = ghWnd;

	WARNING("SysMenu: Обработать DblClick по иконке!");

	if ((x == -32000) || (y == -32000))
	{
		RECT rect, cRect;
		GetWindowRect(ghWnd, &rect);
		cRect = gpConEmu->GetGuiClientRect();
		WINDOWINFO wInfo;   GetWindowInfo(ghWnd, &wInfo);
		int nTabShift =
		    ((gpSet->isCaptionHidden()) && gpConEmu->mp_TabBar->IsTabsShown() && (gpSet->nTabsLocation != 1))
		    ? gpConEmu->mp_TabBar->GetTabbarHeight() : 0;

		if (x == -32000)
			x = rect.right - cRect.right - wInfo.cxWindowBorders;

		if (y == -32000)
			y = rect.bottom - cRect.bottom - wInfo.cyWindowBorders + nTabShift;
	}

	bool iconic = gpConEmu->isIconic();
	bool zoomed = gpConEmu->isZoomed();
	bool visible = IsWindowVisible(ghWnd);
	int style = GetWindowLong(ghWnd, GWL_STYLE);
	HMENU systemMenu = GetSysMenu();

	if (!systemMenu)
		return;

	if (!gpConEmu->mp_Inside)
	{
		EnableMenuItem(systemMenu, SC_RESTORE,
		               MF_BYCOMMAND | ((visible && (iconic || zoomed)) ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(systemMenu, SC_MOVE,
		               MF_BYCOMMAND | ((visible && !(iconic || zoomed)) ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(systemMenu, SC_SIZE,
		               MF_BYCOMMAND | ((visible && (!(iconic || zoomed) && (style & WS_SIZEBOX))) ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(systemMenu, SC_MINIMIZE,
		               MF_BYCOMMAND | ((visible && (!iconic && (style & WS_MINIMIZEBOX))) ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(systemMenu, SC_MAXIMIZE,
		               MF_BYCOMMAND | ((visible && (!zoomed && (style & WS_MAXIMIZEBOX))) ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(systemMenu, ID_TOTRAY, MF_BYCOMMAND | MF_ENABLED);
	}

	mn_TrackMenuPlace = tmp_System;
	SendMessage(ghWnd, WM_INITMENU, (WPARAM)systemMenu, 0);
	SendMessage(ghWnd, WM_INITMENUPOPUP, (WPARAM)systemMenu, MAKELPARAM(0, true));

	// Переехало в OnMenuPopup
	//BOOL bSelectionExist = ActiveCon()->RCon()->isSelectionPresent();
	//EnableMenuItem(systemMenu, ID_CON_COPY, MF_BYCOMMAND | (bSelectionExist?MF_ENABLED:MF_GRAYED));
	SetActiveWindow(ghWnd);
	//mb_InTrackSysMenu = TRUE;
	mn_SysMenuOpenTick = GetTickCount();
	POINT ptCurBefore = {}; GetCursorPos(&ptCurBefore);

	int command = trackPopupMenu(tmp_System, systemMenu,
		 TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON | (bAlignUp ? TPM_BOTTOMALIGN : 0),
		 x, y, ghWnd);
	//mb_InTrackSysMenu = FALSE;
	if (command == 0)
	{
		mn_SysMenuCloseTick = GetTickCount();

		if ((mn_SysMenuCloseTick - mn_SysMenuOpenTick) < GetDoubleClickTime())
		{
			POINT ptCur = {}; GetCursorPos(&ptCur);
			if (gpConEmu->PtDiffTest(ptCur, ptCurBefore.x, ptCurBefore.y, 8))
			{
				LRESULT lHitTest = SendMessage(ghWnd, WM_NCHITTEST, 0, MAKELONG(ptCur.x,ptCur.y));
				if (lHitTest == HTSYSMENU)
				{
					command = SC_CLOSE;
				}
			}
		}
	}
	else
	{
		mn_SysMenuCloseTick = 0;
	}

	if (Icon.isWindowInTray() && gpConEmu->isIconic())
	{
		_ASSERTE(!gpConEmu->mp_Inside);

		switch (command)
		{
			case SC_CLOSE:
				break;

			//case SC_RESTORE:
			//case SC_MOVE:
			//case SC_SIZE:
			//case SC_MINIMIZE:
			//case SC_MAXIMIZE:
			default:
				//SendMessage(ghWnd, WM_TRAYNOTIFY, 0, WM_LBUTTONDOWN);
				//Icon.OnTryIcon(ghWnd, WM_TRAYNOTIFY, 0, WM_LBUTTONDOWN);
				gpConEmu->OnMinimizeRestore(sih_Show);
				break;
		}
	}

	if (command)
	{
		PostMessage(ghWnd, WM_SYSCOMMAND, (WPARAM)command, 0);
	}
}

HMENU CConEmuMenu::CreateDebugMenuPopup()
{
	HMENU hDebug = CreatePopupMenu();
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_CON_TOGGLE_VISIBLE, MenuAccel(vkCtrlWinAltSpace,L"&Real console"));
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_CONPROP, _T("&Properties..."));
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_SCREENSHOT, MenuAccel(vkScreenshot,L"Make &screenshot..."));
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_DUMPCONSOLE, _T("&Dump screen..."));
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_LOADDUMPCONSOLE, _T("&Load screen dump..."));
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_DEBUGGUI, _T("Debug &log (GUI)"));
//#ifdef _DEBUG
//	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_MONITOR_SHELLACTIVITY, _T("Enable &shell log..."));
//#endif
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_DEBUG_SHOWRECTS, _T("Show debug rec&ts"));
	AppendMenu(hDebug, MF_SEPARATOR, 0, NULL);
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_DEBUG_TRAP, _T("Raise exception (Main thread)"));
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_DEBUG_TRAP2, _T("Raise exception (Monitor thread)"));
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_DEBUG_ASSERT, _T("Show assertion"));
	#ifdef TRACK_MEMORY_ALLOCATIONS
	AppendMenu(hDebug, MF_SEPARATOR, 0, NULL);
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_DUMP_MEM_BLK, _T("Dump used memory blocks"));
	#endif
	AppendMenu(hDebug, MF_SEPARATOR, 0, NULL);
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_DEBUGCON, _T("Debug &active process"));
	AppendMenu(hDebug, MF_STRING | MF_ENABLED, ID_MINIDUMP, _T("Active process &memory dump..."));
	return hDebug;
}

HMENU CConEmuMenu::CreateVConListPopupMenu(HMENU ahExist, BOOL abFirstTabOnly)
{
	HMENU h = ahExist ? ahExist : CreatePopupMenu();
	wchar_t szText[128];
	const int nMaxStrLen = 32;

	BOOL lbActiveVCon = FALSE;
	int nActiveCmd = -1; // DWORD MAKELONG(WORD wLow,WORD wHigh);
	CVirtualConsole* pVCon = NULL;
	DWORD nAddFlags = 0;
	
	if (ahExist)
	{
		while (DeleteMenu(ahExist, 0, MF_BYPOSITION))
			;
	}
	
	for (int V = 0; (pVCon = gpConEmu->GetVCon(V, true))!=NULL; V++)
	{
		if ((lbActiveVCon = gpConEmu->isActive(pVCon)))
			nActiveCmd = MAKELONG(1, V+1);
		nAddFlags = 0; //(lbActiveVCon ? MF_DEFAULT : 0);
		CRealConsole* pRCon = pVCon->RCon();
		if (!pRCon)
		{
			wsprintf(szText, L"%i: VConsole", V+1);
			AppendMenu(h, MF_STRING|nAddFlags, MAKELONG(1, V+1), szText);
		}
		else
		{
			ConEmuTab tab = {};
			int R = 0;
			if (!pRCon->GetTab(R, &tab))
			{
				wsprintf(szText, L"%i: RConsole", V+1);
				AppendMenu(h, MF_STRING|nAddFlags, MAKELONG(1, V+1), szText);
			}
			else
			{
				do
				{
					nAddFlags = 0/*((lbActiveVCon && (R==0)) ? MF_DEFAULT : 0)*/
						| ((lbActiveVCon && (abFirstTabOnly || pRCon->GetActiveTab() == R)) ? MF_CHECKED : MF_UNCHECKED)
						#if 0
						| ((tab->Flags() & etfDisabled) ? (MF_DISABLED|MF_GRAYED) : 0)
						#endif
						;
					int nLen = lstrlen(tab.Name/*.Ptr()*/);
					if (!R)
						wsprintf(szText, L"%i: ", V+1);
					else
						wcscpy_c(szText, L"      ");
					if (nLen <= nMaxStrLen)
					{
						wcscat_c(szText, tab.Name/*.Ptr()*/);
					}
					else
					{
						int nCurLen = lstrlen(szText);
						_ASSERTE((nCurLen+10)<nMaxStrLen);
						if ((tab.Type & fwt_TypeMask) == fwt_Panels)
						{
							lstrcpyn(szText+nCurLen, tab.Name/*.Ptr()*/, nMaxStrLen-1-nCurLen);
						}
						else
						{
							szText[nCurLen++] = L'\x2026'; szText[nCurLen] = 0;
							lstrcpyn(szText+nCurLen, tab.Name+nLen-nMaxStrLen, nMaxStrLen-1-nCurLen);
						}
						wcscat_c(szText, L"\x2026"); //...
					}
					AppendMenu(h, MF_STRING|nAddFlags, MAKELONG(R+1, V+1), szText);
				} while (!abFirstTabOnly && pRCon->GetTab(++R, &tab));
			}
		}
	}

	if (nActiveCmd != -1 && !abFirstTabOnly)
	{
		MENUITEMINFO mi = {sizeof(mi), MIIM_STATE|MIIM_ID};
		mi.wID = nActiveCmd;
		GetMenuItemInfo(h, nActiveCmd, FALSE, &mi);
		mi.fState |= MF_DEFAULT;
		SetMenuItemInfo(h, nActiveCmd, FALSE, &mi);
	}
	
	return h;
}

HMENU CConEmuMenu::CreateVConPopupMenu(CVirtualConsole* apVCon, HMENU ahExist, BOOL abAddNew, HMENU& hTerminate)
{
	//BOOL lbEnabled = TRUE;
	HMENU hMenu = ahExist;
	
	CVConGuard VCon;
	if (!apVCon && (CVConGroup::GetActiveVCon(&VCon) >= 0))
		apVCon = VCon.VCon();

	if (!hTerminate)
		hTerminate = CreatePopupMenu();
	
	if (!hMenu)
	{	
		hMenu = CreatePopupMenu();
		
		/*
        MENUITEM "&Close",                      IDM_CLOSE
        MENUITEM "Detach",                      IDM_DETACH
        MENUITEM "&Terminate",                  IDM_TERMINATE
        MENUITEM SEPARATOR
        MENUITEM "&Restart",                    IDM_RESTART
        MENUITEM "Restart as...",               IDM_RESTARTAS
        MENUITEM SEPARATOR
        MENUITEM "New console...",              IDM_NEW
        MENUITEM SEPARATOR
        MENUITEM "&Save",                       IDM_SAVE
        MENUITEM "Save &all",                   IDM_SAVEALL
		*/
		
		AppendMenu(hMenu, MF_STRING | MF_ENABLED,     IDM_CLOSE,     MenuAccel(vkCloseTab,L"&Close tab"));
		AppendMenu(hMenu, MF_STRING | MF_ENABLED,     IDM_DETACH,    L"Detach");
		AppendMenu(hMenu, MF_STRING | MF_ENABLED,     IDM_DUPLICATE, MenuAccel(vkDuplicateRoot,L"Duplica&te root..."));
		//AppendMenu(hMenu, MF_STRING | MF_ENABLED,     IDM_ADMIN_DUPLICATE, MenuAccel(vkDuplicateRootAs,L"Duplica&te as Admin..."));
		AppendMenu(hMenu, MF_STRING | MF_ENABLED,     IDM_RENAMETAB, MenuAccel(vkRenameTab,L"Rena&me tab..."));
		AppendMenu(hTerminate, MF_STRING | MF_ENABLED, IDM_TERMINATECON, MenuAccel(vkMultiClose,L"&Console"));
		AppendMenu(hTerminate, MF_STRING | MF_ENABLED, IDM_TERMINATEGROUP, MenuAccel(vkCloseGroup,L"Active &group"));
		AppendMenu(hTerminate, MF_SEPARATOR, 0, L"");
		AppendMenu(hTerminate, MF_STRING | MF_ENABLED, IDM_TERMINATEPRC, MenuAccel(vkTerminateApp,L"&Active process"));
		AppendMenu(hMenu, MF_POPUP | MF_ENABLED, (UINT_PTR)hTerminate, L"&Terminate");
		AppendMenu(hMenu, MF_STRING | ((apVCon && apVCon->GuiWnd()) ? MF_ENABLED : 0),     IDM_CHILDSYSMENU,    MenuAccel(vkChildSystemMenu,L"Child system menu..."));
		AppendMenu(hMenu, MF_SEPARATOR, 0, L"");
		AppendMenu(hMenu, MF_STRING | MF_ENABLED,     IDM_RESTARTDLG, MenuAccel(vkMultiRecreate,L"&Restart..."));
		AppendMenu(hMenu, MF_STRING | MF_ENABLED,     IDM_RESTART,    L"&Restart");
		AppendMenu(hMenu, MF_STRING | MF_ENABLED,     IDM_RESTARTAS,  L"Restart as Admin");
		if (abAddNew)
		{
			AppendMenu(hMenu, MF_SEPARATOR, 0, L"");
			AppendMenu(hMenu, MF_STRING | MF_ENABLED, ID_NEWCONSOLE, MenuAccel(vkMultiNew,L"New console..."));
			AppendMenu(hMenu, MF_STRING | MF_ENABLED, IDM_ATTACHTO,  MenuAccel(vkMultiNewAttach,L"Attach to..."));
		}
		#if 0
		// Смысл выносить избранный макро заточенный только под редактор Far?
		AppendMenu(hMenu, MF_SEPARATOR, 0, L"");
		AppendMenu(hMenu, MF_STRING | MF_ENABLED,     IDM_SAVE,      L"&Save");
		AppendMenu(hMenu, MF_STRING | MF_ENABLED,     IDM_SAVEALL,   L"Save &all");
		#endif
	}

	if (apVCon)
	{
		bool lbIsFar = apVCon->RCon()->isFar(TRUE/* abPluginRequired */)!=FALSE;
		#ifdef _DEBUG
		bool lbIsPanels = lbIsFar && apVCon->RCon()->isFilePanel(false/* abPluginAllowed */)!=FALSE;
		#endif
		bool lbIsEditorModified = lbIsFar && apVCon->RCon()->isEditorModified()!=FALSE;
		bool lbHaveModified = lbIsFar && apVCon->RCon()->GetModifiedEditors()!=0;
		bool lbCanCloseTab = apVCon->RCon()->CanCloseTab();

		if (lbHaveModified)
		{
			if (!gpSet->sSaveAllMacro || !*gpSet->sSaveAllMacro)
				lbHaveModified = false;
		}

		EnableMenuItem(hMenu, IDM_CLOSE, MF_BYCOMMAND | (lbCanCloseTab ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(hMenu, IDM_DETACH, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_DUPLICATE, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_RENAMETAB, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hTerminate, IDM_TERMINATECON, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hTerminate, IDM_TERMINATEGROUP, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hTerminate, IDM_TERMINATEPRC, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_CHILDSYSMENU, MF_BYCOMMAND | ((apVCon && apVCon->GuiWnd()) ? 0 : MF_GRAYED));
		EnableMenuItem(hMenu, IDM_RESTARTDLG, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_RESTART, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_RESTARTAS, MF_BYCOMMAND | MF_ENABLED);
		//EnableMenuItem(hMenu, IDM_ADMIN_DUPLICATE, MF_BYCOMMAND | (lbIsPanels ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(hMenu, IDM_SAVE, MF_BYCOMMAND | (lbIsEditorModified ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(hMenu, IDM_SAVEALL, MF_BYCOMMAND | (lbHaveModified ? MF_ENABLED : MF_GRAYED));
	}
	else
	{
		EnableMenuItem(hMenu, IDM_CLOSE, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_DETACH, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_DUPLICATE, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_RENAMETAB, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hTerminate, IDM_TERMINATECON, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hTerminate, IDM_TERMINATEGROUP, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hTerminate, IDM_TERMINATEPRC, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_CHILDSYSMENU, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_RESTARTDLG, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_RESTART, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_RESTARTAS, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SAVE, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SAVEALL, MF_BYCOMMAND | MF_GRAYED);
	}

	return hMenu;
}

//void CConEmuMenu::PopulateEditMenuPopup(HMENU hMenu)
HMENU CConEmuMenu::CreateEditMenuPopup(CVirtualConsole* apVCon, HMENU ahExist /*= NULL*/)
{
	CVConGuard VCon;
	if (!apVCon && (CVConGroup::GetActiveVCon(&VCon) >= 0))
		apVCon = VCon.VCon();
		
	BOOL lbEnabled = TRUE;
	BOOL lbSelectionExist = FALSE;
	if (apVCon && apVCon->RCon())
	{
		if (apVCon->RCon()->GuiWnd() && !apVCon->RCon()->isBufferHeight())
			lbEnabled = FALSE; // Если видимо дочернее графическое окно - выделение смысла не имеет
		// Нужно ли серить пункт "Copy"
		lbSelectionExist = lbEnabled && apVCon->RCon()->isSelectionPresent();
	}

	HMENU hMenu = ahExist;
	
	if (!hMenu)
	{	
		hMenu = CreatePopupMenu();
		AppendMenu(hMenu, MF_STRING | (lbEnabled?MF_ENABLED:MF_GRAYED), ID_CON_MARKBLOCK, MenuAccel(vkCTSVkBlockStart,L"Mark &block"));
		AppendMenu(hMenu, MF_STRING | (lbEnabled?MF_ENABLED:MF_GRAYED), ID_CON_MARKTEXT, MenuAccel(vkCTSVkTextStart,L"Mar&k text"));
		AppendMenu(hMenu, MF_STRING | (lbSelectionExist?MF_ENABLED:MF_GRAYED), ID_CON_COPY, L"Cop&y");
		AppendMenu(hMenu, MF_STRING | (lbEnabled?MF_ENABLED:MF_GRAYED), ID_CON_COPY_ALL, MenuAccel(vkCTSVkCopyAll,L"Copy &all"));
		AppendMenu(hMenu, MF_STRING | (lbEnabled?MF_ENABLED:MF_GRAYED), ID_CON_PASTE, L"&Paste");
		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(hMenu, MF_STRING | (lbEnabled?MF_ENABLED:MF_GRAYED), ID_CON_FIND, MenuAccel(vkFindTextDlg,L"&Find text..."));
	}
	else
	{
		EnableMenuItem(hMenu, ID_CON_MARKBLOCK, MF_BYCOMMAND | (lbEnabled?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(hMenu, ID_CON_MARKTEXT, MF_BYCOMMAND | (lbEnabled?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(hMenu, ID_CON_COPY, MF_BYCOMMAND | (lbSelectionExist?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(hMenu, ID_CON_COPY_ALL, MF_BYCOMMAND | (lbEnabled?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(hMenu, ID_CON_PASTE, MF_BYCOMMAND | (lbEnabled?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(hMenu, ID_CON_FIND, MF_BYCOMMAND | (lbEnabled?MF_ENABLED:MF_GRAYED));
	}
	
	return hMenu;
}

HMENU CConEmuMenu::CreateHelpMenuPopup()
{
	HMENU hHelp = CreatePopupMenu();

	if (!gpConEmu->mb_MingwMode)
	{
		if (gpUpd && gpUpd->InUpdate())
			AppendMenu(hHelp, MF_STRING | MF_ENABLED, ID_STOPUPDATE, _T("&Stop updates checking"));
		else
			AppendMenu(hHelp, MF_STRING | MF_ENABLED, ID_CHECKUPDATE, _T("&Check for updates"));

		AppendMenu(hHelp, MF_SEPARATOR, 0, NULL);
	}
	
	AppendMenu(hHelp, MF_STRING | MF_ENABLED, ID_HOMEPAGE, _T("&Visit home page"));
	AppendMenu(hHelp, MF_STRING | MF_ENABLED, ID_DONATE_LINK, _T("&Donate (PayPal)"));

	AppendMenu(hHelp, MF_STRING | MF_ENABLED, ID_WHATS_NEW_FILE, _T("Whats &new (local)"));
	AppendMenu(hHelp, MF_STRING | MF_ENABLED, ID_WHATS_NEW_WWW, _T("Whats new (&web)"));
	AppendMenu(hHelp, MF_SEPARATOR, 0, NULL);

	AppendMenu(hHelp, MF_STRING | MF_ENABLED, ID_REPORTBUG, _T("&Report a bug..."));
	
	if (gpConEmu->ms_ConEmuChm[0])  //Показывать пункт только если есть conemu.chm
		AppendMenu(hHelp, MF_STRING | MF_ENABLED, ID_HELP, _T("&Help"));

	AppendMenu(hHelp, MF_SEPARATOR, 0, NULL);
	AppendMenu(hHelp, MF_STRING | MF_ENABLED, ID_HOTKEYS, MenuAccel(vkWinAltK,L"Hot&keys"));
	AppendMenu(hHelp, MF_STRING | MF_ENABLED, ID_ABOUT, MenuAccel(vkWinAltA,L"&About / Help"));
	
	return hHelp;
}

LPCWSTR CConEmuMenu::MenuAccel(int DescrID, LPCWSTR asText)
{
	if (!asText || !*asText)
	{
		_ASSERTE(asText!=NULL);
		return L"";
	}

	static wchar_t szTemp[255];
	wchar_t szKey[128] = {};
	
	const ConEmuHotKey* pHK = NULL;
	DWORD VkMod = gpSet->GetHotkeyById(DescrID, &pHK);
	if (!ConEmuHotKey::GetHotkey(VkMod) || !pHK)
		return asText;

	pHK->GetHotkeyName(szKey);
	if (!*szKey)
		return asText;
	int nLen = lstrlen(szKey);
	lstrcpyn(szTemp, asText, countof(szTemp)-nLen-4);
	wcscat_c(szTemp, L"\t");
	wcscat_c(szTemp, szKey);

	return szTemp;
}

LRESULT CConEmuMenu::OnSysCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
#ifdef _DEBUG
	wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"OnSysCommand (%i(0x%X), %i)\n", (DWORD)wParam, (DWORD)wParam, (DWORD)lParam);
	DEBUGSTRSIZE(szDbg);
	if (wParam == SC_HOTKEY)
	{
		_ASSERTE(wParam!=SC_HOTKEY);
	}
#endif
	LRESULT result = 0;

	if (wParam >= IDM_VCON_FIRST && wParam <= IDM_VCON_LAST)
	{
		int nNewV = ((int)HIWORD(wParam))-1;
		int nNewR = ((int)LOWORD(wParam))-1; UNREFERENCED_PARAMETER(nNewR);
		
		CVirtualConsole* pVCon = gpConEmu->GetVCon(nNewV);
		if (pVCon)
		{
			// -- в SysMenu показываются только консоли (редакторов/вьюверов там нет)
			//CRealConsole* pRCon = pVCon->RCon();
			//if (pRCon)
			//{
			//	//if (pRCon->CanActivateFarWindow(nNewR))
			//	pRCon->ActivateFarWindow(nNewR);
			//}
			if (!gpConEmu->isActive(pVCon))
				gpConEmu->Activate(pVCon);
			//else
			//	UpdateTabs();
		}
		return 0;
	}

	//switch(LOWORD(wParam))
	switch (wParam)
	{
		case ID_NEWCONSOLE:
			// Создать новую консоль
			gpConEmu->RecreateAction(gpSet->GetDefaultCreateAction(), gpSet->isMultiNewConfirm || isPressed(VK_SHIFT));
			return 0;
			
		case IDM_ATTACHTO:
			gpConEmu->AttachToDialog();
			return 0;
			
		case ID_SETTINGS:
			CSettings::Dialog();
			return 0;
			
		case ID_CON_PASTE:
			{
				CVConGuard VCon;
				if (gpConEmu->GetActiveVCon(&VCon) >= 0)
				{
					VCon->RCon()->Paste();
				}
			}
			return 0;

		case ID_CON_FIND:
			gpConEmu->mp_Find->FindTextDialog();
			return 0;

		case ID_CON_COPY:
		case ID_CON_COPY_ALL:
			{
				CVConGuard VCon;
				if (gpConEmu->GetActiveVCon(&VCon) >= 0)
				{
					bool bCopyAll = (wParam == ID_CON_COPY_ALL);
					VCon->RCon()->DoSelectionCopy(bCopyAll);
				}
			}
			return 0;
			
		case ID_CON_MARKBLOCK:
		case ID_CON_MARKTEXT:
			{
				CVConGuard VCon;
				if (gpConEmu->GetActiveVCon(&VCon) >= 0)
				{
					VCon->RCon()->StartSelection(LOWORD(wParam) == ID_CON_MARKTEXT);
				}
			}
			return 0;
			
		#ifdef SHOW_AUTOSCROLL
		case ID_AUTOSCROLL:
			gpSetCls->AutoScroll = !gpSetCls->AutoScroll;
			CheckMenuItem(gpConEmu->mp_Menu->GetSysMenu(), ID_AUTOSCROLL, MF_BYCOMMAND |
			              (gpSetCls->AutoScroll ? MF_CHECKED : MF_UNCHECKED));
			return 0;
		#endif
			
		case ID_ALWAYSONTOP:
			{
				gpSet->isAlwaysOnTop = !gpSet->isAlwaysOnTop;
				gpConEmu->OnAlwaysOnTop();

				HWND hExt = gpSetCls->mh_Tabs[gpSetCls->thi_Ext];

				if (ghOpWnd && hExt)
				{
					CheckDlgButton(hExt, cbAlwaysOnTop, gpSet->isAlwaysOnTop ? BST_CHECKED : BST_UNCHECKED);
				}
			}
			return 0;
			
		case ID_DUMPCONSOLE:
			{
				CVConGuard VCon;
				if (gpConEmu->GetActiveVCon(&VCon) >= 0)
				{
					VCon->DumpConsole();
				}
			 }
			return 0;

		case ID_SCREENSHOT:
			CConEmuCtrl::MakeScreenshot();

			return 0;

		case ID_LOADDUMPCONSOLE:
			{
				CVConGuard VCon;
				if (gpConEmu->GetActiveVCon(&VCon) >= 0)
				{
					VCon->LoadDumpConsole();
				}
			}
			return 0;
			
		case ID_DEBUGGUI:
			gpConEmu->StartDebugLogConsole();
			return 0;
			
		case ID_DEBUGCON:
			gpConEmu->StartDebugActiveProcess();
			return 0;
		case ID_MINIDUMP:
			gpConEmu->MemoryDumpActiveProcess();
			return 0;
			
		//case ID_MONITOR_SHELLACTIVITY:
		//{
		//	CRealConsole* pRCon = mp_ VActive->RCon();

		//	if (pRCon)
		//		pRCon->LogShellStartStop();

		//	//if (!mb_CreateProcessLogged)
		//	//	StartLogCreateProcess();
		//	//else
		//	//	StopLogCreateProcess();
		//}
		//return 0;
		
		case ID_DEBUG_SHOWRECTS:
			gbDebugShowRects = !gbDebugShowRects;
			gpConEmu->InvalidateAll();
			return 0;

		case ID_DEBUG_TRAP:
			if (MessageBox(L"Are you sure?\nApplication will terminates after that!\nThrow exception in ConEmu's main thread?", MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2)==IDYES)
			{
				#ifdef _DEBUG
				MyAssertTrap();
				#else
				DebugBreak();
				#endif
			}
			return 0;
		case ID_DEBUG_TRAP2:
			if (MessageBox(L"Are you sure?\nApplication will terminates after that!\nThrow exception in ConEmu's monitor thread?", MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2)==IDYES)
			{
				CVConGuard VCon;
				if ((gpConEmu->GetActiveVCon(&VCon) >= 0) && VCon->RCon())
					VCon->RCon()->MonitorAssertTrap();
			}
			return 0;
		case ID_DEBUG_ASSERT:
			Assert(FALSE && "This is test assertion");
			return 0;

		case ID_DUMP_MEM_BLK:
			#ifdef TRACK_MEMORY_ALLOCATIONS
			xf_dump();
			#else
			_ASSERTE(FALSE && "TRACK_MEMORY_ALLOCATIONS not defined");
			#endif
			return 0;
			
		case ID_CON_TOGGLE_VISIBLE:
			{
				CVConGuard VCon;
				if (gpConEmu->GetActiveVCon(&VCon) >= 0)
				{
					VCon->RCon()->ShowConsoleOrGuiClient(-1); // Toggle visibility
				}
			}
			return 0;
			
		case ID_HELP:
		{
			gpConEmu->OnInfo_Help();
			return 0;
		} // case ID_HELP:
		
		case ID_HOMEPAGE:
		{
			gpConEmu->OnInfo_HomePage();
			return 0;
		}
		
		case ID_REPORTBUG:
		{
			gpConEmu->OnInfo_ReportBug();
			return 0;
		}
		
		case ID_CHECKUPDATE:
			gpConEmu->CheckUpdates(TRUE);
			return 0;
		
		case ID_STOPUPDATE:
			if (gpUpd)
				gpUpd->StopChecking();
			return 0;
		
		case ID_HOTKEYS:
		{
			CSettings::Dialog(IDD_SPG_KEYS);
			return 0;
		}

		case ID_DONATE_LINK:
		{
			gpConEmu->OnInfo_Donate();
			return 0;
		}

		case ID_ABOUT:
		{
			gpConEmu->OnInfo_About();
			return 0;
		}

		case ID_WHATS_NEW_FILE:
		case ID_WHATS_NEW_WWW:
		{
			gpConEmu->OnInfo_WhatsNew((wParam == ID_WHATS_NEW_FILE));
			return 0;
		}

		case ID_TOMONITOR:
		{
			if (!gpConEmu->IsSizeFree())
				return 0;
			if (!IsWindowVisible(ghWnd))
				Icon.RestoreWindowFromTray();
			POINT ptCur = {}; GetCursorPos(&ptCur);
			HMONITOR hMon = MonitorFromPoint(ptCur, MONITOR_DEFAULTTOPRIMARY);
			MONITORINFO mi = {sizeof(mi)};
			GetMonitorInfo(hMon, &mi);
			SetWindowPos(ghWnd, HWND_TOP, mi.rcWork.left, mi.rcWork.top, 0,0, SWP_NOSIZE);
			return 0;
		}

		case ID_TOTRAY:
			if (IsWindowVisible(ghWnd))
				Icon.HideWindowToTray();
			else
				Icon.RestoreWindowFromTray();

			return 0;
			
		case ID_CONPROP:
		{
			CVConGuard VCon;
			if ((gpConEmu->GetActiveVCon(&VCon) >= 0) && VCon->RCon())
			{
				#ifdef MSGLOGGER
				{
					// Для отладки, посмотреть, какие пункты меню есть в RealConsole
					HMENU hMenu = ::GetSystemMenu(VCon->RCon()->ConWnd(), FALSE);
					MENUITEMINFO mii; TCHAR szText[255];

					for(int i=0; i<15; i++)
					{
						memset(&mii, 0, sizeof(mii));
						mii.cbSize = sizeof(mii); mii.dwTypeData=szText; mii.cch=255;
						mii.fMask = MIIM_ID|MIIM_STRING|MIIM_SUBMENU;

						if (GetMenuItemInfo(hMenu, i, TRUE, &mii))
						{
							mii.cbSize = sizeof(mii);

							if (mii.hSubMenu)
							{
								MENUITEMINFO mic;

								for(int i=0; i<15; i++)
								{
									memset(&mic, 0, sizeof(mic));
									mic.cbSize = sizeof(mic); mic.dwTypeData=szText; mic.cch=255;
									mic.fMask = MIIM_ID|MIIM_STRING;

									if (GetMenuItemInfo(mii.hSubMenu, i, TRUE, &mic))
									{
										mic.cbSize = sizeof(mic);
									}
									else
									{
										break;
									}
								}
							}
						}
						else
							break;
					}
				}
				#endif
			
			    // Go!
				VCon->RCon()->ShowPropertiesDialog();
			}
			return 0;
		} // case ID_CONPROP:

		case SC_MAXIMIZE_SECRET:
			gpConEmu->SetWindowMode(wmMaximized);
			break;
			
		case SC_RESTORE_SECRET:
			gpConEmu->SetWindowMode(wmNormal);
			break;
			
		case SC_CLOSE:
			CVConGroup::OnScClose();
			break;
		
		case SC_MAXIMIZE:
		{
			DEBUGSTRSYS(L"OnSysCommand(SC_MAXIMIZE)\n");

			if (!mb_PassSysCommand)
			{
				#ifndef _DEBUG
				if (gpConEmu->isPictureView())
					break;
				#endif
				
				gpConEmu->SetWindowMode(wmMaximized);
			}
			else
			{
				result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);
			}

			break;
		} // case SC_MAXIMIZE:
		
		case SC_RESTORE:
		{
			DEBUGSTRSYS(L"OnSysCommand(SC_RESTORE)\n");

			if (!mb_PassSysCommand)
			{
				#ifndef _DEBUG
				if (!gpConEmu->isIconic() && gpConEmu->isPictureView())
					break;
				#endif

				if (gpSet->isQuakeStyle)
				{
					gpConEmu->OnMinimizeRestore(sih_Show/*sih_HideTSA*/);
					break;
				}

				if (gpConEmu->SetWindowMode(gpConEmu->isIconic() ? gpConEmu->WindowMode : wmNormal))
				{
					// abForceChild=TRUE, если в табе запущено GUI приложение - можно передать в него фокус
					gpConEmu->OnFocus(ghWnd, WM_ACTIVATEAPP, TRUE, 0, L"After SC_RESTORE", TRUE);
					break;
				}
			}
			
			// ***
			{
				bool bIconic = ::IsIconic(hWnd);
				bool bPrev = bIconic ? SetRestoreFromMinimized(true) : mb_InRestoreFromMinimized;
				DEBUGTEST(WINDOWPLACEMENT wpl = {sizeof(wpl)}; GetWindowPlacement(ghWnd, &wpl););

				result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);

				SetRestoreFromMinimized(bPrev);

				// abForceChild=TRUE, если в табе запущено GUI приложение - можно передать в него фокус
				gpConEmu->OnFocus(hWnd, WM_ACTIVATEAPP, TRUE, 0, L"From SC_RESTORE", TRUE);
			}

			break;
		} // case SC_RESTORE:
		
		case SC_MINIMIZE:
		{
			DEBUGSTRSYS(L"OnSysCommand(SC_MINIMIZE)\n");

			bool bMin2TSA = gpSet->isMinToTray();

			if (!mb_InScMinimize)
			{
				mb_InScMinimize = true;

				// Запомним, на каком мониторе мы были до минимзации
				gpConEmu->StorePreMinimizeMonitor();

				// Если "фокус" в дочернем Gui приложении - нужно перед скрытием ConEmu "поднять" его
				CVConGuard VCon;
				if ((gpConEmu->GetActiveVCon(&VCon) >= 0) && VCon->RCon()->GuiWnd())
				{
					apiSetForegroundWindow(ghWnd);
				}

				if (gpSet->isQuakeStyle)
				{
					gpConEmu->OnMinimizeRestore(sih_AutoHide);
				}
				else if (bMin2TSA)
				{
					Icon.HideWindowToTray();
				}

				if (bMin2TSA)
				{
					// Окошко уже "спрятано", минимизировать не нужно
					mb_InScMinimize = false;
					break;
				}

				result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);

				mb_InScMinimize = false;
			}
			else
			{
				DEBUGSTRSYS(L"--OnSysCommand(SC_MINIMIZE) skipped, already in cycle\n");
			}
			
			break;
		} // case SC_MINIMIZE:
		
		default:
		{
			if (wParam >= IDM_VCONCMD_FIRST && wParam <= IDM_VCONCMD_LAST)
			{
				CVConGuard VCon;
				if (CVConGroup::GetActiveVCon(&VCon) >= 0)
					ExecPopupMenuCmd(VCon.VCon(), (int)(DWORD)wParam);
				result = 0;
			}
			else if (wParam != 0xF100)
			{
				#ifdef _DEBUG
				wchar_t szDbg[64]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"OnSysCommand(%i)\n", (DWORD)wParam);
				DEBUGSTRSYS(szDbg);
				#endif

				// Зачем вообще SysCommand, полученный в ConEmu, перенаправлять в RealConsole?
				#if 0
				// иначе это приводит к потере фокуса и активации невидимой консоли,
				// перехвате стрелок клавиатуры, и прочей фигни...
				if (wParam<0xF000)
				{
					POSTMESSAGE(ghConWnd, WM_SYSCOMMAND, wParam, lParam, FALSE);
				}
				#endif

				if (wParam == SC_SYSMENUPOPUP_SECRET)
				{
					mn_TrackMenuPlace = tmp_System;
					gpConEmu->mp_Tip->HideTip();
				}

				result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);

				if (wParam == SC_SYSMENUPOPUP_SECRET)
				{
					mn_TrackMenuPlace = tmp_None;
					gpConEmu->mp_Tip->HideTip();
				}
			}
		} // default:
	}

	return result;
}
