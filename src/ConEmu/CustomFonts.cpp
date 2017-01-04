
/*
Copyright (c) 2012 thecybershadow
Copyright (c) 2012-2017 Maximus5
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
#include "CustomFonts.h"
#include "Font.h"
#include "FontPtr.h"
#include "../common/MArray.h"

#ifdef _DEBUG
	#define DEBUG_BDF_DRAW
	//#undef DEBUG_BDF_DRAW
#else
	#undef DEBUG_BDF_DRAW
#endif


// CustomFontFamily

struct CustomFontFamily::Impl
{
	MArray<CustomFont*> fonts;
};

void CustomFontFamily::AddFont(CustomFont* font)
{
	if (!pImpl)
		pImpl = new Impl();
	pImpl->fonts.push_back(font);
}

CustomFont* CustomFontFamily::GetFont(int iSize, BOOL bBold, BOOL bItalic, BOOL bUnderline)
{
	if (!pImpl)
		return NULL;

	CustomFont* pBestFont = NULL;
	int iBestScore = 1000000000;

	//for (std::vector<CustomFont*>::iterator iter = pImpl->fonts.begin(); iter != pImpl->fonts.end(); ++iter)
	for (INT_PTR iter = 0; iter < pImpl->fonts.size(); ++iter)
	{
		int iScore = 0;	// lower is better
		//CustomFont* pFont = *iter;
		CustomFont* pFont = pImpl->fonts[iter];

		iScore += abs(abs(iSize) - abs(pFont->GetSize()));
		if (bBold != pFont->IsBold())
			iScore += 1000;
		if (bItalic != pFont->IsItalic())
			iScore += 1000;
		if (bUnderline != pFont->IsUnderline())
			iScore += 1000;
		if (!pFont->HasBorders())
			iScore += 10000;
		if (!pFont->HasUnicode())
			iScore += 10000;
		if (iScore < iBestScore)
		{
			pBestFont = pFont;
			iBestScore = iScore;
		}
	}

	return pBestFont;
}

CustomFontFamily::~CustomFontFamily()
{
	if (pImpl)
		delete pImpl;
}

// BDFFont

class BDFFont : public CustomFont
{
private:
	int m_Width, m_Height;
	BOOL m_Bold;

	HDC hDC;
	HBITMAP hBitmap;  // for GDI rendering
	BYTE *bpBPixels; DWORD dwStride;
	bool *bpMPixels;  // for manual rendering
	BOOL m_HasUnicode, m_HasBorders;

	BDFFont()
	{
		m_Bold = m_HasUnicode = m_HasBorders = FALSE;
		m_Width = 0;
		m_Height = 0;
	}

	void CreateBitmap()
	{
		HDC hDDC = GetDC(NULL);
		hDC = CreateCompatibleDC(hDDC);
		ReleaseDC(NULL, hDDC);

		struct MyBitmap
		{
			BITMAPINFO bmi;
			// bmi contains bmiColors[1], which is initialized to {0,0,0}
			// but we need space for `white` color, because (biBitCount == 1)
			RGBQUAD space_for_white;
		};

		MyBitmap b = {};
		b.bmi.bmiHeader.biSize        = sizeof(b.bmi.bmiHeader);
		b.bmi.bmiHeader.biWidth       = m_Width * 256;
		b.bmi.bmiHeader.biHeight      = -m_Height * 256;
		b.bmi.bmiHeader.biPlanes      = 1;
		b.bmi.bmiHeader.biBitCount    = 1;
		b.bmi.bmiHeader.biCompression = BI_RGB;
		//TODO: Alpha channel?
		RGBQUAD white = {0xFF,0xFF,0xFF};
		// biBitCount==1: The bitmap is monochrome, and the bmiColors member
		//                of BITMAPINFO must contain two entries.
		b.bmi.bmiColors[1] = white;
		void* pvBits = NULL;
		hBitmap = CreateDIBSection(hDC, &b.bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
		if (hBitmap)
			SelectObject(hDC, hBitmap);
		bpBPixels = (BYTE*)pvBits;
		dwStride = (m_Width * 256) / 8;

		bpMPixels = new bool[m_Width*256 * m_Height*256](); // TODO?
	}

public:
	virtual ~BDFFont()
	{
		if (hDC)
			DeleteDC(hDC);
		if (hBitmap)
			DeleteObject(hBitmap);
		if (bpMPixels)
			delete[] bpMPixels;
	}

	static size_t NextWord(char*& szLine, char*& szWord, char szTerm = ' ')
	{
		char* pszSpace = strchr(szLine, szTerm);
		char* szNext;
		size_t nLen = 0;
		if (!pszSpace)
		{
			nLen = strlen(szLine);
			pszSpace = szLine + nLen;
			szNext = pszSpace;
		}
		else
		{
			nLen = pszSpace - szLine;
			szNext = pszSpace + 1;
			*pszSpace = 0;
		}
		szWord = szLine;
		szLine = szNext;
		return nLen;
	}

	static size_t NextLine(char*& szLine, char*& szWord)
	{
		size_t nLine = NextWord(szLine, szWord, '\n');
		if (nLine && szWord[nLine-1] == '\r')
			szWord[--nLine] = 0;
		return nLine;
	}

	static char* LoadBuffer(const wchar_t* lpszFilePath, char*& pszBuf, char*& pszFileEnd)
	{
		HANDLE h = CreateFile(lpszFilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, NULL);
		if (h == INVALID_HANDLE_VALUE)
			return NULL;

		LARGE_INTEGER liSize = {};
		if (!GetFileSizeEx(h, &liSize) || liSize.HighPart)
		{
			_ASSERTE("GetFileSizeEx failed" && 0);
			CloseHandle(h);
			return NULL;
		}

		pszBuf = (char*)malloc(liSize.LowPart+1);
		if (!pszBuf)
		{
			_ASSERTE("Buffer allocation failed" && 0);
			CloseHandle(h);
			return NULL;
		}

		DWORD nRead = 0;
		BOOL bRead = ReadFile(h, pszBuf, liSize.LowPart, &nRead, NULL);
		CloseHandle(h);

		if (!bRead || (nRead != liSize.LowPart))
		{
			_ASSERTE("BDF file read failed" && 0);
			free(pszBuf);
			return NULL;
		}
		pszBuf[nRead] = 0;

		pszFileEnd = pszBuf + nRead;
		return pszBuf;
	}

	static BDFFont* Load(const wchar_t* lpszFilePath)
	{
		char* pszBuf, *pszFileEnd;
		char* pszCur = LoadBuffer(lpszFilePath, pszBuf, pszFileEnd);
		if (!pszCur)
			return NULL;

		char* szLine;
		char* szWord;
		char* pszEnd;


		BDFFont* b = new BDFFont;

		int iCharIndex = -1;
		int iXOffset = 0, iYOffset = 0;
		DEBUGTEST(int iCharWidth = 0);
		int iCharHeight = 0;
		int iCharXOffset = 0;
		int iCharYOffset = 0;


		while ((pszCur < pszFileEnd) && *pszCur)
		{
			NextLine(pszCur, szLine);
			NextWord(szLine, szWord);
			//std::istringstream iss(line);
			//std::string word;
			//getline(iss, word, ' ');
			if (lstrcmpA(szWord, "WEIGHT_NAME") == 0)
			{
				//getline(iss, word);
				NextWord(szLine, szWord);
				//std::transform(word.begin(), word.end(), word.begin(), tolower);
				CharLowerBuffA(szWord, lstrlenA(szWord));
				//if (word != "\"medium\"" && word != "\"regular\"" && word != "\"normal\"")
				if (lstrcmpA(szWord, "\"medium\"") && lstrcmpA(szWord, "\"regular\"") && lstrcmpA(szWord, "\"normal\""))
					b->m_Bold = TRUE;
			}
			else
			if (lstrcmpA(szWord, "FONTBOUNDINGBOX") == 0)
			{
				//getline(iss, word, ' '); b->m_Width = atoi(word.c_str());
				//getline(iss, word, ' '); b->m_Height = atoi(word.c_str());
				//getline(iss, word, ' '); iXOffset = atoi(word.c_str());
				//getline(iss, word, ' '); iYOffset = atoi(word.c_str());
				NextWord(szLine, szWord); b->m_Width = strtol(szWord, &pszEnd, 10);
				NextWord(szLine, szWord); b->m_Height = strtol(szWord, &pszEnd, 10);
				NextWord(szLine, szWord); iXOffset = strtol(szWord, &pszEnd, 10);
				NextWord(szLine, szWord); iYOffset = strtol(szWord, &pszEnd, 10);
				b->CreateBitmap();
			}
			else
			if (lstrcmpA(szWord, "ENCODING") == 0)
			{
				//getline(iss, word, ' '); iCharIndex = atoi(word.c_str());
				NextWord(szLine, szWord); iCharIndex = strtol(szWord, &pszEnd, 10);
				if (iCharIndex > 0xFFFF)
					iCharIndex = -1;
				if (iCharIndex > 0xFF)
					b->m_HasUnicode = TRUE;
				if (iCharIndex >= 0x2500 && iCharIndex < 0x25A0)
					b->m_HasBorders = TRUE;
			}
			else
			if (lstrcmpA(szWord, "BBX") == 0)
			{
				NextWord(szLine, szWord); DEBUGTEST(iCharWidth = strtol(szWord, &pszEnd, 10));
				NextWord(szLine, szWord); iCharHeight = strtol(szWord, &pszEnd, 10);
				NextWord(szLine, szWord); iCharXOffset = strtol(szWord, &pszEnd, 10);
				NextWord(szLine, szWord); iCharYOffset = strtol(szWord, &pszEnd, 10);
			}
			else
			if (lstrcmpA(szWord, "BITMAP") == 0)
			{
				if (iCharIndex < 0)
					continue;

				int x = b->m_Width  * (iCharIndex % 256) + iCharXOffset + iXOffset;
				int y = b->m_Height * (iCharIndex / 256) + b->m_Height - iCharYOffset + iYOffset - iCharHeight;

				for (int iRow = 0; iRow < iCharHeight; iRow++)
				{
					int by = y*b->m_Width*256;
					NextLine(pszCur, szLine);
					int i = 0;
					while (*szLine)
					{
						if (!szLine[1])
							break;
						char szHex[3] = {szLine[0], szLine[1]};
						int n = strtoul(szHex, &pszEnd, 16);
						for (int bit=0; bit<8; bit++)
						{
							if (n & (0x80 >> bit))
							{
								int px = x+i*8+bit;
								//SetPixel(b->hDC, px, y, 0xFFFFFF);
								b->bpBPixels[y * b->dwStride + (px / 8)] |= 0x80 >> (px % 8);
								b->bpMPixels[by + px] = true;
							}
						}
						szLine += 2;
						i++;
					}
					y++;
				}
			}
		}

		free(pszBuf);

		return b;
	}

	static bool GetFamilyName(const wchar_t* lpszFilePath, wchar_t (&rsFamilyName)[LF_FACESIZE])
	{
		rsFamilyName[0] = 0;
		char* pszBuf, *pszFileEnd, *szLine, *szWord;
		char* pszCur = LoadBuffer(lpszFilePath, pszBuf, pszFileEnd);
		if (!pszCur)
			return false;

		const char* FAMILY_NAME = "FAMILY_NAME \"";
		int FAMILY_NAME_LEN = lstrlenA(FAMILY_NAME);
		//static const std::string FONT("FONT ");
		const char* FONT = "FONT ";
		int FONT_LEN = lstrlenA(FONT);
		char szFamilyName[LF_FACESIZE] = {};

		while ((pszCur < pszFileEnd) && *pszCur)
		{
			NextLine(pszCur, szLine);

			if (memcmp(szLine, FAMILY_NAME, FAMILY_NAME_LEN) == 0)
			{
				lstrcpynA(szFamilyName, szLine + FAMILY_NAME_LEN, countof(szFamilyName));
				char *psz = strchr(szFamilyName, '"');
				if (psz)
					*psz = 0;
				goto wrap;
			}
			else if (memcmp(szLine, FONT, FONT_LEN) == 0)
			{
				// из строки
				// FONT -Schumacher-Clean-Medium-R-Normal--12-120-75-75-C-60-ISO10646-1
				// нам нужно выкусить "Clean"
				for (int n=0; n<3; n++)
					NextWord(szLine, szWord);

				lstrcpynA(szFamilyName, szWord, countof(szFamilyName));

				// Keep looking for FAMILY_NAME
			}
		}

wrap:
		MultiByteToWideChar(CP_ACP, 0, szFamilyName, -1, rsFamilyName, countof(rsFamilyName));
		return (rsFamilyName[0] != 0);
	}

	// ...

	virtual BOOL IsMonospace()
	{
		return TRUE;
	}

	virtual BOOL HasUnicode()
	{
		return m_HasUnicode;
	}

	virtual BOOL HasBorders()
	{
		return m_HasBorders;
	}

	virtual int GetSize()
	{
		long w, h;
		GetBoundingBox(&w, &h);
		return h;
	}

	virtual BOOL IsBold()
	{
		return m_Bold;
	}

	virtual BOOL IsItalic()
	{
		return FALSE;
	}

	virtual BOOL IsUnderline()
	{
		return FALSE;
	}

	virtual void GetBoundingBox(long *pX, long *pY)
	{
		*pX = m_Width;
		*pY = m_Height;
	}

	// Used if Display has less than 32-bit color (CEDC::Create)
	virtual void TextDraw(HDC hDC, int X, int Y, LPCWSTR lpString, UINT cbCount)
	{
		for (; cbCount; cbCount--, lpString++)
		{
			wchar_t ch = *lpString;
			// If this BitBlt(DSPDxax) fails with colour (160118-custom-bdf\2016-01-18_21-23-12.png)
			// we may switch to MaskBlt below, but MaskBlt is slower (somewhat about 25%)
			// Note!
			//   Our hBitmap MUST be created with palette of two RGB colors {0x000000, 0xFFFFFF}
			//   to achieve proper trinaty raster operations on our pixels and foreground color (selected brush)
			BitBlt(hDC, X, Y, m_Width, m_Height, this->hDC, (ch%256)*m_Width, (ch/256)*m_Height, 0x00E20746/*DSPDxax*/);
			//MaskBlt(hDC, X, Y, m_Width, m_Height, hDC/*NULL?*/, X, Y, this->hBitmap, (ch%256)*m_Width, (ch/256)*m_Height, MAKEROP4(PATCOPY/*text/foreground*/, 0x00AA0029/*D:background*/));
			X += m_Width;
		}
	}

	// Normal (?) behavior for TrueColor displays
	virtual void TextDraw(COLORREF* pDstPixels, size_t iDstStride, COLORREF cFG, COLORREF cBG, LPCWSTR lpString, UINT cbCount)
	{
		size_t iSrcSlack = m_Width * 255;
		size_t iDstSlack = iDstStride - m_Width;
		COLORREF colors[2] = {cBG, cFG};
		for (; cbCount; cbCount--, lpString++)
		{
			wchar_t ch = *lpString;
			int fx = m_Width *(ch%256);
			int fy = m_Height*(ch/256);
			bool* pSrcPos = bpMPixels + fx + m_Width*256*fy;
			COLORREF* pDstPos = pDstPixels;
			#ifdef DEBUG_BDF_DRAW
			_ASSERTE(!IsBadWritePtr(pDstPos, m_Height*m_Width*sizeof(*pDstPos)));
			#endif
			if (cBG == CLR_INVALID) // transparent
			{
				for (int y=0; y<m_Height; y++)
				{
					for (int x=0; x<m_Width; x++)
						if (*pSrcPos++)
							*pDstPos++ = cFG;
						else
							pDstPos++;
					pSrcPos += iSrcSlack;
					pDstPos += iDstSlack;
				}
			}
			else // opaque
			{
				for (int y=0; y<m_Height; y++)
				{
					for (int x=0; x<m_Width; x++)
						*pDstPos++ = colors[*pSrcPos++];
					pSrcPos += iSrcSlack;
					pDstPos += iDstSlack;
				}
			}
			pDstPixels += m_Width;
		}
	}
};

