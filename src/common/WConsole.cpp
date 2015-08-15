
/*
Copyright (c) 2009-2015 Maximus5
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
#include "common.hpp"
#include <TlHelp32.h>
#include "CmdLine.h"
#include "Monitors.h"
#include "WConsole.h"
#include "WObjects.h"
#include "HkFunc.h"

#ifndef CONEMU_MINIMAL
#include "WUser.h"
#endif

// Undocumented console message
#define WM_SETCONSOLEINFO			(WM_USER+201)

//#pragma pack(push, 1)
#include <pshpack1.h>

//
//	Structure to send console via WM_SETCONSOLEINFO
//
typedef struct _CONSOLE_INFO
{
	ULONG		Length;
	COORD		ScreenBufferSize;
	COORD		WindowSize;
	ULONG		WindowPosX;
	ULONG		WindowPosY;

	COORD		FontSize;
	ULONG		FontFamily;
	ULONG		FontWeight;
	WCHAR		FaceName[32];

	ULONG		CursorSize;
	ULONG		FullScreen;
	ULONG		QuickEdit;
	ULONG		AutoPosition;
	ULONG		InsertMode;

	USHORT		ScreenColors;
	USHORT		PopupColors;
	ULONG		HistoryNoDup;
	ULONG		HistoryBufferSize;
	ULONG		NumberOfHistoryBuffers;

	COLORREF	ColorTable[16];

	ULONG		CodePage;
	HWND		Hwnd;

	WCHAR		ConsoleTitle[0x100];

} CONSOLE_INFO;

#ifndef CONEMU_MINIMAL
CONSOLE_INFO* gpConsoleInfoStr = NULL;
HANDLE ghConsoleSection = NULL;
const UINT gnConsoleSectionSize = sizeof(CONSOLE_INFO)+1024;
#endif

//#pragma pack(pop)
#include <poppack.h>

MY_CONSOLE_FONT_INFOEX g_LastSetConsoleFont = {};

#ifndef CONEMU_MINIMAL
void WINAPI ShutdownConsole()
{
	if (ghConsoleSection)
	{
		CloseHandle(ghConsoleSection);
		ghConsoleSection = NULL;
	}

	if (gpConsoleInfoStr)
	{
		LocalFree(gpConsoleInfoStr);
		gpConsoleInfoStr = NULL;
	}
}
#endif


//
//	SETCONSOLEINFO.C
//
//	Undocumented method to set console attributes at runtime
//
//	For Vista use the newly documented SetConsoleScreenBufferEx API
//
//	www.catch22.net
//


#ifndef CONEMU_MINIMAL
//
//	Wrapper around WM_SETCONSOLEINFO. We need to create the
//  necessary section (file-mapping) object in the context of the
//  process which owns the console, before posting the message
//
BOOL SetConsoleInfo(HWND hwndConsole, CONSOLE_INFO *pci)
{
	DWORD   dwConsoleOwnerPid, dwCurProcId;
	PVOID   ptrView = 0;
	DWORD   dwLastError=0;
	WCHAR   ErrText[255];
	//
	//	Retrieve the process which "owns" the console
	//
	dwCurProcId = GetCurrentProcessId();
	
	DEBUGTEST(DWORD dwConsoleThreadId =)
	GetWindowThreadProcessId(hwndConsole, &dwConsoleOwnerPid);

	// We'll fail, if console was created by other process
	if (dwConsoleOwnerPid != dwCurProcId)
	{
		#ifdef _DEBUG
		// Wine related
		PROCESSENTRY32W pi = {};
		GetProcessInfo(dwConsoleOwnerPid, &pi);
		if (lstrcmpi(pi.szExeFile, L"wineconsole.exe")!=0)
		{
			wchar_t szDbgMsg[512], szTitle[128];
			szDbgMsg[0] = 0;
			GetModuleFileName(NULL, szDbgMsg, countof(szDbgMsg));
			msprintf(szTitle, countof(szTitle), L"%s: PID=%u", PointToName(szDbgMsg), GetCurrentProcessId());
			msprintf(szDbgMsg, countof(szDbgMsg), L"GetWindowThreadProcessId()\nPID=%u, TID=%u, %s\n%s",
				dwConsoleOwnerPid, dwConsoleThreadId,
				pi.szExeFile, szTitle);
			MessageBox(NULL, szDbgMsg, szTitle, MB_SYSTEMMODAL);
		}
		//_ASSERTE(dwConsoleOwnerPid == dwCurProcId);
		#endif

		return FALSE;
	}

	//
	// Create a SECTION object backed by page-file, then map a view of
	// this section into the owner process so we can write the contents
	// of the CONSOLE_INFO buffer into it
	//
	if (!ghConsoleSection)
	{
		ghConsoleSection = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, gnConsoleSectionSize, 0);

		if (!ghConsoleSection)
		{
			dwLastError = GetLastError();
			_wsprintf(ErrText, SKIPLEN(countof(ErrText)) L"Can't CreateFileMapping(ghConsoleSection). ErrCode=%i", dwLastError);
			MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
			return FALSE;
		}

		_ASSERTE(OnShutdownConsole==NULL || OnShutdownConsole==ShutdownConsole);
		OnShutdownConsole = ShutdownConsole;
	}

	//
	//	Copy our console structure into the section-object
	//
	ptrView = MapViewOfFile(ghConsoleSection, FILE_MAP_WRITE|FILE_MAP_READ, 0, 0, gnConsoleSectionSize);

	if (!ptrView)
	{
		dwLastError = GetLastError();
		_wsprintf(ErrText, SKIPLEN(countof(ErrText)) L"Can't MapViewOfFile. ErrCode=%i", dwLastError);
		MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
	}
	else
	{
		_ASSERTE(pci->Length==sizeof(CONSOLE_INFO));
		//2010-09-19 что-то на XP стало окошко мелькать.
		// при отсылке WM_SETCONSOLEINFO консоль отображается :(
		BOOL lbWasVisible = IsWindowVisible(hwndConsole);
		RECT rcOldPos = {0}, rcAllMonRect = {0};

		if (!lbWasVisible)
		{
			GetWindowRect(hwndConsole, &rcOldPos);
			// В много-мониторных конфигурациях координаты на некоторых могут быть отрицательными!
			rcAllMonRect = GetAllMonitorsWorkspace();
			pci->AutoPosition = FALSE;
			pci->WindowPosX = rcAllMonRect.left - 1280;
			pci->WindowPosY = rcAllMonRect.top - 1024;
		}

		memcpy(ptrView, pci, pci->Length); //-V106
		UnmapViewOfFile(ptrView);

		//  Send console window the "update" message
		DEBUGTEST(LRESULT dwConInfoRc =)
		SendMessage(hwndConsole, WM_SETCONSOLEINFO, (WPARAM)ghConsoleSection, 0);

		DEBUGTEST(DWORD dwConInfoErr = GetLastError());

		if (!lbWasVisible && IsWindowVisible(hwndConsole))
		{
			//DEBUGTEST(Sleep(10));

			apiShowWindow(hwndConsole, SW_HIDE);
			//SetWindowPos(hwndConsole, NULL, rcOldPos.left, rcOldPos.top, 0,0, SWP_NOSIZE|SWP_NOZORDER);
			// -- чтобы на некоторых системах не возникала проблема с позиционированием -> {0,0}
			// Issue 274: Окно реальной консоли позиционируется в неудобном месте
			SetWindowPos(hwndConsole, NULL, 0, 0, 0,0, SWP_NOSIZE|SWP_NOZORDER);
		}
	}

	return TRUE;
}
#endif

#ifndef CONEMU_MINIMAL
//
//	Fill the CONSOLE_INFO structure with information
//  about the current console window
//
void GetConsoleSizeInfo(CONSOLE_INFO *pci)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	HANDLE hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	BOOL lbRc = GetConsoleScreenBufferInfo(hConsoleOut, &csbi);

	if (lbRc)
	{
		pci->ScreenBufferSize = csbi.dwSize;
		pci->WindowSize.X	  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		pci->WindowSize.Y	  = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
		// Было... а это координаты окна (хотя включается флажок "Autoposition"
		//pci->WindowPosX	      = csbi.srWindow.Left;
		//pci->WindowPosY		  = csbi.srWindow.Top;
	}
	else
	{
		_ASSERTE(lbRc);
		pci->ScreenBufferSize.X = pci->WindowSize.X = 80;
		pci->ScreenBufferSize.Y = pci->WindowSize.Y = 25;
	}

	// Поскольку включен флажок "AutoPosition" - то это игнорируется
	pci->WindowPosX = pci->WindowPosY = 0;
	/*
	RECT rcWnd = {0}; GetWindowRect(Get ConsoleWindow(), &rcWnd);
	pci->WindowPosX	      = rcWnd.left;
	pci->WindowPosY		  = rcWnd.top;
	*/
}
#endif



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
// 2. После ее вызова окно консоли безусловно показыватся
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
	DEBUGTEST(BOOL lbWnd = FALSE);

	if (SetConsoleScreenBufferInfoEx_f)
	{
		lbRc = SetConsoleScreenBufferInfoEx_f(hConsoleOutput, lpConsoleScreenBufferInfoEx);

		// Win7 x64 - глюк. после вызова этой функции идет срыв размеров видимой области.
		DEBUGTEST(lbWnd =)
		SetConsoleWindowInfo(hConsoleOutput, TRUE, &lpConsoleScreenBufferInfoEx->srWindow);
	}

	return lbRc;
}

