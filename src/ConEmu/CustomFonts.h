
/*
Copyright (c) 2012 thecybershadow
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

#include <windows.h>
#include <crtdbg.h>

#define CE_BDF_SUFFIX L" [BDF]"
#define CE_BDF_SUFFIX_LEN 6

// CustomFont

class CustomFont
{
public:
	virtual BOOL IsMonospace()=0;
	virtual BOOL HasUnicode()=0;
	virtual BOOL HasBorders()=0;
	virtual void GetBoundingBox(long *pX, long *pY)=0;
	virtual int GetSize()=0;
	virtual BOOL IsBold()=0;
	virtual BOOL IsItalic()=0;
	virtual BOOL IsUnderline()=0;
	virtual void DrawText(HDC hDC, int X, int Y, LPCWSTR lpString, UINT cbCount)=0;
	virtual void DrawText(COLORREF* pPixels, size_t iStride, COLORREF cFG, COLORREF cBG, LPCWSTR lpString, UINT cbCount)=0;
	virtual ~CustomFont() {}
};

// CustomFontFamily

class CustomFontFamily
{
	struct Impl;
	Impl *pImpl;
public:
	CustomFontFamily() : pImpl(NULL) {}
	~CustomFontFamily();
	void AddFont(CustomFont* font);
	CustomFont* GetFont(int iSize, BOOL bBold, BOOL bItalic, BOOL bUnderline);
};

// CEFONT

#define CEFONT_NONE   0
#define CEFONT_GDI    1
#define CEFONT_CUSTOM 2

struct CEFONT
{
	int iType;
	union
	{
		HFONT hFont;
		CustomFont* pCustomFont;
	};

	CEFONT() : iType(CEFONT_NONE), hFont(NULL) {}
	CEFONT(HFONT hFont) : iType(CEFONT_GDI), hFont(hFont) {}

	BOOL IsSet() const
	{
		switch (iType)
		{
		case CEFONT_NONE:
			return FALSE;
		case CEFONT_GDI:
			return hFont != NULL;
		case CEFONT_CUSTOM:
			return pCustomFont != NULL;
		}
		_ASSERT(0);
		return FALSE;
	}

	BOOL Delete()
	{
		BOOL result = FALSE;
		switch (iType)
		{
		case CEFONT_NONE:
			return FALSE;
		case CEFONT_GDI:
			if (hFont)
			{
				result = DeleteObject(hFont);
				hFont = NULL;
			}
			break;
		case CEFONT_CUSTOM:
			if (pCustomFont)
			{
				result = TRUE;
				// delete pCustomFont; // instance is owned by OptionsClass
				pCustomFont = NULL;
			}
			break;
		default:
			_ASSERT(0);
		}
		return result;
	}
};

static bool operator== (const CEFONT &a, const CEFONT &b)
{
	if (a.iType != b.iType)
		return false;
	switch (a.iType)
	{
	case CEFONT_GDI:
		return a.hFont == b.hFont;
	case CEFONT_CUSTOM:
		return a.pCustomFont == b.pCustomFont;
	}
	_ASSERT(0);
	return FALSE;
}

static bool operator!= (const CEFONT &a, const CEFONT &b)
{
	return !(a == b);
}

// CachedBrush (for CEDC)

struct CachedSolidBrush
{
	CachedSolidBrush() : m_Color(CLR_INVALID), m_Brush(NULL) {}
	~CachedSolidBrush();
	HBRUSH Get(COLORREF);
private:
	COLORREF m_Color;
	HBRUSH m_Brush;
};

// CEDC

struct CEDC
{
	HDC hDC;
	COLORREF* pPixels; LONG iWidth;

	CEDC(HDC hDC) : hDC(hDC), pPixels(NULL), m_Font(CEFONT()), m_TextColor(CLR_INVALID), m_BkColor(CLR_INVALID), m_BkMode(-1) {}
	operator HDC() const { return hDC; }

	CEFONT SelectObject(CEFONT font);
	void SetTextColor(COLORREF color);
	void SetBkColor(COLORREF color);
	void SetBkMode( int iBkMode );
	BOOL ExtTextOut(int X, int Y, UINT fuOptions, const RECT *lprc, LPCWSTR lpString, UINT cbCount, const INT *lpDx);
	BOOL ExtTextOutA(int X, int Y, UINT fuOptions, const RECT *lprc, LPCSTR lpString, UINT cbCount, const INT *lpDx);
	BOOL GetTextExtentPoint32( LPCTSTR ch, int c, LPSIZE sz );
private:
	CEFONT m_Font;
	COLORREF m_BkColor, m_TextColor;
	int m_BkMode;

	CachedSolidBrush m_BgBrush, m_FgBrush;
};

// BDF

BOOL BDF_GetFamilyName(LPCTSTR lpszFilePath, wchar_t (&rsFamilyName)[LF_FACESIZE]);
CustomFont* BDF_Load(LPCTSTR lpszFilePath);