// BDF

BOOL BDF_GetFamilyName(LPCTSTR lpszFilePath, wchar_t (&rsFamilyName)[LF_FACESIZE])
{
	if (!BDFFont::GetFamilyName(lpszFilePath, rsFamilyName))
	{
		lstrcpyn(rsFamilyName, PointToName(lpszFilePath), LF_FACESIZE);
		return TRUE;
	}

	return TRUE;
}

CustomFont* BDF_Load( LPCTSTR lpszFilePath )
{
	return BDFFont::Load(lpszFilePath);
}

// CachedBrush (for CEDC)

CachedSolidBrush::~CachedSolidBrush()
{
	if (m_Brush != NULL)
		DeleteObject(m_Brush);
}

HBRUSH CachedSolidBrush::Get(COLORREF c)
{
	if (c != m_Color)
	{
		DeleteObject(m_Brush);
		m_Brush = CreateSolidBrush(c);
	}
	return m_Brush;
}

// CEDC

CEDC::CEDC(HDC hDc)
	: hDC(hDc)
	, hBitmap(NULL)
	, mh_OldBitmap(NULL)
	, mh_OldFont(NULL)
{
	mb_ExtDc = (hDc != NULL);
	iWidth = 0;
	iHeight = 0;
	Reset();
}

void CEDC::Reset()
{
	_ASSERTE(mb_ExtDc || (hDC == NULL && hBitmap == NULL));

	mh_OldBitmap = NULL;
	pPixels = NULL;
	m_Font.Release();
	mh_OldFont = NULL;
	m_BkColor = CLR_INVALID;
	m_TextColor = CLR_INVALID;
	m_BkMode = -1;
	iWidth = 0;
	iHeight = 0;
}

