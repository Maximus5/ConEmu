
/*
Copyright (c) 2014-2017 Maximus5
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
#define SHOWDEBUGSTR

#include "Header.h"
#include "Match.h"
#include "RConData.h"
#include "RealConsole.h"
#include "VConText.h"

#ifdef _DEBUG
#include "ConEmu.h"
#endif

#undef MatchTestAlert
#ifdef _DEBUG
#define MatchTestAlert() UnitMatchTestAlert()
#else
#define MatchTestAlert()
#endif

// В именах файлов недопустимы: "/\:|*?<>~t~r~n
const wchar_t gszBreak[] = {
					/*недопустимые в FS*/
					L'\"', '|', '*', '?', '<', '>', '\t', '\r', '\n',
					/*для простоты - учитываем и рамки*/
					ucArrowUp, ucArrowDown, ucDnScroll, ucUpScroll,
					ucBox100, ucBox75, ucBox50, ucBox25,
					ucBoxDblVert, ucBoxSinglVert, ucBoxDblVertSinglRight, ucBoxDblVertSinglLeft,
					ucBoxDblDownRight, ucBoxDblDownLeft, ucBoxDblUpRight,
					ucBoxDblUpLeft, ucBoxSinglDownRight, ucBoxSinglDownLeft, ucBoxSinglUpRight,
					ucBoxSinglUpLeft, ucBoxSinglDownDblHorz, ucBoxSinglUpDblHorz, ucBoxDblDownDblHorz,
					ucBoxDblUpDblHorz, ucBoxSinglDownHorz, ucBoxSinglUpHorz, ucBoxDblDownSinglHorz,
					ucBoxDblUpSinglHorz, ucBoxDblVertRight, ucBoxDblVertLeft,
					ucBoxSinglVertRight, ucBoxSinglVertLeft, ucBoxDblVertHorz,
					0};
#undef MATCH_SPACINGS
#define MATCH_SPACINGS L" \t\xB6\xB7\x2192\x25A1\x25D9\x266A"
const wchar_t gszSpacing[] = MATCH_SPACINGS; // Пробел, таб, остальные для режима "Show white spaces" в редакторе фара
// more quotation marks
const wchar_t gszQuotStart[] = L"‘«`\'([<{";
const wchar_t gszQuotEnd[] = L"’»`\')]>}";

CMatch::CMatch(CRealConsole* apRCon)
	:m_Type(etr_None)
	,mn_Row(-1), mn_Col(-1)
	,mn_MatchLeft(-1), mn_MatchRight(-1)
	,mn_Start(-1), mn_End(-1)
	,mn_SrcLength(-1)
	,mn_SrcFrom(-1)
	,mp_RCon(apRCon)
{
	ms_Protocol[0] = 0;
}

CMatch::~CMatch()
{
}

#ifdef _DEBUG
void CMatch::UnitTestAlert(LPCWSTR asLine, LPCWSTR asExpected, LPCWSTR pszText)
{
	OutputDebugString(pszText);
}

void CMatch::UnitMatchTestAlert()
{
	int iDbg = 0; // goto wrap; called
}

