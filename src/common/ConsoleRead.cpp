
/*
Copyright (c) 2009-2012 Maximus5
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
#include "../common/common.hpp"
#include "ConsoleRead.h"

//#define DUMP_TEST_READS
#undef DUMP_TEST_READS

LPCSTR GetCpInfoLeads(DWORD nCP, UINT* pnMaxCharSize)
{
	LPCSTR pszLeads = NULL;
	static CPINFOEX cpinfo = {};
	static char szLeads[256] = {};

	if (cpinfo.CodePage != nCP)
	{
		if (!GetCPInfoEx(nCP, 0, &cpinfo) || (cpinfo.MaxCharSize <= 1))
		{
			ZeroStruct(szLeads);
		}
		else
		{
			int c = 0;
			_ASSERTE(countof(cpinfo.LeadByte)>=10);
			for (int i = 0; (i < 5) && cpinfo.LeadByte[i*2]; i++)
			{
				BYTE c1 = cpinfo.LeadByte[i*2];
				BYTE c2 = cpinfo.LeadByte[i*2+1];
				for (BYTE j = c1; (j <= c2) && (c < 254); j++, c++)
				{
					szLeads[c] = j;
				}
			}
			_ASSERTE(c && c <= 255);
			szLeads[c] = 0;
			pszLeads = szLeads;
		}
	}
	else if (cpinfo.MaxCharSize > 1)
	{
		pszLeads = szLeads;
	}

	if (pnMaxCharSize)
		*pnMaxCharSize = cpinfo.MaxCharSize;

	return pszLeads;
}


// Консоль любит глючить, при попытках запроса более определенного количества ячеек.
// MAX_CONREAD_SIZE подобрано экспериментально
BOOL ReadConsoleOutputEx(HANDLE hOut, CHAR_INFO *pData, COORD bufSize, SMALL_RECT rgn)
{
	BOOL lbRc = FALSE;

	DWORD nTick1 = GetTickCount(), nTick2 = 0, nTick3 = 0, nTick4 = 0, nTick5 = 0;

	static bool bDBCS = false, bDBCS_Checked = false;
	if (!bDBCS_Checked)
	{
		bDBCS = (GetSystemMetrics(SM_DBCSENABLED) != 0);
		bDBCS_Checked = true;
	}

	bool   bDBCS_CP = bDBCS;
	LPCSTR szLeads = NULL;
	UINT   MaxCharSize = 0;
	DWORD  nCP, nCP1, nMode;

	if (bDBCS)
	{
		nCP = GetConsoleOutputCP();
		nCP1 = GetConsoleCP();
		GetConsoleMode(hOut, &nMode);

		szLeads = GetCpInfoLeads(nCP, &MaxCharSize);
		if (!szLeads || !*szLeads || MaxCharSize < 2)
		{
			bDBCS_CP = false;
		}
	}

	size_t nBufWidth = bufSize.X;
	int nWidth = (rgn.Right - rgn.Left + 1);
	int nHeight = (rgn.Bottom - rgn.Top + 1);
	int nCurSize = nWidth * nHeight;

	_ASSERTE(bufSize.X >= nWidth);
	_ASSERTE(bufSize.Y >= nHeight);
	_ASSERTE(rgn.Top>=0 && rgn.Bottom>=rgn.Top);
	_ASSERTE(rgn.Left>=0 && rgn.Right>=rgn.Left);

	//MSectionLock RCS;
	//if (gpSrv->pReqSizeSection && !RCS.Lock(gpSrv->pReqSizeSection, TRUE, 30000))
	//{
	//	_ASSERTE(FALSE);
	//	SetLastError(ERROR_INVALID_PARAMETER);
	//	return FALSE;

	//}

	COORD bufCoord = {0,0};
	DWORD dwErrCode = 0;
	CONSOLE_SCREEN_BUFFER_INFO sbi_tmp = {}; BOOL bSbiTmp = (BOOL)-1;

	nTick2 = GetTickCount();

	if (!bDBCS_CP && (nCurSize <= MAX_CONREAD_SIZE))
	{
		if (ReadConsoleOutputW(hOut, pData, bufSize, bufCoord, &rgn))
			lbRc = TRUE;
		nTick3 = GetTickCount();
	}

	if (!lbRc)
	{
		// Придется читать построчно
		
		// Теоретически - можно и блоками, но оверхед очень маленький, а велик
		// шанс обломаться, если консоль "глючит". Поэтому построчно...

		//bufSize.X = TextWidth;
		bufSize.Y = 1;
		bufCoord.X = 0; bufCoord.Y = 0;
		//rgn = gpSrv->sbi.srWindow;

		int Y1 = rgn.Top;
		int Y2 = rgn.Bottom;

		CHAR_INFO* pLine = pData;
		if (!bDBCS_CP)
		{
			for (int y = Y1; y <= Y2; y++, rgn.Top++, pLine+=nBufWidth)
			{
				nTick3 = GetTickCount();
				rgn.Bottom = rgn.Top;

				#ifdef DUMP_TEST_READS
				bSbiTmp = GetConsoleScreenBufferInfo(hOut, &sbi_tmp);
				#endif

				lbRc = ReadConsoleOutputW(hOut, pLine, bufSize, bufCoord, &rgn);

				#ifdef DUMP_TEST_READS
				UNREFERENCED_PARAMETER(sbi_tmp.dwSize.Y);
				#endif

				if (!lbRc)
				{
					dwErrCode = GetLastError();
					_ASSERTE(FALSE && "ReadConsoleOutputW failed in MyReadConsoleOutput");
					break;
				}
				nTick4 = GetTickCount();
			}
		}
		else
		{
			DWORD nAttrsMax = bufSize.X;
			DWORD nCharsMax = nAttrsMax/* *4 */; // -- максимум там вроде на некоторых CP - 4 байта
			wchar_t* pszChars = (wchar_t*)malloc(nCharsMax*sizeof(*pszChars));
			char* pszCharsA = (char*)malloc(nCharsMax*sizeof(*pszCharsA));
			WORD* pnAttrs = (WORD*)malloc(bufSize.X*sizeof(*pnAttrs));
			if (pszChars && pszCharsA && pnAttrs)
			{
				COORD crRead = {rgn.Left,Y1};
				DWORD nChars, nAttrs, nCharsA;
				CHAR_INFO* pLine = pData;
				for (; crRead.Y <= Y2; crRead.Y++, pLine+=nBufWidth)
				{
					nTick3 = GetTickCount();
					rgn.Bottom = rgn.Top;

					nChars = nCharsA = nAttrs = 0;
					BOOL lbRcTxt = ReadConsoleOutputCharacterA(hOut, pszCharsA, nCharsMax, crRead, &nCharsA);
					dwErrCode = GetLastError();
					if (!lbRcTxt || !nCharsA)
					{
						nCharsA = 0;
						lbRcTxt = ReadConsoleOutputCharacterW(hOut, pszChars, nCharsMax, crRead, &nChars);
						dwErrCode = GetLastError();
					}
					BOOL lbRcAtr = ReadConsoleOutputAttribute(hOut, pnAttrs, bufSize.X, crRead, &nAttrs);
					dwErrCode = GetLastError();
					
					lbRc = lbRcTxt && lbRcAtr;

					if (!lbRc)
					{
						dwErrCode = GetLastError();
						_ASSERTE(FALSE && "ReadConsoleOutputAttribute failed in MyReadConsoleOutput");

						CHAR_INFO* p = pLine;
						for (size_t i = 0; i < nAttrsMax; ++i, ++p)
						{
							p->Attributes = 4; // red on black
							p->Char.UnicodeChar = 0xFFFE; // not a character
						}

						break;
					}
					else
					{
						if (nCharsA)
						{
							nChars = MultiByteToWideChar(nCP, 0, pszCharsA, nCharsA, pszChars, nCharsMax);
						}
						CHAR_INFO* p = pLine;
						wchar_t* psz = pszChars;
						WORD* pn = pnAttrs;
						//int i = nAttrsMax;
						//while ((i--) > 0)
						//{
						//	p->Attributes = *(pn++);
						//	p->Char.UnicodeChar = *(psz++);
						//	p++;
						//}
						size_t x1 = min(nChars,nAttrsMax);
						size_t x2 = nAttrsMax;
						for (size_t i = 0; i < x1; ++i, ++p)
						{
							WARNING("Переделать!");
							p->Attributes = *pn & (~(COMMON_LVB_LEADING_BYTE|COMMON_LVB_TRAILING_BYTE));
							p->Char.UnicodeChar = *psz;

							WARNING("Некорректно! pn может указывать на начало блока DBCS/QBCS");
							pn++; // += MaxCharSize;
							psz++;
						}
						WORD nLastAttr = pnAttrs[max(0,(int)nAttrs-1)];
						for (size_t i = x1; i < x2; ++i, ++p)
						{
							p->Attributes = nLastAttr;
							p->Char.UnicodeChar = L' ';
						}
					}
					nTick4 = GetTickCount();
				}
			}
			SafeFree(pszChars);
			SafeFree(pszCharsA);
			SafeFree(pnAttrs);
		}

		nTick5 = GetTickCount();
	}

	UNREFERENCED_PARAMETER(nTick1);
	UNREFERENCED_PARAMETER(nTick2);
	UNREFERENCED_PARAMETER(nTick3);
	UNREFERENCED_PARAMETER(nTick4);
	UNREFERENCED_PARAMETER(nTick5);
	UNREFERENCED_PARAMETER(sbi_tmp.dwSize.Y);
	UNREFERENCED_PARAMETER(bSbiTmp);
	return lbRc;
}