void CEDC::Delete()
{
	if (hDC)
	{
		if (mh_OldFont)
		{
			::SelectObject(hDC, mh_OldFont);
			mh_OldFont = NULL;
		}

		if (mb_ExtDc)
		{
			_ASSERTEX(mh_OldBitmap==NULL);
		}
		else
		{
			if (mh_OldBitmap)
				::SelectObject(hDC, mh_OldBitmap);
			::DeleteDC(hDC);
		}

		hDC = NULL;
	}

	if (hBitmap)
	{
		DeleteObject(hBitmap); hBitmap = NULL;
	}

	Reset();
}

bool CEDC::Create(UINT Width, UINT Height)
{
	Delete();

	const HDC hScreenDC = GetDC(NULL);
	_ASSERTE(hScreenDC);
	wchar_t szInfo[128];

	mb_ExtDc = false;

	hDC = CreateCompatibleDC(hScreenDC);

	if (hDC == NULL)
	{
		Assert((hDC!=NULL) && "CreateCompatibleDC failed!");

		ReleaseDC(NULL, hScreenDC);
		return false;
	}


	//**************************
	//if (ghOpWnd)
	//	gpConEmu->UpdateSizes();
	//**************************

	int iRemote = -1;
	int nPixels = GetDeviceCaps(hScreenDC, BITSPIXEL);
	if (nPixels >= 32)
	{
		// For custom font rendering
		BITMAPINFO bmi;
		bmi.bmiHeader.biSize        = sizeof(bmi.bmiHeader);
		bmi.bmiHeader.biWidth       = Width;
		bmi.bmiHeader.biHeight      = -(i32)Height;
		bmi.bmiHeader.biPlanes      = 1;
		bmi.bmiHeader.biBitCount    = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		void* pvBits;
		hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
		pPixels = (COLORREF*)pvBits;

		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Surface created DIB(%i,%i,%i)=x%08X", (int)bmi.bmiHeader.biWidth, (int)bmi.bmiHeader.biHeight, (int)bmi.bmiHeader.biBitCount, (DWORD)(DWORD_PTR)hBitmap);
		::LogString(szInfo);
	}
	else
	{
		DWORD nErr = GetLastError();
		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Warning! Color depth of your display is low (%i), 32bit recommended! ErrCode=%u", nPixels, nErr);
		::LogString(szInfo);

		hBitmap = CreateCompatibleBitmap(hScreenDC, Width, Height);

		// May be performance drawbacks when using bdf fonts (m_Font.iType == CEFONT_CUSTOM)
		iRemote = GetSystemMetrics(0x1000/*SM_REMOTESESSION*/);
		// Remote desktop to Win2k8 gets here and (nPixels==16)
		_ASSERTE((pPixels || (iRemote==1)) && "Remote desktop? Caps(BitsPerPixel)!=32");

		_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Surface created BMP(%i,%i,Compatible)=x%08X, Remote=%i", (int)Width, (int)Height, (DWORD)(DWORD_PTR)hBitmap, iRemote);
		::LogString(szInfo);
	}

	if (hBitmap)
	{
		iWidth = Width;
		iHeight = Height;

		mh_OldBitmap = (HBITMAP)::SelectObject(hDC, hBitmap);
	}

	ReleaseDC(0, hScreenDC);

	return (hBitmap!=NULL);
}

