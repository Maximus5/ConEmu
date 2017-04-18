
/*
Copyright (c) 2012-2017 Maximus5
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
#include "AboutDlg.h"
#include "Attach.h"
#include "ConEmu.h"
#include "FindDlg.h"
#include "LngRc.h"
#include "Macro.h"
#include "Menu.h"
#include "Menu.h"
#include "Options.h"
#include "OptionsClass.h"
#include "RealConsole.h"
#include "SetColorPalette.h"
#include "SetCmdTask.h"
#include "Status.h"
#include "TabBar.h"
#include "TrayIcon.h"
#include "Update.h"
#include "VConChild.h"
#include "VConGroup.h"
#include "VirtualConsole.h"
#include "../common/MSetter.h"
#include "../common/MToolTip.h"
#include "../common/WUser.h"


#define DEBUGSTRSIZE(s) //DEBUGSTR(s)
#define DEBUGSTRSYS(s) //DEBUGSTR(s)

static const wchar_t* sMenuHotkey = L"1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";


CConEmuMenu::CConEmuMenu()
{
	mb_InNewConPopup = false;
	mb_InNewConPopup = mb_InNewConRPopup = false;
	//mn_FirstTaskID = mn_LastTaskID = 0;
	mb_PassSysCommand = false;
	mb_InScMinimize = false;
	mb_InRestoreFromMinimized = false;
	mn_SysMenuOpenTick = mn_SysMenuCloseTick = 0;
	mn_TrackMenuPlace = tmp_None;
	mb_FromConsoleMenu = false;

	mn_MsgOurSysCommand = gpConEmu->GetRegisteredMessage("UM_SYSCOMMAND");

	HMENU* hMenu[] = {
		&mh_SysDebugPopup, &mh_SysEditPopup, &mh_ActiveVConPopup, &mh_TerminateVConPopup, &mh_VConListPopup, &mh_HelpPopup, // Popup's для SystemMenu
		&mh_InsideSysMenu,
		// А это для VirtualConsole
		&mh_PopupMenu,
		&mh_TerminatePopup,
		&mh_RestartPopup,
		&mh_VConDebugPopup,
		&mh_VConEditPopup,
		&mh_VConViewPopup,
		NULL // end
	};
	mn_MenusCount = countof(hMenu);
	mph_Menus = (HMENU**)malloc(sizeof(hMenu));
	memmove(mph_Menus, hMenu, sizeof(hMenu));
	for (size_t i = 0; mph_Menus[i]; i++)
	{
		*mph_Menus[i] = NULL;
	}
	mb_CmdShowTaskItems = false;
	mn_CmdLastID = 0;
	mp_CmdRClickForce = NULL;
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
	SafeFree(mph_Menus);
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

void CConEmuMenu::CmdTaskPopupItem::Reset(CmdTaskPopupItemType newItemType, int newCmdId, LPCWSTR asName)
{
	_ASSERTE(this);
	memset(this, 0, sizeof(*this));

	this->ItemType = newItemType;
	this->nCmd = newCmdId;

	if (asName)
	{
		SetShortName(asName);
	}
}

void CConEmuMenu::CmdTaskPopupItem::SetShortName(LPCWSTR asName, bool bRightQuote /*= false*/, LPCWSTR asHotKey /*= NULL*/)
{
	CConEmuMenu::CmdTaskPopupItem::SetMenuName(this->szShort, countof(this->szShort), asName, (ItemType == eTaskPopup), bRightQuote, asHotKey);
}

void CConEmuMenu::CmdTaskPopupItem::SetMenuName(wchar_t* pszDisplay, INT_PTR cchDisplayMax, LPCWSTR asName, bool bTrailingPeriod, bool bRightQuote /*= false*/, LPCWSTR asHotKey /*= NULL*/)
{
	int nCurLen = _tcslen(pszDisplay);
	int nLen = _tcslen(asName);

	const wchar_t *pszSrc = asName;
	wchar_t *pszDst = pszDisplay+nCurLen;
	wchar_t *pszEnd = pszDisplay+cchDisplayMax-1;
	_ASSERTE((pszDst+min(8,2*nLen)) <= pszEnd); // Должно быть место

	if (!bTrailingPeriod)
	{
		if (nLen >= cchDisplayMax)
		{
			*(pszDst++) = /*…*/L'\x2026';
			pszSrc = asName+nLen-cchDisplayMax+nCurLen+2;
			_ASSERTE((pszSrc >= asName) && (pszSrc < (asName+nLen)));
		}
	}

	while (*pszSrc && (pszDst < pszEnd))
	{
		if (*pszSrc == L'&')
		{
			if ((pszDst+2) >= pszEnd)
				break;
			*(pszDst++) = L'&';
		}

		*(pszDst++) = *(pszSrc++);
	}

	if (bTrailingPeriod && *pszSrc)
	{
		if ((pszDst + 1) >= pszEnd)
			pszDst = pszDisplay+cchDisplayMax-2;

		*(pszDst++) = /*…*/L'\x2026';
	}

	// Для тасков, показать "»" когда они (сейчас) не разворачиваются в SubMenu
	wchar_t szRight[36] = L"";
	if (bRightQuote)
	{
		if (asHotKey && *asHotKey)
		{
			szRight[0] = L'\t';
			lstrcpyn(szRight+1, asHotKey, 32);
			wcscat_c(szRight, L" \xBB");
		}
		else
		{
			wcscpy_c(szRight, L"\t\xBB");
		}
	}
	else if (asHotKey && *asHotKey)
	{
		szRight[0] = L'\t';
		lstrcpyn(szRight+1, asHotKey, 34);
	}

	if (*szRight)
	{
		INT_PTR nRight = _tcslen(szRight);
		_ASSERTE((nRight+10) < cchDisplayMax);

		if ((pszDst + nRight) >= pszEnd)
			pszDst = pszDisplay+max(0,(cchDisplayMax-(nRight+1)));

		_wcscpy_c(pszDst, cchDisplayMax-(pszDst-pszDisplay), szRight);
		pszDst += _tcslen(pszDst);
		//*(pszDst++) = L'\t';
		//*(pszDst++) = 0xBB /* RightArrow/Quotes */;
	}

	// Terminate with '\0'
	if (pszDst <= pszEnd)
	{
		*pszDst = 0;
	}
	else
	{
		_ASSERTE(pszDst <= pszEnd)
		pszDisplay[cchDisplayMax-1] = 0;
	}
}

bool CConEmuMenu::CreateOrUpdateMenu(HMENU& hMenu, const MenuItem* Items, size_t ItemsCount)
{
	bool bNew = false;
	if (hMenu == NULL)
	{
		bNew = true;
		hMenu = CreatePopupMenu();
	}

	for (size_t i = 0; i < ItemsCount; ++i)
	{
		if (Items[i].mit == mit_Separator)
		{
			if (bNew)
				AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		}
		else if (bNew)
		{
			AppendMenu(hMenu,
				MF_STRING|Items[i].Flags,
				Items[i].MenuId,
				Items[i].HotkeyId ? MenuAccel(Items[i].HotkeyId,Items[i].pszText) : Items[i].pszText);
		}
		else if (Items[i].mit == mit_Command)
		{
			EnableMenuItem(hMenu, Items[i].MenuId, MF_BYCOMMAND|Items[i].Flags);
		}
		else
		{
			CheckMenuItem(hMenu, Items[i].MenuId, MF_BYCOMMAND|Items[i].Flags);
		}
	}

	return bNew;
}

