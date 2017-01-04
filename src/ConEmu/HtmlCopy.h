
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

/*
Sample

Version:0.9
StartHTML:0000000???
EndHTML:0000000???
StartFragment:0000000???
EndFragment:0000000???
SourceURL:https://e.mail.ru/compose/...
<html>
<body>
<!--StartFragment-->
<span style="color: rgb(255, 255, 255); font-family: 'comic sans ms', sans-serif; font-size: 14px; font-style: normal; font-variant: normal; font-weight: normal; letter-spacing: normal; line-height: normal; orphans: auto; text-align: start; text-indent: 0px; text-transform: none; white-space: normal; widows: auto; word-spacing: 0px; -webkit-text-stroke-width: 0px; background-color: rgb(23, 59, 211); display: inline !important; float: none;">
Text
</span>
<!--EndFragment-->
</body>
</html>

*/

#pragma once

class CFormatCopy
{
protected:
	bool bCopyRawCodes;
	struct txt {
		INT_PTR nLen;
		union
		{
			char* pasz;    // UTF-8: Used if (bCopyRawCodes == false)
			wchar_t* pwsz; // Used if (bCopyRawCodes == true)
			void* ptr;
		};
	};
	MArray<txt> m_Items;
	size_t mn_AllItemsLen;
	char szUTF8[6001]; // (szTemp*3 + \0)
public:
	CFormatCopy()
	{
		mn_AllItemsLen = 0;
		bCopyRawCodes = false;
	};
	virtual ~CFormatCopy()
	{
		for (int i = 0; i < m_Items.size(); i++)
			free(m_Items[i].ptr);
		m_Items.clear();
	};
public:
	virtual HGLOBAL CreateResult() = 0;

	virtual void LineAdd(LPCWSTR asText, const WORD* apAttr, const COLORREF* pPal, INT_PTR nLen)
	{
		WORD a = *apAttr;
		LPCWSTR ps = asText;
		const WORD* pa = apAttr;
		ParBegin();
		while ((nLen--) > 0)
		{
			pa++; ps++;
			if (!nLen || (*pa != a))
			{
				TextAdd(asText, ps - asText, pPal[CONFORECOLOR(a)], pPal[CONBACKCOLOR(a)]);
				asText = ps; apAttr = pa;
				a = *apAttr;
			}
		}
		ParEnd();
	};

	virtual void LineAdd(LPCWSTR asText, const CharAttr* apAttr, INT_PTR nLen)
	{
		CharAttr a = *apAttr;
		LPCWSTR ps = asText;
		const CharAttr* pa = apAttr;
		ParBegin();
		while ((nLen--) > 0)
		{
			pa++; ps++;
			if (!nLen || (pa->All != a.All))
			{
				TextAdd(asText, ps - asText, a.crForeColor, a.crBackColor, (a.nFontIndex&1)==1, (a.nFontIndex&2)==2, (a.nFontIndex&4)==4);
				asText = ps; apAttr = pa;
				a = *apAttr;
			}
		}
		ParEnd();
	};

protected:
	virtual void ParBegin() = 0;
	virtual void ParEnd() = 0;
	virtual void TextAdd(LPCWSTR asText, INT_PTR cchLen, COLORREF crFore, COLORREF crBack, bool Bold = false, bool Italic = false, bool Underline = false) = 0;
protected:
	void RawAdd(LPCWSTR asText, INT_PTR cchLen)
	{
		if (bCopyRawCodes)
		{
			// We store data "as is" to copy RAW sequences/html to clipboard
			txt t = {cchLen};
			t.pwsz = (wchar_t*)malloc((cchLen+1)*sizeof(wchar_t));
			wmemmove(t.pwsz, asText, cchLen);
			t.pwsz[cchLen] = 0;
			m_Items.push_back(t);
			mn_AllItemsLen += cchLen;
		}
		else
		{
			// We need to convert data to UTF-8 (HTML clipboard format)
			int nUtfLen = WideCharToMultiByte(CP_UTF8, 0, asText, cchLen, szUTF8, countof(szUTF8)-1, 0, 0);
			if (nUtfLen > 0)
			{
				txt t = {nUtfLen};
				t.pasz = (char*)malloc(nUtfLen+1);
				memmove(t.pasz, szUTF8, nUtfLen);
				t.pasz[nUtfLen] = 0;
				m_Items.push_back(t);
				mn_AllItemsLen += nUtfLen;
			}
		}
	};