void CEDC::SelectFont(CFont* font)
{
	// Already selected?
	if (m_Font.Equal(font))
		return;

	m_Font = font;

	if (font == NULL)
	{
		if (mh_OldFont)
		{
			::SelectObject(hDC, mh_OldFont);
			mh_OldFont = NULL;
		}
	}
	else if (font->iType == CEFONT_GDI)
	{
		HFONT hOldFont = (HFONT)::SelectObject(hDC, font->hFont);
		if (mh_OldFont == NULL)
			mh_OldFont = hOldFont;

		if (m_TextColor != CLR_INVALID)
			::SetTextColor(hDC, m_TextColor);
		if (m_BkColor != CLR_INVALID)
			::SetBkColor(hDC, m_BkColor);
		if (m_BkMode != -1)
			::SetBkMode(hDC, m_BkColor);
	}
}

void CEDC::SetTextColor(COLORREF color)
{
	if ((m_TextColor != color) && (!m_Font.IsSet() || (m_Font->iType != CEFONT_CUSTOM)))
		::SetTextColor(hDC, color);
	m_TextColor = color;
}

void CEDC::SetBkColor(COLORREF color)
{
	if ((m_BkColor != color) && (!m_Font.IsSet() || (m_Font->iType != CEFONT_CUSTOM)))
		::SetBkColor(hDC, color);
	m_BkColor = color;
}