void CMatch::UnitTests()
{
	CEStr szDir;
	GetDirectory(szDir);

	CMatch match(NULL);
	struct TestMatch {
		LPCWSTR src; ExpandTextRangeType etr;
		bool bMatch; LPCWSTR matches[5];
		LPCWSTR pszTestCurDir;
	} Tests[] = {
		// Hyperlinks
		// RA layer request failed: PROPFIND request failed on '/svn': PROPFIND of '/svn': could
		// not connect to server (http://farmanager.googlecode.com) at /usr/lib/perl5/site_perl/Git/SVN.pm line 148
		// 1. Must not match last bracket, dot, comma, semicolon, etc.
		// 2. If url exceeds the line, must request from owner additional data
		//    if it is Far editor - the line must match the screen (no "tab" chars)
		{L"\t" L"(http://abc.com) <http://qwe.com> [http://rty.com] {http://def.com}" L"\t",
			etr_AnyClickable, true, {L"http://abc.com", L"http://qwe.com", L"http://rty.com", L"http://def.com"}},
		{L"\t" L"(http://abc.com) http://qwe.com; http://rty.com, http://def.com." L"\t",
			etr_AnyClickable, true, {L"http://abc.com", L"http://qwe.com", L"http://rty.com", L"http://def.com"}},
		{L"\t" L"text··http://www.abc.com/q?q··text" L"\t", // this line contains '·' which are visualisations of spaces in Far editor
			etr_AnyClickable, true, {L"http://www.abc.com/q?q"}},
		{L"\t" L"file://c:\\temp\\qqq.html" L"\t",
			etr_AnyClickable, true, {L"file://c:\\temp\\qqq.html"}},
		{L"\t" L"file:///c:\\temp\\qqq.html" L"\t",
			etr_AnyClickable, true, {L"file:///c:\\temp\\qqq.html"}},
		{L"\t" L"http://www.farmanager.com" L"\t",
			etr_AnyClickable, true, {L"http://www.farmanager.com"}},
		{L"\t" L"$ http://www.KKK.ru - левее слеша - не срабатывает" L"\t",
			etr_AnyClickable, true, {L"http://www.KKK.ru"}},
		{L"\t" L"C:\\ConEmu>http://www.KKK.ru - ..." L"\t",
			etr_AnyClickable, true, {L"http://www.KKK.ru"}},

		// Just a text files
		{L"\t" L"License.txt	Portable.txt    WhatsNew-ConEmu.txt" L"\t",
			etr_AnyClickable, true, {L"License.txt", L"Portable.txt", L"WhatsNew-ConEmu.txt"}, gpConEmu->ms_ConEmuBaseDir},
		{L"\t" L"License.txt:err" L"\t",
			etr_AnyClickable, true, {L"License.txt"}, gpConEmu->ms_ConEmuBaseDir},
		{L"\t" L" \" abc.cpp \" \"def.h\" " L"\t",
			etr_AnyClickable, true, {L"abc.cpp", L"def.h"}},
		{L"\t" L"class.func('C:\\abc.xls')" L"\t",
			etr_AnyClickable, true, {L"C:\\abc.xls"}},
		{L"\t" L"class::func('C:\\abc.xls')" L"\t",
			etr_AnyClickable, true, {L"C:\\abc.xls"}},
		{L"\t" L"file.ext 2" L"\t",
			etr_AnyClickable, true, {L"file.ext"}},
		{L"\t" L"makefile" L"\t",
			etr_AnyClickable, true, {L"makefile"}},

		// -- VC
		{L"\t" L"1>c:\\sources\\conemu\\realconsole.cpp(8104) : error C2065: 'qqq' : undeclared identifier" L"\t",
			etr_AnyClickable, true, {L"c:\\sources\\conemu\\realconsole.cpp(8104)"}},
		{L"\t" L"DefResolve.cpp(18) : error C2065: 'sdgagasdhsahd' : undeclared identifier" L"\t",
			etr_AnyClickable, true, {L"DefResolve.cpp(18)"}},
		{L"\t" L"DefResolve.cpp(18): warning: note xxx" L"\t",
			etr_AnyClickable, true, {L"DefResolve.cpp(18)"}},
		// -- GCC
		{L"\t" L"ConEmuC.cpp:49: error: 'qqq' does not name a type" L"\t",
			etr_AnyClickable, true, {L"ConEmuC.cpp:49"}},
		{L"\t" L"1.c:3: some message" L"\t",
			etr_AnyClickable, true, {L"1.c:3"}},
		{L"\t" L"file.cpp:29:29: error" L"\t",
			etr_AnyClickable, true, {L"file.cpp:29"}},
		// CPP Check
		{L"\t" L"[common\\PipeServer.h:1145]: (style) C-style pointer casting" L"\t",
			etr_AnyClickable, true, {L"common\\PipeServer.h:1145"}},
		// Delphi
		{L"\t" L"c:\\sources\\FarLib\\FarCtrl.pas(1002) Error: Undeclared identifier: 'PCTL_GETPLUGININFO'" L"\t",
			etr_AnyClickable, true, {L"c:\\sources\\FarLib\\FarCtrl.pas(1002)"}},
		// FPC
		{L"\t" L"FarCtrl.pas(1002,49) Error: Identifier not found 'PCTL_GETPLUGININFO'" L"\t",
			etr_AnyClickable, true, {L"FarCtrl.pas(1002,49)"}},
		// PowerShell
		{L"\t" L"Script.ps1:35 знак:23" L"\t",
			etr_AnyClickable, true, {L"Script.ps1:35"}},
		{L"\t" L"At C:\\Tools\\release.ps1:12 char:8" L"\t",
			etr_AnyClickable, true, {L"C:\\Tools\\release.ps1:12"}},
		// -- Possible?
		{L"\t" L"abc.py (3): some message" L"\t",
			etr_AnyClickable, true, {L"abc.py (3)"}},
		// ASM - подсвечивать нужно "test.asasm(1,1)"
		{L"\t" L"object.Exception@assembler.d(1239): test.asasm(1,1):" L"\t",
			etr_AnyClickable, true, {L"object.Exception@assembler.d(1239)", L"test.asasm(1,1)"}},
		// Issue 1594
		{L"\t" L"/src/class.c:123:m_func(...)" L"\t",
			etr_AnyClickable, true, {L"/src/class.c:123"}},
		{L"\t" L"/src/class.c:123: m_func(...)" L"\t",
			etr_AnyClickable, true, {L"/src/class.c:123"}},

		// -- False detects
		{L"\t" L"29.11.2011 18:31:47" L"\t",
			etr_AnyClickable, false, {}},
		{L"\t" L"C:\\VC\\unicode_far\\macro.cpp  1251 Ln 5951/8291 Col 51 Ch 39 0043h 13:54" L"\t",
			etr_AnyClickable, true, {L"C:\\VC\\unicode_far\\macro.cpp"}},
		{L"\t" L"InfoW1900->SettingsControl(sc.Handle, SCTL_FREE, 0, 0);" L"\t",
			etr_AnyClickable, false, {}},
		{L"\t" L"m_abc.func(1,2,3)" L"\t",
			etr_AnyClickable, false, {}},
		{NULL}
	};

	for (INT_PTR i = 0; Tests[i].src; i++)
	{
		INT_PTR nStartIdx;
		int iSrcLen = lstrlen(Tests[i].src) - 1;
		_ASSERTE(Tests[i].src && Tests[i].src[iSrcLen] == L'\t');

		// Loop through matches
		int iMatchNo = 0, iPrevStart = 0;
		while (true)
		{
			if (Tests[i].bMatch)
			{
				int iMatchLen = lstrlen(Tests[i].matches[iMatchNo]);
				LPCWSTR pszFirst = wcsstr(Tests[i].src, Tests[i].matches[iMatchNo]);
				_ASSERTE(pszFirst);
				nStartIdx = (pszFirst - Tests[i].src);

				match.UnitTestNoMatch(Tests[i].etr, Tests[i].src, iSrcLen, iPrevStart, nStartIdx-1);
				iPrevStart = nStartIdx+iMatchLen;
				match.UnitTestMatch(Tests[i].etr, Tests[i].src, iSrcLen, nStartIdx, iPrevStart-1, Tests[i].matches[iMatchNo]);
			}
			else
			{
				nStartIdx = 0;
				match.UnitTestNoMatch(Tests[i].etr, Tests[i].src, iSrcLen, 0, iSrcLen);
				break;
			}

			// More matches waiting?
			if (Tests[i].matches[++iMatchNo] == NULL)
			{
				match.UnitTestNoMatch(Tests[i].etr, Tests[i].src, iSrcLen, iPrevStart, iSrcLen);
				break;
			}
		}
		//_ASSERTE(iRc == lstrlen(p->txtMatch));
		//_ASSERTE(match.m_Type == p->etrMatch);
	}

	::SetCurrentDirectoryW(szDir);
}

void CMatch::UnitTestMatch(ExpandTextRangeType etr, LPCWSTR asLine, int anLineLen, int anMatchStart, int anMatchEnd, LPCWSTR asMatchText)
{
	int iRc, iCmp;
	CRConDataGuard data;

	for (int i = anMatchStart; i <= anMatchEnd; i++)
	{
		iRc = Match(etr, asLine, anLineLen, i, data, 0);

		if (iRc <= 0)
		{
			UnitTestAlert(asLine, asMatchText, L"Match: must be found\n");
			break;
		}
		else if (mn_MatchLeft != anMatchStart || mn_MatchRight != anMatchEnd)
		{
			UnitTestAlert(asLine, asMatchText, L"Match: do not match required range\n");
			break;
		}

		iCmp = lstrcmp(ms_Match, asMatchText);
		if (iCmp != 0)
		{
			UnitTestAlert(asLine, asMatchText, L"Match: iCmp != 0\n");
			break;
		}
	}
}

void CMatch::UnitTestNoMatch(ExpandTextRangeType etr, LPCWSTR asLine, int anLineLen, int anStart, int anEnd)
{
	int iRc;
	CRConDataGuard data;

	for (int i = anStart; i <= anEnd; i++)
	{
		iRc = Match(etr, asLine, anLineLen, i, data, 0);

		if (iRc > 0)
		{
			UnitTestAlert(asLine, NULL, L"Match: must NOT be found\n");
			break;
		}
	}
}
#endif

