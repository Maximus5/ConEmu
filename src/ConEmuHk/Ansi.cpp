
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

#include <windows.h>
#include <WinError.h>
#include <WinNT.h>
#include <TCHAR.h>
#include "../common/Common.h"
#include "../common/ConEmuCheck.h"
#include "../common/CmdLine.h"
#include "../common/ConsoleAnnotation.h"
#include "../common/HandleKeeper.h"
#include "../common/MConHandle.h"
#include "../common/MSectionSimple.h"
#include "../common/WCodePage.h"
#include "../common/WConsole.h"
#include "../common/WErrGuard.h"
#include "../ConEmu/version.h"

#include "ExtConsole.h"
#include "hlpConsole.h"
#include "SetHook.h"

#include "hkConsole.h"
#include "hkStdIO.h"

///* ***************** */
#include "Ansi.h"
DWORD AnsiTlsIndex = 0;
///* ***************** */

#undef isPressed
#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)

#define ANSI_MAP_CHECK_TIMEOUT 1000

#ifdef _DEBUG
#define DebugString(x) OutputDebugString(x)
#define DebugStringA(x) OutputDebugStringA(x)
#else
#define DebugString(x) //OutputDebugString(x)
#define DebugStringA(x) //OutputDebugStringA(x)
#endif

/* ************ Globals ************ */
extern HMODULE ghOurModule; // Хэндл нашей dll'ки (здесь хуки не ставятся)
#include "MainThread.h"

/* ************ Globals for SetHook ************ */
extern HWND    ghConWnd;      // RealConsole
extern HWND    ghConEmuWnd;   // Root! ConEmu window
extern HWND    ghConEmuWndDC; // ConEmu DC window
extern DWORD   gnGuiPID;
extern wchar_t gsInitConTitle[512];
/* ************ Globals for SetHook ************ */

/* ************ Globals for xTerm/ViM ************ */
TODO("BufferWidth")
typedef DWORD XTermAltBufferFlags;
const XTermAltBufferFlags
	xtb_AltBuffer          = 0x0001,
	xtb_StoredCursor       = 0x0002,
	xtb_StoredScrollRegion = 0x0004,
	xtb_None               = 0;
static struct XTermAltBufferData
{
	XTermAltBufferFlags Flags;
	int    BufferSize;
	COORD  CursorPos;
	SHORT  ScrollStart, ScrollEnd;
} gXTermAltBuffer = {};
/* ************ Globals for xTerm/ViM ************ */

BOOL WINAPI OnCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

// These handles must be registered and released in OnCloseHandle.
HANDLE CEAnsi::ghAnsiLogFile = NULL;
LONG CEAnsi::gnEnterPressed = 0;
bool CEAnsi::gbAnsiLogNewLine = false;
bool CEAnsi::gbAnsiWasNewLine = false;
MSectionSimple* CEAnsi::gcsAnsiLogFile = NULL;

// VIM, etc. Some programs waiting control keys as xterm sequences. Need to inform ConEmu GUI.
bool CEAnsi::gbWasXTermOutput = false;

/* ************ Export ANSI printings ************ */
LONG gnWriteProcessed = 0;
FARPROC CallWriteConsoleW = NULL;
BOOL WINAPI WriteProcessed3(LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, HANDLE hConsoleOutput)
{
	InterlockedIncrement(&gnWriteProcessed);
	DWORD nNumberOfCharsWritten = 0;
	if ((nNumberOfCharsToWrite == (DWORD)-1) && lpBuffer)
	{
		nNumberOfCharsToWrite = lstrlen(lpBuffer);
	}
	BOOL bRc = CEAnsi::OurWriteConsoleW(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, &nNumberOfCharsWritten, NULL);
	if (lpNumberOfCharsWritten) *lpNumberOfCharsWritten = nNumberOfCharsWritten;
	InterlockedDecrement(&gnWriteProcessed);
	return bRc;
}
HANDLE GetStreamHandle(WriteProcessedStream Stream)
{
	HANDLE hConsoleOutput;
	switch (Stream)
	{
	case wps_Output:
		hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE); break;
	case wps_Error:
		hConsoleOutput = GetStdHandle(STD_ERROR_HANDLE); break;
	default:
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
	}
	return hConsoleOutput;
}
BOOL WINAPI WriteProcessed2(LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, WriteProcessedStream Stream)
{
	HANDLE hConsoleOutput = GetStreamHandle(Stream);
	if (!hConsoleOutput || (hConsoleOutput == INVALID_HANDLE_VALUE))
		return FALSE;
	return WriteProcessed3(lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, hConsoleOutput);
}
BOOL WINAPI WriteProcessed(LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten)
{
	return WriteProcessed2(lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, wps_Output);
}
BOOL WINAPI WriteProcessedA(LPCSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, WriteProcessedStream Stream)
{
	BOOL lbRc = FALSE;
	CEAnsi* pObj = NULL;

	ORIGINAL_KRNL(WriteConsoleA);

	if ((nNumberOfCharsToWrite == (DWORD)-1) && lpBuffer)
	{
		nNumberOfCharsToWrite = lstrlenA(lpBuffer);
	}

	// Nothing to write? Or flush buffer?
	if (!lpBuffer || !nNumberOfCharsToWrite || !(int)Stream)
	{
		if (lpNumberOfCharsWritten)
			*lpNumberOfCharsWritten = 0;
		lbRc = TRUE;
		goto fin;
	}

	pObj = CEAnsi::Object();

	if (pObj)
		lbRc = pObj->OurWriteConsoleA(GetStreamHandle(Stream), lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten);
	else
		lbRc = F(WriteConsoleA)(GetStreamHandle(Stream), lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, NULL);

fin:
	return lbRc;
}
/* ************ Export ANSI printings ************ */

void DebugStringUtf8(LPCWSTR asMessage)
{
	#ifdef _DEBUG
	if (!asMessage || !*asMessage)
		return;
	// Only ConEmuC debugger is able to show Utf-8 encoded debug strings
	// So, set bUseUtf8 to false if VS debugger is required
	static bool bUseUtf8 = true;
	if (!bUseUtf8)
	{
		DebugString(asMessage);
		return;
	}
	int iLen = lstrlen(asMessage);
	char szUtf8[200];
	char* pszUtf8 = ((iLen*3+5) < countof(szUtf8)) ? szUtf8 : (char*)malloc(iLen*3+5);
	if (!pszUtf8)
		return;
	pszUtf8[0] = (BYTE)0xEF; pszUtf8[1] = (BYTE)0xBB; pszUtf8[2] = (BYTE)0xBF;
	int iCvt = WideCharToMultiByte(CP_UTF8, 0, asMessage, iLen, pszUtf8+3, iLen*3+1, NULL, NULL);
	if (iCvt > 0)
	{
		_ASSERTE(iCvt < (iLen*3+2));
		pszUtf8[iCvt+3] = 0;
		DebugStringA(pszUtf8);
	}
	if (pszUtf8 != szUtf8)
		free(pszUtf8);
	#endif
}

