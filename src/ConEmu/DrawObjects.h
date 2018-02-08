
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

namespace drawing
{

class Factory;

enum class DrawObjectType
{
	dt_None = 0,
	dt_Canvas,
	dt_Brush,
	dt_Pen,
	dt_Font,
	dt_Bitmap,
	dt_Rgn,
};

enum class CanvasType
{
	Undefined,
	Window,    // ReleaseDC is required
	Client,    // ReleaseDC is required
	Paint,     // EndPaint is required
	Memory,    // DeleteDC is required
};

enum class BkMode
{
	Opaque,
	Transparent,
};


namespace detail
{
	class FactoryImpl;
	class ObjectImplBase;
	class BitmapImpl;
	class BrushImpl;
	class CanvasImpl;
	class FontImpl;
	class PenImpl;
	class RgnImpl;

	class noncopyable
	{
	public:
		noncopyable() {};
		virtual ~noncopyable() {};
		noncopyable(const noncopyable& src) = delete;
		noncopyable& operator=(const noncopyable& src) = delete;
	};

	class nonmoveable
	{
	public:
		nonmoveable() {};
		virtual ~nonmoveable() {};
		nonmoveable(const nonmoveable& src) = delete;
		nonmoveable& operator=(const nonmoveable& src) = delete;
	};


	// This class is not thread-safe, the drawing is expected
	// to be executed in the main thread only
	template<typename T>
	class ObjectPtr
	{
	private:
		T* object = nullptr;

	public:
		ObjectPtr();
		virtual ~ObjectPtr();

		ObjectPtr(T*&& _obj);
		ObjectPtr<T>& operator=(T*&& _obj) = delete;
		void reset(T*&& _obj);

		ObjectPtr(const ObjectPtr<T>& _src);
		ObjectPtr<T>& operator=(const ObjectPtr<T>& _src);

		ObjectPtr(ObjectPtr<T>&& _src);
		ObjectPtr<T>& operator=(ObjectPtr<T>&& _src);

		template<typename H>
		H Handle() const;

		operator const T&() const;
		T* operator->() const;
		T* get() const;
		operator bool() const;
		bool operator!() const;
		bool operator==(const ObjectPtr<T>& _obj) const;
		bool operator!=(const ObjectPtr<T>& _obj) const;
	};
};

using Bitmap = detail::ObjectPtr<detail::BitmapImpl>;
using Brush = detail::ObjectPtr<detail::BrushImpl>;
using Canvas = detail::ObjectPtr<detail::CanvasImpl>;
using Font = detail::ObjectPtr<detail::FontImpl>;
using Pen = detail::ObjectPtr<detail::PenImpl>;
using Rgn = detail::ObjectPtr<detail::RgnImpl>;

struct CanvasState
{
	int    state = 0;

	Bitmap bitmap;
	Brush  brush;
	Font   font;
	Pen    pen;

	bool valid() const { return (state > 0); };
};



class Factory : public detail::noncopyable
{
	detail::FactoryImpl* mp_gdi = nullptr;

public:
	Factory();
	~Factory() {}; // Intentionally empty

	bool Initialize();
	void Finalize();
public:
	// Canvas
	Canvas CreateCanvas(HWND wnd, CanvasType canvas_type);
	Canvas CreateCanvasHDC(HDC dc);
	Canvas CreateCanvasDI(UINT width, UINT height, UINT bits = 32);
	Canvas CreateCanvasCompatible(UINT width, UINT height, HWND compatible);
	CanvasState CanvasSaveState(const Canvas& canvas);
	void CanvasRestoreState(const Canvas& canvas, const CanvasState& state);
	bool SelectBrush(const Canvas& canvas, const Brush& brush);
	bool SelectPen(const Canvas& canvas, const Pen& pen);
	bool SelectFont(const Canvas& canvas, const Font& font);
	bool SelectBitmap(const Canvas& canvas, const Bitmap& bitmap);

	// Text
	bool SetTextColor(const Canvas& canvas, COLORREF clr);
	bool SetBkColor(const Canvas& canvas, COLORREF clr);
	bool SetBkMode(const Canvas& canvas, BkMode bk_mode);
	bool TextDraw(const Canvas& canvas, LPCWSTR pszString, int iLen, int x, int y, const RECT *lprc, UINT Flags, const INT *lpDx);
	bool TextDrawOem(const Canvas& canvas, LPCSTR pszString, int iLen, int x, int y, const RECT *lprc, UINT Flags, const INT *lpDx);
	bool TextExtentPoint(const Canvas& canvas, LPCWSTR pszChars, UINT iLen, SIZE& sz);

	// Region
	Rgn CreateRgn(const RECT& rect);

	// Brush
	Brush CreateBrush(COLORREF color);

	// Pen
	Pen CreatePen(COLORREF color, UINT width = 1);

	// Font
	Font CreateFont(const LOGFONT& font);

	// Bitmap
	Bitmap CreateBitmap(UINT width, UINT height, UINT bits = 32);
	Bitmap CreateBitmap(UINT width, UINT height, const Canvas& compatible);

protected:
	void ReleaseBitmap(Bitmap& bitmap);
	void ReleaseBrush(Brush& brush);
	void ReleaseCanvas(Canvas& canvas);
	void ReleaseFont(Font& font);
	void ReleasePen(Pen& pen);
	void ReleaseRgn(Rgn& rgn);
public:
	template <typename T> void ReleaseObject(T& object);
	template <> void ReleaseObject<Bitmap>(Bitmap& object) { ReleaseBitmap(object); };
	template <> void ReleaseObject<Brush>(Brush& object) { ReleaseBrush(object); };
	template <> void ReleaseObject<Canvas>(Canvas& object) { ReleaseCanvas(object); };
	template <> void ReleaseObject<Font>(Font& object) { ReleaseFont(object); };
	template <> void ReleaseObject<Pen>(Pen& object) { ReleasePen(object); };
	template <> void ReleaseObject<Rgn>(Rgn& object) { ReleaseRgn(object); };

public:
	// Features
	bool IsCompatibleRequired();
};

};

extern drawing::Factory gdi;
