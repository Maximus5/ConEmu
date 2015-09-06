
/*
Copyright (c) 2015 Maximus5
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

#ifdef _DEBUG
	#define DebugString(x) //OutputDebugString(x)
	#define DefTermMsg(s) //MessageBox(NULL, s, L"ConEmuHk", MB_SYSTEMMODAL)
#else
	#define DebugString(x) //OutputDebugString(x)
	#define DefTermMsg(s) //MessageBox(NULL, s, L"ConEmuHk", MB_SYSTEMMODAL)
#endif

#include "../common/common.hpp"

#include "GuiAttach.h"
#include "hkWindow.h"
#include "MainThread.h"
#include "hlpConsole.h"

/* **************** */

extern struct HookModeFar gFarMode;

/* **************** */

BOOL GuiSetForeground(HWND hWnd)
{
	BOOL lbRc = FALSE;

	if (ghConEmuWndDC)
	{
		CESERVER_REQ *pIn = (CESERVER_REQ*)malloc(sizeof(*pIn)), *pOut;
		if (pIn)
		{
			ExecutePrepareCmd(pIn, CECMD_SETFOREGROUND, sizeof(CESERVER_REQ_HDR)+sizeof(u64)); //-V119

			DWORD nConEmuPID = ASFW_ANY;
			GetWindowThreadProcessId(ghConEmuWndDC, &nConEmuPID);
			AllowSetForegroundWindow(nConEmuPID);

			pIn->qwData[0] = (u64)hWnd;
			HWND hConWnd = GetRealConsoleWindow();
			pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);

			if (pOut)
			{
				if (pOut->hdr.cbSize == (sizeof(CESERVER_REQ_HDR)+sizeof(DWORD)))
					lbRc = pOut->dwData[0];
				ExecuteFreeResult(pOut);
			}
			free(pIn);
		}
	}

	return lbRc;
}

void GuiFlashWindow(BOOL bSimple, HWND hWnd, BOOL bInvert, DWORD dwFlags, UINT uCount, DWORD dwTimeout)
{
	if (ghConEmuWndDC)
	{
		CESERVER_REQ *pIn = (CESERVER_REQ*)malloc(sizeof(*pIn)), *pOut;
		if (pIn)
		{
			ExecutePrepareCmd(pIn, CECMD_FLASHWINDOW, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_FLASHWINFO)); //-V119
			pIn->Flash.bSimple = bSimple;
			pIn->Flash.hWnd = hWnd;
			pIn->Flash.bInvert = bInvert;
			pIn->Flash.dwFlags = dwFlags;
			pIn->Flash.uCount = uCount;
			pIn->Flash.dwTimeout = dwTimeout;
			HWND hConWnd = GetRealConsoleWindow();
			pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);

			if (pOut) ExecuteFreeResult(pOut);
			free(pIn);
		}
	}
}

/* **************** */

BOOL WINAPI OnTrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect)
{
	//typedef BOOL (WINAPI* OnTrackPopupMenu_t)(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect);
	ORIGINALFASTEX(TrackPopupMenu,NULL);

	#if defined(_DEBUG)
	wchar_t szMsg[128]; msprintf(szMsg, countof(szMsg), L"TrackPopupMenu(hwnd=0x%08X)\n", (DWORD)hWnd);
	DebugString(szMsg);
	#endif

	// Seems like that is required(?) for Far's EMenu only
	// and can harm other applications: gh#112
	if (ghConEmuWndDC && (gFarMode.cbSize == sizeof(gFarMode)) && gFarMode.bFarHookMode)
	{
		// We have to ensure that hWnd is on top (has focus) because menu expects that
		GuiSetForeground(hWnd);

		// Far Manager related (EMenu especially)
		if (gFarMode.cbSize == sizeof(gFarMode) && gFarMode.bPopupMenuPos)
		{
			gFarMode.bPopupMenuPos = FALSE; // one time
			POINT pt; GetCursorPos(&pt);
			x = pt.x; y = pt.y;
		}
	}

	BOOL lbRc = FALSE;
	if (F(TrackPopupMenu) != NULL)
		lbRc = F(TrackPopupMenu)(hMenu, uFlags, x, y, nReserved, hWnd, prcRect);
	return lbRc;
}


