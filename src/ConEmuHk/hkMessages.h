
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

/* *************************** */

// mouse_event
typedef VOID (WINAPI* Onmouse_event_t)(DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData, ULONG_PTR dwExtraInfo);
VOID WINAPI Onmouse_event(DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData, ULONG_PTR dwExtraInfo);

// SendInput
typedef UINT (WINAPI* OnSendInput_t)(UINT nInputs, LPINPUT pInputs, int cbSize);
UINT WINAPI OnSendInput(UINT nInputs, LPINPUT pInputs, int cbSize);

// PostMessageA
typedef BOOL (WINAPI* OnPostMessageA_t)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI OnPostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

// PostMessageW
typedef BOOL (WINAPI* OnPostMessageW_t)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI OnPostMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

// SendMessageA
typedef LRESULT (WINAPI* OnSendMessageA_t)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI OnSendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

// SendMessageW
typedef LRESULT (WINAPI* OnSendMessageW_t)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI OnSendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

// GetMessageA
typedef BOOL (WINAPI* OnGetMessageA_t)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
BOOL WINAPI OnGetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);

// GetMessageW
typedef BOOL (WINAPI* OnGetMessageW_t)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
BOOL WINAPI OnGetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);

// PeekMessageA
typedef BOOL (WINAPI* OnPeekMessageA_t)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
BOOL WINAPI OnPeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);

// PeekMessageW
typedef BOOL (WINAPI* OnPeekMessageW_t)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
BOOL WINAPI OnPeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
