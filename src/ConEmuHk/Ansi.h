
/*
Copyright (c) 2013-2015 Maximus5
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

#pragma once

#ifdef _DEBUG
	#define DUMP_WRITECONSOLE_LINES
	#define DUMP_UNKNOWN_ESCAPES
#endif

extern DWORD AnsiTlsIndex;
//#include "../common/MMap.h"

#include "../common/ConsoleMixAttr.h"

typedef BOOL (WINAPI* OnWriteConsoleW_t)(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);

#define CEAnsi_MaxPrevPart 160
#define CEAnsi_MaxPrevAnsiPart 80

struct MSectionSimple;

#if defined(__GNUC__)
extern "C" {
#endif
	BOOL WINAPI WriteProcessed(LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten);
#if defined(__GNUC__)
};
#endif

extern FARPROC CallWriteConsoleW;

struct CEAnsi
{
//private:
//	static MMap<DWORD,CEAnsi*> AnsiTls;
public:
	/* ************************************* */
	/* Init and release thread local storage */
	/* ************************************* */
	static CEAnsi* Object(bool bForceCreate = false)
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
	};
	static void Release()
	{
		if (AnsiTlsIndex && (AnsiTlsIndex != TLS_OUT_OF_INDEXES))
		{
			CEAnsi* p = (CEAnsi*)TlsGetValue(AnsiTlsIndex);
			if (p)
			{
				free(p);
			}
			TlsSetValue(AnsiTlsIndex, NULL);
		}
	};

public:
	/* ************************************* */
	/*          Hook routines                */
	/* ************************************* */
	static HANDLE WINAPI OnCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile );
	static BOOL WINAPI OnWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
	static BOOL WINAPI OnWriteConsoleA(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);
	static BOOL WINAPI OnWriteConsoleW(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved);
	static BOOL WINAPI OurWriteConsoleW(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved, bool bInternal = true);
	static BOOL WINAPI OnWriteConsoleOutputCharacterA(HANDLE hConsoleOutput, LPCSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten);
	static BOOL WINAPI OnWriteConsoleOutputCharacterW(HANDLE hConsoleOutput, LPCWSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten);
	static BOOL WINAPI OnScrollConsoleScreenBufferA(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill);
	static BOOL WINAPI OnScrollConsoleScreenBufferW(HANDLE hConsoleOutput, const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, const CHAR_INFO *lpFill);
	static BOOL WINAPI OnSetConsoleMode(HANDLE hConsoleHandle, DWORD dwMode);

public:
	/* ************************************* */
	/*      STATIC Helper routines           */
	/* ************************************* */
	static void StartVimTerm(bool bFromDllStart);
	static void StopVimTerm();

	static void OnReadConsoleBefore(HANDLE hConOut, const CONSOLE_SCREEN_BUFFER_INFO& csbi);
	static void OnReadConsoleAfter(bool bFinal, bool bNoLineFeed);

	static bool IsAnsiCapable(HANDLE hFile, bool* bIsConsoleOutput = NULL);
	static bool IsOutputHandle(HANDLE hFile, DWORD* pMode = NULL);

	static void InitAnsiLog(LPCWSTR asFilePath);
	static void DoneAnsiLog(bool bFinal);

	static void GetFeatures(bool* pbAnsiAllowed, bool* pbSuppressBells);

	static SHORT GetDefaultTextAttr();

	static HANDLE ghLastAnsiCapable /*= NULL*/;
	static HANDLE ghLastAnsiNotCapable /*= NULL*/;
	static HANDLE ghLastConOut /*= NULL*/;
	static HANDLE ghAnsiLogFile /*= NULL*/;
	static MSectionSimple* gcsAnsiLogFile;

	static bool gbWasXTermOutput;

protected:
	static int NextNumber(LPCWSTR& asMS);

	static void StartXTermMode(bool bStart);

public:
	struct AnsiEscCode
	{
		wchar_t  First;  // ESC (27)
		wchar_t  Second; // any of 64 to 95 ('@' to '_')
		wchar_t  Action; // any of 64 to 126 (@ to ~). this is terminator
		wchar_t  Skip;   // Если !=0 - то эту последовательность нужно пропустить
		int      ArgC;
		int      ArgV[16];
		LPCWSTR  ArgSZ; // Reserved for key mapping
		size_t   cchArgSZ;

	#ifdef _DEBUG
		LPCWSTR  pszEscStart;
		size_t   nTotalLen;
	#endif

		int      PvtLen;
		wchar_t  Pvt[16];
	};

public:
	/* ************************************* */
	/*         Working methods               */
	/* ************************************* */
	BOOL WriteAnsiCodes(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten);
