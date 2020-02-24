
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

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include <gtest/gtest.h>
#include "Header.h"
#include "ConEmu.h"
#include "Match.h"
#include "RConData.h"
#include "RealConsole.h"
#include "VConText.h"
#include <unordered_set>


TEST(CMatch, UnitTests)
{
	CEStr szDir;
	GetDirectory(szDir);

	std::wstring last_checked_file;
	std::unordered_set<std::wstring> known_files = {
		L"license.txt", L"portable.txt", L"whatsnew-conemu.txt", L"abc.cpp", L"def.h",
		L"c:\\abc.xls", L"file.ext", L"makefile", L"c:\\sources\\conemu\\realconsole.cpp"
		L"defresolve.cpp", L"conemuc.cpp", L"1.c", L"file.cpp", L"common\\pipeserver.h",
		L"c:\\sources\\farlib\\farctrl.pas", L"farctrl.pas", L"script.ps1", L"c:\\tools\\release.ps1",
		L"abc.py", L"/src/class.c", L"c:\\vc\\unicode_far\\macro.cpp",
	};
	CMatch match([&known_files, &last_checked_file](LPCWSTR asSrc, CEStr&) {
		last_checked_file = asSrc;
		const auto found = known_files.count(last_checked_file);
		return found;
	});
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

	auto UnitTestMatch = [&match](ExpandTextRangeType etr, LPCWSTR asLine, int anLineLen, int anMatchStart, int anMatchEnd, LPCWSTR asMatchText)
	{
		int iRc, iCmp;
		CRConDataGuard data;

		for (int i = anMatchStart; i <= anMatchEnd; i++)
		{
			iRc = match.Match(etr, asLine, anLineLen, i, data, 0);

			if (iRc <= 0)
			{
				FAIL() << L"Match: must be found; line=" << asLine << L"; match=" << asMatchText;
				break;
			}
			else if (match.mn_MatchLeft != anMatchStart || match.mn_MatchRight != anMatchEnd)
			{
				FAIL() << L"Match: do not match required range; line=" << asLine << L"; match=" << asMatchText;
				break;
			}

			iCmp = lstrcmp(match.ms_Match, asMatchText);
			if (iCmp != 0)
			{
				FAIL() << L"Match: iCmp != 0; line=" << asLine << L"; match=" << asMatchText;
				break;
			}
		}
	};

	auto UnitTestNoMatch = [&match](ExpandTextRangeType etr, LPCWSTR asLine, int anLineLen, int anStart, int anEnd)
	{
		int iRc;
		CRConDataGuard data;

		for (int i = anStart; i <= anEnd; i++)
		{
			iRc = match.Match(etr, asLine, anLineLen, i, data, 0);

			if (iRc > 0)
			{
				FAIL() << L"Match: must NOT be found; line=" << asLine;
				break;
			}
		}
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

				UnitTestNoMatch(Tests[i].etr, Tests[i].src, iSrcLen, iPrevStart, nStartIdx-1);
				iPrevStart = nStartIdx+iMatchLen;
				UnitTestMatch(Tests[i].etr, Tests[i].src, iSrcLen, nStartIdx, iPrevStart-1, Tests[i].matches[iMatchNo]);
			}
			else
			{
				nStartIdx = 0;
				UnitTestNoMatch(Tests[i].etr, Tests[i].src, iSrcLen, 0, iSrcLen);
				break;
			}

			// More matches waiting?
			if (Tests[i].matches[++iMatchNo] == NULL)
			{
				UnitTestNoMatch(Tests[i].etr, Tests[i].src, iSrcLen, iPrevStart, iSrcLen);
				break;
			}
		}
		//_ASSERTE(iRc == lstrlen(p->txtMatch));
		//_ASSERTE(match.m_Type == p->etrMatch);
	}

	::SetCurrentDirectoryW(szDir);
}
