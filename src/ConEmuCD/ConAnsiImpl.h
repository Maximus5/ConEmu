
/*
Copyright (c) 2013-present Maximus5
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

#include <mutex>
#include "../common/ConsoleMixAttr.h"
#include "../common/WCodePage.h"
#include "ConData.h"
#include "ConAnsi.h"

// #condata Remove this include
#include "../ConEmuHk/ExtConsole.h"


class SrvAnsiImpl
{
protected:
	std::unique_lock<std::mutex> m_UseLock;
	SrvAnsi* m_Owner = nullptr;
	condata::Table* m_Table = nullptr;

public:
	SrvAnsiImpl(SrvAnsi* _owner, condata::Table* _table);
	~SrvAnsiImpl();

	SrvAnsiImpl() = delete;
	SrvAnsiImpl(const SrvAnsiImpl&) = delete;
	SrvAnsiImpl(SrvAnsiImpl&&) = delete;

	/// Main routine - writer entry point
	bool OurWriteConsole(const wchar_t* lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten);

protected:
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

protected:
	int NextNumber(LPCWSTR& asMS);

	bool WriteAnsiCodes(LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten);

	void WriteAnsiCode_CSI(AnsiEscCode& Code);
	void WriteAnsiCode_OSC(AnsiEscCode& Code);
	void WriteAnsiCode_VIM(AnsiEscCode& Code);
	bool ReportString(LPCWSTR asRet);
	void ReportConsoleTitle();
	void ReportTerminalPixelSize();
	void ReportTerminalCharSize(int code);
	void ReportCursorPosition();

	void XTermSaveRestoreCursor(bool bSaveCursor);
	void XTermAltBuffer(bool bSetAltBuffer);

	bool WriteText(LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten);
	bool FullReset();
	void ForwardLF();
	void ForwardCRLF();
	void ReverseLF();
	void DoSleep(LPCWSTR asMS);
	void EscCopyCtrlString(wchar_t* pszDst, LPCWSTR asMsg, ssize_t cchMaxLen);
	void DoMessage(LPCWSTR asMsg, ssize_t cchLen);
	void DoProcess(LPCWSTR asCmd, ssize_t cchLen);
	void DoGuiMacro(LPCWSTR asCmd, ssize_t cchLen);
	void DoPrintEnv(LPCWSTR asCmd, ssize_t cchLen);
	void DoSendCWD(LPCWSTR asCmd, ssize_t cchLen);
	void DoSetProgress(WORD st, WORD pr, LPCWSTR pszName = NULL);
	bool IsAnsiExecAllowed(LPCWSTR asCmd);

	int NextEscCode(LPCWSTR lpBuffer, LPCWSTR lpEnd, wchar_t (&szPreDump)[SrvAnsi::CEAnsi_MaxPrevPart], DWORD& cchPrevPart, LPCWSTR& lpStart, LPCWSTR& lpNext, AnsiEscCode& Code, bool ReEntrance = FALSE);

	// Store absolute coords by relative ANSI values
	void SetScrollRegion(bool bRegion, int nStart = 0, int nEnd = 0);

};
