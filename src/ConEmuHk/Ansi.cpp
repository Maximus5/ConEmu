
/*
Copyright (c) 2012-2015 Maximus5
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

#define DEFINE_HOOK_MACROS


#include <windows.h>
#include <WinError.h>
#include <WinNT.h>
#include <TCHAR.h>
#include "../common/common.hpp"
#include "../common/ConEmuCheck.h"
#include "../common/CmdLine.h"
#include "SetHook.h"
#include "ConEmuHooks.h"
#include "../common/ConsoleAnnotation.h"
#include "ExtConsole.h"
#include "../common/MConHandle.h"
#include "../common/MSectionSimple.h"
#include "../common/WConsole.h"
#include "../ConEmu/version.h"

///* ***************** */
#include "Ansi.h"
DWORD AnsiTlsIndex = 0;
///* ***************** */

#undef isPressed
#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)

#define ANSI_MAP_CHECK_TIMEOUT 1000

#ifdef _DEBUG
#define DebugString(x) OutputDebugString(x)
#else
#define DebugString(x) //OutputDebugString(x)
#endif

/* ************ Globals ************ */
extern HMODULE ghOurModule; // Хэндл нашей dll'ки (здесь хуки не ставятся)
extern BOOL    gbHooksTemporaryDisabled;
#include "MainThread.h"

/* ************ Globals for SetHook ************ */
extern HWND    ghConWnd;      // RealConsole
extern HWND    ghConEmuWnd;   // Root! ConEmu window
extern HWND    ghConEmuWndDC; // ConEmu DC window
extern DWORD   gnGuiPID;
extern GetConsoleWindow_T gfGetRealConsoleWindow;
//extern HWND WINAPI GetRealConsoleWindow(); // Entry.cpp
extern HANDLE ghCurrentOutBuffer;
extern HANDLE ghStdOutHandle;
extern wchar_t gsInitConTitle[512];
/* ************ Globals for SetHook ************ */

/* ************ Globals for ViM ************ */
static int  gnVimTermWasChangedBuffer = 0;
static bool gbVimTermWasChangedBuffer = false;
//void StartVimTerm(bool bFromDllStart);
//void StopVimTerm();
/* ************ Globals for ViM ************ */

#ifdef _DEBUG
	// Forward (external in Entry.cpp)
	void FIRST_ANSI_CALL(const BYTE* lpBuf, DWORD nNumberOfBytes);
#else
	// Dummy macro
	#define FIRST_ANSI_CALL(lpBuf,nNumberOfBytes)
#endif

BOOL WINAPI OnCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

// These handles must be registered and released in OnCloseHandle.
HANDLE CEAnsi::ghLastAnsiCapable = NULL;
HANDLE CEAnsi::ghLastAnsiNotCapable = NULL;
HANDLE CEAnsi::ghLastConOut = NULL;
HANDLE CEAnsi::ghAnsiLogFile = NULL;
MSectionSimple* CEAnsi::gcsAnsiLogFile = NULL;

// VIM, etc. Some programs waiting control keys as xterm sequences. Need to inform ConEmu GUI.
bool CEAnsi::gbWasXTermOutput = false;

/* ************ Export ANSI printings ************ */
LONG gnWriteProcessed = 0;
BOOL WINAPI WriteProcessed(LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten)
{
	HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	InterlockedIncrement(&gnWriteProcessed);
	BOOL bRc = CEAnsi::OurWriteConsoleW(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, NULL);
	InterlockedDecrement(&gnWriteProcessed);
	return bRc;
}
/* ************ Export ANSI printings ************ */

void CEAnsi::InitAnsiLog(LPCWSTR asFilePath)
{
	// Already initialized?
	if (ghAnsiLogFile)
		return;

	gcsAnsiLogFile = new MSectionSimple(true);
	HANDLE hLog = CreateFile(asFilePath, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hLog && (hLog != INVALID_HANDLE_VALUE))
	{
		// Succeeded
		ghAnsiLogFile = hLog;
	}
	else
	{
		SafeDelete(gcsAnsiLogFile);
	}
}

void CEAnsi::DoneAnsiLog(bool bFinal)
{
	if (!ghAnsiLogFile)
		return;
	if (!bFinal)
	{
		FlushFileBuffers(ghAnsiLogFile);
	}
	else
	{
		HANDLE h = ghAnsiLogFile;
		ghAnsiLogFile = NULL;
		CloseHandle(h);
		SafeDelete(gcsAnsiLogFile);
	}
}

void CEAnsi::WriteAnsiLog(LPCWSTR lpBuffer, DWORD nChars)
{
	if (!ghAnsiLogFile)
		return;
	char sBuf[80*3]; // Will be enough for most cases
	BOOL bWrite = FALSE;
	DWORD nWritten = 0;
	int nNeed = WideCharToMultiByte(CP_UTF8, 0, lpBuffer, nChars, NULL, 0, NULL, NULL);
	if (nNeed < 1)
		return;
	char* pszBuf = (nNeed <= countof(sBuf)) ? sBuf : (char*)malloc(nNeed);
	if (!pszBuf)
		return;
	int nLen = WideCharToMultiByte(CP_UTF8, 0, lpBuffer, nChars, pszBuf, nNeed, NULL, NULL);
	// Must be OK, but check it
	if (nLen > 0 && nLen <= nNeed)
	{
		// Handle multi-thread writers
		// But multi-process writers can't be handled correctly
		MSectionLockSimple lock; lock.Lock(gcsAnsiLogFile, 500);
		SetFilePointer(ghAnsiLogFile, 0, NULL, FILE_END);
		bWrite = WriteFile(ghAnsiLogFile, pszBuf, nLen, &nWritten, NULL);
	}
	if (pszBuf != sBuf)
		free(pszBuf);
	UNREFERENCED_PARAMETER(bWrite);
}

void CEAnsi::GetFeatures(bool* pbAnsiAllowed, bool* pbSuppressBells)
{
	static DWORD nLastCheck = 0;
	static bool bAnsiAllowed = true;
	static bool bSuppressBells = true;

	if (nLastCheck || ((GetTickCount() - nLastCheck) > ANSI_MAP_CHECK_TIMEOUT))
	{
		CESERVER_CONSOLE_MAPPING_HDR* pMap = GetConMap();
		//	(CESERVER_CONSOLE_MAPPING_HDR*)malloc(sizeof(CESERVER_CONSOLE_MAPPING_HDR));
		if (pMap)
		{
			//if (!::LoadSrvMapping(ghConWnd, *pMap) || !pMap->bProcessAnsi)
			//	bAnsiAllowed = false;
			//else
			//	bAnsiAllowed = true;

			bAnsiAllowed = ((pMap != NULL) && ((pMap->Flags & CECF_ProcessAnsi) != 0));
			bSuppressBells = ((pMap != NULL) && ((pMap->Flags & CECF_SuppressBells) != 0));

			//free(pMap);
		}
		nLastCheck = GetTickCount();
	}

	if (pbAnsiAllowed)
		*pbAnsiAllowed = bAnsiAllowed;
	if (pbSuppressBells)
		*pbSuppressBells = bSuppressBells;
}

bool CEAnsi::IsAnsiCapable(HANDLE hFile, bool* bIsConsoleOutput /*= NULL*/)
{
	bool bAnsi = false;
	DWORD Mode = 0;
	bool bIsOut = false;

	if (hFile == NULL)
	{
		// Проверка настроек на старте?
		bIsOut = true;
		Mode = ENABLE_PROCESSED_OUTPUT;
	}
	else
	{
		bIsOut = IsOutputHandle(hFile, &Mode);
	}

	if (bIsOut)
	{
		bAnsi = ((Mode & ENABLE_PROCESSED_OUTPUT) == ENABLE_PROCESSED_OUTPUT);
		if (!bAnsi)
		{
			#ifdef _DEBUG
			HANDLE hIn  = GetStdHandle(STD_INPUT_HANDLE);
			HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
			HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
			#endif
			// Где-то по дороге между "cmd /c test.cmd", "pause" в нем и "WriteConsole"
			// слетает ENABLE_PROCESSED_OUTPUT, поэтому пока обрабатываем всегда.
			bAnsi = true;
		}


		if (bAnsi)
		{
			bool bAnsiAllowed; GetFeatures(&bAnsiAllowed, NULL);

			if (!bAnsiAllowed)
				bAnsi = false;
		}
	}

	if (bIsConsoleOutput)
		*bIsConsoleOutput = bIsOut;

	return bAnsi;
}

bool CEAnsi::IsOutputHandle(HANDLE hFile, DWORD* pMode /*= NULL*/)
{
	if (!hFile)
		return false;

	if (hFile == ghLastConOut)
		return true;

	if (hFile == ghLastAnsiCapable)
		return true;

	if (hFile == ghLastAnsiNotCapable)
		return false;

	bool  bOk = false;
	DWORD Mode = 0, nErrCode = 0;
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};

	// GetConsoleMode не совсем подходит, т.к. он проверяет и ConIn & ConOut
	// Поэтому, добавляем
	if (GetConsoleMode(hFile, &Mode))
	{
		if (!GetConsoleScreenBufferInfoCached(hFile, &csbi, TRUE))
		{
			nErrCode = GetLastError();
			_ASSERTEX(nErrCode == ERROR_INVALID_HANDLE);
			ghLastAnsiNotCapable = hFile;
		}
		else
		{
			if (pMode)
				*pMode = Mode;
			//Processed = (Mode & ENABLE_PROCESSED_OUTPUT);
			ghLastAnsiCapable = hFile;
			bOk = true;
		}
	}
	else
	{
		ghLastAnsiNotCapable = hFile;
	}

	return bOk;
}



//BOOL WINAPI OnWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
//BOOL WINAPI OnWriteConsoleA(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);
//BOOL WINAPI OnWriteConsoleW(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);
//BOOL WINAPI OnWriteConsoleOutputCharacterA(HANDLE hConsoleOutput, LPCSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten);
//BOOL WINAPI OnWriteConsoleOutputCharacterW(HANDLE hConsoleOutput, LPCWSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten);

//typedef BOOL (WINAPI* OnWriteConsoleW_t)(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);
//BOOL WriteAnsiCodes(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten);

//struct DisplayParm
//{
//	BOOL WasSet;
//	BOOL BrightOrBold;     // 1
//	BOOL ItalicOrInverse;  // 3
//	BOOL BackOrUnderline;  // 4
//	int  TextColor;        // 30-37,38,39
//	BOOL Text256;          // 38
//    int  BackColor;        // 40-47,48,49
//    BOOL Back256;          // 48
//	// xterm
//	BOOL Inverse;
//} gDisplayParm = {};

CEAnsi::DisplayParm CEAnsi::gDisplayParm = {};

//struct DisplayCursorPos
//{
//    // Internal
//    COORD StoredCursorPos;
//	// Esc[?1h 	Set cursor key to application 	DECCKM
//	// Esc[?1l 	Set cursor key to cursor 	DECCKM
//	BOOL CursorKeysApp; // "1h"
//} gDisplayCursor = {};

CEAnsi::DisplayCursorPos CEAnsi::gDisplayCursor = {};

//struct DisplayOpt
//{
//	BOOL  WrapWasSet;
//	SHORT WrapAt; // Rightmost X coord (1-based)
//	//
//	BOOL  AutoLfNl; // LF/NL (default off): Automatically follow echo of LF, VT or FF with CR.
//	//
//	BOOL  ScrollRegion;
//	SHORT ScrollStart, ScrollEnd; // 1-based line indexes
//	//
//	BOOL  ShowRawAnsi; // \e[3h display ANSI control characters (TRUE), \e[3l process ANSI (FALSE, normal mode)
//} gDisplayOpt;

CEAnsi::DisplayOpt CEAnsi::gDisplayOpt = {};

//const size_t cchMaxPrevPart = 160;
//static wchar_t gsPrevAnsiPart[cchMaxPrevPart] = {};
//static INT_PTR gnPrevAnsiPart = 0;
//static wchar_t gsPrevAnsiPart2[cchMaxPrevPart] = {};
//static INT_PTR gnPrevAnsiPart2 = 0;
//const  INT_PTR MaxPrevAnsiPart = 80;

#ifdef _DEBUG
static const wchar_t szAnalogues[32] =
{
	32, 9786, 9787, 9829, 9830, 9827, 9824, 8226, 9688, 9675, 9689, 9794, 9792, 9834, 9835, 9788,
	9658, 9668, 8597, 8252,  182,  167, 9632, 8616, 8593, 8595, 8594, 8592, 8735, 8596, 9650, 9660
};
#endif

/*static*/ SHORT CEAnsi::GetDefaultTextAttr()
{
	// Default console colors
	static SHORT clrDefault = 0;
	if (clrDefault)
		return clrDefault;

	HANDLE hIn = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (!GetConsoleScreenBufferInfo(hIn, &csbi))
		return (clrDefault = 7);

	static SHORT Con2Ansi[16] = {0,4,2,6,1,5,3,7,8|0,8|4,8|2,8|6,8|1,8|5,8|3,8|7};
	clrDefault = Con2Ansi[CONFORECOLOR(csbi.wAttributes)]
		| (Con2Ansi[CONBACKCOLOR(csbi.wAttributes)] << 4);

	return clrDefault;
}


