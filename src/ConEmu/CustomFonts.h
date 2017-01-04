
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

#pragma once

#include "Header.h"
#include "FontPtr.h"

class CFont;

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
	virtual void TextDraw(HDC hDC, int X, int Y, LPCWSTR lpString, UINT cbCount)=0;
	virtual void TextDraw(COLORREF* pPixels, size_t iStride, COLORREF cFG, COLORREF cBG, LPCWSTR lpString, UINT cbCount)=0;
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
	HBITMAP hBitmap;
	COLORREF* pPixels;
	LONG iWidth, iHeight;

	CEDC(HDC hDc);
	//~CEDC(); -- it is caller responsibility

	bool Create(UINT Width, UINT Height);
	void Delete();

	operator HDC() const { return hDC; }

	void SelectFont(CFont* font);
	void SetTextColor(COLORREF color);
	void SetBkColor(COLORREF color);
	void SetBkMode( int iBkMode );
	bool TextDraw(int X, int Y, UINT fuOptions, const RECT *lprc, LPCWSTR lpString, UINT cbCount, const INT *lpDx);
	bool TextDrawOem(int X, int Y, UINT fuOptions, const RECT *lprc, LPCSTR lpString, UINT cbCount, const INT *lpDx);
	bool TextExtentPoint( LPCTSTR ch, int c, LPSIZE sz );
private:
	bool mb_ExtDc;
	void Reset();

	HBITMAP mh_OldBitmap;
	CFontPtr m_Font;
	HFONT mh_OldFont;
	COLORREF m_BkColor, m_TextColor;
	int m_BkMode;

	CachedSolidBrush m_BgBrush, m_FgBrush;
};

// BDF

BOOL BDF_GetFamilyName(LPCTSTR lpszFilePath, wchar_t (&rsFamilyName)[LF_FACESIZE]);
CustomFont* BDF_Load(LPCTSTR lpszFilePath);
