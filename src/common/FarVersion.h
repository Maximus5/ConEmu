
/*
Copyright (c) 2012-2017 Maximus5
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

static void SetDefaultFarVersion(FarVersion& gFarVersion)
{
	gFarVersion.dwVer = 0; // Reset all fields
	gFarVersion.dwVerMajor = 2;
	gFarVersion.dwVerMinor = 0;
	gFarVersion.dwBuild = 995; // "default"
	gFarVersion.dwBits = WIN3264TEST(32,64);
}

static bool LoadModuleVersion(LPCWSTR asModulePath, VS_FIXEDFILEINFO& Version, wchar_t* ErrText, size_t cchErrMax)
{
	bool lbRc = false;
	DWORD dwErr = 0;

	if (ErrText) ErrText[0] = 0;

	if (asModulePath && *asModulePath)
	{
		DWORD dwRsrvd = 0;
		DWORD dwSize = GetFileVersionInfoSize(asModulePath, &dwRsrvd);

		if (dwSize>0)
		{
			void *pVerData = calloc(dwSize, 1);

			if (pVerData)
			{
				VS_FIXEDFILEINFO *lvs = NULL;
				UINT nLen = 0;

				if (GetFileVersionInfo(asModulePath, 0, dwSize, pVerData))
				{
					wchar_t szSlash[3]; lstrcpyW(szSlash, L"\\");

					if (VerQueryValue((void*)pVerData, szSlash, (void**)&lvs, &nLen))
					{
						_ASSERTE(nLen == sizeof(*lvs));
						memmove(&Version, lvs, min(nLen, sizeof(Version)));
						lbRc = true;
					}
					else
					{
						dwErr = GetLastError();
						if (ErrText) _wcscpy_c(ErrText, cchErrMax, L"LoadAppVersion.VerQueryValue(\"\\\") failed!\n");
					}
				}
				else
				{
					dwErr = GetLastError();
					if (ErrText) _wcscpy_c(ErrText, cchErrMax, L"LoadAppVersion.GetFileVersionInfo() failed!\n");
				}

				free(pVerData);
			}
			else
			{
				if (ErrText) _wsprintf(ErrText, SKIPLEN(cchErrMax) L"LoadAppVersion failed! Can't allocate %n bytes!\n", dwSize);
			}
		}
		else
		{
			dwErr = GetLastError();
			if (ErrText) _wcscpy_c(ErrText, cchErrMax, L"LoadAppVersion.GetFileVersionInfoSize() failed!\n");
		}

		if (ErrText && *ErrText) _wcscat_c(ErrText, cchErrMax, asModulePath);
	}
	else
	{
		dwErr = 0;
		if (ErrText) _wcscpy_c(ErrText, cchErrMax, L"Invalid asModulePath was specified");
	}

	if (ErrText && *ErrText)
	{
		if (dwErr)
		{
			int nCurLen = lstrlen(ErrText);
			_wsprintf(ErrText+nCurLen, SKIPLEN(cchErrMax-nCurLen) L"\nErrCode=0x%08X", dwErr);
		}
	}

	if (!lbRc)
	{
		ZeroStruct(Version);
	}

	return lbRc;
}

static bool LoadAppVersion(LPCWSTR FarPath, VS_FIXEDFILEINFO& Version, wchar_t (&ErrText)[512])
{
	return LoadModuleVersion(FarPath, Version, ErrText, countof(ErrText));
}

static void ConvertVersionToFarVersion(const VS_FIXEDFILEINFO& lvs, FarVersion& gFarVersion)
{
	gFarVersion.dwVer = lvs.dwFileVersionMS;

	// Начиная с XXX сборки - Build переехал в старшее слово
	if (HIWORD(lvs.dwFileVersionLS) == 0)
	{
		gFarVersion.dwBuild = lvs.dwFileVersionLS;
	}
	else
	{
		gFarVersion.dwBuild = HIWORD(lvs.dwFileVersionLS);
	}
}

static bool LoadFarVersion(LPCWSTR FarPath, FarVersion& gFarVersion, wchar_t (&ErrText)[512])
{
	bool lbRc = false;
	VS_FIXEDFILEINFO lvs = {};

	lbRc = LoadAppVersion(FarPath, lvs, ErrText);

	if (lbRc)
	{
		ConvertVersionToFarVersion(lvs, gFarVersion);
	}
	else
	{
		SetDefaultFarVersion(gFarVersion);
	}

	return lbRc;
}

static bool LoadFarVersion(FarVersion& gFarVersion, wchar_t (&ErrText)[512])
{
	bool lbRc = false;
	wchar_t FarPath[MAX_PATH+1];

	ErrText[0] = 0;

	gFarVersion.dwBits = WIN3264TEST(32,64);

	if (GetModuleFileName(0,FarPath,MAX_PATH))
	{
		lbRc = LoadFarVersion(FarPath, gFarVersion, ErrText);
	}
	else
	{
		lstrcpyW(ErrText, L"LoadFarVersion.GetModuleFileName() failed!");
		SetDefaultFarVersion(gFarVersion);
	}

	return lbRc;
}