void CEAnsi::ReSetDisplayParm(HANDLE hConsoleOutput, BOOL bReset, BOOL bApply)
{
	WARNING("Эту функу нужно дергать при смене буферов, закрытии дескрипторов, и т.п.");

	if (bReset || !gDisplayParm.WasSet)
	{
		if (bReset)
			memset(&gDisplayParm, 0, sizeof(gDisplayParm));

		WORD nDefColors = GetDefaultTextAttr();
		gDisplayParm.TextColor = CONFORECOLOR(nDefColors);
		gDisplayParm.BackColor = CONBACKCOLOR(nDefColors);
		gDisplayParm.WasSet = TRUE;
	}

	if (bApply)
	{
		// на дисплей
		ExtAttributesParm attr = {sizeof(attr), hConsoleOutput};
		//DWORD wAttrs = 0;

		// Ansi colors
		static DWORD ClrMap[8] = {0,4,2,6,1,5,3,7};
		// XTerm-256 colors
		static DWORD RgbMap[256] = {0,4,2,6,1,5,3,7, 8+0,8+4,8+2,8+6,8+1,8+5,8+3,8+7, // System Ansi colors
			/*16*/0x000000, /*17*/0x5f0000, /*18*/0x870000, /*19*/0xaf0000, /*20*/0xd70000, /*21*/0xff0000, /*22*/0x005f00, /*23*/0x5f5f00, /*24*/0x875f00, /*25*/0xaf5f00, /*26*/0xd75f00, /*27*/0xff5f00,
			/*28*/0x008700, /*29*/0x5f8700, /*30*/0x878700, /*31*/0xaf8700, /*32*/0xd78700, /*33*/0xff8700, /*34*/0x00af00, /*35*/0x5faf00, /*36*/0x87af00, /*37*/0xafaf00, /*38*/0xd7af00, /*39*/0xffaf00,
			/*40*/0x00d700, /*41*/0x5fd700, /*42*/0x87d700, /*43*/0xafd700, /*44*/0xd7d700, /*45*/0xffd700, /*46*/0x00ff00, /*47*/0x5fff00, /*48*/0x87ff00, /*49*/0xafff00, /*50*/0xd7ff00, /*51*/0xffff00,
			/*52*/0x00005f, /*53*/0x5f005f, /*54*/0x87005f, /*55*/0xaf005f, /*56*/0xd7005f, /*57*/0xff005f, /*58*/0x005f5f, /*59*/0x5f5f5f, /*60*/0x875f5f, /*61*/0xaf5f5f, /*62*/0xd75f5f, /*63*/0xff5f5f,
			/*64*/0x00875f, /*65*/0x5f875f, /*66*/0x87875f, /*67*/0xaf875f, /*68*/0xd7875f, /*69*/0xff875f, /*70*/0x00af5f, /*71*/0x5faf5f, /*72*/0x87af5f, /*73*/0xafaf5f, /*74*/0xd7af5f, /*75*/0xffaf5f,
			/*76*/0x00d75f, /*77*/0x5fd75f, /*78*/0x87d75f, /*79*/0xafd75f, /*80*/0xd7d75f, /*81*/0xffd75f, /*82*/0x00ff5f, /*83*/0x5fff5f, /*84*/0x87ff5f, /*85*/0xafff5f, /*86*/0xd7ff5f, /*87*/0xffff5f,
			/*88*/0x000087, /*89*/0x5f0087, /*90*/0x870087, /*91*/0xaf0087, /*92*/0xd70087, /*93*/0xff0087, /*94*/0x005f87, /*95*/0x5f5f87, /*96*/0x875f87, /*97*/0xaf5f87, /*98*/0xd75f87, /*99*/0xff5f87,
			/*100*/0x008787, /*101*/0x5f8787, /*102*/0x878787, /*103*/0xaf8787, /*104*/0xd78787, /*105*/0xff8787, /*106*/0x00af87, /*107*/0x5faf87, /*108*/0x87af87, /*109*/0xafaf87, /*110*/0xd7af87, /*111*/0xffaf87,
			/*112*/0x00d787, /*113*/0x5fd787, /*114*/0x87d787, /*115*/0xafd787, /*116*/0xd7d787, /*117*/0xffd787, /*118*/0x00ff87, /*119*/0x5fff87, /*120*/0x87ff87, /*121*/0xafff87, /*122*/0xd7ff87, /*123*/0xffff87,
			/*124*/0x0000af, /*125*/0x5f00af, /*126*/0x8700af, /*127*/0xaf00af, /*128*/0xd700af, /*129*/0xff00af, /*130*/0x005faf, /*131*/0x5f5faf, /*132*/0x875faf, /*133*/0xaf5faf, /*134*/0xd75faf, /*135*/0xff5faf,
			/*136*/0x0087af, /*137*/0x5f87af, /*138*/0x8787af, /*139*/0xaf87af, /*140*/0xd787af, /*141*/0xff87af, /*142*/0x00afaf, /*143*/0x5fafaf, /*144*/0x87afaf, /*145*/0xafafaf, /*146*/0xd7afaf, /*147*/0xffafaf,
			/*148*/0x00d7af, /*149*/0x5fd7af, /*150*/0x87d7af, /*151*/0xafd7af, /*152*/0xd7d7af, /*153*/0xffd7af, /*154*/0x00ffaf, /*155*/0x5fffaf, /*156*/0x87ffaf, /*157*/0xafffaf, /*158*/0xd7ffaf, /*159*/0xffffaf,
			/*160*/0x0000d7, /*161*/0x5f00d7, /*162*/0x8700d7, /*163*/0xaf00d7, /*164*/0xd700d7, /*165*/0xff00d7, /*166*/0x005fd7, /*167*/0x5f5fd7, /*168*/0x875fd7, /*169*/0xaf5fd7, /*170*/0xd75fd7, /*171*/0xff5fd7,
			/*172*/0x0087d7, /*173*/0x5f87d7, /*174*/0x8787d7, /*175*/0xaf87d7, /*176*/0xd787d7, /*177*/0xff87d7, /*178*/0x00afd7, /*179*/0x5fafd7, /*180*/0x87afd7, /*181*/0xafafd7, /*182*/0xd7afd7, /*183*/0xffafd7,
			/*184*/0x00d7d7, /*185*/0x5fd7d7, /*186*/0x87d7d7, /*187*/0xafd7d7, /*188*/0xd7d7d7, /*189*/0xffd7d7, /*190*/0x00ffd7, /*191*/0x5fffd7, /*192*/0x87ffd7, /*193*/0xafffd7, /*194*/0xd7ffd7, /*195*/0xffffd7,
			/*196*/0x0000ff, /*197*/0x5f00ff, /*198*/0x8700ff, /*199*/0xaf00ff, /*200*/0xd700ff, /*201*/0xff00ff, /*202*/0x005fff, /*203*/0x5f5fff, /*204*/0x875fff, /*205*/0xaf5fff, /*206*/0xd75fff, /*207*/0xff5fff,
			/*208*/0x0087ff, /*209*/0x5f87ff, /*210*/0x8787ff, /*211*/0xaf87ff, /*212*/0xd787ff, /*213*/0xff87ff, /*214*/0x00afff, /*215*/0x5fafff, /*216*/0x87afff, /*217*/0xafafff, /*218*/0xd7afff, /*219*/0xffafff,
			/*220*/0x00d7ff, /*221*/0x5fd7ff, /*222*/0x87d7ff, /*223*/0xafd7ff, /*224*/0xd7d7ff, /*225*/0xffd7ff, /*226*/0x00ffff, /*227*/0x5fffff, /*228*/0x87ffff, /*229*/0xafffff, /*230*/0xd7ffff, /*231*/0xffffff,
			/*232*/0x080808, /*233*/0x121212, /*234*/0x1c1c1c, /*235*/0x262626, /*236*/0x303030, /*237*/0x3a3a3a, /*238*/0x444444, /*239*/0x4e4e4e, /*240*/0x585858, /*241*/0x626262, /*242*/0x6c6c6c, /*243*/0x767676,
			/*244*/0x808080, /*245*/0x8a8a8a, /*246*/0x949494, /*247*/0x9e9e9e, /*248*/0xa8a8a8, /*249*/0xb2b2b2, /*250*/0xbcbcbc, /*251*/0xc6c6c6, /*252*/0xd0d0d0, /*253*/0xdadada, /*254*/0xe4e4e4, /*255*/0xeeeeee
		};

		int  TextColor;        // 30-37,38,39
		BOOL Text256;          // 38
		int  BackColor;        // 40-47,48,49
		BOOL Back256;          // 48

		if (!gDisplayParm.Inverse)
		{
			TextColor = gDisplayParm.TextColor;
			Text256 = gDisplayParm.Text256;
			BackColor = gDisplayParm.BackColor;
			Back256 = gDisplayParm.Back256;
		}
		else
		{
			TextColor = gDisplayParm.BackColor;
			Text256 = gDisplayParm.Back256;
			BackColor = gDisplayParm.TextColor;
			Back256 = gDisplayParm.Text256;
		}


		if (Text256)
		{
			if (Text256 == 2)
			{
				attr.Attributes.Flags |= CECF_FG_24BIT;
				attr.Attributes.ForegroundColor = TextColor&0xFFFFFF;
			}
			else
			{
				if (TextColor > 15)
					attr.Attributes.Flags |= CECF_FG_24BIT;
				attr.Attributes.ForegroundColor = RgbMap[TextColor&0xFF];
			}

			if (gDisplayParm.BrightOrBold)
				attr.Attributes.Flags |= CECF_FG_BOLD;
			if (gDisplayParm.ItalicOrInverse)
				attr.Attributes.Flags |= CECF_FG_ITALIC;
			if (gDisplayParm.BackOrUnderline)
				attr.Attributes.Flags |= CECF_FG_UNDERLINE;
		}
		else
		{
			attr.Attributes.ForegroundColor |= ClrMap[TextColor&0x7]
				| (gDisplayParm.BrightOrBold ? 0x08 : 0);
		}

		if (Back256)
		{
			if (Back256 == 2)
			{
				attr.Attributes.Flags |= CECF_BG_24BIT;
				attr.Attributes.BackgroundColor = BackColor&0xFFFFFF;
			}
			else
			{
				if (BackColor > 15)
					attr.Attributes.Flags |= CECF_BG_24BIT;
				attr.Attributes.BackgroundColor = RgbMap[BackColor&0xFF];
			}
		}
		else
		{
			attr.Attributes.BackgroundColor |= ClrMap[BackColor&0x7]
				| ((gDisplayParm.BackOrUnderline && !Text256) ? 0x8 : 0);
		}


		//SetConsoleTextAttribute(hConsoleOutput, (WORD)wAttrs);
		ExtSetAttributes(&attr);
	}
}


#if defined(DUMP_UNKNOWN_ESCAPES) || defined(DUMP_WRITECONSOLE_LINES)
void CEAnsi::DumpEscape(LPCWSTR buf, size_t cchLen, int iUnknown)
{
	if (!buf || !cchLen)
	{
		// В общем, много кто грешит попытками записи "пустых строк"
		// Например, clink, perl, ...
		//_ASSERTEX((buf && cchLen) || (gszClinkCmdLine && buf));
	}
	else if (iUnknown == 1)
	{
		_ASSERTEX(FALSE && "Unknown Esc Sequence!");
	}

	wchar_t szDbg[200];
	size_t nLen = cchLen;
	static int nWriteCallNo = 0;

	switch (iUnknown)
	{
	case 1:
		//wcscpy_c(szDbg, L"###Unknown Esc Sequence: ");
		msprintf(szDbg, countof(szDbg), L"[%u] ###Unknown Esc Sequence: ", GetCurrentThreadId());
		break;
	case 2:
		msprintf(szDbg, countof(szDbg), L"[%u] ###Bad Unicode Translation: ", GetCurrentThreadId());
		break;
	default:
		msprintf(szDbg, countof(szDbg), L"[%u] AnsiDump #%u: ", GetCurrentThreadId(), ++nWriteCallNo);
	}

	size_t nStart = lstrlenW(szDbg);
	wchar_t* pszDst = szDbg + nStart;

	if (buf && cchLen)
	{
		const wchar_t* pszSrc = (wchar_t*)buf;
		size_t nCur = 0;
		while (nLen)
		{
			switch (*pszSrc)
			{
			case L'\r':
				*(pszDst++) = L'\\'; *(pszDst++) = L'r';
				break;
			case L'\n':
				*(pszDst++) = L'\\'; *(pszDst++) = L'n';
				break;
			case L'\t':
				*(pszDst++) = L'\\'; *(pszDst++) = L't';
				break;
			case L'\x1B':
				*(pszDst++) = szAnalogues[0x1B];
				break;
			case 0:
				*(pszDst++) = L'\\'; *(pszDst++) = L'0';
				break;
			case L'\\':
				*(pszDst++) = L'\\'; *(pszDst++) = L'\\';
				break;
			default:
				*(pszDst++) = *pszSrc;
			}
			pszSrc++;
			nLen--;
			nCur++;

			if (nCur >= 80)
			{
				*(pszDst++) = L'\n'; *pszDst = 0;
				DebugString(szDbg);
				wmemset(szDbg, L' ', nStart);
				nCur = 0;
				pszDst = szDbg + nStart;
			}
		}
	}
	else if (gszClinkCmdLine)
	{
		pszDst -= 2;
		const wchar_t* psEmptyClink = L" - <empty sequence, clink?>";
		size_t nClinkLen = lstrlenW(psEmptyClink);
		wmemcpy(pszDst, psEmptyClink, nClinkLen);
		pszDst += nClinkLen;
	}
	*(pszDst++) = L'\n'; *pszDst = 0;

	DebugString(szDbg);
}
#endif

#ifdef DUMP_UNKNOWN_ESCAPES
#define DumpUnknownEscape(buf, cchLen) DumpEscape(buf, cchLen, true)
#else
#define DumpUnknownEscape(buf,cchLen)
#endif


// When user type smth in the prompt, screen buffer may be scrolled
void CEAnsi::OnReadConsoleBefore(HANDLE hConOut, const CONSOLE_SCREEN_BUFFER_INFO& csbi)
{
	CEAnsi* pObj = CEAnsi::Object();
	if (!pObj)
		return;

	static LONG nLastReadId = GetCurrentProcessId();

	WORD NewRowId;
	CEConsoleMark Test = {};

	COORD crPos[] = {{4,csbi.dwCursorPosition.Y-1},csbi.dwCursorPosition};
	_ASSERTEX(countof(crPos)==countof(pObj->m_RowMarks.SaveRow) && countof(crPos)==countof(pObj->m_RowMarks.RowId));

	pObj->m_RowMarks.csbi = csbi;

	for (int i = 0; i < 2; i++)
	{
		pObj->m_RowMarks.SaveRow[i] = -1;
		pObj->m_RowMarks.RowId[i] = 0;

		if (crPos[i].X < 4 || crPos[i].Y < 0)
			continue;

		if (ReadConsoleRowId(hConOut, crPos[i].Y, &Test))
		{
			pObj->m_RowMarks.SaveRow[i] = crPos[i].Y;
			pObj->m_RowMarks.RowId[i] = Test.RowId;
		}
		else
		{
			NewRowId = LOWORD(InterlockedIncrement(&nLastReadId));
			if (!NewRowId) NewRowId = LOWORD(InterlockedIncrement(&nLastReadId));

			if (WriteConsoleRowId(hConOut, crPos[i].Y, NewRowId))
			{
				pObj->m_RowMarks.SaveRow[i] = crPos[i].Y;
				pObj->m_RowMarks.RowId[i] = NewRowId;
			}
		}
	}

	// Succeesfull mark?
	_ASSERTEX(((pObj->m_RowMarks.RowId[0] || pObj->m_RowMarks.RowId[1]) && (pObj->m_RowMarks.RowId[0] != pObj->m_RowMarks.RowId[1])) || (!csbi.dwCursorPosition.X && !csbi.dwCursorPosition.Y));
}
void CEAnsi::OnReadConsoleAfter(bool bFinal, bool bNoLineFeed)
{
	CEAnsi* pObj = CEAnsi::Object();
	if (!pObj)
		return;

	if (pObj->m_RowMarks.SaveRow[0] < 0 && pObj->m_RowMarks.SaveRow[1] < 0)
		return;

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	SHORT nMarkedRow = -1;
	CEConsoleMark Test = {};

	if (!GetConsoleScreenBufferInfo(hConOut, &csbi))
		goto wrap;

	if (!FindConsoleRowId(hConOut, csbi.dwCursorPosition.Y, &nMarkedRow, &Test))
		goto wrap;

	for (int i = 1; i >= 0; i--)
	{
		if ((pObj->m_RowMarks.SaveRow[i] >= 0) && (pObj->m_RowMarks.RowId[i] == Test.RowId))
		{
			if (pObj->m_RowMarks.SaveRow[i] == nMarkedRow)
			{
				_ASSERTEX(
					((pObj->m_RowMarks.csbi.dwCursorPosition.Y < (pObj->m_RowMarks.csbi.dwSize.Y-1))
						|| (bNoLineFeed && (pObj->m_RowMarks.csbi.dwCursorPosition.Y == (pObj->m_RowMarks.csbi.dwSize.Y-1))))
					&& "Nothing was changed? Strange, scrolling was expected");
				goto wrap;
			}
			// Well, we get scroll distance
			_ASSERTEX(nMarkedRow < pObj->m_RowMarks.SaveRow[i]); // Upside scroll expected
			ExtScrollScreenParm scrl = {sizeof(scrl), essf_ExtOnly, hConOut, nMarkedRow - pObj->m_RowMarks.SaveRow[i]};
			ExtScrollScreen(&scrl);
			goto wrap;
		}
	}

wrap:
	// Clear it
	for (int i = 0; i < 2; i++)
	{
		pObj->m_RowMarks.SaveRow[i] = -1;
		pObj->m_RowMarks.RowId[i] = 0;
	}
}