void CEAnsi::InitAnsiLog(LPCWSTR asFilePath)
{
	// Already initialized?
	if (ghAnsiLogFile)
		return;

	ScopedObject(CLastErrorGuard);

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

	if (gbAnsiLogNewLine || (gnEnterPressed > 0))
	{
		CEAnsi::WriteAnsiLogUtf8("\n", 1);
	}

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

UINT CEAnsi::GetCodePage()
{
	UINT cp = gCpConv.nDefaultCP ? gCpConv.nDefaultCP : GetConsoleOutputCP();
	return cp;
}

BOOL CEAnsi::WriteAnsiLogUtf8(const char* lpBuffer, DWORD nChars)
{
	if (!lpBuffer || !nChars)
		return FALSE;
	BOOL bWrite; DWORD nWritten;
	// Handle multi-thread writers
	// But multi-process writers can't be handled correctly
	MSectionLockSimple lock; lock.Lock(gcsAnsiLogFile, 500);
	SetFilePointer(ghAnsiLogFile, 0, NULL, FILE_END);
	ORIGINAL_KRNL(WriteFile);
	bWrite = F(WriteFile)(ghAnsiLogFile, lpBuffer, nChars, &nWritten, NULL);
	UNREFERENCED_PARAMETER(nWritten);
	gnEnterPressed = 0; gbAnsiLogNewLine = false;
	gbAnsiWasNewLine = (lpBuffer[nChars-1] == '\n');
	return bWrite;
}

// This may be called to log ReadConsoleA result
void CEAnsi::WriteAnsiLogA(LPCSTR lpBuffer, DWORD nChars)
{
	if (!ghAnsiLogFile || !lpBuffer || !nChars)
		return;

	ScopedObject(CLastErrorGuard);

	UINT cp = GetCodePage();
	if (cp == CP_UTF8)
	{
		char* pszBuf = NULL;
		int iEnterShift = 0;
		if (gbAnsiLogNewLine)
		{
			if ((lpBuffer[0] == '\n') || ((nChars > 1) && (lpBuffer[0] == '\r') && (lpBuffer[1] == '\n')))
				gbAnsiLogNewLine = false;
			else if ((pszBuf = (char*)malloc(nChars+1)) != NULL)
			{
				pszBuf[0] = '\n';
				memmove(pszBuf+1, lpBuffer, nChars);
				lpBuffer = pszBuf;
			}
		}

		WriteAnsiLogUtf8(lpBuffer, nChars);

		SafeFree(pszBuf);
	}
	else
	{
		// We don't check here for gbAnsiLogNewLine, because some codepages may even has different codes for CR+LF
		wchar_t sBuf[80*3];
		BOOL bWrite = FALSE;
		DWORD nWritten = 0;
		int nNeed = MultiByteToWideChar(cp, 0, lpBuffer, nChars, NULL, 0);
		if (nNeed < 1)
			return;
		wchar_t* pszBuf = (nNeed <= countof(sBuf)) ? sBuf : (wchar_t*)malloc(nNeed*sizeof(*pszBuf));
		if (!pszBuf)
			return;
		int nLen = MultiByteToWideChar(cp, 0, lpBuffer, nChars, pszBuf, nNeed);
		// Must be OK, but check it
		if (nLen > 0 && nLen <= nNeed)
		{
			WriteAnsiLogW(pszBuf, nLen);
		}
		if (pszBuf != sBuf)
			free(pszBuf);
	}
}

void CEAnsi::WriteAnsiLogW(LPCWSTR lpBuffer, DWORD nChars)
{
	if (!ghAnsiLogFile || !lpBuffer || !nChars)
		return;

	ScopedObject(CLastErrorGuard);

	// Cygwin (in RealConsole mode, not connector) don't write CR+LF to screen,
	// it uses SetConsoleCursorPosition instead after receiving '\n' from readline
	int iEnterShift = 0;
	if (gbAnsiLogNewLine)
	{
		if ((lpBuffer[0] == '\n') || ((nChars > 1) && (lpBuffer[0] == '\r') && (lpBuffer[1] == '\n')))
			gbAnsiLogNewLine = false;
		else
			iEnterShift = 1;
	}

	char sBuf[80*3]; // Will be enough for most cases
	BOOL bWrite = FALSE;
	DWORD nWritten = 0;
	int nNeed = WideCharToMultiByte(CP_UTF8, 0, lpBuffer, nChars, NULL, 0, NULL, NULL);
	if (nNeed < 1)
		return;
	char* pszBuf = ((nNeed + iEnterShift + 1) <= countof(sBuf)) ? sBuf : (char*)malloc(nNeed+iEnterShift+1);
	if (!pszBuf)
		return;
	if (iEnterShift)
		pszBuf[0] = '\n';
	int nLen = WideCharToMultiByte(CP_UTF8, 0, lpBuffer, nChars, pszBuf+iEnterShift, nNeed, NULL, NULL);
	// Must be OK, but check it
	if (nLen > 0 && nLen <= nNeed)
	{
		pszBuf[iEnterShift+nNeed] = 0;
		bWrite = WriteAnsiLogUtf8(pszBuf, nLen+iEnterShift);
	}
	if (pszBuf != sBuf)
		free(pszBuf);
	UNREFERENCED_PARAMETER(bWrite);
}

void CEAnsi::WriteAnsiLogFarPrompt()
{
	_ASSERTE(ghAnsiLogFile!=NULL && "Caller must check this");
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (!GetConsoleScreenBufferInfo(hCon, &csbi))
		return;
	wchar_t* pszBuf = (wchar_t*)calloc(csbi.dwSize.X+2,sizeof(*pszBuf));
	DWORD nChars = csbi.dwSize.X;
	// We expect Far has already put "black user screen" and do CR/LF
	// if Far's keybar is hidden, we are on (csbi.dwSize.Y-1), otherwise - (csbi.dwSize.Y-2)
	_ASSERTE(csbi.dwCursorPosition.Y >= (csbi.dwSize.Y-2) && csbi.dwCursorPosition.Y <= (csbi.dwSize.Y-1));
	COORD crFrom = {0, csbi.dwCursorPosition.Y-1};
	if (pszBuf
		&& ReadConsoleOutputCharacterW(hCon, pszBuf, nChars, crFrom, &nChars)
		&& nChars)
	{
		// Do RTrim first
		while (nChars && (pszBuf[nChars-1] == L' '))
			nChars--;
		// Add CR+LF
		pszBuf[nChars++] = L'\r'; pszBuf[nChars++] = L'\n';
		// And do the logging
		WriteAnsiLogW(pszBuf, nChars);
	}
	free(pszBuf);
}

void CEAnsi::AnsiLogEnterPressed()
{
	if (!ghAnsiLogFile)
		return;
	InterlockedIncrement(&gnEnterPressed);
	gbAnsiLogNewLine = true;
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
//	SHORT ScrollStart, ScrollEnd; // 0-based absolute line indexes
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


void CEAnsi::DumpEscape(LPCWSTR buf, size_t cchLen, DumpEscapeCodes iUnknown)
{
#if defined(DUMP_UNKNOWN_ESCAPES) || defined(DUMP_WRITECONSOLE_LINES)
	if (!buf || !cchLen)
	{
		// В общем, много кто грешит попытками записи "пустых строк"
		// Например, clink, perl, ...
		//_ASSERTEX((buf && cchLen) || (gszClinkCmdLine && buf));
	}

	wchar_t szDbg[200];
	size_t nLen = cchLen;
	static int nWriteCallNo = 0;

	switch (iUnknown)
	{
	case de_Unknown/*1*/:
		msprintf(szDbg, countof(szDbg), L"[%u] ###Unknown Esc Sequence: ", GetCurrentThreadId());
		break;
	case de_BadUnicode/*2*/:
		msprintf(szDbg, countof(szDbg), L"[%u] ###Bad Unicode Translation: ", GetCurrentThreadId());
		break;
	case de_Ignored/*3*/:
		msprintf(szDbg, countof(szDbg), L"[%u] ###Ignored Esc Sequence: ", GetCurrentThreadId());
		break;
	case de_UnkControl/*4*/:
		msprintf(szDbg, countof(szDbg), L"[%u] ###Unknown Esc Control: ", GetCurrentThreadId());
		break;
	case de_Report/*5*/:
		msprintf(szDbg, countof(szDbg), L"[%u] ###Back Report: ", GetCurrentThreadId());
		break;
	case de_ScrollRegion/*6*/:
		msprintf(szDbg, countof(szDbg), L"[%u] ###Scroll region: ", GetCurrentThreadId());
		break;
	case de_Comment/*7*/:
		msprintf(szDbg, countof(szDbg), L"[%u] ###Internal comment: ", GetCurrentThreadId());
		break;
	default:
		msprintf(szDbg, countof(szDbg), L"[%u] AnsiDump #%u: ", GetCurrentThreadId(), ++nWriteCallNo);
	}

	size_t nStart = lstrlenW(szDbg);
	wchar_t* pszDst = szDbg + nStart;
	wchar_t* pszFrom = szDbg;

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
			case 7:
				*(pszDst++) = L'\\'; *(pszDst++) = L'a';
				break;
			case 8:
				*(pszDst++) = L'\\'; *(pszDst++) = L'b';
				break;
			case 0x7F:
				*(pszDst++) = '\\'; *(pszDst++) = 'x'; *(pszDst++) = '7'; *(pszDst++) = 'F';
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
				*(pszDst++) = 0xB6; // L'¶';
				*(pszDst++) = L'\n';
				*pszDst = 0;
				// Try to pass UTF-8 encoded strings to debugger
				DebugStringUtf8(szDbg);
				wmemset(szDbg, L' ', nStart);
				nCur = 0;
				pszFrom = pszDst = szDbg + nStart;
			}
		}
	}
	else
	{
		pszDst -= 2;
		const wchar_t* psEmptyMessage = L" - <empty sequence>";
		size_t nMsgLen = lstrlenW(psEmptyMessage);
		wmemcpy(pszDst, psEmptyMessage, nMsgLen);
		pszDst += nMsgLen;
	}

	if (pszDst > pszFrom)
	{
		*(pszDst++) = 0xB6; // L'¶';
		*(pszDst++) = L'\n';
		*pszDst = 0;
		// Try to pass UTF-8 encoded strings to debugger
		DebugStringUtf8(szDbg);
	}

	if (iUnknown == 1)
	{
		_ASSERTEX(FALSE && "Unknown Esc Sequence!");
	}
