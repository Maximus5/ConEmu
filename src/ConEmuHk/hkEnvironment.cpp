
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

#define HIDE_USE_EXCEPTION_INFO

#ifdef _DEBUG
	#define DebugString(x) //OutputDebugString(x)
#else
	#define DebugString(x) //OutputDebugString(x)
#endif

#include "../common/Common.h"

#include "Ansi.h"
#include "hkEnvironment.h"

/* **************** */

void CheckVariables()
{
	// Пока что он проверяет и меняет только ENV_CONEMUANSI_VAR_W ("ConEmuANSI")
	GetConMap(FALSE);
}

void CheckAnsiConVar(LPCWSTR asName)
{
	bool bAnsi = false;
	CEAnsi::GetFeatures(&bAnsi, NULL);

	if (bAnsi)
	{
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO lsbi = {{0,0}};
		if (GetConsoleScreenBufferInfo(hOut, &lsbi))
		{
			wchar_t szInfo[40];
			msprintf(szInfo, countof(szInfo), L"%ux%u (%ux%u)", lsbi.dwSize.X, lsbi.dwSize.Y, lsbi.srWindow.Right-lsbi.srWindow.Left+1, lsbi.srWindow.Bottom-lsbi.srWindow.Top+1);
			SetEnvironmentVariable(ENV_ANSICON_VAR_W, szInfo);

			static WORD clrDefault = 0xFFFF;
			if ((clrDefault == 0xFFFF) && LOBYTE(lsbi.wAttributes))
				clrDefault = LOBYTE(lsbi.wAttributes);
			msprintf(szInfo, countof(szInfo), L"%X", clrDefault);
			SetEnvironmentVariable(ENV_ANSICON_DEF_VAR_W, szInfo);
		}
	}
	else
	{
		SetEnvironmentVariable(ENV_ANSICON_VAR_W, NULL);
		SetEnvironmentVariable(ENV_ANSICON_DEF_VAR_W, NULL);
	}
}


/* **************** */

BOOL WINAPI OnSetEnvironmentVariableA(LPCSTR lpName, LPCSTR lpValue)
{
	//typedef BOOL (WINAPI* OnSetEnvironmentVariableA_t)(LPCSTR lpName, LPCSTR lpValue);
	ORIGINAL_KRNL(SetEnvironmentVariableA);

	if (lpName && *lpName)
	{
		if (lstrcmpiA(lpName, ENV_CONEMUFAKEDT_VAR_A) == 0)
		{
			MultiByteToWideChar(CP_OEMCP, 0, lpValue ? lpValue : "", -1, gszTimeEnvVarSave, countof(gszTimeEnvVarSave)-1);
		}
	}

	BOOL b = F(SetEnvironmentVariableA)(lpName, lpValue);

	return b;
}


BOOL WINAPI OnSetEnvironmentVariableW(LPCWSTR lpName, LPCWSTR lpValue)
{
	//typedef BOOL (WINAPI* OnSetEnvironmentVariableW_t)(LPCWSTR lpName, LPCWSTR lpValue);
	ORIGINAL_KRNL(SetEnvironmentVariableW);

	if (lpName && *lpName)
	{
		if (lstrcmpi(lpName, ENV_CONEMUFAKEDT_VAR_W) == 0)
		{
			lstrcpyn(gszTimeEnvVarSave, lpValue ? lpValue : L"", countof(gszTimeEnvVarSave));
		}
	}

	BOOL b = F(SetEnvironmentVariableW)(lpName, lpValue);

	return b;
}


DWORD WINAPI OnGetEnvironmentVariableA(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize)
{
	//typedef DWORD (WINAPI* OnGetEnvironmentVariableA_t)(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize);
	ORIGINAL_KRNL(GetEnvironmentVariableA);
	DWORD lRc = 0;

	if (lpName)
	{
		if ((lstrcmpiA(lpName, ENV_CONEMUANSI_VAR_A) == 0)
			|| (lstrcmpiA(lpName, ENV_CONEMUHWND_VAR_A) == 0)
			|| (lstrcmpiA(lpName, ENV_CONEMUDIR_VAR_A) == 0)
			|| (lstrcmpiA(lpName, ENV_CONEMUBASEDIR_VAR_A) == 0)
			)
		{
			CheckVariables();
		}
		else if (lstrcmpiA(lpName, ENV_CONEMUANSI_VAR_A) == 0)
		{
			CheckAnsiConVar(ENV_CONEMUANSI_VAR_W);
		}
		else if (lstrcmpiA(lpName, ENV_ANSICON_DEF_VAR_A) == 0)
		{
			CheckAnsiConVar(ENV_ANSICON_DEF_VAR_W);
		}
		else if (lstrcmpiA(lpName, ENV_ANSICON_VER_VAR_A) == 0)
		{
			if (lpBuffer && ((INT_PTR)nSize > lstrlenA(ENV_ANSICON_VER_VALUE)))
			{
				lstrcpynA(lpBuffer, ENV_ANSICON_VER_VALUE, nSize);
				lRc = lstrlenA(ENV_ANSICON_VER_VALUE);
			}
			goto wrap;
		}
	}

	lRc = F(GetEnvironmentVariableA)(lpName, lpBuffer, nSize);
wrap:
	return lRc;
}


DWORD WINAPI OnGetEnvironmentVariableW(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize)
{
	//typedef DWORD (WINAPI* OnGetEnvironmentVariableW_t)(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize);
	ORIGINAL_KRNL(GetEnvironmentVariableW);
	DWORD lRc = 0;

	if (lpName)
	{
		if ((lstrcmpiW(lpName, ENV_CONEMUANSI_VAR_W) == 0)
			|| (lstrcmpiW(lpName, ENV_CONEMUHWND_VAR_W) == 0)
			|| (lstrcmpiW(lpName, ENV_CONEMUDIR_VAR_W) == 0)
			|| (lstrcmpiW(lpName, ENV_CONEMUBASEDIR_VAR_W) == 0)
			)
		{
			CheckVariables();
		}
		else if ((lstrcmpiW(lpName, ENV_CONEMUANSI_VAR_W) == 0)
				|| (lstrcmpiW(lpName, ENV_ANSICON_DEF_VAR_W) == 0))
		{
			CheckAnsiConVar(lpName);
		}
		else if (lstrcmpiW(lpName, ENV_ANSICON_VER_VAR_W) == 0)
		{
			if (lpBuffer && ((INT_PTR)nSize > lstrlenA(ENV_ANSICON_VER_VALUE)))
			{
				lstrcpynW(lpBuffer, _CRT_WIDE(ENV_ANSICON_VER_VALUE), nSize);
				lRc = lstrlenA(ENV_ANSICON_VER_VALUE);
			}
			goto wrap;
		}
	}

	lRc = F(GetEnvironmentVariableW)(lpName, lpBuffer, nSize);
wrap:
	return lRc;
}

#if 0
LPCH WINAPI OnGetEnvironmentStringsA()
{
	//typedef LPCH (WINAPI* OnGetEnvironmentStringsA_t)();
	ORIGINAL_KRNL(GetEnvironmentStringsA);

	CheckVariables();

	LPCH lpRc = F(GetEnvironmentStringsA)();
	return lpRc;
}
#endif


LPWCH WINAPI OnGetEnvironmentStringsW()
{
	//typedef LPWCH (WINAPI* OnGetEnvironmentStringsW_t)();
	ORIGINAL_KRNL(GetEnvironmentStringsW);

	CheckVariables();

	LPWCH lpRc = F(GetEnvironmentStringsW)();
	return lpRc;
}
