
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

#define SHOWDEBUGSTR

#define DEBUGSTRICON(x) //DEBUGSTR(x)

#define HIDE_USE_EXCEPTION_INFO
#include "header.h"
#include "TrayIcon.h"
#include "ConEmu.h"
#include "Options.h"
#include "OptionsClass.h"
#include "Menu.h"
#include "PushInfo.h"
#include "../common/WUser.h"

#ifndef NIN_BALLOONUSERCLICK
#define NIN_BALLOONSHOW         (WM_USER + 2)
#define NIN_BALLOONTIMEOUT      (WM_USER + 4)
#define NIN_BALLOONUSERCLICK    (WM_USER + 5)
#endif

#define MY_BALLOON_TICK 2000

TrayIcon Icon;

TrayIcon::TrayIcon()
{
	memset(&IconData, 0, sizeof(IconData));
	mb_WindowInTray = false;
	mb_InHidingToTray = false;
	m_MsgSource = tsa_Source_None;
	mb_SecondTimeoutMsg = false; mn_BalloonShowTick = 0;
	//mn_SysItemId[0] = SC_MINIMIZE;
	//mn_SysItemId[1] = SC_MAXIMIZE_SECRET;
	//mn_SysItemId[2] = SC_RESTORE_SECRET;
	//mn_SysItemId[3] = SC_SIZE;
	//mn_SysItemId[4] = SC_MOVE;
	//memset(mn_SysItemState, 0, sizeof(mn_SysItemState));
	mh_Balloon = NULL;
}

TrayIcon::~TrayIcon()
{
}

void TrayIcon::SettingsChanged()
{
	bool bShowTSA = gpSet->isAlwaysShowTrayIcon();

	if (bShowTSA)
	{
		AddTrayIcon(); // добавит или обновит tooltip
	}
	else
	{
		if (IsWindowVisible(ghWnd))
			RemoveTrayIcon();
	}

	if (gpSetCls->GetPage(thi_Taskbar))
	{
		CheckDlgButton(gpSetCls->GetPage(thi_Taskbar), cbAlwaysShowTrayIcon, bShowTSA);
	}
}

void TrayIcon::AddTrayIcon()
{
	_ASSERTE(IconData.hIcon!=NULL);

	if (!mb_WindowInTray)
	{
		mb_SecondTimeoutMsg = false; mn_BalloonShowTick = 0;
		GetWindowText(ghWnd, IconData.szTip, countof(IconData.szTip));
		Shell_NotifyIcon(NIM_ADD, (NOTIFYICONDATA*)&IconData);
		mb_WindowInTray = true;
	}
	else
	{
		UpdateTitle();
	}
}

void TrayIcon::RemoveTrayIcon(bool bForceRemove /*= false*/)
{
	if (mb_WindowInTray && (bForceRemove || (m_MsgSource == tsa_Source_None)))
	{
		mb_SecondTimeoutMsg = false; mn_BalloonShowTick = 0;
		Shell_NotifyIcon(NIM_DELETE, (NOTIFYICONDATA*)&IconData);
		mb_WindowInTray = false;
		mh_Balloon = NULL;
	}
}

void TrayIcon::UpdateTitle()
{
	// Добавил tsa_Source_None. Если висит Balloon с сообщением,
	// то нет смысла обновлять тултип, он все-равно не виден...
	if (!mb_WindowInTray || !IconData.hIcon || (m_MsgSource != tsa_Source_None))
		return;

	// это тултип, который появляется при наведении курсора мышки
	wchar_t szTitle[128] = {};
	GetWindowText(ghWnd, szTitle, countof(szTitle));
	if (*IconData.szInfo || *IconData.szInfoTitle || lstrcmp(szTitle, IconData.szTip))
	{
		IconData.szInfo[0] = 0;
		IconData.szInfoTitle[0] = 0;
		lstrcpyn(IconData.szTip, szTitle, countof(IconData.szTip));
		Shell_NotifyIcon(NIM_MODIFY, (NOTIFYICONDATA*)&IconData);
	}
}

