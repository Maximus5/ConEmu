
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

#define HIDE_USE_EXCEPTION_INFO
#include "Common.h"
#include "MProcess.h"
#include "CmdLine.h"
#include "Monitors.h"
#include "WConsole.h"
#include "WObjects.h"


// gh-1402: On CJK versions of Win10 the COMMON_LVB_*_BYTE is set unexpectedly for many glyphs
BOOL isConsoleBadDBCS()
{
	static int isBadDBCS = 0;
	if (!isBadDBCS)
	{
		isBadDBCS = (IsWin10() && IsWinDBCS()) ? 1 : -1;
	}
	return (isBadDBCS == 1);
}



MY_CONSOLE_FONT_INFOEX g_LastSetConsoleFont = {};



// Vista+ only
BOOL apiGetConsoleScreenBufferInfoEx(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx)
{
	typedef BOOL (WINAPI* GetConsoleScreenBufferInfoEx_t)(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx);
	static GetConsoleScreenBufferInfoEx_t GetConsoleScreenBufferInfoEx_f = NULL;
	static bool bFuncChecked = false;

	if (!bFuncChecked)
	{
		HMODULE hKernel32 = GetModuleHandle(L"kernel32.dll");
		GetConsoleScreenBufferInfoEx_f = (GetConsoleScreenBufferInfoEx_t)GetProcAddress(hKernel32, "GetConsoleScreenBufferInfoEx");
		bFuncChecked = true;
	}

	BOOL lbRc = FALSE;

	if (GetConsoleScreenBufferInfoEx_f)
	{
		lbRc = GetConsoleScreenBufferInfoEx_f(hConsoleOutput, lpConsoleScreenBufferInfoEx);
	}

	return lbRc;
}

// Vista+ only
// Функция глюкавая. По крайней мере в Win7.
// 1. После ее вызова слетает видимая область в окне консоли
// 2. После ее вызова окно консоли безусловно показывается
// 1 - поправлено здесь, 2 - озаботиться должен вызывающий
BOOL apiSetConsoleScreenBufferInfoEx(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx)
{
	typedef BOOL (WINAPI* SetConsoleScreenBufferInfoEx_t)(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx);
	static SetConsoleScreenBufferInfoEx_t SetConsoleScreenBufferInfoEx_f = NULL;
	static bool bFuncChecked = false;

	if (!bFuncChecked)
	{
		HMODULE hKernel32 = GetModuleHandle(L"kernel32.dll");
		SetConsoleScreenBufferInfoEx_f = (SetConsoleScreenBufferInfoEx_t)GetProcAddress(hKernel32, "SetConsoleScreenBufferInfoEx");
		bFuncChecked = true;
	}

	BOOL lbRc = FALSE;
	BOOL lbWnd = FALSE;

	if (SetConsoleScreenBufferInfoEx_f)
	{
		lbRc = SetConsoleScreenBufferInfoEx_f(hConsoleOutput, lpConsoleScreenBufferInfoEx);

		// Win7 x64 BUGBUG? dimension and location of visible area is broken after call

		lbWnd = SetConsoleWindowInfo(hConsoleOutput, TRUE, &lpConsoleScreenBufferInfoEx->srWindow);
	}

	UNREFERENCED_PARAMETER(lbWnd);
	return lbRc;
}


#ifdef _DEBUG
// When detaching RCon, we set user-friendly font
bool g_IgnoreSetLargeFont = false;
#endif


// Vista+ Kernel32::GetCurrentConsoleFontEx
BOOL apiGetCurrentConsoleFontEx(HANDLE hConsoleOutput, BOOL bMaximumWindow, MY_CONSOLE_FONT_INFOEX* lpConsoleCurrentFontEx)
{
	typedef BOOL (WINAPI *PGetCurrentConsoleFontEx)(HANDLE /*hConsoleOutput*/, BOOL /*bMaximumWindow*/, MY_CONSOLE_FONT_INFOEX* /*lpConsoleCurrentFontEx*/);
	static PGetCurrentConsoleFontEx GetCurrentConsoleFontEx = NULL;
	static bool bFuncChecked = false;

	if (!bFuncChecked)
	{
		HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
		if (!hKernel)
			return FALSE;
		GetCurrentConsoleFontEx = (PGetCurrentConsoleFontEx)
			GetProcAddress(hKernel, "GetCurrentConsoleFontEx");
		bFuncChecked = true;
	}
	if (!GetCurrentConsoleFontEx)
	{
		return FALSE;
	}

	BOOL lbRc = GetCurrentConsoleFontEx(hConsoleOutput, bMaximumWindow, lpConsoleCurrentFontEx);

	#ifdef _DEBUG
	if (lbRc && (lpConsoleCurrentFontEx->dwFontSize.Y > 10))
	{
		static bool bWarned = false;
		if (!bWarned && !g_IgnoreSetLargeFont)
		{
			_ASSERTE(FALSE && "GetCurrentConsoleFontEx detects large RealConsole font");
			bWarned = TRUE;
		}
	}
	#endif

	return lbRc;
}