void CConEmuMenu::OnNewConPopupMenu(POINT* ptWhere /*= NULL*/, DWORD nFlags /*= 0*/, bool bShowTaskItems /*= false*/)
{
	mb_CmdShowTaskItems = bShowTaskItems;
	HMENU hPopup = CreatePopupMenu();
	LPCWSTR pszCurCmd = NULL;
	if (!ptWhere && !nFlags)
	{
		if (gpConEmu->mp_TabBar->IsTabsShown() && (gpSet->nTabsLocation == 1))
			nFlags = TPM_BOTTOMALIGN;
	}
	bool lbReverse = (nFlags & TPM_BOTTOMALIGN) == TPM_BOTTOMALIGN;

	CVConGuard VCon;

	if ((gpConEmu->GetActiveVCon(&VCon) >= 0) && VCon->RCon())
		pszCurCmd = VCon->RCon()->GetCmd();

	LPCWSTR pszHistory = gpSet->HistoryGet(0);
	int nInsertPos = lbReverse ? 0 : -1;
	mn_CmdLastID = 0;
	//int nFirstID = 0, nLastID = 0, nFirstGroupID = 0, nLastGroupID = 0;
	//int nCreateID = 0, nSetupID = 0, nResetID = 0;
	//bool bWasHistory = false;

	//memset(m_CmdPopupMenu, 0, sizeof(m_CmdPopupMenu));
	m_CmdTaskPopup.clear();

	bool bSeparator = true;
	CmdTaskPopupItem itm = {CmdTaskPopupItem::eNone};

	// "New console dialog..."
	itm.Reset(CmdTaskPopupItem::eNewDlg, ++mn_CmdLastID, L"New console dialog...");
	InsertMenu(hPopup, nInsertPos, MF_BYPOSITION|MF_ENABLED|MF_STRING, itm.nCmd, itm.szShort);
	m_CmdTaskPopup.push_back(itm);

	// Команда активной консоли
	if (pszCurCmd && *pszCurCmd)
	{
		itm.Reset(CmdTaskPopupItem::eCmd, ++mn_CmdLastID, pszCurCmd);
		itm.pszCmd = pszCurCmd;
		InsertMenu(hPopup, nInsertPos, MF_BYPOSITION|MF_ENABLED|MF_STRING, itm.nCmd, itm.szShort);
		m_CmdTaskPopup.push_back(itm);
	}


	// Tasks begins
	{
		InsertMenu(hPopup, nInsertPos, MF_BYPOSITION|MF_SEPARATOR, 0, 0);

		// -- don't. let show current instance tasks only
		//// Обновить группы команд
		//gpSet->LoadCmdTasks(NULL);

		int nGroup = 0, nCurGroupCount = 0;
		const CommandTasks* pGrp = NULL;
		HMENU hCurPopup = hPopup;
		//const wchar_t* sMenuHotkey = L"1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		int nMenuHotkeyMax = _tcslen(sMenuHotkey);
		//bool bWasTasks = false;

		struct FolderInfo
		{
			wchar_t szFolderName[64];
			HMENU   hPopup;
			int     nGrpCount;
		};
		MArray<FolderInfo> Folders;
		FolderInfo flNew = {};
		CEStr szTempName;

		while ((pGrp = gpSet->CmdTaskGet(nGroup++)) != NULL)
		{
			int nGrpCount = -1;
			LPCWSTR pszFolder;
			HMENU hGrpPopup = NULL;

			LPCWSTR pszTaskName = pGrp->pszName ? pGrp->pszName : L"<NULL>";

			if ((pszFolder = wcsstr(pszTaskName, L"::")) != NULL)
			{
				// "Far::Latest", "Far::Far 1.7", "Build::ConEmu GUI", ...
				wchar_t szFolderName[64] = L"";
				lstrcpyn(szFolderName, pszTaskName, min(countof(szFolderName)-2, (pszFolder - pszTaskName + 1)));
				wcscat_c(szFolderName, L"}");
				for (INT_PTR f = 0; f < Folders.size(); f++)
				{
					FolderInfo& fl = Folders[f];
					if (lstrcmp(fl.szFolderName, szFolderName) == 0)
					{
						nGrpCount = (++fl.nGrpCount);
						hGrpPopup = fl.hPopup;
					}
				}

				if (!hGrpPopup)
				{
					// So create new popup "1: {Shells}" for example

					itm.Reset(CmdTaskPopupItem::eMore, -1);

					nCurGroupCount++;
					if (nCurGroupCount >= 1 && nCurGroupCount <= nMenuHotkeyMax)
					{
						_wsprintf(itm.szShort, SKIPLEN(countof(itm.szShort)) L"&%c: ", sMenuHotkey[nCurGroupCount-1]);
						int iLen = lstrlen(itm.szShort);
						lstrcpyn(itm.szShort+iLen, szFolderName, countof(itm.szShort)-iLen);
					}
					else
					{
						lstrcpyn(itm.szShort, szFolderName, countof(itm.szShort));
					}

					itm.hPopup = CreatePopupMenu();
					if (!InsertMenu(hCurPopup, nInsertPos, MF_BYPOSITION|MF_POPUP|MF_STRING|MF_ENABLED, (UINT_PTR)itm.hPopup, itm.szShort))
						break;
					hGrpPopup = itm.hPopup;
					m_CmdTaskPopup.push_back(itm);

					ZeroStruct(flNew);
					wcscpy_c(flNew.szFolderName, szFolderName);
					flNew.hPopup = itm.hPopup;
					flNew.nGrpCount = nGrpCount = 1;
					Folders.push_back(flNew);
				}

				szTempName = lstrmerge(L"{", pszFolder + 2);
				pszTaskName = szTempName;
			}

			if (!hGrpPopup)
			{
				if ((nCurGroupCount++) >= MAX_CMD_GROUP_SHOW)
				{
					itm.Reset(CmdTaskPopupItem::eMore, -1);
					wcscpy_c(itm.szShort, L"&More tasks"); // отдельной функцией, т.к. "Reset" - экранирует "&"
					itm.hPopup = CreatePopupMenu();
					if (!InsertMenu(hCurPopup, nInsertPos, MF_BYPOSITION|MF_POPUP|MF_STRING|MF_ENABLED, (UINT_PTR)itm.hPopup, itm.szShort))
						break;
					hCurPopup = itm.hPopup;
					m_CmdTaskPopup.push_back(itm);
					nCurGroupCount = 1;
				}
				nGrpCount = nCurGroupCount;
				hGrpPopup = hCurPopup;
			}

			// Next task
			itm.Reset(CmdTaskPopupItem::eTaskPopup, ++mn_CmdLastID);
			itm.pGrp = pGrp;
			//itm.pszCmd = NULL; // pGrp->pszCommands; - don't show hint, there is SubMenu on RClick

			if (nGrpCount >= 1 && nGrpCount <= nMenuHotkeyMax)
			{
				_wsprintf(itm.szShort, SKIPLEN(countof(itm.szShort)) L"&%c: ", sMenuHotkey[nGrpCount-1]);
			}
			else
			{
				itm.szShort[0] = 0;
				_ASSERTE(nGrpCount>=1 && nGrpCount<=nMenuHotkeyMax); // Too many tasks in one submenu?
			}

			wchar_t szHotkey[128];
			itm.SetShortName(pszTaskName, !mb_CmdShowTaskItems, pGrp->HotKey.GetHotkeyName(szHotkey, false));

			if (mb_CmdShowTaskItems)
			{
				itm.hPopup = CreatePopupMenu();
				InsertMenu(hGrpPopup, nInsertPos, MF_BYPOSITION|MF_POPUP|MF_STRING|MF_ENABLED, (UINT_PTR)itm.hPopup, itm.szShort);
			}
			else
			{
				InsertMenu(hGrpPopup, nInsertPos, MF_BYPOSITION|MF_ENABLED|MF_STRING, itm.nCmd, itm.szShort);
			}
			m_CmdTaskPopup.push_back(itm);

			//bWasTasks = true;
		}
		//// was any task appended?
		//bSeparator = bWasTasks;

		itm.Reset(CmdTaskPopupItem::eSetupTasks, ++mn_CmdLastID, MenuAccel(vkWinAltT,L"Setup tasks..."));
		InsertMenu(hPopup, nInsertPos, MF_BYPOSITION|MF_ENABLED|MF_STRING, itm.nCmd, itm.szShort);
		m_CmdTaskPopup.push_back(itm);
		bSeparator = true;
	}
	// Tasks ends

	// Is history present?
	if (pszHistory && *pszHistory)
	{
		if (bSeparator)
		{
			bSeparator = false;
			InsertMenu(hPopup, nInsertPos, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
		}

		//bool bSeparator = false;
		int nCount = 0;
		while ((pszHistory = gpSet->HistoryGet(nCount)) && (nCount < MAX_CMD_HISTORY_SHOW))
		{
			// Текущий - будет первым
			if (!pszCurCmd || lstrcmp(pszCurCmd, pszHistory))
			{
				itm.Reset(CmdTaskPopupItem::eCmd, ++mn_CmdLastID, pszHistory);
				itm.pszCmd = pszHistory;

				InsertMenu(hPopup, nInsertPos, MF_BYPOSITION|MF_STRING|MF_ENABLED, itm.nCmd, itm.szShort);
				m_CmdTaskPopup.push_back(itm);
			}

			pszHistory += _tcslen(pszHistory)+1;
			nCount++;
		}

		itm.Reset(CmdTaskPopupItem::eClearHistory, ++mn_CmdLastID, L"Clear history...");
		InsertMenu(hPopup, nInsertPos, MF_BYPOSITION|MF_STRING|MF_ENABLED, itm.nCmd, itm.szShort);
		m_CmdTaskPopup.push_back(itm);
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
		lpExcl = &rcBtnRect;
	}
	else
	{
		GetClientRect(ghWnd, &rcBtnRect);
		MapWindowPoints(ghWnd, NULL, (LPPOINT)&rcBtnRect, 2);
		rcBtnRect.left = rcBtnRect.right;
		rcBtnRect.bottom = rcBtnRect.top;
	}

	//mn_FirstTaskID = nFirstGroupID; mn_LastTaskID = nLastGroupID;

	mp_CmdRClickForce = NULL;
	mb_InNewConRPopup = false; // JIC

	mb_InNewConPopup = true;
	int nId = trackPopupMenu(tmp_Cmd, hPopup, nAlign|TPM_RETURNCMD/*|TPM_NONOTIFY*/,
	                         rcBtnRect.right,rcBtnRect.bottom, ghWnd, lpExcl);
	mb_InNewConPopup = mb_InNewConRPopup = false;
	//ShowMenuHint(NULL);

	if ((nId >= 1) || mp_CmdRClickForce)
	{
		if (mp_CmdRClickForce)
		{
			itm = *mp_CmdRClickForce;
		}
		else
		{
			itm.ItemType = CmdTaskPopupItem::eNone;
			for (INT_PTR i = 0; i < m_CmdTaskPopup.size(); i++)
			{
				if (m_CmdTaskPopup[i].nCmd == nId)
				{
					itm = m_CmdTaskPopup[i];
					break;
				}
			}
		}

		if (itm.ItemType == CmdTaskPopupItem::eNewDlg)
		{
			gpConEmu->RecreateAction(gpSetCls->GetDefaultCreateAction(), TRUE);
		}
		else if (itm.ItemType == CmdTaskPopupItem::eSetupTasks)
		{
			CSettings::Dialog(IDD_SPG_TASKS);
		}
		else if (itm.ItemType == CmdTaskPopupItem::eClearHistory)
		{
			gpSetCls->ResetCmdHistory();
		}
		else if ((itm.ItemType == CmdTaskPopupItem::eCmd)
			|| (itm.ItemType == CmdTaskPopupItem::eTaskCmd)
			|| (itm.ItemType == CmdTaskPopupItem::eTaskAll)
			|| (itm.ItemType == CmdTaskPopupItem::eTaskPopup))
		{
			RConStartArgs con;
			if ((itm.ItemType == CmdTaskPopupItem::eTaskAll) || (itm.ItemType == CmdTaskPopupItem::eTaskPopup))
			{
				const CommandTasks* pGrp = (const CommandTasks*)itm.pGrp;
				con.pszSpecialCmd = lstrdup(pGrp->pszName);
				pGrp->ParseGuiArgs(&con);
				_ASSERTE(con.pszSpecialCmd && *con.pszSpecialCmd==TaskBracketLeft && con.pszSpecialCmd[lstrlen(con.pszSpecialCmd)-1]==TaskBracketRight);
			}
			else if (itm.ItemType == CmdTaskPopupItem::eTaskCmd)
			{
				const CommandTasks* pGrp = (const CommandTasks*)itm.pGrp;

				// May be directory was set in task properties?
				pGrp->ParseGuiArgs(&con);

				// Task pre-options, for example ">*cmd"
				con.pszSpecialCmd = lstrdup(gpConEmu->ParseScriptLineOptions(itm.pszCmd, NULL, &con));

				con.ProcessNewConArg();

				_ASSERTE(con.pszSpecialCmd && *con.pszSpecialCmd);
			}
			else if (itm.ItemType == CmdTaskPopupItem::eCmd)
			{
				_ASSERTE(itm.pszCmd && (*itm.pszCmd != L'>'));

				con.pszSpecialCmd = lstrdup(itm.pszCmd);

				_ASSERTE(con.pszSpecialCmd && *con.pszSpecialCmd);
			}

			if (isPressed(VK_SHIFT))
			{
				int nRc = gpConEmu->RecreateDlg(&con);

				if (nRc != IDC_START)
					return;

				CVConGroup::Redraw();
			}
			// Issue 1564: Add tasks to history too
			else if ((itm.ItemType == CmdTaskPopupItem::eCmd) || (itm.ItemType == CmdTaskPopupItem::eTaskPopup))
			{
				gpSet->HistoryAdd(con.pszSpecialCmd);
			}

			//Собственно, запуск
			if ((gpSetCls->IsMulti() || !gpConEmu->isVConExists(0))
					&& (con.aRecreate != cra_CreateWindow))
			{
				gpConEmu->CreateCon(&con, true);
			}
			else
			{
				gpConEmu->CreateWnd(&con);
			}
		}
	}

	// Release handles
	for (INT_PTR i = 0; i < m_CmdTaskPopup.size(); i++)
	{
		if (m_CmdTaskPopup[i].hPopup)
			DestroyMenu(m_CmdTaskPopup[i].hPopup);
		SafeFree(m_CmdTaskPopup[i].pszTaskBuf);
	}
	DestroyMenu(hPopup);
}

void CConEmuMenu::OnNewConPopupMenuRClick(HMENU hMenu, UINT nItemPos)
{
	if (!mb_InNewConPopup || mb_InNewConRPopup)
		return;
	_ASSERTE(mn_TrackMenuPlace == tmp_Cmd);

	MENUITEMINFO mi = {sizeof(mi)};
	mi.fMask = MIIM_ID|MIIM_FTYPE|MIIM_SUBMENU;
	if (!GetMenuItemInfo(hMenu, nItemPos, TRUE, &mi))
		return;

	wchar_t szClass[128] = {};
	POINT ptCur = {}; GetCursorPos(&ptCur);
	HWND hMenuWnd = WindowFromPoint(ptCur);
	GetClassName(hMenuWnd, szClass, countof(szClass));
	if (lstrcmp(szClass, L"#32768") != 0)
		hMenuWnd = NULL;

	int nId = (int)mi.wID;
	HMENU hPopup = mi.hSubMenu;

	//if (mn_LastTaskID < 0 || nId > (UINT)mn_LastTaskID)
	//	return;
	CmdTaskPopupItem* itm = NULL;
	for (INT_PTR i = 0; i < m_CmdTaskPopup.size(); i++)
	{
		if  ((mb_CmdShowTaskItems && (m_CmdTaskPopup[i].hPopup && m_CmdTaskPopup[i].hPopup == hPopup))
			|| (!mb_CmdShowTaskItems && (m_CmdTaskPopup[i].nCmd == nId)))
		{
			itm = &(m_CmdTaskPopup[i]);
			break;
		}
	}
	// Проверяем, по чему щелкнули
	if (!itm || itm->ItemType != CmdTaskPopupItem::eTaskPopup)
		return;

	// Если таски созданы были как SubMenu?
	if (mb_CmdShowTaskItems)
	{
		// Need to close parentmenu
		if (hMenuWnd)
		{
			HWND hParent = GetParent(hMenuWnd);
			wchar_t szClass[100];
			GetClassName(hParent, szClass, countof(szClass));
			// Если таски были открыты через "More tasks" - нужно закрыть и родительские субменю
			_ASSERTE(hParent == ghWnd);

			mp_CmdRClickForce = itm;
			PostMessage(hMenuWnd, WM_CLOSE, 0, 0);
		}
	}
	else
	{
		const CommandTasks* pGrp = (const CommandTasks*)itm->pGrp;
		if (!pGrp || !pGrp->pszCommands || !*pGrp->pszCommands)
			return;

		// Поехали
		HMENU hPopup = CreatePopupMenu();
		int nLastID = FillTaskPopup(hPopup, itm);

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

		CmdTaskPopupItem itm = {CmdTaskPopupItem::eNone};
		if (nRetID > 0)
		{
			for (INT_PTR i = 0; i < m_CmdTaskPopup.size(); i++)
			{
				if (m_CmdTaskPopup[i].nCmd == nRetID)
				{
					itm = m_CmdTaskPopup[i];
					break;
				}
			}
		}

		if (itm.ItemType == CmdTaskPopupItem::eTaskCmd)
		{
			// Need to close parentmenu
			if (hMenuWnd)
			{
				PostMessage(hMenuWnd, WM_CLOSE, 0, 0);
			}

			RConStartArgs con;

			// May be directory was set in task properties?
			pGrp->ParseGuiArgs(&con);

			LPCWSTR pszCmd = gpConEmu->ParseScriptLineOptions(itm.pszCmd, NULL, &con);

			// Well, start selected line from Task
			con.pszSpecialCmd = lstrdup(pszCmd);
			if (!con.pszSpecialCmd)
			{
				_ASSERTE(con.pszSpecialCmd!=NULL);
			}
			else
			{
				con.ProcessNewConArg();

				if (isPressed(VK_SHIFT))
				{
					int nRc = gpConEmu->RecreateDlg(&con);

					if (nRc != IDC_START)
						return;

					CVConGroup::Redraw();
				}
				//Don't add task subitems to history
				//else
				//{
				//	gpSet->HistoryAdd(con.pszSpecialCmd);
				//}

				//Собственно, запуск
				if (gpSetCls->IsMulti())
					gpConEmu->CreateCon(&con, true);
				else
					gpConEmu->CreateWnd(&con);
			}
		}

		//SafeFree(pszDataW);
	}
}

bool CConEmuMenu::OnMenuSelected_NewCon(HMENU hMenu, WORD nID, WORD nFlags)
{
	if (!mb_InNewConPopup)
	{
		_ASSERTE(mb_InNewConPopup);
		return false;
	}

	CmdTaskPopupItem itm = {CmdTaskPopupItem::eNone};
	for (INT_PTR i = 0; i < m_CmdTaskPopup.size(); i++)
	{
		if (m_CmdTaskPopup[i].nCmd == nID)
		{
			itm = m_CmdTaskPopup[i];
			break;
		}
	}

	if (itm.ItemType == CmdTaskPopupItem::eCmd && itm.pszCmd)
	{
		LPCWSTR pszCmd = itm.pszCmd;
		if (itm.szShort[0] && pszCmd && lstrcmp(pszCmd, itm.szShort))
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

			ShowMenuHint(pszCmd, &pt);
		}
		else
		{
			ShowMenuHint(NULL);
		}
	}
	else
	{
		ShowMenuHint(NULL);
	}
	return true;
}


