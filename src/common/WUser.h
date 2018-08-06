
/*
Copyright (c) 2009-present Maximus5
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

#include "Common.h"

bool apiSetForegroundWindow(HWND ahWnd);
bool apiShowWindow(HWND ahWnd, int anCmdShow);
bool apiShowWindowAsync(HWND ahWnd, int anCmdShow);

// WinAPI wrappers
void getWindowInfo(HWND ahWnd, wchar_t (&rsInfo)[1024], bool bProcessName = false, LPDWORD pnPID = NULL);

bool IsUserAdmin();
bool GetLogonSID (HANDLE hToken, wchar_t **ppszSID);

/// Duplicate our current process handle for use in anTargetPID
HANDLE DuplicateProcessHandle(DWORD anTargetPID);
/// Duplicate array of handles for use in anTargetPID
bool DuplicateHandleForPID(const DWORD anTargetPID, const size_t count, const HANDLE ahSourceHandle[], HANDLE ahDestHandle[]);

/// Used in GUI during settings loading
void FindComspec(ConEmuComspec* pOpt, bool bCmdAlso = true);
/// Update env.var `ComSpec` and (if requested) the `PATH` for compec from *pOpt*
void UpdateComspec(ConEmuComspec* pOpt, bool DontModifyPath = false);

/// Set env.var from `asName` to *expanded* value from `asValue`
void SetEnvVarExpanded(LPCWSTR asName, LPCWSTR asValue);