void TrayIcon::ShowTrayIcon(LPCTSTR asInfoTip /*= NULL*/, TrayIconMsgSource aMsgSource /*= tsa_Source_None*/)
{
	m_MsgSource = aMsgSource;
	if (asInfoTip && *asInfoTip)
	{
		// Сообщение, которое сейчас всплывет
		IconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_INFO | NIF_TIP;
		lstrcpyn(IconData.szInfoTitle, gpConEmu->GetDefaultTitle(), countof(IconData.szInfoTitle));
		lstrcpyn(IconData.szInfo, asInfoTip, countof(IconData.szInfo));
	}
	else
	{
		IconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		IconData.szInfo[0] = 0;
		IconData.szInfoTitle[0] = 0;
	}
	AddTrayIcon(); // добавит или обновит tooltip
}

void TrayIcon::HideWindowToTray(LPCTSTR asInfoTip /* = NULL */)
{
	if (gpConEmu->mp_Inside)
	{
		// No sense to try...
		return;
	}

	if (gpConEmu->InQuakeAnimation())
	{
		_ASSERTE(FALSE && "Calling TrayIcon::HideWindowToTray from QuakeAnimation");
		return;
	}

	gpConEmu->LogString(L"TrayIcon::HideWindowToTray");

	mb_InHidingToTray = true;

	ShowTrayIcon(asInfoTip);

	if (IsWindowVisible(ghWnd))
	{
		gpConEmu->ShowMainWindow(SW_HIDE);
	}
	HMENU hMenu = gpConEmu->mp_Menu->GetSysMenu(/*ghWnd, false*/);
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

void TrayIcon::RestoreWindowFromTray(bool abIconOnly /*= false*/, bool abDontCallShowWindow /*= false*/)
{
	if (!abIconOnly)
	{
		gpConEmu->SetWindowMode(gpConEmu->GetWindowMode());

		if (!abDontCallShowWindow && !IsWindowVisible(ghWnd))
		{
			gpConEmu->ShowMainWindow(SW_SHOW);
		}

		apiSetForegroundWindow(ghWnd);
	}

	if (IsWindowVisible(ghWnd))
	{
		//EnableMenuItem(GetSystemMenu(ghWnd, false), ID_TOTRAY, MF_BYCOMMAND | MF_ENABLED);
		HMENU hMenu = gpConEmu->mp_Menu->GetSysMenu(/*ghWnd, false*/);
		SetMenuItemText(hMenu, ID_TOTRAY, TRAY_ITEM_HIDE_NAME);
	}

	//for (int i = 0; i < countof(mn_SysItemId); i++)
	//{
	//	EnableMenuItem(hMenu, mn_SysItemId[i], MF_BYCOMMAND | mn_SysItemState[i]);
	//}

	if (!gpSet->isAlwaysShowTrayIcon())
		RemoveTrayIcon();
	else
		UpdateTitle();
}

void TrayIcon::LoadIcon(HWND inWnd, int inIconResource)
{
	IconData.cbSize = sizeof(IconData);
	IconData.uID = 1;
	IconData.hWnd = inWnd;
	IconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_INFO | NIF_TIP;
	IconData.uCallbackMessage = WM_TRAYNOTIFY;
	//lstrcpyn(IconData.szInfoTitle, gpConEmu->GetDefaultTitle(), countof(IconData.szInfoTitle));
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
	#ifdef _DEBUG
	wchar_t szMsg[128];
	#endif

	switch (lParam)
	{
		case WM_LBUTTONUP:
		case NIN_BALLOONUSERCLICK:
			#ifdef _DEBUG
			_wsprintf(szMsg, SKIPLEN(countof(szMsg)) (lParam==WM_LBUTTONUP) ? L"TSA: WM_LBUTTONUP(%i,0x%08X)\n" : L"TSA: NIN_BALLOONUSERCLICK(%i,0x%08X)\n", (int)wParam, (DWORD)lParam);
			DEBUGSTRICON(szMsg);
			#endif
			if (gpSet->isQuakeStyle)
			{
				bool bJustActivate = false;
				SingleInstanceShowHideType sih = sih_QuakeShowHide;
				SingleInstanceShowHideType sihHide = gpSet->isMinToTray() ? sih_HideTSA : sih_Minimize;

				if (IsWindowVisible(ghWnd) && !gpConEmu->isIconic())
				{
					if (gpSet->isAlwaysOnTop || (gpSet->isQuakeStyle == 2))
					{
						sih = sihHide;
					}
					else
					{
						UINT nVisiblePart = gpConEmu->IsQuakeVisible();
						if (nVisiblePart >= QUAKEVISIBLELIMIT)
						{
							sih = sihHide;
						}
						else
						{
							// Если поверх ConEmu есть какое-то окно, то ConEmu нужно поднять?
							// Не "выезжать" а просто "вынести наверх", если видимая область достаточно большая
							bJustActivate = (nVisiblePart >= QUAKEVISIBLETRASH) && !gpConEmu->isIconic();
						}
					}
				}

				if (bJustActivate)
				{
					SetForegroundWindow(ghWnd);
				}
				else
				{
					gpConEmu->DoMinimizeRestore(sih);
				}
			}
			else if (gpSet->isAlwaysShowTrayIcon() && IsWindowVisible(ghWnd))
			{
				if (gpSet->isMinToTray())
					Icon.HideWindowToTray();
				else
					SendMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
			}
			else
			{
				Icon.RestoreWindowFromTray();
			}

			switch (m_MsgSource)
			{
			case tsa_Source_Updater:
				m_MsgSource = tsa_Source_None;
				gpConEmu->CheckUpdates(2);
				break;
			case tsa_Push_Notify:
				gpConEmu->mp_PushInfo->OnNotificationClick();
				m_MsgSource = tsa_Source_None;
				break;
			}
			break;
		case NIN_BALLOONSHOW:
			#ifdef _DEBUG
			_wsprintf(szMsg, SKIPLEN(countof(szMsg)) L"TSA: NIN_BALLOONSHOW(%i,0x%08X)\n", (int)wParam, (DWORD)lParam);
			DEBUGSTRICON(szMsg);
			#endif
			mn_BalloonShowTick = GetTickCount();
			break;
		case NIN_BALLOONTIMEOUT:
			{
				#ifdef _DEBUG
				_wsprintf(szMsg, SKIPLEN(countof(szMsg)) L"TSA: NIN_BALLOONTIMEOUT(%i,0x%08X)\n", (int)wParam, (DWORD)lParam);
				DEBUGSTRICON(szMsg);
				#endif

				if (mb_SecondTimeoutMsg
					|| (mn_BalloonShowTick && ((GetTickCount() - mn_BalloonShowTick) > MY_BALLOON_TICK)))
				{
					m_MsgSource = tsa_Source_None;
					Icon.RestoreWindowFromTray(TRUE);
				}
				else if (!mb_SecondTimeoutMsg && (mn_BalloonShowTick && ((GetTickCount() - mn_BalloonShowTick) > MY_BALLOON_TICK)))
				{
					mb_SecondTimeoutMsg = true;
				}
			}
			break;
		case WM_RBUTTONUP:
		{
			#ifdef _DEBUG
			_wsprintf(szMsg, SKIPLEN(countof(szMsg)) L"TSA: WM_RBUTTONUP(%i,0x%08X)\n", (int)wParam, (DWORD)lParam);
			DEBUGSTRICON(szMsg);
			#endif
			POINT mPos;
			GetCursorPos(&mPos);
			gpConEmu->SetIgnoreQuakeActivation(true);
			apiSetForegroundWindow(ghWnd);
			LogString(L"ShowSysmenu called from (TSA)");
			gpConEmu->mp_Menu->ShowSysmenu(mPos.x, mPos.y);
			gpConEmu->SetIgnoreQuakeActivation(false);
			PostMessage(hWnd, WM_NULL, 0, 0);
		}
		break;

	#ifdef _DEBUG
	default:
		_wsprintf(szMsg, SKIPLEN(countof(szMsg)) L"TSA: OnTryIcon(uMsg, wParam=%i, lParam=0x%04X)\n", messg, (int)wParam, (DWORD)lParam);
		DEBUGSTRICON(szMsg);
	#endif
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

void TrayIcon::OnTaskbarCreated()
{
	if (mb_WindowInTray)
	{
		mb_WindowInTray = false;
		AddTrayIcon();
	}
}