BOOL WINAPI CEAnsi::OnScrollConsoleScreenBufferA(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill)
{
	typedef BOOL (WINAPI* OnScrollConsoleScreenBufferA_t)(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill);
	ORIGINALFAST(ScrollConsoleScreenBufferA);
	//BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;

	if (IsOutputHandle(hConsoleOutput))
	{
		WARNING("Проверка аргументов! Скролл может быть частичным");
		ExtScrollScreenParm scrl = {sizeof(scrl), essf_ExtOnly, hConsoleOutput, dwDestinationOrigin.Y - lpScrollRectangle->Top};
		ExtScrollScreen(&scrl);
	}

	lbRc = F(ScrollConsoleScreenBufferA)(hConsoleOutput, lpScrollRectangle, lpClipRectangle, dwDestinationOrigin, lpFill);

	return lbRc;
}

BOOL WINAPI CEAnsi::OnScrollConsoleScreenBufferW(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill)
{
	typedef BOOL (WINAPI* OnScrollConsoleScreenBufferW_t)(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill);
	ORIGINALFAST(ScrollConsoleScreenBufferW);
	//BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;

	if (IsOutputHandle(hConsoleOutput))
	{
		WARNING("Проверка аргументов! Скролл может быть частичным");
		ExtScrollScreenParm scrl = {sizeof(scrl), essf_ExtOnly, hConsoleOutput, dwDestinationOrigin.Y - lpScrollRectangle->Top};
		ExtScrollScreen(&scrl);
	}

	//Warning: This function called from "cmd.exe /c cls" whith arguments:
	//lpScrollRectangle - full scroll buffer
	//lpClipRectangle - NULL
	//dwDestinationOrigin = {0, -9999}

	lbRc = F(ScrollConsoleScreenBufferW)(hConsoleOutput, lpScrollRectangle, lpClipRectangle, dwDestinationOrigin, lpFill);

	return lbRc;
}