BOOL WINAPI OnTrackPopupMenuEx(HMENU hmenu, UINT fuFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm)
{
	//typedef BOOL (WINAPI* OnTrackPopupMenuEx_t)(HMENU hmenu, UINT fuFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm);
	ORIGINALFASTEX(TrackPopupMenuEx,NULL);

	#if defined(_DEBUG)
	wchar_t szMsg[128]; msprintf(szMsg, countof(szMsg), L"TrackPopupMenuEx(hwnd=0x%08X)\n", (DWORD)hWnd);
	DebugString(szMsg);
	#endif

	if (ghConEmuWndDC)
	{
		// Необходимо "поднять" наверх консольное окно, т.к. от него идет меню
		GuiSetForeground(hWnd);

		if (gFarMode.cbSize == sizeof(gFarMode) && gFarMode.bPopupMenuPos)
		{
			gFarMode.bPopupMenuPos = FALSE; // однократно
			POINT pt; GetCursorPos(&pt);
			x = pt.x; y = pt.y;
		}
	}

	BOOL lbRc = FALSE;
	if (F(TrackPopupMenuEx) != NULL)
		lbRc = F(TrackPopupMenuEx)(hmenu, fuFlags, x, y, hWnd, lptpm);
	return lbRc;
}


BOOL WINAPI OnFlashWindow(HWND hWnd, BOOL bInvert)
{
	//typedef BOOL (WINAPI* OnFlashWindow_t)(HWND hWnd, BOOL bInvert);
	ORIGINALFASTEX(FlashWindow,NULL);

	if (ghConEmuWndDC)
	{
		GuiFlashWindow(TRUE, hWnd, bInvert, 0,0,0);
		return TRUE;
	}

	BOOL lbRc = NULL;
	if (F(FlashWindow) != NULL)
		lbRc = F(FlashWindow)(hWnd, bInvert);
	return lbRc;
}


BOOL WINAPI OnFlashWindowEx(PFLASHWINFO pfwi)
{
	//typedef BOOL (WINAPI* OnFlashWindowEx_t)(PFLASHWINFO pfwi);
	ORIGINALFASTEX(FlashWindowEx,NULL);

	if (ghConEmuWndDC)
	{
		GuiFlashWindow(FALSE, pfwi->hwnd, 0, pfwi->dwFlags, pfwi->uCount, pfwi->dwTimeout);
		return TRUE;
	}

	BOOL lbRc = FALSE;
	if (F(FlashWindowEx) != NULL)
		lbRc = F(FlashWindowEx)(pfwi);
	return lbRc;
}


BOOL WINAPI OnGetWindowRect(HWND hWnd, LPRECT lpRect)
{
	//typedef BOOL (WINAPI* OnGetWindowRect_t)(HWND hWnd, LPRECT lpRect);
	ORIGINALFASTEX(GetWindowRect,NULL);
	BOOL lbRc = FALSE;

	if ((hWnd == ghConWnd) && ghConEmuWndDC)
	{
		//EMenu gui mode issues (center in window). "Remove" Far window from mouse cursor.
		hWnd = ghConEmuWndDC;
	}

	if (F(GetWindowRect) != NULL)
		lbRc = F(GetWindowRect)(hWnd, lpRect);
	//if (ghConEmuWndDC && lpRect)
	//{
	//	//EMenu text mode issues. "Remove" Far window from mouse cursor.
	//	POINT ptCur = {0};
	//	GetCursorPos(&ptCur);
	//	lpRect->left += ptCur.x;
	//	lpRect->right += ptCur.x;
	//	lpRect->top += ptCur.y;
	//	lpRect->bottom += ptCur.y;
	//}

	_ASSERTRESULT(lbRc);
	return lbRc;
}


BOOL WINAPI OnScreenToClient(HWND hWnd, LPPOINT lpPoint)
{
	//typedef BOOL (WINAPI* OnScreenToClient_t)(HWND hWnd, LPPOINT lpPoint);
	ORIGINALFASTEX(ScreenToClient,NULL);
	BOOL lbRc = FALSE;

	if (F(ScreenToClient) != NULL)
		lbRc = F(ScreenToClient)(hWnd, lpPoint);

	if (lbRc && ghConEmuWndDC && lpPoint && (hWnd == ghConWnd))
	{
		//EMenu text mode issues. "Remove" Far window from mouse cursor.
		lpPoint->x = lpPoint->y = -1;
	}

	return lbRc;
}