void CEDC::SetBkMode(int iBkMode)
{
	if ((m_BkMode != iBkMode) && (!m_Font.IsSet() || (m_Font->iType != CEFONT_CUSTOM)))
		::SetBkMode(hDC, iBkMode);
	m_BkMode = iBkMode;
}

static COLORREF FlipChannels(COLORREF c)
{
	return (c >> 16) | (c & 0x00FF00) | ((c & 0xFF) << 16);
}

bool CEDC::TextDraw(int X, int Y, UINT fuOptions, const RECT *lprc, LPCWSTR lpString, UINT cbCount, const INT *lpDx)
{
	BOOL lbRc = FALSE;
	CEFONT_TYPE iType = m_Font.IsSet() ? m_Font->iType : CEFONT_NONE;

	switch (iType)
	{
	case CEFONT_GDI:
	case CEFONT_NONE:
		{
			lbRc = ::ExtTextOut(hDC, X, Y, fuOptions, lprc, lpString, cbCount, lpDx);
			break;
		}

	case CEFONT_CUSTOM:
		{
			_ASSERTE(m_Font.IsSet());
			if (pPixels)
			{
				// We get here when our display has 32bit color depth (OK and fast)
				m_Font->pCustomFont->TextDraw(pPixels + X + Y*iWidth, iWidth,
					FlipChannels(m_TextColor), fuOptions & ETO_OPAQUE ? FlipChannels(m_BkColor) : CLR_INVALID, lpString, cbCount);
			}
			else
			{
				// We get here for non-true-color displays (compatibility, slower)

				//TODO: Opaque is not used at the moment, background is painted separately
				_ASSERTE(!(fuOptions & ETO_OPAQUE));
				if (fuOptions & ETO_OPAQUE)
				{
					//hOldBrush = ::SelectObject(hDC, m_BgBrush.Get(m_BkColor));
					LONG w, h;
					m_Font->pCustomFont->GetBoundingBox(&w, &h);
					RECT r = {X, Y, (LONG)(X+w*cbCount), Y+h};
					FillRect(hDC, &r, m_BgBrush.Get(m_BkColor));
				}

				//TODO: Optimize: no need to switch brush if it was already selected
				HBRUSH hOldBrush = (HBRUSH)::SelectObject(hDC, m_FgBrush.Get(m_TextColor));

				m_Font->pCustomFont->TextDraw(hDC, X, Y, lpString, cbCount);

				//TODO: Optimize: no need to revert brush if we paint transparent text now...
				::SelectObject(hDC, hOldBrush);
			}

			lbRc = TRUE;
			break;
		} // CEFONT_CUSTOM

	default:
		{
			_ASSERTE(iType==CEFONT_GDI || iType==CEFONT_CUSTOM);
			lbRc = FALSE;
		}
	}

	return (lbRc != FALSE);
}