	HGLOBAL CreateResultInternal(const char* pszHdr = NULL, INT_PTR nHdrLen = 0)
	{
		INT_PTR nCharCount = (nHdrLen/*Header*/ + mn_AllItemsLen + 3);
		HGLOBAL hCopy = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, (nCharCount+1)*(bCopyRawCodes ? sizeof(wchar_t) : sizeof(char)));

		if (!hCopy)
		{
			Assert(hCopy != NULL);
		}
		else if (bCopyRawCodes)
		{
			wchar_t *pch = (wchar_t*)GlobalLock(hCopy);
			if (!pch)
			{
				Assert(pch != NULL);
				GlobalFree(hCopy);
				hCopy = NULL;
			}
			else
			{
				for (int i = 0; i < m_Items.size(); i++)
				{
					txt t = m_Items[i];
					wmemmove(pch, t.pwsz, t.nLen);
					pch += t.nLen;
				}

				*pch = 0;

				GlobalUnlock(hCopy);
			}
		}
		else
		{
			char *pch = (char*)GlobalLock(hCopy);
			if (!pch)
			{
				Assert(pch != NULL);
				GlobalFree(hCopy);
				hCopy = NULL;
			}
			else
			{
				if (pszHdr && (nHdrLen > 0))
				{
					memmove(pch, pszHdr, nHdrLen);
					pch += nHdrLen;
				}

				for (int i = 0; i < m_Items.size(); i++)
				{
					txt t = m_Items[i];
					memmove(pch, t.pasz, t.nLen);
					pch += t.nLen;
				}

				*pch = 0;

				GlobalUnlock(hCopy);
			}
		}

		return hCopy;
	};
};

class CHtmlCopy : public CFormatCopy
{
protected:
	size_t mn_TextStart, mn_TextEnd;
	bool mb_Finalized, mb_ParOpened;
	wchar_t szTemp[2000], szBack[64], szFore[64], szId[8];
	wchar_t szBold[20], szItalic[20], szUnderline[20];
protected:
	LPCWSTR FormatColor(COLORREF clr, wchar_t* rsBuf)
	{
		_wsprintf(rsBuf, SKIPLEN(8) L"#%02X%02X%02X",
			(UINT)(clr & 0xFF), (UINT)((clr & 0xFF00)>>8), (UINT)((clr & 0xFF0000)>>16));
		return rsBuf;
	};
public:
	CHtmlCopy(bool abPlainHtml, LPCWSTR asBuild, LPCWSTR asFont, int anFontHeight, COLORREF crFore, COLORREF crBack)
		: CFormatCopy()
	{
		mb_Finalized = mb_ParOpened = false;
		mn_TextStart = 0; mn_TextEnd = 0;

		bCopyRawCodes = abPlainHtml;

		LPCWSTR pszHtml = L"<!DOCTYPE>\r\n"
			L"<HTML>\r\n"
			L"<HEAD>\r\n"
			L"<META http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">\r\n"
			L"<META name=\"GENERATOR\" content=\"ConEmu %s[%u]\">\n"
			L"</HEAD>\r\n"
			L"<BODY>\r\n";
		_wsprintf(szTemp, SKIPLEN(countof(szTemp)) pszHtml, asBuild, WIN3264TEST(32,64));
		RawAdd(szTemp, _tcslen(szTemp));

		mn_TextStart = mn_AllItemsLen; mn_TextEnd = 0;
		msprintf(szTemp, countof(szTemp),
			L"<DIV class=\"%s\" style=\"font-family: '%s'; font-size: %upx; text-align: start; text-indent: 0px; margin: 0;\">\r\n",
			asBuild, asFont, anFontHeight);
		RawAdd(szTemp, _tcslen(szTemp));
	};

