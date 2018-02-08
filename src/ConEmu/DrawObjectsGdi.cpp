
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


#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "../common/defines.h"
#include "../common/Memory.h"
#include "../common/MAssert.h"

#include "DrawObjectsGdi.h"

namespace drawing
{
namespace detail
{

// **********************************
//
// FactoryImplGdi
//
// **********************************
FactoryImplGdi::FactoryImplGdi()
{
}

FactoryImplGdi::~FactoryImplGdi()
{
}


bool FactoryImplGdi::Initialize()
{
	// Nothing here yet
	return true;
}

void FactoryImplGdi::Finalize()
{
	// Release internal resources?
}

drawing::Canvas FactoryImplGdi::CreateCanvas(HWND wnd, CanvasType canvas_type)
{
	switch (canvas_type)
	{
	case CanvasType::Window:
		{
			Canvas window_canvas(new CanvasImpl(::GetWindowDC(wnd), wnd, CanvasType::Window));
			window_canvas->SaveState(CanvasSaveState(window_canvas));
			return window_canvas;
		}
	case CanvasType::Client:
		{
			Canvas client_canvas(new CanvasImpl(::GetDC(wnd), wnd, CanvasType::Client));
			client_canvas->SaveState(CanvasSaveState(client_canvas));
			return client_canvas;
		}
	case CanvasType::Paint:
		{
			PAINTSTRUCT ps = {};
			HDC hdc = ::BeginPaint(wnd, &ps);
			Canvas ps_canvas(new CanvasImpl(hdc, wnd, ps));
			ps_canvas->SaveState(CanvasSaveState(ps_canvas));
			return ps_canvas;
		}
	}
	_ASSERTE(FALSE && "Unexpected canvas_type");
	return Canvas();
}

// This may be used when HDC comes from WindowAPI as wParam/lParam in message
drawing::Canvas FactoryImplGdi::CreateCanvasHDC(HDC dc)
{
	Canvas dc_canvas(new CanvasImpl(dc, true/*external*/, CanvasType::Undefined));
	dc_canvas->SaveState(CanvasSaveState(dc_canvas));
	return dc_canvas;
}

drawing::Canvas FactoryImplGdi::CreateCanvasDI(UINT width, UINT height, UINT bits /*= 32*/)
{
	Canvas ScreenDC(CreateCanvas(NULL, CanvasType::Client));

	Canvas mem_dc(new CanvasImpl(::CreateCompatibleDC(ScreenDC.Handle<HDC>()), false, CanvasType::Memory));
	if (mem_dc)
	{
		mem_dc->SaveState(CanvasSaveState(mem_dc));

		// #GDI Implement GdiFlush?
		// You need to guarantee that the GDI subsystem has completed any drawing
		// to a bitmap created by CreateDIBSection before you draw to the bitmap yourself.
		// Access to the bitmap must be synchronized. Do this by calling the GdiFlush function.
		// This applies to any use of the pointer to the bitmap bit values, including passing
		// the pointer in calls to functions such as SetDIBits.

		Bitmap bitmap(CreateBitmap(width, height, bits));
		if (bitmap)
		{
			if (SelectBitmap(mem_dc, bitmap))
				return mem_dc;
			_ASSERTE(FALSE && "SelectBitmap fails");
		}
		else
		{
			_ASSERTE(FALSE && "CreateBitmap fails");
		}
	}
	else
	{
		_ASSERTE(FALSE && "CreateCompatibleDC fails");
	}

	return Canvas();
}

drawing::Canvas FactoryImplGdi::CreateCanvasCompatible(UINT width, UINT height, HWND compatible)
{
	Canvas ScreenDC(CreateCanvas(NULL, CanvasType::Client));

	Canvas comp_dc(new CanvasImpl(::CreateCompatibleDC(ScreenDC.Handle<HDC>()), false, CanvasType::Memory));
	if (compatible)
	{
		comp_dc->SaveState(CanvasSaveState(comp_dc));

		Bitmap bitmap(CreateBitmap(width, height, ScreenDC));
		if (bitmap)
		{
			if (SelectBitmap(comp_dc, bitmap))
				return comp_dc;
			_ASSERTE(FALSE && "SelectBitmap fails");
		}
		else
		{
			_ASSERTE(FALSE && "CreateBitmap fails");
		}
	}
	else
	{
		_ASSERTE(FALSE && "CreateCompatibleDC fails");
	}

	return Canvas();
}

int FactoryImplGdi::CanvasSaveState(const Canvas& canvas)
{
	return ::SaveDC(canvas.Handle<HDC>());
}

void FactoryImplGdi::CanvasRestoreState(const Canvas& canvas, const CanvasState& state)
{
	if (!state.valid())
	{
		_ASSERTE(state.valid());
		return;
	}
	::RestoreDC(canvas.Handle<HDC>(), state.state);
}

void FactoryImplGdi::ReleaseCanvas(Canvas& canvas)
{
	_ASSERTE(canvas);
	if (canvas->isExternal())
		return;

	switch (canvas->getCanvasType())
	{
	case CanvasType::Window:
	case CanvasType::Client:
		::ReleaseDC(canvas->getWindow(), canvas.Handle<HDC>());
		break;

	case CanvasType::Paint:
		::EndPaint(canvas->getWindow(), canvas->getPaintStruct());
		break;

	case CanvasType::Memory:
		::DeleteDC(canvas.Handle<HDC>());
		break;

	default:
		{
			_ASSERTE(FALSE && "Unsupported CanvasType");
		}
	}
}

bool FactoryImplGdi::SelectBrush(const Canvas& canvas, const Brush& brush)
{
	if (!canvas || !brush)
		return false;
	::SelectObject(canvas.Handle<HDC>(), brush.Handle<HBRUSH>());
	return true;
}

bool FactoryImplGdi::SelectPen(const Canvas& canvas, const Pen& pen)
{
	if (!canvas || !pen)
		return false;
	::SelectObject(canvas.Handle<HDC>(), pen.Handle<HPEN>());
	return true;
}

bool FactoryImplGdi::SelectFont(const Canvas& canvas, const Font& font)
{
	if (!canvas || !font)
		return false;
	::SelectObject(canvas.Handle<HDC>(), font.Handle<HFONT>());
	return true;
}

bool FactoryImplGdi::SelectBitmap(const Canvas& canvas, const Bitmap& bitmap)
{
	if (!canvas || !bitmap)
		return false;
	::SelectObject(canvas.Handle<HDC>(), bitmap.Handle<HBITMAP>());
	return true;
}

bool FactoryImplGdi::SetTextColor(const Canvas& canvas, COLORREF clr)
{
	if (!canvas)
		return false;
	::SetTextColor(canvas.Handle<HDC>(), clr);
	return true;
}

bool FactoryImplGdi::SetBkColor(const Canvas& canvas, COLORREF clr)
{
	if (!canvas)
		return false;
	::SetBkColor(canvas.Handle<HDC>(), clr);
	return true;
}

bool FactoryImplGdi::SetBkMode(const Canvas& canvas, BkMode bk_mode)
{
	if (!canvas)
		return false;
	::SetBkMode(canvas.Handle<HDC>(), (bk_mode == BkMode::Opaque) ? OPAQUE : TRANSPARENT);
	return true;
}

bool FactoryImplGdi::TextDraw(const Canvas& canvas, LPCWSTR pszString, int iLen, int x, int y, const RECT *lprc, UINT Flags, const INT *lpDx)
{
	if (!canvas)
		return false;
	if (!::ExtTextOut(canvas.Handle<HDC>(), x, y, Flags, lprc, pszString, iLen, lpDx))
		return false;
	return true;
}

bool FactoryImplGdi::TextDrawOem(const Canvas& canvas, LPCSTR pszString, int iLen, int x, int y, const RECT *lprc, UINT Flags, const INT *lpDx)
{
	if (!canvas)
		return false;
	if (!::ExtTextOutA(canvas.Handle<HDC>(), x, y, Flags, lprc, pszString, iLen, lpDx))
		return false;
	return true;
}

bool FactoryImplGdi::TextExtentPoint(const Canvas& canvas, LPCWSTR pszChars, UINT iLen, SIZE& sz)
{
	if (!canvas)
		return false;
	if (!::GetTextExtentPoint32(canvas.Handle<HDC>(), pszChars, iLen, &sz))
		return false;
	return true;
}

drawing::Rgn FactoryImplGdi::CreateRgn(const RECT& rect)
{
	return Rgn(new RgnImpl(::CreateRectRgnIndirect(&rect), false, rect));
}

void FactoryImplGdi::ReleaseRgn(Rgn& rgn)
{
	::DeleteObject(rgn.Handle<HRGN>());
}

Brush FactoryImplGdi::CreateBrush(COLORREF color)
{
	return Brush(new BrushImpl(::CreateSolidBrush(color), false, color));
}

void FactoryImplGdi::ReleaseBrush(Brush& brush)
{
	::DeleteObject(brush.Handle<HBRUSH>());
}

Pen FactoryImplGdi::CreatePen(COLORREF color, UINT width /*= 1*/)
{
	return Pen(new PenImpl(::CreatePen(PS_SOLID, width, color), false, color, width)); 
}

void FactoryImplGdi::ReleasePen(Pen& pen)
{
	::DeleteObject(pen.Handle<HPEN>());
}

Font FactoryImplGdi::CreateFont(const LOGFONT& font)
{
	return Font(new FontImpl(::CreateFontIndirect(&font), false, font));
}

void FactoryImplGdi::ReleaseFont(Font& font)
{
	::DeleteObject(font.Handle<HFONT>());
}

Bitmap FactoryImplGdi::CreateBitmap(UINT width, UINT height, UINT bits /*= 32*/)
{
	// #GDI Use CreateDIBSection here?
}

Bitmap FactoryImplGdi::CreateBitmap(UINT width, UINT height, const CanvasImpl& compatible)
{
	return Bitmap(new BitmapImpl(::CreateCompatibleBitmap(compatible.Handle<HDC>(), width, height), false, width, height));
}

void FactoryImplGdi::ReleaseBitmap(Bitmap& bitmap)
{
	::DeleteObject(bitmap.Handle<HBITMAP>());
}

bool FactoryImplGdi::IsCompatibleRequired()
{
	static int iVista = 0;
	if (iVista == 0)
	{
		_ASSERTE(_WIN32_WINNT_VISTA==0x600);
		OSVERSIONINFOEXW osvi = {sizeof(osvi), HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA)};
		DWORDLONG const dwlConditionMask = ::VerSetConditionMask(::VerSetConditionMask(0
			, VER_MAJORVERSION, VER_GREATER_EQUAL)
			, VER_MINORVERSION, VER_GREATER_EQUAL);
		iVista = ::VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask) ? 1 : -1;
	}

	int nBPP = -1;
	bool bComp = (iVista < 0);
	if (!bComp)
	{
		Canvas ScreenDC(CreateCanvas(NULL, CanvasType::Client));
		nBPP = ::GetDeviceCaps(ScreenDC.Handle<HDC>(), BITSPIXEL);
		if (nBPP < 32)
		{
			bComp = true;
		}
	}

	return bComp;
}

};
};
