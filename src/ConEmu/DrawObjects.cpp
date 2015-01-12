
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


#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "../common/defines.h"
#include "../common/Memory.h"
#include "../common/MAssert.h"

#include "DrawObjects.h"



// **********************************
//
// CDrawObjectBase
//
// **********************************

CDrawObjectBase::CDrawObjectBase(CDrawFactory* pFactory, DrawObjectType Type, HANDLE Obj /*= NULL*/, bool External /*= false*/)
	: m_Type(Type)
	, mh_Object(Obj)
	, mb_External(External)
	, mp_Factory(pFactory)
{
}

void CDrawObjectBase::Free()
{
	if (mh_Object)
		InternalFree();
	mh_Object = NULL;
	mb_External = false;

	// We DON'T reset m_Type here!
	// We need to know ‘object type’ while object is valid
	// -- m_Type = dt_None;
}

void CDrawObjectBase::FinalRelease()
{
	InternalFree();
	_ASSERTE(mh_Object==NULL);

	CDrawObjectBase* pObject = this;
	delete pObject;
}

CDrawObjectBase::~CDrawObjectBase()
{
	InternalFree();
}

HANDLE CDrawObjectBase::Set(HANDLE Obj)
{
	if (mh_Object != Obj)
	{
		InternalFree();
		mh_Object = Obj;
	}
	return mh_Object;
}



// **********************************
//
// CDrawCanvas
//
// **********************************
CDrawCanvas::CDrawCanvas(CDrawFactory* pFactory)
	: CDrawObjectBase(pFactory, dt_Canvas)
	, mh_Wnd(NULL)
	, mn_ThreadID(GetCurrentThreadId())
	, mb_WindowDC(false)
	, mh_OldBrush(NULL), mh_OldPen(NULL), mh_OldFont(NULL)
	, mp_Brush(NULL), mp_Pen(NULL), mp_Font(NULL)
	, mh_OldBitmap(NULL)
	, mp_Bitmap(NULL)
	, mb_SelfBitmap(false)
{
}

void CDrawCanvas::InternalFree()
{
	if (mh_Object)
	{
		// if NULL passes as parameter - selects "original" object
		if (mh_OldBrush)
			this->SelectBrush(NULL);
		if (mh_OldPen)
			this->SelectPen(NULL);
		if (mh_OldFont)
			this->SelectFont(NULL);
		if (mh_OldBitmap)
			this->SelectBitmap(NULL);
	}

	if (mb_SelfBitmap && mp_Bitmap)
		SafeRelease(mp_Bitmap);

	InternalFreeCanvas();

	mh_Object = NULL;
	mh_Wnd = NULL;
	mb_SelfBitmap = false;
	mb_External = false;
}



// **********************************
//
// CDrawCanvas
//
// **********************************
CDrawObjectBrush::CDrawObjectBrush(CDrawFactory* pFactory)
	: CDrawObjectBase(pFactory, dt_Brush)
	, m_Color(0)
{
}



// **********************************
//
// CDrawObjectPen
//
// **********************************
CDrawObjectPen::CDrawObjectPen(CDrawFactory* pFactory)
	: CDrawObjectBase(pFactory, dt_Pen)
	, m_Color(0)
	, mn_Width(1)
{
}



// **********************************
//
// CDrawObjectFont
//
// **********************************
CDrawObjectFont::CDrawObjectFont(CDrawFactory* pFactory)
	: CDrawObjectBase(pFactory, dt_Font)
	, m_LogFont()
{
}



// **********************************
//
// CDrawObjectBitmap
//
// **********************************
CDrawObjectBitmap::CDrawObjectBitmap(CDrawFactory* pFactory)
	: CDrawObjectBase(pFactory, dt_Bitmap)
	, mn_Width(0)
	, mn_Height(0)
{
}