bool CEDC::TextDrawOem(int X, int Y, UINT fuOptions, const RECT *lprc, LPCSTR lpString, UINT cbCount, const INT *lpDx)
{
	BOOL lbRc = FALSE;
	CEFONT_TYPE iType = m_Font.IsSet() ? m_Font->iType : CEFONT_NONE;

	switch (iType)
	{
	case CEFONT_GDI:
	case CEFONT_NONE:
		{
			lbRc = ::ExtTextOutA(hDC, X, Y, fuOptions, lprc, lpString, cbCount, lpDx);
			break;
		}

	case CEFONT_CUSTOM:
		{
			_ASSERTE(m_Font.IsSet());
			wchar_t* lpWString = (wchar_t*)malloc((cbCount+1) * sizeof(wchar_t));
			if (lpWString)
			{
				//for (UINT cb=0; cb<cbCount; cb++)
				//	lpWString[cb] = lpString[cb];  // WideCharToMultiByte?
				MultiByteToWideChar(CP_OEMCP, 0, lpString, -1, lpWString, cbCount);
				lpWString[cbCount] = 0; // AsciiZ в принципе не требуется, но для удобства.

				lbRc = TextDraw(X, Y, fuOptions, lprc, lpWString, cbCount, lpDx);

				free(lpWString);
			}
			break;
		} // CEFONT_CUSTOM

	default:
		{
			_ASSERTE(iType==CEFONT_GDI || iType==CEFONT_CUSTOM);
			lbRc = FALSE;
		}
	}

	return (lbRc != FALSE);
}