// Returns the length of matched string
int CMatch::Match(ExpandTextRangeType etr, LPCWSTR asLine/*This may be NOT 0-terminated*/, int anLineLen/*Length of buffer*/, int anFrom/*Cursor pos*/, CRConDataGuard& data, int nFromLine)
{
	m_Type = etr_None;
	ms_Match.Empty();
	mn_Row = mn_Col = -1;
	ms_Protocol[0] = 0;
	mn_MatchLeft = mn_MatchRight = -1;
	mn_Start = mn_End = -1;

	if (!asLine || !*asLine || (anFrom < 0) || (anLineLen <= anFrom))
		return 0;

	if (!m_SrcLine.Set(asLine, anLineLen))
    	return 0;
	mn_SrcFrom = anFrom;
	mn_SrcLength = anLineLen;

	int iRc = 0;

	if (etr == etr_Word)
	{
		if (MatchWord(m_SrcLine.ms_Val, anLineLen, anFrom, mn_MatchLeft, mn_MatchRight, data, nFromLine))
		{
			_ASSERTE(mn_MatchRight >= mn_MatchLeft);
			iRc = (mn_MatchRight - mn_MatchLeft + 1);
			m_Type = etr_Word;
		}
	}
	else if (etr == etr_AnyClickable)
	{
		if (MatchAny(data, nFromLine))
		{
			_ASSERTE(mn_MatchRight >= mn_MatchLeft);
			_ASSERTE(m_Type != etr_None);
			iRc = (mn_MatchRight - mn_MatchLeft + 1);
		}
		else if (MatchFileNoExt(data, nFromLine))
		{
			_ASSERTE(mn_MatchRight >= mn_MatchLeft);
			_ASSERTE(m_Type == etr_File);
			iRc = (mn_MatchRight - mn_MatchLeft + 1);
		}
	}
	else
	{
		_ASSERTE(FALSE && "Unsupported etr argument value!");
	}

	return iRc;
}


/* ****************************************** */

bool CMatch::IsFileLineTerminator(LPCWSTR pChar, LPCWSTR pszTermint)
{
	// Рассчитано на закрывающие : или ) или ] или ,
	if (wcschr(pszTermint, *pChar))
		return true;
	// Script.ps1:35 знак:23
	if (*pChar == L' ')
	{
		// few chars, colon, digits
		for (int i = 1; i < 20; i++)
		{
			if (pChar[i] == 0 || !isAlpha(pChar[i])) //wcschr(L" \t\xA0", pChar[i]))
				return true;
			if (pChar[i+1] == L':')
			{
				if (isDigit(pChar[i+2]))
				{
					for (int j = i+3; j < 25; j++)
					{
						if (isDigit(pChar[j]))
							continue;
						if (isSpace(pChar[j]))
							return true;
					}
				}
				break;
			}
		}
	}
	return false;
}

void CMatch::StoreMatchText(LPCWSTR asPrefix, LPCWSTR pszTrimRight)
{
	int iMailTo = asPrefix ? lstrlen(asPrefix/*L"mailto:"*/) : 0;
	INT_PTR cchTextMax = (mn_MatchRight - mn_MatchLeft + 1 + iMailTo);

	wchar_t* pszText = ms_Match.GetBuffer(cchTextMax);
	if (!pszText)
	{
		return; // Недостаточно памяти под текст?
	}

	if (iMailTo)
	{
		// Добавить префикс протокола
		lstrcpyn(pszText, asPrefix/*L"mailto:"*/, iMailTo+1);
		pszText += iMailTo;
		cchTextMax -= iMailTo;
	}

	if (pszTrimRight)
	{
		while ((mn_MatchRight > mn_MatchLeft) && wcschr(pszTrimRight, m_SrcLine.ms_Val[mn_MatchRight]))
			mn_MatchRight--;
	}

	// Return hyperlink target
	memmove(pszText, m_SrcLine.ms_Val+mn_MatchLeft, (mn_MatchRight - mn_MatchLeft + 1)*sizeof(*pszText));
	pszText[mn_MatchRight - mn_MatchLeft + 1] = 0;
}

bool CMatch::IsValidFile(LPCWSTR asFrom, int anLen, LPCWSTR pszInvalidChars, LPCWSTR pszSpacing, int& rnLen)
{
	while (anLen > 0)
	{
		wchar_t wc = asFrom[anLen-1];
		if (wcschr(pszSpacing, wc) || wcschr(pszInvalidChars, wc))
			anLen--;
		else
			break;
	}

	LPCWSTR pszFile, pszBadChar;

	if ((anLen <= 0) || !(pszFile = ms_FileCheck.Set(asFrom, anLen)) || ms_FileCheck.IsEmpty())
		return false;

	pszBadChar = wcspbrk(pszFile, pszInvalidChars);
	if (pszBadChar != NULL)
		return false;

	pszBadChar = wcspbrk(pszFile, pszSpacing);
	if (pszBadChar != NULL)
		return false;

	// where are you, regexps...
	if ((anLen <= 1) || !::IsFilePath(pszFile))
		return false;
	if (pszFile[0] == L'.' && (pszFile[1] == 0 || (pszFile[1] == L'.' && (pszFile[2] == 0 || (pszFile[2] == L'.' && (pszFile[3] == 0))))))
		return false;
	CharLowerBuff(ms_FileCheck.ms_Val, anLen);
	if ((wcschr(pszFile, L'.') == NULL)
		//&& (wcsncmp(pszFile, L"make", 4) != 0)
		&& (wcspbrk(pszFile, L"abcdefghijklmnopqrstuvwxyz") == NULL))
		return false;

	CEStr szFullPath;
	if (mp_RCon)
	{
		if (!mp_RCon->GetFileFromConsole(pszFile, szFullPath))
			return false;
	}

	rnLen = anLen;
	return true;
}