#endif
}

#ifdef DUMP_UNKNOWN_ESCAPES
#define DumpUnknownEscape(buf, cchLen) DumpEscape(buf, cchLen, de_Unknown)
#define DumpKnownEscape(buf, cchLen, eType) DumpEscape(buf, cchLen, eType)
#else
#define DumpUnknownEscape(buf,cchLen)
#define DumpKnownEscape(buf, cchLen, eType)
#endif

static LONG nLastReadId = 0;

// When user type smth in the prompt, screen buffer may be scrolled
// It would be nice to do the same in "ConEmuC -StoreCWD"
void CEAnsi::OnReadConsoleBefore(HANDLE hConOut, const CONSOLE_SCREEN_BUFFER_INFO& csbi)
{
	CEAnsi* pObj = CEAnsi::Object();
	if (!pObj)
		return;

	if (!nLastReadId)
		nLastReadId = GetCurrentProcessId();

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

	// Successful mark?
	_ASSERTEX(((pObj->m_RowMarks.RowId[0] || pObj->m_RowMarks.RowId[1]) && (pObj->m_RowMarks.RowId[0] != pObj->m_RowMarks.RowId[1])) || (!csbi.dwCursorPosition.X && !csbi.dwCursorPosition.Y));

	// Store info in MAPPING
	CESERVER_CONSOLE_APP_MAPPING* pAppMap = gpAppMap ? gpAppMap->Ptr() : NULL;
	if (pAppMap)
	{
		pAppMap->csbiPreRead = csbi;
		pAppMap->nPreReadRowID[0] = pObj->m_RowMarks.RowId[0];
		pAppMap->nPreReadRowID[1] = pObj->m_RowMarks.RowId[1];
	}
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


BOOL CEAnsi::WriteText(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, BOOL abCommit /*= FALSE*/, EXTREADWRITEFLAGS AddFlags /*= ewtf_None*/)
{
	BOOL lbRc = FALSE;
	DWORD /*nWritten = 0,*/ nTotalWritten = 0;

	#ifdef _DEBUG
	ORIGINAL_KRNL(WriteConsoleW);
	OnWriteConsoleW_t pfnDbgWriteConsoleW = F(WriteConsoleW);
	_ASSERTE(((_WriteConsoleW == pfnDbgWriteConsoleW) || !HooksWereSet) && "It must point to CallPointer for 'unhooked' call");
	#endif

	ExtWriteTextParm write = {sizeof(write), ewtf_Current|AddFlags, hConsoleOutput};
	write.Private = (void*)(FARPROC)_WriteConsoleW;

	GetFeatures(NULL, &mb_SuppressBells);
	if (mb_SuppressBells)
	{
		write.Flags |= ewtf_NoBells;
	}

	if (gDisplayOpt.WrapWasSet && (gDisplayOpt.WrapAt > 0))
	{
		write.Flags |= ewtf_WrapAt;
		write.WrapAtCol = gDisplayOpt.WrapAt;
	}

	if (gDisplayOpt.ScrollRegion)
	{
		write.Flags |= ewtf_Region;
		_ASSERTEX(gDisplayOpt.ScrollStart>=0 && gDisplayOpt.ScrollEnd>=gDisplayOpt.ScrollStart);
		write.Region.top = gDisplayOpt.ScrollStart;
		write.Region.bottom = gDisplayOpt.ScrollEnd;
		write.Region.left = write.Region.right = 0; // not used yet
	}

	if (gbWasXTermOutput)
	{
		// top - writes all lines using full console width
		// and thereafter some ANSI-s and "\r\n"
		int iDontWrap = 0;
		for (DWORD n = 0; (n < nNumberOfCharsToWrite) && (iDontWrap < 3); n++)
		{
			iDontWrap |= (lpBuffer[n] == L'\r' || lpBuffer[n] == L'\n') ? 1 : 2;
		}
		// If only printable chars are written
		if (iDontWrap == 2)
		{
			// ExtWriteText will check (AI) if it must not wrap&scroll
			write.Flags |= ewtf_DontWrap;
		}
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

// NON-static, because we need to ‘cache’ parts of non-translated MBCS chars (one UTF-8 symbol may be transmitted by up to *three* parts)
BOOL CEAnsi::OurWriteConsoleA(HANDLE hConsoleOutput, const char *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten)
{
	_ASSERTE(this!=NULL);
	BOOL lbRc = FALSE;
	wchar_t* buf = NULL;
	wchar_t szTemp[280]; // would be enough in most cases
	INT_PTR bufMax;
	DWORD cp;
	CpCvtResult cvt;
	const char *pSrc, *pTokenStart;
	wchar_t *pDst, *pDstEnd;
	DWORD nWritten = 0, nTotalWritten = 0;

	ORIGINAL_KRNL(WriteConsoleA);

	// Nothing to write? Or flush buffer?
	if (!lpBuffer || !nNumberOfCharsToWrite || !hConsoleOutput || (hConsoleOutput == INVALID_HANDLE_VALUE))
	{
		if (lpNumberOfCharsWritten)
			*lpNumberOfCharsWritten = 0;
		lbRc = TRUE;
		goto fin;
	}

	if (!mp_Cvt
		&& !(mp_Cvt = (CpCvt*)calloc(sizeof(*mp_Cvt),1)))
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto fin;
	}

	if ((nNumberOfCharsToWrite + 3) >= countof(szTemp))
	{
		bufMax = nNumberOfCharsToWrite + 3;
		buf = (wchar_t*)malloc(bufMax*sizeof(wchar_t));
	}
	else
	{
		buf = szTemp;
		bufMax = countof(szTemp);
	}
	if (!buf)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto fin;
	}

	cp = GetCodePage();
	mp_Cvt->SetCP(cp);

	lbRc = TRUE;
	pSrc = pTokenStart = lpBuffer;
	pDst = buf; pDstEnd = buf + bufMax - 3;
	for (DWORD n = 0; n < nNumberOfCharsToWrite; n++, pSrc++)
	{
		if (pDst >= pDstEnd)
		{
			_ASSERTE((pDst < (buf+bufMax)) && "wchar_t buffer overflow while converting");
			buf[(pDst - buf)] = 0; // It's not required, just to easify debugging
			lbRc = OurWriteConsoleW(hConsoleOutput, buf, (pDst - buf), &nWritten, NULL);
			if (lbRc) nTotalWritten += nWritten;
			pDst = buf;
		}
		cvt = mp_Cvt->Convert(*pSrc, *pDst);
		switch (cvt)
		{
		case ccr_OK:
		case ccr_BadUnicode:
			pDst++;
			break;
		case ccr_Surrogate:
		case ccr_BadTail:
		case ccr_DoubleBad:
			mp_Cvt->GetTail(*(++pDst));
			pDst++;
			break;
		}
	}

	if (pDst > buf)
	{
		_ASSERTE((pDst < (buf+bufMax)) && "wchar_t buffer overflow while converting");
		buf[(pDst - buf)] = 0; // It's not required, just to easify debugging
		lbRc = OurWriteConsoleW(hConsoleOutput, buf, (pDst - buf), &nWritten, NULL);
		if (lbRc) nTotalWritten += nWritten;
	}

	// Issue 1291:	Python fails to print string sequence with ASCII character followed by Chinese character.
	if (lpNumberOfCharsWritten && lbRc)
	{
		*lpNumberOfCharsWritten = nNumberOfCharsToWrite;
	}

fin:
	if (buf != szTemp)
		SafeFree(buf);
	return lbRc;
}

BOOL CEAnsi::OurWriteConsoleW(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved, bool bInternal /*= false*/)
{
	ORIGINAL_KRNL(WriteConsoleW);
	BOOL lbRc = FALSE;
	//ExtWriteTextParm wrt = {sizeof(wrt), ewtf_None, hConsoleOutput};
	bool bIsConOut = false;
	bool bIsAnsi = false;

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

	// In debug builds: Write to debug console all console Output
	DumpKnownEscape((wchar_t*)lpBuffer, nNumberOfCharsToWrite, de_Normal);

	CEAnsi* pObj = NULL;
	CEStr CpCvt;

	if (lpBuffer && nNumberOfCharsToWrite && hConsoleOutput)
	{
		bIsAnsi = HandleKeeper::IsAnsiCapable(hConsoleOutput, &bIsConOut);

		if (ghAnsiLogFile && bIsConOut)
		{
			CEAnsi::WriteAnsiLogW((LPCWSTR)lpBuffer, nNumberOfCharsToWrite);
		}
	}

	if (lpBuffer && nNumberOfCharsToWrite && hConsoleOutput && bIsAnsi)
	{
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

	if (!bIsAnsi || ((pObj = CEAnsi::Object()) == NULL))
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
				goto wrap;
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

				// Special one char codes? Like "ESC 7" and so on...
				if ((lpBuffer + 1) < lpEnd)
				{
					// But it may be some "special" codes
					switch (lpBuffer[1])
					{
					case L'7': // Save xterm cursor
					case L'8': // Restore xterm cursor
					case L'c': // Full reset
					case L'=':
					case L'>':
					case L'M': // Reverse LF
					case L'E': // CR-LF
					case L'D': // LF
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

				// If tail is larger than 2 chars, continue
				if ((lpBuffer + 2) < lpEnd)
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
					if (!wcschr(L"[](|", Code.Second))
					{
						// Don't assert on rawdump of KeyEvents.exe Esc key presses
						// 10:00:00 KEY_EVENT_RECORD: Dn, 1, Vk="VK_ESCAPE" [27/0x001B], Scan=0x0001 uChar=[U='\x1b' (0x001B): A='\x1b' (0x1B)]
						bool bStandaloneEscChar = (lpStart < lpSaveStart) && ((*(lpSaveStart-1) == L'\'' && Code.Second == L'\'') || (*(lpSaveStart-1) == L' ' && Code.Second == L' '));
						//_ASSERTEX(bStandaloneEscChar && "Unsupported control sequence?");
						if (!bStandaloneEscChar)
						{
							DumpKnownEscape(Code.pszEscStart, min(Code.nTotalLen,32), de_UnkControl);
						}
						continue; // invalid code
					}

					// Теперь идут параметры.
					++lpBuffer; // переместим указатель на первый символ ЗА CSI (после '[')

					switch (Code.Second)
					{
					case L'|':
						// vim-xterm-emulation
					case L'(':
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
						// Finalizing (ST) with "\x1B\\" or "\x07"
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
						// and others... look at CEAnsi::WriteAnsiCode_OSC

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
									if ((lpBuffer + 1) >= lpEnd)
									{
										// Sequence is not complete yet!
										break;
									}
									else if (lpBuffer[1] == L'\\')
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
						// Sequence is not complete, we have to store it to concatenate
						// and check on future write call. Below.
						break;

					default:
						// Unknown sequence, use common termination rules
						Code.Skip = Code.Second;
						Code.ArgSZ = lpBuffer;
						Code.cchArgSZ = 0;
						while (lpBuffer < lpEnd)
						{
							// Terminator ASCII symbol: from `@` to `~`
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

// From the cursor position!
BOOL CEAnsi::ScrollLine(HANDLE hConsoleOutput, int nDir)
{
	ExtScrollScreenParm scrl = {sizeof(scrl), essf_Current|essf_Commit, hConsoleOutput, nDir, {}, L' '};
	BOOL lbRc = ExtScrollLine(&scrl);
	return lbRc;
}

BOOL CEAnsi::ScrollScreen(HANDLE hConsoleOutput, int nDir)
{
	ExtScrollScreenParm scrl = {sizeof(scrl), essf_Current|essf_Commit, hConsoleOutput, nDir, {}, L' '};
	if (gDisplayOpt.ScrollRegion)
	{
		_ASSERTEX(gDisplayOpt.ScrollStart>=0 && gDisplayOpt.ScrollEnd>=gDisplayOpt.ScrollStart);
		scrl.Region.top = gDisplayOpt.ScrollStart;
		scrl.Region.bottom = gDisplayOpt.ScrollEnd;
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

BOOL CEAnsi::FullReset(HANDLE hConsoleOutput)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (!GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
		return FALSE;

	ReSetDisplayParm(hConsoleOutput, TRUE, TRUE);

	// Easy way to drop all lines
	ScrollScreen(hConsoleOutput, -csbi.dwSize.Y);

	// Reset cursor
	COORD cr0 = {};
	SetConsoleCursorPosition(hConsoleOutput, cr0);

	//TODO? Saved cursor position?

	return TRUE;
}

BOOL CEAnsi::ForwardLF(HANDLE hConsoleOutput, BOOL& bApply)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (!GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
		return FALSE;

	if (bApply)
	{
		ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
		bApply = FALSE;
	}

	if (csbi.dwCursorPosition.Y == (csbi.dwSize.Y - 1))
	{
		WriteText(pfnWriteConsoleW, hConsoleOutput, L"\n", 1, NULL);
		SetConsoleCursorPosition(hConsoleOutput, csbi.dwCursorPosition);
	}
	else if (csbi.dwCursorPosition.Y < (csbi.dwSize.Y - 1))
	{
		COORD cr = {csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y + 1};
		SetConsoleCursorPosition(hConsoleOutput, cr);
		if (cr.Y > csbi.srWindow.Bottom)
		{
			SMALL_RECT rcNew = csbi.srWindow;
			rcNew.Bottom = cr.Y;
			rcNew.Top = cr.Y - (csbi.srWindow.Bottom - csbi.srWindow.Top);
			_ASSERTE(rcNew.Top >= 0);
			SetConsoleWindowInfo(hConsoleOutput, TRUE, &rcNew);
		}
	}
	else
	{
		_ASSERTE(csbi.dwCursorPosition.Y > 0);
	}

	return TRUE;
}

BOOL CEAnsi::ReverseLF(HANDLE hConsoleOutput, BOOL& bApply)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (!GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
		return FALSE;

	if (bApply)
	{
		ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
		bApply = FALSE;
	}

	if (csbi.dwCursorPosition.Y == csbi.srWindow.Top)
	{
		LinesInsert(hConsoleOutput, 1);
	}
	else if (csbi.dwCursorPosition.Y > 0)
	{
		COORD cr = {csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y - 1};
		SetConsoleCursorPosition(hConsoleOutput, cr);
	}
	else
	{
		_ASSERTE(csbi.dwCursorPosition.Y > 0);
	}

	return TRUE;
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
		_ASSERTEX(gDisplayOpt.ScrollStart>=0 && gDisplayOpt.ScrollEnd>=gDisplayOpt.ScrollStart);
		TopLine = max(gDisplayOpt.ScrollStart,0);
		BottomLine = max(gDisplayOpt.ScrollEnd,0);
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
		_ASSERTEX(gDisplayOpt.ScrollStart>=0 && gDisplayOpt.ScrollEnd>gDisplayOpt.ScrollStart);
		// ScrollStart & ScrollEnd are 0-based absolute line indexes
		// relative to VISIBLE area, these are not absolute buffer coords
		TopLine = gDisplayOpt.ScrollStart;
		BottomLine = gDisplayOpt.ScrollEnd;
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

		// Some predefined as `time`, `date`, `cd`, ...
		if (!nMax)
		{
			if ((lstrcmpi(pszVarName, L"date") == 0)
				|| (lstrcmpi(pszVarName, L"time") == 0))
			{
				SYSTEMTIME st = {}; GetLocalTime(&st);
				if (lstrcmpi(pszVarName, L"date") == 0)
					_wsprintf(szValue, SKIPCOUNT(szValue) L"%u-%02u-%02u", st.wYear, st.wMonth, st.wDay);
				else
					_wsprintf(szValue, SKIPCOUNT(szValue) L"%u:%02u:%02u.%03u", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
				nMax = lstrlen(szValue);
			}
			#if 0
			else if (lstrcmpi(pszVarName, L"cd") == 0)
			{
				//TODO: If possible
			}
			#endif
		}

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

	DumpKnownEscape(asRet, nLen, de_Report);

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
				lbRc = WriteText(_WriteConsoleW, hConsoleOutput, lpBuffer, nWrite, lpNumberOfCharsWritten, FALSE);
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

					case L'7':
					case L'8':
						//TODO: 7 - Save Cursor and _Attributes_
						//TODO: 8 - Restore Cursor and _Attributes_
						XTermSaveRestoreCursor((Code.Second == L'7'), hConsoleOutput);
						break;
					case L'c':
						// Full reset
						FullReset(hConsoleOutput);
						lbApply = FALSE;
						break;
					case L'M':
						ReverseLF(hConsoleOutput, lbApply);
						break;
					case L'E':
						WriteText(_WriteConsoleW, hConsoleOutput, L"\r\n", 2, NULL);
						break;
					case L'D':
						ForwardLF(hConsoleOutput, lbApply);
						break;
					case L'=':
					case L'>':
						// xterm "ESC ="
					case L'(':
						// xterm G0..G3?
						DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored);
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
		XTermSaveRestoreCursor(true, hConsoleOutput);
		break;

	case L'u':
		// Restore cursor position
		XTermSaveRestoreCursor(false, hConsoleOutput);
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
	case L'd': // Moves the cursor to line n.
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
			case L'd':
				// Moves the cursor to line n.
				crNewPos.Y = (Code.ArgC > 0) ? (Code.ArgV[0] - 1) : 0;
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
			ORIGINAL_KRNL(SetConsoleCursorPosition);
			F(SetConsoleCursorPosition)(hConsoleOutput, crNewPos);

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
				// ViM: need to fill whole screen with selected background color, so Apply attributes
				ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
				lbApply = FALSE;
			}

			int nCmd = (Code.ArgC > 0) ? Code.ArgV[0] : 0;
			bool resetCursor = false;
			COORD cr0 = {};
			int nChars = 0;
			int nScroll = 0;

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
				resetCursor = true;
				break;
			case 3:
				// xterm: clear scrollback buffer entirely
				if (csbi.srWindow.Top > 0)
				{
					nScroll = -csbi.srWindow.Top;
					cr0.X = csbi.dwCursorPosition.X;
					cr0.Y = max(0,(csbi.dwCursorPosition.Y-csbi.srWindow.Top));
					resetCursor = true;
				}
				break;
			default:
				DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
			}

			if (nChars > 0)
			{
				ExtFillOutputParm fill = {sizeof(fill), efof_Current|efof_Attribute|efof_Character,
					hConsoleOutput, {}, L' ', cr0, (DWORD)nChars};
				ExtFillOutput(&fill);
			}

			if (resetCursor)
			{
				SetConsoleCursorPosition(hConsoleOutput, cr0);
			}

			if (nScroll)
			{
				ScrollScreen(hConsoleOutput, nScroll);
			}

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
				nChars = csbi.dwSize.X - csbi.dwCursorPosition.X;
				break;
			case 1: // clear from cursor to beginning of the line
				cr0.X = 0;
				nChars = csbi.dwCursorPosition.X + 1;
				break;
			case 2: // clear entire line
				cr0.X = 0;
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
			// Values are 1-based
			SetScrollRegion(true, true, Code.ArgV[0], Code.ArgV[1], hConsoleOutput);
		}
		else
		{
			SetScrollRegion(false);
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
		ScrollLine(hConsoleOutput, ((Code.ArgC > 0 && Code.ArgV[0] > 0) ? Code.ArgV[0] : 1));
		break;
	case L'P':
		// Delete P s Character(s) (default = 1) (DCH).
		ScrollLine(hConsoleOutput, -((Code.ArgC > 0 && Code.ArgV[0] > 0) ? Code.ArgV[0] : 1));
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
				if ((Code.PvtLen == 1) && (Code.Pvt[0] == L'?'))
				{
					gDisplayCursor.CursorKeysApp = (Code.Action == L'h');

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

					ChangeTermMode(tmc_AppCursorKeys, (Code.Action == L'h'));
				}
				else
					DumpUnknownEscape(Code.pszEscStart, Code.nTotalLen);
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
				DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored);
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
			case 4:
				if (Code.PvtLen == 0)
				{
					/* h=Insert Mode (IRM), l=Replace Mode (IRM) */
					// Nano posts the `ESC [ 4 l` on start, but do not post `ESC [ 4 h` on exit, that is strange...
					DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored); // ignored for now
				}
				else if ((Code.PvtLen == 1) && (Code.Pvt[0] == L'?'))
				{
					/* h=Smooth (slow) scroll, l=Jump (fast) scroll */
					DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored); // ignored for now
				}
				else
					DumpUnknownEscape(Code.pszEscStart, Code.nTotalLen);
				break;
			case 12:   /* SRM: set echo mode */
			case 1000: /* VT200_MOUSE */
			case 1002: /* BTN_EVENT_MOUSE */
			case 1003: /* ANY_EVENT_MOUSE */
			case 1004: /* FOCUS_EVENT_MOUSE */
			case 1005: /* Xterm's UTF8 encoding for mouse positions */
			case 1006: /* Xterm's CSI-style mouse encoding */
				// xmux/screen?
				if ((Code.PvtLen == 1) && (Code.Pvt[0] == L'?'))
					DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored); // ignored for now
				else
					DumpUnknownEscape(Code.pszEscStart, Code.nTotalLen);
				break;
			case 47:   /* alternate screen */
			case 1047: /* alternate screen */
			case 1049: /* cursor & alternate screen */
				// xmux/screen: Alternate screen
				if ((Code.PvtLen == 1) && (Code.Pvt[0] == L'?'))
				{
					if (lbApply)
					{
						ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
						lbApply = FALSE;
					}

					// \e[?1049h: save cursor pos
					if ((Code.ArgV[0] == 1049) && (Code.Action == L'h'))
						XTermSaveRestoreCursor(true, hConsoleOutput);
					// h: switch to alternative buffer without backscroll
					// l: restore saved scrollback buffer
					XTermAltBuffer((Code.Action == L'h'));
					// \e[?1049l - restore cursor pos
					if ((Code.ArgV[0] == 1049) && (Code.Action == L'l'))
						XTermSaveRestoreCursor(false, hConsoleOutput);
				}
				else
				{
					DumpUnknownEscape(Code.pszEscStart, Code.nTotalLen);
				}
				break;
			case 1048: /* save/restore cursor */
				if ((Code.PvtLen == 1) && (Code.Pvt[0] == L'?'))
					XTermSaveRestoreCursor((Code.Action == L'h'), hConsoleOutput);
				else
					DumpUnknownEscape(Code.pszEscStart, Code.nTotalLen);
				break;
			case 2004: /* bracketed paste */
				/* All "pasted" text will be wrapped in `\e[200~ ... \e[201~` */
				if ((Code.PvtLen == 1) && (Code.Pvt[0] == L'?'))
					ChangeTermMode(tmc_BracketedPaste, (Code.Action == L'h'));
				else
					DumpUnknownEscape(Code.pszEscStart, Code.nTotalLen);
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
		if (Code.PvtLen > 0)
		{
			DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored);
		}
		// Set display mode (colors, fonts, etc.)
		else if (!Code.ArgC)
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
					gDisplayParm.Inverse = TRUE;
					gDisplayParm.WasSet = TRUE;
					break;
				case 27:
					// Positive (not inverse)
					gDisplayParm.Inverse = FALSE;
					gDisplayParm.WasSet = TRUE;
					break;
				case 25:
					// Steady (not blinking)
					DumpKnownEscape(Code.pszEscStart,Code.nTotalLen,de_Ignored);
					break;
				case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37:
					gDisplayParm.TextColor = (Code.ArgV[i] - 30);
					if (gDisplayParm.BrightOrBold == 2)
						gDisplayParm.BrightOrBold = FALSE;
					else
						gDisplayParm.BrightOrBold &= ~2;
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
					if (gDisplayParm.BackOrUnderline == 2)
						gDisplayParm.BackOrUnderline = FALSE;
					else
						gDisplayParm.BackOrUnderline &= ~2;
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
					gDisplayParm.BrightOrBold |= 2;
					gDisplayParm.WasSet = TRUE;
					break;
				case 100: case 101: case 102: case 103: case 104: case 105: case 106: case 107:
					gDisplayParm.BackColor = (Code.ArgV[i] - 100);
					gDisplayParm.Back256 = FALSE;
					gDisplayParm.BackOrUnderline |= 2;
					gDisplayParm.WasSet = TRUE;
					break;
				case 10:
					// Something strange and unknown... (received from ssh)
					DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored);
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
			// msprintf(szVerInfo, countof(szVerInfo), L"\x1B>%u;%u;0c", (int)'C', MVV_1*10000+MVV_2*100+MVV_3);
			// Emulate xterm version 136?
			wcscpy_c(szVerInfo, L"\x1B[>0;136;0c");
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

	// Finalizing (ST) with "\x1B\\" or "\x07"
	//ESC ] 0 ; txt ST        Set icon name and window title to txt.
	//ESC ] 1 ; txt ST        Set icon name to txt.
	//ESC ] 2 ; txt ST        Set window title to txt.
	//ESC ] 4 ; num; txt ST   Set ANSI color num to txt.
	//ESC ] 9 ... ST          ConEmu specific
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
		// TODO: the following is suggestion for exact palette colors
		// TODO: but we are using standard xterm palette or truecolor 24bit palette
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
		// ESC ] 9 ; 10 ST               Request xterm keyboard emulation
		// ESC ] 9 ; 11; "*txt*" ST      Just a ‘comment’, skip it.
		// ESC ] 9 ; 12 ST               Let ConEmu treat current cursor position as prompt start. Useful with `PS1`.
		if (Code.ArgSZ[1] == L';')
		{
			if (Code.ArgSZ[2] == L'1')
			{
				if (Code.ArgSZ[3] == L';')
				{
					// ESC ] 9 ; 1 ; ms ST
					DoSleep(Code.ArgSZ+4);
				}
				else if (Code.ArgSZ[3] == L'0')
				{
					// ESC ] 9 ; 10 ST
					if (!gbWasXTermOutput)
						CEAnsi::StartXTermMode(true);
				}
				else if (Code.ArgSZ[3] == L'1' && Code.ArgSZ[4] == L';')
				{
					// ESC ] 9 ; 11; "*txt*" ST - Just a ‘comment’, skip it.
					DumpKnownEscape(Code.ArgSZ+5, lstrlen(Code.ArgSZ+5), de_Comment);
				}
				else if (Code.ArgSZ[3] == L'2')
				{
					// ESC ] 9 ; 12 ST
					StorePromptBegin();
				}
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

void CEAnsi::SetScrollRegion(bool bRegion, bool bRelative, int nStart, int nEnd, HANDLE hConsoleOutput)
{
	if (bRegion)
	{
		if (bRelative)
		{
			HANDLE hConOut = hConsoleOutput ? hConsoleOutput : GetStdHandle(STD_OUTPUT_HANDLE);
			CONSOLE_SCREEN_BUFFER_INFO csbi = {};
			TODO("Determine the lowest dirty line of console?");
			if (GetConsoleScreenBufferInfoCached(hConOut, &csbi, TRUE))
			{
				nStart += csbi.srWindow.Top;
				nEnd += csbi.srWindow.Top;
			}
		}
		// We need 0-based absolute values
		gDisplayOpt.ScrollStart = nStart - 1;
		gDisplayOpt.ScrollEnd = nEnd - 1;
		// Validate
		if (!(gDisplayOpt.ScrollRegion = (gDisplayOpt.ScrollStart>=0 && gDisplayOpt.ScrollEnd>=gDisplayOpt.ScrollStart)))
		{
			_ASSERTEX(gDisplayOpt.ScrollStart>=0 && gDisplayOpt.ScrollEnd>=gDisplayOpt.ScrollStart);
		}
		wchar_t szLog[40]; msprintf(szLog, countof(szLog), L"{%i,%i}", gDisplayOpt.ScrollStart, gDisplayOpt.ScrollEnd);
	}
	else
	{
		gDisplayOpt.ScrollRegion = FALSE;
		DumpKnownEscape(L"<Disabled>", 10, de_ScrollRegion);
	}
}

void CEAnsi::XTermSaveRestoreCursor(bool bSaveCursor, HANDLE hConsoleOutput /*= NULL*/)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	HANDLE hConOut = hConsoleOutput ? hConsoleOutput : GetStdHandle(STD_OUTPUT_HANDLE);

	if (bSaveCursor)
	{
		// Save cursor position (can not be nested)
		if (GetConsoleScreenBufferInfoCached(hConOut, &csbi))
		{
			gDisplayCursor.StoredCursorPos = csbi.dwCursorPosition;
			gDisplayCursor.bCursorPosStored = TRUE;
		}
	}
	else
	{
		// Restore cursor position
		SetConsoleCursorPosition(hConOut, gDisplayCursor.StoredCursorPos);
	}
}

/// Change current buffer
/// Alternative buffer in XTerm is used to "fullscreen"
/// applications like Vim. There is no scrolling and this
/// mode is used to save current backscroll contents and
/// restore it when application exits
void CEAnsi::XTermAltBuffer(bool bSetAltBuffer)
{
	if (bSetAltBuffer)
	{
		// Once!
		if ((gXTermAltBuffer.Flags & xtb_AltBuffer))
			return;

		CONSOLE_SCREEN_BUFFER_INFO csbi1 = {}, csbi2 = {};
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if (GetConsoleScreenBufferInfoCached(hOut, &csbi1, TRUE))
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
						// Ensure we have fresh information (size was changed)
						GetConsoleScreenBufferInfoCached(hOut, &csbi2, TRUE);

						// Clear current screen contents, don't move cursor position
						COORD cr0 = {};
						DWORD nChars = csbi2.dwSize.X * csbi2.dwSize.Y;
						ExtFillOutputParm fill = {sizeof(fill), efof_Current|efof_Attribute|efof_Character,
							hOut, {}, L' ', cr0, (DWORD)nChars};
						ExtFillOutput(&fill);

						TODO("BufferWidth");
						if (!(gXTermAltBuffer.Flags & xtb_AltBuffer))
						{
							// Backscroll length
							gXTermAltBuffer.BufferSize = (csbi1.dwSize.Y > (csbi1.srWindow.Bottom - csbi1.srWindow.Top + 1))
								? csbi1.dwSize.Y : 0;
							gXTermAltBuffer.Flags = xtb_AltBuffer;
							// Stored cursor pos
							if (gDisplayCursor.bCursorPosStored)
							{
								gXTermAltBuffer.CursorPos = gDisplayCursor.StoredCursorPos;
								gXTermAltBuffer.Flags |= xtb_StoredCursor;
							}
							// Stored scroll region
							if (gDisplayOpt.ScrollRegion)
							{
								gXTermAltBuffer.ScrollStart = gDisplayOpt.ScrollStart;
								gXTermAltBuffer.ScrollEnd = gDisplayOpt.ScrollEnd;
								gXTermAltBuffer.Flags |= xtb_StoredScrollRegion;
							}
						}
					}
					ExecuteFreeResult(pIn);
					ExecuteFreeResult(pOut);
				}
			}
		}
	}
	else
	{
		if (!(gXTermAltBuffer.Flags & xtb_AltBuffer))
			return;

		// Однократно
		gXTermAltBuffer.Flags &= ~xtb_AltBuffer;

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
			if (!gXTermAltBuffer.BufferSize)
				pIn->AltBuf.AbFlags |= abf_BufferOff;
			pIn->AltBuf.BufferHeight = gXTermAltBuffer.BufferSize;
			// Async - is not allowed. Otherwise current application (cmd.exe for example)
			// may start printing before server finishes buffer restoration
			pOut = ExecuteSrvCmd(gnServerPID, pIn, ghConWnd);
			ExecuteFreeResult(pIn);
			ExecuteFreeResult(pOut);
			// Restore saved states
			if ((gDisplayCursor.bCursorPosStored = !!(gXTermAltBuffer.Flags & xtb_StoredCursor)))
			{
				gDisplayCursor.StoredCursorPos = gXTermAltBuffer.CursorPos;
			}
			if ((gDisplayOpt.ScrollRegion = !!(gXTermAltBuffer.Flags & xtb_StoredScrollRegion)))
			{
				gDisplayOpt.ScrollStart = gXTermAltBuffer.ScrollStart;
				gDisplayOpt.ScrollEnd = gXTermAltBuffer.ScrollEnd;
			}
		}
	}
}

