//
//	SETCONSOLEINFO.C
//
//	Undocumented method to set console attributes at runtime
//
//	For Vista use the newly documented SetConsoleScreenBufferEx API
//
//	www.catch22.net
//

#include <windows.h>
#include <stdarg.h>
#include "..\common\common.hpp"


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
	DWORD   dwConsoleOwnerPid, dwCurProcId, dwConsoleThreadId, dwMapSize;
	HANDLE	hSection=NULL;
	PVOID   ptrView = 0;
	DWORD   dwLastError=0;
	WCHAR   ErrText[255];
	
	//
	//	Retrieve the process which "owns" the console
	//	
	dwCurProcId = GetCurrentProcessId();
	dwConsoleThreadId = GetWindowThreadProcessId(hwndConsole, &dwConsoleOwnerPid);

	// We'll fail, if console was created by other process
	if (dwConsoleOwnerPid != dwCurProcId) {
		_ASSERTE(dwConsoleOwnerPid == dwCurProcId);
		return FALSE;
	}
	

	//
	// Create a SECTION object backed by page-file, then map a view of
	// this section into the owner process so we can write the contents 
	// of the CONSOLE_INFO buffer into it
	//
	dwMapSize = pci->Length + 1024;
	hSection = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, dwMapSize, 0);
	if (!hSection) {
		dwLastError = GetLastError();
		wsprintf(ErrText, L"Can't CreateFileMapping. ErrCode=%i", dwLastError);
		MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);

	} else {
		//
		//	Copy our console structure into the section-object
		//
		ptrView = MapViewOfFile(hSection, FILE_MAP_WRITE|FILE_MAP_READ, 0, 0, dwMapSize);
		if (!ptrView) {
			dwLastError = GetLastError();
			wsprintf(ErrText, L"Can't MapViewOfFile. ErrCode=%i", dwLastError);
			MessageBox(NULL, ErrText, L"ConEmu", MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);

		} else {
			memcpy(ptrView, pci, pci->Length);

			UnmapViewOfFile(ptrView);

			//  Send console window the "update" message
			LRESULT dwConInfoRc = 0;
			DWORD dwConInfoErr = 0;
			
			dwConInfoRc = SendMessage(hwndConsole, WM_SETCONSOLEINFO, (WPARAM)hSection, 0);
			dwConInfoErr = GetLastError();
		}
	}

	// Clean memory
	if (hSection)
		CloseHandle(hSection);

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

	GetConsoleScreenBufferInfo(hConsoleOut, &csbi);

	pci->ScreenBufferSize = csbi.dwSize;
	pci->WindowSize.X	  = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	pci->WindowSize.Y	  = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	pci->WindowPosX	      = csbi.srWindow.Left;
	pci->WindowPosY		  = csbi.srWindow.Top;
}


#if defined(__GNUC__)
#define __in
#define __out
#undef ENABLE_AUTO_POSITION
#endif

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


void SetConsoleFontSizeTo(HWND inConWnd, int inSizeY, int inSizeX, wchar_t *asFontName)
{
	HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
	if (!hKernel)
		return;
	PGetCurrentConsoleFontEx GetCurrentConsoleFontEx = (PGetCurrentConsoleFontEx)
		GetProcAddress(hKernel, "GetCurrentConsoleFontEx");
	PSetCurrentConsoleFontEx SetCurrentConsoleFontEx = (PSetCurrentConsoleFontEx)
		GetProcAddress(hKernel, "SetCurrentConsoleFontEx");

	if (GetCurrentConsoleFontEx && SetCurrentConsoleFontEx) // We have Vista
	{
		CONSOLE_FONT_INFOEX cfi = {sizeof(CONSOLE_FONT_INFOEX)};
		//GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
		cfi.dwFontSize.X = inSizeX;
		cfi.dwFontSize.Y = inSizeY;
		lstrcpynW(cfi.FaceName, asFontName ? asFontName : L"Lucida Console", LF_FACESIZE);
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
		lstrcpynW(ci.FaceName, asFontName ? asFontName : L"Lucida Console", 32);

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

		SetConsoleInfo(inConWnd, &ci);
	}
}
