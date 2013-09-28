
/*
Copyright (c) 2013 Maximus5
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

class CHtmlCopy
{
protected:
	struct txt {
		INT_PTR nLen;
		char* psz;
	};
	MArray<txt> m_Items; // UTF-8
	size_t mn_AllItemsLen, mn_TextStart, mn_TextEnd;
	COLORREF mcr_Fore, mcr_Back;
	bool mb_Finalized, mb_ParOpened;
	wchar_t szTemp[2000], szBack[64], szFore[64], szBold[20], szItalic[20], szId[8];
	char szUTF8[6001]; // (szTemp*3 + \0)
protected:
	LPCWSTR FormatColor(COLORREF clr, wchar_t* rsBuf)
	{
		_wsprintf(rsBuf, SKIPLEN(8) L"#%02X%02X%02X",
			(UINT)(clr & 0xFF), (UINT)((clr & 0xFF00)>>8), (UINT)((clr & 0xFF0000)>>16));
		return rsBuf;
	};
public:
	CHtmlCopy()
	{
		mcr_Fore = 7; mcr_Back = 0; mn_AllItemsLen = 0; mb_Finalized = mb_ParOpened = false;
		mn_TextStart = 0; mn_TextEnd = 0;
	}

	void Init(LPCWSTR asBuild, LPCWSTR asFont, COLORREF crFore, COLORREF crBack)
	{
		LPCWSTR pszHtml = L"<!DOCTYPE>\r\n"
			L"<HTML>\r\n"
			L"<HEAD>\r\n"
			L"<META http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">\r\n"
			L"<META name=\"GENERATOR\" content=\"ConEmu %s[%u]\">\n"
			L"<STYLE>\r\n"
			L"P { margin:0 }\r\n"
			L"</STYLE>\r\n"
			L"</HEAD>\r\n"
			L"<BODY>\r\n";
		_wsprintf(szTemp, SKIPLEN(countof(szTemp)) pszHtml, asBuild, WIN3264TEST(32,64));
		RawAdd(szTemp, _tcslen(szTemp));

		mn_TextStart = mn_AllItemsLen; mn_TextEnd = 0;
		_wsprintf(szTemp, SKIPLEN(countof(szTemp))
			L"<DIV class=\"%s\" style=\"font-family:%s;background-color: %s; color: %s\">\r\n",
			asBuild, asFont, FormatColor(crBack, szBack), FormatColor(crFore, szFore));
		RawAdd(szTemp, _tcslen(szTemp));
	};

	~CHtmlCopy()
	{
		for (int i = 0; i < m_Items.size(); i++)
			free(m_Items[i].psz);
		m_Items.clear();
	};

	void ParBegin()
	{
		LPCWSTR pszPar = mb_ParOpened ? L"</p>\r\n<p>" : L"<p>";
		RawAdd(pszPar, _tcslen(pszPar));
		mb_ParOpened = true;
	}
	void ParEnd()
	{
		if (!mb_ParOpened)
			return;
		mb_ParOpened = false;
		LPCWSTR pszPar = L"</p>\r\n";
		RawAdd(pszPar, _tcslen(pszPar));
	}

	void TextAdd(LPCWSTR asText, INT_PTR cchLen, COLORREF crFore, COLORREF crBack, bool Bold = false, bool Italic = false)
	{
		bool bSpan = (mcr_Fore != crFore) || (mcr_Back != crBack);
		// Open (special colors, fonts, outline?)
		if (bSpan)
		{
			if (mcr_Fore != crFore)
				_wsprintf(szFore, SKIPLEN(countof(szFore)) L"color: %s; ", FormatColor(crFore, szId));
			else
				szFore[0] = 0;

			if (mcr_Back != crBack)
				_wsprintf(szBack, SKIPLEN(countof(szBack)) L"background-color: %s; ", FormatColor(crBack, szId));
			else
				szBack[0] = 0;

			if (Bold)
				wcscpy_c(szBold, L"font-weight: bold; ");
			else
				szBold[0] = 0;

			if (Italic)
				wcscpy_c(szItalic, L"font-style: italic; ");
			else
				szItalic[0] = 0;

			_wsprintf(szTemp, SKIPLEN(countof(szTemp)) L"<span style=\"%s%s%s%s\">", szFore, szBack, szBold, szItalic);
			RawAdd(szTemp, _tcslen(szTemp));
		}

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
		if (bSpan)
		{
			RawAdd(L"</span>", 7);
		}
	}

	void LineAdd(LPCWSTR asText, const WORD* apAttr, const COLORREF* pPal, INT_PTR nLen)
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
				TextAdd(asText, ps - asText, pPal[a & 0xF], pPal[(a & 0xF0)>>4]);
				asText = ps; apAttr = pa;
				a = *apAttr;
			}
		}
		ParEnd();
	};

	void LineAdd(LPCWSTR asText, const CharAttr* apAttr, INT_PTR nLen)
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
				TextAdd(asText, ps - asText, a.crForeColor, a.crBackColor, (a.nFontIndex&1)==1, (a.nFontIndex&2)==2);
				asText = ps; apAttr = pa;
				a = *apAttr;
			}
		}
		ParEnd();
	};

	void RawAdd(LPCWSTR asText, INT_PTR cchLen)
	{
		//TODO("Optimization?");
		int nUtfLen = WideCharToMultiByte(CP_UTF8, 0, asText, cchLen, szUTF8, countof(szUTF8)-1, 0, 0);
		if (nUtfLen > 0)
		{
			txt t = {nUtfLen};
			t.psz = (char*)malloc(nUtfLen+1);
			memmove(t.psz, szUTF8, nUtfLen);
			t.psz[nUtfLen] = 0;
			m_Items.push_back(t);
			mn_AllItemsLen += nUtfLen;
		}
	};

	HGLOBAL CreateResult(bool abAddFormatHdr)
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

		char szHdr[200];
		INT_PTR nHdrLen = 0;
		if (abAddFormatHdr)
		{
			// First, we need to calculate header len
			LPCSTR pszHdrFmt = "Version:1.0\r\nStartHTML:%09u\r\nEndHTML:%09u\r\nStartFragment:%09u\r\nEndFragment:%09u\r\nStartSelection:%09u\r\nEndSelection:%09u\r\n";
			_wsprintfA(szHdr, SKIPLEN(countof(szHdr)) pszHdrFmt, 0,0,0,0,0,0);
			nHdrLen = strlen(szHdr);
			// Calculate positions
			_wsprintfA(szHdr, SKIPLEN(countof(szHdr)) pszHdrFmt,
				(UINT)(nHdrLen), (UINT)(nHdrLen+mn_AllItemsLen),
				(UINT)(nHdrLen+mn_TextStart), (UINT)(nHdrLen+mn_TextEnd),
				(UINT)(nHdrLen+mn_TextStart), (UINT)(nHdrLen+mn_TextEnd));
			_ASSERTE(nHdrLen == strlen(szHdr));
		}

		INT_PTR nCharCount = (nHdrLen/*Header*/ + mn_AllItemsLen + 3);
		HGLOBAL hUTF = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, nCharCount+1);

		if (!hUTF)
		{
			Assert(hUTF != NULL);
		}
		else
		{
			char *pch = (char*)GlobalLock(hUTF);
			if (!pch)
			{
				Assert(pch != NULL);
				GlobalFree(hUTF);
				hUTF = NULL;
			}
			else
			{
				if (abAddFormatHdr)
				{
					memmove(pch, szHdr, nHdrLen);
					pch += nHdrLen;
				}
				
				for (int i = 0; i < m_Items.size(); i++)
				{
					txt t = m_Items[i];
					memmove(pch, t.psz, t.nLen);
					pch += t.nLen;
				}

				*pch = 0;

				GlobalUnlock(hUTF);
			}

		}

		return hUTF;
	}
};
