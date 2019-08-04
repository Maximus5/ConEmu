
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

#include "Header.h"
#include "../common/MSetter.h"
#include "../common/MStrEsc.h"
#include "../common/WUser.h"

#include "AboutDlg.h"
#include "Options.h"
#include "helper.h"
#include "LngRc.h"
#include "ConEmu.h"
#include "MyClipboard.h"
#include "version.h"

BOOL gbInDisplayLastError = FALSE;

int DisplayLastError(LPCTSTR asLabel, DWORD dwError /* =0 */, DWORD dwMsgFlags /* =0 */, LPCWSTR asTitle /*= NULL*/, HWND hParent /*= NULL*/)
{
	int nBtn = 0;
	DWORD dw = dwError ? dwError : GetLastError();
	wchar_t* lpMsgBuf = NULL;
	wchar_t *out = NULL;
	MCHKHEAP

	if (dw && (dw != (DWORD)-1))
	{
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL);
		INT_PTR nLen = _tcslen(asLabel)+64+(lpMsgBuf ? _tcslen(lpMsgBuf) : 0);
		out = new wchar_t[nLen];
		swprintf_c(out, nLen/*#SECURELEN*/, _T("%s\nLastError=0x%08X\n%s"), asLabel, dw, lpMsgBuf);
	}

	if (gbMessagingStarted) apiSetForegroundWindow(hParent ? hParent : ghWnd);

	if (!dwMsgFlags) dwMsgFlags = MB_SYSTEMMODAL | MB_ICONERROR;

	BOOL lb = gbInDisplayLastError; gbInDisplayLastError = TRUE;
	nBtn = MsgBox(out ? out : asLabel, dwMsgFlags, asTitle, hParent);
	gbInDisplayLastError = lb;

	MCHKHEAP
	if (lpMsgBuf)
		LocalFree(lpMsgBuf);
	if (out)
		delete [] out;
	MCHKHEAP
	return nBtn;
}

static DWORD gn_MainThreadId;
bool isMainThread()
{
	DWORD dwTID = GetCurrentThreadId();
	return (dwTID == gn_MainThreadId);
}

void initMainThread()
{
	gn_MainThreadId = GetCurrentThreadId();
}

/// Converts Windows path "C:\path 1" to Posix "'/c/path 1'" or "/c/path\ 1"
/// @param asWinPath   Source Windows path, double-quotes ("\"C:\path\"") are not expected here
/// @param bAutoQuote  if true - use strong quoting for strings with special characters
/// @param asMntPrefix Mount prefix from RCon: "/mnt", "/cygdrive", "" or NULL
/// @param path        Buffer for result
/// @return NULL on errors or the pointer to converted #path
const wchar_t* DupCygwinPath(LPCWSTR asWinPath, bool bAutoQuote, LPCWSTR asMntPrefix, CEStr& path)
{
	if (!asWinPath || !*asWinPath)
	{
		_ASSERTE(asWinPath && *asWinPath);
		path.Clear();
		return NULL;
	}

	const wchar_t* unquotedSpecials = L" ()$!'\"";
	const wchar_t* quotedSpecials = L"'";
	_ASSERTE(wcslen(quotedSpecials)==1 && wcschr(unquotedSpecials, quotedSpecials[0]));

	// We expect either: cd 'c:\folder with space\spaces and bang !! bang'
	//               or: cd /c/folder\ with\ space/spaces\ and\ bang\ \!\!\ bang
	// Here we use single-quote (strong quotation) and if string contains

	bool useQuote = bAutoQuote ? (wcspbrk(asWinPath, unquotedSpecials) != nullptr) : false;

	// Some chars must be escaped
	const wchar_t* posixSpec = useQuote ? quotedSpecials : unquotedSpecials;

	size_t cchLen = _tcslen(asWinPath)
		+ (useQuote ? 3 : 1) // two or zero quotes + null-termination
		+ (asMntPrefix ? _tcslen(asMntPrefix) : 0) // '/cygwin' or '/mnt' prefix
		+ 1/*Possible space-termination on paste*/;
	if (wcspbrk(asWinPath, posixSpec) != NULL)
	{
		const wchar_t *pch = wcspbrk(asWinPath, posixSpec);
		while (pch)
		{
			cchLen += useQuote ? 3 : 1;
			pch = wcspbrk(pch+1, posixSpec);
		}
	}
	wchar_t* pszResult = path.GetBuffer(cchLen+1);
	if (!pszResult)
		return NULL;
	wchar_t* psz = pszResult;

	if (useQuote)
	{
		*(psz++) = L'\'';
	}

	bool stripDoubleQuot = (asWinPath[0] == L'"');
	if (stripDoubleQuot)
		++asWinPath;

	// Drive letter!
	if (asWinPath[0] && (asWinPath[1] == L':'))
	{
		// '/cygwin' or '/mnt' prefix
		LPCWSTR pszPrefix = asMntPrefix;
		if (pszPrefix)
			while (*pszPrefix)
				*(psz++) = *(pszPrefix++);
		*(psz++) = L'/';
		*(psz++) = static_cast<wchar_t>(tolower(asWinPath[0]));
		asWinPath += 2;
	}
	else
	{
		// А bash понимает сетевые пути?
		_ASSERTE((psz[0] == L'\\' && psz[1] == L'\\') || (wcschr(psz, L'\\')==NULL));
	}

	while (*asWinPath)
	{
		if (stripDoubleQuot && *asWinPath == L'"' && !*(asWinPath+1))
			break;
		if (*asWinPath == L'\\')
		{
			*(psz++) = L'/';
			asWinPath++;
		}
		else
		{
			if (wcschr(posixSpec, *asWinPath))
			{
				if (!useQuote)
					*(psz++) = L'\\';
				else
				{
					_ASSERTE(*asWinPath == L'\'');
					*(psz++) = L'\''; *(psz++) = L'\\'; *(psz++) = L'\'';
				}
			}
			*(psz++) = *(asWinPath++);
		}
	}

	if (useQuote)
		*(psz++) = L'\'';
	*psz = 0;

	return pszResult;
}