bool CConEmuMenu::OnMenuSelected(HMENU hMenu, WORD nID, WORD nFlags)
{
	bool bRc = true;

	wchar_t szInfo[80];
	_wsprintf(szInfo, SKIPCOUNT(szInfo) L"OnMenuSelected selected, ID=%u, Flags=x%04X", nID, nFlags);
	LogString(szInfo);

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
void CConEmuMenu::ShowPopupMenu(CVirtualConsole* apVCon, POINT ptCur, DWORD Align /* = TPM_LEFTALIGN */, bool abFromConsole /*= false*/)
{
	CVConGuard guard(apVCon);
	BOOL lbNeedCreate = (mh_PopupMenu == NULL);

	#if 0
	// -- dummy code, TPM_LEFTALIGN==0
	if (!Align)
		Align = TPM_LEFTALIGN;
	#endif

	// Создать или обновить enable/disable
	mh_PopupMenu = CreateVConPopupMenu(apVCon, mh_PopupMenu, true, abFromConsole);
	if (!mh_PopupMenu)
	{
		MBoxAssert(mh_PopupMenu!=NULL);
		return;
	}

	if (lbNeedCreate)
	{
		AppendMenu(mh_PopupMenu, MF_SEPARATOR, 0, 0);

		_ASSERTE(mh_VConEditPopup == NULL);
		mh_VConEditPopup = CreateEditMenuPopup(apVCon);
		AppendMenu(mh_PopupMenu, MF_POPUP|MF_ENABLED, (UINT_PTR)mh_VConEditPopup, _T("Ed&it"));

		_ASSERTE(mh_VConViewPopup == NULL);
		mh_VConViewPopup = CreateViewMenuPopup(apVCon);
		AppendMenu(mh_PopupMenu, MF_POPUP|MF_ENABLED, (UINT_PTR)mh_VConViewPopup, _T("&View (palettes)"));

		_ASSERTE(mh_VConDebugPopup == NULL);
		mh_VConDebugPopup = CreateDebugMenuPopup();
		AppendMenu(mh_PopupMenu, MF_POPUP|MF_ENABLED, (UINT_PTR)mh_VConDebugPopup, _T("&Debug"));
	}
	else
	{
		// обновить enable/disable пунктов меню
		CreateEditMenuPopup(apVCon, mh_VConEditPopup);
		// имена палитр и флажок текущей
		CreateViewMenuPopup(apVCon, mh_VConViewPopup);
	}

	// Некузяво. Может вслыть тултип под меню
	//ptCur.x++; ptCur.y++; // чтобы меню можно было сразу закрыть левым кликом.

	mb_FromConsoleMenu = abFromConsole;

	int nCmd = trackPopupMenu(tmp_VCon, mh_PopupMenu,
	                          Align | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
	                          ptCur.x, ptCur.y, ghWnd);

	if (!nCmd)
		return; // отмена

	ExecPopupMenuCmd(tmp_VCon, apVCon, nCmd);

	mb_FromConsoleMenu = false;
}

void CConEmuMenu::ExecPopupMenuCmd(TrackMenuPlace place, CVirtualConsole* apVCon, int nCmd)
{
	CVConGuard guard;
	if (!guard.Attach(apVCon))
		return;

	switch (nCmd)
	{
		case IDM_CLOSE:
			if (!mb_FromConsoleMenu && gpSet->isOneTabPerGroup && CVConGroup::isGroup(apVCon))
				CVConGroup::CloseGroup(apVCon);
			else
				apVCon->RCon()->CloseTab();
			break;
		case IDM_DETACH:
			apVCon->RCon()->DetachRCon();
			break;
		case IDM_UNFASTEN:
			apVCon->RCon()->Unfasten();
			break;
		case IDM_RENAMETAB:
			apVCon->RCon()->DoRenameTab();
			break;
		case IDM_CHANGEAFFINITY:
			apVCon->RCon()->ChangeAffinityPriority();
			break;
		case IDM_DUPLICATE:
		case IDM_ADMIN_DUPLICATE:
			if ((nCmd == IDM_ADMIN_DUPLICATE) || isPressed(VK_SHIFT))
				apVCon->RCon()->AdminDuplicate();
			else
				apVCon->RCon()->DuplicateRoot();
			break;
		case IDM_SPLIT2RIGHT:
		case IDM_SPLIT2BOTTOM:
		{
			LPWSTR pszMacro = lstrdup((nCmd == IDM_SPLIT2RIGHT) ? L"Split(0,50,0)" : L"Split(0,0,50)");
			LPWSTR pszRc = ConEmuMacro::ExecuteMacro(pszMacro, apVCon->RCon());
			SafeFree(pszMacro);
			SafeFree(pszRc);
			break;
		}
		case IDM_CHILDSYSMENU:
			apVCon->RCon()->ChildSystemMenu();
			break;

		case IDM_TERMINATEPRC:
			// Active console/pane: Terminate active process
			apVCon->RCon()->TerminateActiveProcess(isPressed(VK_SHIFT), 0);
			break;
		case IDM_TERMINATEBUTSHELL:
			// Active console/pane: Terminate all except root process
			apVCon->RCon()->TerminateAllButShell(isPressed(VK_SHIFT));
			break;
		case IDM_TERMINATECON:
			// Active console/pane: Do normal close
			apVCon->RCon()->CloseConsole(false, true/*confimation may be disabled in settings*/);
			break;

		case IDM_TERMINATEGROUP:
			// Active group: Do normal close of each pane
		case IDM_TERMINATEPRCGROUP:
			// Active group: Terminate active process in each pane
			CVConGroup::CloseGroup(apVCon, (nCmd==IDM_TERMINATEPRCGROUP));
			break;

		case IDM_TERMINATEALLCON:
			// Do normal close of all consoles (tabs and panes)
		case IDM_TERMINATECONEXPT:
			// Close all tabs and panes except active console/pane
		case IDM_TERMINATEZOMBIES:
			// Close ‘zombies’ (where ‘Press Esc to close console’ is displayed)
			CVConGroup::CloseAllButActive((nCmd == IDM_TERMINATECONEXPT) ? apVCon : NULL, (nCmd == IDM_TERMINATEZOMBIES), false);
			break;

		case IDM_RESTART:
		case IDM_RESTARTAS:
		case IDM_RESTARTDLG:
			if (apVCon->isActive(false))
			{
				gpConEmu->RecreateAction(cra_RecreateTab/*TRUE*/, (nCmd==IDM_RESTARTDLG) || isPressed(VK_SHIFT), (nCmd==IDM_RESTARTAS) ? crb_On : crb_Undefined);
			}
			else
			{
				MBoxAssert(apVCon->isActive(false));
			}

			break;

		case ID_NEWCONSOLE:
			gpConEmu->RecreateAction(gpSetCls->GetDefaultCreateAction(), gpSet->isMultiNewConfirm || isPressed(VK_SHIFT));
			break;
		case IDM_ATTACHTO:
			OnSysCommand(ghWnd, IDM_ATTACHTO, 0);
			break;

		//case IDM_SAVE:
		//	apVCon->RCon()->PostMacro(L"F2");
		//	break;
		//case IDM_SAVEALL:
		//	apVCon->RCon()->PostMacro(gpSet->sSaveAllMacro);
		//	break;

		default:
			if ((place == tmp_VCon) && (nCmd >= ID_CON_SETPALETTE_FIRST) && (nCmd <= ID_CON_SETPALETTE_LAST))
			{
				apVCon->ChangePalette(nCmd - ID_CON_SETPALETTE_FIRST);
			}
			else if (nCmd >= IDM_VCONCMD_FIRST && nCmd <= IDM_VCONCMD_LAST)
			{
				_ASSERTE(FALSE && "Unhandled command!");
			}
			else if (nCmd >= 0xAB00)
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
			AppendMenu(mh_InsideSysMenu, MF_STRING|MF_ENABLED, SC_CLOSE, MenuAccel(vkCloseConEmu,L"&Close ConEmu"));
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
				InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED, ID_TOMONITOR, MenuAccel(vkJumpActiveMonitor,L"Bring &here"));
			InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED, ID_TOTRAY, TRAY_ITEM_HIDE_NAME/* L"Hide to &TSA" */);
		}
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_SEPARATOR, 0, 0);

		//InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED, ID_ABOUT, _T("&About / Help"));
		if (mh_HelpPopup) DestroyMenu(mh_HelpPopup);
		mh_HelpPopup = CreateHelpMenuPopup();
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_POPUP|MF_ENABLED, (UINT_PTR)mh_HelpPopup, _T("Hel&p"));
		//if (ms_ConEmuChm[0])  //Показывать пункт только если есть conemu.chm
		//	InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED, ID_HELP, _T("&Help"));

		// --------------------
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_SEPARATOR, 0, 0);

		if (mh_SysDebugPopup) DestroyMenu(mh_SysDebugPopup);
		mh_SysDebugPopup = CreateDebugMenuPopup();
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_POPUP|MF_ENABLED, (UINT_PTR)mh_SysDebugPopup, _T("&Debug"));

		if (mh_SysEditPopup) DestroyMenu(mh_SysEditPopup);
		mh_SysEditPopup = CreateEditMenuPopup(NULL);
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_POPUP|MF_ENABLED, (UINT_PTR)mh_SysEditPopup, _T("&Edit"));

		// --------------------
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_SEPARATOR, 0, 0);

		if (mh_VConListPopup) DestroyMenu(mh_VConListPopup);
		mh_VConListPopup = CreateVConListPopupMenu(mh_VConListPopup, TRUE/*abFirstTabOnly*/);
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_POPUP|MF_ENABLED, (UINT_PTR)mh_VConListPopup, _T("Console &list"));

		if (mh_ActiveVConPopup) DestroyMenu(mh_ActiveVConPopup);
		if (mh_TerminateVConPopup) { DestroyMenu(mh_TerminateVConPopup); mh_TerminateVConPopup = NULL; }
		mh_ActiveVConPopup = CreateVConPopupMenu(NULL, NULL, false, false);
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_POPUP|MF_ENABLED, (UINT_PTR)mh_ActiveVConPopup, _T("Acti&ve console"));

		// --------------------
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_SEPARATOR, 0, 0);
		if (!gpConEmu->mp_Inside)
		{
			InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED|(gpSet->isAlwaysOnTop ? MF_CHECKED : 0),
				ID_ALWAYSONTOP, MenuAccel(vkAlwaysOnTop,L"Al&ways on top"));
		}
		#ifdef SHOW_AUTOSCROLL
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED|(gpSetCls->AutoScroll ? MF_CHECKED : 0),
			ID_AUTOSCROLL, _T("Auto scro&ll"));
		#endif
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED, ID_SETTINGS,   MenuAccel(vkWinAltP,L"Sett&ings..."));
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED, IDM_ATTACHTO,  MenuAccel(vkMultiNewAttach,L"Attach t&o..."));
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED, ID_NEWCONSOLE, MenuAccel(vkMultiNew,L"New console..."));
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

