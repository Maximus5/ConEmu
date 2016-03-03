
/*
Copyright (c) 2014-2016 Maximus5
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

#include "../common/CmdLine.h"

class CMatch;
class CRealConsole;

typedef int (*CMatchGetNextPart)(LPARAM lParam, CMatch* pMatch);

class CMatch
{
public:
	// What was found
	ExpandTextRangeType m_Type;
	// That may be converted, cleared or expanded...
	CEStr ms_Match;
	// Compiler errors - row, col
	int mn_Row, mn_Col;
	// When that was URL, this contains the protocol
	wchar_t ms_Protocol[32];
	// Left/right indexes of matched string in the ms_SrcLine
	int mn_MatchLeft, mn_MatchRight;

	// Indexes in the source string, informational
	int mn_Start, mn_End;

	// What was the source
	CEStr m_SrcLine/*Copy of the source buffer (one line)*/;
	int mn_SrcLength/*Current length*/;
	int mn_SrcFrom/*Cursor pos*/;

protected:
	CRealConsole* mp_RCon;

public:
	CMatch(CRealConsole* apRCon);
	~CMatch();

public:
	// Returns the length of matched string
	int Match(ExpandTextRangeType etr, LPCWSTR asLine/*This may be NOT 0-terminated*/, int anLineLen/*Length of buffer*/, int anFrom/*Cursor pos*/);

protected:
	static bool IsFileLineTerminator(LPCWSTR pChar, LPCWSTR pszTermint);
	static bool FindRangeStart(int& crFrom/*[In/Out]*/, int& crTo/*[In/Out]*/, bool& bUrlMode, LPCWSTR pszBreak, LPCWSTR pszUrlDelim, LPCWSTR pszSpacing, LPCWSTR pszUrl, LPCWSTR pszProtocol, LPCWSTR pChar, int nLen);
	static bool CheckValidUrl(int& crFrom/*[In/Out]*/, int& crTo/*[In/Out]*/, bool& bUrlMode, LPCWSTR pszUrlDelim, LPCWSTR pszUrl, LPCWSTR pszProtocol, LPCWSTR pChar, int nLen);
protected:
	bool MatchAny();
	bool MatchFileNoExt();
	bool MatchWord(LPCWSTR asLine/*This may be NOT 0-terminated*/, int anLineLen/*Length of buffer*/, int anFrom/*Cursor pos*/, int& rnStart, int& rnEnd);
	void StoreMatchText(LPCWSTR asPrefix, LPCWSTR pszTrimRight);
protected:
	CEStr ms_FileCheck;
	bool IsValidFile(LPCWSTR asFrom, int anLen, LPCWSTR pszInvalidChars, LPCWSTR pszSpacing, int& rnLen);

public:
	#ifdef _DEBUG
	static void UnitTests();
	void UnitTestMatch(ExpandTextRangeType etr, LPCWSTR asLine, int anLineLen, int anMatchStart, int anMatchEnd, LPCWSTR asMatchText);
	void UnitTestNoMatch(ExpandTextRangeType etr, LPCWSTR asLine, int anLineLen, int anStart, int anEnd);
	void UnitTestAlert(LPCWSTR asLine, LPCWSTR asExpected, LPCWSTR pszText);
	void UnitMatchTestAlert();
	#endif
};