BOOL WINAPI OnShowCursor(BOOL bShow)
{
	//typedef BOOL (WINAPI* OnShowCursor_t)(BOOL bShow);
	ORIGINALFASTEX(ShowCursor,NULL);
	BOOL bRc = FALSE;

	if (gbIsMinTtyProcess)
	{
		if (!bShow)
		{
			// Issue 888: ConEmu-Mintty: Mouse Cursor is hidden after application switch
			// _ASSERTEX(bShow!=FALSE);
			bShow = TRUE; // Disallow cursor hiding
		}
	}

	if (F(ShowCursor))
	{
		bRc = F(ShowCursor)(bShow);
	}

	return bRc;
}


HWND WINAPI OnSetFocus(HWND hWnd)
{
	//typedef HWND (WINAPI* OnSetFocus_t)(HWND hWnd);
	ORIGINALFASTEX(SetFocus,NULL);
	HWND hRet = NULL;
	DWORD nPID = 0, nTID = 0;

	if (ghAttachGuiClient && ((nTID = GetWindowThreadProcessId(hWnd, &nPID)) != 0))
	{
		ghAttachGuiFocused = (nPID == GetCurrentProcessId()) ? hWnd : NULL;
	}

	if (F(SetFocus))
		hRet = F(SetFocus)(hWnd);

	return hRet;
}


BOOL WINAPI OnShowWindow(HWND hWnd, int nCmdShow)
{
	//typedef BOOL (WINAPI* OnShowWindow_t)(HWND hWnd, int nCmdShow);
	ORIGINALFASTEX(ShowWindow,NULL);
	BOOL lbRc = FALSE, lbGuiAttach = FALSE, lbInactiveTab = FALSE;
	static bool bShowWndCalled = false;

	if (gbPrepareDefaultTerminal && ghConWnd && (hWnd == ghConWnd) && nCmdShow && (gnVsHostStartConsole > 0))
	{
		DefTermMsg(L"ShowWindow(hConWnd) calling");
		nCmdShow = SW_HIDE;
		gnVsHostStartConsole--;
	}

	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConWnd))
	{
		#ifdef _DEBUG
		if (hWnd == ghConEmuWndDC)
		{
			static bool bShowWarned = false;
			if (!bShowWarned) { bShowWarned = true; _ASSERTE(hWnd != ghConEmuWndDC && L"OnShowWindow(ghConEmuWndDC)"); }
		}
		else
		{
			static bool bShowWarned = false;
			if (!bShowWarned) { bShowWarned = true; _ASSERTE(hWnd != ghConEmuWndDC && L"OnShowWindow(ghConWnd)"); }
		}
		#endif
		return TRUE; // обманем
	}

	if ((ghAttachGuiClient == hWnd) || (!ghAttachGuiClient && gbAttachGuiClient))
		OnShowGuiClientWindow(hWnd, nCmdShow, lbGuiAttach, lbInactiveTab);

	if (F(ShowWindow))
	{
		lbRc = F(ShowWindow)(hWnd, nCmdShow);
		// Первый вызов может быть обломным, из-за того, что корневой процесс
		// запускается с wShowCmd=SW_HIDE (чтобы не мелькал)
		if (!bShowWndCalled)
		{
			bShowWndCalled = true;
			if (!lbRc && nCmdShow && !IsWindowVisible(hWnd))
			{
				F(ShowWindow)(hWnd, nCmdShow);
			}
		}

		// Если вкладка НЕ активная - то вернуть фокус в ConEmu
		if (lbGuiAttach && lbInactiveTab && nCmdShow && ghConEmuWnd)
		{
			SetForegroundWindow(ghConEmuWnd);
		}
	}
	DWORD dwErr = GetLastError();

	if (lbGuiAttach)
		OnPostShowGuiClientWindow(hWnd, nCmdShow);

	SetLastError(dwErr);
	return lbRc;
}


HWND WINAPI OnSetParent(HWND hWndChild, HWND hWndNewParent)
{
	//typedef HWND (WINAPI* OnSetParent_t)(HWND hWndChild, HWND hWndNewParent);
	ORIGINALFASTEX(SetParent,NULL);
	HWND lhRc = NULL;

	if (ghConEmuWndDC && hWndChild == ghConEmuWndDC)
	{
		_ASSERTE(hWndChild != ghConEmuWndDC);
		return NULL; // обманем
	}

	if (hWndNewParent && ghConEmuWndDC)
	{
		_ASSERTE(hWndNewParent!=ghConEmuWnd);
		//hWndNewParent = ghConEmuWndDC;
	}

	if (F(SetParent))
	{
		lhRc = F(SetParent)(hWndChild, hWndNewParent);
	}

	return lhRc;
}