	virtual ~CHtmlCopy()
	{
	};

protected:
	virtual void ParBegin() override
	{
		if (mb_ParOpened)
			ParEnd();
		mb_ParOpened = true;
	};
	virtual void ParEnd() override
	{
		if (!mb_ParOpened)
			return;
		mb_ParOpened = false;
		RawAdd(L"<br>\r\n", 6);
	};

	virtual void TextAdd(LPCWSTR asText, INT_PTR cchLen, COLORREF crFore, COLORREF crBack, bool Bold = false, bool Italic = false, bool Underline = false) override
	{
		// Open (special colors, fonts, outline?)
		_wsprintf(szFore, SKIPLEN(countof(szFore)) L"color: %s; ", FormatColor(crFore, szId));
		_wsprintf(szBack, SKIPLEN(countof(szBack)) L"background-color: %s; ", FormatColor(crBack, szId));
		wcscpy_c(szBold, Bold ? L"font-weight: bold; " : L"");
		wcscpy_c(szItalic, Italic ? L"font-style: italic; " : L"");
		wcscpy_c(szUnderline, Underline ? L"text-decoration: underline; " : L"");
		_wsprintf(szTemp, SKIPLEN(countof(szTemp)) L"<span style=\"%s%s%s%s%s\">", szFore, szBack, szBold, szItalic, szUnderline);
		RawAdd(szTemp, _tcslen(szTemp));

		// Text
		LPCWSTR pszEnd = asText + cchLen;
		wchar_t* pszDst = szTemp;
		wchar_t* pszDstEnd = szTemp + countof(szTemp);
		const INT_PTR minTagLen = 10; // ~~
		bool bSpace = true;
		while (asText < pszEnd)
		{
			if ((pszDst + minTagLen) >= pszDstEnd)
			{
				RawAdd(szTemp, pszDst - szTemp);
				pszDst = szTemp;
			}

			switch (*asText)
			{
				case 0:
				case L' ':
				case 0xA0:
					if (bSpace || (*asText == 0xA0))
					{
						_wcscpy_c(pszDst, minTagLen, L"&nbsp;");
						pszDst += 6;
					}
					else
					{
						*(pszDst++) = L' ';
						bSpace = true;
					}
					break;
				case L'&':
					_wcscpy_c(pszDst, minTagLen, L"&amp;");
					pszDst += 5;
					break;
				case L'"':
					_wcscpy_c(pszDst, minTagLen, L"&quot;");
					pszDst += 6;
					break;
				case L'<':
					_wcscpy_c(pszDst, minTagLen, L"&lt;");
					pszDst += 4;
					break;
				case L'>':
					_wcscpy_c(pszDst, minTagLen, L"&gt;");
					pszDst += 4;
					break;
				default:
					*(pszDst++) = *asText;
					bSpace = false;
			}

			asText++;
		}

		if (pszDst > szTemp)
		{
			RawAdd(szTemp, pszDst - szTemp);
		}

		// Fin
		RawAdd(L"</span>", 7);
	};

public:
	virtual HGLOBAL CreateResult() override
	{
		if (!mb_Finalized)
		{
			if (mb_ParOpened)
				ParEnd();
			mb_Finalized = true;
			RawAdd(L"</DIV>", 6);
			mn_TextEnd = mn_AllItemsLen;

			LPCWSTR pszFin = L"\r\n</BODY>\r\n</HTML>\r\n";
			RawAdd(pszFin, _tcslen(pszFin));
		}

		char szHdr[255];
		INT_PTR nHdrLen = 0;
		if (!bCopyRawCodes)
		{
			// First, we need to calculate header len
			LPCSTR pszURL = CEHOMEPAGE_A;
			LPCSTR pszHdrFmt = "Version:1.0\r\n"
					"StartHTML:%09u\r\n"
					"EndHTML:%09u\r\n"
					"StartFragment:%09u\r\n"
					"EndFragment:%09u\r\n"
					"StartSelection:%09u\r\n"
					"EndSelection:%09u\r\n"
					"SourceURL:%s\r\n";
			_wsprintfA(szHdr, SKIPLEN(countof(szHdr)) pszHdrFmt, 0,0,0,0,0,0, pszURL);
			nHdrLen = strlen(szHdr);
			// Calculate positions
			_wsprintfA(szHdr, SKIPLEN(countof(szHdr)) pszHdrFmt,
				(UINT)(nHdrLen), (UINT)(nHdrLen+mn_AllItemsLen),
				(UINT)(nHdrLen+mn_TextStart), (UINT)(nHdrLen+mn_TextEnd),
				(UINT)(nHdrLen+mn_TextStart), (UINT)(nHdrLen+mn_TextEnd),
				pszURL);
			_ASSERTE(nHdrLen == strlen(szHdr));
		}

		// Just compile all string to one block
		HGLOBAL hCopy = CreateResultInternal(bCopyRawCodes ? NULL : szHdr, nHdrLen);
		return hCopy;
	};
};