int CConEmuMenu::FillTaskPopup(HMENU hMenu, CmdTaskPopupItem* pParent)
{
	const CommandTasks* pGrp = (const CommandTasks*)pParent->pGrp;

	CmdTaskPopupItem itm = {CmdTaskPopupItem::eNone};

	//itm.Reset(CmdTaskPopupItem::eTask, ++mn_CmdLastID, L"Run all commands");
	//itm.pGrp = pGrp;

	int nCount = 0;
	int nMenuHotkeyMax = _tcslen(sMenuHotkey);

	if (pGrp && pGrp->pszCommands && *pGrp->pszCommands)
	{
		SafeFree(pParent->pszTaskBuf);
		pParent->pszTaskBuf = lstrdup(pGrp->pszCommands);

		wchar_t *pszDataW = pParent->pszTaskBuf;
		wchar_t *pszLine = pszDataW;
		wchar_t *pszNewLine = wcschr(pszLine, L'\n');
		//const UINT nStartID = 1000;
		//UINT nLastID = 0;
		//LPCWSTR pszLines[MAX_CONSOLE_COUNT];

		while (*pszLine && (nCount < MAX_CONSOLE_COUNT))
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
				itm.Reset(CmdTaskPopupItem::eTaskCmd, ++mn_CmdLastID);
				itm.pGrp = pGrp;
				itm.pszCmd = pszLine;
				if (nCount <= nMenuHotkeyMax)
					_wsprintf(itm.szShort, SKIPLEN(countof(itm.szShort)) L"&%c: ", sMenuHotkey[nCount]);
				else
					itm.szShort[0] = 0;

				// Обработать возможные "&" в строке запуска, чтобы они в меню видны были
				//itm.pszTaskBuf = lstrmerge(itm.szShort, pszLine);
				INT_PTR cchItemMax = _tcslen(itm.szShort) + 2*_tcslen(pszLine) + 1;
				itm.pszTaskBuf = (wchar_t*)malloc(cchItemMax*sizeof(*itm.pszTaskBuf));
				_wcscpy_c(itm.pszTaskBuf, cchItemMax, itm.szShort);
				CmdTaskPopupItem::SetMenuName(itm.pszTaskBuf, cchItemMax, pszLine, true);

				InsertMenu(hMenu, -1, MF_BYPOSITION|MF_ENABLED|MF_STRING, itm.nCmd, itm.pszTaskBuf);
				m_CmdTaskPopup.push_back(itm);

				nCount++;
			}

			if (!pszNewLine) break;

			pszLine = pszNewLine+1;

			if (!*pszLine) break;

			while ((*pszLine == L'\r') || (*pszLine == L'\n'))
				pszLine++; // пропустить все переводы строк

			pszNewLine = wcschr(pszLine, L'\n');
		}
	}

	return nCount;
}

