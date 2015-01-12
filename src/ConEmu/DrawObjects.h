
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
#include "../common/defines.h"
#include "RefRelease.h"

enum DrawObjectType
{
	dt_None = 0,
	dt_Canvas,
	dt_Brush,
	dt_Pen,
	dt_Font,
	dt_Bitmap,
};

class CDrawCanvas;
class CDrawObjectBrush;
class CDrawObjectPen;
class CDrawObjectFont;
class CDrawObjectBitmap;



class CDrawFactory
{
public:
	// Canvas
	virtual CDrawCanvas* NewCanvas(HWND Wnd) = 0;
	virtual CDrawCanvas* NewCanvasHDC(HDC Dc) = 0;
	virtual CDrawCanvas* NewCanvasDI(UINT Width, UINT Height, UINT Bits = 32) = 0;
	virtual CDrawCanvas* NewCanvasCompatible(UINT Width, UINT Height, HWND Compatible) = 0;
	// Brush
	virtual CDrawObjectBrush* NewBrush(COLORREF Color) = 0;
	// Pen
	virtual CDrawObjectPen* NewPen(COLORREF Color, UINT Width = 1) = 0;
	// Font
	virtual CDrawObjectFont* NewFont(const LOGFONT& Font) = 0;
	// Bitmap
	virtual CDrawObjectBitmap* NewBitmap(UINT Width, UINT Height, CDrawCanvas* Compatible = NULL) = 0;
public:
	// Features
	virtual bool IsCompatibleRequired() = 0;
};



class CDrawObjectBase : public CRefRelease
{
protected:
	DrawObjectType m_Type;
	bool           mb_External;
	HANDLE         mh_Object;
	CDrawFactory*  mp_Factory;

public:
	CDrawObjectBase(CDrawFactory* pFactory, DrawObjectType Type, HANDLE Obj = NULL, bool External = false);
protected:
	virtual ~CDrawObjectBase();
protected:
	// May be overrided
	virtual HANDLE Set(HANDLE Obj);
	// Must be overrided
	virtual void InternalFree() = 0;
protected:
	// Implements CRefRelease: calls Free() and delete(this)
	virtual void FinalRelease() override;
	// Calls InternalFree, nulls mh_Object
	void Free();
};



class CDrawCanvas : public CDrawObjectBase
{
protected:
	HWND   mh_Wnd;
	DWORD  mn_ThreadID;
	bool   mb_WindowDC;
protected:
	HANDLE mh_OldBrush, mh_OldPen, mh_OldFont;

	CDrawObjectBrush  *mp_Brush;
	CDrawObjectPen    *mp_Pen;
	CDrawObjectFont   *mp_Font;

protected:
	HANDLE mh_OldBitmap;
	CDrawObjectBitmap *mp_Bitmap;
	bool mb_SelfBitmap;

public:
	CDrawCanvas(CDrawFactory* pFactory);

public:
	// Initialization routines: have to be implemented in descendants
	virtual bool InitCanvas(HWND Wnd) = 0;
	virtual bool InitCanvasHDC(HDC Dc) = 0;
	virtual bool InitCanvasDI(UINT Width, UINT Height, UINT Bits = 32) = 0;
	virtual bool InitCanvasCompatible(UINT Width, UINT Height, HWND Compatible) = 0;

protected:
	virtual void InternalFree() override;
	virtual void InternalFreeCanvas() = 0;

public:
	// Helpers: if NULL passes as parameter - selects "original" object
	virtual void SelectBrush(CDrawObjectBrush* Brush) = 0;
	virtual void SelectPen(CDrawObjectPen* Pen) = 0;
	virtual void SelectFont(CDrawObjectFont* Font) = 0;
	virtual void SelectBitmap(CDrawObjectBitmap* Bitmap) = 0;

public:
	// Helpers for painting objects (like checkboxes, radio buttons), drawing text, blitting bitmaps and so on..
	#if 0
	virtual bool DrawButtonCheck(int x, int y, int cx, int cy, DWORD DFCS_Flags) = 0;
	virtual bool DrawButtonRadio(int x, int y, int cx, int cy, DWORD DFCS_Flags) = 0;
	#endif
public:
	// Text
	virtual void SetTextColor(COLORREF clr) = 0;
	virtual void SetBkColor(COLORREF clr) = 0;
	virtual void SetBkMode(int iBkMode) = 0;
	virtual bool TextDraw(LPCWSTR pszString, int iLen, int x, int y, const RECT *lprc, UINT Flags, const INT *lpDx) = 0;
	virtual bool TextDrawOem(LPCSTR pszString, int iLen, int x, int y, const RECT *lprc, UINT Flags, const INT *lpDx) = 0;
	virtual bool TextExtentPoint(LPCWSTR pszChars, UINT iLen, LPSIZE sz) = 0;
};



class CDrawObjectBrush : public CDrawObjectBase
{
protected:
	COLORREF m_Color;
public:
	CDrawObjectBrush(CDrawFactory* pFactory);
};



class CDrawObjectPen : public CDrawObjectBase
{
protected:
	COLORREF m_Color;
	UINT mn_Width;
public:
	CDrawObjectPen(CDrawFactory* pFactory);
};



class CDrawObjectFont : public CDrawObjectBase
{
protected:
	LOGFONT m_LogFont;
public:
	CDrawObjectFont(CDrawFactory* pFactory);
};



class CDrawObjectBitmap : public CDrawObjectBase
{
protected:
	UINT mn_Width, mn_Height;
public:
	CDrawObjectBitmap(CDrawFactory* pFactory);
};



template<class _Ty>
class CDrawObjectPtr
{
private:
	CRefRelease *mp_Ref;

	void Set(_Ty* apRef)
	{
		if (mp_Ref != apRef)
		{
			if (apRef)
				apRef->AddRef();
			SafeRelease(mp_Ref);
			mp_Ref = apRef;
		}
	};

public:
	CDrawObjectPtr()
		: mp_Ref(NULL)
	{
	};

	CDrawObjectPtr(_Ty* apRef)
		: mp_Ref(NULL)
	{
		Set(apRef);
	};

	~CDrawObjectPtr()
	{
		SafeRelease(mp_Ref);
	};

	operator _Ty*() const
	{
		return dynamic_cast<_Ty*>(mp_Ref);
	};

	_Ty* operator->()
	{
		return dynamic_cast<_Ty*>(mp_Ref);
	};

	CDrawObjectPtr<_Ty>& operator=(_Ty* apRef)
	{
		Set(apRef);
		return *this;
	};
};

typedef CDrawObjectPtr<CDrawCanvas> CCanvasPtr;
typedef CDrawObjectPtr<CDrawObjectBrush> CBrushPtr;
typedef CDrawObjectPtr<CDrawObjectPen> CPenPtr;
typedef CDrawObjectPtr<CDrawObjectFont> CFontPtr;
typedef CDrawObjectPtr<CDrawObjectBitmap> CBitmapPtr;
