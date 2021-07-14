
/*
Copyright (c) 2012-present Maximus5
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

// ReSharper disable once CppUnusedIncludeDirective
#include "MenuIds.h"

#include <vector>

enum MenuItemType
{
	mit_Separator,
	mit_Command,
	mit_Option,
	mit_Popup,
};

struct MenuItem
{
	MenuItemType mit;
	UINT_PTR     MenuId;  // HMENU for mit_Popup is here
	UINT         HotkeyId;
	UINT         Flags; // MF_xxx are declared as LONG constants
	LPCWSTR      pszText;
};

class CConEmuMenu
{
public:
	CConEmuMenu();
	~CConEmuMenu();

public:
	void OnNewConPopupMenu(POINT* ptWhere = nullptr, DWORD nFlags = 0, bool bShowTaskItems = false);

	LRESULT OnInitMenuPopup(HWND hWnd, HMENU hMenu, LPARAM lParam);
	bool OnMenuSelected(HMENU hMenu, WORD nID, WORD nFlags);
	void OnMenuRClick(HMENU hMenu, UINT nItemPos);

	POINT CalcTabMenuPos(CVirtualConsole* apVCon);
	HMENU GetSysMenu(BOOL abInitial = FALSE);

	void ShowSysmenu(int x=-32000, int y=-32000, DWORD nFlags = 0);
	void OnNcIconLClick();

	HMENU CreateDebugMenuPopup();
	HMENU CreateEditMenuPopup(CVirtualConsole* apVCon, HMENU ahExist = nullptr);
	HMENU CreateViewMenuPopup(CVirtualConsole* apVCon, HMENU ahExist = nullptr);
	HMENU CreateHelpMenuPopup();
	static HMENU CreateVConListPopupMenu(HMENU ahExist, BOOL abFirstTabOnly);
	HMENU CreateVConPopupMenu(CVirtualConsole* apVCon, HMENU ahExist, bool abAddNew, bool abFromConsole);
	bool  CreateOrUpdateMenu(HMENU& hMenu, const MenuItem* items, size_t ItemsCount);

	int trackPopupMenu(TrackMenuPlace place, HMENU hMenu, UINT uFlags, int x, int y, HWND hWnd, RECT *prcRect = nullptr);

	void ShowEditMenu(CVirtualConsole* apVCon, POINT ptCur, DWORD Align = TPM_LEFTALIGN);
	void ShowPopupMenu(CVirtualConsole* apVCon, POINT ptCur, DWORD Align = TPM_LEFTALIGN, bool abFromConsole = false);
	void ExecPopupMenuCmd(TrackMenuPlace place, CVirtualConsole* apVCon, int nCmd);

	LRESULT OnSysCommand(HWND hWnd, WPARAM wParam, LPARAM lParam, UINT Msg = 0);

	// Returns previous value
	bool SetPassSysCommand(bool abPass = true);
	bool GetPassSysCommand() const;
	bool SetInScMinimize(bool abInScMinimize);
	bool GetInScMinimize() const;
	bool SetRestoreFromMinimized(bool abInRestore);
	bool GetRestoreFromMinimized() const;
	TrackMenuPlace SetTrackMenuPlace(TrackMenuPlace tmpPlace);
	TrackMenuPlace GetTrackMenuPlace() const;

private:
	bool OnMenuSelected_NewCon(HMENU hMenu, WORD nID, WORD nFlags);
	void OnNewConPopupMenuRClick(HMENU hMenu, UINT nItemPos);

	void UpdateSysMenu(HMENU hSysMenu);

	void ShowMenuHint(HMENU hMenu, WORD nID, WORD nFlags);
	void ShowMenuHint(LPCWSTR pszText, POINT* ppt = nullptr);

	// В режиме "правый клик по keybar в FarManager" - при выделении
	// пункта (например Alt+Shift+F9) "нажать" в консоли Alt+Shift
	// чтобы Far перерисовал панель кнопок (показал подсказку для Alt+Shift+...)
	void ShowKeyBarHint(HMENU hMenu, WORD nID, WORD nFlags);

	LPCWSTR MenuAccel(int descrId, LPCWSTR asText);

private:
	bool  mb_InNewConPopup = false;
	bool mb_InNewConRPopup = false;
	//int   mn_FirstTaskID, mn_LastTaskID; // MenuItemID for Tasks, when mb_InNewConPopup==true
	DWORD mn_SysMenuOpenTick = 0;
	DWORD mn_SysMenuCloseTick = 0;
	bool  mb_PassSysCommand = false;
	bool  mb_InScMinimize = false;
	bool  mb_InRestoreFromMinimized = false;
	UINT  mn_MsgOurSysCommand;
	bool  mb_FromConsoleMenu = false;

	TrackMenuPlace mn_TrackMenuPlace = tmp_None;

	//struct CmdHistory
	//{
	//	int nCmd;
	//	LPCWSTR pszCmd;
	//	wchar_t szShort[32];
	//} m_CmdPopupMenu[MAX_CMD_HISTORY+1]; // структура для меню выбора команды новой консоли

	struct CmdTaskPopupItem
	{
		enum CmdTaskPopupItemType { eNone, eTaskPopup, eTaskAll, eTaskCmd, eMore, eCmd, eNewDlg, eSetupTasks, eClearHistory } ItemType = {};
		int nCmd = 0;
		const void/*CommandTasks*/* pGrp = nullptr;
		LPCWSTR pszCmd = nullptr; // pointer to the parent.pszTaskBuf
		wchar_t szShort[64] = L"";
		HMENU hPopup = nullptr;
		CEStr pszTaskBuf = nullptr;
		bool bPopupInitialized = false;

		void Reset(CmdTaskPopupItemType newItemType, int newCmdId, LPCWSTR asName = nullptr);
		void SetShortName(LPCWSTR asName, bool bRightQuote = false, LPCWSTR asHotKey = nullptr);
		static void SetMenuName(wchar_t* pszDisplay, size_t cchDisplayMax, LPCWSTR asName, bool bTrailingPeriod, bool bRightQuote = false, LPCWSTR asHotKey = nullptr);
	};
	bool mb_CmdShowTaskItems = false;
	MArray<CmdTaskPopupItem> m_CmdTaskPopup;
	int mn_CmdLastID = 0;
	CmdTaskPopupItem* mp_CmdRClickForce = nullptr;

	int FillTaskPopup(HMENU hMenu, CmdTaskPopupItem* pParent);

	// Used in CConEmuMain

	// Popup's for SystemMenu
	HMENU mh_SysDebugPopup = nullptr;
	HMENU mh_SysEditPopup = nullptr;
	HMENU mh_ActiveVConPopup = nullptr;
	HMENU mh_TerminateVConPopup = nullptr;
	HMENU mh_VConListPopup = nullptr;
	HMENU mh_HelpPopup = nullptr;

	HMENU mh_InsideSysMenu = nullptr;

	// Used in VirtualConsole
	HMENU mh_PopupMenu = nullptr;
	HMENU mh_TerminatePopup = nullptr;
	HMENU mh_RestartPopup = nullptr;
	HMENU mh_VConDebugPopup = nullptr;
	HMENU mh_VConEditPopup = nullptr;
	HMENU mh_VConViewPopup = nullptr;
	// Array
	const std::vector<HMENU*> m_phMenus;
};
