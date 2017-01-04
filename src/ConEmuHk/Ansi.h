
/*
Copyright (c) 2013-2017 Maximus5
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

#include "ExtConsole.h"
#include "hkConsoleOutput.h"

#define CEAnsi_MaxPrevPart 512
#define CEAnsi_MaxPrevAnsiPart 256

struct MSectionSimple;

enum WriteProcessedStream
{
	wps_None   = 0,
	wps_Output = 1,
	wps_Error  = 2,
	wps_Input  = 4, // Reserved for StdInput
	wps_Ansi   = 8, // Reserved as a Flag for IsAnsiCapable
};

#if defined(__GNUC__)
extern "C" {
#endif
	BOOL WINAPI WriteProcessed(LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten);
	BOOL WINAPI WriteProcessed2(LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, WriteProcessedStream Stream);
	BOOL WINAPI WriteProcessed3(LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, HANDLE hConsoleOutput);
	BOOL WINAPI WriteProcessedA(LPCSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, WriteProcessedStream Stream);
#if defined(__GNUC__)
};
#endif

extern FARPROC CallWriteConsoleW;

// In debug build if SHOW_FIRST_ANSI_CALL is defined,
// MsgBox may be shown on first ANSI sequence printout
#ifdef _DEBUG
	extern void FIRST_ANSI_CALL(const BYTE* lpBuf, DWORD nNumberOfBytes); // Entry.cpp
#else
	#define FIRST_ANSI_CALL(lpBuf,nNumberOfBytes)
#endif

enum DumpEscapeCodes
{
	de_Normal = 0,
	de_Unknown = 1,
	de_BadUnicode = 2,
	de_Ignored = 3,
	de_UnkControl = 4,
	de_Report = 5,
	de_ScrollRegion = 6,
	de_Comment = 7,
};

struct CpCvt;

struct CEAnsi
{
//private:
//	static MMap<DWORD,CEAnsi*> AnsiTls;
public:
	/* ************************************* */
	/* Init and release thread local storage */
	/* ************************************* */
	static CEAnsi* Object(bool bForceCreate = false);
	static void Release();

public:
	/* ************************************* */
	/*      STATIC Helper routines           */
	/* ************************************* */
	static void StartVimTerm(bool bFromDllStart);
	static void StopVimTerm();

	static BOOL OurWriteConsoleW(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved, bool bInternal = true);

	static void OnReadConsoleBefore(HANDLE hConOut, const CONSOLE_SCREEN_BUFFER_INFO& csbi);
	static void OnReadConsoleAfter(bool bFinal, bool bNoLineFeed);

	static void InitAnsiLog(LPCWSTR asFilePath);
	static void DoneAnsiLog(bool bFinal);

	static void GetFeatures(bool* pbAnsiAllowed, bool* pbSuppressBells);

	static SHORT GetDefaultTextAttr();

	static HANDLE ghAnsiLogFile /*= NULL*/;
	static LONG   gnEnterPressed /*= 0*/;
	static bool   gbAnsiLogNewLine /*= false*/;
	static bool   gbAnsiWasNewLine /*= false*/;
	static MSectionSimple* gcsAnsiLogFile;

	static bool gbWasXTermOutput;

protected:
	static int NextNumber(LPCWSTR& asMS);

public:
	static void ChangeTermMode(TermModeCommand mode, DWORD value, DWORD nPID = 0);
	static void StartXTermMode(bool bStart);
	static void StorePromptBegin();

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
	// NON-static, because we need to ‘cache’ parts of non-translated MBCS chars (one UTF-8 symbol may be transmitted by up to *three* parts)
	BOOL OurWriteConsoleA(HANDLE hConsoleOutput, const char *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten);
	// Unicode method
	BOOL WriteAnsiCodes(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten);
protected:
	CpCvt* mp_Cvt;
protected:
	void WriteAnsiCode_CSI(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, AnsiEscCode& Code, BOOL& lbApply);
	void WriteAnsiCode_OSC(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, AnsiEscCode& Code, BOOL& lbApply);
	void WriteAnsiCode_VIM(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, AnsiEscCode& Code, BOOL& lbApply);
	BOOL ReportString(LPCWSTR asRet);
	void ReportConsoleTitle();
	void ReportTerminalPixelSize();
	static BOOL WriteAnsiLogUtf8(const char* lpBuffer, DWORD nChars);
