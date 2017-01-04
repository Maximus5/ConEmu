
/*
Copyright (c) 2009-2017 Maximus5
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
#include "WConsole.h"
#include "WUser.h"

// Evaluate default width for the font
int EvaluateDefaultFontWidth(int inSizeY, const wchar_t *asFontName)
{
	if (inSizeY <= 0)
		return 0;

	int nDefaultX = inSizeY * 10 / 16; // rough
	LOGFONT lf = {inSizeY};
	lstrcpyn(lf.lfFaceName, asFontName ? asFontName : L"Lucida Console", countof(lf.lfFaceName));
	HFONT hFont = CreateFontIndirect(&lf);
	if (hFont)
	{
		HDC hDC = CreateCompatibleDC(NULL);
		if (hDC)
		{
			HFONT hOldF = (HFONT)SelectObject(hDC, hFont);
			TEXTMETRIC tm = {};
			BOOL lbTM = GetTextMetrics(hDC, &tm);
			if (lbTM && (tm.tmAveCharWidth > 0))
			{
				nDefaultX = tm.tmAveCharWidth;
			}
			SelectObject(hDC, hOldF);
			DeleteDC(hDC);
		}
		DeleteObject(hFont);
	}

	return nDefaultX;
}

void SetUserFriendlyFont(HWND hConWnd, int newFontY = 0, int newFontX = 0)
{
	// Соответствующие функции появились только в API Vista
	// Win2k & WinXP - доступны только хаки, что не подходит
	_ASSERTE(_WIN32_WINNT_VISTA==0x600);
	OSVERSIONINFOEXW osvi = {sizeof(osvi), HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA)};
	DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);
	if (!VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask))
		return;

	HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD crVisibleSize = {};
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (GetConsoleScreenBufferInfo(hOutput, &csbi))
	{
		crVisibleSize.X = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		crVisibleSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	}

	if ((crVisibleSize.X <= 0) || (crVisibleSize.Y <= 0))
	{
		_ASSERTE((crVisibleSize.X > 0) && (crVisibleSize.Y > 0));
		return;
	}

	int curSizeY = 0, curSizeX = 0;
	wchar_t sFontName[LF_FACESIZE] = L"";

	if (apiGetConsoleFontSize(hOutput, curSizeY, curSizeX, sFontName) && curSizeY && curSizeX)
	{
		if (newFontY <= 0 || newFontX <= 0)
		{
			DEBUGTEST(COORD crLargest = MyGetLargestConsoleWindowSize(hOutput));

			HMONITOR hMon = MonitorFromWindow(hConWnd, MONITOR_DEFAULTTOPRIMARY);
			MONITORINFO mi = {sizeof(mi)};
			int nMaxX = 0, nMaxY = 0;
			if (GetMonitorInfo(hMon, &mi))
			{
				nMaxX = mi.rcWork.right - mi.rcWork.left - 2*GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CYCAPTION);
				nMaxY = mi.rcWork.bottom - mi.rcWork.top - 2*GetSystemMetrics(SM_CYSIZEFRAME);
			}

			if ((nMaxX > 0) && (nMaxY > 0))
			{
				int nFontX = nMaxX  / crVisibleSize.X;
				int nFontY = nMaxY / crVisibleSize.Y;
				// Too large height?
				if (nFontY > 28)
				{
					nFontX = 28 * nFontX / nFontY;
					nFontY = 28;
				}

				// Evaluate default width for the font
				int nEvalX = EvaluateDefaultFontWidth(nFontY, sFontName);
				if (nEvalX > 0)
				{
					if ((nEvalX > nFontX) && (nFontX > 0))
						nFontY = nFontX * nFontY / nEvalX;
					else
						nFontX = nEvalX;
				}

				// Look in the registry?
				HKEY hk;
				DWORD nRegSize = 0, nLen;
				if (!RegOpenKeyEx(HKEY_CURRENT_USER, L"Console", 0, KEY_READ, &hk))
				{
					if (RegQueryValueEx(hk, L"FontSize", NULL, NULL, (LPBYTE)&nRegSize, &(nLen=sizeof(nRegSize))) || nLen!=sizeof(nRegSize))
						nRegSize = 0;
					RegCloseKey(hk);
				}
				if (!nRegSize && !RegOpenKeyEx(HKEY_CURRENT_USER, L"Console\\%SystemRoot%_system32_cmd.exe", 0, KEY_READ, &hk))
				{
					if (RegQueryValueEx(hk, L"FontSize", NULL, NULL, (LPBYTE)&nRegSize, &(nLen=sizeof(nRegSize))) || nLen!=sizeof(nRegSize))
						nRegSize = 0;
					RegCloseKey(hk);
				}
				if ((HIWORD(nRegSize) > curSizeY) && (HIWORD(nRegSize) < nFontY)
					&& (LOWORD(nRegSize) > curSizeX) && (LOWORD(nRegSize) < nFontX))
				{
					nFontY = HIWORD(nRegSize);
					nFontX = LOWORD(nRegSize);
				}

				if ((nFontX > curSizeX) || (nFontY > curSizeY))
				{
					newFontY = nFontY; newFontX = nFontX;
				}
			}
		}
	}

	if ((newFontY > 0) && (newFontX > 0))
	{
		if (!*sFontName)
			lstrcpyn(sFontName, L"Lucida Console", countof(sFontName));
		apiSetConsoleFontSize(hOutput, newFontY, newFontX, sFontName);
	}
}

void CorrectConsolePos(HWND hConWnd)
{
	RECT rcNew = {};
	if (GetWindowRect(hConWnd, &rcNew))
	{
		HMONITOR hMon = MonitorFromWindow(hConWnd, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi = {sizeof(mi)};
		//int nMaxX = 0, nMaxY = 0;
		if (GetMonitorInfo(hMon, &mi))
		{
			int newW = (rcNew.right-rcNew.left), newH = (rcNew.bottom-rcNew.top);
			int newX = rcNew.left, newY = rcNew.top;

			if (newX < mi.rcWork.left)
				newX = mi.rcWork.left;
			else if (rcNew.right > mi.rcWork.right)
				newX = max(mi.rcWork.left,(mi.rcWork.right-newW));

			if (newY < mi.rcWork.top)
				newY = mi.rcWork.top;
			else if (rcNew.bottom > mi.rcWork.bottom)
				newY = max(mi.rcWork.top,(mi.rcWork.bottom-newH));

			if ((newX != rcNew.left) || (newY != rcNew.top))
				SetWindowPos(hConWnd, HWND_TOP, newX, newY,0,0, SWP_NOSIZE);
		}
	}
}

void EmergencyShow(HWND hConWnd, int newFontY /*= 0*/, int newFontX /*= 0*/)
{
	if (!hConWnd)
		hConWnd = GetConsoleWindow();

	if (IsWindowVisible(hConWnd))
		return; // уже, делать ничего не будем

	if (!IsWindow(hConWnd))
		return; // Invalid HWND

	wchar_t szMutex[80];
	_wsprintf(szMutex, SKIPCOUNT(szMutex) L"ConEmuEmergencyShow:%08X", (DWORD)(DWORD_PTR)hConWnd);
	HANDLE hMutex = CreateMutex(NULL, FALSE, szMutex);

	//_ASSERTE(FALSE && "EmergencyShow was called, Continue?");

	DWORD dwWaitResult = WaitForSingleObject(hMutex, 5000);

	if (dwWaitResult == WAIT_OBJECT_0)
	{
		if (!IsWindowVisible(hConWnd))
		{
			// Note, this funciton will fail if called from ConEmu plugin
			// Only servers (ConEmuCD) will be succeeded here
			SetUserFriendlyFont(hConWnd, newFontY, newFontX);

			CorrectConsolePos(hConWnd);

			SetWindowPos(hConWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);

			apiShowWindowAsync(hConWnd, SW_SHOWNORMAL);
		}

		ReleaseMutex(hMutex);
	}
	else
	{
		// Invalid mutex operation? Just show the console
		apiShowWindowAsync(hConWnd, SW_SHOWNORMAL);
	}

	if (!IsWindowEnabled(hConWnd))
		EnableWindow(hConWnd, true);

	CloseHandle(hMutex);
}