bool CMatch::FindRangeStart(int& crFrom/*[In/Out]*/, int& crTo/*[In/Out]*/, bool& bUrlMode, LPCWSTR pszBreak, LPCWSTR pszUrlDelim, LPCWSTR pszSpacing, LPCWSTR pszUrl, LPCWSTR pszProtocol, LPCWSTR pChar, int nLen)
{
	bool lbRc = false;

	// Курсор над скобкой "(3): ..." из текста "abc.py (3): ..."
	//if ((crFrom > 2) && wcschr(L"([", pChar[crFrom]) && wcschr(pszSpacing, pChar[crFrom-1]) && !wcschr(pszSpacing, pChar[crFrom-2]))
	//	crFrom--;

	int iAlphas = 0;

	// Курсор над комментарием?
	// Попробуем найти начало имени файла
	// 131026 Allows '?', otherwise links like http://go.com/fwlink/?LinkID=1 may fail
	while ((crFrom > 0)
		&& ((pChar[crFrom-1]==L'?') || !wcschr(bUrlMode ? pszUrlDelim : pszBreak, pChar[crFrom-1])))
	{
		// Check this before pszSpacing comparison because otherwise we'll fail on smth like
		// Bla-bla-bla C:\your-file.txt
		// if the mouse was over "C:\"
		if (isAlpha(pChar[crFrom]))
		{
			iAlphas++;
		}

		if (iAlphas > 0)
		{
			TODO("Если вводить поддержку powershell 'Script.ps1:35 знак:23' нужно будет не прерываться перед национальными словами 'знак'");
			// Disallow leading spaces
			if (wcschr(pszSpacing, pChar[crFrom-1]))
				break;
		}

		if (!bUrlMode && pChar[crFrom] == L'/')
		{
			if ((crFrom >= 2) && ((crFrom + 1) < nLen)
				&& ((pChar[crFrom+1] == L'/') && (pChar[crFrom-1] == L':')
					&& wcschr(pszUrl, pChar[crFrom-2]))) // как минимум одна буква на протокол
			{
				crFrom++;
			}

			if ((crFrom >= 3)
				&& ((pChar[crFrom-1] == L'/') // как минимум одна буква на протокол
					&& (((pChar[crFrom-2] == L':') && wcschr(pszUrl, pChar[crFrom-3])) // http://www.ya.ru
						|| ((crFrom >= 4) && (pChar[crFrom-2] == L'/') && (pChar[crFrom-3] == L':') && wcschr(pszUrl, pChar[crFrom-4])) // file:///c:\file.html
					))
				)
			{
				bUrlMode = true;
				crTo = crFrom-2;
				crFrom -= 3;
				while ((crFrom > 0) && wcschr(pszProtocol, pChar[crFrom-1]))
					crFrom--;
				break;
			}
			else if ((pChar[crFrom] == L'/') && (crFrom >= 1) && (pChar[crFrom-1] == L'/'))
			{
				crFrom++;
				break; // Комментарий в строке?
			}
		}

		crFrom--;

		if (pChar[crFrom] == L':')
		{
			if (pChar[crFrom+1] == L' ')
			{
				// ASM - подсвечивать нужно "test.asasm(1,1)"
				// object.Exception@assembler.d(1239): test.asasm(1,1):
				crFrom += 2;
				break;
			}
			else if (bUrlMode && pChar[crFrom+1] != L'\\' && pChar[crFrom+1] != L'/')
			{
				goto wrap; // Не оно
			}
		}
	}

	while (((crFrom+1) < nLen) && wcschr(pszSpacing, pChar[crFrom]))
		crFrom++;

	if (crFrom > crTo)
	{
		goto wrap; // Fail?
	}

	lbRc = true;
wrap:
	return lbRc;
}

bool CMatch::CheckValidUrl(int& crFrom/*[In/Out]*/, int& crTo/*[In/Out]*/, bool& bUrlMode, LPCWSTR pszUrlDelim, LPCWSTR pszUrl, LPCWSTR pszProtocol, LPCWSTR pChar, int nLen)
{
	bool lbRc = false;

	WARNING("Тут пока работаем в экранных координатах");

	// URL? (Курсор мог стоять над протоколом)
	while ((crTo < nLen) && wcschr(pszProtocol, pChar[crTo]))
		crTo++;
	if (((crTo+1) < nLen) && (pChar[crTo] == L':'))
	{
		if (((crTo+4) < nLen) && (pChar[crTo+1] == L'/') && (pChar[crTo+2] == L'/'))
		{
			bUrlMode = true;
			if (wcschr(pszUrl+2 /*пропустить ":/"*/, pChar[crTo+3])
				|| ((((crTo+5) < nLen) && (pChar[crTo+3] == L'/'))
					&& wcschr(pszUrl+2 /*пропустить ":/"*/, pChar[crTo+4]))
				)
			{
				crFrom = crTo;
				while ((crFrom > 0) && wcschr(pszProtocol, pChar[crFrom-1]))
					crFrom--;
			}
			else
			{
				goto wrap; // Fail
			}
		}
	}

	lbRc = true;
wrap:
	return lbRc;
}