LRESULT CConEmuMenu::OnInitMenuPopup(HWND hWnd, HMENU hMenu, LPARAM lParam)
{
	// Уже должен быть выставлен тип меню, иначе не будут всплывать подсказки для пунктов меню
	_ASSERTE(mn_TrackMenuPlace != tmp_None);
	if (!hMenu)
	{
		_ASSERTE(hMenu!=NULL);
		return 0;
	}

	DefWindowProc(hWnd, WM_INITMENUPOPUP, (WPARAM)hMenu, lParam);

	MENUITEMINFO mi = {sizeof(mi)};
	wchar_t szText[255];
	mi.fMask = MIIM_STRING; mi.dwTypeData = szText; mi.cch = countof(szText);
	BOOL bIsSysMenu = GetMenuItemInfo(hMenu, SC_CLOSE, FALSE, &mi);


	if (HIWORD(lParam))
	{
		if (mn_TrackMenuPlace != tmp_System)
		{
			_ASSERTE(mn_TrackMenuPlace == tmp_System);
			return 0;
		}

		// при всплытии "Help/Debug/..." submenu сюда мы тоже попадаем

		if (bIsSysMenu)
		{
			UpdateSysMenu(hMenu);

			//BOOL bSelectionExist = FALSE;

			CVConGuard VCon;
			CVirtualConsole* pVCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon.VCon() : NULL;
			//if (pVCon && pVCon->RCon())
			//	bSelectionExist = pVCon->RCon()->isSelectionPresent();

			//EnableMenuItem(hMenu, ID_CON_COPY, MF_BYCOMMAND|(bSelectionExist?MF_ENABLED:MF_GRAYED));
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
				CreateVConPopupMenu(NULL, mh_ActiveVConPopup, false, false);
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

	if (mn_TrackMenuPlace == tmp_Cmd)
	{
		CmdTaskPopupItem* p = NULL;
		for (INT_PTR i = 0; i < m_CmdTaskPopup.size(); i++)
		{
			if (m_CmdTaskPopup[i].hPopup == hMenu)
			{
				p = &(m_CmdTaskPopup[i]);
				break;
			}
		}

		if (mb_CmdShowTaskItems && p && (p->ItemType == CmdTaskPopupItem::eTaskPopup) && !p->bPopupInitialized)
		{
			p->bPopupInitialized = TRUE;
			const CommandTasks* pGrp = (const CommandTasks*)p->pGrp;

			//CmdTaskPopupItem itm = {CmdTaskPopupItem::eNone};

			//itm.Reset(CmdTaskPopupItem::eTask, ++mn_CmdLastID, L"Run all commands");
			//itm.pGrp = pGrp;

			//int nCount = 0;
			//int nMenuHotkeyMax = _tcslen(sMenuHotkey);

			if (pGrp && pGrp->pszCommands && *pGrp->pszCommands)
			{
				int nCount = FillTaskPopup(hMenu, p);

				//p->pszTaskBuf = lstrdup(pGrp->pszCommands);
				//wchar_t *pszDataW = p->pszTaskBuf;
				//wchar_t *pszLine = pszDataW;
				//wchar_t *pszNewLine = wcschr(pszLine, L'\n');
				////const UINT nStartID = 1000;
				////UINT nLastID = 0;
				////LPCWSTR pszLines[MAX_CONSOLE_COUNT];

				//while (*pszLine && (nCount < MAX_CONSOLE_COUNT))
				//{
				//	if (pszNewLine)
				//	{
				//		*pszNewLine = 0;
				//		if ((pszNewLine > pszDataW) && (*(pszNewLine-1) == L'\r'))
				//			*(pszNewLine-1) = 0;
				//	}

				//	while (*pszLine == L' ') pszLine++;

				//	if (*pszLine)
				//	{
				//		itm.Reset(CmdTaskPopupItem::eTaskCmd, ++mn_CmdLastID);
				//		itm.pGrp = pGrp;
				//		itm.pszCmd = pszLine;
				//		if (nCount <= nMenuHotkeyMax)
				//			_wsprintf(itm.szShort, SKIPLEN(countof(itm.szShort)) L"&%c: ", sMenuHotkey[nCount]);
				//		else
				//			itm.szShort[0] = 0;

				//		// Обработать возможные "&" в строке запуска, чтобы они в меню видны были
				//		//itm.pszTaskBuf = lstrmerge(itm.szShort, pszLine);
				//		INT_PTR cchItemMax = _tcslen(itm.szShort) + 2*_tcslen(pszLine) + 1;
				//		itm.pszTaskBuf = (wchar_t*)malloc(cchItemMax*sizeof(*itm.pszTaskBuf));
				//		_wcscpy_c(itm.pszTaskBuf, cchItemMax, itm.szShort);
				//		CmdTaskPopupItem::SetMenuName(itm.pszTaskBuf, cchItemMax, pszLine, true);

				//		InsertMenu(hMenu, -1, MF_BYPOSITION|MF_ENABLED|MF_STRING, itm.nCmd, itm.pszTaskBuf);
				//		m_CmdTaskPopup.push_back(itm);

				//		nCount++;
				//	}

				//	if (!pszNewLine) break;

				//	pszLine = pszNewLine+1;

				//	if (!*pszLine) break;

				//	while ((*pszLine == L'\r') || (*pszLine == L'\n'))
				//		pszLine++; // пропустить все переводы строк

				//	pszNewLine = wcschr(pszLine, L'\n');
				//}

				if (nCount > 1)
				{
					CmdTaskPopupItem itm = {CmdTaskPopupItem::eNone};
					InsertMenu(hMenu, 0, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);

					itm.Reset(CmdTaskPopupItem::eTaskAll, ++mn_CmdLastID, L"All task tabs");
					itm.pGrp = pGrp;
					InsertMenu(hMenu, 0, MF_BYPOSITION|MF_ENABLED|MF_STRING, itm.nCmd, itm.szShort);
					m_CmdTaskPopup.push_back(itm);

					nCount++;
				}
			}
		}
	}

	return 0;
}

int CConEmuMenu::trackPopupMenu(TrackMenuPlace place, HMENU hMenu, UINT uFlags, int x, int y, HWND hWnd, RECT *prcRect /* = NULL*/)
{
	ShowMenuHint(NULL);
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

	wchar_t szInfo[100];
	_wsprintf(szInfo, SKIPCOUNT(szInfo) L"TrackPopupMenuEx started, place=%i, flags=x%08X, {%i,%i}", place, uFlags, x, y);
	LogString(szInfo);

	SetLastError(0);
	int cmd = TrackPopupMenuEx(hMenu, uFlags, x, y, hWnd, &ex);

	_wsprintf(szInfo, SKIPCOUNT(szInfo) L"TrackPopupMenuEx done, command=%i, code=%u", cmd, GetLastError());
	LogString(szInfo);

	mn_TrackMenuPlace = prevPlace;

	ShowMenuHint(NULL);

	return cmd;
}

void CConEmuMenu::ShowMenuHint(LPCWSTR pszText, POINT* ppt)
{
	enum LastHintStyle { eNone, eStatus, eTip };
	static LastHintStyle last = eNone;

	if (pszText && *pszText)
	{
		// Hide last hint
		if (last != eNone)
			ShowMenuHint(NULL);

		POINT pt = {};
		if (ppt)
			pt = *ppt;
		else
			GetCursorPos(&pt);

		// if status bar exists - show hint there
		if (gpSet->isStatusBarShow)
		{
			if (gpConEmu->mp_Status)
			{
				gpConEmu->mp_Status->SetStatus(pszText);
				last = eStatus;
			}
		}
		else
		{
			if (gpConEmu->mp_Tip)
			{
				gpConEmu->mp_Tip->ShowTip(ghWnd, ghWnd, pszText, TRUE, pt, g_hInstance);
				last = eTip;
			}
		}
	}
	else
	{
		switch (last)
		{
		case eStatus:
			if (gpConEmu->mp_Status)
				gpConEmu->mp_Status->SetStatus(NULL);
			break;
		case eTip:
			if (gpConEmu->mp_Tip)
				gpConEmu->mp_Tip->HideTip();
			break;
		}
		last = eNone;
	}
}

void CConEmuMenu::ShowMenuHint(HMENU hMenu, WORD nID, WORD nFlags)
{
	if (nID && !(nFlags & MF_POPUP))
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
			if (CLngRc::getHint(nMenuID, szText, countof(szText)))
			{
				ShowMenuHint(szText, &pt);
				return;
			}
		}
	}

	ShowMenuHint(NULL);
}

void CConEmuMenu::ShowKeyBarHint(HMENU hMenu, WORD nID, WORD nFlags)
{
	if (nID && !(nFlags & MF_POPUP))
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
		LogString(L"ShowSysmenu called from (OnNcIconLClick)");
		ShowSysmenu();
	}
}

