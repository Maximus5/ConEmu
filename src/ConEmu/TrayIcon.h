
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

#define TRAY_ITEM_HIDE_NAME     L"Hide to &TSA"
#define TRAY_ITEM_RESTORE_NAME  L"Restore from &TSA"

class TrayIcon
{
	private:
		NOTIFYICONDATA IconData;

		void SetMenuItemText(HMENU hMenu, UINT nID, LPCWSTR pszText);

		//UINT mn_SysItemId[5];
		//UINT mn_SysItemState[5];

		bool mb_InHidingToTray;
		bool mb_WindowInTray;

	public:
		bool isWindowInTray() { return mb_WindowInTray; }
		bool isHidingToTray() { return mb_InHidingToTray; }
		

		TrayIcon();
		~TrayIcon();

		void HideWindowToTray(LPCTSTR asInfoTip = NULL);
		void RestoreWindowFromTray(BOOL abIconOnly = FALSE);
		void LoadIcon(HWND inWnd, int inIconResource);
		//void Delete();
		void UpdateTitle();
		void SettingsChanged();
		LRESULT OnTryIcon(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
		void AddTrayIcon();
		void RemoveTrayIcon();
};
