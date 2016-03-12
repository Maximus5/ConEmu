
/*
Copyright (c) 2014-2016 Maximus5
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
#define SHOWDEBUGSTR

#include "Header.h"
#include "RConFiles.h"
#include "RealConsole.h"
#include "../common/WFiles.h"

CRConFiles::CRConFiles(CRealConsole* apRCon)
	: mp_RCon(apRCon)
{
}

CRConFiles::~CRConFiles()
{
}

LPCWSTR CRConFiles::GetFileFromConsole(LPCWSTR asSrc, CEStr& szFull)
{
	CEStr szWinPath;
	LPCWSTR pszWinPath = szWinPath.Attach(MakeWinPath(asSrc));
	if (!pszWinPath || !*pszWinPath)
	{
		_ASSERTE(pszWinPath && *pszWinPath);
		return NULL;
	}

	if (IsFilePath(pszWinPath, true))
	{
		if (!FileExists(pszWinPath)) // otherwise it will cover directories too
			return NULL;
		szFull.Attach(szWinPath.Detach());
	}
	else
	{
		CEStr szDir;
		LPCWSTR pszDir = mp_RCon->GetConsoleCurDir(szDir);
		_ASSERTE(pszDir && wcschr(pszDir,L'/')==NULL);

		// Попытаться просканировать один-два уровеня подпапок
		bool bFound = FileExistSubDir(pszDir, pszWinPath, 1, szFull);

		if (!bFound)
		{
			// git diffs style, but with backslashes (they are converted already)
			// "a/src/ConEmu.cpp" and "b/src/ConEmu.cpp"
			if (pszWinPath[0] && (pszWinPath[1] == L'\\') && wcschr(L"abicwo", pszWinPath[0]))
			{
				LPCWSTR pszSlash = pszWinPath;
				while (!bFound && ((pszSlash = wcschr(pszSlash, L'\\')) != NULL))
				{
					while (*pszSlash == L'\\')
						pszSlash++;
					if (!*pszSlash)
						break;
					bFound = FileExistSubDir(pszDir, pszSlash, 1, szFull);
				}
			}
		}

		if (!bFound)
		{
			// If there is "src" subfolder in the current folder
			CEStr szSrc(JoinPath(pszDir, L"src"));
			if (DirectoryExists(szSrc))
			{
				bFound = FileExistSubDir(szSrc, pszWinPath, 1, szFull);
			}
		}

		if (!bFound)
		{
			return NULL;
		}
	}

	if (!szFull.IsEmpty())
	{
		// "src\conemu\realconsole.cpp" --> "src\ConEmu\RealConsole.cpp"
		MakePathProperCase(szFull);
	}

	return szFull;
}

void CRConFiles::ResetCache()
{
}