HANDLE WINAPI CEAnsi::OnCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	typedef HANDLE (WINAPI* OnCreateFileW_t)(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
	ORIGINALFAST(CreateFileW);
	//BOOL bMainThread = FALSE; // поток не важен
	HANDLE h;

	h = F(CreateFileW)(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	DWORD nLastErr = GetLastError();

	DebugString(L"CEAnsi::OnCreateFileW executed");

	// Just a check for "string" validity
	if (lpFileName && (((DWORD_PTR)lpFileName) & ~0xFFFF)
		// CON output is opening with following flags
		&& (dwDesiredAccess & GENERIC_WRITE)
		&& ((dwShareMode & (FILE_SHARE_READ|FILE_SHARE_WRITE)) == (FILE_SHARE_READ|FILE_SHARE_WRITE))
		)
	{
		DEBUGTEST(HANDLE hStd = GetStdHandle(STD_OUTPUT_HANDLE));
		if (lstrcmpi(lpFileName, L"CON") == 0)
		{
			ghLastConOut = h;
		}
		DebugString(L"CEAnsi::OnCreateFileW checked");
	}

	SetLastError(nLastErr);
	return h;
}

BOOL WINAPI CEAnsi::OnWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	typedef BOOL (WINAPI* OnWriteFile_t)(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
	ORIGINALFAST(WriteFile);
	//BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;
	DWORD nDBCSCP = 0;

	FIRST_ANSI_CALL((const BYTE*)lpBuffer, nNumberOfBytesToWrite);

	//if (gpStartEnv->bIsDbcs)
	//{
	//	nDBCSCP = GetConsoleOutputCP();
	//}

	if (lpBuffer && nNumberOfBytesToWrite && hFile && IsAnsiCapable(hFile))
		lbRc = OnWriteConsoleA(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, NULL);
	else
		lbRc = F(WriteFile)(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);

	return lbRc;
}

BOOL WINAPI CEAnsi::OnWriteConsoleA(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved)
{
	typedef BOOL (WINAPI* OnWriteConsoleA_t)(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);
	ORIGINALFAST(WriteConsoleA);
	//BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;
	wchar_t* buf = NULL;
	DWORD len, cp;
	static bool badUnicode = false;
	DEBUGTEST(bool curBadUnicode = badUnicode);
	DEBUGTEST(wchar_t szTmp[2] = L"");

	FIRST_ANSI_CALL((const BYTE*)lpBuffer, nNumberOfCharsToWrite);

	if (badUnicode)
	{
		badUnicode = false;
		#ifdef _DEBUG
		if (lpBuffer && nNumberOfCharsToWrite)
		{
			szTmp[0] = ((char*)lpBuffer)[0];
			DumpEscape(szTmp, 1, 2);
		}
		#endif
		goto badchar;
	}

	if (lpBuffer && nNumberOfCharsToWrite && hConsoleOutput && IsAnsiCapable(hConsoleOutput))
	{
		cp = gCpConv.nDefaultCP ? gCpConv.nDefaultCP : GetConsoleOutputCP();

		DWORD nLastErr = 0;
		DWORD nFlags = 0; //MB_ERR_INVALID_CHARS;

		if (! ((cp == 42) || (cp == 65000) || (cp >= 57002 && cp <= 57011) || (cp >= 50220 && cp <= 50229)) )
		{
			nFlags = MB_ERR_INVALID_CHARS;
			nLastErr = GetLastError();
		}

		len = MultiByteToWideChar(cp, nFlags, (LPCSTR)lpBuffer, nNumberOfCharsToWrite, 0, 0);

		if (nFlags /*gpStartEnv->bIsDbcs*/ && (GetLastError() == ERROR_NO_UNICODE_TRANSLATION))
		{
			badUnicode = true;
			#ifdef _DEBUG
			szTmp[0] = ((char*)lpBuffer)[0];
			DumpEscape(szTmp, 1, 2);
			#endif
			SetLastError(nLastErr);
			goto badchar;
		}

		buf = (wchar_t*)malloc((len+1)*sizeof(wchar_t));
		if (buf == NULL)
		{
			if (lpNumberOfCharsWritten != NULL)
				*lpNumberOfCharsWritten = 0;
		}
		else
		{
			DWORD newLen = MultiByteToWideChar(cp, nFlags, (LPCSTR)lpBuffer, nNumberOfCharsToWrite, buf, len);
			_ASSERTEX(newLen==len);
			buf[newLen] = 0; // ASCII-Z, хотя, если функцию WriteConsoleW зовет приложение - "\0" может и не быть...

			HANDLE hWrite;
			MConHandle hConOut(L"CONOUT$");
			if (hConsoleOutput == ghLastConOut)
				hWrite = hConOut;
			else
				hWrite = hConsoleOutput;

			DWORD nWideWritten = 0;
			lbRc = OurWriteConsoleW(hWrite, buf, len, &nWideWritten, NULL);

			// Issue 1291:	Python fails to print string sequence with ASCII character followed by Chinese character.
			if (lpNumberOfCharsWritten)
			{
				*lpNumberOfCharsWritten = (nWideWritten == len) ? nNumberOfCharsToWrite : nWideWritten;
			}
		}
		goto fin;
	}

badchar:
	// По идее, сюда попадать не должны. Ошибка в параметрах?
	_ASSERTEX((lpBuffer && nNumberOfCharsToWrite && hConsoleOutput && IsAnsiCapable(hConsoleOutput)) || (curBadUnicode||badUnicode));
	lbRc = F(WriteConsoleA)(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);

fin:
	SafeFree(buf);
	return lbRc;
}

TODO("По хорошему, после WriteConsoleOutputAttributes тоже нужно делать efof_ResetExt");
// Но пока можно это проигнорировать, большинство (?) программ, использует ее в связке
// WriteConsoleOutputAttributes/WriteConsoleOutputCharacter

BOOL WINAPI CEAnsi::OnWriteConsoleOutputCharacterA(HANDLE hConsoleOutput, LPCSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten)
{
	typedef BOOL (WINAPI* OnWriteConsoleOutputCharacterA_t)(HANDLE hConsoleOutput, LPCSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten);
	ORIGINALFAST(WriteConsoleOutputCharacterA);
	//BOOL bMainThread = FALSE; // поток не важен

	FIRST_ANSI_CALL((const BYTE*)lpCharacter, nLength);

	ExtFillOutputParm fll = {sizeof(fll),
		efof_Attribute|(gDisplayParm.WasSet ? efof_Current : efof_ResetExt),
		hConsoleOutput, {}, 0, dwWriteCoord, nLength};
	ExtFillOutput(&fll);

	BOOL lbRc = F(WriteConsoleOutputCharacterA)(hConsoleOutput, lpCharacter, nLength, dwWriteCoord, lpNumberOfCharsWritten);

	return lbRc;
}

BOOL WINAPI CEAnsi::OnWriteConsoleOutputCharacterW(HANDLE hConsoleOutput, LPCWSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten)
{
	typedef BOOL (WINAPI* OnWriteConsoleOutputCharacterW_t)(HANDLE hConsoleOutput, LPCWSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten);
	ORIGINALFAST(WriteConsoleOutputCharacterW);
	//BOOL bMainThread = FALSE; // поток не важен

	FIRST_ANSI_CALL((const BYTE*)lpCharacter, nLength);

	ExtFillOutputParm fll = {sizeof(fll),
		efof_Attribute|(gDisplayParm.WasSet ? efof_Current : efof_ResetExt),
		hConsoleOutput, {}, 0, dwWriteCoord, nLength};
	ExtFillOutput(&fll);

	BOOL lbRc = F(WriteConsoleOutputCharacterW)(hConsoleOutput, lpCharacter, nLength, dwWriteCoord, lpNumberOfCharsWritten);

	return lbRc;
}

BOOL CEAnsi::WriteText(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, BOOL abCommit /*= FALSE*/)
{
	BOOL lbRc = FALSE;
	DWORD /*nWritten = 0,*/ nTotalWritten = 0;

	ExtWriteTextParm write = {sizeof(write), ewtf_Current, hConsoleOutput};
	write.Private = (void*)(FARPROC)_WriteConsoleW;

	if (gDisplayOpt.WrapWasSet && (gDisplayOpt.WrapAt > 0))
	{
		write.Flags |= ewtf_WrapAt;
		write.WrapAtCol = gDisplayOpt.WrapAt;
	}

	if (gDisplayOpt.ScrollRegion)
	{
		write.Flags |= ewtf_Region;
		_ASSERTEX(gDisplayOpt.ScrollStart>=1 && gDisplayOpt.ScrollEnd>=gDisplayOpt.ScrollStart);
		write.Region.top = gDisplayOpt.ScrollStart-1;
		write.Region.bottom = gDisplayOpt.ScrollEnd-1;
		write.Region.left = write.Region.right = 0; // not used yet
	}

	//lbRc = _WriteConsoleW(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, &nTotalWritten, NULL);
	write.Buffer = lpBuffer;
	write.NumberOfCharsToWrite = nNumberOfCharsToWrite;

	lbRc = ExtWriteText(&write);
	if (lbRc)
	{
		if (write.NumberOfCharsWritten)
			nTotalWritten += write.NumberOfCharsWritten;
		if (write.ScrolledRowsUp > 0)
			gDisplayCursor.StoredCursorPos.Y = max(0,((int)gDisplayCursor.StoredCursorPos.Y - (int)write.ScrolledRowsUp));
	}

	if (lpNumberOfCharsWritten)
		*lpNumberOfCharsWritten = nTotalWritten;

	return lbRc;
}

BOOL WINAPI CEAnsi::OnWriteConsoleW(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved)
{
	return OurWriteConsoleW(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved, false);
}

BOOL WINAPI CEAnsi::OurWriteConsoleW(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved, bool bInternal /*= false*/)
{
	ORIGINALFAST(WriteConsoleW);
	//BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;
	//ExtWriteTextParm wrt = {sizeof(wrt), ewtf_None, hConsoleOutput};
	bool bIsConOut = false;

	FIRST_ANSI_CALL((const BYTE*)lpBuffer, nNumberOfCharsToWrite);

#if 0
	// Store prompt(?) for clink 0.1.1
	if ((gnAllowClinkUsage == 1) && nNumberOfCharsToWrite && lpBuffer && gpszLastWriteConsole && gcchLastWriteConsoleMax)
	{
		size_t cchMax = min(gcchLastWriteConsoleMax-1,nNumberOfCharsToWrite);
		gpszLastWriteConsole[cchMax] = 0;
		wmemmove(gpszLastWriteConsole, (const wchar_t*)lpBuffer, cchMax);
	}
#endif

	#ifdef DUMP_WRITECONSOLE_LINES
	// Логирование в отладчик ВСЕГО, что пишется в консольный Output
	DumpEscape((wchar_t*)lpBuffer, nNumberOfCharsToWrite, false);
	#endif

	CEAnsi* pObj = NULL;
	CEStr CpCvt;

	if (lpBuffer && nNumberOfCharsToWrite && hConsoleOutput && IsAnsiCapable(hConsoleOutput, &bIsConOut))
	{
		if (ghAnsiLogFile)
			CEAnsi::WriteAnsiLog((LPCWSTR)lpBuffer, nNumberOfCharsToWrite);

		// if that was API call of WriteConsoleW
		if (!bInternal && gCpConv.nFromCP && gCpConv.nToCP)
		{
			// Convert from unicode to MBCS
			char* pszTemp = NULL;
			int iMBCSLen = WideCharToMultiByte(gCpConv.nFromCP, 0, (LPCWSTR)lpBuffer, nNumberOfCharsToWrite, NULL, 0, NULL, NULL);
			if ((iMBCSLen > 0) && ((pszTemp = (char*)malloc(iMBCSLen)) != NULL))
			{
				BOOL bFailed = FALSE; // Do not do conversion if some chars can't be mapped
				iMBCSLen = WideCharToMultiByte(gCpConv.nFromCP, 0, (LPCWSTR)lpBuffer, nNumberOfCharsToWrite, pszTemp, iMBCSLen, NULL, &bFailed);
				if ((iMBCSLen > 0) && !bFailed)
				{
					int iWideLen = MultiByteToWideChar(gCpConv.nToCP, 0, pszTemp, iMBCSLen, NULL, 0);
					if (iWideLen > 0)
					{
						wchar_t* ptrBuf = CpCvt.GetBuffer(iWideLen);
						if (ptrBuf)
						{
							iWideLen = MultiByteToWideChar(gCpConv.nToCP, 0, pszTemp, iMBCSLen, ptrBuf, iWideLen);
							if (iWideLen > 0)
							{
								lpBuffer = ptrBuf;
								nNumberOfCharsToWrite = iWideLen;
							}
						}
					}
				}
			}
			SafeFree(pszTemp);
		}

		pObj = CEAnsi::Object();
		if (pObj)
		{
			if (pObj->gnPrevAnsiPart || pObj->gDisplayOpt.WrapWasSet)
			{
				// Если остался "хвост" от предущей записи - сразу, без проверок
				lbRc = pObj->WriteAnsiCodes(F(WriteConsoleW), hConsoleOutput, (const wchar_t*)lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten);
				goto ansidone;
			}
			else
			{
				_ASSERTEX(ESC==27 && BEL==7 && DSC==0x90);
				const wchar_t* pch = (const wchar_t*)lpBuffer;
				for (size_t i = nNumberOfCharsToWrite; i--; pch++)
				{
					// Если в выводимой строке встречается "Ansi ESC Code" - выводим сами
					TODO("Non-CSI codes, like as BEL, BS, CR, LF, FF, TAB, VT, SO, SI");
					if (*pch == ESC /*|| *pch == BEL*/ /*|| *pch == ENQ*/)
					{
						lbRc = pObj->WriteAnsiCodes(F(WriteConsoleW), hConsoleOutput, (const wchar_t*)lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten);
						goto ansidone;
					}
				}
			}
		}
	}

	if (!bIsConOut || ((pObj = CEAnsi::Object()) == NULL))
	{
		lbRc = F(WriteConsoleW)(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);
	}
	else
	{
		lbRc = pObj->WriteText(F(WriteConsoleW), hConsoleOutput, (LPCWSTR)lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, TRUE);
		//wrt.Flags = ewtf_Current|ewtf_Commit;
		//wrt.Buffer = (const wchar_t*)lpBuffer;
		//wrt.NumberOfCharsToWrite = nNumberOfCharsToWrite;
		//wrt.Private = F(WriteConsoleW);
		//lbRc = ExtWriteText(&wrt);
		//if (lbRc)
		//{
		//	if (lpNumberOfCharsWritten)
		//		*lpNumberOfCharsWritten = wrt.NumberOfCharsWritten;
		//	if (wrt.ScrolledRowsUp > 0)
		//		gDisplayCursor.StoredCursorPos.Y = max(0,((int)gDisplayCursor.StoredCursorPos.Y - (int)wrt.ScrolledRowsUp));
		//}
	}
	goto wrap;

ansidone:
	{
		ExtCommitParm cmt = {sizeof(cmt), hConsoleOutput};
		ExtCommit(&cmt);
	}
wrap:
	return lbRc;
}

//struct AnsiEscCode
//{
//	wchar_t  First;  // ESC (27)
//	wchar_t  Second; // any of 64 to 95 ('@' to '_')
//	wchar_t  Action; // any of 64 to 126 (@ to ~). this is terminator
//	wchar_t  Skip;   // Если !=0 - то эту последовательность нужно пропустить
//	int      ArgC;
//	int      ArgV[16];
//	LPCWSTR  ArgSZ; // Reserved for key mapping
//	size_t   cchArgSZ;
//
//#ifdef _DEBUG
//	LPCWSTR  pszEscStart;
//	size_t   nTotalLen;
//#endif
//
//	int      PvtLen;
//	wchar_t  Pvt[16];
//};


// 0 - нет (в lpBuffer только текст)
// 1 - в Code помещена Esc последовательность (может быть простой текст ДО нее)
// 2 - нет, но кусок последовательности сохранен в gsPrevAnsiPart
int CEAnsi::NextEscCode(LPCWSTR lpBuffer, LPCWSTR lpEnd, wchar_t (&szPreDump)[CEAnsi_MaxPrevPart], DWORD& cchPrevPart, LPCWSTR& lpStart, LPCWSTR& lpNext, CEAnsi::AnsiEscCode& Code, BOOL ReEntrance /*= FALSE*/)
{
	int iRc = 0;
	wchar_t wc;

	LPCWSTR lpSaveStart = lpBuffer;
	lpStart = lpBuffer;

	_ASSERTEX(cchPrevPart==0);

	if (gnPrevAnsiPart && !ReEntrance)
	{
		if (*gsPrevAnsiPart == 27)
		{
			_ASSERTEX(gnPrevAnsiPart < 79);
			INT_PTR nCurPrevLen = gnPrevAnsiPart;
			INT_PTR nAdd = min((lpEnd-lpBuffer),(INT_PTR)countof(gsPrevAnsiPart)-nCurPrevLen-1);
			// Need to check buffer overflow!!!
			_ASSERTEX((INT_PTR)countof(gsPrevAnsiPart)>(nCurPrevLen+nAdd));
			wmemcpy(gsPrevAnsiPart+nCurPrevLen, lpBuffer, nAdd);
			gsPrevAnsiPart[nCurPrevLen+nAdd] = 0;

			WARNING("Проверить!!!");
			LPCWSTR lpReStart, lpReNext;
			int iCall = NextEscCode(gsPrevAnsiPart, gsPrevAnsiPart+nAdd+gnPrevAnsiPart, szPreDump, cchPrevPart, lpReStart, lpReNext, Code, TRUE);
			if (iCall == 1)
			{
				if ((lpReNext - gsPrevAnsiPart) >= gnPrevAnsiPart)
				{
					// Bypass unrecognized ESC sequences to screen?
					if (lpReStart > gsPrevAnsiPart)
					{
						INT_PTR nSkipLen = (lpReStart - gsPrevAnsiPart); //DWORD nWritten;
						_ASSERTEX(nSkipLen<=countof(gsPrevAnsiPart) && nSkipLen<=gnPrevAnsiPart);
						DumpUnknownEscape(gsPrevAnsiPart, nSkipLen);

						//WriteText(_WriteConsoleW, hConsoleOutput, gsPrevAnsiPart, nSkipLen, &nWritten);
						_ASSERTEX(nSkipLen <= ((int)CEAnsi_MaxPrevPart - (int)cchPrevPart));
						memmove(szPreDump, gsPrevAnsiPart, nSkipLen);
						cchPrevPart += nSkipLen;

						if (nSkipLen < gnPrevAnsiPart)
						{
							memmove(gsPrevAnsiPart, lpReStart, (gnPrevAnsiPart - nSkipLen)*sizeof(*gsPrevAnsiPart));
							gnPrevAnsiPart -= nSkipLen;
						}
						else
						{
							_ASSERTEX(nSkipLen == gnPrevAnsiPart);
							*gsPrevAnsiPart = 0;
							gnPrevAnsiPart = 0;
						}
						lpReStart = gsPrevAnsiPart;
					}
					_ASSERTEX(lpReStart == gsPrevAnsiPart);
					lpStart = lpBuffer; // nothing to dump before Esc-sequence
					_ASSERTEX((lpReNext - gsPrevAnsiPart) >= gnPrevAnsiPart);
					WARNING("Проверить!!!");
					lpNext = lpBuffer + (lpReNext - gsPrevAnsiPart - gnPrevAnsiPart);
				}
				else
				{
					_ASSERTEX((lpReNext - gsPrevAnsiPart) >= gnPrevAnsiPart);
					lpStart = lpNext = lpBuffer;
				}
				gnPrevAnsiPart = 0;
				gsPrevAnsiPart[0] = 0;
				iRc = 1;
				goto wrap2;
			}
			else if (iCall == 2)
			{
				gnPrevAnsiPart = nCurPrevLen+nAdd;
				_ASSERTEX(gsPrevAnsiPart[nCurPrevLen+nAdd] == 0);
				iRc = 2;
				goto wrap2;
			}

			_ASSERTEX((iCall == 1) && "Invalid esc sequence, need dump to screen?");
		}
		else
		{
			_ASSERTEX(*gsPrevAnsiPart == 27);
		}
	}


	while (lpBuffer < lpEnd)
	{
		switch (*lpBuffer)
		{
		case 27:
			{
				INT_PTR nLeft;
				LPCWSTR lpEscStart = lpBuffer;

				#ifdef _DEBUG
				Code.pszEscStart = lpBuffer;
				Code.nTotalLen = 0;
				#endif

				// minimal length failed?
				if ((lpBuffer + 2) >= lpEnd)
				{
					// But it may be some "special" codes
					switch (lpBuffer[1])
					{
					case L'=':
					case L'>':
						// xterm?
						Code.First = 27;
						Code.Second = *(++lpBuffer);
						Code.ArgC = 0;
						Code.PvtLen = 0;
						Code.Pvt[0] = 0;
						lpEnd = (++lpBuffer);
						iRc = 1;
						goto wrap;
					}
				}
				else
				{
					// Set lpSaveStart to current start of Esc sequence, it was set to beginning of buffer
					_ASSERTEX(lpSaveStart <= lpBuffer);
					lpSaveStart = lpBuffer;

					Code.First = 27;
					Code.Second = *(++lpBuffer);
					Code.ArgC = 0;
					Code.PvtLen = 0;
					Code.Pvt[0] = 0;

					TODO("Bypass unrecognized ESC sequences to screen? Don't try to eliminate 'Possible' sequences?");
					//if (((Code.Second < 64) || (Code.Second > 95)) && (Code.Second != 124/* '|' - vim-xterm-emulation */))
					if (!wcschr(L"[]|=>", Code.Second))
					{
						// Don't assert on rawdump of KeyEvents.exe Esc key presses
						// 10:00:00 KEY_EVENT_RECORD: Dn, 1, Vk="VK_ESCAPE" [27/0x001B], Scan=0x0001 uChar=[U='\x1b' (0x001B): A='\x1b' (0x1B)]
						bool bStandaloneEscChar = (lpStart < lpSaveStart) && ((*(lpSaveStart-1) == L'\'' && Code.Second == L'\'') || (*(lpSaveStart-1) == L' ' && Code.Second == L' '));
						_ASSERTEX(bStandaloneEscChar && "Unsupported control sequence?");
						continue; // invalid code
					}

					// Теперь идут параметры.
					++lpBuffer; // переместим указатель на первый символ ЗА CSI (после '[')

					switch (Code.Second)
					{
					case L'|':
						// vim-xterm-emulation
					case L'[':
						// Standard
						Code.Skip = 0;
						Code.ArgSZ = NULL;
						Code.cchArgSZ = 0;
						{
							int nValue = 0, nDigits = 0;
							#ifdef _DEBUG
							LPCWSTR pszSaveStart = lpBuffer;
							#endif

							while (lpBuffer < lpEnd)
							{
								switch (*lpBuffer)
								{
								case L'0': case L'1': case L'2': case L'3': case L'4':
								case L'5': case L'6': case L'7': case L'8': case L'9':
									nValue = (nValue * 10) + (((int)*lpBuffer) - L'0');
									++nDigits;
									break;

								case L';':
									// Даже если цифр не было - default "0"
									if (Code.ArgC < (int)countof(Code.ArgV))
										Code.ArgV[Code.ArgC++] = nValue; // save argument
									nDigits = nValue = 0;
									break;

								default:
									if (((wc = *lpBuffer) >= 64) && (wc <= 126))
									{
										// Fin
										Code.Action = wc;
										if (nDigits && (Code.ArgC < (int)countof(Code.ArgV)))
										{
											Code.ArgV[Code.ArgC++] = nValue;
										}
										lpStart = lpSaveStart;
										lpEnd = lpBuffer+1;
										iRc = 1;
										goto wrap;
									}
									else
									{
										if ((Code.PvtLen+1) < (int)countof(Code.Pvt))
										{
											Code.Pvt[Code.PvtLen++] = wc; // Skip private symbols
											Code.Pvt[Code.PvtLen] = 0;
										}
									}
								}
								++lpBuffer;
							}
						}
						// В данном запросе (на запись) конца последовательности нет,
						// оставшийся хвост нужно сохранить в буфере, для следующего запроса
						// Ниже
						break;

					case L']':
						// Finalizing with "\x1B\\" or "\x07"
						// "%]4;16;rgb:00/00/00%\" - "%" is ESC
						// "%]0;this is the window titleBEL"
						// ESC ] 0 ; txt ST        Set icon name and window title to txt.
						// ESC ] 1 ; txt ST        Set icon name to txt.
						// ESC ] 2 ; txt ST        Set window title to txt.
						// ESC ] 4 ; num; txt ST   Set ANSI color num to txt.
						// ESC ] 10 ; txt ST       Set dynamic text color to txt.
						// ESC ] 4 6 ; name ST     Change log file to name (normally disabled
						//					       by a compile-time option)
						// ESC ] 5 0 ; fn ST       Set font to fn.
						//Following 2 codes - from linux terminal
						// ESC ] P nrrggbb         Set palette, with parameter given in 7
                        //                         hexadecimal digits after the final P :-(.
						//                         Here n is the color (0-15), and rrggbb indicates
						//                         the red/green/blue values (0-255).
						// ESC ] R                 reset palette

						// ConEmu specific
						// ESC ] 9 ; 1 ; ms ST           Sleep. ms - milliseconds
						// ESC ] 9 ; 2 ; "txt" ST        Show GUI MessageBox ( txt ) for dubug purposes
						// ESC ] 9 ; 3 ; "txt" ST        Set TAB text
						// ESC ] 9 ; 4 ; st ; pr ST      When _st_ is 0: remove progress. When _st_ is 1: set progress value to _pr_ (number, 0-100). When _st_ is 2: set error state in progress on Windows 7 taskbar
						// ESC ] 9 ; 5 ST                Wait for ENTER/SPACE/ESC. Set EnvVar "ConEmuWaitKey" to ENTER/SPACE/ESC on exit.
						// ESC ] 9 ; 6 ; "txt" ST        Execute GuiMacro. Set EnvVar "ConEmuMacroResult" on exit.

						Code.ArgSZ = lpBuffer;
						Code.cchArgSZ = 0;
						//Code.Skip = Code.Second;

						while (lpBuffer < lpEnd)
						{
							if ((lpBuffer[0] == 7) ||
								(lpBuffer[0] == 27) /* we'll check the proper terminator below */)
							{
								Code.Action = *Code.ArgSZ; // первый символ последовательности
								Code.cchArgSZ = (lpBuffer - Code.ArgSZ);
								lpStart = lpSaveStart;
								if (lpBuffer[0] == 27)
								{
									if (((lpBuffer + 1) < lpEnd) && (lpBuffer[1] == L'\\'))
									{
										lpEnd = lpBuffer + 2;
									}
									else
									{
										lpEnd = lpBuffer - 1;
										_ASSERTE(*(lpEnd+1) == 27);
										DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
										iRc = 0;
										goto wrap;
									}
								}
								else
								{
									lpEnd = lpBuffer + 1;
								}
								iRc = 1;
								goto wrap;
							}
							++lpBuffer;
						}
						// В данном запросе (на запись) конца последовательности нет,
						// оставшийся хвост нужно сохранить в буфере, для следующего запроса
						// Ниже
						break;

					case L'=':
					case L'>':
						// xterm?
						lpEnd = lpBuffer;
						iRc = 1;
						goto wrap;

					default:
						// Неизвестный код, обрабатываем по общим правилам
						Code.Skip = Code.Second;
						Code.ArgSZ = lpBuffer;
						Code.cchArgSZ = 0;
						while (lpBuffer < lpEnd)
						{
							if (((wc = *lpBuffer) >= 64) && (wc <= 126))
							{
								Code.Action = wc;
								lpStart = lpSaveStart;
								lpEnd = lpBuffer+1;
								iRc = 1;
								goto wrap;
							}
							++lpBuffer;
						}

					} // end of "switch (Code.Second)"
				} // end of minimal length check

				if ((nLeft = (lpEnd - lpEscStart)) <= CEAnsi_MaxPrevAnsiPart)
				{
					if (ReEntrance)
					{
						//_ASSERTEX(!ReEntrance && "Need to be checked!"); -- seems to be OK

						// gsPrevAnsiPart2 stored for debug purposes only (fully excess)
						wmemmove(gsPrevAnsiPart2, lpEscStart, nLeft);
						gsPrevAnsiPart2[nLeft] = 0;
						gnPrevAnsiPart2 = nLeft;
					}
					else
					{
						wmemmove(gsPrevAnsiPart, lpEscStart, nLeft);
						gsPrevAnsiPart[nLeft] = 0;
						gnPrevAnsiPart = nLeft;
					}
				}
				else
				{
					_ASSERTEX(FALSE && "Too long Esc-sequence part, Need to be checked!");
				}

				lpStart = lpEscStart;

				iRc = 2;
				goto wrap;
			} // end of "case 27:"
			break;
		} // end of "switch (*lpBuffer)"

		++lpBuffer;
	} // end of "while (lpBuffer < lpEnd)"

wrap:
	lpNext = lpEnd;

	#ifdef _DEBUG
	if (iRc == 1)
		Code.nTotalLen = (lpEnd - Code.pszEscStart);
	#endif
wrap2:
	_ASSERTEX((iRc==0) || (lpStart>=lpSaveStart && lpStart<lpEnd));
	return iRc;
}

BOOL CEAnsi::ScrollScreen(HANDLE hConsoleOutput, int nDir)
{
	ExtScrollScreenParm scrl = {sizeof(scrl), essf_Current|essf_Commit, hConsoleOutput, nDir, {}, L' '};
	if (gDisplayOpt.ScrollRegion)
	{
		_ASSERTEX(gDisplayOpt.ScrollStart>=1 && gDisplayOpt.ScrollEnd>=gDisplayOpt.ScrollStart);
		scrl.Region.top = gDisplayOpt.ScrollStart-1;
		scrl.Region.bottom = gDisplayOpt.ScrollEnd-1;
		scrl.Flags |= essf_Region;
	}

	BOOL lbRc = ExtScrollScreen(&scrl);

	return lbRc;
}

BOOL CEAnsi::PadAndScroll(HANDLE hConsoleOutput, CONSOLE_SCREEN_BUFFER_INFO& csbi)
{
	BOOL lbRc = FALSE;
	COORD crFrom = {csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y};
	DEBUGTEST(DWORD nCount = csbi.dwSize.X - csbi.dwCursorPosition.X);

	/*
	lbRc = FillConsoleOutputAttribute(hConsoleOutput, GetDefaultTextAttr(), nCount, crFrom, &nWritten)
		&& FillConsoleOutputCharacter(hConsoleOutput, L' ', nCount, crFrom, &nWritten);
	*/

	if ((csbi.dwCursorPosition.Y + 1) >= csbi.dwSize.Y)
	{
		lbRc = ScrollScreen(hConsoleOutput, -1);
		crFrom.X = 0;
	}
	else
	{
		crFrom.X = 0; crFrom.Y++;
		lbRc = TRUE;
	}

	csbi.dwCursorPosition = crFrom;
	SetConsoleCursorPosition(hConsoleOutput, crFrom);

	return lbRc;
}

BOOL CEAnsi::LinesInsert(HANDLE hConsoleOutput, const int LinesCount)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (!GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
	{
		_ASSERTEX(FALSE && "GetConsoleScreenBufferInfoCached failed");
		return FALSE;
	}

	int TopLine, BottomLine;
	if (gDisplayOpt.ScrollRegion)
	{
		_ASSERTEX(gDisplayOpt.ScrollStart>=1 && gDisplayOpt.ScrollEnd>=gDisplayOpt.ScrollStart);
		TopLine = max(gDisplayOpt.ScrollStart-1,0);
		BottomLine = max(gDisplayOpt.ScrollEnd-1,0);
	}
	else
	{
		TODO("What we need to scroll? Buffer or visible rect?");
		TopLine = csbi.dwCursorPosition.Y;
		BottomLine = csbi.dwSize.Y - 1;
	}

	// Apply default color before scrolling!
	ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);

	ExtScrollScreenParm scrl = {
		sizeof(scrl), essf_Current|essf_Commit|essf_Region, hConsoleOutput,
		LinesCount, {}, L' ', {0, TopLine, 0, BottomLine}};
	BOOL lbRc = ExtScrollScreen(&scrl);
	return lbRc;
}

