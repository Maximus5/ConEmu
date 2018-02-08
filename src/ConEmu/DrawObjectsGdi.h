
/*
Copyright (c) 2015-2018 Maximus5
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
#include "DrawObjectsImpl.h"

namespace drawing
{
namespace detail
{

class FactoryImplGdi : public FactoryImpl
{
public:
	FactoryImplGdi();
	virtual ~FactoryImplGdi();

	virtual bool Initialize() override;
	virtual void Finalize() override;

public:
	// Canvas
	virtual Canvas CreateCanvas(HWND wnd, CanvasType canvas_type) override;
	virtual Canvas CreateCanvasHDC(HDC dc) override;
	virtual Canvas CreateCanvasDI(UINT width, UINT height, UINT bits = 32) override;
	virtual Canvas CreateCanvasCompatible(UINT width, UINT height, HWND compatible) override;
	virtual int CanvasSaveState(const Canvas& canvas) override;
	virtual void CanvasRestoreState(const Canvas& canvas, const CanvasState& state) override;
	virtual void ReleaseCanvas(Canvas& canvas) override;
	virtual bool SelectBrush(const Canvas& canvas, const Brush& brush) override;
	virtual bool SelectPen(const Canvas& canvas, const Pen& pen) override;
	virtual bool SelectFont(const Canvas& canvas, const Font& font) override;
	virtual bool SelectBitmap(const Canvas& canvas, const Bitmap& bitmap) override;

	// Text
	virtual bool SetTextColor(const Canvas& canvas, COLORREF clr) override;
	virtual bool SetBkColor(const Canvas& canvas, COLORREF clr) override;
	virtual bool SetBkMode(const Canvas& canvas, BkMode bk_mode) override;
	virtual bool TextDraw(const Canvas& canvas, LPCWSTR pszString, int iLen, int x, int y, const RECT *lprc, UINT Flags, const INT *lpDx) override;
	virtual bool TextDrawOem(const Canvas& canvas, LPCSTR pszString, int iLen, int x, int y, const RECT *lprc, UINT Flags, const INT *lpDx) override;
	virtual bool TextExtentPoint(const Canvas& canvas, LPCWSTR pszChars, UINT iLen, SIZE& sz) override;

	// Region
	virtual Rgn CreateRgn(const RECT& rect) override;
	virtual void ReleaseRgn(Rgn& rgn) override;

	// Brush
	virtual Brush CreateBrush(COLORREF color) override;
	virtual void ReleaseBrush(Brush& brush) override;

	// Pen
	virtual Pen CreatePen(COLORREF color, UINT width = 1) override;
	virtual void ReleasePen(Pen& pen) override;

	// Font
	virtual Font CreateFont(const LOGFONT& font) override;
	virtual void ReleaseFont(Font& font) override;

	// Bitmap
	virtual Bitmap CreateBitmap(UINT width, UINT height, UINT bits = 32) override;
	virtual Bitmap CreateBitmap(UINT width, UINT height, const CanvasImpl& compatible) override;
	virtual void ReleaseBitmap(Bitmap& bitmap) override;

public:
	// Features
	bool IsCompatibleRequired() override;
};

};
};
