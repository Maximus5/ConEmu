
/*
Copyright (c) 2013 Maximus5
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


#define HIDE_USE_EXCEPTION_INFO
#include <windows.h>
#include "common.hpp"
#include "MStrSafe.h"

void SetConEmuEnvHWND(LPCWSTR pszVarName, HWND hWnd)
{
	if (hWnd)
	{
		// Установить переменную среды с дескриптором окна
		wchar_t szVar[16];
		msprintf(szVar, countof(szVar), L"0x%08X", (DWORD)(DWORD_PTR)hWnd); //-V205
		SetEnvironmentVariable(pszVarName, szVar);
	}
	else
	{
		SetEnvironmentVariable(pszVarName, NULL);
	}
}

void SetConEmuEnvVar(HWND hConEmuWnd)
{
	SetConEmuEnvHWND(ENV_CONEMUHWND_VAR_W, hConEmuWnd);
	if (hConEmuWnd)
	{
		DWORD nPID = 0; GetWindowThreadProcessId(hConEmuWnd, &nPID);
		wchar_t szVar[16];
		msprintf(szVar, countof(szVar), L"%u", nPID);
		SetEnvironmentVariable(ENV_CONEMUPID_VAR_W, szVar);
	}
	else
	{
		SetEnvironmentVariable(ENV_CONEMUPID_VAR_W, NULL);
	}
}

void SetConEmuEnvVarChild(HWND hDcWnd, HWND hBackWnd)
{
	SetConEmuEnvHWND(ENV_CONEMUDRAW_VAR_W, hDcWnd);
	SetConEmuEnvHWND(ENV_CONEMUBACK_VAR_W, hBackWnd);
}
