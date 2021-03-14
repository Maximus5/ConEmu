
/*
Copyright (c) 2014-present Maximus5
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
#include <functional>

class CMatch;
class CRealConsole;
class CRConDataGuard;

typedef int (*CMatchGetNextPart)(LPARAM lParam, CMatch* pMatch);

class CMatch
{
public:
	// What was found
	ExpandTextRangeType m_Type = etr_None;
	// That may be converted, cleared or expanded...
	CEStr ms_Match;
	// Compiler errors - row, col
	int mn_Row = -1, mn_Col = -1;
	// When that was URL, this contains the protocol
	wchar_t ms_Protocol[32] = L"";
	// Left/right indexes of matched string in the ms_SrcLine
	int mn_MatchLeft = -1, mn_MatchRight = -1;

	// Indexes in the source string, informational
	int mn_Start = -1, mn_End = -1;

	// What was the source, Copy of the source buffer (one line)
	CEStr m_SrcLine;
	int mn_SrcLength = -1;
	//Cursor pos
	int mn_SrcFrom = -1;

protected:
	std::function<bool(LPCWSTR asSrc, CEStr& szFull)> getFileFromConsole_;

public:
	CMatch(std::function<bool(LPCWSTR asSrc, CEStr& szFull)>&& getFileFromConsole);
	~CMatch();

public:
	/// @brief Returns the length of matched string
	/// @param etr ExpandTextRangeType flags
	/// @param asLine Source text, may be NOT 0-terminated
	/// @param anLineLen Length of buffer
	/// @param anFrom Cursor pos (index, where we start range expanding)
	/// @param data CRConDataGuard
	/// @param nFromLine line index in data
	/// @return 0 - if not found, otherwise the length of the found (matched) fragment
	int Match(ExpandTextRangeType etr, const wchar_t* asLine, int anLineLen, int anFrom, CRConDataGuard& data, int nFromLine);

protected:
	static bool IsFileLineTerminator(LPCWSTR pChar, LPCWSTR pszTermint);
	static bool FindRangeStart(int& crFrom/*[In/Out]*/, int& crTo/*[In/Out]*/, bool& bUrlMode, LPCWSTR pszBreak, LPCWSTR pszUrlDelim, LPCWSTR pszSpacing, LPCWSTR pszUrl, LPCWSTR pszProtocol, LPCWSTR pChar, int nLen);
	static bool CheckValidUrl(int& crFrom/*[In/Out]*/, int& crTo/*[In/Out]*/, bool& bUrlMode, LPCWSTR pszUrlDelim, LPCWSTR pszUrl, LPCWSTR pszProtocol, LPCWSTR pChar, int nLen);
protected:
	bool MatchAny(CRConDataGuard& data, int nFromLine);
	bool MatchFileNoExt(CRConDataGuard& data, int nFromLine);
	bool MatchWord(LPCWSTR asLine/*This may be NOT 0-terminated*/, int anLineLen/*Length of buffer*/, int anFrom/*Cursor pos*/, int& rnStart, int& rnEnd, CRConDataGuard& data, int nFromLine);
	void StoreMatchText(LPCWSTR asPrefix, LPCWSTR pszTrimRight);
	bool GetNextLine(CRConDataGuard& data, int nLine);
protected:
	CEStr ms_FileCheck;
	bool IsValidFile(LPCWSTR asFrom, int anLen, LPCWSTR pszInvalidChars, LPCWSTR pszSpacing, int& rnLen);
};