/*
ViM need some hacks in current ConEmu versions
This is because
1) 256 colors mode requires NO scroll buffer
2) can't find ATM legal way to switch Alternative/Primary buffer by request from ViM
*/

void CEAnsi::StartVimTerm(bool bFromDllStart)
{
	// Only certain versions of Vim able to use xterm-256 in ConEmu
	if (gnExeFlags & (caf_Cygwin1|caf_Msys1|caf_Msys2))
		return;

	// For native vim - don't handle "--help" and "--version" switches
	// Has no sense for cygwin/msys, but they are skipped above
	CEStr lsArg;
	LPCWSTR pszCmdLine = GetCommandLineW();
	LPCWSTR pszCompare[] = {L"--help", L"-h", L"--version", NULL};
	while (0 == NextArg(&pszCmdLine, lsArg))
	{
		for (INT_PTR i = 0; pszCompare[i]; i++)
		{
			if (wcscmp(lsArg, pszCompare[i]) == 0)
				return;
		}
	}

	XTermAltBuffer(true);
}

void CEAnsi::StopVimTerm()
{
	if (gbWasXTermOutput)
	{
		CEAnsi::StartXTermMode(false);
	}

	XTermAltBuffer(false);
}

void CEAnsi::ChangeTermMode(TermModeCommand mode, DWORD value)
{
	CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_STARTXTERM, sizeof(CESERVER_REQ_HDR)+2*sizeof(DWORD));
	if (pIn)
	{
		pIn->dwData[0] = mode;
		pIn->dwData[1] = value;
		CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
		ExecuteFreeResult(pIn);
		ExecuteFreeResult(pOut);
	}
}