HWND WINAPI OnGetParent(HWND hWnd)
{
	//typedef HWND (WINAPI* OnGetParent_t)(HWND hWnd);
	ORIGINALFASTEX(GetParent,NULL);
	HWND lhRc = NULL;

	//if (ghConEmuWndDC && hWnd == ghConEmuWndDC)
	//{
	//	hWnd = ghConEmuWnd;
	//}
	if (ghConEmuWndDC)
	{
		if (ghAttachGuiClient)
		{
			if (hWnd == ghAttachGuiClient || hWnd == ghConEmuWndDC)
			{
				// Обмануть GUI-клиента, пусть он думает, что он "сверху"
				hWnd = ghConEmuWnd;
			}
		}
		else if (hWnd == ghConEmuWndDC)
		{
			hWnd = ghConEmuWnd;
		}
	}

	if (F(GetParent))
	{
		lhRc = F(GetParent)(hWnd);
	}

	return lhRc;
}


HWND WINAPI OnGetWindow(HWND hWnd, UINT uCmd)
{
	//typedef HWND (WINAPI* OnGetWindow_t)(HWND hWnd, UINT uCmd);
	ORIGINALFASTEX(GetWindow,NULL);
	HWND lhRc = NULL;

	if (ghConEmuWndDC)
	{
		if (ghAttachGuiClient)
		{
			if ((hWnd == ghAttachGuiClient || hWnd == ghConEmuWndDC) && (uCmd == GW_OWNER))
			{
				hWnd = ghConEmuWnd;
			}
		}
		else if ((hWnd == ghConEmuWndDC) && (uCmd == GW_OWNER))
		{
			hWnd = ghConEmuWnd;
		}
	}

	if (F(GetWindow))
	{
		lhRc = F(GetWindow)(hWnd, uCmd);

		if (ghAttachGuiClient && (uCmd == GW_OWNER) && (lhRc == ghConEmuWndDC))
		{
			_ASSERTE(lhRc != ghConEmuWndDC);
			lhRc = ghAttachGuiClient;
		}
	}

	_ASSERTRESULT(lhRc!=NULL);
	return lhRc;
}


HWND WINAPI OnGetAncestor(HWND hWnd, UINT gaFlags)
{
	//typedef HWND (WINAPI* OnGetAncestor_t)(HWND hWnd, UINT gaFlags);
	ORIGINALFASTEX(GetAncestor,NULL);
	HWND lhRc = NULL;

	#ifdef LOG_GETANCESTOR
	if (ghAttachGuiClient)
	{
		wchar_t szInfo[1024];
		getWindowInfo(hWnd, szInfo);
		lstrcat(szInfo, L"\n");
		DebugString(szInfo);
	}
	#endif

	//if (ghConEmuWndDC && hWnd == ghConEmuWndDC)
	//{
	//	hWnd = ghConEmuWnd;
	//}
	if (ghConEmuWndDC)
	{
		#ifdef _DEBUG
		if ((GetKeyState(VK_CAPITAL) & 1))
		{
			int nDbg = 0;
		}
		#endif

		if (ghAttachGuiClient)
		{
			// Обмануть GUI-клиента, пусть он думает, что он "сверху"
			if (hWnd == ghAttachGuiClient || hWnd == ghConEmuWndDC)
			{
				hWnd = ghConEmuWnd;
			}
			#if 0
			else
			{
				wchar_t szName[255];
				GetClassName(hWnd, szName, countof(szName));
				if (wcsncmp(szName, L"WindowsForms", 12) == 0)
				{
					GetWindowText(hWnd, szName, countof(szName));
					if (wcsncmp(szName, L"toolStrip", 8) == 0 || wcsncmp(szName, L"menuStrip", 8) == 0)
					{
						hWnd = ghConEmuWndDC;
					}
				}
			}
			#endif
		}
		else if (hWnd == ghConEmuWndDC)
		{
			hWnd = ghConEmuWnd;
		}
	}

	if (F(GetAncestor))
	{
		lhRc = F(GetAncestor)(hWnd, gaFlags);

		if (ghAttachGuiClient && (gaFlags == GA_ROOTOWNER || gaFlags == GA_ROOT) && lhRc == ghConEmuWnd)
		{
			lhRc = ghAttachGuiClient;
		}
	}

	_ASSERTRESULT(lhRc);
	return lhRc;
}


