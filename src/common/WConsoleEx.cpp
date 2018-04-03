
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
#include "WUser.h"

// Undocumented console message
#define WM_SETCONSOLEINFO			(WM_USER+201)

//#pragma pack(push, 1)
#include <pshpack1.h>

//
//	Structure to send console via WM_SETCONSOLEINFO
//
struct CONSOLE_INFO
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

};


// Do not use any heap functions in this object!
// Its dtor called when heap is already deinitialized!
struct ConsoleSectionHelper
{
	CONSOLE_INFO* pConsoleInfo;
	HANDLE hSection;
	static const UINT ConsoleSectionSize = sizeof(CONSOLE_INFO)+1024;

	ConsoleSectionHelper()
		: pConsoleInfo(NULL)
		, hSection(NULL)
	{
	};

	CONSOLE_INFO* CreateConsoleData()
	{
		// Must not be used in Vista+, these OS have appropriate console API,
		// hacking is required in Win2k and WinXP only!
		_ASSERTE(!IsWinVerOrHigher(0x600));
		// Must be statically initialized by compiler
		_ASSERTE(ConsoleSectionSize == (sizeof(CONSOLE_INFO)+1024));
		// Allocate buffer
		if (!pConsoleInfo)
		{
			pConsoleInfo = (CONSOLE_INFO*)LocalAlloc(LPTR, sizeof(CONSOLE_INFO));
			if (pConsoleInfo)
			{
				pConsoleInfo->Length = sizeof(CONSOLE_INFO);
			}
		}
		return pConsoleInfo;
	}

	HANDLE CreateConsoleSection()
	{
		// Must not be used in Vista+, these OS have appropriate console API,
		// hacking is required in Win2k and WinXP only!
		_ASSERTE(!IsWinVerOrHigher(0x600));
		// Must be statically initialized by compiler
		_ASSERTE(ConsoleSectionSize == (sizeof(CONSOLE_INFO)+1024));
		// Create memory object, if it was not created yet
		if (!hSection)
		{
			hSection = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, ConsoleSectionSize, 0);
		}
		return hSection;
	}

	~ConsoleSectionHelper()
	{
		if (hSection)
		{
			CloseHandle(hSection);
			hSection = NULL;
		}

		if (pConsoleInfo)
		{
			LocalFree(pConsoleInfo);
			pConsoleInfo = NULL;
		}
	};
};
// gh-515 noted a crash, from OnShutdownConsole, so it was changed to ctor/dtor
ConsoleSectionHelper gsMapHelper;


//#pragma pack(pop)
#include <poppack.h>


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
	if (!gsMapHelper.CreateConsoleSection())
	{
		dwLastError = GetLastError();
		swprintf_c(ErrText, L"Can't CreateFileMapping(hSection). ErrCode=%i", dwLastError);
		MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
		return FALSE;
	}

	//
	//	Copy our console structure into the section-object
	//
	ptrView = MapViewOfFile(gsMapHelper.hSection, FILE_MAP_WRITE|FILE_MAP_READ, 0, 0, gsMapHelper.ConsoleSectionSize);

	if (!ptrView)
	{
		dwLastError = GetLastError();
		swprintf_c(ErrText, L"Can't MapViewOfFile. ErrCode=%i", dwLastError);
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
		SendMessage(hwndConsole, WM_SETCONSOLEINFO, (WPARAM)gsMapHelper.hSection, 0);

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


void SetConsoleFontSizeTo(HWND inConWnd, int inSizeY, int inSizeX, const wchar_t *asFontName, WORD anTextColors /*= 0*/, WORD anPopupColors /*= 0*/)
{
	if (IsWin6())
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

		if (!gsMapHelper.CreateConsoleData())
		{
			_ASSERTE(gsMapHelper.pConsoleInfo!=NULL);
			return; // memory allocation failed
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
		GetConsoleSizeInfo(gsMapHelper.pConsoleInfo);
		// set these to zero to keep current settings
		gsMapHelper.pConsoleInfo->FontSize.X				= inSizeX;
		gsMapHelper.pConsoleInfo->FontSize.Y				= inSizeY;
		gsMapHelper.pConsoleInfo->FontFamily				= 0;//0x30;//FF_MODERN|FIXED_PITCH;//0x30;
		gsMapHelper.pConsoleInfo->FontWeight				= 0;//0x400;
		lstrcpynW(gsMapHelper.pConsoleInfo->FaceName, asFontName ? asFontName : L"Lucida Console", countof(gsMapHelper.pConsoleInfo->FaceName)); //-V303
		gsMapHelper.pConsoleInfo->CursorSize				= 25;
		gsMapHelper.pConsoleInfo->FullScreen				= FALSE;
		gsMapHelper.pConsoleInfo->QuickEdit					= FALSE;
		gsMapHelper.pConsoleInfo->AutoPosition				= FALSE; // 0x10000;
		RECT rcCon; GetWindowRect(inConWnd, &rcCon);
		gsMapHelper.pConsoleInfo->WindowPosX = rcCon.left;
		gsMapHelper.pConsoleInfo->WindowPosY = rcCon.top;
		gsMapHelper.pConsoleInfo->InsertMode				= TRUE;
		gsMapHelper.pConsoleInfo->ScreenColors				= anTextColors; //MAKEWORD(0x7, 0x0);
		gsMapHelper.pConsoleInfo->PopupColors				= anPopupColors; //MAKEWORD(0x5, 0xf);
		gsMapHelper.pConsoleInfo->HistoryNoDup				= FALSE;
		gsMapHelper.pConsoleInfo->HistoryBufferSize			= 50;
		gsMapHelper.pConsoleInfo->NumberOfHistoryBuffers	= 32; //-V112

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
				{L"HistoryBufferSize", 16, 999, &gsMapHelper.pConsoleInfo->HistoryBufferSize},
				{L"NumberOfHistoryBuffers", 16, 999, &gsMapHelper.pConsoleInfo->NumberOfHistoryBuffers}
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
			gsMapHelper.pConsoleInfo->ColorTable[i] = DefaultColors[i];

		gsMapHelper.pConsoleInfo->CodePage					= GetConsoleOutputCP();//0;//0x352;
		gsMapHelper.pConsoleInfo->Hwnd						= inConWnd;
		gsMapHelper.pConsoleInfo->ConsoleTitle[0] = 0;

		// Send data to console window
		SetConsoleInfo(inConWnd, gsMapHelper.pConsoleInfo);
	}
}