void CConEmuMenu::ShowSysmenu(int x, int y, DWORD nFlags /*= 0*/)
{
	//if (!Wnd)
	//	Wnd = ghWnd;

	WARNING("SysMenu: Обработать DblClick по иконке!");

	if ((x == -32000) || (y == -32000))
	{
		RECT rect = {}, cRect = {};
		GetWindowRect(ghWnd, &rect);
		cRect = gpConEmu->GetGuiClientRect();
		WINDOWINFO wInfo = {sizeof(wInfo)};
		GetWindowInfo(ghWnd, &wInfo);
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
	bool visible = _bool(::IsWindowVisible(ghWnd));
	int style = GetWindowLong(ghWnd, GWL_STYLE);
	HMENU systemMenu = GetSysMenu();

	if (!systemMenu)
		return;

	if (!gpConEmu->mp_Inside)
	{
		EnableMenuItem(systemMenu, SC_RESTORE,
		               MF_BYCOMMAND|((visible && (iconic || zoomed)) ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(systemMenu, SC_MOVE,
		               MF_BYCOMMAND|((visible && !(iconic || zoomed)) ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(systemMenu, SC_SIZE,
		               MF_BYCOMMAND|((visible && (!(iconic || zoomed) && (style & WS_SIZEBOX))) ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(systemMenu, SC_MINIMIZE,
		               MF_BYCOMMAND|((visible && (!iconic && (style & WS_MINIMIZEBOX))) ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(systemMenu, SC_MAXIMIZE,
		               MF_BYCOMMAND|((visible && (!zoomed && (style & WS_MAXIMIZEBOX))) ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(systemMenu, ID_TOTRAY, MF_BYCOMMAND|MF_ENABLED);
	}

	mn_TrackMenuPlace = tmp_System;
	SendMessage(ghWnd, WM_INITMENU, (WPARAM)systemMenu, 0);
	SendMessage(ghWnd, WM_INITMENUPOPUP, (WPARAM)systemMenu, MAKELPARAM(0, true));

	SetActiveWindow(ghWnd);

	mn_SysMenuOpenTick = GetTickCount();
	POINT ptCurBefore = {}; GetCursorPos(&ptCurBefore);

	int command = trackPopupMenu(tmp_System, systemMenu,
		 TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON | nFlags,
		 x, y, ghWnd);

	if (command == 0)
	{
		bool bLbnPressed = isPressed(VK_LBUTTON);
		mn_SysMenuCloseTick = GetTickCount();

		if (bLbnPressed && ((mn_SysMenuCloseTick - mn_SysMenuOpenTick) < GetDoubleClickTime()))
		{
			POINT ptCur = {}; GetCursorPos(&ptCur);
			if (PtDiffTest(ptCur, ptCurBefore.x, ptCurBefore.y, 8))
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

	wchar_t szInfo[64]; _wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"ShowSysmenu result: %i", command);
	LogString(szInfo);

	if (command && Icon.isWindowInTray() && gpConEmu->isIconic())
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
				gpConEmu->DoMinimizeRestore(sih_Show);
				break;
		}
	}

	if (command)
	{
		// gpConEmu will trigger our OnSysCommand method on post received
		PostMessage(ghWnd, mn_MsgOurSysCommand/*WM_SYSCOMMAND*/, (WPARAM)command, 0);
	}
}

HMENU CConEmuMenu::CreateDebugMenuPopup()
{
	HMENU hDebug = CreatePopupMenu();
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_CON_TOGGLE_VISIBLE, MenuAccel(vkCtrlWinAltSpace,L"&Real console"));
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_CONPROP, _T("&Properties..."));
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_SCREENSHOT, MenuAccel(vkScreenshot,L"Make &screenshot..."));
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_DUMPCONSOLE, _T("&Dump screen..."));
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_LOADDUMPCONSOLE, _T("&Load screen dump..."));
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_DEBUGGUI, _T("Debug &log (GUI)"));
//#ifdef _DEBUG
//	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_MONITOR_SHELLACTIVITY, _T("Enable &shell log..."));
//#endif
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_DEBUG_SHOWRECTS, _T("Show debug rec&ts"));
	AppendMenu(hDebug, MF_SEPARATOR, 0, NULL);
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_DEBUG_TRAP, _T("Raise exception (Main thread)"));
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_DEBUG_TRAP2, _T("Raise exception (Monitor thread)"));
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_DEBUG_ASSERT, _T("Show assertion"));
	#ifdef TRACK_MEMORY_ALLOCATIONS
	AppendMenu(hDebug, MF_SEPARATOR, 0, NULL);
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_DUMP_MEM_BLK, _T("Dump used memory blocks"));
	#endif
	AppendMenu(hDebug, MF_SEPARATOR, 0, NULL);
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_DEBUGCON, MenuAccel(vkDebugProcess,L"Debug &active process..."));
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_MINIDUMP, MenuAccel(vkDumpProcess,L"Active process &memory dump..."));
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_MINIDUMPTREE, MenuAccel(vkDumpTree,L"Active &tree memory dump..."));
	return hDebug;
}

