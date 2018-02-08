
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

#include "DrawObjectsImpl.h"

namespace drawing
{
namespace detail
{



// **********************************
//
// ObjectImplBase
//
// **********************************

ObjectImplBase::ObjectImplBase(DrawObjectType _Type, HANDLE _Obj /*= NULL*/, bool _External /*= false*/)
	: Type(_Type)
	, Object((_Obj && (_Obj != INVALID_HANDLE_VALUE)) ? _Obj : NULL)
	, External(_External)
{
}

ObjectImplBase::~ObjectImplBase()
{
	_ASSERTE(Object == NULL);
}

int ObjectImplBase::AddRef() const
{
	if (Counter <= 0)
	{
		_ASSERTE(Counter >= 1); // Object was not created or already disposed
		return 0;
	}
	return ++Counter;
}

int ObjectImplBase::Release() const
{
	return --Counter;
}

bool ObjectImplBase::isExternal() const
{
	return External;
}

void ObjectImplBase::resetObject()
{
	// Object MUST BE already disposed by FactoryImpl!
	Object = NULL;
}

template <class H>
H ObjectImplBase::Handle() const
{
	return (H)Object;
}

DrawObjectType ObjectImplBase::getType() const
{
	return Type;
}

ObjectImplBase::operator bool() const
{
	return (Object && (Object != INVALID_HANDLE_VALUE));
}

bool ObjectImplBase::operator==(const ObjectImplBase& o) const
{
	return (Type == o.Type) && (Object == o.Object);
}

bool ObjectImplBase::operator!=(const ObjectImplBase& o) const
{
	return !(*this == o);
}



// **********************************
//
// CDrawCanvas
//
// **********************************
CanvasImpl::CanvasImpl(HDC _hdc, HWND _hwnd, CanvasType _canvas_type)
	: ObjectImplBase(DrawObjectType::dt_Canvas, _hdc, false)
	, mh_Wnd(_hwnd)
	, m_canvas_type(_canvas_type)
{
	if (_hwnd)
		mn_ThreadID = GetWindowThreadProcessId(_hwnd, nullptr);
	else
		_ASSERTE(_hwnd!=NULL);
}

CanvasImpl::CanvasImpl(HDC _hdc, HWND _hwnd, const PAINTSTRUCT& ps)
	: ObjectImplBase(DrawObjectType::dt_Canvas, _hdc, false)
	, mh_Wnd(_hwnd)
	, m_canvas_type(CanvasType::Paint)
{
	if (_hwnd)
		mn_ThreadID = GetWindowThreadProcessId(_hwnd, nullptr);
	else
		_ASSERTE(_hwnd!=NULL);
}

CanvasImpl::CanvasImpl(HDC _hdc, bool _external, CanvasType _canvas_type)
	: ObjectImplBase(DrawObjectType::dt_Canvas, _hdc, false)
	, m_canvas_type(_canvas_type)
{
	_ASSERTE(_canvas_type == CanvasType::Memory);
}

CanvasType CanvasImpl::getCanvasType() const
{
	return m_canvas_type;
}

const PAINTSTRUCT* CanvasImpl::getPaintStruct() const
{
	_ASSERTE(m_canvas_type == CanvasType::Paint);
	return &m_ps;
}

HWND CanvasImpl::getWindow() const
{
	_ASSERTE(m_canvas_type == CanvasType::Paint || m_canvas_type == CanvasType::Window);
	return mh_Wnd;
}

void CanvasImpl::SaveState(int state)
{
	if (m_prev_state.state != 0)
	{
		_ASSERTE(m_prev_state.state == 0);
		return;
	}
	m_prev_state = m_cur_state;
	m_prev_state.state = state;
}



// **********************************
//
// RgnImpl
//
// **********************************
RgnImpl::RgnImpl(HRGN _hrgn, bool _external, const RECT& _bounding /*= RECT{}*/)
	: ObjectImplBase(DrawObjectType::dt_Rgn, _hrgn, _external)
	, m_Bounding(_bounding)
{
}



// **********************************
//
// BrushImpl
//
// **********************************
BrushImpl::BrushImpl(HBRUSH _hbrush, bool _external, COLORREF _color /*= 0*/)
	: ObjectImplBase(DrawObjectType::dt_Brush, _hbrush, _external)
	, m_Color(_color)
{
}



// **********************************
//
// PenImpl
//
// **********************************
PenImpl::PenImpl(HPEN _hpen, bool _external, COLORREF _color /*= 0*/, UINT _width /*= 1*/)
	: ObjectImplBase(DrawObjectType::dt_Pen, _hpen, _external)
	, m_Color(_color)
	, mn_Width(_width)
{
}



// **********************************
//
// FontImpl
//
// **********************************
FontImpl::FontImpl(HFONT _hfont, bool _external, const LOGFONT& _lf /*= LOGFONT{}*/)
	: ObjectImplBase(DrawObjectType::dt_Font, _hfont, _external)
	, m_LogFont(_lf)
{
}



// **********************************
//
// BitmapImpl
//
// **********************************
BitmapImpl::BitmapImpl(HBITMAP _hbitmap, bool _external, UINT _width /*= 0*/, UINT _height /*= 0*/)
	: ObjectImplBase(DrawObjectType::dt_Bitmap, _hbitmap, _external)
	, mn_Width(_width)
	, mn_Height(_height)
{
}

};
};
