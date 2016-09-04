
/*
Copyright (c) 2016 Maximus5
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

#include "../common/Common.h"
#include "../common/WCodePage.h"
#include "../ConEmu/version.h"

#include "ConEmuC.h"
#include "UnicodeTest.h"

void PrintConsoleInfo()
{
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);

	wchar_t szInfo[1024]; DWORD nTmp;
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	CONSOLE_CURSOR_INFO ci = {};

	wchar_t szMinor[8] = L""; lstrcpyn(szMinor, _T(MVV_4a), countof(szMinor));
	//msprintf не умеет "%02u"
	_wsprintf(szInfo, SKIPLEN(countof(szInfo))
		L"ConEmu %02u%02u%02u%s %s\r\n"
		L"OS Version: %u.%u.%u (%u:%s)\r\n",
		MVV_1, MVV_2, MVV_3, szMinor, WIN3264TEST(L"x86",L"x64"),
		gOSVer.dwMajorVersion, gOSVer.dwMinorVersion, gOSVer.dwBuildNumber, gOSVer.dwPlatformId, gOSVer.szCSDVersion);
	WriteConsoleW(hOut, szInfo, lstrlen(szInfo), &nTmp, NULL);

	msprintf(szInfo, countof(szInfo), L"SM_IMMENABLED=%u, SM_DBCSENABLED=%u, ACP=%u, OEMCP=%u\r\n",
		GetSystemMetrics(SM_IMMENABLED), GetSystemMetrics(SM_DBCSENABLED), GetACP(), GetOEMCP());
	WriteConsoleW(hOut, szInfo, lstrlen(szInfo), &nTmp, NULL);

	msprintf(szInfo, countof(szInfo), L"ConHWND=0x%08X, Class=\"", LODWORD(ghConWnd));
	GetClassName(ghConWnd, szInfo+lstrlen(szInfo), 255);
	lstrcpyn(szInfo+lstrlen(szInfo), L"\"\r\n", 4);
	WriteConsoleW(hOut, szInfo, lstrlen(szInfo), &nTmp, NULL);

	struct FONT_INFOEX
	{
		ULONG cbSize;
		DWORD nFont;
		COORD dwFontSize;
		UINT  FontFamily;
		UINT  FontWeight;
		WCHAR FaceName[LF_FACESIZE];
	};
	typedef BOOL (WINAPI* GetCurrentConsoleFontEx_t)(HANDLE hConsoleOutput, BOOL bMaximumWindow, FONT_INFOEX* lpConsoleCurrentFontEx);
	HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
	GetCurrentConsoleFontEx_t _GetCurrentConsoleFontEx = (GetCurrentConsoleFontEx_t)(hKernel ? GetProcAddress(hKernel,"GetCurrentConsoleFontEx") : NULL);
	if (!_GetCurrentConsoleFontEx)
	{
		lstrcpyn(szInfo, L"Console font info: Not available\r\n", countof(szInfo));
	}
	else
	{
		FONT_INFOEX info = {sizeof(info)};
		if (!_GetCurrentConsoleFontEx(hOut, FALSE, &info))
		{
			msprintf(szInfo, countof(szInfo), L"Console font info: Failed, code=%u\r\n", GetLastError());
		}
		else
		{
			info.FaceName[LF_FACESIZE-1] = 0;
			msprintf(szInfo, countof(szInfo), L"Console font info: %u, {%ux%u}, %u, %u, \"%s\"\r\n",
				info.nFont, info.dwFontSize.X, info.dwFontSize.Y, info.FontFamily, info.FontWeight,
				info.FaceName);
		}
	}
	WriteConsoleW(hOut, szInfo, lstrlen(szInfo), &nTmp, NULL);

	DWORD nInMode = 0, nOutMode = 0, nErrMode = 0;
	GetConsoleMode(hIn, &nInMode);
	GetConsoleMode(hOut, &nOutMode);
	GetConsoleMode(hErr, &nErrMode);
	msprintf(szInfo, countof(szInfo), L"Handles: In=x%X (Mode=x%X) Out=x%X (x%X) Err=x%X (x%X)\r\n",
		LODWORD(hIn), nInMode, LODWORD(hOut), nOutMode, LODWORD(hErr), nErrMode);
	WriteConsoleW(hOut, szInfo, lstrlen(szInfo), &nTmp, NULL);


	if (GetConsoleScreenBufferInfo(hOut, &csbi))
		msprintf(szInfo, countof(szInfo), L"Buffer={%i,%i} Window={%i,%i}-{%i,%i} MaxSize={%i,%i}\r\n",
			csbi.dwSize.X, csbi.dwSize.Y,
			csbi.srWindow.Left, csbi.srWindow.Top, csbi.srWindow.Right, csbi.srWindow.Bottom,
			csbi.dwMaximumWindowSize.X, csbi.dwMaximumWindowSize.Y);
	else
		msprintf(szInfo, countof(szInfo), L"GetConsoleScreenBufferInfo failed, code=%u\r\n",
			GetLastError());
	WriteConsoleW(hOut, szInfo, lstrlen(szInfo), &nTmp, NULL);

	if (GetConsoleCursorInfo(hOut, &ci))
		msprintf(szInfo, countof(szInfo), L"Cursor: Pos={%i,%i} Size=%i%% %s\r\n",
			csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y, ci.dwSize, ci.bVisible ? L"Visible" : L"Hidden");
	else
		msprintf(szInfo, countof(szInfo), L"GetConsoleCursorInfo failed, code=%u\r\n",
			GetLastError());
	WriteConsoleW(hOut, szInfo, lstrlen(szInfo), &nTmp, NULL);

	DWORD nCP = GetConsoleCP();
	DWORD nOutCP = GetConsoleOutputCP();
	CPINFOEX cpinfo = {};

	msprintf(szInfo, countof(szInfo), L"ConsoleCP=%u, ConsoleOutputCP=%u\r\n", nCP, nOutCP);
	WriteConsoleW(hOut, szInfo, lstrlen(szInfo), &nTmp, NULL);

	for (UINT i = 0; i <= 1; i++)
	{
		if (i && (nOutCP == nCP))
			break;

		if (!GetCPInfoEx(i ? nOutCP : nCP, 0, &cpinfo))
		{
			msprintf(szInfo, countof(szInfo), L"GetCPInfoEx(%u) failed, code=%u\r\n", nCP, GetLastError());
		}
		else
		{
			msprintf(szInfo, countof(szInfo),
				L"CP%u: Max=%u "
				L"Def=x%02X,x%02X UDef=x%X\r\n"
				L"  Lead=x%02X,x%02X,x%02X,x%02X,x%02X,x%02X,x%02X,x%02X,x%02X,x%02X,x%02X,x%02X\r\n"
				L"  Name=\"%s\"\r\n",
				cpinfo.CodePage, cpinfo.MaxCharSize,
				cpinfo.DefaultChar[0], cpinfo.DefaultChar[1], cpinfo.UnicodeDefaultChar,
				cpinfo.LeadByte[0], cpinfo.LeadByte[1], cpinfo.LeadByte[2], cpinfo.LeadByte[3], cpinfo.LeadByte[4], cpinfo.LeadByte[5],
				cpinfo.LeadByte[6], cpinfo.LeadByte[7], cpinfo.LeadByte[8], cpinfo.LeadByte[9], cpinfo.LeadByte[10], cpinfo.LeadByte[11],
				cpinfo.CodePageName);
		}
		WriteConsoleW(hOut, szInfo, lstrlen(szInfo), &nTmp, NULL);
	}
}

// Returns:
// * CERR_UNICODE_CHK_OKAY(142), if RealConsole supports
//   unicode characters output.
// * CERR_UNICODE_CHK_FAILED(141), if RealConsole CAN'T
//   output/store unicode characters.
// This function is called by: ConEmuC.exe /CHECKUNICODE
int CheckUnicodeFont()
{
	// Print version and console information first
	PrintConsoleInfo();

	int iRc = CERR_UNICODE_CHK_FAILED;

	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);

	wchar_t szText[80] = UnicodeTestString;
	wchar_t szColor[32] = ColorTestString;
	CHAR_INFO cWrite[80];
	CHAR_INFO cRead[80] = {};
	WORD aWrite[80], aRead[80] = {};
	wchar_t sAttrWrite[80] = {}, sAttrRead[80] = {}, sAttrBlock[80] = {};
	wchar_t szInfo[1024]; DWORD nTmp;
	wchar_t szCheck[80] = L"", szBlock[80] = L"";
	BOOL bInfo = FALSE, bWrite = FALSE, bRead = FALSE, bCheck = FALSE;
	DWORD nLen = lstrlen(szText), nWrite = 0, nRead = 0, nErr = 0;
	WORD nDefColor = 7;
	DWORD nColorLen = lstrlen(szColor);
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	CONSOLE_CURSOR_INFO ci = {};
	_ASSERTE(nLen<=35); // ниже на 2 буфер множится

	const WORD N = 0x07, H = N|COMMON_LVB_REVERSE_VIDEO;
	CHAR_INFO cHighlight[] = {{{'N'},N},{{'o'},N},{{'r'},N},{{'m'},N},{{'a'},N},{{'l'},N},
		{{' '},N},
		{{'R'},H},{{'e'},H},{{'v'},H},{{'e'},H},{{'r'},H},{{'s'},H},{{'e'},H}};

	if (GetConsoleScreenBufferInfo(hOut, &csbi))
		nDefColor = csbi.wAttributes;

	// Simlify checking of ConEmu's "colorization"
	WriteConsoleW(hOut, L"\r\n", 2, &nTmp, NULL);
	WORD nColor = 7;
	for (DWORD n = 0; n < nColorLen; n++)
	{
		SetConsoleTextAttribute(hOut, nColor);
		WriteConsoleW(hOut, szColor+n, 1, &nTmp, NULL);
		nColor++; if (nColor == 16) nColor = 7;
	}
	WriteConsoleW(hOut, L"\r\n", 2, &nTmp, NULL);

	// Test of "Reverse video"
	GetConsoleScreenBufferInfo(hOut, &csbi);
	COORD crHiSize = {countof(cHighlight), 1}, cr0 = {};
	SMALL_RECT srHiPos = {0, csbi.dwCursorPosition.Y, crHiSize.X-1, csbi.dwCursorPosition.Y};
	WriteConsoleOutputW(hOut, cHighlight, crHiSize, cr0, &srHiPos);
	WriteConsoleW(hOut, L"\r\n", 2, &nTmp, NULL);

	WriteConsoleW(hOut, L"\r\nCheck ", 8, &nTmp, NULL);


	if ((bInfo = GetConsoleScreenBufferInfo(hOut, &csbi)) != FALSE)
	{
		for (DWORD i = 0; i < nLen; i++)
		{
			cWrite[i].Char.UnicodeChar = szText[i];
			aWrite[i] = 10 + (i % 6);
			cWrite[i].Attributes = aWrite[i];
			msprintf(sAttrWrite+i, 2, L"%X", aWrite[i]);
		}

		COORD cr0 = {0,0};
		if ((bWrite = WriteConsoleOutputCharacterW(hOut, szText, nLen, csbi.dwCursorPosition, &nWrite)) != FALSE)
		{
			if (((bRead = ReadConsoleOutputCharacterW(hOut, szCheck, nLen*2, csbi.dwCursorPosition, &nRead)) != FALSE)
				/*&& (nRead == nLen)*/)
			{
				bCheck = (memcmp(szText, szCheck, nLen*sizeof(szText[0])) == 0);
				if (bCheck)
				{
					iRc = CERR_UNICODE_CHK_OKAY;
				}
			}

			if (ReadConsoleOutputAttribute(hOut, aRead, nLen, csbi.dwCursorPosition, &nTmp))
			{
				for (UINT i = 0; i < nTmp; i++)
				{
					msprintf(sAttrRead+i, 2, L"%X", aRead[i] & 0xF);
				}
			}

			COORD crBufSize = {(SHORT)sizeof(cRead),1};
			SMALL_RECT rcRead = {csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y, csbi.dwCursorPosition.X+(SHORT)nLen*2, csbi.dwCursorPosition.Y};
			if (ReadConsoleOutputW(hOut, cRead, crBufSize, cr0, &rcRead))
			{
				for (UINT i = 0; i < nTmp; i++)
				{
					szBlock[i] = cRead[i].Char.UnicodeChar;
					msprintf(sAttrBlock+i, 2, L"%X", cRead[i].Attributes & 0xF);
				}
			}
		}
	}

	//if (!bRead || !bCheck)
	{
		nErr = GetLastError();

		msprintf(szInfo, countof(szInfo),
			L"\r\n" // чтобы не затереть сам текст
			L"Text: %s\r\n"
			L"Read: %s\r\n"
			L"Blck: %s\r\n"
			L"Attr: %s\r\n"
			L"Read: %s\r\n"
			L"Blck: %s\r\n"
			L"Info: %u,%u,%u,%u,%u,%u\r\n"
			L"\r\n%s\r\n",
			szText, szCheck, szBlock, sAttrWrite, sAttrRead, sAttrBlock,
			nErr, bInfo, bWrite, nWrite, bRead, nRead,
			bCheck ? L"Unicode check succeeded" :
			bRead ? L"Unicode check FAILED!" : L"Unicode is not supported in this console!"
			);
		//MessageBoxW(NULL, szInfo, szTitle, MB_ICONEXCLAMATION|MB_ICONERROR);
	}
	//else
	//{
	//	lstrcpyn(szInfo, L"\r\nUnicode check succeeded\r\n", countof(szInfo));
	//}

	//WriteConsoleW(hOut, L"\r\n", 2, &nTmp, NULL);
	//WriteConsoleW(hOut, szCheck, nRead, &nTmp, NULL);

	//LPCWSTR pszText = bCheck ? L"\r\nUnicode check succeeded\r\n" : L"\r\nUnicode check FAILED!\r\n";
	WriteConsoleW(hOut, szInfo, lstrlen(szInfo), &nWrite, NULL);

	return iRc;
}