// Вернуть путь с обратными слешами, если диск указан в cygwin-формате - добавить двоеточие
// asAnyPath может быть полным или относительным путем, например
// C:\Src\file.c
// C:/Src/file.c
// /C/Src/file.c
// //server/share/file
// \\server\share/path/file
// /cygdrive/C/Src/file.c
// ..\folder/file.c
LPCWSTR MakeWinPath(LPCWSTR asAnyPath, LPCWSTR pszMntPrefix, CEStr& szWinPath)
{
	// Drop spare prefix, point to "/" after "/cygdrive"
	int iSkip = startswith(asAnyPath, pszMntPrefix ? pszMntPrefix : L"/cygdrive", true);
	if (iSkip > 0 && asAnyPath[iSkip] != L'/')
		iSkip = 0;
	LPCWSTR pszSrc = asAnyPath + ((iSkip > 0) ? iSkip : 0);

	// Prepare buffer
	int iLen = lstrlen(pszSrc);
	if (iLen < 1)
	{
		_ASSERTE(lstrlen(pszSrc) > 0);
		szWinPath.Clear();
		return NULL;
	}

	// #CYGDRIVE In some cases we may try to select real location of "~" folder (cygwin and msys)
	if (pszSrc[0] == L'~' && (pszSrc[1] == 0 || pszSrc[1] == L'/'))
	{
		szWinPath.Set(pszSrc);
		return szWinPath;
	}

	// Диск в cygwin формате?
	wchar_t cDrive = 0;
	if ((pszSrc[0] == L'/' || pszSrc[0] == L'\\')
		&& isDriveLetter(pszSrc[1])
		&& (pszSrc[2] == L'/' || pszSrc[2] == L'\\' || pszSrc[2] == 0))
	{
		cDrive = pszSrc[1];
		CharUpperBuff(&cDrive, 1);
		pszSrc += 2;
		iLen++;
	}

	// Формируем буфер
	wchar_t* pszRc = szWinPath.GetBuffer(iLen);
	if (!pszRc)
	{
		_ASSERTE(pszRc && "malloc failed");
		szWinPath.Clear();
		return NULL;
	}
	// Make path
	wchar_t* pszDst = pszRc;
	if (cDrive)
	{
		*(pszDst++) = cDrive;
		*(pszDst++) = L':';
		*(pszDst) = 0;
		iLen -= 2;
	}
	else
	{
		*(pszDst) = 0;
	}
	if (*pszSrc)
		_wcscpy_c(pszDst, iLen+1, pszSrc);
	else
		_wcscpy_c(pszDst, iLen+1, L"\\");
	// Convert slashes
	pszDst = wcschr(pszDst, L'/');
	while (pszDst)
	{
		*pszDst = L'\\';
		pszDst = wcschr(pszDst+1, L'/');
	}
	// Ready
	return pszRc;
}

