
/*
Copyright (c) 2009-2017 Maximus5
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

// --> Header.h
//enum CEStatusFlags
//{
//	csf_VertDelim           = 0x00000001,
//	csf_HorzDelim           = 0x00000002,
//	csf_SystemColors        = 0x00000004,
//	csf_NoVerticalPad       = 0x00000008,
//};


// Добавлять можно в любое место, настройки именованые
enum CEStatusItems
{
	csi_Info = 0,

	csi_ActiveVCon,

	csi_CapsLock,
	csi_NumLock,
	csi_ScrollLock,
	csi_ViewLock,
	csi_InputLocale,
	csi_KeyHooks,
	csi_TermModes,
	csi_RConModes,

	csi_WindowPos,
	csi_WindowSize,
	csi_WindowClient,
	csi_WindowWork,
	csi_WindowBack,
	csi_WindowDC,
	csi_WindowStyle,
	csi_WindowStyleEx,
	csi_HwndFore,
	csi_HwndFocus,
	csi_Zoom,
	csi_DPI,

	csi_ActiveBuffer,
	csi_ConsolePos,
	csi_ConsoleSize,
	csi_BufferSize,
	csi_CursorX,
	csi_CursorY,
	csi_CursorSize,
	csi_CursorInfo,
	csi_CellInfo,
	csi_ConEmuPID,
	csi_ConEmuHWND,
	csi_ConEmuView,
	csi_Server,
	csi_ServerHWND,
	csi_Transparency,

	csi_NewVCon,
	csi_SyncInside,
	csi_ActiveProcess,
	csi_ConsoleTitle,

	csi_Time,

	csi_SizeGrip,

	//
	csi_Last
};

struct StatusColInfo
{
	CEStatusItems nID;
	LPCWSTR sSettingName;
	LPCWSTR sName; // Shown in menu (select active columns)
	LPCWSTR sHelp; // Shown in Info, when hover over item
};

class CRealConsole;

class CStatus
{
private:
	// Warning!!! тут индекс не соответствует nID, т.к. некоторые элементы могут быть отключены
	// Warning!!! а в этом векторе добавлены только видимые элементы!
	struct strItems {
		BOOL bShow;
		CEStatusItems nID;
		wchar_t sText[255];
		int nTextLen;
		SIZE TextSize;
		wchar_t szFormat[64];
		RECT rcClient; // Координаты относительно клиентской области ghWnd
	} m_Items[csi_Last];

	// В этом векторе - nID уже не нужен, т.к. здесь это индекс
	struct strData {
		LPCWSTR sSettingName;
		LPCWSTR sName;
		LPCWSTR sHelp;
		wchar_t sText[255];
		wchar_t szFormat[64];
	} m_Values[csi_Last];

	wchar_t ms_ConEmuBuild[32];

	RECT mrc_LastStatus;

	wchar_t ms_Status[200];

	RECT mrc_LastResizeCol;
	bool mb_StatusResizing;
	POINT mpt_StatusResizePt;
	POINT mpt_StatusResizeCmp;
	RECT mrc_StatusResizeRect;
	void DoStatusResize(const POINT& ptScr);

	SIZE mn_BmpSize;
	HBITMAP mh_Bmp, mb_OldBmp;
	HDC mh_MemDC;

	bool FillItems();

	CEStatusItems m_ClickedItemDesc;
	//wchar_t ms_ClickedItemDesc[128];
	void SetClickedItemDesc(CEStatusItems anID);

	//bool mb_WasClick;

	bool mb_InPopupMenu; // true - на время _любого_ PopupMenu
	bool mb_InSetupMenu; // true - для ShowStatusSetupMenu
	void ShowStatusSetupMenu();
	void ShowVConMenu(POINT pt);
	void ShowTransparencyMenu(POINT pt);
	void ShowZoomMenu(POINT pt);
	void ShowTermModeMenu(POINT pt);

	bool  mb_DataChanged; // данные изменились, нужна отрисовка
	//bool  mb_Invalidated;
	DWORD mn_LastPaintTick;

	bool OnDataChanged(CEStatusItems *ChangedID, size_t Count, bool abForceOnChange = false);

	bool LoadActiveProcess(CRealConsole* pRCon, wchar_t* pszText, int cchMax);

	bool mb_Caps, mb_Num, mb_Scroll;
	bool mb_KeyHooks;
	DWORD_PTR mhk_Locale; // CConEmuMain::GetActiveKeyboardLayout()
	bool IsKeyboardChanged();

	bool mb_ViewLock;
	wchar_t ms_ViewLockHint[100];

	DWORD mn_Style, mn_ExStyle;
	LONG mn_Zoom, mn_Dpi;
	HWND mh_Fore, mh_Focus;
	DWORD mn_ForePID, mn_FocusPID;
	wchar_t ms_ForeInfo[1024], ms_FocusInfo[1024];
	bool IsWindowChanged();

	SYSTEMTIME mt_LastTime;
	bool IsTimeChanged();

	bool ProcessTransparentMenuId(WORD nCmd, bool abAlphaOnly);
	bool ProcessZoomMenuId(WORD nCmd);
	bool ProcessTermModeMenuId(WORD nCmd);

	bool isSettingsOpened(UINT nOpenPageID = 0);

public:
	CStatus();
	virtual ~CStatus();

	static size_t GetAllStatusCols(StatusColInfo** ppColumns);

public:
	void PaintStatus(HDC hPaint, LPRECT prcStatus = NULL);
	void UpdateStatusBar(bool abForce = false, bool abRepaintNow = false);
	void InvalidateStatusBar(LPRECT rcInvalidated = NULL);

	void SetStatus(LPCWSTR asStatus);

	void OnTimer();
	void OnWindowReposition(const RECT* prcNew = NULL);
	void OnConsoleChanged(const CONSOLE_SCREEN_BUFFER_INFO* psbi, const CONSOLE_CURSOR_INFO* pci, const TOPLEFTCOORD* pTopLeft, bool bForceUpdate);
	void OnCursorChanged(const COORD* pcr, const CONSOLE_CURSOR_INFO* pci, int nMaxX = 0, int nMaxY = 0);
	void OnConsoleBufferChanged(CRealConsole* pRCon);
	//void OnConsoleTitleChanged(CRealConsole* pRCon);
	void OnActiveVConChanged(int nIndex/*0-based*/, CRealConsole* pRCon);
	void OnServerChanged(DWORD nMainServerPID, DWORD nAltServerPID);
	void OnKeyboardChanged();
	void OnTransparency();

	bool IsStatusResizing();
	bool IsCursorOverResizeMark(POINT ptCurClient);
	bool IsResizeAllowed();
	bool ProcessStatusMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, POINT ptCurClient, LRESULT& lResult);
	void ProcessMenuHighlight(HMENU hMenu, WORD nID, WORD nFlags);

	LPCWSTR GetSettingName(CEStatusItems nID);

	bool GetStatusBarClientRect(RECT* rc);
	bool GetStatusBarItemRect(CEStatusItems nID, RECT* rc);

public:
	typedef int (*StatusMenuOptionsChecked)(int nValue, int* pnNextValue);
	struct StatusMenuOptions
	{
		WORD nMenuID;
		LPCWSTR sText;
		int nValue;
		StatusMenuOptionsChecked pfnChecked;
	};
	static int Transparent_IsMenuChecked(int nValue, int* pnNextValue);
	static int Zoom_IsMenuChecked(int nValue, int* pnNextValue);
	static int TermMode_IsMenuChecked(int nValue, int* pnNextValue);
protected:
	HMENU CreateStatusMenu(StatusMenuOptions* pItems, size_t nCount);
	StatusMenuOptions* GetStatusMenuItem(WORD nMenuID, StatusMenuOptions* pItems, size_t nCount);
	int ShowStatusBarMenu(POINT pt, HMENU hPopup, CEStatusItems csi);
};