class CAnsiCopy : public CFormatCopy
{
protected:
	bool mb_ParOpened;
	wchar_t szSeq[80], szTemp[256];
	bool mb_Palette;
	COLORREF clrPalette[16];
	bool mb_Bold, mb_Italic, mb_Underline;
protected:
	LPCWSTR FormatColor(COLORREF crFore, COLORREF crBack, bool Bold, bool Italic, bool Underline, wchar_t (&rsBuf)[80])
	{
		wchar_t szBk[80], szBIU[16] = L"";
		UINT Ansi[8] = {0, 4, 2, 6, 1, 5, 3, 7};

		// Prepare styles first
		if (Bold != mb_Bold)
		{
			mb_Bold = Bold;
			wcscat_c(szBIU, Bold ? L";1" : L";22");
		}
		if (Italic != mb_Italic)
		{
			mb_Italic = Italic;
			wcscat_c(szBIU, Italic ? L";3" : L";23");
		}
		if (Underline != mb_Underline)
		{
			mb_Underline = Underline;
			wcscat_c(szBIU, Underline ? L";3" : L";24");
		}

		// 0..15 indexed colors
		if (mb_Palette)
		{
			for (int i = 0; i < 8; i++)
			{
				if (clrPalette[i] == crFore)
				{
					_wsprintf(rsBuf, SKIPCOUNT(rsBuf) L"\x1B[%um", 30+Ansi[i]);
					goto do_bk;
				}
			}
			for (int i = 8; i < 16; i++)
			{
				if (clrPalette[i] == crFore)
				{
					_wsprintf(rsBuf, SKIPCOUNT(rsBuf) L"\x1B[%um", 90+Ansi[i-8]);
					goto do_bk;
				}
			}
		}

		//TODO: xterm-256 palette?

		// xterm 24-bit colors
		_wsprintf(rsBuf, SKIPCOUNT(rsBuf) L"\x1B[38;2;%u;%u;%um",
			(UINT)(crFore & 0xFF), (UINT)((crFore & 0xFF00)>>8), (UINT)((crFore & 0xFF0000)>>16));

	do_bk:
		// 0..15 indexed colors
		if (mb_Palette)
		{
			for (int i = 0; i < 8; i++)
			{
				if (clrPalette[i] == crBack)
				{
					_wsprintf(szBk, SKIPCOUNT(szBk) L"\x1B[%um", 40+Ansi[i]);
					goto add_bk;
				}
			}
			for (int i = 8; i < 16; i++)
			{
				if (clrPalette[i] == crBack)
				{
					_wsprintf(szBk, SKIPCOUNT(szBk) L"\x1B[%um", 100+Ansi[i-8]);
					goto add_bk;
				}
			}
		}

		//TODO: xterm-256 palette?

		// xterm 24-bit colors
		_wsprintf(szBk, SKIPCOUNT(szBk) L"\x1B[48;2;%u;%u;%um",
			(UINT)(crBack & 0xFF), (UINT)((crBack & 0xFF00)>>8), (UINT)((crBack & 0xFF0000)>>16));
	add_bk:
		wcscat_c(rsBuf, szBk);
		if (szBIU[0])
		{
			wcscat_c(szBIU, L"m");
			wcscat_c(rsBuf, L"\x1B[");
			wcscat_c(rsBuf, szBIU + 1);
		}
		return rsBuf;
	};
public:
	CAnsiCopy(COLORREF* pclrPalette/*[16]*/, COLORREF crFore, COLORREF crBack)
		: CFormatCopy()
	{
		mb_ParOpened = false;
		mb_Bold = mb_Italic = mb_Underline = false;
		memset(szSeq, 0, sizeof(szSeq));
		memset(szTemp, 0, sizeof(szTemp));

		bCopyRawCodes = true;

		mb_Palette = (pclrPalette != NULL);
		if (mb_Palette)
			memmove(clrPalette, pclrPalette, sizeof(clrPalette));

		#if 0
		// Init format
		FormatColor(crFore, crBack, szSeq);
		// Unset Bold/Italic/Underline
		RawAdd(szTemp, _tcslen(szTemp));
		#endif
	};