// Vista+ Kernel32::SetCurrentConsoleFontEx
BOOL apiSetCurrentConsoleFontEx(HANDLE hConsoleOutput, BOOL bMaximumWindow, MY_CONSOLE_FONT_INFOEX* lpConsoleCurrentFontEx)
{
	typedef BOOL (WINAPI *PSetCurrentConsoleFontEx)(HANDLE /*hConsoleOutput*/, BOOL /*bMaximumWindow*/, MY_CONSOLE_FONT_INFOEX* /*lpConsoleCurrentFontEx*/);
	static PSetCurrentConsoleFontEx SetCurrentConsoleFontEx = NULL;
	static bool bFuncChecked = false;

	if (!bFuncChecked)
	{
		HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
		if (!hKernel)
			return FALSE;
		SetCurrentConsoleFontEx = (PSetCurrentConsoleFontEx)
			GetProcAddress(hKernel, "SetCurrentConsoleFontEx");
		bFuncChecked = true;
	}
	if (!SetCurrentConsoleFontEx)
	{
		return FALSE;
	}

	#ifdef _DEBUG
	if (lpConsoleCurrentFontEx->dwFontSize.Y > 10)
	{
		static bool bWarned = false;
		if (!bWarned && !g_IgnoreSetLargeFont)
		{
			_ASSERTE(FALSE && "Going to set large RealConsole font");
			bWarned = TRUE;
		}
	}
	#endif

	BOOL lbRc = SetCurrentConsoleFontEx(hConsoleOutput, bMaximumWindow, lpConsoleCurrentFontEx);
	return lbRc;
}



//Used in RM_ALTSERVER
BOOL apiInitConsoleFontSize(HANDLE hOutput)
{
	BOOL lbRc = FALSE;

	HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
	if (!hKernel)
		return FALSE;

	if (IsWin6())  // We have Vista
	{
		MY_CONSOLE_FONT_INFOEX cfi = { sizeof(cfi) };
		if (apiGetCurrentConsoleFontEx(hOutput, FALSE, &cfi))
		{
			g_LastSetConsoleFont = cfi;
			lbRc = TRUE;
		}
	}

	return lbRc;
}

// Vista+ only
BOOL apiGetConsoleFontSize(HANDLE hOutput, int &SizeY, int &SizeX, wchar_t (&rsFontName)[LF_FACESIZE])
{
	HMODULE hKernel = GetModuleHandle(L"kernel32.dll");

	if (!hKernel)
	{
		_ASSERTE(hKernel!=NULL);
		return FALSE;
	}

	BOOL lbRc = FALSE;

	if (IsWin6())  // We have Vista
	{
		MY_CONSOLE_FONT_INFOEX cfi = {sizeof(cfi)};
		lbRc = apiGetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
		if (lbRc)
		{
			SizeX = cfi.dwFontSize.X;
			SizeY = cfi.dwFontSize.Y;
			lstrcpynW(rsFontName, cfi.FaceName, countof(rsFontName));
		}
	}

	return lbRc;
}

// Vista+ only
BOOL apiSetConsoleFontSize(HANDLE hOutput, int inSizeY, int inSizeX, const wchar_t *asFontName)
{
	HMODULE hKernel = GetModuleHandle(L"kernel32.dll");

	if (!hKernel)
	{
		_ASSERTE(hKernel!=NULL);
		return FALSE;
	}

	BOOL lbRc = FALSE;

	if (IsWin6())  // We have Vista
	{
		MY_CONSOLE_FONT_INFOEX cfi = {sizeof(cfi)};
		//apiGetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
		cfi.dwFontSize.X = inSizeX;
		cfi.dwFontSize.Y = inSizeY;
		lstrcpynW(cfi.FaceName, asFontName ? asFontName : L"Lucida Console", countof(cfi.FaceName));

		HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
		lbRc = apiSetCurrentConsoleFontEx(hConOut, FALSE, &cfi);

		// Store for further comparison
		if (lbRc)
		{
			MY_CONSOLE_FONT_INFOEX cfiSet = {sizeof(cfiSet)};
			if (apiGetCurrentConsoleFontEx(hConOut, FALSE, &cfiSet))
			{
				// Win10 can't set "Lucida Console 3x5" and we get "4x6"
				_ASSERTE(_abs(cfiSet.dwFontSize.X-cfi.dwFontSize.X)<=2 && _abs(cfiSet.dwFontSize.Y-cfi.dwFontSize.Y)<=1);
				g_LastSetConsoleFont = cfiSet;
			}
			else
			{
				DEBUGTEST(DWORD dwErr = GetLastError());
				_ASSERTE(FALSE && "apiGetConsoleScreenBufferInfoEx failed");
				g_LastSetConsoleFont = cfi;
			}
		}
	}

	return lbRc;
}

