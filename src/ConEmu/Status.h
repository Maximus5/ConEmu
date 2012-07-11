
/*
Copyright (c) 2009-2012 Maximus5
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

enum CEStatusItems
{
	csi_Info = 0,

	csi_ActiveVCon,
	csi_NewVCon,

	csi_SyncInside,

	csi_CapsLock,
	csi_NumLock,
	csi_ScrollLock,
	csi_InputLocale,

	csi_WindowPos,
	csi_WindowSize,
	csi_WindowClient,
	csi_WindowWork,

	csi_ActiveBuffer,
	csi_ConsolePos,
	csi_ConsoleSize,
	csi_BufferSize,
	csi_CursorX,
	csi_CursorY,
	csi_CursorSize,
	csi_CursorInfo,
	csi_ConEmuPID,
	csi_ConEmuHWND,
	csi_ConEmuView,
	csi_Server,
	csi_ServerHWND,
	csi_Transparency,

	//
	csi_Last
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

	RECT mrc_LastStatus;

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

	bool  mb_DataChanged; // данные изменились, нужна отрисовка
	//bool  mb_Invalidated;
	DWORD mn_LastPaintTick;

	void OnDataChanged(CEStatusItems *ChangedID, size_t Count, bool abForceOnChange = false);

	bool mb_Caps, mb_Num, mb_Scroll;
	DWORD_PTR mhk_Locale; // CConEmuMain::GetActiveKeyboardLayout()
	bool IsKeyboardChanged();

	bool ProcessTransparentMenuId(WORD nCmd, bool abAlphaOnly);

	bool isSettingsOpened(UINT nOpenPageID = 0);

	void InvalidateStatusBar();

public:
	CStatus();
	virtual ~CStatus();

public:
	void PaintStatus(HDC hPaint, RECT rcStatus);
	void UpdateStatusBar(bool abForce = false);

	void OnTimer();
	void OnWindowReposition(const RECT* prcNew = NULL);
	void OnConsoleChanged(const CONSOLE_SCREEN_BUFFER_INFO* psbi, const CONSOLE_CURSOR_INFO* pci, bool bForceUpdate);
	void OnCursorChanged(const COORD* pcr, const CONSOLE_CURSOR_INFO* pci, int nMaxX = 0, int nMaxY = 0);
	void OnConsoleBufferChanged(CRealConsole* pRCon);
	void OnActiveVConChanged(int nIndex/*0-based*/, CRealConsole* pRCon);
	void OnServerChanged(DWORD nMainServerPID, DWORD nAltServerPID);
	void OnKeyboardChanged();
	void OnTransparency();

	bool ProcessStatusMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, POINT ptCurClient, LRESULT& lResult);
	void ProcessMenuHighlight(HMENU hMenu, WORD nID, WORD nFlags);

	LPCWSTR GetSettingName(CEStatusItems nID);

	bool GetStatusBarClientRect(RECT* rc);

	void UpdateStatusFont();
};