	virtual ~CAnsiCopy()
	{
	};

protected:
	virtual void ParBegin() override
	{
		if (mb_ParOpened)
			ParEnd();
		mb_ParOpened = true;
	};
	virtual void ParEnd() override
	{
		if (!mb_ParOpened)
			return;
		mb_ParOpened = false;
		RawAdd(L"\x1B[m\r\n", 5);
	};

	virtual void TextAdd(LPCWSTR asText, INT_PTR cchLen, COLORREF crFore, COLORREF crBack, bool Bold = false, bool Italic = false, bool Underline = false) override
	{
		// Set colors
		wcscpy_c(szTemp, FormatColor(crFore, crBack, Bold, Italic, Underline, szSeq));
		// Set/Unset Bold/Italic/Underline
		RawAdd(szTemp, _tcslen(szTemp));

		// Text
		LPCWSTR pszEnd = asText + cchLen;
		wchar_t* pszDst = szTemp;
		wchar_t* pszDstEnd = szTemp + countof(szTemp);
		const INT_PTR minTagLen = 10; // ~~

		while (asText < pszEnd)
		{
			if ((pszDst + minTagLen) >= pszDstEnd)
			{
				RawAdd(szTemp, pszDst - szTemp);
				pszDst = szTemp;
			}

			switch (*asText)
			{
				case 0:
					*(pszDst++) = L' ';
					break;
				//case L'\\':
				//	_wcscpy_c(pszDst, minTagLen, L"\\\\");
				//	pszDst += 2;
				//	break;
				default:
					*(pszDst++) = *asText;
			}

			asText++;
		}

		if (pszDst > szTemp)
		{
			RawAdd(szTemp, pszDst - szTemp);
		}

		// Fin
	};

public:
	virtual HGLOBAL CreateResult() override
	{
		// Reset coloring
		if (mb_ParOpened)
		{
			wcscpy_c(szTemp, L"\x1B[m");
			RawAdd(szTemp, _tcslen(szTemp));
		}

		// Just compile all string to one block
		HGLOBAL hCopy = CreateResultInternal(NULL, 0);
		return hCopy;
	};
};