// Vista+ only
BOOL apiFixFontSizeForBufferSize(HANDLE hOutput, COORD dwSize, char* pszUtfLog /*= NULL*/, int cchLogMax /*= 0*/)
{
	BOOL lbRetry = FALSE;
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};

	if (pszUtfLog) *pszUtfLog = 0;

	if (dwSize.Y > 0 && dwSize.X > 0
		&& GetConsoleScreenBufferInfo(hOutput, &csbi))
	{
		COORD crLargest = MyGetLargestConsoleWindowSize(hOutput);
		int nMaxX = GetSystemMetrics(SM_CXFULLSCREEN);
		int nMaxY = GetSystemMetrics(SM_CYFULLSCREEN);

		#ifdef _DEBUG
		// Для отладки, смотрим какой размер получится по crLargest
		int nDbgFontX = crLargest.X ? (nMaxX / crLargest.X) : -1;
		int nDbgFontY = crLargest.Y ? (nMaxY / crLargest.Y) : -1;
		#endif

		int curSizeY, curSizeX, newSizeY, newSizeX, calcSizeY, calcSizeX;
		wchar_t sFontName[LF_FACESIZE];
		if (apiGetConsoleFontSize(hOutput, curSizeY, curSizeX, sFontName) && curSizeY && curSizeX)
		{
			// Увеличение
			if (crLargest.X && crLargest.Y && ((dwSize.X > crLargest.X) || (dwSize.Y > crLargest.Y)))
			{
				// Теперь прикинуть, какой размер шрифта нам нужен
				newSizeY = std::max(1,(nMaxY / (dwSize.Y+1)));
				newSizeX = std::max(1,(nMaxX / (dwSize.X+1)));
				if ((newSizeY < curSizeY) || (newSizeX < curSizeX))
				{
					calcSizeX = newSizeY * curSizeX / curSizeY;
					calcSizeY = newSizeX * curSizeY / curSizeX;
					if (calcSizeY < curSizeY)
						calcSizeX = std::min(calcSizeX,(calcSizeY * curSizeX / curSizeY));
					if (calcSizeX < curSizeX)
						calcSizeY = std::min(calcSizeY,(calcSizeX * curSizeY / curSizeX));

					newSizeY = std::max(1,std::min(calcSizeY,curSizeY));
					newSizeX = std::max(1,std::min(calcSizeX,curSizeX));
					lbRetry = TRUE;
				}
			}
			// Уменьшение
			else if ((dwSize.X <= (csbi.srWindow.Right - csbi.srWindow.Left))
				|| (dwSize.Y <= (csbi.srWindow.Bottom - csbi.srWindow.Top)))
			{
				int nMinY = GetSystemMetrics(SM_CYMIN) - GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYCAPTION);
				int nMinX = GetSystemMetrics(SM_CXMIN) - 2*GetSystemMetrics(SM_CXSIZEFRAME);
				if ((nMinX > 0) && (nMinY > 0))
				{
					// Теперь прикинуть, какой размер шрифта нам нужен
					newSizeY = (nMinY / (dwSize.Y)) + 1;
					// Win8. На базовых настройках разбиение консоли дважды по Ctrl+Shift+O
					// дает ошибку - размер шрифта в третьей консоли оказывается недостаточным
					newSizeX = (nMinX / (dwSize.X)) + 2;
					if ((newSizeY > curSizeY) || (newSizeX > curSizeX))
					{
						calcSizeX = newSizeY * curSizeX / curSizeY;
						calcSizeY = newSizeX * curSizeY / curSizeX;
						if (calcSizeY > curSizeY)
							calcSizeX = std::max(calcSizeX,(calcSizeY * curSizeX / curSizeY));
						if (calcSizeX > curSizeX)
							calcSizeY = std::max(calcSizeY,(calcSizeX * curSizeY / curSizeX));

						newSizeY = std::max(calcSizeY,curSizeY);
						newSizeX = std::max(calcSizeX,curSizeX);
						lbRetry = TRUE;
					}
				}
			}

			if (lbRetry)
			{
				lbRetry = apiSetConsoleFontSize(hOutput, newSizeY, newSizeX, sFontName);
			}
		}
	}

	return lbRetry;
}



static bool gbPauseConsoleWasRequested = false;

