
/*
Copyright (c) 2009-2010 Maximus5
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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

TrayIcon Icon;

TrayIcon::TrayIcon()
{
    memset(&IconData, 0, sizeof(IconData));
    isWindowInTray = false;
}

TrayIcon::~TrayIcon()
{
}

void TrayIcon::HideWindowToTray()
{
	_ASSERTE(IconData.hIcon!=NULL);
    GetWindowText(ghWnd, IconData.szTip, countof(IconData.szTip));
    Shell_NotifyIcon(NIM_ADD, &IconData);
    ShowWindow(ghWnd, SW_HIDE);
    isWindowInTray = true;
}

void TrayIcon::UpdateTitle()
{
	if (!isWindowInTray || !IconData.hIcon)
		return;
		
    GetWindowText(ghWnd, IconData.szTip, countof(IconData.szTip));
    Shell_NotifyIcon(NIM_MODIFY, &IconData);
}

void TrayIcon::RestoreWindowFromTray()
{
    ShowWindow(ghWnd, SW_SHOW); 
    SetForegroundWindow(ghWnd);
    Shell_NotifyIcon(NIM_DELETE, &IconData);
    EnableMenuItem(GetSystemMenu(ghWnd, false), ID_TOTRAY, MF_BYCOMMAND | MF_ENABLED);
    isWindowInTray = false;
}

void TrayIcon::LoadIcon(HWND inWnd, int inIconResource)
{
    IconData.cbSize = sizeof(NOTIFYICONDATA);
    IconData.uID = 1;
    IconData.hWnd = inWnd;
    IconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    IconData.uCallbackMessage = WM_TRAYNOTIFY;
    //!!!ICON
    IconData.hIcon = hClassIconSm;
    //(HICON)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(inIconResource), IMAGE_ICON, 
    //    GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
}

void TrayIcon::Delete()
{
    Shell_NotifyIcon(NIM_DELETE, &IconData);
    memset(&IconData, 0, sizeof(IconData));
}

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
        SetForegroundWindow(hWnd);
        gConEmu.ShowSysmenu(hWnd, hWnd, mPos.x, mPos.y);
        PostMessage(hWnd, WM_NULL, 0, 0);
        }
        break;
    } 
	return 0;
}
