//
//	SETCONSOLEINFO.C
//
//	Undocumented method to set console attributes
//  at runtime including console palette (NT4, 2000, XP).
//
//	VOID WINAPI SetConsolePalette(COLORREF palette[16])
//
//	For Vista use the newly documented SetConsoleScreenBufferEx API
//
//	www.catch22.net
//
#include "Header.h"
//#include <stdio.h>
#include <stdarg.h>

// only in Win2k+  (use FindWindow for NT4)
//HWND WINAPI GetConsoleWindow();

// Undocumented console message
#define WM_SETCONSOLEINFO			(WM_USER+201)

#pragma pack(push, 1)

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

#pragma pack(pop)

//
//	Wrapper around WM_SETCONSOLEINFO. We need to create the
//  necessary section (file-mapping) object in the context of the
//  process which owns the console, before posting the message
//
BOOL SetConsoleInfo(HWND hwndConsole, CONSOLE_INFO *pci)
{
	DWORD   dwConsoleOwnerPid, dwCurProcId;
	HANDLE  hProcess=NULL;
	HANDLE	hSection=NULL, hDupSection=NULL;
	PVOID   ptrView = 0;
	DWORD   dwLastError=0;
	WCHAR   ErrText[255];
	
	//
	//	Open the process which "owns" the console
	//	
	dwCurProcId = GetCurrentProcessId();
	GetWindowThreadProcessId(hwndConsole, &dwConsoleOwnerPid);
	
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwConsoleOwnerPid);
	dwLastError = GetLastError();
	//Какого черта OpenProcess не дает себя открыть? GetLastError возвращает 5
	if (hProcess==NULL) {
		if (dwConsoleOwnerPid == dwCurProcId) {
			hProcess = GetCurrentProcess();
		} else {
			wsprintf(ErrText, L"Can't open console process. ErrCode=%i", dwLastError);
			MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
			return FALSE;
		}
	}

	//
	// Create a SECTION object backed by page-file, then map a view of
	// this section into the owner process so we can write the contents 
	// of the CONSOLE_INFO buffer into it
	//
	hSection = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, pci->Length, 0);
	if (!hSection) {
		dwLastError = GetLastError();
		wsprintf(ErrText, L"Can't CreateFileMapping. ErrCode=%i", dwLastError);
		MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);

	} else {
		//
		//	Copy our console structure into the section-object
		//
		ptrView = MapViewOfFile(hSection, FILE_MAP_WRITE|FILE_MAP_READ, 0, 0, pci->Length);
		if (!ptrView) {
			dwLastError = GetLastError();
			wsprintf(ErrText, L"Can't MapViewOfFile. ErrCode=%i", dwLastError);
			MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);

		} else {
			memcpy(ptrView, pci, pci->Length);

			UnmapViewOfFile(ptrView);

			//
			//	Map the memory into owner process
			//
			if (!DuplicateHandle(GetCurrentProcess(), hSection, hProcess, &hDupSection, 0, FALSE, DUPLICATE_SAME_ACCESS)
				|| !hDupSection)
			{
				dwLastError = GetLastError();
				wsprintf(ErrText, L"Can't DuplicateHandle. ErrCode=%i", dwLastError);
				MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);

			} else {
				//  Send console window the "update" message
				DWORD dwConInfoRc = SendMessage(hwndConsole, WM_SETCONSOLEINFO, (WPARAM)hDupSection, 0);
				DWORD dwConInfoErr = GetLastError();
			}
		}
	}



	//
	// clean up
	//

	if (hDupSection) {
		if (dwConsoleOwnerPid == dwCurProcId) {
			// Если это наша консоль - зачем с нитями извращаться
			CloseHandle(hDupSection);
		} else {
			HANDLE  hThread=NULL;
			hThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)CloseHandle, hDupSection, 0, 0);
			if (hThread) CloseHandle(hThread);
		}
	}

	if (hSection)
		CloseHandle(hSection);
	if ((dwConsoleOwnerPid!=dwCurProcId) && hProcess)
		CloseHandle(hProcess);

	return TRUE;
}

//
//	Fill the CONSOLE_INFO structure with information
//  about the current console window
//
static void GetConsoleSizeInfo(CONSOLE_INFO *pci)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	HANDLE hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);

	GetConsoleScreenBufferInfo(hConsoleOut, &csbi);

	pci->ScreenBufferSize = csbi.dwSize;
	pci->WindowSize.X	  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	pci->WindowSize.Y	  = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	pci->WindowPosX	      = csbi.srWindow.Left;
	pci->WindowPosY		  = csbi.srWindow.Top;
}

