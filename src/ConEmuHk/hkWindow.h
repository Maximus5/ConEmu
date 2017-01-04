
/*
Copyright (c) 2015-2017 Maximus5
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
#include "SetHook.h"

/* *************************** */

BOOL GuiSetForeground(HWND hWnd);
void GuiFlashWindow(CEFlashType fType, HWND hWnd, BOOL bInvert, DWORD dwFlags, UINT uCount, DWORD dwTimeout);

/* *************************** */

HOOK_PROTOTYPE(CreateWindowExA,HWND,WINAPI,(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam));
HOOK_PROTOTYPE(CreateWindowExW,HWND,WINAPI,(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam));
HOOK_PROTOTYPE(FlashWindow,BOOL,WINAPI,(HWND hWnd, BOOL bInvert));
HOOK_PROTOTYPE(FlashWindowEx,BOOL,WINAPI,(PFLASHWINFO pfwi));
HOOK_PROTOTYPE(GetActiveWindow,HWND,WINAPI,());
HOOK_PROTOTYPE(GetAncestor,HWND,WINAPI,(HWND hWnd, UINT gaFlags));
HOOK_PROTOTYPE(GetClassNameA,int,WINAPI,(HWND hWnd, LPSTR lpClassName, int nMaxCount));
HOOK_PROTOTYPE(GetClassNameW,int,WINAPI,(HWND hWnd, LPWSTR lpClassName, int nMaxCount));
HOOK_PROTOTYPE(GetForegroundWindow,HWND,WINAPI,());
HOOK_PROTOTYPE(GetParent,HWND,WINAPI,(HWND hWnd));
HOOK_PROTOTYPE(GetWindow,HWND,WINAPI,(HWND hWnd, UINT uCmd));
HOOK_PROTOTYPE(GetWindowPlacement,BOOL,WINAPI,(HWND hWnd, WINDOWPLACEMENT *lpwndpl));
HOOK_PROTOTYPE(GetWindowRect,BOOL,WINAPI,(HWND hWnd, LPRECT lpRect));
HOOK_PROTOTYPE(GetWindowTextA,int,WINAPI,(HWND hWnd, LPSTR lpString, int nMaxCount));
HOOK_PROTOTYPE(GetWindowTextW,int,WINAPI,(HWND hWnd, LPWSTR lpString, int nMaxCount));
HOOK_PROTOTYPE(GetWindowTextLengthA,int,WINAPI,(HWND hWnd));
HOOK_PROTOTYPE(GetWindowTextLengthW,int,WINAPI,(HWND hWnd));
HOOK_PROTOTYPE(MoveWindow,BOOL,WINAPI,(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint));
HOOK_PROTOTYPE(ScreenToClient,BOOL,WINAPI,(HWND hWnd, LPPOINT lpPoint));
HOOK_PROTOTYPE(SetFocus,HWND,WINAPI,(HWND hWnd));
HOOK_PROTOTYPE(SetForegroundWindow,BOOL,WINAPI,(HWND hWnd));
HOOK_PROTOTYPE(SetMenu,BOOL,WINAPI,(HWND hWnd, HMENU hMenu));
HOOK_PROTOTYPE(SetParent,HWND,WINAPI,(HWND hWndChild, HWND hWndNewParent));
HOOK_PROTOTYPE(SetWindowPlacement,BOOL,WINAPI,(HWND hWnd, WINDOWPLACEMENT *lpwndpl));
HOOK_PROTOTYPE(SetWindowPos,BOOL,WINAPI,(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags));
HOOK_PROTOTYPE(ShowCursor,BOOL,WINAPI,(BOOL bShow));
HOOK_PROTOTYPE(ShowWindow,BOOL,WINAPI,(HWND hWnd, int nCmdShow));
HOOK_PROTOTYPE(TrackPopupMenu,BOOL,WINAPI,(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, CONST RECT * prcRect));
HOOK_PROTOTYPE(TrackPopupMenuEx,BOOL,WINAPI,(HMENU hmenu, UINT fuFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm));

HOOK_PROTOTYPE(SetWindowLongA,LONG,WINAPI,(HWND hWnd, int nIndex, LONG dwNewLong));
HOOK_PROTOTYPE(SetWindowLongW,LONG,WINAPI,(HWND hWnd, int nIndex, LONG dwNewLong));

#ifdef WIN64
HOOK_PROTOTYPE(SetWindowLongPtrA,LONG_PTR,WINAPI,(HWND hWnd, int nIndex, LONG_PTR dwNewLong));
HOOK_PROTOTYPE(SetWindowLongPtrW,LONG_PTR,WINAPI,(HWND hWnd, int nIndex, LONG_PTR dwNewLong));
#endif

HOOK_PROTOTYPE(GetWindowLongA,LONG,WINAPI,(HWND hWnd, int nIndex));
HOOK_PROTOTYPE(GetWindowLongW,LONG,WINAPI,(HWND hWnd, int nIndex));

#ifdef WIN64
HOOK_PROTOTYPE(GetWindowLongPtrA,LONG_PTR,WINAPI,(HWND hWnd, int nIndex));
HOOK_PROTOTYPE(GetWindowLongPtrW,LONG_PTR,WINAPI,(HWND hWnd, int nIndex));
#endif