public:
	static UINT GetCodePage();
	static void WriteAnsiLogA(LPCSTR lpBuffer, DWORD nChars);
	static void WriteAnsiLogW(LPCWSTR lpBuffer, DWORD nChars);
	static void WriteAnsiLogFarPrompt();
	static void AnsiLogEnterPressed();
protected:
	static void XTermSaveRestoreCursor(bool bSaveCursor, HANDLE hConsoleOutput = NULL);
	static void XTermAltBuffer(bool bSetAltBuffer);
public:

	void ReSetDisplayParm(HANDLE hConsoleOutput, BOOL bReset, BOOL bApply);

	static void DumpEscape(LPCWSTR buf, size_t cchLen, DumpEscapeCodes iUnknown);

	BOOL WriteText(OnWriteConsoleW_t _WriteConsoleW, HANDLE hConsoleOutput, LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, BOOL abCommit = FALSE, EXTREADWRITEFLAGS AddFlags = ewtf_None);
	BOOL ScrollLine(HANDLE hConsoleOutput, int nDir);
	BOOL ScrollScreen(HANDLE hConsoleOutput, int nDir);
	BOOL PadAndScroll(HANDLE hConsoleOutput, CONSOLE_SCREEN_BUFFER_INFO& csbi);
	BOOL FullReset(HANDLE hConsoleOutput);
	BOOL ForwardLF(HANDLE hConsoleOutput, BOOL& bApply);
	BOOL ReverseLF(HANDLE hConsoleOutput, BOOL& bApply);
	BOOL LinesInsert(HANDLE hConsoleOutput, const int LinesCount);
	BOOL LinesDelete(HANDLE hConsoleOutput, const int LinesCount);
	void DoSleep(LPCWSTR asMS);
	void EscCopyCtrlString(wchar_t* pszDst, LPCWSTR asMsg, INT_PTR cchMaxLen);
	void DoMessage(LPCWSTR asMsg, INT_PTR cchLen);
	void DoProcess(LPCWSTR asCmd, INT_PTR cchLen);
	void DoGuiMacro(LPCWSTR asCmd, INT_PTR cchLen);
	void DoPrintEnv(LPCWSTR asCmd, INT_PTR cchLen);
	void DoSendCWD(LPCWSTR asCmd, INT_PTR cchLen);
	bool IsAnsiExecAllowed(LPCWSTR asCmd);

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
		UINT BrightOrBold;     // 1
		BOOL ItalicOrInverse;  // 3
		UINT BackOrUnderline;  // 4
		int  TextColor;        // 30-37,38,39
		BOOL Text256;          // 38
		int  BackColor;        // 40-47,48,49
		BOOL Back256;          // 48
		// xterm
		BOOL Inverse;
	}; // gDisplayParm = {};
	// Bad thing... Thought, it must be synced between thread, but when?
	static DisplayParm gDisplayParm;

	friend BOOL WINAPI OnWriteConsoleOutputCharacterA(HANDLE hConsoleOutput, LPCSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten);
	friend BOOL WINAPI OnWriteConsoleOutputCharacterW(HANDLE hConsoleOutput, LPCWSTR lpCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten);

	struct DisplayCursorPos
	{
		// Internal
		BOOL  bCursorPosStored;
		COORD StoredCursorPos;
		// Esc[?1h 	Set cursor key to application 	DECCKM
		// Esc[?1l 	Set cursor key to cursor 	DECCKM
		BOOL CursorKeysApp; // "?1h"
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
		SHORT ScrollStart, ScrollEnd; // 0-based absolute line indexes
		//
		BOOL  ShowRawAnsi; // \e[3h display ANSI control characters (TRUE), \e[3l process ANSI (FALSE, normal mode)
	}; // gDisplayOpt;
	// Bad thing again...
	static DisplayOpt gDisplayOpt;
	// Store absolute coords by relative ANSI values
	void SetScrollRegion(bool bRegion, bool bRelative = true, int nStart = 0, int nEnd = 0, HANDLE hConsoleOutput = NULL);

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
