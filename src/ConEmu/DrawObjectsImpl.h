
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
#include "../common/defines.h"
#include "DrawObjects.h"

namespace drawing
{
namespace detail
{

class ObjectImplBase : public detail::noncopyable, public detail::nonmoveable
{
private:
	DrawObjectType Type = DrawObjectType::dt_None;
	HANDLE         Object = NULL;
	bool           External = false;

	mutable int    Counter = 1;

protected:
	friend class drawing::Factory;

	// AFTER Factory call appropriate function over handle (e.g. ReleaseDC((HDC)Object)) we set Object to NULL
	void resetObject();

public:
	ObjectImplBase(DrawObjectType _Type, HANDLE _Obj = NULL, bool _External = false);
	virtual ~ObjectImplBase();

	// It would be better to make them protected
	int AddRef() const;
	int Release() const;

	template <class H>
	H Handle() const;

	// if true - no need to physical disposing the handle (e.g. handles from GetStockObject(...))
	bool isExternal() const;

	DrawObjectType getType() const;

	operator bool() const;
	bool operator==(const ObjectImplBase& o) const;
	bool operator!=(const ObjectImplBase& o) const;
};



class CanvasImpl : public ObjectImplBase
{
private:
	HWND        mh_Wnd = NULL;
	DWORD       mn_ThreadID = 0;
	CanvasType  m_canvas_type = CanvasType::Undefined;
	PAINTSTRUCT m_ps = {};

protected:
	friend class drawing::Factory;

	CanvasState  m_cur_state;
	CanvasState  m_prev_state;

public:
	CanvasImpl() = delete;
	CanvasImpl(HDC _hdc, HWND _hwnd, CanvasType _canvas_type);
	CanvasImpl(HDC _hdc, HWND _hwnd, const PAINTSTRUCT& ps);
	CanvasImpl(HDC _hdc, bool _external, CanvasType _canvas_type);

	CanvasType getCanvasType() const;
	const PAINTSTRUCT* getPaintStruct() const;
	HWND getWindow() const;

	void SaveState(int state);
};



class RgnImpl : public ObjectImplBase
{
protected:
	RECT m_Bounding = {};
public:
	RgnImpl() = delete;
	RgnImpl(HRGN _hrgn, bool _external, const RECT& _bounding = RECT{});
};


class BrushImpl : public ObjectImplBase
{
protected:
	COLORREF m_Color = 0;
public:
	BrushImpl() = delete;
	BrushImpl(HBRUSH _hbrush, bool _external, COLORREF _color = 0);
};



class PenImpl : public ObjectImplBase
{
protected:
	COLORREF m_Color = 0;
	UINT mn_Width = 1;
public:
	PenImpl() = delete;
	PenImpl(HPEN _hpen, bool _external, COLORREF _color = 0, UINT _width = 1);
};



class FontImpl : public ObjectImplBase
{
protected:
	LOGFONT m_LogFont = {};
public:
	FontImpl() = delete;
	FontImpl(HFONT _hfont, bool _external, const LOGFONT& _lf = LOGFONT{});
};



class BitmapImpl : public ObjectImplBase
{
protected:
	UINT mn_Width = 0, mn_Height = 0;
public:
	BitmapImpl(HBITMAP _hbitmap, bool _external, UINT _width = 0, UINT _height = 0);
};



class FactoryImpl : public detail::noncopyable
{
public:
	FactoryImpl();
	virtual ~FactoryImpl();

	virtual bool Initialize() = 0;
	virtual void Finalize() = 0;

public:
	// Canvas
	virtual Canvas CreateCanvas(HWND wnd, CanvasType canvas_type) = 0;
	virtual Canvas CreateCanvasHDC(HDC dc) = 0;
	virtual Canvas CreateCanvasDI(UINT width, UINT height, UINT bits = 32) = 0;
	virtual Canvas CreateCanvasCompatible(UINT width, UINT height, HWND compatible) = 0;
	virtual int CanvasSaveState(const Canvas& canvas) = 0;
	virtual void CanvasRestoreState(const Canvas& canvas, const CanvasState& state) = 0;
	virtual void ReleaseCanvas(Canvas& canvas) = 0;
	virtual bool SelectBrush(const Canvas& canvas, const Brush& brush) = 0;
	virtual bool SelectPen(const Canvas& canvas, const Pen& pen) = 0;
	virtual bool SelectFont(const Canvas& canvas, const Font& font) = 0;
	virtual bool SelectBitmap(const Canvas& canvas, const Bitmap& bitmap) = 0;

	// Text
	virtual bool SetTextColor(const Canvas& canvas, COLORREF clr) = 0;
	virtual bool SetBkColor(const Canvas& canvas, COLORREF clr) = 0;
	virtual bool SetBkMode(const Canvas& canvas, BkMode bk_mode) = 0;
	virtual bool TextDraw(const Canvas& canvas, LPCWSTR pszString, int iLen, int x, int y, const RECT *lprc, UINT Flags, const INT *lpDx) = 0;
	virtual bool TextDrawOem(const Canvas& canvas, LPCSTR pszString, int iLen, int x, int y, const RECT *lprc, UINT Flags, const INT *lpDx) = 0;
	virtual bool TextExtentPoint(const Canvas& canvas, LPCWSTR pszChars, UINT iLen, SIZE& sz) = 0;

	// Region
	virtual Rgn CreateRgn(const RECT& rect) = 0;
	virtual void ReleaseRgn(Rgn& rgn) = 0;

	// Brush
	virtual Brush CreateBrush(COLORREF color) = 0;
	virtual void ReleaseBrush(Brush& brush) = 0;

	// Pen
	virtual Pen CreatePen(COLORREF color, UINT width = 1) = 0;
	virtual void ReleasePen(Pen& pen) = 0;

	// Font
	virtual Font CreateFont(const LOGFONT& font) = 0;
	virtual void ReleaseFont(Font& font) = 0;

	// Bitmap
	virtual Bitmap CreateBitmap(UINT width, UINT height, UINT bits = 32) = 0;
	virtual Bitmap CreateBitmap(UINT width, UINT height, const CanvasImpl& compatible) = 0;
	virtual void ReleaseBitmap(Bitmap& bitmap) = 0;

public:
	// Features
	virtual bool IsCompatibleRequired() = 0;
};


};
};