protected:
	void WriteAnsiCode_CSI(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, AnsiEscCode& Code, BOOL& lbApply);
	void WriteAnsiCode_OSC(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, AnsiEscCode& Code, BOOL& lbApply);
	void WriteAnsiCode_VIM(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, AnsiEscCode& Code, BOOL& lbApply);
	BOOL ReportString(LPCWSTR asRet);
	void ReportConsoleTitle();
	static void WriteAnsiLog(LPCWSTR lpBuffer, DWORD nChars);
public:

	void ReSetDisplayParm(HANDLE hConsoleOutput, BOOL bReset, BOOL bApply);

	#if defined(DUMP_UNKNOWN_ESCAPES) || defined(DUMP_WRITECONSOLE_LINES)
	static void DumpEscape(LPCWSTR buf, size_t cchLen, int iUnknown);
	#endif

	BOOL WriteText(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, BOOL abCommit = FALSE);
	BOOL ScrollScreen(HANDLE hConsoleOutput, int nDir);
	BOOL PadAndScroll(HANDLE hConsoleOutput, CONSOLE_SCREEN_BUFFER_INFO& csbi);
	BOOL LinesInsert(HANDLE hConsoleOutput, const int LinesCount);
	BOOL LinesDelete(HANDLE hConsoleOutput, const int LinesCount);
	void DoSleep(LPCWSTR asMS);
	void EscCopyCtrlString(wchar_t* pszDst, LPCWSTR asMsg, INT_PTR cchMaxLen);
	void DoMessage(LPCWSTR asMsg, INT_PTR cchLen);
	void DoProcess(LPCWSTR asCmd, INT_PTR cchLen);
	void DoPrintEnv(LPCWSTR asCmd, INT_PTR cchLen);
	void DoSendCWD(LPCWSTR asCmd, INT_PTR cchLen);

	int NextEscCode(LPCWSTR lpBuffer, LPCWSTR lpEnd, wchar_t (&szPreDump)[CEAnsi_MaxPrevPart], DWORD& cchPrevPart, LPCWSTR& lpStart, LPCWSTR& lpNext, AnsiEscCode& Code, BOOL ReEntrance = FALSE);

protected:
	/* ************************************* */
	/*        Instance variables             */
	/* ************************************* */
	OnWriteConsoleW_t pfnWriteConsoleW;
	HANDLE mh_WriteOutput;

	struct DisplayParm
	{
		BOOL WasSet;
		BOOL BrightOrBold;     // 1
		BOOL ItalicOrInverse;  // 3
		BOOL BackOrUnderline;  // 4
		int  TextColor;        // 30-37,38,39
		BOOL Text256;          // 38
	    int  BackColor;        // 40-47,48,49
	    BOOL Back256;          // 48
		// xterm
		BOOL Inverse;
	}; // gDisplayParm = {};
	// Bad thing... Thought, it must be synced between thread, but when?
	static DisplayParm gDisplayParm;

	struct DisplayCursorPos
	{
	    // Internal
	    COORD StoredCursorPos;
		// Esc[?1h 	Set cursor key to application 	DECCKM
		// Esc[?1l 	Set cursor key to cursor 	DECCKM
		BOOL CursorKeysApp; // "1h"
	}; // gDisplayCursor = {};
	// Bad thing again...
	static DisplayCursorPos gDisplayCursor;

	struct DisplayOpt
	{
		BOOL  WrapWasSet;
		SHORT WrapAt; // Rightmost X coord (1-based)
		//
		BOOL  AutoLfNl; // LF/NL (default off): Automatically follow echo of LF, VT or FF with CR.
		//
		BOOL  ScrollRegion;
		SHORT ScrollStart, ScrollEnd; // 1-based line indexes (relative to VISIBLE area, these are not absolute buffer coords)
		//
		BOOL  ShowRawAnsi; // \e[3h display ANSI control characters (TRUE), \e[3l process ANSI (FALSE, normal mode)
	}; // gDisplayOpt;
	// Bad thing again...
	static DisplayOpt gDisplayOpt;

	wchar_t gsPrevAnsiPart[CEAnsi_MaxPrevPart]; // = {};
	INT_PTR gnPrevAnsiPart; // = 0;
	wchar_t gsPrevAnsiPart2[CEAnsi_MaxPrevPart]; // = {};
	INT_PTR gnPrevAnsiPart2; // = 0;

	bool mb_SuppressBells;

	// In "ReadLine" we can't control scrolling
	// thats why we need to mark some rows for identification
	struct XtermStoreMarks
	{
		// [0] - above CursorPos, but if CursorPos was on FIRST line - it can't be...
		// [1] - exactly on CursorPos, but if CursorX on the FIRST column - this will be illegal
		SHORT SaveRow[2];
		WORD  RowId[2];
		CONSOLE_SCREEN_BUFFER_INFO csbi;
	} m_RowMarks;
};