//Issue 469: Some programs requires "ConsoleWindowClass" for GetConsoleWindow
int WINAPI OnGetClassNameA(HWND hWnd, LPSTR lpClassName, int nMaxCount)
{
	//typedef int (WINAPI *OnGetClassNameA_t)(HWND hWnd, LPSTR lpClassName, int nMaxCount);
	ORIGINALFASTEX(GetClassNameA,NULL);
	int iRc = 0;
	if (ghConEmuWndDC && hWnd == ghConEmuWndDC && lpClassName)
	{
		//lstrcpynA(lpClassName, RealConsoleClass, nMaxCount);
		WideCharToMultiByte(CP_ACP, 0, RealConsoleClass, -1, lpClassName, nMaxCount, 0,0);
		iRc = lstrlenA(lpClassName);
	}
	else if (F(GetClassNameA))
	{
		iRc = F(GetClassNameA)(hWnd, lpClassName, nMaxCount);
	}
	return iRc;
}


int WINAPI OnGetClassNameW(HWND hWnd, LPWSTR lpClassName, int nMaxCount)
{
	//typedef int (WINAPI *OnGetClassNameW_t)(HWND hWnd, LPWSTR lpClassName, int nMaxCount);
	ORIGINALFASTEX(GetClassNameW,NULL);
	int iRc = 0;
	if (ghConEmuWndDC && hWnd == ghConEmuWndDC && lpClassName)
	{
		lstrcpynW(lpClassName, RealConsoleClass, nMaxCount);
		iRc = lstrlenW(lpClassName);
	}
	else if (F(GetClassNameW))
	{
		iRc = F(GetClassNameW)(hWnd, lpClassName, nMaxCount);
	}
	return iRc;
}


HWND WINAPI OnGetActiveWindow()
{
	//typedef HWND (WINAPI* OnGetActiveWindow_t)();
	ORIGINALFASTEX(GetActiveWindow,NULL);
	HWND hWnd = NULL;

	if (F(GetActiveWindow))
		hWnd = F(GetActiveWindow)();

	#if 1
	if (ghAttachGuiClient)
	{
		if (hWnd == ghConEmuWnd || hWnd == ghConEmuWndDC)
			hWnd = ghAttachGuiClient;
		else if (hWnd == NULL)
			hWnd = ghAttachGuiClient;
	}
	#endif

	return hWnd;
}


HWND WINAPI OnGetForegroundWindow()
{
	//typedef HWND (WINAPI* OnGetForegroundWindow_t)();
	ORIGINALFASTEX(GetForegroundWindow,NULL);

	HWND hFore = NULL;
	if (F(GetForegroundWindow))
		hFore = F(GetForegroundWindow)();

	if (ghConEmuWndDC)
	{
		if (ghAttachGuiClient && ((hFore == ghConEmuWnd) || (hFore == ghConEmuWndDC)))
		{
			// Обмануть GUI-клиента, пусть он думает, что он "сверху"
			hFore = ghAttachGuiClient;
		}
		else if (hFore == ghConEmuWnd)
		{
			hFore = ghConEmuWndDC;
		}
	}

	return hFore;
}


BOOL WINAPI OnSetForegroundWindow(HWND hWnd)
{
	//typedef BOOL (WINAPI* OnSetForegroundWindow_t)(HWND hWnd);
	ORIGINALFASTEX(SetForegroundWindow,NULL);

	BOOL lbRc = FALSE;

	if (ghConEmuWndDC)
	{
		if (hWnd == ghConEmuWndDC)
			hWnd = ghConEmuWnd;
		lbRc = GuiSetForeground(hWnd);
	}

	// ConEmu наверное уже все сделал, но на всякий случай, дернем и здесь
	if (F(SetForegroundWindow) != NULL)
	{
		lbRc = F(SetForegroundWindow)(hWnd);

		//if (gbShowOnSetForeground && lbRc)
		//{
		//	if (IsWindow(hWnd) && !IsWindowVisible(hWnd))
		//		ShowWindow(hWnd, SW_SHOW);
		//}
	}

	return lbRc;
}


