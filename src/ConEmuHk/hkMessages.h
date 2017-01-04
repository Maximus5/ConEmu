
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

HOOK_PROTOTYPE(GetMessageA,BOOL,WINAPI,(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax));
HOOK_PROTOTYPE(GetMessageW,BOOL,WINAPI,(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax));
HOOK_PROTOTYPE(mouse_event,VOID,WINAPI,(DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData, ULONG_PTR dwExtraInfo));
HOOK_PROTOTYPE(PeekMessageA,BOOL,WINAPI,(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg));
HOOK_PROTOTYPE(PeekMessageW,BOOL,WINAPI,(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg));
HOOK_PROTOTYPE(PostMessageA,BOOL,WINAPI,(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam));
HOOK_PROTOTYPE(PostMessageW,BOOL,WINAPI,(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam));
HOOK_PROTOTYPE(SendInput,UINT,WINAPI,(UINT nInputs, LPINPUT pInputs, int cbSize));
HOOK_PROTOTYPE(SendMessageA,LRESULT,WINAPI,(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam));
HOOK_PROTOTYPE(SendMessageW,LRESULT,WINAPI,(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam));
