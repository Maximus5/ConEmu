
/*
Copyright (c) 2009-2011 Maximus5
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

#include "header.h"
#include "TrayIcon.h"
#include "ConEmu.h"
#include "Options.h"

TrayIcon Icon;

TrayIcon::TrayIcon()
{
	memset(&IconData, 0, sizeof(IconData));
	mb_WindowInTray = false;
	mb_InHidingToTray = false;
	//mn_SysItemId[0] = SC_MINIMIZE;
	//mn_SysItemId[1] = SC_MAXIMIZE_SECRET;
	//mn_SysItemId[2] = SC_RESTORE_SECRET;
	//mn_SysItemId[3] = SC_SIZE;
	//mn_SysItemId[4] = SC_MOVE;
	//memset(mn_SysItemState, 0, sizeof(mn_SysItemState));
}

TrayIcon::~TrayIcon()
{
}

void TrayIcon::SettingsChanged()
{
	if (gpSet->isAlwaysShowTrayIcon)
	{
		AddTrayIcon(); // добавит или обновит tooltip
	}
	else
	{
		if (IsWindowVisible(ghWnd))
			RemoveTrayIcon();
	}

	//if (ghWnd)
	//{
	//	DWORD_PTR nStyleEx = GetWindowLongPtr(ghWnd, GWL_EXSTYLE);
	//	DWORD_PTR nNewStyleEx = nStyleEx;
	//	if (gpSet->isAlwaysShowTrayIcon == 2)
	//		nNewStyleEx |= WS_EX_TOOLWINDOW;
	//	else if (nNewStyleEx & WS_EX_TOOLWINDOW)
	//		nNewStyleEx &= ~WS_EX_TOOLWINDOW;
	//	if (nNewStyleEx != nStyleEx)
	//		SetWindowLongPtr(ghWnd, GWL_EXSTYLE, nNewStyleEx);
	//}
}

void TrayIcon::AddTrayIcon()
{
	_ASSERTE(IconData.hIcon!=NULL);

	if (!mb_WindowInTray)
	{
		GetWindowText(ghWnd, IconData.szTip, countof(IconData.szTip));
		Shell_NotifyIcon(NIM_ADD, &IconData);
		mb_WindowInTray = true;
	}
	else
	{
		UpdateTitle();
	}
}

void TrayIcon::RemoveTrayIcon()
{
	if (mb_WindowInTray)
	{
		Shell_NotifyIcon(NIM_DELETE, &IconData);
		mb_WindowInTray = false;
	}
}

void TrayIcon::UpdateTitle()
{
	if (!mb_WindowInTray || !IconData.hIcon)
		return;

	GetWindowText(ghWnd, IconData.szTip, countof(IconData.szTip));
	Shell_NotifyIcon(NIM_MODIFY, &IconData);
}

void TrayIcon::HideWindowToTray(LPCTSTR asInfoTip /* = NULL */)
{
	mb_InHidingToTray = true;
	if (asInfoTip && *asInfoTip)
	{
		IconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_INFO | NIF_TIP;
		lstrcpyn(IconData.szInfoTitle, gpConEmu->ms_ConEmuVer, countof(IconData.szInfoTitle));
		lstrcpyn(IconData.szInfo, asInfoTip, countof(IconData.szInfo));
	}
	else
	{
		IconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		IconData.szInfo[0] = 0;
		IconData.szInfoTitle[0] = 0;
	}
	AddTrayIcon(); // добавит или обновит tooltip
	if (IsWindowVisible(ghWnd))
	{
		apiShowWindow(ghWnd, SW_HIDE);
	}
	HMENU hMenu = GetSystemMenu(ghWnd, false);
	SetMenuItemText(hMenu, ID_TOTRAY, TRAY_ITEM_RESTORE_NAME);
	mb_InHidingToTray = false;
	//for (int i = 0; i < countof(mn_SysItemId); i++)
	//{
	//	MENUITEMINFO mi = {sizeof(mi)};
	//	mi.fMask = MIIM_STATE;
	//	GetMenuItemInfo(hMenu, mn_SysItemId[i], FALSE, &mi);
	//	mn_SysItemState[i] = (mi.fState & (MFS_DISABLED|MFS_GRAYED|MFS_ENABLED));
	//	EnableMenuItem(hMenu, mn_SysItemId[i], MF_BYCOMMAND | MF_GRAYED);
	//}
}

void TrayIcon::RestoreWindowFromTray()
{
	apiShowWindow(ghWnd, SW_SHOW);
	apiSetForegroundWindow(ghWnd);
	//EnableMenuItem(GetSystemMenu(ghWnd, false), ID_TOTRAY, MF_BYCOMMAND | MF_ENABLED);
	HMENU hMenu = GetSystemMenu(ghWnd, false);
	SetMenuItemText(hMenu, ID_TOTRAY, TRAY_ITEM_HIDE_NAME);

	//for (int i = 0; i < countof(mn_SysItemId); i++)
	//{
	//	EnableMenuItem(hMenu, mn_SysItemId[i], MF_BYCOMMAND | mn_SysItemState[i]);
	//}

	if (!gpSet->isAlwaysShowTrayIcon)
		RemoveTrayIcon();
}

void TrayIcon::LoadIcon(HWND inWnd, int inIconResource)
{
	IconData.cbSize = sizeof(NOTIFYICONDATA);
	IconData.uID = 1;
	IconData.hWnd = inWnd;
	IconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_INFO | NIF_TIP;
	IconData.uCallbackMessage = WM_TRAYNOTIFY;
	//lstrcpyn(IconData.szInfoTitle, gpConEmu->ms_ConEmuVer, countof(IconData.szInfoTitle));
	IconData.uTimeout = 10000;
	//!!!ICON
	IconData.hIcon = hClassIconSm;
	//(HICON)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(inIconResource), IMAGE_ICON,
	//    GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
}

//void TrayIcon::Delete()
//{
//	// Не посылать в Shell сообщения, если иконки нет
//	if (mb_WindowInTray)
//	{
//	    Shell_NotifyIcon(NIM_DELETE, &IconData);
//		memset(&IconData, 0, sizeof(IconData));
//	}
//}

LRESULT TrayIcon::OnTryIcon(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch(lParam)
	{
		case WM_LBUTTONUP:
			Icon.RestoreWindowFromTray();
			break;
		case WM_RBUTTONUP:
		{
			POINT mPos;
			GetCursorPos(&mPos);
			apiSetForegroundWindow(hWnd);
			gpConEmu->ShowSysmenu(hWnd, mPos.x, mPos.y);
			PostMessage(hWnd, WM_NULL, 0, 0);
		}
		break;
	}

	return 0;
}

void TrayIcon::SetMenuItemText(HMENU hMenu, UINT nID, LPCWSTR pszText)
{
	wchar_t szText[128];
	MENUITEMINFO mi = {sizeof(MENUITEMINFO)};
	mi.fMask = MIIM_STRING;
	lstrcpyn(szText, pszText, countof(szText));
	mi.dwTypeData = szText;
	SetMenuItemInfo(hMenu, nID, FALSE, &mi);
}
