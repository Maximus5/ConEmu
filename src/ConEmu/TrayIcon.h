
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

#define TRAY_ITEM_HIDE_NAME     L"Hide to &TSA"
#define TRAY_ITEM_RESTORE_NAME  L"Restore from &TSA"

enum TrayIconMsgSource
{
	tsa_Source_None = 0,
	tsa_Source_Updater,
	tsa_Config_Error,
	tsa_Console_Size,
	tsa_Default_Term,
	tsa_Push_Notify,
};

class TrayIcon
{
	private:
		struct NOTIFYICONDATA_Win2k
		{
			DWORD cbSize;
			HWND hWnd;
			UINT uID;
			UINT uFlags;
			UINT uCallbackMessage;
			HICON hIcon;
			WCHAR  szTip[128];
			DWORD dwState;
			DWORD dwStateMask;
			WCHAR  szInfo[256];
			union {
				UINT  uTimeout;
				UINT  uVersion;  // used with NIM_SETVERSION, values 0, 3 and 4
			} DUMMYUNIONNAME;
			WCHAR  szInfoTitle[64];
			DWORD dwInfoFlags;
		};
		NOTIFYICONDATA_Win2k IconData;

		void SetMenuItemText(HMENU hMenu, UINT nID, LPCWSTR pszText);

		//UINT mn_SysItemId[5];
		//UINT mn_SysItemState[5];

		bool mb_InHidingToTray;
		bool mb_WindowInTray;
		TrayIconMsgSource m_MsgSource;
		bool mb_SecondTimeoutMsg;
		DWORD mn_BalloonShowTick;
		HWND mh_Balloon;

	public:
		bool isWindowInTray() { return mb_WindowInTray; }
		bool isHidingToTray() { return mb_InHidingToTray; }


		TrayIcon();
		~TrayIcon();

		void ShowTrayIcon(LPCTSTR asInfoTip = NULL, TrayIconMsgSource aMsgSource = tsa_Source_None);
		void HideWindowToTray(LPCTSTR asInfoTip = NULL);
		void RestoreWindowFromTray(bool abIconOnly = false, bool abDontCallShowWindow = false);
		void LoadIcon(HWND inWnd, int inIconResource);
		//void Delete();
		void UpdateTitle();
		void SettingsChanged();
		LRESULT OnTryIcon(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
		void AddTrayIcon();
		void RemoveTrayIcon(bool bForceRemove = false);

		void OnTaskbarCreated();
};
