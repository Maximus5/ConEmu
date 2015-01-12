
/*
Copyright (c) 2015-2016 Maximus5
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
#include "DrawObjects.h"

class CDrawFactoryGdi : public CDrawFactory
{
public:
	CDrawFactoryGdi();
	virtual ~CDrawFactoryGdi();

public:
	// Canvas
	virtual CDrawCanvas* NewCanvas(HWND Wnd) override;
	virtual CDrawCanvas* NewCanvasHDC(HDC Dc) override;
	virtual CDrawCanvas* NewCanvasDI(UINT Width, UINT Height, UINT Bits = 32) override;
	virtual CDrawCanvas* NewCanvasCompatible(UINT Width, UINT Height, HWND Compatible) override;
	// Brush
	virtual CDrawObjectBrush* NewBrush(COLORREF Color) override;
	// Pen
	virtual CDrawObjectPen* NewPen(COLORREF Color, UINT Width = 1) override;
	// Font
	virtual CDrawObjectFont* NewFont(const LOGFONT& Font) override;
	// Bitmap
	virtual CDrawObjectBitmap* NewBitmap(UINT Width, UINT Height, CDrawCanvas* Compatible = NULL) override;
public:
	// Features
	virtual bool IsCompatibleRequired() override;
};

class CDrawCanvasGdi : public CDrawCanvas
{
public:
	CDrawCanvasGdi(CDrawFactory* pFactory);

public:
	virtual bool InitCanvas(HWND Wnd) override;
	virtual bool InitCanvasHDC(HDC Dc) override;
	virtual bool InitCanvasDI(UINT Width, UINT Height, UINT Bits = 32) override;
	virtual bool InitCanvasCompatible(UINT Width, UINT Height, HWND Compatible) override;

protected:
	// Methods
	virtual void InternalFreeCanvas() override;

public:
	// Operators
	operator HDC() const;

public:
	// Helpers
	virtual void SelectBrush(CDrawObjectBrush* Brush) override;
	virtual void SelectPen(CDrawObjectPen* Pen) override;
	virtual void SelectFont(CDrawObjectFont* Font) override;
	virtual void SelectBitmap(CDrawObjectBitmap* Bitmap) override;

public:
	// Helpers for painting objects (like checkboxes, radio buttons), drawing text, blitting bitmaps and so on..
	#if 0
	virtual bool DrawButtonCheck(int x, int y, int cx, int cy, DWORD DFCS_Flags) override;
	virtual bool DrawButtonRadio(int x, int y, int cx, int cy, DWORD DFCS_Flags) override;
	#endif
protected:
	// Text
	virtual void SetTextColor(COLORREF clr) override;
	virtual void SetBkColor(COLORREF clr) override;
	virtual void SetBkMode(int iBkMode) override;
	virtual bool TextDraw(LPCWSTR pszString, int iLen, int x, int y, const RECT *lprc, UINT Flags, const INT *lpDx) override;
	virtual bool TextDrawOem(LPCSTR pszString, int iLen, int x, int y, const RECT *lprc, UINT Flags, const INT *lpDx) override;
	virtual bool TextExtentPoint(LPCWSTR pszChars, UINT iLen, LPSIZE sz) override;
};

class CDrawObjectBrushGdi : public CDrawObjectBrush
{
public:
	CDrawObjectBrushGdi(CDrawFactory* pFactory);

public:
	bool InitBrush(COLORREF Color);

protected:
	virtual void InternalFree();

public:
	// Operators
	operator HBRUSH() const;
};

class CDrawObjectPenGdi : public CDrawObjectPen
{
public:
	CDrawObjectPenGdi(CDrawFactory* pFactory);

public:
	bool InitPen(COLORREF Color, UINT Width = 1);

protected:
	virtual void InternalFree();

public:
	// Operators
	operator HPEN() const;
};

class CDrawObjectFontGdi : public CDrawObjectFont
{
public:
	CDrawObjectFontGdi(CDrawFactory* pFactory);

public:
	bool InitFont(const LOGFONT& Font);

protected:
	virtual void InternalFree();

public:
	// Operators
	operator HFONT() const;
};

class CDrawObjectBitmapGdi : public CDrawObjectBitmap
{
public:
	CDrawObjectBitmapGdi(CDrawFactory* pFactory);

public:
	bool InitBitmap(UINT Width, UINT Height, CDrawCanvas* Compatible = NULL);

protected:
	virtual void InternalFree();

public:
	// Operators
	operator HBITMAP() const;
};