bool CMatch::MatchFileNoExt(CRConDataGuard& data, int nFromLine)
{
	LPCWSTR pszLine = m_SrcLine.ms_Val;
	if (!pszLine || !*pszLine || (mn_SrcFrom < 0) || (mn_SrcLength <= mn_SrcFrom))
		return false;

	const wchar_t szLatin[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_";

	mn_MatchLeft = mn_SrcFrom;
	if (wcschr(szLatin, pszLine[mn_MatchLeft]))
	{
		while ((mn_MatchLeft > 0) && wcschr(szLatin, pszLine[mn_MatchLeft-1]))
		{
			mn_MatchLeft--;
		}
	}
	else
	{
		while (((mn_MatchLeft+1) < mn_SrcLength) && wcschr(szLatin, pszLine[mn_MatchLeft+1]))
		{
			mn_MatchLeft++;
		}
	}

	mn_MatchRight = mn_MatchLeft;
	while (((mn_MatchRight+1) < mn_SrcLength) && wcschr(szLatin, pszLine[mn_MatchRight+1]))
	{
		mn_MatchRight++;
	}

	int nNakedFileLen = 0;
	if (!IsValidFile(m_SrcLine.ms_Val+mn_MatchLeft, mn_MatchRight - mn_MatchLeft + 1, gszBreak, gszSpacing, nNakedFileLen))
		return false;

	mn_MatchRight = mn_MatchLeft + nNakedFileLen - 1;
	StoreMatchText(NULL, NULL);
	m_Type = etr_File;
	return true;
}

bool CMatch::MatchWord(LPCWSTR asLine/*This may be NOT 0-terminated*/, int anLineLen/*Length of buffer*/, int anFrom/*Cursor pos*/, int& rnStart, int& rnEnd, CRConDataGuard& data, int nFromLine)
{
	rnStart = rnEnd = anFrom;

	TODO("Проверить на ошибки после добавления горизонтальной прокрутки");

	if (!asLine || !*asLine || (anFrom < 0) || (anLineLen <= anFrom))
		return false;

	TODO("Setting to define word-break characters (was gh-328 issue)");

	struct cmp {
		static bool isChar(ucs32 inChar)
		{
			return (isCharSpace(inChar) || isCharSeparate(inChar));
		}
	};

	bool (*cmpFunc)(ucs32 inChar) = isCharNonWord;
	// If user double-clicks on "═════════════" - try to select this block of characters?
	if (isCharNonWord(asLine[rnStart]))
		cmpFunc = cmp::isChar;

	while ((rnStart > 0)
		&& !(cmpFunc(asLine[rnStart-1])))
	{
		rnStart--;
	}

	const wchar_t szLeftBkt[] = L"<({[", szRightBkt[] = L">)}]";
	int iStopOnRghtBkt = -1;

	// Trim leading punctuation except of "." (accept dot-files like ".bashrc")
	while (((rnStart+1) < rnEnd)
		&& (asLine[rnStart] != L'.') && isCharPunctuation(asLine[rnStart]))
	{
		if (iStopOnRghtBkt == -1)
		{
			const wchar_t* pchLeftBkt = wcschr(szLeftBkt, asLine[rnStart]);
			iStopOnRghtBkt = (int)(pchLeftBkt - szLeftBkt);
			_ASSERTE(iStopOnRghtBkt > 0 && (size_t)iStopOnRghtBkt < wcslen(szRightBkt));
		}
		rnStart++;
	}

	bool bStopOnBkt = false;

	while (((rnEnd+1) < anLineLen)
		&& !(cmpFunc(asLine[rnEnd+1])))
	{
		if ((iStopOnRghtBkt >= 0) && (asLine[rnEnd+1] == szRightBkt[iStopOnRghtBkt]))
		{
			bStopOnBkt = true;
			break;
		}
		rnEnd++;
	}

	if (!bStopOnBkt)
	{
		// Now trim trailing punctuation
		while (((rnEnd-1) > rnStart)
			&& isCharPunctuation(asLine[rnEnd]))
		{
			rnEnd--;
		}

		// If part contains (leading) brackets - don't trim trailing brackets
		for (int i = rnStart; i <= rnEnd; i++)
		{
			if (wcschr(szLeftBkt, asLine[i]))
			{
				// Include trailing brackets
				while (((rnEnd+1) < anLineLen)
					&& wcschr(szRightBkt, asLine[rnEnd+1]))
				{
					rnEnd++;
				}
				// Done
				break;
			}
		}
	}

	// Done
	StoreMatchText(NULL, NULL);

	return true;
}

// Used for URL wrapped on several lines
bool CMatch::GetNextLine(CRConDataGuard& data, int nLine)
{
	// Perhaps not only for "URL"?

	if (!data.isValid())
		return false;

	ConsoleLinePtr line;
	if (!data->GetConsoleLine(nLine, line) || line.nLen <= 0 || !line.pChar)
		return false;

	// ConsoleLinePtr doesn't contain Z-terminated strings, only RAW characters
	CEStr add;
	wchar_t* ptr = add.GetBuffer(line.nLen);
	if (!ptr)
		return false;
	wmemmove(ptr, line.pChar, line.nLen);
	ptr[line.nLen] = 0;
	// Append new data
	if (!lstrmerge(&m_SrcLine.ms_Val, ptr))
		return false;
	mn_SrcLength += line.nLen;
	return true;
}

bool CMatch::MatchAny(CRConDataGuard& data, int nFromLine)
{
	bool bFound = false;

	// В именах файлов недопустимы: "/\:|*?<>~t~r~n а для простоты - учитываем и рамки
	const wchar_t* pszBreak = gszBreak;
	const wchar_t* pszSpacing = gszSpacing; // Пробел, таб, остальные для режима "Show white spaces" в редакторе фара
	const wchar_t* pszSeparat = L" \t:(";
	const wchar_t* pszTermint = L":)],";
	const wchar_t* pszDigits  = L"0123456789";
	const wchar_t* pszSlashes = L"/\\";
	const wchar_t* pszUrl = L":/\\:%#ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz;?@&=+$,-_.!~*'()0123456789";
	const wchar_t* pszUrlTrimRight = L".,;";
	const wchar_t* pszProtocol = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_+-.";
	const wchar_t* pszEMail = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_+-.";
	const wchar_t* pszUrlDelim = L"\\\"<>{}[]^`'\r\n" MATCH_SPACINGS;
	const wchar_t* pszUrlFileDelim = L"\"<>^\r\n" MATCH_SPACINGS;
	const wchar_t* pszEndBrackets = L">])}";
	const wchar_t* pszPuctuators = L".,:;…!?";
	int nColons = 0;
	bool bUrlMode = false, bMaybeMail = false;
	SHORT MailX = -1;
	bool bDigits = false, bLineNumberFound = false, bWasSeparator = false;
	bool bWasPunctuator = false;
	bool bNakedFile = false;
	int nNakedFileLen = 0;
	enum {
		ef_NotFound = 0,
		ef_DotFound = 1,
		ef_ExtFound = 2,
	} iExtFound = ef_NotFound;
	int iBracket = 0;
	int iSpaces = 0;
	int iQuotStart = -1;
	const wchar_t* pszTest;

	mn_MatchLeft = mn_MatchRight = mn_SrcFrom;

	// Курсор над знаком препинания и за ним пробел? Допускать только ':' - это для строк/колонок с ошибками
	if ((wcschr(L";,.", m_SrcLine.ms_Val[mn_SrcFrom])) && (((mn_SrcFrom+1) >= mn_SrcLength) || wcschr(pszSpacing, m_SrcLine.ms_Val[mn_SrcFrom+1])))
	{
		MatchTestAlert();
		goto wrap;
	}

	// Курсор над протоколом? (http, etc)
	if (!CheckValidUrl(mn_MatchLeft, mn_MatchRight, bUrlMode, pszUrlDelim, pszUrl, pszProtocol, m_SrcLine.ms_Val, mn_SrcLength))
	{
		MatchTestAlert();
		goto wrap;
	}

	if (!bUrlMode)
	{
		// Not URL mode, restart searching
		mn_MatchLeft = mn_MatchRight = mn_SrcFrom;

		// Курсор над комментарием?
		// Попробуем найти начало имени файла
		if (!FindRangeStart(mn_MatchLeft, mn_MatchRight, bUrlMode, pszBreak, pszUrlDelim, pszSpacing, pszUrl, pszProtocol, m_SrcLine.ms_Val, mn_SrcLength))
		{
			MatchTestAlert();
			goto wrap;
		}

		// Try again with URL?
		if (mn_MatchLeft < mn_SrcFrom)
		{
			if (!CheckValidUrl(mn_MatchLeft, mn_MatchRight, bUrlMode, pszUrlDelim, pszUrl, pszProtocol, m_SrcLine.ms_Val, mn_SrcLength))
			{
				MatchTestAlert();
				goto wrap;
			}
		}
	}

	// Starts with quotation?
	if ((pszTest = wcschr(gszQuotStart, m_SrcLine.ms_Val[mn_MatchLeft])) != NULL)
	{
		iQuotStart = (int)(pszTest - gszQuotStart);
	}

	// Чтобы корректно флаги обработались (типа наличие расширения и т.п.)
	mn_MatchRight = mn_MatchLeft;

	// Теперь - найти конец.
	// Считаем, что для файлов конец это двоеточие, после которого идет описание ошибки
	// Для протоколов (http/...) - первый недопустимый символ

	TODO("Можно бы и просто открытие файлов прикрутить, без требования 'строки с ошибкой'");

	// -- VC
	// 1>c:\sources\conemu\realconsole.cpp(8104) : error C2065: 'qqq' : undeclared identifier
	// DefResolve.cpp(18) : error C2065: 'sdgagasdhsahd' : undeclared identifier
	// DefResolve.cpp(18): warning: note xxx
	// -- GCC
	// ConEmuC.cpp:49: error: 'qqq' does not name a type
	// 1.c:3: some message
	// file.cpp:29:29: error
	// CPP Check
	// [common\PipeServer.h:1145]: (style) C-style pointer casting
	// Delphi
	// c:\sources\FarLib\FarCtrl.pas(1002) Error: Undeclared identifier: 'PCTL_GETPLUGININFO'
	// FPC
	// FarCtrl.pas(1002,49) Error: Identifier not found "PCTL_GETPLUGININFO"
	// PowerShell
	// Script.ps1:35 знак:23
	// -- Possible?
	// abc.py (3): some message
	// ASM - подсвечивать нужно "test.asasm(1,1)"
	// object.Exception@assembler.d(1239): test.asasm(1,1):
	// Issue 1594
	// /src/class.c:123:m_func(...)
	// /src/class.c:123: m_func(...)

	// -- URL's
	// file://c:\temp\qqq.html
	// http://www.farmanager.com
	// $ http://www.KKK.ru - левее слеша - не срабатывает
	// C:\ConEmu>http://www.KKK.ru - ...
	// -- False detects
	// 29.11.2011 18:31:47
	// C:\VC\unicode_far\macro.cpp  1251 Ln 5951/8291 Col 51 Ch 39 0043h 13:54
	// InfoW1900->SettingsControl(sc.Handle, SCTL_FREE, 0, 0);

	// Нас не интересуют строки типа "11.05.2010 10:20:35"
	// В имени файла должна быть хотя бы одна буква (расширение), причем английская
	// Поехали
	if (bUrlMode)
	{
		LPCWSTR pszDelim = (wcsncmp(m_SrcLine.ms_Val+mn_MatchLeft, L"file://", 7) == 0) ? pszUrlFileDelim : pszUrlDelim;
		int nextLine = 1;
		while (true)
		{
			if ((mn_MatchRight+1) >= mn_SrcLength && data.isValid())
			{
				if (!GetNextLine(data, nextLine+nFromLine))
					break;
				++nextLine;
			}

			if (((mn_MatchRight+1) >= mn_SrcLength) || wcschr(pszDelim, m_SrcLine.ms_Val[mn_MatchRight+1]))
			{
				break;
			}

			mn_MatchRight++;
		}
		DEBUGTEST(int i4break = 0);
	}
	else while ((mn_MatchRight+1) < mn_SrcLength)
	{
		if ((m_SrcLine.ms_Val[mn_MatchRight] == L'/') && ((mn_MatchRight+1) < mn_SrcLength) && (m_SrcLine.ms_Val[mn_MatchRight+1] == L'/')
			&& !((mn_MatchRight > 1) && (m_SrcLine.ms_Val[mn_MatchRight] == L':'))) // и НЕ URL адрес
		{
			MatchTestAlert();
			goto wrap; // Не оно (комментарий в строке)
		}

		if (bWasSeparator // " \t:("
			&& (isDigit(m_SrcLine.ms_Val[mn_MatchRight])
				|| (bDigits && (m_SrcLine.ms_Val[mn_MatchRight] == L',')))) // FarCtrl.pas(1002,49) Error:
		{
			if (!bDigits && (mn_MatchLeft < mn_MatchRight) /*&& (m_SrcLine.ms_Val[mn_MatchRight-1] == L':')*/)
			{
				bDigits = true;
				// Если перед разделителем был реальный файл - сразу выставим флажок "номер строки найден"
				if (IsValidFile(m_SrcLine.ms_Val+mn_MatchLeft, mn_MatchRight - mn_MatchLeft - 1, pszBreak, pszSpacing, nNakedFileLen))
				{
					bLineNumberFound = true;
				}
				// Skip leading quotation mark?
				else if ((iQuotStart >= 0)
					&& IsValidFile(m_SrcLine.ms_Val+mn_MatchLeft+1, mn_MatchRight - mn_MatchLeft - 2, pszBreak, pszSpacing, nNakedFileLen))
				{
					mn_MatchLeft++;
					bLineNumberFound = true;
				}
			}
		}
		else if (bWasSeparator && bLineNumberFound
			&& wcschr(L":)] \t", m_SrcLine.ms_Val[mn_MatchRight]))
		{
			//// gcc такие строки тоже может выкинуть
			//// file.cpp:29:29: error
			//mn_MatchRight--;
			break;
		}
		else
		{
			if (iExtFound != ef_ExtFound)
			{
				if (iExtFound == ef_NotFound)
				{
					if (m_SrcLine.ms_Val[mn_MatchRight] == L'.')
						iExtFound = ef_ExtFound;
				}
				else
				{
					// Не особо заморачиваясь с точками и прочим. Просто небольшая страховка от ложных срабатываний...
					if ((m_SrcLine.ms_Val[mn_MatchRight] >= L'a' && m_SrcLine.ms_Val[mn_MatchRight] <= L'z') || (m_SrcLine.ms_Val[mn_MatchRight] >= L'A' && m_SrcLine.ms_Val[mn_MatchRight] <= L'Z'))
					{
						iExtFound = ef_ExtFound;
						iBracket = 0;
					}
				}
			}

			if (iExtFound == 2)
			{
				if (m_SrcLine.ms_Val[mn_MatchRight] == L'.')
				{
					iExtFound = ef_DotFound;
					iBracket = 0;
				}
				else if (wcschr(pszSlashes, m_SrcLine.ms_Val[mn_MatchRight]) != NULL)
				{
					// Был слеш, значит расширения - еще нет
					iExtFound = ef_NotFound;
					iBracket = 0;
					bWasSeparator = false;
				}
				else if (wcschr(pszSpacing, m_SrcLine.ms_Val[mn_MatchRight]) && wcschr(pszSpacing, m_SrcLine.ms_Val[mn_MatchRight-1]))
				{
					// Слишком много пробелов
					iExtFound = ef_NotFound;
					iBracket = 0;
					bWasSeparator = false;
				}
				// Stop on naked file if we found space after it
				else if (!bLineNumberFound && !bMaybeMail && wcschr(pszSpacing, m_SrcLine.ms_Val[mn_MatchRight])
					// But don't stop if there is a line number: "abc.py (3): ..."
					&& !(((mn_MatchRight+3) < mn_SrcLength)
							&& (m_SrcLine.ms_Val[mn_MatchRight+1] == L'(')
							&& isDigit(m_SrcLine.ms_Val[mn_MatchRight+2]))
					// Is this a file without digits (line/col)?
					&& IsValidFile(m_SrcLine.ms_Val+mn_MatchLeft, mn_MatchRight - mn_MatchLeft + 1, pszBreak, pszSpacing, nNakedFileLen))
				{
					// File without digits, just for opening in the editor
					bNakedFile = true;
					break;
				}
				// Stop if after colon there is another letter but a digit
				else if (!bLineNumberFound && !bMaybeMail
					&& (m_SrcLine.ms_Val[mn_MatchRight] == L':')
							&& (((mn_MatchRight+1) >= mn_SrcLength)
								|| !isDigit(m_SrcLine.ms_Val[mn_MatchRight+1]))
					// Is this a file without digits (line/col)?
					&& IsValidFile(m_SrcLine.ms_Val+mn_MatchLeft, mn_MatchRight - mn_MatchLeft, pszBreak, pszSpacing, nNakedFileLen))
				{
					// File without digits, just for opening in the editor
					bNakedFile = true;
					mn_MatchRight--;
					break;
				}
				else
				{
					bWasSeparator = (wcschr(pszSeparat, m_SrcLine.ms_Val[mn_MatchRight]) != NULL);
				}
			}

			if ((iQuotStart >= 0) && (m_SrcLine.ms_Val[mn_MatchRight] == gszQuotEnd[iQuotStart]))
			{
				bNakedFile = IsValidFile(m_SrcLine.ms_Val+mn_MatchLeft+1, mn_MatchRight - mn_MatchLeft - 1, pszBreak, pszSpacing, nNakedFileLen);
				if (bNakedFile || bMaybeMail)
				{
					mn_MatchLeft++;
					mn_MatchRight--;
					break;
				}
			}

			if (bWasPunctuator && !bLineNumberFound)
			{
				if (bMaybeMail)
				{
					// Если после мейла нашли что-то кроме точки
					if ((m_SrcLine.ms_Val[mn_MatchRight-1] != L'.')
						// или после точки - пробельный символ
						|| wcschr(pszSpacing, m_SrcLine.ms_Val[mn_MatchRight]))
					{
						break;
					}
				}
				else if (wcschr(pszSpacing, m_SrcLine.ms_Val[mn_MatchRight]))
				{
					bNakedFile = IsValidFile(m_SrcLine.ms_Val+mn_MatchLeft, mn_MatchRight - mn_MatchLeft - 1, pszBreak, pszSpacing, nNakedFileLen);
					if (bNakedFile)
					{
						mn_MatchRight--;
						break;
					}
				}
			}

			bWasPunctuator = (wcschr(pszPuctuators, m_SrcLine.ms_Val[mn_MatchRight]) != NULL);

			// Рассчитано на закрывающие : или ) или ] или ,
			_ASSERTE(pszTermint[0]==L':' && pszTermint[1]==L')' && pszTermint[2]==L']' && pszTermint[3]==L',' && pszTermint[4]==0);
			// Script.ps1:35 знак:23
			if (bDigits && IsFileLineTerminator(m_SrcLine.ms_Val+mn_MatchRight, pszTermint))
			{
				// Validation
				if (((m_SrcLine.ms_Val[mn_MatchRight] == L':' || m_SrcLine.ms_Val[mn_MatchRight] == L' ')
						// Issue 1594: /src/class.c:123:m_func(...)
						/* && (wcschr(pszSpacing, m_SrcLine.ms_Val[mn_MatchRight+1])
							|| wcschr(pszDigits, m_SrcLine.ms_Val[mn_MatchRight+1]))*/)
				// Если номер строки обрамлен скобками - скобки должны быть сбалансированы
				|| ((m_SrcLine.ms_Val[mn_MatchRight] == L')') && (iBracket == 1)
						&& ((m_SrcLine.ms_Val[mn_MatchRight+1] == L':')
							|| wcschr(pszSpacing, m_SrcLine.ms_Val[mn_MatchRight+1])
							|| wcschr(pszDigits, m_SrcLine.ms_Val[mn_MatchRight+1])))
				// [file.cpp:1234]: (cppcheck)
				|| ((m_SrcLine.ms_Val[mn_MatchRight] == L']') && (m_SrcLine.ms_Val[mn_MatchRight+1] == L':'))
					)
				{
					//_ASSERTE(bLineNumberFound==false);
					//bLineNumberFound = true;
					break; // found?
				}
			}

			// Issue 1758: Support file/line format for php: C:\..\test.php:28
			if (bDigits && !wcschr(pszSpacing, m_SrcLine.ms_Val[mn_MatchRight]))
			{
				bDigits = false;
			}

			switch (m_SrcLine.ms_Val[mn_MatchRight])
			{
			// Пока регулярок нет...
			case L'(': iBracket++; break;
			case L')': iBracket--; break;
			case L'/': case L'\\': iBracket = 0; break;
			case L'@':
				if (MailX != -1)
				{
					bMaybeMail = false;
				}
				else if (((mn_MatchRight > 0) && wcschr(pszEMail, m_SrcLine.ms_Val[mn_MatchRight-1]))
					&& (((mn_MatchRight+1) < mn_SrcLength) && wcschr(pszEMail, m_SrcLine.ms_Val[mn_MatchRight+1])))
				{
					bMaybeMail = true;
					MailX = mn_MatchRight;
				}
				break;
			}

			if (m_SrcLine.ms_Val[mn_MatchRight] == L':')
				nColons++;
			else if (m_SrcLine.ms_Val[mn_MatchRight] == L'\\' || m_SrcLine.ms_Val[mn_MatchRight] == L'/')
				nColons = 0;
		}

		if (nColons >= 2)
			break;

		mn_MatchRight++;
		if (wcschr(pszBreak, m_SrcLine.ms_Val[mn_MatchRight]))
		{
			if (bMaybeMail)
				break;
			if ((bLineNumberFound) || !IsValidFile(m_SrcLine.ms_Val+mn_MatchLeft, mn_MatchRight - mn_MatchLeft + 1, pszBreak, pszSpacing, nNakedFileLen))
			{
				MatchTestAlert();
				goto wrap; // Не оно
			}
			// File without digits, just for opening in the editor
			_ASSERTE(!bLineNumberFound);
			bNakedFile = true;
			break;
		}
		else if (wcschr(pszSpacing, m_SrcLine.ms_Val[mn_MatchRight]))
		{
			if ((++iSpaces) > 1)
			{
				if ((bLineNumberFound) || !IsValidFile(m_SrcLine.ms_Val+mn_MatchLeft, mn_MatchRight - mn_MatchLeft + 1, pszBreak, pszSpacing, nNakedFileLen))
				{
					if (!bLineNumberFound && bMaybeMail)
						break;
					MatchTestAlert();
					goto wrap; // Не оно?
				}
				// File without digits, just for opening in the editor
				_ASSERTE(!bLineNumberFound);
				bNakedFile = true;
				break;
			}
		}
		else
		{
			iSpaces = 0;
		}
	} // end of 'while ((mn_MatchRight+1) < mn_SrcLength)'

	if (bUrlMode)
	{
		// Считаем, что OK
		bMaybeMail = false;
		// Cut off ending punctuators
		if (wcschr(pszPuctuators, m_SrcLine.ms_Val[mn_MatchRight]))
			mn_MatchRight--;
		// Cut off ending bracket if it is
		if (wcschr(pszEndBrackets, m_SrcLine.ms_Val[mn_MatchRight]))
			mn_MatchRight--;
	}
	else
	{
		if (!bNakedFile && !bMaybeMail && ((mn_MatchRight+1) == mn_SrcLength) && !bLineNumberFound && (iExtFound == ef_ExtFound))
		{
			bNakedFile = IsValidFile(m_SrcLine.ms_Val+mn_MatchLeft, mn_MatchRight - mn_MatchLeft + 1, pszBreak, pszSpacing, nNakedFileLen);
		}

		if (bLineNumberFound)
			bMaybeMail = false;

		if (bNakedFile)
		{
			// correct the end pos
			mn_MatchRight = mn_MatchLeft + nNakedFileLen - 1;
			_ASSERTE(mn_MatchRight > mn_MatchLeft);
			// and no need to check for colon
		}
		else if (bMaybeMail)
		{
			// no need to check for colon
		}
		else if ((m_SrcLine.ms_Val[mn_MatchRight] != L':'
				&& m_SrcLine.ms_Val[mn_MatchRight] != L' '
				&& !((m_SrcLine.ms_Val[mn_MatchRight] == L')') && iBracket == 1)
				&& !((m_SrcLine.ms_Val[mn_MatchRight] == L']') && (m_SrcLine.ms_Val[mn_MatchRight+1] == L':'))
			)
			|| !bLineNumberFound || (nColons > 2))
		{
			MatchTestAlert();
			goto wrap;
		}

		if (bMaybeMail || (!bMaybeMail && !bNakedFile && m_SrcLine.ms_Val[mn_MatchRight] != L')'))
			mn_MatchRight--;

		// Откатить ненужные пробелы
		while ((mn_MatchLeft < mn_MatchRight) && wcschr(pszSpacing, m_SrcLine.ms_Val[mn_MatchLeft]))
			mn_MatchLeft++;
		while ((mn_MatchRight > mn_MatchLeft) && wcschr(pszSpacing, m_SrcLine.ms_Val[mn_MatchRight]))
			mn_MatchRight--;

		if (bMaybeMail)
		{
			// Для мейлов - проверяем допустимые символы (чтобы пробелов не было и прочего мусора)
			int x = MailX - 1; _ASSERTE(x>=0);
			while ((x > 0) && wcschr(pszEMail, m_SrcLine.ms_Val[x-1]))
				x--;
			mn_MatchLeft = x;
			x = MailX + 1; _ASSERTE(x<mn_SrcLength);
			while (((x+1) < mn_SrcLength) && wcschr(pszEMail, m_SrcLine.ms_Val[x+1]))
				x++;
			mn_MatchRight = x;
		}
		else if (bNakedFile)
		{
			// ???
		}
		else
		{
			if ((mn_MatchLeft + 4) > mn_MatchRight) // 1.c:1: //-V112
			{
				// Слишком коротко, считаем что не оно
				MatchTestAlert();
				goto wrap;
			}

			// Проверить, чтобы был в наличии номер строки
			if (!(m_SrcLine.ms_Val[mn_MatchRight] >= L'0' && m_SrcLine.ms_Val[mn_MatchRight] <= L'9') // ConEmuC.cpp:49:
				&& !(m_SrcLine.ms_Val[mn_MatchRight] == L')' && (m_SrcLine.ms_Val[mn_MatchRight-1] >= L'0' && m_SrcLine.ms_Val[mn_MatchRight-1] <= L'9'))) // ConEmuC.cpp(49) :
			{
				MatchTestAlert();
				goto wrap; // Номера строки нет
			}
			// [file.cpp:1234]: (cppcheck) ?
			if ((m_SrcLine.ms_Val[mn_MatchRight+1] == L']') && (m_SrcLine.ms_Val[mn_MatchLeft] == L'['))
				mn_MatchLeft++;
			// Чтобы даты ошибочно не подсвечивать:
			// 29.11.2011 18:31:47
			{
				bool bNoDigits = false;
				for (int i = mn_MatchLeft; i <= mn_MatchRight; i++)
				{
					if (m_SrcLine.ms_Val[i] < L'0' || m_SrcLine.ms_Val[i] > L'9')
					{
						bNoDigits = true;
					}
				}
				if (!bNoDigits)
				{
					MatchTestAlert();
					goto wrap;
				}
			}
			// -- уже включены // Для красивости в VC включить скобки
			//if ((m_SrcLine.ms_Val[mn_MatchRight] == L')') && (m_SrcLine.ms_Val[mn_MatchRight+1] == L':'))
			//	mn_MatchRight++;
		}
	} // end "else / if (bUrlMode)"

	// Check mouse pos, it must be inside region
	if ((mn_SrcFrom < mn_MatchLeft) || (mn_MatchRight < mn_SrcFrom))
	{
		MatchTestAlert();
		goto wrap;
	}

	// Ok
	if (mn_MatchRight >= mn_MatchLeft)
	{
		bFound = true;

		_ASSERTE(!bMaybeMail || !bUrlMode); // Одновременно - флаги не могут быть выставлены!
		LPCWSTR pszPrefix = (bMaybeMail && !bUrlMode) ? L"mailto:" : NULL;
		if (bMaybeMail && !bUrlMode)
			bUrlMode = true;

		StoreMatchText(pszPrefix, bUrlMode ? pszUrlTrimRight : NULL);

		#ifdef _DEBUG
		if (!bUrlMode && wcsstr(ms_Match.ms_Val, L"//")!=NULL)
		{
			_ASSERTE(FALSE);
		}
		#endif
	}

	if (bUrlMode)
		m_Type = etr_Url;
	else
		m_Type = (bLineNumberFound ? etr_FileRow : etr_File);

wrap:
	return bFound;
}