//
// Set palette of current console
//
//	palette should be of the form:
//
// COLORREF DefaultColors[16] = 
// {
//	0x00000000, 0x00800000, 0x00008000, 0x00808000,
//	0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0, 
//	0x00808080,	0x00ff0000, 0x0000ff00, 0x00ffff00,
//	0x000000ff, 0x00ff00ff,	0x0000ffff, 0x00ffffff
// };
//
VOID WINAPI SetConsolePalette(COLORREF palette[16])
{
	CONSOLE_INFO ci = { sizeof(ci) };
	int i;
        HWND hwndConsole = GetConsoleWindow();

	// get current size/position settings rather than using defaults..
	GetConsoleSizeInfo(&ci);

	// set these to zero to keep current settings
	ci.FontSize.X				= 0;//8;
	ci.FontSize.Y				= 0;//12;
	ci.FontFamily				= 0;//0x30;//FF_MODERN|FIXED_PITCH;//0x30;
	ci.FontWeight				= 0;//0x400;
	//lstrcpyW(ci.FaceName, L"Terminal");
	ci.FaceName[0]				= L'\0';

	ci.CursorSize				= 25;
	ci.FullScreen				= FALSE;
	ci.QuickEdit				= TRUE;
	ci.AutoPosition				= 0x10000;
	ci.InsertMode				= TRUE;
	ci.ScreenColors				= MAKEWORD(0x7, 0x0);
	ci.PopupColors				= MAKEWORD(0x5, 0xf);
	
	ci.HistoryNoDup				= FALSE;
	ci.HistoryBufferSize		= 50;
	ci.NumberOfHistoryBuffers	= 4;

	// colour table
	for(i = 0; i < 16; i++)
		ci.ColorTable[i] = palette[i];

	ci.CodePage					= GetConsoleOutputCP();//0;//0x352;
	ci.Hwnd						= hwndConsole;

	lstrcpyW(ci.ConsoleTitle, L"");

	SetConsoleInfo(hwndConsole, &ci);
}

//VISTA support:
#ifndef ENABLE_AUTO_POSITION
typedef struct _CONSOLE_FONT_INFOEX {
	ULONG cbSize;
	DWORD nFont;
	COORD dwFontSize;
	UINT FontFamily;
	UINT FontWeight;
	WCHAR FaceName[LF_FACESIZE];
} CONSOLE_FONT_INFOEX, *PCONSOLE_FONT_INFOEX;
#endif

typedef BOOL (WINAPI *PGetCurrentConsoleFontEx)(__in HANDLE hConsoleOutput,__in BOOL bMaximumWindow,__out PCONSOLE_FONT_INFOEX lpConsoleCurrentFontEx);
typedef BOOL (WINAPI *PSetCurrentConsoleFontEx)(__in HANDLE hConsoleOutput,__in BOOL bMaximumWindow,__out PCONSOLE_FONT_INFOEX lpConsoleCurrentFontEx);


void SetConsoleFontSizeTo(HWND inConWnd, int inSizeX, int inSizeY)
{
	PGetCurrentConsoleFontEx GetCurrentConsoleFontEx = (PGetCurrentConsoleFontEx)
		GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "GetCurrentConsoleFontEx");
	PSetCurrentConsoleFontEx SetCurrentConsoleFontEx = (PSetCurrentConsoleFontEx)
		GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "SetCurrentConsoleFontEx");

	if (GetCurrentConsoleFontEx && SetCurrentConsoleFontEx) // We have Vista
	{
		CONSOLE_FONT_INFOEX cfi = {sizeof(CONSOLE_FONT_INFOEX)};
		GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
		cfi.dwFontSize.X = inSizeX;
		cfi.dwFontSize.Y = inSizeY;
		SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
	}
	else // We have other NT
	{
		const COLORREF DefaultColors[16] = 
		{
			0x00000000, 0x00800000, 0x00008000, 0x00808000,
			0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0, 
			0x00808080,	0x00ff0000, 0x0000ff00, 0x00ffff00,
			0x000000ff, 0x00ff00ff,	0x0000ffff, 0x00ffffff
		};

		CONSOLE_INFO ci = { sizeof(ci) };
		int i;

		// get current size/position settings rather than using defaults..
		GetConsoleSizeInfo(&ci);

		// set these to zero to keep current settings
		ci.FontSize.X				= inSizeX;
		ci.FontSize.Y				= inSizeY;
		ci.FontFamily				= 0;//0x30;//FF_MODERN|FIXED_PITCH;//0x30;
		ci.FontWeight				= 0;//0x400;
		_tcscpy(ci.FaceName, _T("Lucida Console"));

		ci.CursorSize				= 25;
		ci.FullScreen				= FALSE;
		ci.QuickEdit				= FALSE;
		ci.AutoPosition				= 0x10000;
		ci.InsertMode				= TRUE;
		ci.ScreenColors				= MAKEWORD(0x7, 0x0);
		ci.PopupColors				= MAKEWORD(0x5, 0xf);

		ci.HistoryNoDup				= FALSE;
		ci.HistoryBufferSize		= 50;
		ci.NumberOfHistoryBuffers	= 4;

		// color table
		for(i = 0; i < 16; i++)
			ci.ColorTable[i] = DefaultColors[i];

		ci.CodePage					= GetConsoleOutputCP();//0;//0x352;
		ci.Hwnd						= inConWnd;

		*ci.ConsoleTitle = NULL;

		//!!! чего-то не работает... а в
        // F:\VCProject\FarPlugin\ConEmu\080703\ConEmu\setconsoleinfo.cpp
		//вроде ок
		SetConsoleInfo(inConWnd, &ci);
	}
}