// Unit test for internal purpose, it utilizes CpCvt
// to convert series of UTF-8 characters into unicode
// This function is called by: ConEmuC.exe /TESTUNICODE
int TestUnicodeCvt()
{
	wchar_t szInfo[140]; DWORD nWrite;
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	// Some translations check
	{
		char chUtf8[] =
			"## "
			"\x41\xF0\x9D\x94\xB8\x41" // 'A', double-struck capital A as surrogate pair, 'A'
			" # "
			"\xE9\xE9\xE9"             // Invalid unicode translations
			" # "
			"\xE6\x96\x87\xE6\x9C\xAC" // Chinese '文' and '本'
			"\xEF\xBC\x8E"             // Fullwidth Full Stop
			" ##\r\n"
			;
		CpCvt cvt = {}; wchar_t wc; char* pch = chUtf8;
		cvt.SetCP(65001);

		msprintf(szInfo, countof(szInfo),
			L"CVT test: CP=%u Max=%u Def=%c: ",
			cvt.CP.CodePage, cvt.CP.MaxCharSize, cvt.CP.UnicodeDefaultChar
			);
		WriteConsoleW(hOut, szInfo, lstrlen(szInfo), &nWrite, NULL);

		while (*pch)
		{
			switch (cvt.Convert(*(pch++), wc))
			{
			case ccr_OK:
			case ccr_BadUnicode:
				WriteConsoleW(hOut, &wc, 1, &nWrite, NULL);
				break;
			case ccr_Surrogate:
			case ccr_BadTail:
			case ccr_DoubleBad:
				WriteConsoleW(hOut, &wc, 1, &nWrite, NULL);
				cvt.GetTail(wc);
				WriteConsoleW(hOut, &wc, 1, &nWrite, NULL);
				break;
			}
		}
	}

	return 0;
}