BOOL WINAPI OnSetMenu(HWND hWnd, HMENU hMenu)
{
	//typedef BOOL (WINAPI* OnSetMenu_t)(HWND hWnd, HMENU hMenu);
	ORIGINALFASTEX(SetMenu,NULL);

	BOOL lbRc = FALSE;

	if (hMenu && ghAttachGuiClient && hWnd == ghAttachGuiClient)
	{
		if ((gnAttachGuiClientFlags & (agaf_WS_CHILD|agaf_NoMenu|agaf_DotNet)) == (agaf_WS_CHILD|agaf_NoMenu))
		{
			gnAttachGuiClientFlags &= ~(agaf_WS_CHILD|agaf_NoMenu);
			DWORD_PTR dwStyle = GetWindowLongPtr(ghAttachGuiClient, GWL_STYLE);
			DWORD_PTR dwNewStyle = (dwStyle & ~WS_CHILD) | (gnAttachGuiClientStyle & WS_POPUP);
			if (dwStyle != dwNewStyle)
			{
				SetWindowLongPtr(ghAttachGuiClient, GWL_STYLE, dwNewStyle);
			}
		}
	}

	if (F(SetMenu) != NULL)
	{
		lbRc = F(SetMenu)(hWnd, hMenu);
	}

	return lbRc;
}


BOOL WINAPI OnMoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint)
{
	//typedef BOOL (WINAPI* OnMoveWindow_t)(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
	ORIGINALFASTEX(MoveWindow,NULL);
	BOOL lbRc = FALSE;

	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConEmuWnd))
		return TRUE; // обманем. приложениям запрещено "двигать" ConEmuDC

	if (ghConEmuWndDC && ghAttachGuiClient && hWnd == ghAttachGuiClient)
	{
		// GUI приложениями запрещено самостоятельно двигаться внутри ConEmu
		OnSetGuiClientWindowPos(hWnd, NULL, X, Y, nWidth, nHeight, 0);
	}

	if (F(MoveWindow))
		lbRc = F(MoveWindow)(hWnd, X, Y, nWidth, nHeight, bRepaint);

	return lbRc;
}


LONG WINAPI OnSetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong)
{
	//typedef LONG (WINAPI* OnSetWindowLongA_t)(HWND hWnd, int nIndex, LONG dwNewLong);
	ORIGINALFASTEX(SetWindowLongA,NULL);
	LONG lRc = 0;

	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConEmuWnd))
	{
		_ASSERTRESULT(FALSE);
		SetLastError(ERROR_INVALID_HANDLE);
		lRc = 0; // обманем. приложениям запрещено менять ConEmuDC
	}
	else if (F(SetWindowLongA))
	{
		lRc = F(SetWindowLongA)(hWnd, nIndex, dwNewLong);
	}

	return lRc;
}


LONG WINAPI OnSetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong)
{
	//typedef LONG (WINAPI* OnSetWindowLongW_t)(HWND hWnd, int nIndex, LONG dwNewLong);
	ORIGINALFASTEX(SetWindowLongW,NULL);
	LONG lRc = 0;

	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConEmuWnd))
	{
		_ASSERTRESULT(FALSE);
		SetLastError(ERROR_INVALID_HANDLE);
		lRc = 0; // обманем. приложениям запрещено менять ConEmuDC
	}
	else if (F(SetWindowLongW))
	{
		lRc = F(SetWindowLongW)(hWnd, nIndex, dwNewLong);
	}

	return lRc;
}


#ifdef WIN64
LONG_PTR WINAPI OnSetWindowLongPtrA(HWND hWnd, int nIndex, LONG_PTR dwNewLong)
{
	//typedef LONG_PTR (WINAPI* OnSetWindowLongPtrA_t)(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
	ORIGINALFASTEX(SetWindowLongPtrA,NULL);
	LONG_PTR lRc = 0;

	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConEmuWnd))
	{
		_ASSERTRESULT(FALSE);
		SetLastError(ERROR_INVALID_HANDLE);
		lRc = 0; // обманем. приложениям запрещено менять ConEmuDC
	}
	else if (F(SetWindowLongPtrA))
	{
		lRc = F(SetWindowLongPtrA)(hWnd, nIndex, dwNewLong);
	}

	return lRc;
}
#endif


#ifdef WIN64
LONG_PTR WINAPI OnSetWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong)
{
	//typedef LONG_PTR (WINAPI* OnSetWindowLongPtrW_t)(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
	ORIGINALFASTEX(SetWindowLongPtrW,NULL);
	LONG_PTR lRc = 0;

	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConEmuWnd))
	{
		_ASSERTRESULT(FALSE);
		SetLastError(ERROR_INVALID_HANDLE);
		lRc = 0; // обманем. приложениям запрещено менять ConEmuDC
	}
	else if (F(SetWindowLongPtrW))
	{
		lRc = F(SetWindowLongPtrW)(hWnd, nIndex, dwNewLong);
	}

	return lRc;
}
#endif