BOOL apiPauseConsoleOutput(HWND hConWnd, bool bPause)
{
	BOOL bOk = FALSE, bCi = FALSE;
	DWORD_PTR dwRc = 0;
	DWORD nTimeout = 15000;

	if (bPause)
	{
		CONSOLE_CURSOR_INFO ci = {};
		bCi = GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);

		bOk = (SendMessageTimeout(hConWnd, WM_SYSCOMMAND, 65522, 0, SMTO_NORMAL, nTimeout, &dwRc) != 0);

		// In Win2k we can't be sure about active selection,
		// so we check for cursor height equal to 100%
		gbPauseConsoleWasRequested = (bCi && ci.dwSize < 100);
	}
	else
	{
		bOk = (SendMessageTimeout(hConWnd, WM_KEYDOWN, VK_ESCAPE, 0, SMTO_NORMAL, nTimeout, &dwRc) != 0);
		gbPauseConsoleWasRequested = false;
	}

	return bOk;
}

// Function GetConsoleSelectionInfo does NOT exists in Windows 2000
// The function is used ONLY for checking if console is "Paused",
// because when selection is present - StdOut is placed in "halt" state
BOOL apiGetConsoleSelectionInfo(PCONSOLE_SELECTION_INFO lpConsoleSelectionInfo)
{
	typedef BOOL (WINAPI* GetConsoleSelectionInfo_t)(PCONSOLE_SELECTION_INFO lpConsoleSelectionInfo);
	static GetConsoleSelectionInfo_t getConsoleSelectionInfo = NULL;
	static bool bFuncChecked = false;

	if (!bFuncChecked)
	{
		HMODULE hKernel32 = GetModuleHandle(L"kernel32.dll");
		getConsoleSelectionInfo = hKernel32 ? (GetConsoleSelectionInfo_t)GetProcAddress(hKernel32, "GetConsoleSelectionInfo") : NULL;
		bFuncChecked = true;
	}

	BOOL bRc = FALSE;
	if (getConsoleSelectionInfo)
	{
		bRc = getConsoleSelectionInfo(lpConsoleSelectionInfo);
	}
	else if (IsWin2kEql()) // Otherwise, that might be Windows 2000
	{
		// We can't be sure, but if cursor height is 100% it will be selection most probably
		bool bProbableSelection = false;
		CONSOLE_CURSOR_INFO ci = {};
		if (gbPauseConsoleWasRequested)
		{
			bRc = GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
			if (bRc && (ci.dwSize == 100))
			{
				bProbableSelection = true;
			}
		}

		memset(lpConsoleSelectionInfo, 0, sizeof(*lpConsoleSelectionInfo));
		if (bProbableSelection)
		{
			lpConsoleSelectionInfo->dwFlags = CONSOLE_SELECTION_IN_PROGRESS;
		}
	}
	else
	{
		_ASSERTE(FALSE && "GetConsoleSelectionInfo was not found!");
	}
	return bRc;
}

COORD MyGetLargestConsoleWindowSize(HANDLE hConsoleOutput)
{
	// Fails in Wine
	COORD crMax = GetLargestConsoleWindowSize(hConsoleOutput);
	DWORD dwErr = (crMax.X && crMax.Y) ? 0 : GetLastError();
	UNREFERENCED_PARAMETER(dwErr);

	// Wine BUG
	//if (!crMax.X || !crMax.Y)
	if ((crMax.X == 80 && crMax.Y == 24) && IsWine())
	{
		crMax.X = 999;
		crMax.Y = 999;
	}
	#ifdef _DEBUG
	else if (IsWin10())
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi = {};
		GetConsoleScreenBufferInfo(hConsoleOutput, &csbi);

		// Windows 10 Preview had a new bug in GetLargestConsoleWindowSize
		_ASSERTE((crMax.X > (csbi.srWindow.Right-csbi.srWindow.Left+1)) && (crMax.Y > (csbi.srWindow.Bottom-csbi.srWindow.Top+1)));
		//crMax.X = std::max(crMax.X,555);
		//crMax.Y = std::max(crMax.Y,555);
	}
	#endif

	return crMax;
}

MSetConTextAttr::MSetConTextAttr(HANDLE hConOut, WORD wAttributes)
	: mh_ConOut(NULL)
	, m_csbi()
{
	if (GetConsoleScreenBufferInfo(hConOut, &m_csbi))
	{
		_ASSERTE(m_csbi.wAttributes);
		mh_ConOut = hConOut;
		SetConsoleTextAttribute(mh_ConOut, wAttributes);
	}
}

MSetConTextAttr::~MSetConTextAttr()
{
	if (mh_ConOut && m_csbi.wAttributes)
	{
		SetConsoleTextAttribute(mh_ConOut, m_csbi.wAttributes);
	}
}
