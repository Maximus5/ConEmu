
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

#include <utility>

#include "DrawObjects.h"
#include "DrawObjectsGdi.h"

drawing::Factory gdi;

#define assert_object(obj, fail) \
	if (!obj) { _ASSERTE((bool)obj); return fail; } \
	if (!mp_gdi) { _ASSERTE(mp_gdi!=nullptr); return fail; }
#define assert_impl(nil) \
	if (!mp_gdi) { _ASSERTE(mp_gdi!=nullptr); return nil; }
#define assert_impl_void() \
	if (!mp_gdi) { _ASSERTE(mp_gdi!=nullptr); return; }

namespace drawing
{

namespace detail
{
// **********************************
//
// ObjectPtr
//
// **********************************

template<class T>
ObjectPtr<T>::~ObjectPtr()
{
	reset(nullptr);
}

template<class T>
ObjectPtr<T>::ObjectPtr()
{
	_ASSERTE(object == nullptr);
}

template<class T>
ObjectPtr<T>::ObjectPtr(T*&& _obj)
{
	_ASSERTE(object == nullptr);
	reset(std::move(_obj));
}

template<typename T>
ObjectPtr<T>::ObjectPtr(const ObjectPtr<T>& _src)
{
	_ASSERTE(object == nullptr);
	if (_src.object && _src.object->AddRef() > 0)
	{
		object = _src.object;
	}
}

template<typename T>
ObjectPtr<T>::ObjectPtr(ObjectPtr<T>&& _src)
{
	_ASSERTE(object == nullptr);
	std::swap(object, _src.object);
}

template<typename T>
ObjectPtr<T>& ObjectPtr<T>::operator=(const ObjectPtr<T>& _src)
{
	if (object != _src.object)
	{
		reset(nullptr);

		if (_src.object && _src.object->AddRef() > 0)
		{
			object = _src.object;
		}
	}
	return *this;
}

template<typename T>
ObjectPtr<T>& ObjectPtr<T>::operator=(ObjectPtr<T>&& _src)
{
	if (object != _src.object)
	{
		reset(nullptr);

		std::swap(object, _src.object);
	}
}

template<typename T>
void ObjectPtr<T>::reset(T*&& _obj)
{
	if (_obj != object)
	{
		if (object)
		{
			if (object->Release() <= 0)
			{
				gdi.ReleaseObject(*this);
				delete object;
			}
		}

		object = _obj;
	}
}

template<class T>
ObjectPtr<T>::operator const T&() const
{
	if (!object)
		throw "Object was not initialized";
	return *object;
}

template<class T>
T* ObjectPtr<T>::operator->() const
{
	_ASSERTE(object != nullptr);
	return object;
}

template<class T>
T* ObjectPtr<T>::get() const
{
	return object;
}

template<class T>
ObjectPtr<T>::operator bool() const
{
	return (object != nullptr) && (object->operator bool());
}

template<class T>
bool ObjectPtr<T>::operator!() const
{
	return !(this->operator bool());
}

template<class T>
template<typename H>
H ObjectPtr<T>::Handle() const
{
	return object ? object->Handle<H>() : NULL;
}

template<typename T>
bool ObjectPtr<T>::operator==(const ObjectPtr<T>& _obj) const
{
	if (this->operator bool() != _obj.operator bool())
		return false;

	if (!object)
	{
		_ASSERTE(_obj.object == nullptr);
		return true;
	}

	_ASSERTE(_obj.object != nullptr);

	return (*object == *_obj.object);
}

template<typename T>
bool ObjectPtr<T>::operator!=(const ObjectPtr<T>& _obj) const
{
	return !(*this == _obj);
}

};


/* ********************************************************* */



/* ********************************************************* */

Factory::Factory()
{
	#ifdef _DEBUG
	detail::ObjectPtr<detail::BrushImpl> b(new detail::BrushImpl((HBRUSH)1, true));
	HBRUSH h = b.Handle<HBRUSH>();
	{
		detail::ObjectPtr<detail::BrushImpl> b1 = b;
		(b1 == b);
		(b1 != b);
	}
	const detail::BrushImpl& bp = b;
	b.reset(new detail::BrushImpl((HBRUSH)2, true));
	#endif
}

bool Factory::Initialize()
{
	if (mp_gdi)
	{
		_ASSERTE(FALSE && "Already initialized!");
		return false;
	}

	mp_gdi = new detail::FactoryImplGdi();
	return mp_gdi->Initialize();
}


drawing::Canvas Factory::CreateCanvas(HWND wnd, CanvasType canvas_type)
{
	assert_impl(drawing::Canvas());
	return mp_gdi->CreateCanvas(wnd, canvas_type);
}

drawing::Canvas Factory::CreateCanvasHDC(HDC dc)
{
	assert_impl(drawing::Canvas());
	return mp_gdi->CreateCanvasHDC(dc);
}

drawing::Canvas Factory::CreateCanvasDI(UINT width, UINT height, UINT bits /*= 32*/)
{
	assert_impl(drawing::Canvas());
	return mp_gdi->CreateCanvasDI(width, height, bits);
}

drawing::Canvas Factory::CreateCanvasCompatible(UINT width, UINT height, HWND compatible)
{
	assert_impl(Canvas());
	return mp_gdi->CreateCanvasCompatible(width, height, compatible);
}

drawing::CanvasState Factory::CanvasSaveState(const Canvas& canvas)
{
	assert_impl(CanvasState{});
	CanvasState state = canvas->m_cur_state;
	state.state = mp_gdi->CanvasSaveState(canvas);
	return state;
}

void Factory::CanvasRestoreState(const Canvas& canvas, const CanvasState& state)
{
	assert_impl_void();
	mp_gdi->CanvasRestoreState(canvas, state);
	canvas->m_cur_state = state;
}

bool Factory::SelectBrush(const drawing::Canvas& canvas, const drawing::Brush& brush)
{
	assert_object(canvas,false);
	HANDLE prev_brush = NULL;
	if (!mp_gdi->SelectBrush(canvas, brush))
		return false;
	return true;
}

bool Factory::SelectPen(const drawing::Canvas& canvas, const drawing::Pen& pen)
{
	assert_object(canvas,false);
	if (!mp_gdi->SelectPen(canvas, pen))
		return false;
	return true;
}

bool Factory::SelectFont(const drawing::Canvas& canvas, const drawing::Font& font)
{
	assert_object(canvas,false);
	if (!mp_gdi->SelectFont(canvas, font))
		return false;
	return true;
}

bool Factory::SelectBitmap(const drawing::Canvas& canvas, const drawing::Bitmap& bitmap)
{
	assert_object(canvas,false);
	if (!mp_gdi->SelectBitmap(canvas, bitmap))
		return false;
	return true;
}

bool Factory::SetTextColor(const drawing::Canvas& canvas, COLORREF clr)
{
	assert_object(canvas,false);
	return mp_gdi->SetTextColor(canvas, clr);
}

bool Factory::SetBkColor(const drawing::Canvas& canvas, COLORREF clr)
{
	assert_object(canvas,false);
	return mp_gdi->SetBkColor(canvas, clr);
}

bool Factory::SetBkMode(const drawing::Canvas& canvas, drawing::BkMode bk_mode)
{
	assert_object(canvas,false);
	return mp_gdi->SetBkMode(canvas, bk_mode);
}

bool Factory::TextDraw(const drawing::Canvas& canvas, LPCWSTR pszString, int iLen, int x, int y, const RECT *lprc, UINT Flags, const INT *lpDx)
{
	assert_object(canvas,false);
	return mp_gdi->TextDraw(canvas, pszString, iLen, x, y, lprc, Flags, lpDx);
}

bool Factory::TextDrawOem(const drawing::Canvas& canvas, LPCSTR pszString, int iLen, int x, int y, const RECT *lprc, UINT Flags, const INT *lpDx)
{
	assert_object(canvas,false);
	return mp_gdi->TextDrawOem(canvas, pszString, iLen, x, y, lprc, Flags, lpDx);
}

bool Factory::TextExtentPoint(const drawing::Canvas& canvas, LPCWSTR pszChars, UINT iLen, SIZE& sz)
{
	assert_object(canvas,false);
	return mp_gdi->TextExtentPoint(canvas, pszChars, iLen, sz);
}

drawing::Rgn Factory::CreateRgn(const RECT& rect)
{
	assert_impl(drawing::Rgn());
	return mp_gdi->CreateRgn(rect);
}

drawing::Brush Factory::CreateBrush(COLORREF color)
{
	assert_impl(drawing::Brush());
	return mp_gdi->CreateBrush(color);
}

drawing::Pen Factory::CreatePen(COLORREF color, UINT width /*= 1*/)
{
	assert_impl(drawing::Pen());
	return mp_gdi->CreatePen(color, width);
}

drawing::Font Factory::CreateFont(const LOGFONT& font)
{
	assert_impl(drawing::Font());
	return mp_gdi->CreateFont(font);
}

drawing::Bitmap Factory::CreateBitmap(UINT width, UINT height, UINT bits /*= 32*/)
{
	assert_impl(drawing::Bitmap());
	return mp_gdi->CreateBitmap(width, height, bits);
}

drawing::Bitmap Factory::CreateBitmap(UINT width, UINT height, const Canvas& compatible)
{
	assert_impl(drawing::Bitmap());
	return mp_gdi->CreateBitmap(width, height, compatible);
}

bool Factory::IsCompatibleRequired()
{
	assert_impl(false);
	return mp_gdi->IsCompatibleRequired();
}

/* **** Release **** */
void Factory::ReleaseBitmap(drawing::Bitmap& bitmap)
{
	assert_impl_void();
	if (bitmap)
	{
		if (!bitmap->isExternal())
			mp_gdi->ReleaseBitmap(bitmap);
		bitmap->resetObject();
	}
}

void Factory::ReleaseBrush(drawing::Brush& brush)
{
	assert_impl_void();
	if (brush)
	{
		if (!brush->isExternal())
			mp_gdi->ReleaseBrush(brush);
		brush->resetObject();
	}
}

void Factory::ReleaseCanvas(drawing::Canvas& canvas)
{
	assert_impl_void();
	if (canvas)
	{
		if (canvas->m_prev_state.valid())
		{
			CanvasRestoreState(canvas, canvas->m_prev_state);
		}

		canvas->m_cur_state = canvas->m_prev_state;

		canvas->m_prev_state = CanvasState();

		if (!canvas->isExternal())
			mp_gdi->ReleaseCanvas(canvas);
		canvas->resetObject();
	}
}

void Factory::ReleaseFont(drawing::Font& font)
{
	assert_impl_void();
	if (font)
	{
		if (!font->isExternal())
			mp_gdi->ReleaseFont(font);
		font->resetObject();
	}
}

void Factory::ReleasePen(drawing::Pen& pen)
{
	assert_impl_void();
	if (pen)
	{
		if (!pen->isExternal())
			mp_gdi->ReleasePen(pen);
		pen->resetObject();
	}
}

void Factory::ReleaseRgn(drawing::Rgn& rgn)
{
	assert_impl_void();
	if (rgn)
	{
		if (!rgn->isExternal())
			mp_gdi->ReleaseRgn(rgn);
		rgn->resetObject();
	}
}

};