int WINAPI OnGetWindowTextLengthA(HWND hWnd)
{
	//typedef int (WINAPI* OnGetWindowTextLengthA_t)(HWND hWnd);
	ORIGINALFASTEX(GetWindowTextLengthA,NULL);
	int iRc = 0;

	FixHwnd4ConText(hWnd);

	if (F(GetWindowTextLengthA))
		iRc = F(GetWindowTextLengthA)(hWnd);

	return iRc;
}


int WINAPI OnGetWindowTextLengthW(HWND hWnd)
{
	//typedef int (WINAPI* OnGetWindowTextLengthW_t)(HWND hWnd);
	ORIGINALFASTEX(GetWindowTextLengthW,NULL);
	int iRc = 0;

	FixHwnd4ConText(hWnd);

	if (F(GetWindowTextLengthW))
		iRc = F(GetWindowTextLengthW)(hWnd);

	return iRc;
}


int WINAPI OnGetWindowTextA(HWND hWnd, LPSTR lpString, int nMaxCount)
{
	//typedef int (WINAPI* OnGetWindowTextA_t)(HWND hWnd, LPSTR lpString, int nMaxCount);
	ORIGINALFASTEX(GetWindowTextA,NULL);
	int iRc = 0;

	FixHwnd4ConText(hWnd);

	if (F(GetWindowTextA))
		iRc = F(GetWindowTextA)(hWnd, lpString, nMaxCount);

	return iRc;
}


int WINAPI OnGetWindowTextW(HWND hWnd, LPWSTR lpString, int nMaxCount)
{
	//typedef int (WINAPI* OnGetWindowTextW_t)(HWND hWnd, LPWSTR lpString, int nMaxCount);
	ORIGINALFASTEX(GetWindowTextW,NULL);
	int iRc = 0;

	FixHwnd4ConText(hWnd);

	if (F(GetWindowTextW))
		iRc = F(GetWindowTextW)(hWnd, lpString, nMaxCount);

	#ifdef DEBUG_CON_TITLE
	wchar_t szPrefix[32]; _wsprintf(szPrefix, SKIPCOUNT(szPrefix) L"GetWindowTextW(x%08X)='", (DWORD)(DWORD_PTR)hWnd);
	CEStr lsDbg(lstrmerge(szPrefix, lpString, L"'\n"));
	OutputDebugString(lsDbg);
	if (gFarMode.cbSize && lpString && gpLastSetConTitle && gpLastSetConTitle->ms_Arg)
	{
		int iCmp = lstrcmp(gpLastSetConTitle->ms_Arg, lpString);
		_ASSERTE((iCmp == 0) && "Console window title was changed outside or was not applied yet");
	}
	#endif

	return iRc;
}


BOOL WINAPI OnSetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
	//typedef BOOL (WINAPI* OnSetWindowPos_t)(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
	ORIGINALFASTEX(SetWindowPos,NULL);
	BOOL lbRc = FALSE;

	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConEmuWnd))
	{
		_ASSERTRESULT(FALSE);
		return TRUE; // обманем. приложениям запрещено "двигать" ConEmuDC
	}

	if (ghConEmuWndDC && ghAttachGuiClient && hWnd == ghAttachGuiClient)
	{
		// GUI приложениями запрещено самостоятельно двигаться внутри ConEmu
		OnSetGuiClientWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
	}

	if (F(SetWindowPos))
		lbRc = F(SetWindowPos)(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);

	_ASSERTRESULT(lbRc);
	return lbRc;
}


BOOL WINAPI OnGetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl)
{
	//typedef BOOL (WINAPI* OnGetWindowPlacement_t)(HWND hWnd, WINDOWPLACEMENT *lpwndpl);
	ORIGINALFASTEX(GetWindowPlacement,NULL);
	BOOL lbRc = FALSE;

	if (F(GetWindowPlacement))
		lbRc = F(GetWindowPlacement)(hWnd, lpwndpl);

	if (lbRc && ghConEmuWndDC && !gbGuiClientExternMode && ghAttachGuiClient
		&& hWnd == ghAttachGuiClient)
	{
		MapWindowPoints(ghConEmuWndDC, NULL, (LPPOINT)&lpwndpl->rcNormalPosition, 2);
	}

	return lbRc;
}