BOOL CEAnsi::LinesDelete(HANDLE hConsoleOutput, const int LinesCount)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (!GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
	{
		_ASSERTEX(FALSE && "GetConsoleScreenBufferInfoCached failed");
		return FALSE;
	}

	int TopLine, BottomLine;
	if (gDisplayOpt.ScrollRegion)
	{
		_ASSERTEX(gDisplayOpt.ScrollStart>=1 && gDisplayOpt.ScrollEnd>gDisplayOpt.ScrollStart);
		// ScrollStart & ScrollEnd are 1-based line indexes
		// relative to VISIBLE area, these are not absolute buffer coords
		TopLine = gDisplayOpt.ScrollStart-1+csbi.srWindow.Top;
		BottomLine = gDisplayOpt.ScrollEnd-1+csbi.srWindow.Top;
	}
	else
	{
		TODO("What we need to scroll? Buffer or visible rect?");
		TopLine = csbi.dwCursorPosition.Y;
		BottomLine = csbi.dwSize.Y - 1;
	}

	if (BottomLine < TopLine)
	{
		_ASSERTEX(FALSE && "Invalid (empty) scroll region");
		return FALSE;
	}

	// Apply default color before scrolling!
	ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);

	ExtScrollScreenParm scrl = {
		sizeof(scrl), essf_Current|essf_Commit|essf_Region, hConsoleOutput,
		-LinesCount, {}, L' ', {0, TopLine, 0, BottomLine}};
	BOOL lbRc = ExtScrollScreen(&scrl);
	return lbRc;
}

int CEAnsi::NextNumber(LPCWSTR& asMS)
{
	wchar_t wc;
	int ms = 0;
	while (((wc = *(asMS++)) >= L'0') && (wc <= L'9'))
		ms = (ms * 10) + (int)(wc - L'0');
	return ms;
}

// ESC ] 9 ; 1 ; ms ST           Sleep. ms - milliseconds
void CEAnsi::DoSleep(LPCWSTR asMS)
{
	int ms = NextNumber(asMS);
	if (!ms)
		ms = 100;
	else if (ms > 10000)
		ms = 10000;
	// Delay
	Sleep(ms);
}

void CEAnsi::EscCopyCtrlString(wchar_t* pszDst, LPCWSTR asMsg, INT_PTR cchMaxLen)
{
	if (!pszDst)
	{
		_ASSERTEX(pszDst!=NULL);
		return;
	}

	if (cchMaxLen < 0)
	{
		_ASSERTEX(cchMaxLen >= 0);
		cchMaxLen = 0;
	}
	if (cchMaxLen > 1)
	{
		if ((asMsg[0] == L'"') && (asMsg[cchMaxLen-1] == L'"'))
		{
			asMsg++;
			cchMaxLen -= 2;
		}
	}

	if (cchMaxLen > 0)
		wmemmove(pszDst, asMsg, cchMaxLen);
	pszDst[max(cchMaxLen,0)] = 0;
}

// ESC ] 9 ; 2 ; "txt" ST          Show GUI MessageBox ( txt ) for dubug purposes
void CEAnsi::DoMessage(LPCWSTR asMsg, INT_PTR cchLen)
{
	wchar_t* pszText = (wchar_t*)malloc((cchLen+1)*sizeof(*pszText));

	if (pszText)
	{
		EscCopyCtrlString(pszText, asMsg, cchLen);
		//if (cchLen > 0)
		//	wmemmove(pszText, asMsg, cchLen);
		//pszText[cchLen] = 0;

		wchar_t szExe[MAX_PATH] = {};
		GetModuleFileName(NULL, szExe, countof(szExe));
		wchar_t szTitle[MAX_PATH+64];
		msprintf(szTitle, countof(szTitle), L"PID=%u, %s", GetCurrentProcessId(), PointToName(szExe));

		GuiMessageBox(ghConEmuWnd, pszText, szTitle, MB_ICONINFORMATION|MB_SYSTEMMODAL);

		free(pszText);
	}
}

