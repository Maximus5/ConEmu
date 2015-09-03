
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

#pragma once

#include <windows.h>

/* *************************** */

BOOL GuiSetForeground(HWND hWnd);
void GuiFlashWindow(BOOL bSimple, HWND hWnd, BOOL bInvert, DWORD dwFlags, UINT uCount, DWORD dwTimeout);

/* *************************** */

// TrackPopupMenu
typedef BOOL (WINAPI* OnTrackPopupMenu_t)(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect);
BOOL WINAPI OnTrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect);

// TrackPopupMenuEx
typedef BOOL (WINAPI* OnTrackPopupMenuEx_t)(HMENU hmenu, UINT fuFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm);
BOOL WINAPI OnTrackPopupMenuEx(HMENU hmenu, UINT fuFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm);

// FlashWindow
typedef BOOL (WINAPI* OnFlashWindow_t)(HWND hWnd, BOOL bInvert);
BOOL WINAPI OnFlashWindow(HWND hWnd, BOOL bInvert);

// FlashWindowEx
typedef BOOL (WINAPI* OnFlashWindowEx_t)(PFLASHWINFO pfwi);
BOOL WINAPI OnFlashWindowEx(PFLASHWINFO pfwi);

// GetWindowRect
typedef BOOL (WINAPI* OnGetWindowRect_t)(HWND hWnd, LPRECT lpRect);
BOOL WINAPI OnGetWindowRect(HWND hWnd, LPRECT lpRect);

// ScreenToClient
typedef BOOL (WINAPI* OnScreenToClient_t)(HWND hWnd, LPPOINT lpPoint);
BOOL WINAPI OnScreenToClient(HWND hWnd, LPPOINT lpPoint);

// ShowCursor
typedef BOOL (WINAPI* OnShowCursor_t)(BOOL bShow);
BOOL WINAPI OnShowCursor(BOOL bShow);

// SetFocus
typedef HWND (WINAPI* OnSetFocus_t)(HWND hWnd);
HWND WINAPI OnSetFocus(HWND hWnd);

// ShowWindow
typedef BOOL (WINAPI* OnShowWindow_t)(HWND hWnd, int nCmdShow);
BOOL WINAPI OnShowWindow(HWND hWnd, int nCmdShow);

// SetParent
typedef HWND (WINAPI* OnSetParent_t)(HWND hWndChild, HWND hWndNewParent);
HWND WINAPI OnSetParent(HWND hWndChild, HWND hWndNewParent);

// GetParent
typedef HWND (WINAPI* OnGetParent_t)(HWND hWnd);
HWND WINAPI OnGetParent(HWND hWnd);

// GetWindow
typedef HWND (WINAPI* OnGetWindow_t)(HWND hWnd, UINT uCmd);
HWND WINAPI OnGetWindow(HWND hWnd, UINT uCmd);

// GetAncestor
typedef HWND (WINAPI* OnGetAncestor_t)(HWND hWnd, UINT gaFlags);
HWND WINAPI OnGetAncestor(HWND hWnd, UINT gaFlags);

// GetClassNameA
typedef int (WINAPI *OnGetClassNameA_t)(HWND hWnd, LPSTR lpClassName, int nMaxCount);
int WINAPI OnGetClassNameA(HWND hWnd, LPSTR lpClassName, int nMaxCount);

// GetClassNameW
typedef int (WINAPI *OnGetClassNameW_t)(HWND hWnd, LPWSTR lpClassName, int nMaxCount);
int WINAPI OnGetClassNameW(HWND hWnd, LPWSTR lpClassName, int nMaxCount);

// GetActiveWindow
typedef HWND (WINAPI* OnGetActiveWindow_t)();
HWND WINAPI OnGetActiveWindow();

// GetForegroundWindow
typedef HWND (WINAPI* OnGetForegroundWindow_t)();
HWND WINAPI OnGetForegroundWindow();

// SetForegroundWindow
typedef BOOL (WINAPI* OnSetForegroundWindow_t)(HWND hWnd);
BOOL WINAPI OnSetForegroundWindow(HWND hWnd);

// SetMenu
typedef BOOL (WINAPI* OnSetMenu_t)(HWND hWnd, HMENU hMenu);
BOOL WINAPI OnSetMenu(HWND hWnd, HMENU hMenu);

// MoveWindow
typedef BOOL (WINAPI* OnMoveWindow_t)(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
BOOL WINAPI OnMoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);

// SetWindowLongA
typedef LONG (WINAPI* OnSetWindowLongA_t)(HWND hWnd, int nIndex, LONG dwNewLong);
LONG WINAPI OnSetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong);

// SetWindowLongW
typedef LONG (WINAPI* OnSetWindowLongW_t)(HWND hWnd, int nIndex, LONG dwNewLong);
LONG WINAPI OnSetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong);

#ifdef WIN64
// SetWindowLongPtrA
typedef LONG_PTR (WINAPI* OnSetWindowLongPtrA_t)(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
LONG_PTR WINAPI OnSetWindowLongPtrA(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
#endif

#ifdef WIN64
// SetWindowLongPtrW
typedef LONG_PTR (WINAPI* OnSetWindowLongPtrW_t)(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
LONG_PTR WINAPI OnSetWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
#endif

// GetWindowTextLengthA
typedef int (WINAPI* OnGetWindowTextLengthA_t)(HWND hWnd);
int WINAPI OnGetWindowTextLengthA(HWND hWnd);

// GetWindowTextLengthW
typedef int (WINAPI* OnGetWindowTextLengthW_t)(HWND hWnd);
int WINAPI OnGetWindowTextLengthW(HWND hWnd);

// GetWindowTextA
typedef int (WINAPI* OnGetWindowTextA_t)(HWND hWnd, LPSTR lpString, int nMaxCount);
int WINAPI OnGetWindowTextA(HWND hWnd, LPSTR lpString, int nMaxCount);

// GetWindowTextW
typedef int (WINAPI* OnGetWindowTextW_t)(HWND hWnd, LPWSTR lpString, int nMaxCount);
int WINAPI OnGetWindowTextW(HWND hWnd, LPWSTR lpString, int nMaxCount);

// SetWindowPos
typedef BOOL (WINAPI* OnSetWindowPos_t)(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
BOOL WINAPI OnSetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);

// GetWindowPlacement
typedef BOOL (WINAPI* OnGetWindowPlacement_t)(HWND hWnd, WINDOWPLACEMENT *lpwndpl);
BOOL WINAPI OnGetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl);

// SetWindowPlacement
typedef BOOL (WINAPI* OnSetWindowPlacement_t)(HWND hWnd, WINDOWPLACEMENT *lpwndpl);
BOOL WINAPI OnSetWindowPlacement(HWND hWnd, WINDOWPLACEMENT *lpwndpl);

// CreateWindowExA
typedef HWND (WINAPI* OnCreateWindowExA_t)(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
HWND WINAPI OnCreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);

// CreateWindowExW
typedef HWND (WINAPI* OnCreateWindowExW_t)(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
HWND WINAPI OnCreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
