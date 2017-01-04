
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


#pragma once

#include "MenuIds.h"

enum MenuItemType
{
	mit_Separator,
	mit_Command,
	mit_Option,
};

struct MenuItem
{
	MenuItemType mit;
	UINT MenuId;
	UINT HotkeyId;
	LONG Flags; // MF_xxx are declared as LONG constants
	LPCWSTR pszText;
};

class CConEmuMenu
{
public:
	CConEmuMenu();
	~CConEmuMenu();

public:
	void OnNewConPopupMenu(POINT* ptWhere = NULL, DWORD nFlags = 0, bool bShowTaskItems = false);

	LRESULT OnInitMenuPopup(HWND hWnd, HMENU hMenu, LPARAM lParam);
	bool OnMenuSelected(HMENU hMenu, WORD nID, WORD nFlags);
	void OnMenuRClick(HMENU hMenu, UINT nItemPos);

	POINT CalcTabMenuPos(CVirtualConsole* apVCon);
	HMENU GetSysMenu(BOOL abInitial = FALSE);

	void ShowSysmenu(int x=-32000, int y=-32000, DWORD nFlags = 0);
	void OnNcIconLClick();

	HMENU CreateDebugMenuPopup();
	HMENU CreateEditMenuPopup(CVirtualConsole* apVCon, HMENU ahExist = NULL);
	HMENU CreateViewMenuPopup(CVirtualConsole* apVCon, HMENU ahExist = NULL);
	HMENU CreateHelpMenuPopup();
	HMENU CreateVConListPopupMenu(HMENU ahExist, BOOL abFirstTabOnly);
	HMENU CreateVConPopupMenu(CVirtualConsole* apVCon, HMENU ahExist, bool abAddNew, bool abFromConsole);
	bool  CreateOrUpdateMenu(HMENU& hMenu, const MenuItem* Items, size_t ItemsCount);

	int trackPopupMenu(TrackMenuPlace place, HMENU hMenu, UINT uFlags, int x, int y, HWND hWnd, RECT *prcRect = NULL);

	void ShowPopupMenu(CVirtualConsole* apVCon, POINT ptCur, DWORD Align = TPM_LEFTALIGN, bool abFromConsole = false);
	void ExecPopupMenuCmd(TrackMenuPlace place, CVirtualConsole* apVCon, int nCmd);

	LRESULT OnSysCommand(HWND hWnd, WPARAM wParam, LPARAM lParam, UINT Msg = 0);

	// Returns previous value
	bool SetPassSysCommand(bool abPass = true);
	bool GetPassSysCommand();
	bool SetInScMinimize(bool abInScMinimize);
	bool GetInScMinimize();
	bool SetRestoreFromMinimized(bool abInRestore);
	bool GetRestoreFromMinimized();
	TrackMenuPlace SetTrackMenuPlace(TrackMenuPlace tmpPlace);
	TrackMenuPlace GetTrackMenuPlace();

private:
	bool OnMenuSelected_NewCon(HMENU hMenu, WORD nID, WORD nFlags);
	void OnNewConPopupMenuRClick(HMENU hMenu, UINT nItemPos);

	void UpdateSysMenu(HMENU hSysMenu);

	void ShowMenuHint(HMENU hMenu, WORD nID, WORD nFlags);
	void ShowMenuHint(LPCWSTR pszText, POINT* ppt = NULL);

	// В режиме "правый клик по keybar в FarManager" - при выделении
	// пункта (например Alt+Shift+F9) "нажать" в консоли Alt+Shift
	// чтобы Far перерисовал панель кнопок (показал подсказку для Alt+Shift+...)
	void ShowKeyBarHint(HMENU hMenu, WORD nID, WORD nFlags);

	LPCWSTR MenuAccel(int DescrID, LPCWSTR asText);

private:
	bool  mb_InNewConPopup, mb_InNewConRPopup;
	//int   mn_FirstTaskID, mn_LastTaskID; // MenuItemID for Tasks, when mb_InNewConPopup==true
	DWORD mn_SysMenuOpenTick, mn_SysMenuCloseTick;
	bool  mb_PassSysCommand;
	bool  mb_InScMinimize;
	bool  mb_InRestoreFromMinimized;
	UINT  mn_MsgOurSysCommand;
	bool  mb_FromConsoleMenu;

	TrackMenuPlace mn_TrackMenuPlace;

	//struct CmdHistory
	//{
	//	int nCmd;
	//	LPCWSTR pszCmd;
	//	wchar_t szShort[32];
	//} m_CmdPopupMenu[MAX_CMD_HISTORY+1]; // структура для меню выбора команды новой консоли

	struct CmdTaskPopupItem
	{
		enum CmdTaskPopupItemType { eNone, eTaskPopup, eTaskAll, eTaskCmd, eMore, eCmd, eNewDlg, eSetupTasks, eClearHistory } ItemType;
		int nCmd;
		const void/*CommandTasks*/* pGrp;
		LPCWSTR pszCmd;
		wchar_t szShort[64];
		HMENU hPopup;
		wchar_t* pszTaskBuf;
		BOOL bPopupInitialized;

		void Reset(CmdTaskPopupItemType newItemType, int newCmdId, LPCWSTR asName = NULL);
		void SetShortName(LPCWSTR asName, bool bRightQuote = false, LPCWSTR asHotKey = NULL);
		static void SetMenuName(wchar_t* pszDisplay, INT_PTR cchDisplayMax, LPCWSTR asName, bool bTrailingPeriod, bool bRightQuote = false, LPCWSTR asHotKey = NULL);
	};
	bool mb_CmdShowTaskItems;
	MArray<CmdTaskPopupItem> m_CmdTaskPopup;
	int mn_CmdLastID;
	CmdTaskPopupItem* mp_CmdRClickForce;
	int FillTaskPopup(HMENU hMenu, CmdTaskPopupItem* pParent);

	// Эти из CConEmuMain
	HMENU mh_SysDebugPopup, mh_SysEditPopup, mh_ActiveVConPopup, mh_TerminateVConPopup, mh_VConListPopup, mh_HelpPopup; // Popup's для SystemMenu
	HMENU mh_InsideSysMenu;
	// А это из VirtualConsole
	HMENU mh_PopupMenu;
	HMENU mh_TerminatePopup;
	HMENU mh_RestartPopup;
	HMENU mh_VConDebugPopup;
	HMENU mh_VConEditPopup;
	HMENU mh_VConViewPopup;
	// Array
	size_t mn_MenusCount; HMENU** mph_Menus;
};