void CEAnsi::StartXTermMode(bool bStart)
{
	// May be triggered by ConEmuT, official Vim builds and in some other cases
	//_ASSERTEX((gXTermAltBuffer.Flags & xtb_AltBuffer) && (gbWasXTermOutput!=bStart));
	_ASSERTEX(gbWasXTermOutput!=bStart);

	// Remember last mode
	gbWasXTermOutput = bStart;
	ChangeTermMode(tmc_Keyboard, bStart ? te_xterm : te_win32);
}

// This is useful when user press Shift+Home,
// we'll select only "typed command" part, without "prompt"
void CEAnsi::StorePromptBegin()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (GetConsoleScreenBufferInfo(hConOut, &csbi))
	{
		OnReadConsoleBefore(hConOut, csbi);
	}
}


//static
CEAnsi* CEAnsi::Object(bool bForceCreate /*= false*/)
{
	if (!AnsiTlsIndex)
	{
		AnsiTlsIndex = TlsAlloc();
	}

	if ((!AnsiTlsIndex) || (AnsiTlsIndex == TLS_OUT_OF_INDEXES))
	{
		_ASSERTEX(AnsiTlsIndex && AnsiTlsIndex!=TLS_OUT_OF_INDEXES);
		return NULL;
	}

	//if (!AnsiTls.Initialized())
	//{
	//	AnsiTls.Init(
	//}

	CEAnsi* p = NULL;

	if (!bForceCreate)
	{
		p = (CEAnsi*)TlsGetValue(AnsiTlsIndex);
	}

	if (!p)
	{
		p = (CEAnsi*)calloc(1,sizeof(*p));
		if (p) p->GetDefaultTextAttr(); // Initialize "default attributes"
		TlsSetValue(AnsiTlsIndex, p);
	}

	return p;
}

//static
void CEAnsi::Release()
{
	if (AnsiTlsIndex && (AnsiTlsIndex != TLS_OUT_OF_INDEXES))
	{
		CEAnsi* p = (CEAnsi*)TlsGetValue(AnsiTlsIndex);
		if (p)
		{
			SafeFree(p->mp_Cvt);
			free(p);
		}
		TlsSetValue(AnsiTlsIndex, NULL);
	}
}