wchar_t* MakeStraightSlashPath(LPCWSTR asWinPath)
{
	wchar_t* pszSlashed = lstrdup(asWinPath);
	wchar_t* p = wcschr(pszSlashed, L'\\');
	while (p)
	{
		*p = L'/';
		p = wcschr(p+1, L'\\');
	}
	return pszSlashed;
}

bool FixDirEndSlash(wchar_t* rsPath)
{
	int nLen = rsPath ? lstrlen(rsPath) : 0;
	// Do not cut slash from "C:\"
	if ((nLen > 3) && (rsPath[nLen-1] == L'\\'))
	{
		rsPath[nLen-1] = 0;
		return true;
	}
	else if ((nLen > 0) && (rsPath[nLen-1] == L':'))
	{
		// The root of drive must have end slash
		rsPath[nLen] = L'\\'; rsPath[nLen+1] = 0;
		return true;
	}
	return false;
}

// TODO: Optimize: Now pszDst must be (4x len in maximum for "\xFF" form) for bSet==true
void EscapeChar(bool bSet, LPCWSTR& pszSrc, LPWSTR& pszDst)
{
	if (bSet)
	{
		// Set escapes: wchar(13) --> "\\r"
		EscapeChar(pszSrc, pszDst);
	}
	else
	{
		// Remove escapes: "\\r" --> wchar(13), etc.
		UnescapeChar(pszSrc, pszDst);
	}
}

bool isKey(DWORD wp,DWORD vk)
{
	bool bEq = ((wp==vk)
		|| ((vk==VK_LSHIFT||vk==VK_RSHIFT)&&wp==VK_SHIFT)
		|| ((vk==VK_LCONTROL||vk==VK_RCONTROL)&&wp==VK_CONTROL)
		|| ((vk==VK_LMENU||vk==VK_RMENU)&&wp==VK_MENU));
	return bEq;
}

// pszWords - '|'separated
void StripWords(wchar_t* pszText, const wchar_t* pszWords)
{
	wchar_t dummy[MAX_PATH];
	LPCWSTR pszWord = pszWords;
	while (pszWord && *pszWord)
	{
		LPCWSTR pszNext = wcschr(pszWord, L'|');
		if (!pszNext) pszNext = pszWord + _tcslen(pszWord);

		int nLen = (int)(pszNext - pszWord);
		if (nLen > 0)
		{
			lstrcpyn(dummy, pszWord, std::min((int)countof(dummy),(nLen+1)));
			wchar_t* pszFound;
			while ((pszFound = StrStrI(pszText, dummy)) != NULL)
			{
				size_t nLeft = _tcslen(pszFound);
				size_t nCurLen = nLen;
				// Strip spaces after replaced token
				while (pszFound[nCurLen] == L' ')
					nCurLen++;
				if (nLeft <= nCurLen)
				{
					*pszFound = 0;
					break;
				}
				else
				{
					wmemmove(pszFound, pszFound+nCurLen, nLeft - nCurLen + 1);
				}
			}
		}

		if (!*pszNext)
			break;
		pszWord = pszNext + 1;
	}
}

void StripLines(wchar_t* pszText, LPCWSTR pszCommentMark)
{
	if (!pszText || !*pszText || !pszCommentMark || !*pszCommentMark)
		return;

	wchar_t* pszSrc = pszText;
	wchar_t* pszDst = pszText;
	INT_PTR iLeft = wcslen(pszText) + 1;
	INT_PTR iCmp = wcslen(pszCommentMark);

	while (iLeft > 1)
	{
		wchar_t* pszEOL = wcspbrk(pszSrc, L"\r\n");
		if (!pszEOL)
			pszEOL = pszSrc + iLeft;
		else if (pszEOL[0] == L'\r' && pszEOL[1] == L'\n')
			pszEOL += 2;
		else
			pszEOL ++;

		INT_PTR iLine = pszEOL - pszSrc;

		if (wcsncmp(pszSrc, pszCommentMark, iCmp) == 0)
		{
			// Drop this line
			if (iLeft <= iLine)
			{
				_ASSERTE(iLeft >= iLine);
				*pszDst = 0;
				break;
			}
			else
			{
				wmemmove(pszDst, pszEOL, iLeft - iLine);
				iLeft -= iLine;
			}
		}
		else
		{
			// Skip to next line
			iLeft -= iLine;
			pszSrc += iLine;
			pszDst += iLine;
		}
	}

	*pszDst = 0;
}