// ESC ] 9 ; 7 ; "cmd" ST        Run some process with arguments
void CEAnsi::DoProcess(LPCWSTR asCmd, INT_PTR cchLen)
{
	// We need zero-terminated string
	wchar_t* pszCmdLine = (wchar_t*)malloc((cchLen + 1)*sizeof(*asCmd));

	if (pszCmdLine)
	{
		EscCopyCtrlString(pszCmdLine, asCmd, cchLen);

		STARTUPINFO si = {sizeof(si)};
		PROCESS_INFORMATION pi = {};

		BOOL bCreated = OnCreateProcessW(NULL, pszCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
		if (bCreated)
		{
			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}

		free(pszCmdLine);
	}
}

// ESC ] 9 ; 8 ; "env" ST        Output value of environment variable
void CEAnsi::DoPrintEnv(LPCWSTR asCmd, INT_PTR cchLen)
{
	if (!pfnWriteConsoleW)
		return;

	// We need zero-terminated string
	wchar_t* pszVarName = (wchar_t*)malloc((cchLen + 1)*sizeof(*asCmd));

	if (pszVarName)
	{
		EscCopyCtrlString(pszVarName, asCmd, cchLen);

		wchar_t szValue[MAX_PATH];
		wchar_t* pszValue = szValue;
		DWORD cchMax = countof(szValue);
		DWORD nMax = GetEnvironmentVariable(pszVarName, pszValue, cchMax);
		if (nMax >= cchMax)
		{
			cchMax = nMax+1;
			pszValue = (wchar_t*)malloc(cchMax*sizeof(*pszValue));
			nMax = pszValue ? GetEnvironmentVariable(pszVarName, szValue, countof(szValue)) : 0;
		}

		if (nMax)
		{
			TODO("Process here ANSI colors TOO! But now it will be 'reentrance'?");
			WriteText(pfnWriteConsoleW, mh_WriteOutput, pszValue, nMax, &cchMax);
		}

		if (pszValue && pszValue != szValue)
			free(pszValue);
		free(pszVarName);
	}
}

// ESC ] 9 ; 9 ; "cwd" ST        Inform ConEmu about shell current working directory
void CEAnsi::DoSendCWD(LPCWSTR asCmd, INT_PTR cchLen)
{
	// We need zero-terminated string
	wchar_t* pszCWD = (wchar_t*)malloc((cchLen + 1)*sizeof(*asCmd));

	if (pszCWD)
	{
		EscCopyCtrlString(pszCWD, asCmd, cchLen);

		// Sends CECMD_STORECURDIR into RConServer
		SendCurrentDirectory(ghConWnd, pszCWD);

		free(pszCWD);
	}
}


BOOL CEAnsi::ReportString(LPCWSTR asRet)
{
	if (!asRet || !*asRet)
		return FALSE;
	INPUT_RECORD ir[16] = {};
	int nLen = lstrlen(asRet);
	INPUT_RECORD* pir = (nLen <= (int)countof(ir)) ? ir : (INPUT_RECORD*)calloc(nLen,sizeof(INPUT_RECORD));
	if (!pir)
		return FALSE;

	INPUT_RECORD* p = pir;
	LPCWSTR pc = asRet;
	for (int i = 0; i < nLen; i++, p++, pc++)
	{
		p->EventType = KEY_EVENT;
		p->Event.KeyEvent.bKeyDown = TRUE;
		p->Event.KeyEvent.wRepeatCount = 1;
		p->Event.KeyEvent.uChar.UnicodeChar = *pc;
	}

	DWORD nWritten = 0;
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	BOOL bSuccess = WriteConsoleInput(hIn, pir, nLen, &nWritten) && (nWritten == nLen);

	if (pir != ir)
		free(pir);
	return bSuccess;
}

void CEAnsi::ReportConsoleTitle()
{
	wchar_t sTitle[MAX_PATH*2+6] = L"\x1B]l";
	wchar_t* p = sTitle+3;
	_ASSERTEX(lstrlen(sTitle)==3);

	DWORD nTitle = GetConsoleTitle(sTitle+3, MAX_PATH*2);
	p = sTitle+3+min(nTitle,MAX_PATH*2);
	*(p++) = L'\x1B';
	*(p++) = L'\\';
	*(p++) = 0;

	ReportString(sTitle);
}

BOOL CEAnsi::WriteAnsiCodes(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten)
{
	BOOL lbRc = TRUE, lbApply = FALSE;
	LPCWSTR lpEnd = (lpBuffer + nNumberOfCharsToWrite);
	AnsiEscCode Code = {};
	wchar_t szPreDump[CEAnsi_MaxPrevPart];
	DWORD cchPrevPart;

	GetFeatures(NULL, &mb_SuppressBells);

	// Store this pointer
	pfnWriteConsoleW = _WriteConsoleW;
	// Ans current output handle
	mh_WriteOutput = hConsoleOutput;

	//ExtWriteTextParm write = {sizeof(write), ewtf_Current, hConsoleOutput};
	//write.Private = _WriteConsoleW;

	while (lpBuffer < lpEnd)
	{
		LPCWSTR lpStart = NULL, lpNext = NULL; // Required to be NULL-initialized

		// '^' is ESC
		// ^[0;31;47m   $E[31;47m   ^[0m ^[0;1;31;47m  $E[1;31;47m  ^[0m

		cchPrevPart = 0;

		int iEsc = NextEscCode(lpBuffer, lpEnd, szPreDump, cchPrevPart, lpStart, lpNext, Code);

		if (cchPrevPart)
		{
			if (lbApply)
			{
				ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
				lbApply = FALSE;
			}

			lbRc = WriteText(_WriteConsoleW, hConsoleOutput, szPreDump, cchPrevPart, lpNumberOfCharsWritten);
			if (!lbRc)
				goto wrap;
		}

		if (iEsc != 0)
		{
			if (lpStart > lpBuffer)
			{
				_ASSERTEX((lpStart-lpBuffer) < (INT_PTR)nNumberOfCharsToWrite);

				if (lbApply)
				{
					ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
					lbApply = FALSE;
				}

				DWORD nWrite = (DWORD)(lpStart - lpBuffer);
				//lbRc = WriteText(_WriteConsoleW, hConsoleOutput, lpBuffer, nWrite, lpNumberOfCharsWritten);
				lbRc = WriteText(_WriteConsoleW, hConsoleOutput, lpBuffer, nWrite, lpNumberOfCharsWritten);
				if (!lbRc)
					goto wrap;
				//write.Buffer = lpBuffer;
				//write.NumberOfCharsToWrite = nWrite;
				//lbRc = ExtWriteText(&write);
				//if (!lbRc)
				//	goto wrap;
				//else
				//{
				//	if (lpNumberOfCharsWritten)
				//		*lpNumberOfCharsWritten = write.NumberOfCharsWritten;
				//	if (write.ScrolledRowsUp > 0)
				//		gDisplayCursor.StoredCursorPos.Y = max(0,((int)gDisplayCursor.StoredCursorPos.Y - (int)write.ScrolledRowsUp));
				//}
			}

			if (iEsc == 1)
			{
				if (Code.Skip)
				{
					DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
				}
				else
				{
					switch (Code.Second)
					{
					case L'[':
						{
							lbApply = TRUE;
							WriteAnsiCode_CSI(_WriteConsoleW, hConsoleOutput, Code, lbApply);

						} // case L'[':
						break;

					case L']':
						{
							lbApply = TRUE;
							WriteAnsiCode_OSC(_WriteConsoleW, hConsoleOutput, Code, lbApply);

						} // case L']':
						break;

					case L'|':
						{
							// vim-xterm-emulation
							lbApply = TRUE;
							WriteAnsiCode_VIM(_WriteConsoleW, hConsoleOutput, Code, lbApply);
						} // case L'|':
						break;

					case L'=':
					case L'>':
						// xterm "ESC ="
						break;

					default:
						DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
					}
				}
			}
		}
		else //if (iEsc != 2) // 2 - means "Esc part stored in buffer"
		{
			_ASSERTEX(iEsc == 0);
			if (lpNext > lpBuffer)
			{
				if (lbApply)
				{
					ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
					lbApply = FALSE;
				}

				DWORD nWrite = (DWORD)(lpNext - lpBuffer);
				//lbRc = WriteText(_WriteConsoleW, hConsoleOutput, lpBuffer, nWrite, lpNumberOfCharsWritten);
				lbRc = WriteText(_WriteConsoleW, hConsoleOutput, lpBuffer, nWrite, lpNumberOfCharsWritten);
				if (!lbRc)
					goto wrap;
				//write.Buffer = lpBuffer;
				//write.NumberOfCharsToWrite = nWrite;
				//lbRc = ExtWriteText(&write);
				//if (!lbRc)
				//	goto wrap;
				//else
				//{
				//	if (lpNumberOfCharsWritten)
				//		*lpNumberOfCharsWritten = write.NumberOfCharsWritten;
				//	if (write.ScrolledRowsUp > 0)
				//		gDisplayCursor.StoredCursorPos.Y = max(0,((int)gDisplayCursor.StoredCursorPos.Y - (int)write.ScrolledRowsUp));
				//}
			}
		}

		if (lpNext > lpBuffer)
		{
			lpBuffer = lpNext;
		}
		else
		{
			_ASSERTEX(lpNext > lpBuffer || lpNext == NULL);
			++lpBuffer;
		}
	}

	if (lbRc && lpNumberOfCharsWritten)
		*lpNumberOfCharsWritten = nNumberOfCharsToWrite;

wrap:
	if (lbApply)
	{
		ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
		lbApply = FALSE;
	}
	return lbRc;
}

void CEAnsi::WriteAnsiCode_CSI(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, AnsiEscCode& Code, BOOL& lbApply)
{
	/*

CSI ? P m h			DEC Private Mode Set (DECSET)
	P s = 4 7 → Use Alternate Screen Buffer (unless disabled by the titeInhibit resource)
	P s = 1 0 4 7 → Use Alternate Screen Buffer (unless disabled by the titeInhibit resource)
	P s = 1 0 4 8 → Save cursor as in DECSC (unless disabled by the titeInhibit resource)
	P s = 1 0 4 9 → Save cursor as in DECSC and use Alternate Screen Buffer, clearing it first (unless disabled by the titeInhibit resource). This combines the effects of the 1 0 4 7 and 1 0 4 8 modes. Use this with terminfo-based applications rather than the 4 7 mode.

CSI ? P m l			DEC Private Mode Reset (DECRST)
	P s = 4 7 → Use Normal Screen Buffer
	P s = 1 0 4 7 → Use Normal Screen Buffer, clearing screen first if in the Alternate Screen (unless disabled by the titeInhibit resource)
	P s = 1 0 4 8 → Restore cursor as in DECRC (unless disabled by the titeInhibit resource)
	P s = 1 0 4 9 → Use Normal Screen Buffer and restore cursor as in DECRC (unless disabled by the titeInhibit resource). This combines the effects of the 1 0 4 7 and 1 0 4 8 modes. Use this with terminfo-based applications rather than the 4 7 mode.


CSI P s @			Insert P s (Blank) Character(s) (default = 1) (ICH)

	*/
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};

	switch (Code.Action) // case sensitive
	{
	case L's':
		// Save cursor position (can not be nested)
		if (GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
			gDisplayCursor.StoredCursorPos = csbi.dwCursorPosition;
		break;

	case L'u':
		// Restore cursor position
		SetConsoleCursorPosition(hConsoleOutput, gDisplayCursor.StoredCursorPos);
		break;

	case L'H': // Set cursor position (1-based)
	case L'f': // Same as 'H'
	case L'A': // Cursor up by N rows
	case L'B': // Cursor right by N cols
	case L'C': // Cursor right by N cols
	case L'D': // Cursor left by N cols
	case L'E': // Moves cursor to beginning of the line n (default 1) lines down.
	case L'F': // Moves cursor to beginning of the line n (default 1) lines up.
	case L'G': // Moves the cursor to column n.
		// Change cursor position
		if (GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
		{
			COORD crNewPos = csbi.dwCursorPosition;

			switch (Code.Action)
			{
			case L'H':
			case L'f':
				// Set cursor position (1-based)
				crNewPos.Y = (Code.ArgC > 0 && Code.ArgV[0]) ? (Code.ArgV[0] - 1) : 0;
				crNewPos.X = (Code.ArgC > 1 && Code.ArgV[1]) ? (Code.ArgV[1] - 1) : 0;
				break;
			case L'A':
				// Cursor up by N rows
				crNewPos.Y -= (Code.ArgC > 0 && Code.ArgV[0]) ? Code.ArgV[0] : 1;
				break;
			case L'B':
				// Cursor down by N rows
				crNewPos.Y += (Code.ArgC > 0 && Code.ArgV[0]) ? Code.ArgV[0] : 1;
				break;
			case L'C':
				// Cursor right by N cols
				crNewPos.X += (Code.ArgC > 0 && Code.ArgV[0]) ? Code.ArgV[0] : 1;
				break;
			case L'D':
				// Cursor left by N cols
				crNewPos.X -= (Code.ArgC > 0 && Code.ArgV[0]) ? Code.ArgV[0] : 1;
				break;
			case L'E':
				// Moves cursor to beginning of the line n (default 1) lines down.
				crNewPos.Y += (Code.ArgC > 0 && Code.ArgV[0]) ? Code.ArgV[0] : 1;
				crNewPos.X = 0;
				break;
			case L'F':
				// Moves cursor to beginning of the line n (default 1) lines up.
				crNewPos.Y -= (Code.ArgC > 0 && Code.ArgV[0]) ? Code.ArgV[0] : 1;
				crNewPos.X = 0;
				break;
			case L'G':
				// Moves the cursor to column n.
				crNewPos.X = (Code.ArgC > 0) ? (Code.ArgV[0] - 1) : 0;
				break;
			#ifdef _DEBUG
			default:
				_ASSERTEX(FALSE && "Missed (sub)case value!");
			#endif
			}

			// Check Row
			if (crNewPos.Y < 0)
				crNewPos.Y = 0;
			else if (crNewPos.Y >= csbi.dwSize.Y)
				crNewPos.Y = csbi.dwSize.Y - 1;
			// Check Col
			if (crNewPos.X < 0)
				crNewPos.X = 0;
			else if (crNewPos.X >= csbi.dwSize.X)
				crNewPos.X = csbi.dwSize.X - 1;
			// Goto
            SetConsoleCursorPosition(hConsoleOutput, crNewPos);

			if (gbIsVimProcess)
				gbIsVimAnsi = true;
		} // case L'H': case L'f': case 'A': case L'B': case L'C': case L'D':
		break;

	case L'J': // Clears part of the screen
		// Clears the screen and moves the cursor to the home position (line 0, column 0).
		if (GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
		{
			if (lbApply)
			{
				// Apply default color before scrolling!
				ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
				lbApply = FALSE;
			}

			int nCmd = (Code.ArgC > 0) ? Code.ArgV[0] : 0;
			COORD cr0 = {};
			int nChars = 0;

			switch (nCmd)
			{
			case 0:
				// clear from cursor to end of screen
				cr0 = csbi.dwCursorPosition;
				nChars = (csbi.dwSize.X - csbi.dwCursorPosition.X)
					+ csbi.dwSize.X * (csbi.dwSize.Y - csbi.dwCursorPosition.Y - 1);
				break;
			case 1:
				// clear from cursor to beginning of the screen
				nChars = csbi.dwCursorPosition.X + 1
					+ csbi.dwSize.X * csbi.dwCursorPosition.Y;
				break;
			case 2:
				// clear entire screen and moves cursor to upper left
				nChars = csbi.dwSize.X * csbi.dwSize.Y;
				break;
			default:
				DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
			}

			if (lbApply)
			{
				// ViM: need to fill whole screen with selected background color, so Apply attributes
				ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
				lbApply = FALSE;
			}

			if (nChars > 0)
			{
				ExtFillOutputParm fill = {sizeof(fill), efof_Current|efof_Attribute|efof_Character,
					hConsoleOutput, {}, L' ', cr0, (DWORD)nChars};
				ExtFillOutput(&fill);
				//DWORD nWritten = 0;
				//FillConsoleOutputAttribute(hConsoleOutput, GetDefaultTextAttr(), nChars, cr0, &nWritten);
				//FillConsoleOutputCharacter(hConsoleOutput, L' ', nChars, cr0, &nWritten);
			}

			if (nCmd == 2)
			{
				SetConsoleCursorPosition(hConsoleOutput, cr0);
			}

			//TODO("Need to clear attributes?");
			//ReSetDisplayParm(hConsoleOutput, TRUE/*bReset*/, TRUE/*bApply*/);
		} // case L'J':
		break;

	case L'K': // Erases part of the line
		// Clears all characters from the cursor position to the end of the line
		// (including the character at the cursor position).
		if (GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
		{
			if (lbApply)
			{
				// Apply default color before scrolling!
				ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
				lbApply = FALSE;
			}

			TODO("Need to clear attributes?");
			int nChars = 0;
			int nCmd = (Code.ArgC > 0) ? Code.ArgV[0] : 0;
			COORD cr0 = csbi.dwCursorPosition;

			switch (nCmd)
			{
			case 0: // clear from cursor to the end of the line
				nChars = csbi.dwSize.X - csbi.dwCursorPosition.X - 1;
				break;
			case 1: // clear from cursor to beginning of the line
				cr0.X = cr0.Y = 0;
				nChars = csbi.dwCursorPosition.X + 1;
				break;
			case 2: // clear entire line
				cr0.X = cr0.Y = 0;
				nChars = csbi.dwSize.X;
				break;
			default:
				DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
			}


			if (nChars > 0)
			{
				ExtFillOutputParm fill = {sizeof(fill), efof_Current|efof_Attribute|efof_Character,
					hConsoleOutput, {}, L' ', cr0, (DWORD)nChars };
				ExtFillOutput(&fill);
				//DWORD nWritten = 0;
				//FillConsoleOutputAttribute(hConsoleOutput, GetDefaultTextAttr(), nChars, cr0, &nWritten);
				//FillConsoleOutputCharacter(hConsoleOutput, L' ', nChars, cr0, &nWritten);
			}
		} // case L'K':
		break;

	case L'r':
		//\027[Pt;Pbr
		//
		//Pt is the number of the top line of the scrolling region;
		//Pb is the number of the bottom line of the scrolling region
		// and must be greater than Pt.
		//(The default for Pt is line 1, the default for Pb is the end
		// of the screen)
		//
		if ((Code.ArgC >= 2) && (Code.ArgV[0] >= 1) && (Code.ArgV[1] >= Code.ArgV[0]))
		{
			gDisplayOpt.ScrollRegion = TRUE;
			// Lines are 1-based
			gDisplayOpt.ScrollStart = Code.ArgV[0];
			gDisplayOpt.ScrollEnd = Code.ArgV[1];
			_ASSERTEX(gDisplayOpt.ScrollStart>=1 && gDisplayOpt.ScrollEnd>=gDisplayOpt.ScrollStart);
		}
		else
		{
			gDisplayOpt.ScrollRegion = FALSE;
		}
		break;

	case L'S':
		// Scroll whole page up by n (default 1) lines. New lines are added at the bottom.
		if (lbApply)
		{
			// Apply default color before scrolling!
			ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
			lbApply = FALSE;
		}
		ScrollScreen(hConsoleOutput, (Code.ArgC > 0 && Code.ArgV[0] > 0) ? -Code.ArgV[0] : -1);
		break;

	case L'L':
		// Insert P s Line(s) (default = 1) (IL).
		LinesInsert(hConsoleOutput, (Code.ArgC > 0 && Code.ArgV[0] > 0) ? Code.ArgV[0] : 1);
		break;
	case L'M':
		// Delete N Line(s) (default = 1) (DL).
		// This is actually "Scroll UP N line(s) inside defined scrolling region"
		LinesDelete(hConsoleOutput, (Code.ArgC > 0 && Code.ArgV[0] > 0) ? Code.ArgV[0] : 1);
		break;

	case L'@':
		// Insert P s (Blank) Character(s) (default = 1) (ICH).
		DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
		break;
	case L'P':
		// Delete P s Character(s) (default = 1) (DCH).
		DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
		break;

	case L'T':
		// Scroll whole page down by n (default 1) lines. New lines are added at the top.
		if (lbApply)
		{
			// Apply default color before scrolling!
			ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
		}
		TODO("Define scrolling region");
		ScrollScreen(hConsoleOutput, (Code.ArgC > 0 && Code.ArgV[0] > 0) ? Code.ArgV[0] : 1);
		break;

	case L'h':
	case L'l':
		// Set/ReSet Mode
		if (Code.ArgC > 0)
		{
			//ESC [ 3 h
			//       DECCRM (default off): Display control chars.

			//ESC [ 4 h
			//       DECIM (default off): Set insert mode.

			//ESC [ 20 h
			//       LF/NL (default off): Automatically follow echo of LF, VT or FF with CR.

			//ESC [ ? 1 h
			//	  DECCKM (default off): When set, the cursor keys send an ESC O prefix,
			//	  rather than ESC [.

			//ESC [ ? 3 h
			//	  DECCOLM (default off = 80 columns): 80/132 col mode switch.  The driver
			//	  sources note that this alone does not suffice; some user-mode utility
			//	  such as resizecons(8) has to change the hardware registers on the
			//	  console video card.

			//ESC [ ? 5 h
			//	  DECSCNM (default off): Set reverse-video mode.

			//ESC [ ? 6 h
			//	  DECOM (default off): When set, cursor addressing is relative to the
			//	  upper left corner of the scrolling region.


			//ESC [ ? 8 h
			//	  DECARM (default on): Set keyboard autorepeat on.

			//ESC [ ? 9 h
			//	  X10 Mouse Reporting (default off): Set reporting mode to 1 (or reset to
			//	  0) -- see below.

			//ESC [ ? 25 h
			//	  DECTECM (default on): Make cursor visible.

			//ESC [ ? 1000 h
			//	  X11 Mouse Reporting (default off): Set reporting mode to 2 (or reset to
			//	  0) -- see below.

			switch (Code.ArgV[0])
			{
			case 1:
				_ASSERTEX(Code.PvtLen==1 && Code.Pvt[0]==L'?');
				gDisplayCursor.CursorKeysApp = (Code.Action == L'h');
				TODO("What need to send to APP input instead of VK_xxx? (vim.exe)");
				if (gbIsVimProcess)
				{
					TODO("Need to find proper way for activation alternative buffer from ViM?");
					if (Code.Action == L'h')
					{
						StartVimTerm(false);
					}
					else
					{
						StopVimTerm();
					}
				}
				break;
			case 3:
				gDisplayOpt.ShowRawAnsi = (Code.Action == L'h');
				break;
			case 7:
				//ESC [ ? 7 h
				//	  DECAWM (default off): Set autowrap on.  In this mode, a graphic
				//	  character emitted after column 80 (or column 132 of DECCOLM is on)
				//	  forces a wrap to the beginning of the following line first.
				//ESC [ = 7 h
				//    Enables line wrapping
				//ESC [ 7 ; _col_ h
				//    Our extension. _col_ - wrap at column (1-based), default = 80
				if ((gDisplayOpt.WrapWasSet = (Code.Action == L'h')))
				{
					gDisplayOpt.WrapAt = ((Code.ArgC > 1) && (Code.ArgV[1] > 0)) ? Code.ArgV[1] : 80;
				}
				break;
			case 20:
				// Ignored for now
				gDisplayOpt.AutoLfNl = (Code.Action == L'h');
				break;
			case 25:
				{
					CONSOLE_CURSOR_INFO ci = {};
					if (GetConsoleCursorInfo(hConsoleOutput, &ci))
					{
						ci.bVisible = (Code.Action == L'h');
						SetConsoleCursorInfo(hConsoleOutput, &ci);
					}
				}
				break;
			default:
				DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
			}

			//switch (Code.ArgV[0])
			//{
			//case 0: case 1:
			//	// 40x25
			//	if ((gDisplayOpt.WrapWasSet = (Code.Action == L'h')))
			//	{
			//		gDisplayOpt.WrapAt = 40;
			//	}
			//	break;
			//case 2: case 3:
			//	// 80x25
			//	if ((gDisplayOpt.WrapWasSet = (Code.Action == L'h')))
			//	{
			//		gDisplayOpt.WrapAt = 80;
			//	}
			//	break;
			//case 7:
			//	{
			//		DWORD Mode = 0;
			//		GetConsoleMode(hConsoleOutput, &Mode);
			//		if (Code.Action == L'h')
			//			Mode |= ENABLE_WRAP_AT_EOL_OUTPUT;
			//		else
			//			Mode &= ~ENABLE_WRAP_AT_EOL_OUTPUT;
			//		SetConsoleMode(hConsoleOutput, Mode);
			//	} // enable/disable line wrapping
			//	break;
			//}
		}
		else
		{
			DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
		}
		break; // case L'h': case L'l':

	case L'n':
		if (Code.ArgC > 0)
		{
			switch (*Code.ArgV)
			{
			case 5:
				//ESC [ 5 n
				//      Device status report (DSR): Answer is ESC [ 0 n (Terminal OK).
				//
				ReportString(L"\x1B[0n");
				break;
			case 6:
				//ESC [ 6 n
				//      Cursor position report (CPR): Answer is ESC [ y ; x R, where x,y is the
				//      cursor location.
				if (GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
				{
					wchar_t sCurInfo[32];
					msprintf(sCurInfo, countof(sCurInfo),
						L"\x1B[%u;%uR",
						csbi.dwCursorPosition.Y+1, csbi.dwCursorPosition.X+1);
					ReportString(sCurInfo);
				}
				break;
			default:
				DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
			}
		}
		else
		{
			DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
		}
		break;

	case L'm':
		// Set display mode (colors, fonts, etc.)
		if (!Code.ArgC)
		{
			ReSetDisplayParm(hConsoleOutput, TRUE, FALSE);
		}
		else
		{
			for (int i = 0; i < Code.ArgC; i++)
			{
				switch (Code.ArgV[i])
				{
				case 0:
					ReSetDisplayParm(hConsoleOutput, TRUE, FALSE);
					break;
				case 1:
					gDisplayParm.BrightOrBold = TRUE;
					gDisplayParm.WasSet = TRUE;
					break;
				case 2:
					// Faint, decreased intensity (ISO 6429)
				case 22:
					// Normal (neither bold nor faint).
					gDisplayParm.BrightOrBold = FALSE;
					gDisplayParm.WasSet = TRUE;
					break;
				case 3:
					gDisplayParm.ItalicOrInverse = TRUE;
					gDisplayParm.WasSet = TRUE;
					break;
				case 23:
					// Not italicized (ISO 6429)
					gDisplayParm.ItalicOrInverse = FALSE;
					gDisplayParm.WasSet = TRUE;
					break;
				case 4: // Underlined
				case 5: // Blink
					TODO("Check standard");
					gDisplayParm.BackOrUnderline = TRUE;
					gDisplayParm.WasSet = TRUE;
					break;
				case 24:
					// Not underlined
					gDisplayParm.BackOrUnderline = FALSE;
					gDisplayParm.WasSet = TRUE;
					break;
				case 7:
					// Reverse video
					TODO("Check standard");
					break;
				case 27:
					// Positive (not inverse)
					break;
				case 25:
					// Steady (not blinking)
					break;
				case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37:
					gDisplayParm.TextColor = (Code.ArgV[i] - 30);
					gDisplayParm.Text256 = FALSE;
					gDisplayParm.WasSet = TRUE;
					break;
				case 38:
					// xterm-256 colors
					// ESC [ 38 ; 5 ; I m -- set foreground to I (0..255) color from xterm palette
					if (((i+2) < Code.ArgC) && (Code.ArgV[i+1] == 5))
					{
						gDisplayParm.TextColor = (Code.ArgV[i+2] & 0xFF);
						gDisplayParm.Text256 = 1;
						gDisplayParm.WasSet = TRUE;
						i += 2;
					}
					// xterm-256 colors
					// ESC [ 38 ; 2 ; R ; G ; B m -- set foreground to RGB(R,G,B) 24-bit color
					else if (((i+4) < Code.ArgC) && (Code.ArgV[i+1] == 2))
					{
						gDisplayParm.TextColor = RGB((Code.ArgV[i+2] & 0xFF),(Code.ArgV[i+3] & 0xFF),(Code.ArgV[i+4] & 0xFF));
						gDisplayParm.Text256 = 2;
						gDisplayParm.WasSet = TRUE;
						i += 4;
					}
					break;
				case 39:
					// Reset
					gDisplayParm.TextColor = GetDefaultTextAttr() & 0xF;
					gDisplayParm.Text256 = FALSE;
					gDisplayParm.WasSet = TRUE;
					break;
				case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47:
					gDisplayParm.BackColor = (Code.ArgV[i] - 40);
					gDisplayParm.Back256 = FALSE;
					gDisplayParm.WasSet = TRUE;
					break;
				case 48:
					// xterm-256 colors
					// ESC [ 48 ; 5 ; I m -- set background to I (0..255) color from xterm palette
					if (((i+2) < Code.ArgC) && (Code.ArgV[i+1] == 5))
					{
						gDisplayParm.BackColor = (Code.ArgV[i+2] & 0xFF);
						gDisplayParm.Back256 = 1;
						gDisplayParm.WasSet = TRUE;
						i += 2;
					}
					// xterm-256 colors
					// ESC [ 48 ; 2 ; R ; G ; B m -- set background to RGB(R,G,B) 24-bit color
					else if (((i+4) < Code.ArgC) && (Code.ArgV[i+1] == 2))
					{
						gDisplayParm.BackColor = RGB((Code.ArgV[i+2] & 0xFF),(Code.ArgV[i+3] & 0xFF),(Code.ArgV[i+4] & 0xFF));
						gDisplayParm.Back256 = 2;
						gDisplayParm.WasSet = TRUE;
						i += 4;
					}
					break;
				case 49:
					// Reset
					gDisplayParm.BackColor = (GetDefaultTextAttr() & 0xF0) >> 4;
					gDisplayParm.Back256 = FALSE;
					gDisplayParm.WasSet = TRUE;
					break;
				case 90: case 91: case 92: case 93: case 94: case 95: case 96: case 97:
					gDisplayParm.TextColor = (Code.ArgV[i] - 90);
					gDisplayParm.Text256 = FALSE;
					gDisplayParm.BrightOrBold = TRUE;
					gDisplayParm.WasSet = TRUE;
					break;
				case 100: case 101: case 102: case 103: case 104: case 105: case 106: case 107:
					gDisplayParm.BackColor = (Code.ArgV[i] - 100);
					gDisplayParm.Back256 = FALSE;
					gDisplayParm.BackOrUnderline = TRUE;
					gDisplayParm.WasSet = TRUE;
					break;
				default:
					DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
				}
			}
		}
		break; // "[...m"

	case L't':
		if (Code.ArgC > 0 && Code.ArgC <= 3)
		{
			wchar_t sCurInfo[32];
			for (int i = 0; i < Code.ArgC; i++)
			{
				switch (Code.ArgV[i])
				{
				case 18:
				case 19:
					// 1 8 --> Report the size of the text area in characters as CSI 8 ; height ; width t
					// 1 9 --> Report the size of the screen in characters as CSI 9 ; height ; width t
					if (GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
					{
						msprintf(sCurInfo, countof(sCurInfo),
							L"\x1B[8;%u;%ut",
							csbi.srWindow.Bottom-csbi.srWindow.Top+1, csbi.srWindow.Right-csbi.srWindow.Left+1);
						ReportString(sCurInfo);
					}
					break;
				case 21:
					// 2 1 --> Report xterm window�s title as OSC l title ST
					ReportConsoleTitle();
					break;
				default:
					TODO("ANSI: xterm window manipulation");
					//Window manipulation (from dtterm, as well as extensions). These controls may be disabled using the allowWindowOps resource. Valid values for the first (and any additional parameters) are:
					// 1 --> De-iconify window.
					// 2 --> Iconify window.
					// 3 ; x ; y --> Move window to [x, y].
					// 4 ; height ; width --> Resize the xterm window to height and width in pixels.
					// 5 --> Raise the xterm window to the front of the stacking order.
					// 6 --> Lower the xterm window to the bottom of the stacking order.
					// 7 --> Refresh the xterm window.
					// 8 ; height ; width --> Resize the text area to [height;width] in characters.
					// 9 ; 0 --> Restore maximized window.
					// 9 ; 1 --> Maximize window (i.e., resize to screen size).
					// 1 1 --> Report xterm window state. If the xterm window is open (non-iconified), it returns CSI 1 t . If the xterm window is iconified, it returns CSI 2 t .
					// 1 3 --> Report xterm window position as CSI 3 ; x; y t
					// 1 4 --> Report xterm window in pixels as CSI 4 ; height ; width t
					// 2 0 --> Report xterm window�s icon label as OSC L label ST
					// >= 2 4 --> Resize to P s lines (DECSLPP)
					DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
				}
			}
		}
		else
		{
			DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
		}
		break;

	case L'c':
		// echo -e "\e[>c"
		if ((Code.PvtLen == 1) && (Code.Pvt[0] == L'>')
			&& ((Code.ArgC < 1) || (Code.ArgV[0] == 0)))
		{
			// P s = 0 or omitted -> request the terminal's identification code.
			wchar_t szVerInfo[64];
			// this will be "ESC > 67 ; build ; 0 c"
			// 67 is ASCII code of 'C' (ConEmu, yeah)
			// Other terminals report examples: MinTTY -> 77, rxvt -> 82, screen -> 83
			msprintf(szVerInfo, countof(szVerInfo), L"\x1B>%u;%u;0c", (int)'C', MVV_1*10000+MVV_2*100+MVV_3);
			ReportString(szVerInfo);
		}
		// echo -e "\e[c"
		else if ((Code.ArgC < 1) || (Code.ArgV[0] == 0))
		{
			// Report "VT100 with Advanced Video Option"
			ReportString(L"\x1B[?1;2c");
		}
		else
		{
			DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
		}
		break;

	case L'X':
		// CSI P s X:  Erase P s Character(s) (default = 1) (ECH)
		if (GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
		{
			if (lbApply)
			{
				// Apply default color before scrolling!
				ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
				lbApply = FALSE;
			}

			int nCount = (Code.ArgC > 0) ? Code.ArgV[0] : 1;
			int nScreenLeft = csbi.dwSize.X - csbi.dwCursorPosition.X - 1 + (csbi.dwSize.X * (csbi.dwSize.Y - csbi.dwCursorPosition.Y - 1));
			int nChars = min(nCount,nScreenLeft);
			COORD cr0 = csbi.dwCursorPosition;

			if (nChars > 0)
			{
				ExtFillOutputParm fill = {sizeof(fill), efof_Current|efof_Attribute|efof_Character,
					hConsoleOutput, {}, L' ', cr0, (DWORD)nChars };
				ExtFillOutput(&fill);
			}
		} // case L'X':
		break;

	default:
		DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
	} // switch (Code.Action)
}

void CEAnsi::WriteAnsiCode_OSC(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, AnsiEscCode& Code, BOOL& lbApply)
{
	if (!Code.ArgSZ)
	{
		DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
		return;
	}
	//ESC ] 0 ; txt ST        Set icon name and window title to txt.
	//ESC ] 1 ; txt ST        Set icon name to txt.
	//ESC ] 2 ; txt ST        Set window title to txt.
	//ESC ] 4 ; num; txt ST   Set ANSI color num to txt.
	//ESC ] 10 ; txt ST       Set dynamic text color to txt.
	//ESC ] 4 6 ; name ST     Change log file to name (normally disabled
	//					      by a compile-time option)
	//ESC ] 5 0 ; fn ST       Set font to fn.

	switch (*Code.ArgSZ)
	{
	case L'0':
	case L'1':
	case L'2':
		if (Code.ArgSZ[1] == L';' && Code.ArgSZ[2])
		{
			wchar_t* pszNewTitle = (wchar_t*)malloc(sizeof(wchar_t)*(Code.cchArgSZ));
			if (pszNewTitle)
			{
				EscCopyCtrlString(pszNewTitle, Code.ArgSZ+2, Code.cchArgSZ-2);
				SetConsoleTitle(*pszNewTitle ? pszNewTitle : gsInitConTitle);
				free(pszNewTitle);
			}
		}
		break;

	case L'4':
		// the following is suggestion for exact palette colors
		// bug we are using standard xterm palette or truecolor 24bit palette
		_ASSERTEX(Code.ArgSZ[1] == L';');
		break;

	case L'9':
		// ConEmu specific
		// ESC ] 9 ; 1 ; ms ST           Sleep. ms - milliseconds
		// ESC ] 9 ; 2 ; "txt" ST        Show GUI MessageBox ( txt ) for dubug purposes
		// ESC ] 9 ; 3 ; "txt" ST        Set TAB text
		// ESC ] 9 ; 4 ; st ; pr ST      When _st_ is 0: remove progress. When _st_ is 1: set progress value to _pr_ (number, 0-100). When _st_ is 2: set error state in progress on Windows 7 taskbar
		// ESC ] 9 ; 5 ST                Wait for ENTER/SPACE/ESC. Set EnvVar "ConEmuWaitKey" to ENTER/SPACE/ESC on exit.
		// ESC ] 9 ; 6 ; "txt" ST        Execute GuiMacro. Set EnvVar "ConEmuMacroResult" on exit.
		// ESC ] 9 ; 7 ; "cmd" ST        Run some process with arguments
		// ESC ] 9 ; 8 ; "env" ST        Output value of environment variable
		// ESC ] 9 ; 9 ; "cwd" ST        Inform ConEmu about shell current working directory
		// -- You may specify timeout _s_ in seconds. - �� ��������
		if (Code.ArgSZ[1] == L';')
		{
			if (Code.ArgSZ[2] == L'1' && Code.ArgSZ[3] == L';')
			{
				DoSleep(Code.ArgSZ+4);
			}
			else if (Code.ArgSZ[2] == L'2' && Code.ArgSZ[3] == L';')
			{
				DoMessage(Code.ArgSZ+4, Code.cchArgSZ - 4);
			}
			else if (Code.ArgSZ[2] == L'3' && Code.ArgSZ[3] == L';')
			{
				CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_SETTABTITLE, sizeof(CESERVER_REQ_HDR)+sizeof(wchar_t)*(Code.cchArgSZ));
				if (pIn)
				{
					EscCopyCtrlString((wchar_t*)pIn->wData, Code.ArgSZ+4, Code.cchArgSZ-4);
					CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
					ExecuteFreeResult(pIn);
					ExecuteFreeResult(pOut);
				}
			}
			else if (Code.ArgSZ[2] == L'4')
			{
				WORD st = 0, pr = 0;
				LPCWSTR pszName = NULL;
				if (Code.ArgSZ[3] == L';')
				{
					switch (Code.ArgSZ[4])
					{
					case L'0':
						break;
					case L'1': // Normal
					case L'2': // Error
						st = Code.ArgSZ[4] - L'0';
						if (Code.ArgSZ[5] == L';')
						{
							LPCWSTR pszValue = Code.ArgSZ + 6;
							pr = NextNumber(pszValue);
						}
						break;
					case L'3':
						st = 3; // Indeterminate
						break;
					case L'4':
					case L'5':
						st = Code.ArgSZ[4] - L'0';
						pszName = (Code.ArgSZ[5] == L';') ? (Code.ArgSZ + 6) : NULL;
						break;
					}
				}
				GuiSetProgress(st,pr,pszName);
			}
			else if (Code.ArgSZ[2] == L'5')
			{
				//int s = 0;
				//if (Code.ArgSZ[3] == L';')
				//	s = NextNumber(Code.ArgSZ+4);
				BOOL bSucceeded = FALSE;
				DWORD nRead = 0;
				INPUT_RECORD r = {};
				HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
				//DWORD nStartTick = GetTickCount();
				while ((bSucceeded = ReadConsoleInput(hIn, &r, 1, &nRead)) && nRead)
				{
					if ((r.EventType == KEY_EVENT) && r.Event.KeyEvent.bKeyDown)
					{
						if ((r.Event.KeyEvent.wVirtualKeyCode == VK_RETURN)
							|| (r.Event.KeyEvent.wVirtualKeyCode == VK_SPACE)
							|| (r.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE))
						{
							break;
						}
					}
				}
				if (bSucceeded && ((r.Event.KeyEvent.wVirtualKeyCode == VK_RETURN)
							|| (r.Event.KeyEvent.wVirtualKeyCode == VK_SPACE)
							|| (r.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)))
				{
					SetEnvironmentVariable(ENV_CONEMU_WAITKEY_W,
						(r.Event.KeyEvent.wVirtualKeyCode == VK_RETURN) ? L"RETURN" :
						(r.Event.KeyEvent.wVirtualKeyCode == VK_SPACE)  ? L"SPACE" :
						(r.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) ? L"ESC" :
						L"???");
				}
				else
				{
					SetEnvironmentVariable(ENV_CONEMU_WAITKEY_W, L"");
				}
			}
			else if (Code.ArgSZ[2] == L'6' && Code.ArgSZ[3] == L';')
			{
				CESERVER_REQ* pOut = NULL;
				CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_GUIMACRO, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_GUIMACRO)+sizeof(wchar_t)*(Code.cchArgSZ));

				if (pIn)
				{
					EscCopyCtrlString(pIn->GuiMacro.sMacro, Code.ArgSZ+4, Code.cchArgSZ-4);
					pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
				}

				// EnvVar "ConEmuMacroResult"
				SetEnvironmentVariable(CEGUIMACRORETENVVAR, pOut && pOut->GuiMacro.nSucceeded ? pOut->GuiMacro.sMacro : NULL);

				ExecuteFreeResult(pOut);
				ExecuteFreeResult(pIn);
			}
			else if (Code.ArgSZ[2] == L'7' && Code.ArgSZ[3] == L';')
			{
				DoProcess(Code.ArgSZ+4, Code.cchArgSZ - 4);
			}
			else if (Code.ArgSZ[2] == L'8' && Code.ArgSZ[3] == L';')
			{
				if (lbApply)
				{
					ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
					lbApply = FALSE;
				}
				DoPrintEnv(Code.ArgSZ+4, Code.cchArgSZ - 4);
			}
			else if (Code.ArgSZ[2] == L'9' && Code.ArgSZ[3] == L';')
			{
				DoSendCWD(Code.ArgSZ+4, Code.cchArgSZ - 4);
			}
		}
		break;

	default:
		DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
	}
}

void CEAnsi::WriteAnsiCode_VIM(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, AnsiEscCode& Code, BOOL& lbApply)
{
	if (!gbWasXTermOutput && !gnWriteProcessed)
	{
		gbWasXTermOutput = true;
		CEAnsi::StartXTermMode(true);
	}

	switch (Code.Action)
	{
	case L'm':
		// Set xterm display modes (colors, fonts, etc.)
		if (!Code.ArgC)
		{
			//ReSetDisplayParm(hConsoleOutput, TRUE, FALSE);
			DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
		}
		else
		{
			for (int i = 0; i < Code.ArgC; i++)
			{
				switch (Code.ArgV[i])
				{
				case 7:
					gDisplayParm.BrightOrBold = FALSE;
					gDisplayParm.ItalicOrInverse = FALSE;
					gDisplayParm.BackOrUnderline = FALSE;
					gDisplayParm.Inverse = FALSE;
					gDisplayParm.WasSet = TRUE;
					break;
				case 15:
					gDisplayParm.BrightOrBold = TRUE;
					gDisplayParm.WasSet = TRUE;
					break;
				case 112:
					gDisplayParm.Inverse = TRUE;
					gDisplayParm.WasSet = TRUE;
					break;
				case 143:
					// What is this?
					break;
				default:
					DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
				}
			}
		}
		break; // "|...m"
	}
}

BOOL WINAPI CEAnsi::OnSetConsoleMode(HANDLE hConsoleHandle, DWORD dwMode)
{
	typedef BOOL (WINAPI* OnSetConsoleMode_t)(HANDLE hConsoleHandle, DWORD dwMode);
	ORIGINALFAST(SetConsoleMode);
	//BOOL bMainThread = FALSE; // поток не важен
	BOOL lbRc = FALSE;

	#if 0
	if (!(dwMode & ENABLE_PROCESSED_OUTPUT))
	{
		_ASSERTEX((dwMode & ENABLE_PROCESSED_OUTPUT)==ENABLE_PROCESSED_OUTPUT);
	}
	#endif

	if (gbIsVimProcess)
	{
		if ((dwMode & (ENABLE_WRAP_AT_EOL_OUTPUT|ENABLE_PROCESSED_OUTPUT)) != (ENABLE_WRAP_AT_EOL_OUTPUT|ENABLE_PROCESSED_OUTPUT))
		{
			if (IsOutputHandle(hConsoleHandle))
			{
				dwMode |= ENABLE_WRAP_AT_EOL_OUTPUT|ENABLE_PROCESSED_OUTPUT;
			}
			else
			{
				dwMode |= ENABLE_WINDOW_INPUT;
			}
		}
	}


	lbRc = F(SetConsoleMode)(hConsoleHandle, dwMode);

	return lbRc;
}


/*
ViM need some hacks in current ConEmu versions
This is because
1) 256 colors mode requires NO scroll buffer
2) can't find ATM legal way to switch Alternative/Primary buffer by request from ViM
*/

//static int  gnVimTermWasChangedBuffer = 0;
//static bool gbVimTermWasChangedBuffer = false;

void CEAnsi::StartVimTerm(bool bFromDllStart)
{
	if (/*bFromDllStart ||*/ gbVimTermWasChangedBuffer)
		return;

	TODO("Переделать на команду сервера?");

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (GetConsoleScreenBufferInfoCached(hOut, &csbi, TRUE))
	{
		// -- Turn on "alternative" buffer even if not scrolling exist now
		//if (csbi.dwSize.Y > (csbi.srWindow.Bottom - csbi.srWindow.Top + 1))
		{
			CESERVER_REQ *pIn = NULL, *pOut = NULL;
			pIn = ExecuteNewCmd(CECMD_ALTBUFFER,sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_ALTBUFFER));
			if (pIn)
			{
				pIn->AltBuf.AbFlags = abf_BufferOff|abf_SaveContents;
				pOut = ExecuteSrvCmd(gnServerPID, pIn, ghConWnd);
				if (pOut)
				{
					TODO("BufferWidth");
					gnVimTermWasChangedBuffer = csbi.dwSize.Y;
					gbVimTermWasChangedBuffer = true;
				}
				ExecuteFreeResult(pIn);
				ExecuteFreeResult(pOut);
			}

			//COORD crNoScroll = {csbi.srWindow.Right-csbi.srWindow.Left+1, csbi.srWindow.Bottom-csbi.srWindow.Top+1};
			//BOOL bRc = SetConsoleScreenBufferSize(hOut, crNoScroll);
			//if (bRc)
			//{
			//	gnVimTermWasChangedBuffer = csbi.dwSize.Y;
			//	gbVimTermWasChangedBuffer = true;
			//}
			//else
			//{
			//	_ASSERTEX(FALSE && "Failed to reset buffer height to ViM");
			//}
		}
	}
}

void CEAnsi::StopVimTerm()
{
	if (gbWasXTermOutput)
	{
		gbWasXTermOutput = false;
		CEAnsi::StartXTermMode(false);
	}

	if (!gbVimTermWasChangedBuffer)
		return;

	// Однократно
	gbVimTermWasChangedBuffer = false;

	// Сброс расширенных атрибутов!
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (GetConsoleScreenBufferInfoCached(hOut, &csbi, TRUE))
	{
		WORD nDefAttr = GetDefaultTextAttr();
		// Сброс только расширенных атрибутов
		ExtFillOutputParm fill = {sizeof(fill), /*efof_ResetExt|*/efof_Attribute/*|efof_Character*/, hOut,
			{CECF_NONE,(COLORREF)(nDefAttr&0xF),COLORREF((nDefAttr&0xF0)>>4)},
			L' ', {0,0}, (DWORD)(csbi.dwSize.X * csbi.dwSize.Y)};
		ExtFillOutput(&fill);
		CEAnsi* pObj = CEAnsi::Object();
		if (pObj)
			pObj->ReSetDisplayParm(hOut, TRUE, TRUE);
		else
			SetConsoleTextAttribute(hOut, nDefAttr);
	}

	// Восстановление прокрутки и данных
	CESERVER_REQ *pIn = NULL, *pOut = NULL;
	pIn = ExecuteNewCmd(CECMD_ALTBUFFER,sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_ALTBUFFER));
	if (pIn)
	{
		TODO("BufferWidth");
		pIn->AltBuf.AbFlags = abf_BufferOn|abf_RestoreContents;
		pIn->AltBuf.BufferHeight = gnVimTermWasChangedBuffer;
		// Async - нельзя. Иначе cmd может отрисовать prompt раньше чем управится сервер.
		pOut = ExecuteSrvCmd(gnServerPID, pIn, ghConWnd);
		ExecuteFreeResult(pIn);
		ExecuteFreeResult(pOut);
	}

	/*
	TODO("Переделать на команду сервера?");

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (GetConsoleScreenBufferInfoCached(hOut, &csbi, TRUE))
	{
		if (csbi.dwSize.Y < gnVimTermWasChangedBuffer)
		{
			COORD crScroll = {csbi.dwSize.X, gnVimTermWasChangedBuffer};
			BOOL bRc = SetConsoleScreenBufferSize(hOut, crScroll);
			if (!bRc)
			{
				_ASSERTEX(FALSE && "Failed to return scroll!");
			}
		}
	}
	*/
}

void CEAnsi::StartXTermMode(bool bStart)
{
	_ASSERTEX(gbVimTermWasChangedBuffer && (gbWasXTermOutput==bStart));

	CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_STARTXTERM, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
	if (pIn)
	{
		pIn->dwData[0] = bStart ? te_xterm : te_win32;
		CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
		ExecuteFreeResult(pIn);
		ExecuteFreeResult(pOut);
	}
}