HMENU CConEmuMenu::CreateVConListPopupMenu(HMENU ahExist, BOOL abFirstTabOnly)
{
	HMENU h = ahExist ? ahExist : CreatePopupMenu();
	wchar_t szText[128];
	const int nMaxStrLen = 32;

	BOOL lbActiveVCon = FALSE;
	int nActiveCmd = -1; // DWORD MAKELONG(WORD wLow,WORD wHigh);
	DWORD nAddFlags = 0;

	if (ahExist)
	{
		while (DeleteMenu(ahExist, 0, MF_BYPOSITION))
			;
	}

	CVConGuard VCon;
	for (int V = 0; CVConGroup::GetVCon(V, &VCon, true); V++)
	{
		if ((lbActiveVCon = VCon->isActive(false)))
			nActiveCmd = MAKELONG(1, V+1);
		nAddFlags = 0; //(lbActiveVCon ? MF_DEFAULT : 0);
		CRealConsole* pRCon = VCon->RCon();
		if (!pRCon)
		{
			wsprintf(szText, L"%i: VConsole", V+1);
			AppendMenu(h, MF_STRING|nAddFlags, MAKELONG(1, V+1), szText);
		}
		else
		{
			CTab tab(__FILE__,__LINE__);
			int R = 0;
			if (!pRCon->GetTab(R, tab))
			{
				wsprintf(szText, L"%i: RConsole", V+1);
				AppendMenu(h, MF_STRING|nAddFlags, MAKELONG(1, V+1), szText);
			}
			else
			{
				do
				{
					bool bActive = (pRCon->GetActiveTab() == R);
					_ASSERTE(bActive == ((tab->Flags() & fwt_CurrentFarWnd) == fwt_CurrentFarWnd));

					nAddFlags = 0/*((lbActiveVCon && (R==0)) ? MF_DEFAULT : 0)*/
						| ((lbActiveVCon && (abFirstTabOnly || bActive)) ? MF_CHECKED : MF_UNCHECKED)
						#if 0
						| ((tab->Flags() & etfDisabled) ? (MF_DISABLED|MF_GRAYED) : 0)
						#endif
						;

					LPCWSTR pszName = pRCon->GetTabTitle(tab);
					int nLen = lstrlen(pszName);
					if (!R)
						wsprintf(szText, L"%i: ", V+1);
					else
						wcscpy_c(szText, L"      ");
					if (nLen <= nMaxStrLen)
					{
						wcscat_c(szText, pszName);
					}
					else
					{
						int nCurLen = lstrlen(szText);
						_ASSERTE((nCurLen+10)<nMaxStrLen);
						if (tab->Type() == fwt_Panels)
						{
							lstrcpyn(szText+nCurLen, pszName, nMaxStrLen-1-nCurLen);
						}
						else
						{
							szText[nCurLen++] = L'\x2026'; szText[nCurLen] = 0;
							lstrcpyn(szText+nCurLen, pszName+nLen-nMaxStrLen, nMaxStrLen-1-nCurLen);
						}
						wcscat_c(szText, L"\x2026"); //...
					}
					AppendMenu(h, MF_STRING|nAddFlags, MAKELONG(R+1, V+1), szText);
				} while (!abFirstTabOnly && pRCon->GetTab(++R, tab));
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

// abFromConsole - menu was triggered by either mouse click on the *console surface*, or hotkey presses in the console
HMENU CConEmuMenu::CreateVConPopupMenu(CVirtualConsole* apVCon, HMENU ahExist, bool abAddNew, bool abFromConsole)
{
	//BOOL lbEnabled = TRUE;
	HMENU hMenu = ahExist;

	CVConGuard VCon;
	if (!apVCon && (CVConGroup::GetActiveVCon(&VCon) >= 0))
		apVCon = VCon.VCon();

	HMENU& hTerminate = mh_TerminatePopup;
	if (!hTerminate)
	{
		// "Close or &kill"
		hTerminate = CreatePopupMenu();
		AppendMenu(hTerminate, MF_STRING|MF_ENABLED, IDM_TERMINATECON,     MenuAccel(vkMultiClose,L"Close active &console"));
		AppendMenu(hTerminate, MF_STRING|MF_ENABLED, IDM_TERMINATEPRC,     MenuAccel(vkTerminateApp,L"Kill &active process"));
		AppendMenu(hTerminate, MF_STRING|MF_ENABLED, IDM_TERMINATEBUTSHELL,MenuAccel(vkTermButShell,L"Kill all but &shell"));
		AppendMenu(hTerminate, MF_SEPARATOR, 0, L"");
		AppendMenu(hTerminate, MF_STRING|MF_ENABLED, IDM_TERMINATEGROUP,   MenuAccel(vkCloseGroup,L"Close active &group"));
		AppendMenu(hTerminate, MF_STRING|MF_ENABLED, IDM_TERMINATEPRCGROUP,MenuAccel(vkCloseGroupPrc,L"Kill active &processes"));
		AppendMenu(hTerminate, MF_SEPARATOR, 0, L"");
		AppendMenu(hTerminate, MF_STRING|MF_ENABLED, IDM_TERMINATEALLCON,  MenuAccel(vkCloseAllCon,L"Close &all consoles"));
		AppendMenu(hTerminate, MF_STRING|MF_ENABLED, IDM_TERMINATEZOMBIES, MenuAccel(vkCloseZombies,L"Close all &zombies"));
		AppendMenu(hTerminate, MF_STRING|MF_ENABLED, IDM_TERMINATECONEXPT, MenuAccel(vkCloseExceptCon,L"Close e&xcept active"));
		AppendMenu(hTerminate, MF_SEPARATOR, 0, L"");
		AppendMenu(hTerminate, MF_STRING|MF_ENABLED, IDM_DETACH,           MenuAccel(vkConDetach,L"Detach"));
		AppendMenu(hTerminate, MF_STRING|MF_ENABLED, IDM_UNFASTEN,         MenuAccel(vkConUnfasten,L"Unfasten"));
	}

	HMENU& hRestart = mh_RestartPopup;
	if (!hRestart)
	{
		// "&Restart or duplicate"
		hRestart = CreatePopupMenu();
		AppendMenu(hRestart, MF_STRING|MF_ENABLED, IDM_DUPLICATE,        MenuAccel(vkDuplicateRoot,L"Duplica&te root..."));
		//AppendMenu(hMenu,  MF_STRING|MF_ENABLED, IDM_ADMIN_DUPLICATE,  MenuAccel(vkDuplicateRootAs,L"Duplica&te as Admin..."));
		AppendMenu(hRestart, MF_SEPARATOR, 0, L"");
		AppendMenu(hRestart, MF_STRING|MF_ENABLED, IDM_SPLIT2RIGHT,      MenuAccel(vkSplitNewConH,L"Split to ri&ght"));
		AppendMenu(hRestart, MF_STRING|MF_ENABLED, IDM_SPLIT2BOTTOM,     MenuAccel(vkSplitNewConV,L"Split to &bottom"));
		AppendMenu(hRestart, MF_SEPARATOR, 0, L"");
		AppendMenu(hRestart, MF_STRING|MF_ENABLED, IDM_RESTARTDLG,       MenuAccel(vkMultiRecreate,L"&Restart..."));
		AppendMenu(hRestart, MF_STRING|MF_ENABLED, IDM_RESTART,          L"&Restart");
		AppendMenu(hRestart, MF_STRING|MF_ENABLED, IDM_RESTARTAS,        L"Restart as Admin");
	}

	CEStr lsCloseText;
	if (abFromConsole)
		lsCloseText.Set(L"&Close console");
	else if (gpSet->isOneTabPerGroup && CVConGroup::isGroup(apVCon))
		lsCloseText.Set(L"&Close tab group");
	else
		lsCloseText.Set(L"&Close tab");
	lsCloseText.Set(MenuAccel(vkCloseTab, lsCloseText.ms_Val));

	if (!hMenu)
	{
		hMenu = CreatePopupMenu();

		AppendMenu(hMenu,      MF_STRING|MF_ENABLED, IDM_CLOSE,            lsCloseText);
		AppendMenu(hMenu,      MF_STRING|MF_ENABLED, IDM_RENAMETAB,        MenuAccel(vkRenameTab,L"Rena&me tab..."));
		AppendMenu(hMenu,      MF_STRING|MF_ENABLED, IDM_CHANGEAFFINITY,   MenuAccel(vkAffinity,L"A&ffinity/priority..."));
		AppendMenu(hMenu,      MF_POPUP|MF_ENABLED, (UINT_PTR)hRestart,    L"&Restart or duplicate");
		AppendMenu(hMenu,      MF_POPUP|MF_ENABLED, (UINT_PTR)hTerminate,  L"Close or &kill");
		AppendMenu(hMenu,      MF_STRING|((apVCon && apVCon->GuiWnd())?MF_ENABLED:0),
		                                             IDM_CHILDSYSMENU,     MenuAccel(vkChildSystemMenu,L"Child system menu..."));
		if (abAddNew)
		{
			AppendMenu(hMenu,  MF_SEPARATOR, 0, L"");
			AppendMenu(hMenu,  MF_STRING|MF_ENABLED, ID_NEWCONSOLE,        MenuAccel(vkMultiNew,L"New console..."));
			AppendMenu(hMenu,  MF_STRING|MF_ENABLED, IDM_ATTACHTO,         MenuAccel(vkMultiNewAttach,L"Attach to..."));
		}
		#if 0
		// Смысл выносить избранный макро заточенный только под редактор Far?
		AppendMenu(hMenu, MF_SEPARATOR, 0, L"");
		AppendMenu(hMenu, MF_STRING|MF_ENABLED,     IDM_SAVE,      L"&Save");
		AppendMenu(hMenu, MF_STRING|MF_ENABLED,     IDM_SAVEALL,   L"Save &all");
		#endif
	}
	else
	{
		MENUITEMINFO mi = {sizeof(mi)};
		wchar_t szText[255]; wcscpy_c(szText, lsCloseText);
		mi.fMask = MIIM_STRING; mi.dwTypeData = szText;
		SetMenuItemInfo(hMenu, IDM_CLOSE, FALSE, &mi);
	}

	if (apVCon)
	{
		//bool lbIsFar = apVCon->RCon()->isFar(TRUE/* abPluginRequired */)!=FALSE;
		//#ifdef _DEBUG
		//bool lbIsPanels = lbIsFar && apVCon->RCon()->isFilePanel(false/* abPluginAllowed */)!=FALSE;
		//#endif
		//bool lbIsEditorModified = lbIsFar && apVCon->RCon()->isEditorModified()!=FALSE;
		//bool lbHaveModified = lbIsFar && apVCon->RCon()->GetModifiedEditors()!=0;
		bool lbCanCloseTab = apVCon->RCon()->CanCloseTab();

		//if (lbHaveModified)
		//{
		//	if (!gpSet->sSaveAllMacro || !*gpSet->sSaveAllMacro)
		//		lbHaveModified = false;
		//}

		EnableMenuItem(hMenu,      IDM_CLOSE,             MF_BYCOMMAND|(lbCanCloseTab ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(hMenu,      IDM_DETACH,            MF_BYCOMMAND|MF_ENABLED);
		EnableMenuItem(hMenu,      IDM_DUPLICATE,         MF_BYCOMMAND|MF_ENABLED);
		EnableMenuItem(hMenu,      IDM_SPLIT2RIGHT,       MF_BYCOMMAND|MF_ENABLED);
		EnableMenuItem(hMenu,      IDM_SPLIT2BOTTOM,      MF_BYCOMMAND|MF_ENABLED);
		EnableMenuItem(hMenu,      IDM_RENAMETAB,         MF_BYCOMMAND|MF_ENABLED);
		EnableMenuItem(hMenu,      IDM_CHANGEAFFINITY,    MF_BYCOMMAND|MF_ENABLED);
		EnableMenuItem(hTerminate, IDM_TERMINATECON,      MF_BYCOMMAND|MF_ENABLED);
		EnableMenuItem(hTerminate, IDM_TERMINATEGROUP,    MF_BYCOMMAND|MF_ENABLED);
		EnableMenuItem(hTerminate, IDM_TERMINATEPRCGROUP, MF_BYCOMMAND|MF_ENABLED);
		EnableMenuItem(hTerminate, IDM_TERMINATEPRC,      MF_BYCOMMAND|MF_ENABLED);
		EnableMenuItem(hTerminate, IDM_TERMINATEBUTSHELL, MF_BYCOMMAND|MF_ENABLED);
		EnableMenuItem(hMenu,      IDM_CHILDSYSMENU,      MF_BYCOMMAND|((apVCon && apVCon->GuiWnd()) ? 0 : MF_GRAYED));
		EnableMenuItem(hMenu,      IDM_RESTARTDLG,        MF_BYCOMMAND|MF_ENABLED);
		EnableMenuItem(hMenu,      IDM_RESTART,           MF_BYCOMMAND|MF_ENABLED);
		EnableMenuItem(hMenu,      IDM_RESTARTAS,         MF_BYCOMMAND|MF_ENABLED);
		//EnableMenuItem(hMenu,    IDM_ADMIN_DUPLICATE,   MF_BYCOMMAND|(lbIsPanels ? MF_ENABLED : MF_GRAYED));
		//EnableMenuItem(hMenu,      IDM_SAVE,              MF_BYCOMMAND|(lbIsEditorModified ? MF_ENABLED : MF_GRAYED));
		//EnableMenuItem(hMenu,      IDM_SAVEALL,           MF_BYCOMMAND|(lbHaveModified ? MF_ENABLED : MF_GRAYED));
	}
	else
	{
		EnableMenuItem(hMenu,      IDM_CLOSE,             MF_BYCOMMAND|MF_GRAYED);
		EnableMenuItem(hMenu,      IDM_DETACH,            MF_BYCOMMAND|MF_GRAYED);
		EnableMenuItem(hMenu,      IDM_DUPLICATE,         MF_BYCOMMAND|MF_GRAYED);
		EnableMenuItem(hMenu,      IDM_SPLIT2RIGHT,       MF_BYCOMMAND|MF_GRAYED);
		EnableMenuItem(hMenu,      IDM_SPLIT2BOTTOM,      MF_BYCOMMAND|MF_GRAYED);
		EnableMenuItem(hMenu,      IDM_RENAMETAB,         MF_BYCOMMAND|MF_GRAYED);
		EnableMenuItem(hMenu,      IDM_CHANGEAFFINITY,    MF_BYCOMMAND|MF_GRAYED);
		EnableMenuItem(hTerminate, IDM_TERMINATECON,      MF_BYCOMMAND|MF_GRAYED);
		EnableMenuItem(hTerminate, IDM_TERMINATEGROUP,    MF_BYCOMMAND|MF_GRAYED);
		EnableMenuItem(hTerminate, IDM_TERMINATEPRCGROUP, MF_BYCOMMAND|MF_GRAYED);
		EnableMenuItem(hTerminate, IDM_TERMINATEPRC,      MF_BYCOMMAND|MF_GRAYED);
		EnableMenuItem(hTerminate, IDM_TERMINATEBUTSHELL, MF_BYCOMMAND|MF_GRAYED);
		EnableMenuItem(hMenu,      IDM_CHILDSYSMENU,      MF_BYCOMMAND|MF_GRAYED);
		EnableMenuItem(hMenu,      IDM_RESTARTDLG,        MF_BYCOMMAND|MF_GRAYED);
		EnableMenuItem(hMenu,      IDM_RESTART,           MF_BYCOMMAND|MF_GRAYED);
		EnableMenuItem(hMenu,      IDM_RESTARTAS,         MF_BYCOMMAND|MF_GRAYED);
		//EnableMenuItem(hMenu,      IDM_SAVE,              MF_BYCOMMAND|MF_GRAYED);
		//EnableMenuItem(hMenu,      IDM_SAVEALL,           MF_BYCOMMAND|MF_GRAYED);
	}

	return hMenu;
}

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

	const BYTE fmt = gpSet->isCTSHtmlFormat;
	// MenuItemType mit; UINT MenuId; UINT HotkeyId; UINT Flags; LPCWSTR pszText;
	MenuItem Items[] = {
		{ mit_Command,   ID_CON_MARKBLOCK,  vkCTSVkBlockStart, (lbEnabled?MF_ENABLED:MF_GRAYED), L"Mark &block"       },
		{ mit_Command,   ID_CON_MARKTEXT,   vkCTSVkTextStart,  (lbEnabled?MF_ENABLED:MF_GRAYED), L"Mar&k text"        },
		{ mit_Separator },
		{ mit_Command,   ID_CON_COPY,       0,          (lbSelectionExist?MF_ENABLED:MF_GRAYED), L"Cop&y"             },
		{ mit_Command,   ID_CON_COPY_ALL,   vkCTSVkCopyAll,    (lbEnabled?MF_ENABLED:MF_GRAYED), L"Copy &all"         },
		{ mit_Command,   ID_CON_PASTE,      vkPasteText,       (lbEnabled?MF_ENABLED:MF_GRAYED), L"&Paste"            },
		{ mit_Separator },
		{ mit_Command,   ID_RESET_TERMINAL, vkResetTerminal,   (lbEnabled?MF_ENABLED:MF_GRAYED), L"&Reset terminal"   },
		{ mit_Separator },
		{ mit_Option,    ID_CON_COPY_HTML0, 0,             ((fmt == 0)?MF_CHECKED:MF_UNCHECKED), L"Plain &text only"  },
		{ mit_Option,    ID_CON_COPY_HTML1, 0,             ((fmt == 1)?MF_CHECKED:MF_UNCHECKED), L"Copy &HTML format" },
		{ mit_Option,    ID_CON_COPY_HTML2, 0,             ((fmt == 2)?MF_CHECKED:MF_UNCHECKED), L"Copy a&s HTML"     },
		{ mit_Option,    ID_CON_COPY_HTML3, 0,             ((fmt == 3)?MF_CHECKED:MF_UNCHECKED), L"A&NSI sequences"   },
		{ mit_Separator },
		{ mit_Command,   ID_CON_FIND,       vkFindTextDlg,     (lbEnabled?MF_ENABLED:MF_GRAYED), L"&Find text..."     },
	};

	CreateOrUpdateMenu(hMenu, Items, countof(Items));

	return hMenu;
}

HMENU CConEmuMenu::CreateViewMenuPopup(CVirtualConsole* apVCon, HMENU ahExist /*= NULL*/)
{
	CVConGuard VCon;
	if (!apVCon && (CVConGroup::GetActiveVCon(&VCon) >= 0))
		apVCon = VCon.VCon();

	bool  bNew = (ahExist == NULL);
	HMENU hMenu = bNew ? CreatePopupMenu() : ahExist;

	const ColorPalette* pPal = NULL;

	int iActiveIndex = apVCon->GetPaletteIndex();

	int i = 0;
	int iBreak = 0;
	while ((i < (ID_CON_SETPALETTE_LAST-ID_CON_SETPALETTE_FIRST)) && (pPal = gpSet->PaletteGet(i)))
	{
		if (!pPal->pszName)
		{
			_ASSERTE(pPal->pszName);
			break;
		}
		wchar_t szItem[128] = L"";
		CmdTaskPopupItem::SetMenuName(szItem, countof(szItem), pPal->pszName, true);

		if (!iBreak && i && (pPal->pszName[0] != L'<'))
			iBreak = i;

		MENUITEMINFO mi = {sizeof(mi)};
		mi.fMask = MIIM_STRING|MIIM_STATE|MIIM_FTYPE;
		mi.dwTypeData = szItem; mi.cch = countof(szItem);
		mi.fState =
			// Add 'CheckMark' to the current palette (if it differs from ConEmu global one)
			((i==iActiveIndex) ? MFS_CHECKED : MFS_UNCHECKED)
			;
		mi.fType = MFT_STRING
			// Ensure palettes list will not be too long, ATM there are 25 predefined palettes
			| ((iBreak && !(i % iBreak)) ? MFT_MENUBREAK : 0)
			;
		if (bNew || !SetMenuItemInfo(hMenu, ID_CON_SETPALETTE_FIRST+i, FALSE, &mi))
		{
			_ASSERTE(MFS_CHECKED == MF_CHECKED && MFT_MENUBREAK == MF_MENUBREAK && MFT_STRING == 0);
			AppendMenu(hMenu, MF_STRING | mi.fState | mi.fType, ID_CON_SETPALETTE_FIRST+i, szItem);
		}

		i++;
	}

	for (int j = i; j <= ID_CON_SETPALETTE_LAST; j++)
	{
		DeleteMenu(hMenu, ID_CON_SETPALETTE_FIRST+j, MF_BYCOMMAND);
	}

	return hMenu;
}

HMENU CConEmuMenu::CreateHelpMenuPopup()
{
	HMENU hHelp = CreatePopupMenu();

	if (gpConEmu->isUpdateAllowed())
	{
		if (gpUpd && gpUpd->InUpdate())
			AppendMenu(hHelp, MF_STRING|MF_ENABLED, ID_STOPUPDATE, _T("&Stop updates checking"));
		else
			AppendMenu(hHelp, MF_STRING|MF_ENABLED, ID_CHECKUPDATE, _T("&Check for updates"));

		AppendMenu(hHelp, MF_SEPARATOR, 0, NULL);
	}

	AppendMenu(hHelp, MF_STRING|MF_ENABLED, IDM_HOMEPAGE, _T("&Visit home page"));
	AppendMenu(hHelp, MF_STRING|MF_ENABLED, IDM_DONATE_LINK, _T("&Donate ConEmu"));

	AppendMenu(hHelp, MF_STRING|MF_ENABLED, ID_WHATS_NEW_FILE, _T("Whats &new (local)"));
	AppendMenu(hHelp, MF_STRING|MF_ENABLED, ID_WHATS_NEW_WWW, _T("Whats new (&web)"));
	AppendMenu(hHelp, MF_SEPARATOR, 0, NULL);

	AppendMenu(hHelp, MF_STRING|MF_ENABLED, IDM_REPORTBUG, _T("&Report a bug..."));

	if (gpConEmu->ms_ConEmuChm[0])  //Показывать пункт только если есть conemu.chm
		AppendMenu(hHelp, MF_STRING|MF_ENABLED, ID_HELP, _T("&Help"));

	AppendMenu(hHelp, MF_SEPARATOR, 0, NULL);
	AppendMenu(hHelp, MF_STRING|MF_ENABLED, ID_HOTKEYS, MenuAccel(vkWinAltK,L"Hot&keys"));
	AppendMenu(hHelp, MF_STRING|MF_ENABLED, ID_ONLINEHELP, MenuAccel(vkWinAltH,L"Online &Help"));
	AppendMenu(hHelp, MF_STRING|MF_ENABLED, ID_ABOUT, MenuAccel(vkWinAltA,L"&About / Help"));

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
	if (!VkMod || !ConEmuHotKey::GetHotkey(VkMod) || !pHK)
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

LRESULT CConEmuMenu::OnSysCommand(HWND hWnd, WPARAM wParam, LPARAM lParam, UINT Msg /*= 0*/)
{
	static LONG InCall = 0;
	MSetter inCall(&InCall);
	wchar_t szDbg[128], szName[32];
	wcscpy_c(szName, Msg == WM_SYSCOMMAND ? L" - WM_SYSCOMMAND" : Msg == mn_MsgOurSysCommand ? L" - UM_SYSCOMMAND" : L"");
	_wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"OnSysCommand (%i(0x%X), %i, %i)%s", (DWORD)wParam, (DWORD)wParam, (DWORD)lParam, InCall, szName);
	LogString(szDbg);

	#ifdef _DEBUG
	wcscat_c(szDbg, L"\n");
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
		#ifdef _DEBUG
		int nNewR = ((int)LOWORD(wParam))-1; UNREFERENCED_PARAMETER(nNewR);
		#endif

		CVConGuard VCon;
		if (CVConGroup::GetVCon(nNewV, &VCon))
		{
			// -- в SysMenu показываются только консоли (редакторов/вьюверов там нет)
			if (!VCon->isActive(false))
			{
				gpConEmu->Activate(VCon.VCon());
			}
		}
		return 0;
	}

	//switch(LOWORD(wParam))
	switch (wParam)
	{
		case ID_NEWCONSOLE:
			// Создать новую консоль
			gpConEmu->RecreateAction(gpSetCls->GetDefaultCreateAction(), gpSet->isMultiNewConfirm || isPressed(VK_SHIFT));
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

		case ID_RESET_TERMINAL:
			{
				CVConGuard VCon;
				if (gpConEmu->GetActiveVCon(&VCon) >= 0)
				{
					CEStr szMacro(L"Write(\"\\ec\")"), szResult;
					szResult = ConEmuMacro::ExecuteMacro(szMacro.ms_Val, VCon->RCon());
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
					CECopyMode CopyMode = (wParam == ID_CON_COPY_ALL) ? cm_CopyAll : cm_CopySel;
					VCon->RCon()->DoSelectionCopy(CopyMode);
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

		case ID_CON_COPY_HTML0:
			gpSet->isCTSHtmlFormat = 0;
			return 0;
		case ID_CON_COPY_HTML1:
			gpSet->isCTSHtmlFormat = 1;
			return 0;
		case ID_CON_COPY_HTML2:
			gpSet->isCTSHtmlFormat = 2;
			return 0;
		case ID_CON_COPY_HTML3:
			gpSet->isCTSHtmlFormat = 3;
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
				gpConEmu->DoAlwaysOnTopSwitch();

				HWND hExt = gpSetCls->GetPage(thi_Features);

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
		case ID_MINIDUMPTREE:
			gpConEmu->MemoryDumpActiveProcess(wParam==ID_MINIDUMPTREE);
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
			ConEmuAbout::OnInfo_ThrowTrapException(true);
			return 0;
		case ID_DEBUG_TRAP2:
			ConEmuAbout::OnInfo_ThrowTrapException(false);
			return 0;
		case ID_DEBUG_ASSERT:
			_ASSERT(FALSE && "This is DEBUG test assertion");
			AssertBox(_T("FALSE && \"This is RELEASE test assertion\""), _T(__FILE__), __LINE__);
			return 0;

		case ID_DUMP_MEM_BLK:
			#ifdef USE_XF_DUMP
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
			ConEmuAbout::OnInfo_Help();
			return 0;
		} // case ID_HELP:

		case IDM_HOMEPAGE:
		{
			ConEmuAbout::OnInfo_HomePage();
			return 0;
		}

		case IDM_REPORTBUG:
		{
			ConEmuAbout::OnInfo_ReportBug();
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

		case IDM_DONATE_LINK:
		{
			ConEmuAbout::OnInfo_Donate();
			return 0;
		}

		case ID_ONLINEHELP:
		{
			ConEmuAbout::OnInfo_OnlineWiki();
			return 0;
		}

		case ID_ABOUT:
		{
			ConEmuAbout::OnInfo_About();
			return 0;
		}

		case ID_WHATS_NEW_FILE:
		case ID_WHATS_NEW_WWW:
		{
			ConEmuAbout::OnInfo_WhatsNew((wParam == ID_WHATS_NEW_FILE));
			return 0;
		}

		case ID_TOMONITOR:
		{
			gpConEmu->DoBringHere();
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
			gpConEmu->OnScClose();
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
					gpConEmu->DoMinimizeRestore(sih_Show/*sih_HideTSA*/);
					break;
				}

				if (gpConEmu->SetWindowMode(gpConEmu->isIconic() ? gpConEmu->GetWindowMode() : wmNormal))
				{
					// abForceChild=TRUE, если в табе запущено GUI приложение - можно передать в него фокус
					gpConEmu->OnFocus(ghWnd, WM_ACTIVATEAPP, TRUE, 0, L"After SC_RESTORE", TRUE);
					break;
				}
			}

			// ***
			{
				bool bIconic = _bool(::IsIconic(hWnd));
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
					gpConEmu->DoMinimizeRestore(sih_AutoHide);
					// We still need to initiate 'Minimize' behavior
					// to make OS move the focus to the previous window
					// So, continue to the WM_SYSCOMMAND
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
				if (InCall > 2)
				{
					_ASSERTE(InCall == 1); // Infinite loop?
					break;
				}
				CVConGuard VCon;
				if (CVConGroup::GetActiveVCon(&VCon) >= 0)
					ExecPopupMenuCmd(tmp_System, VCon.VCon(), (int)(DWORD)wParam);
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
					ShowMenuHint(NULL);
				}

				result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);

				if (wParam == SC_SYSMENUPOPUP_SECRET)
				{
					mn_TrackMenuPlace = tmp_None;
					ShowMenuHint(NULL);
				}
			}
		} // default:
	}

	return result;
}
