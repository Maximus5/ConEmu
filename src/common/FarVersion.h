
/*
Copyright (c) 2012 Maximus5
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

static bool LoadFarVersion(LPCWSTR FarPath, FarVersion& gFarVersion, wchar_t (&ErrText)[512])
{
	bool lbRc = false;
	DWORD dwErr = 0;

	ErrText[0] = 0;

	if (FarPath && *FarPath)
	{
		DWORD dwRsrvd = 0;
		DWORD dwSize = GetFileVersionInfoSize(FarPath, &dwRsrvd);

		if (dwSize>0)
		{
			void *pVerData = calloc(dwSize, 1);

			if (pVerData)
			{
				VS_FIXEDFILEINFO *lvs = NULL;
				UINT nLen = sizeof(lvs);

				if (GetFileVersionInfo(FarPath, 0, dwSize, pVerData))
				{
					wchar_t szSlash[3]; lstrcpyW(szSlash, L"\\");

					if (VerQueryValue((void*)pVerData, szSlash, (void**)&lvs, &nLen))
					{
						gFarVersion.dwVer = lvs->dwFileVersionMS;

						// Начиная с XXX сборки - Build переехал в старшее слово
						if (HIWORD(lvs->dwFileVersionLS) == 0)
							gFarVersion.dwBuild = lvs->dwFileVersionLS;
						else
							gFarVersion.dwBuild = HIWORD(lvs->dwFileVersionLS);

						lbRc = true;
					}
					else
					{
						dwErr = GetLastError(); lstrcpyW(ErrText, L"LoadFarVersion.VerQueryValue(\"\\\") failed!\n");
					}
				}
				else
				{
					dwErr = GetLastError(); lstrcpyW(ErrText, L"LoadFarVersion.GetFileVersionInfo() failed!\n");
				}

				free(pVerData);
			}
			else
			{
				_wsprintf(ErrText, SKIPLEN(countof(ErrText)) L"LoadFarVersion failed! Can't allocate %n bytes!\n", dwSize);
			}
		}
		else
		{
			dwErr = GetLastError(); lstrcpyW(ErrText, L"LoadFarVersion.GetFileVersionInfoSize() failed!\n");
		}

		if (ErrText[0]) lstrcatW(ErrText, FarPath);
	}
	else
	{
		dwErr = 0; lstrcpyW(ErrText, L"Invalid FarPath was specified");
	}

	if (ErrText[0])
	{
		if (dwErr)
		{
			int nCurLen = lstrlen(ErrText);
			_wsprintf(ErrText+nCurLen, SKIPLEN(countof(ErrText)-nCurLen) L"\nErrCode=0x%08X", dwErr);
		}

		//MessageBox(0, ErrText, L"ConEmu plugin", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
	}

	if (!lbRc)
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