typedef BOOL (WINAPI *PSetCurrentConsoleFontEx)(HANDLE /*hConsoleOutput*/, BOOL /*bMaximumWindow*/, MY_CONSOLE_FONT_INFOEX* /*lpConsoleCurrentFontEx*/);
typedef BOOL (WINAPI *PGetCurrentConsoleFontEx)(HANDLE /*hConsoleOutput*/, BOOL /*bMaximumWindow*/, MY_CONSOLE_FONT_INFOEX* /*lpConsoleCurrentFontEx*/);

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

	//typedef BOOL (WINAPI *PGetCurrentConsoleFontEx)(HANDLE /*hConsoleOutput*/, BOOL /*bMaximumWindow*/, MY_CONSOLE_FONT_INFOEX* /*lpConsoleCurrentFontEx*/);
	PGetCurrentConsoleFontEx GetCurrentConsoleFontEx = (PGetCurrentConsoleFontEx)
	        GetProcAddress(hKernel, "GetCurrentConsoleFontEx");

	if (GetCurrentConsoleFontEx)  // We have Vista
	{
		MY_CONSOLE_FONT_INFOEX cfi = {sizeof(cfi)};
		lbRc = GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
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

	//typedef BOOL (WINAPI *PSetCurrentConsoleFontEx)(HANDLE /*hConsoleOutput*/, BOOL /*bMaximumWindow*/, MY_CONSOLE_FONT_INFOEX* /*lpConsoleCurrentFontEx*/);
	PSetCurrentConsoleFontEx SetCurrentConsoleFontEx = (PSetCurrentConsoleFontEx)
	        GetProcAddress(hKernel, "SetCurrentConsoleFontEx");

	//typedef BOOL (WINAPI *PGetCurrentConsoleFontEx)(HANDLE /*hConsoleOutput*/, BOOL /*bMaximumWindow*/, MY_CONSOLE_FONT_INFOEX* /*lpConsoleCurrentFontEx*/);
	PGetCurrentConsoleFontEx GetCurrentConsoleFontEx = (PGetCurrentConsoleFontEx)
	        GetProcAddress(hKernel, "GetCurrentConsoleFontEx");

	if (SetCurrentConsoleFontEx)  // We have Vista
	{
		MY_CONSOLE_FONT_INFOEX cfi = {sizeof(cfi)};
		//GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
		cfi.dwFontSize.X = inSizeX;
		cfi.dwFontSize.Y = inSizeY;
		lstrcpynW(cfi.FaceName, asFontName ? asFontName : L"Lucida Console", countof(cfi.FaceName));

		HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
		lbRc = SetCurrentConsoleFontEx(hConOut, FALSE, &cfi);

		// Store for further comparison
		if (lbRc)
		{
			MY_CONSOLE_FONT_INFOEX cfiSet = {sizeof(cfiSet)};
			if (GetCurrentConsoleFontEx && GetCurrentConsoleFontEx(hConOut, FALSE, &cfiSet))
			{
				// Win10 can't set "Lucida Console 3x5" and we get "4x6"
				_ASSERTE(_abs(cfiSet.dwFontSize.X-cfi.dwFontSize.X)<=1 && _abs(cfiSet.dwFontSize.Y-cfi.dwFontSize.Y)<=1);
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
				newSizeY = max(1,(nMaxY / (dwSize.Y+1)));
				newSizeX = max(1,(nMaxX / (dwSize.X+1)));
				if ((newSizeY < curSizeY) || (newSizeX < curSizeX))
				{
					calcSizeX = newSizeY * curSizeX / curSizeY;
					calcSizeY = newSizeX * curSizeY / curSizeX;
					if (calcSizeY < curSizeY)
						calcSizeX = min(calcSizeX,(calcSizeY * curSizeX / curSizeY));
					if (calcSizeX < curSizeX)
						calcSizeY = min(calcSizeY,(calcSizeX * curSizeY / curSizeX));

					newSizeY = max(1,min(calcSizeY,curSizeY));
					newSizeX = max(1,min(calcSizeX,curSizeX));
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
							calcSizeX = max(calcSizeX,(calcSizeY * curSizeX / curSizeY));
						if (calcSizeX > curSizeX)
							calcSizeY = max(calcSizeY,(calcSizeX * curSizeY / curSizeX));

						newSizeY = max(calcSizeY,curSizeY);
						newSizeX = max(calcSizeX,curSizeX);
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



#ifndef CONEMU_MINIMAL
void SetConsoleFontSizeTo(HWND inConWnd, int inSizeY, int inSizeX, const wchar_t *asFontName, WORD anTextColors /*= 0*/, WORD anPopupColors /*= 0*/)
{
	_ASSERTE(_WIN32_WINNT_VISTA==0x600);
	OSVERSIONINFOEXW osvi = {sizeof(osvi), HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA)};
	DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);
	if (VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask))
	{
		// We have Vista
		apiSetConsoleFontSize(GetStdHandle(STD_OUTPUT_HANDLE), inSizeY, inSizeX, asFontName);
	}
	else 
	{
		// We have older NT (Win2k, WinXP)
		// So... using undocumented hack

		const COLORREF DefaultColors[16] =
		{
			0x00000000, 0x00800000, 0x00008000, 0x00808000,
			0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
			0x00808080,	0x00ff0000, 0x0000ff00, 0x00ffff00,
			0x000000ff, 0x00ff00ff,	0x0000ffff, 0x00ffffff
		};

		if (!gpConsoleInfoStr)
		{
			gpConsoleInfoStr = (CONSOLE_INFO*)LocalAlloc(LPTR, sizeof(CONSOLE_INFO));

			if (!gpConsoleInfoStr)
			{
				_ASSERTE(gpConsoleInfoStr!=NULL);
				return; // memory allocation failed
			}

			gpConsoleInfoStr->Length = sizeof(CONSOLE_INFO);
		}

		if (!anTextColors)
		{
			CONSOLE_SCREEN_BUFFER_INFO csbi = {};
			GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
			anTextColors = csbi.wAttributes ? csbi.wAttributes : MAKEWORD(0x7, 0x0);
		}
		if (!anPopupColors)
		{
			anPopupColors = MAKEWORD(0x5, 0xf);
		}

		// get current size/position settings rather than using defaults..
		GetConsoleSizeInfo(gpConsoleInfoStr);
		// set these to zero to keep current settings
		gpConsoleInfoStr->FontSize.X				= inSizeX;
		gpConsoleInfoStr->FontSize.Y				= inSizeY;
		gpConsoleInfoStr->FontFamily				= 0;//0x30;//FF_MODERN|FIXED_PITCH;//0x30;
		gpConsoleInfoStr->FontWeight				= 0;//0x400;
		lstrcpynW(gpConsoleInfoStr->FaceName, asFontName ? asFontName : L"Lucida Console", countof(gpConsoleInfoStr->FaceName)); //-V303
		gpConsoleInfoStr->CursorSize				= 25;
		gpConsoleInfoStr->FullScreen				= FALSE;
		gpConsoleInfoStr->QuickEdit					= FALSE;
		//gpConsoleInfoStr->AutoPosition			= 0x10000;
		gpConsoleInfoStr->AutoPosition				= FALSE;
		RECT rcCon; GetWindowRect(inConWnd, &rcCon);
		gpConsoleInfoStr->WindowPosX = rcCon.left;
		gpConsoleInfoStr->WindowPosY = rcCon.top;
		gpConsoleInfoStr->InsertMode				= TRUE;
		gpConsoleInfoStr->ScreenColors				= anTextColors; //MAKEWORD(0x7, 0x0);
		gpConsoleInfoStr->PopupColors				= anPopupColors; //MAKEWORD(0x5, 0xf);
		gpConsoleInfoStr->HistoryNoDup				= FALSE;
		gpConsoleInfoStr->HistoryBufferSize			= 50;
		gpConsoleInfoStr->NumberOfHistoryBuffers	= 32; //-V112

		// Issue 700: Default history buffers count too small.
		HKEY hkConsole = NULL;
		LONG lRegRc;
		if (0 == (lRegRc = RegCreateKeyEx(HKEY_CURRENT_USER, L"Console\\ConEmu", 0, NULL, 0, KEY_READ, NULL, &hkConsole, NULL)))
		{
			DWORD nSize = sizeof(DWORD), nValue, nType;
			struct {
				LPCWSTR pszName;
				DWORD nMin, nMax;
				ULONG *pnVal;
			} BufferValues[] = {
				{L"HistoryBufferSize", 16, 999, &gpConsoleInfoStr->HistoryBufferSize},
				{L"NumberOfHistoryBuffers", 16, 999, &gpConsoleInfoStr->NumberOfHistoryBuffers}
			};
			for (size_t i = 0; i < countof(BufferValues); ++i)
			{
				lRegRc = RegQueryValueEx(hkConsole, BufferValues[i].pszName, NULL, &nType, (LPBYTE)&nValue, &nSize);
				if ((lRegRc == 0) && (nType == REG_DWORD) && (nSize == sizeof(DWORD)))
				{
					if (nValue < BufferValues[i].nMin)
						nValue = BufferValues[i].nMin;
					else if (nValue > BufferValues[i].nMax)
						nValue = BufferValues[i].nMax;

					if (nValue != *BufferValues[i].pnVal)
						*BufferValues[i].pnVal = nValue;
				}
			}
		}

		// color table
		for(size_t i = 0; i < 16; i++)
			gpConsoleInfoStr->ColorTable[i] = DefaultColors[i];

		gpConsoleInfoStr->CodePage					= GetConsoleOutputCP();//0;//0x352;
		gpConsoleInfoStr->Hwnd						= inConWnd;
		gpConsoleInfoStr->ConsoleTitle[0] = 0;

		// Send data to console window
		SetConsoleInfo(inConWnd, gpConsoleInfoStr);
	}
}
#endif
/*
-- пробовал в Win7 это не работает
void SetConsoleBufferSize(HWND inConWnd, int anWidth, int anHeight, int anBufferHeight)
{
	if (!gpConsoleInfoStr) {
		_ASSERTE(gpConsoleInfoStr!=NULL);
		return; // memory allocation failed
	}

	TODO("Заполнить и другие текущие значения!");
	gpConsoleInfoStr->CodePage					= GetConsoleOutputCP();//0;//0x352;

	// Теперь собственно, что хотим поменять
	gpConsoleInfoStr->ScreenBufferSize.X = gpConsoleInfoStr->WindowSize.X = anWidth;
	gpConsoleInfoStr->WindowSize.Y = anHeight;
	gpConsoleInfoStr->ScreenBufferSize.Y = anBufferHeight;

	SetConsoleInfo(inConWnd, gpConsoleInfoStr);
}
*/

COORD MyGetLargestConsoleWindowSize(HANDLE hConsoleOutput)
{
	// Fails in Wine
	COORD crMax = hkFunc.getLargestConsoleWindowSize(hConsoleOutput);
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
		//crMax.X = max(crMax.X,555);
		//crMax.Y = max(crMax.Y,555);
	}
	#endif

	return crMax;
}