bool CEDC::TextExtentPoint(LPCTSTR ch, int c, LPSIZE sz)
{
	BOOL lbRc = FALSE;
	CEFONT_TYPE iType = m_Font.IsSet() ? m_Font->iType : CEFONT_NONE;

	switch (iType)
	{
	case CEFONT_GDI:
	case CEFONT_NONE:
		{
			// GetTextExtentPoint32 fails on some symbols. Example, U+27F6 (⟶)
			// * it **may** be painted by ExtTextOut in **some** sequences (surrounding glyphs matter?)
			// * but GetTextExtentPoint32 always returns width of "absent" glyph (codepoint does not exist in almost any font...)
			RECT rc = {};
			lbRc = (::DrawText(hDC, ch, c, &rc, DT_CALCRECT|DT_NOPREFIX) != 0);
			sz->cx = rc.right; sz->cy = rc.bottom;
			break;
		}

	case CEFONT_CUSTOM:
		{
			_ASSERTE(c>=1); _ASSERTE(m_Font.IsSet());
			m_Font->pCustomFont->GetBoundingBox(&sz->cx, &sz->cy);
			// All glyphs in bdf are painted in their own cells,
			// non-printable, composites and double-width glyphs
			// are not supported at the moment (in bdf)
			sz->cx *= c;
			lbRc = TRUE;
			break;
		} // CEFONT_CUSTOM

	default:
		{
			_ASSERTE(iType==CEFONT_GDI || iType==CEFONT_CUSTOM);
			lbRc = FALSE;
		}
	}

	return (lbRc != FALSE);
}