BOOL WINAPI OnSetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl)
{
	//typedef BOOL (WINAPI* OnSetWindowPlacement_t)(HWND hWnd, WINDOWPLACEMENT *lpwndpl);
	ORIGINALFASTEX(SetWindowPlacement,NULL);
	BOOL lbRc = FALSE;
	WINDOWPLACEMENT wpl = {sizeof(wpl)};

	if (ghConEmuWndDC && (hWnd == ghConEmuWndDC || hWnd == ghConEmuWnd))
		return TRUE; // обманем. приложениям запрещено "двигать" ConEmuDC

	if (lpwndpl && ghConEmuWndDC && ghAttachGuiClient && !gbGuiClientExternMode && hWnd == ghAttachGuiClient)
	{
		// GUI приложениями запрещено самостоятельно двигаться внутри ConEmu
		int X, Y, cx, cy;
		if (OnSetGuiClientWindowPos(hWnd, NULL, X, Y, cx, cy, 0))
		{
			wpl.showCmd = SW_RESTORE;
			wpl.rcNormalPosition.left = X;
			wpl.rcNormalPosition.top = Y;
			wpl.rcNormalPosition.right = X + cx;
			wpl.rcNormalPosition.bottom = Y + cy;
			wpl.ptMinPosition = lpwndpl->ptMinPosition;
			wpl.ptMaxPosition = lpwndpl->ptMaxPosition;
			lpwndpl = &wpl;
		}
	}

	if (F(SetWindowPlacement))
		lbRc = F(SetWindowPlacement)(hWnd, lpwndpl);

	return lbRc;
}


HWND WINAPI OnCreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	//typedef HWND (WINAPI* OnCreateWindowExA_t)(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
	ORIGINALFASTEX(CreateWindowExA,NULL);
	HWND hWnd = NULL;
	BOOL bAttachGui = FALSE, bStyleHidden = FALSE;
	DWORD lStyle = dwStyle, lStyleEx = dwExStyle;

	if (CheckCanCreateWindow(lpClassName, NULL, dwStyle, dwExStyle, hWndParent, bAttachGui, bStyleHidden)
		&& F(CreateWindowExA) != NULL)
	{
		if (bAttachGui)
		{
			if ((grcConEmuClient.right > grcConEmuClient.left) && (grcConEmuClient.bottom > grcConEmuClient.top))
			{
				x = grcConEmuClient.left; y = grcConEmuClient.top;
				nWidth = grcConEmuClient.right - grcConEmuClient.left; nHeight = grcConEmuClient.bottom - grcConEmuClient.top;
			}
			else
			{
				_ASSERTEX((grcConEmuClient.right > grcConEmuClient.left) && (grcConEmuClient.bottom > grcConEmuClient.top));
			}
		}

		hWnd = F(CreateWindowExA)(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
		DWORD dwErr = GetLastError();

		if (hWnd && bAttachGui)
		{
			OnGuiWindowAttached(hWnd, hMenu, lpClassName, NULL, lStyle, lStyleEx, bStyleHidden);

			SetLastError(dwErr);
		}
	}

	return hWnd;
}


HWND WINAPI OnCreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	//typedef HWND (WINAPI* OnCreateWindowExW_t)(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
	ORIGINALFASTEX(CreateWindowExW,NULL);
	HWND hWnd = NULL;
	BOOL bAttachGui = FALSE, bStyleHidden = FALSE;
	DWORD lStyle = dwStyle, lStyleEx = dwExStyle;

	if (CheckCanCreateWindow(NULL, lpClassName, dwStyle, dwExStyle, hWndParent, bAttachGui, bStyleHidden)
		&& F(CreateWindowExW) != NULL)
	{
		if (bAttachGui)
		{
			x = grcConEmuClient.left; y = grcConEmuClient.top;
			nWidth = grcConEmuClient.right - grcConEmuClient.left; nHeight = grcConEmuClient.bottom - grcConEmuClient.top;
		}

		hWnd = F(CreateWindowExW)(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
		DWORD dwErr = GetLastError();

		if (hWnd && bAttachGui)
		{
			OnGuiWindowAttached(hWnd, hMenu, NULL, lpClassName, lStyle, lStyleEx, bStyleHidden);

			SetLastError(dwErr);
		}
	}

	_ASSERTRESULT(hWnd!=NULL);
	return hWnd;
}
