
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

#include "DrawObjectsGdi.h"

// **********************************
//
// CDrawFactoryGdi
//
// **********************************
CDrawFactoryGdi::CDrawFactoryGdi()
{
}

CDrawFactoryGdi::~CDrawFactoryGdi()
{
}

CDrawCanvas* CDrawFactoryGdi::NewCanvas(HWND Wnd)
{
	CDrawCanvasGdi* pCanvas = new CDrawCanvasGdi(this);
	if (!pCanvas->InitCanvas(Wnd))
		SafeRelease(pCanvas);
	return pCanvas;
}

CDrawCanvas* CDrawFactoryGdi::NewCanvasHDC(HDC Dc)
{
	CDrawCanvasGdi* pCanvas = new CDrawCanvasGdi(this);
	if (!pCanvas->InitCanvasHDC(Dc))
		SafeRelease(pCanvas);
	return pCanvas;
}

CDrawCanvas* CDrawFactoryGdi::NewCanvasDI(UINT Width, UINT Height, UINT Bits /*= 32*/)
{
	CDrawCanvasGdi* pCanvas = new CDrawCanvasGdi(this);
	if (!pCanvas->InitCanvasDI(Width, Height, Bits))
		SafeRelease(pCanvas);
	return pCanvas;
}

CDrawCanvas* CDrawFactoryGdi::NewCanvasCompatible(UINT Width, UINT Height, HWND Compatible)
{
	CDrawCanvasGdi* pCanvas = new CDrawCanvasGdi(this);
	if (!pCanvas->InitCanvasCompatible(Width, Height, Compatible))
		SafeRelease(pCanvas);
	return pCanvas;
}

CDrawObjectBrush* CDrawFactoryGdi::NewBrush(COLORREF Color)
{
	CDrawObjectBrushGdi* pBrush = new CDrawObjectBrushGdi(this);
	if (!pBrush->InitBrush(Color))
		SafeRelease(pBrush);
	return pBrush;
}

CDrawObjectPen* CDrawFactoryGdi::NewPen(COLORREF Color, UINT Width /*= 1*/)
{
	CDrawObjectPenGdi* pPen = new CDrawObjectPenGdi(this);
	if (!pPen->InitPen(Color, Width))
		SafeRelease(pPen);
	return pPen;
}

CDrawObjectFont* CDrawFactoryGdi::NewFont(const LOGFONT& Font)
{
	CDrawObjectFontGdi* pFont = new CDrawObjectFontGdi(this);
	if (!pFont->InitFont(Font))
		SafeRelease(pFont);
	return pFont;
}

CDrawObjectBitmap* CDrawFactoryGdi::NewBitmap(UINT Width, UINT Height, CDrawCanvas* Compatible /*= NULL*/)
{
	CDrawObjectBitmapGdi* pBitmap = new CDrawObjectBitmapGdi(this);
	if (!pBitmap->InitBitmap(Width, Height))
		SafeRelease(pBitmap);
	return pBitmap;
}

bool CDrawFactoryGdi::IsCompatibleRequired()
{
	static int iVista = 0;
	if (iVista == 0)
	{
		_ASSERTE(_WIN32_WINNT_VISTA==0x600);
		OSVERSIONINFOEXW osvi = {sizeof(osvi), HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA)};
		DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);
		iVista = VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask) ? 1 : -1;
	}

	int nBPP = -1;
	bool bComp = (iVista < 0);
	if (!bComp)
	{
		CDrawCanvasGdi ScreenDC(this);
		ScreenDC.InitCanvas(NULL);
		nBPP = GetDeviceCaps(ScreenDC.operator HDC(), BITSPIXEL);
		if (nBPP < 32)
		{
			bComp = true;
		}
	}

	return bComp;
}



// **********************************
//
// CDrawObjectCanvas
//
// **********************************
CDrawCanvasGdi::CDrawCanvasGdi(CDrawFactory* pFactory)
	: CDrawCanvas(pFactory)
{
}

bool CDrawCanvasGdi::InitCanvas(HWND Wnd)
{
	Free();
	mb_WindowDC = true;
	mb_External = false;
	mh_Object = GetDC(Wnd);
	return (mh_Object!=NULL);
}

bool CDrawCanvasGdi::InitCanvasHDC(HDC Dc)
{
	Free();
	mb_WindowDC = false; // doesn't matter, actually
	mb_External = true;
	mh_Object = Dc;
	return (mh_Object!=NULL);
}

bool CDrawCanvasGdi::InitCanvasDI(UINT Width, UINT Height, UINT Bits /*= 32*/)
{
	Free();
	mb_WindowDC = false;
	mb_External = false;

	_ASSERTE(mp_Bitmap == NULL);
	SafeRelease(mp_Bitmap);

	CDrawCanvasGdi ScreenDC(mp_Factory);

	if (!mp_Factory->IsCompatibleRequired() || ScreenDC.InitCanvas(NULL))
	{
		mh_Object = CreateCompatibleDC(ScreenDC);

		mp_Bitmap = mp_Factory->NewBitmap(Width, Height, &ScreenDC);
	}

	return (mh_Object!=NULL);
}

bool CDrawCanvasGdi::InitCanvasCompatible(UINT Width, UINT Height, HWND Compatible)
{
	Free();
	mb_WindowDC = false;
	mb_External = false;

	_ASSERTE(mp_Bitmap == NULL);
	SafeRelease(mp_Bitmap);

	CDrawCanvasGdi ScreenDC(mp_Factory);

	if (ScreenDC.InitCanvas(Compatible))
	{
		mh_Object = CreateCompatibleDC(ScreenDC.operator HDC());

		if ((mp_Bitmap = mp_Factory->NewBitmap(Width, Height, &ScreenDC)) != NULL)
			SelectBitmap(mp_Bitmap);
		else
			Free();
	}

	return (mh_Object!=NULL);
}

CDrawCanvasGdi::operator HDC() const
{
	return (HDC)mh_Object;
}

void CDrawCanvasGdi::InternalFreeCanvas()
{
	if (mh_Object && !mb_External)
	{
		if (mb_WindowDC)
			ReleaseDC(mh_Wnd, (HDC)mh_Object);
		else
			DeleteDC((HDC)mh_Object);
	}
}

void CDrawCanvasGdi::SelectBrush(CDrawObjectBrush* Brush)
{
	HDC hdc = operator HDC();
	if (!hdc)
		return;

	if (Brush)
	{
		HBRUSH hOld = (HBRUSH)SelectObject(hdc, dynamic_cast<CDrawObjectBrushGdi*>(Brush)->operator HBRUSH());
		if (!mh_OldBrush)
			mh_OldBrush = hOld;
		mp_Brush = Brush;
	}
	else if (mh_OldBrush)
	{
		SelectObject(hdc, (HBRUSH)mh_OldBrush);
		mh_OldBrush = NULL;
	}
}

void CDrawCanvasGdi::SelectPen(CDrawObjectPen* Pen)
{
	HDC hdc = operator HDC();
	if (!hdc)
		return;

	if (Pen)
	{
		HPEN hOld = (HPEN)SelectObject(hdc, dynamic_cast<CDrawObjectPenGdi*>(Pen)->operator HPEN());
		if (!mh_OldPen)
			mh_OldPen = hOld;
		mp_Pen = Pen;
	}
	else if (mh_OldPen)
	{
		SelectObject(hdc, (HPEN)mh_OldPen);
		mh_OldPen = NULL;
	}
}

void CDrawCanvasGdi::SelectFont(CDrawObjectFont* Font)
{
	HDC hdc = operator HDC();
	if (!hdc)
		return;

	if (Font)
	{
		HFONT hOld = (HFONT)SelectObject(hdc, dynamic_cast<CDrawObjectFontGdi*>(Font)->operator HFONT());
		if (!mh_OldFont)
			mh_OldFont = hOld;
		mp_Font = Font;
	}
	else if (mh_OldFont)
	{
		SelectObject(hdc, (HFONT)mh_OldFont);
		mh_OldFont = NULL;
	}
}

void CDrawCanvasGdi::SelectBitmap(CDrawObjectBitmap* Bitmap)
{
	HDC hdc = operator HDC();
	if (!hdc)
		return;

	if (Bitmap)
	{
		HFONT hOld = (HFONT)SelectObject(hdc, dynamic_cast<CDrawObjectBitmapGdi*>(Bitmap)->operator HBITMAP());
		if (!mh_OldBitmap)
			mh_OldBitmap = hOld;
		if (mp_Bitmap == Bitmap)
			mb_SelfBitmap = true;
		else
			mp_Bitmap = Bitmap;
	}
	else if (mh_OldBitmap)
	{
		SelectObject(hdc, (HFONT)mh_OldBitmap);
		mh_OldBitmap = NULL;
	}
}

#if 0
bool CDrawCanvasGdi::DrawButtonCheck(int x, int y, int cx, int cy, DWORD DFCS_Flags)
{
	return false;
}

bool CDrawCanvasGdi::DrawButtonRadio(int x, int y, int cx, int cy, DWORD DFCS_Flags)
{
	return false;
}
#endif

void CDrawCanvasGdi::SetTextColor(COLORREF clr)
{
	::SetTextColor(operator HDC(), clr);
}

void CDrawCanvasGdi::SetBkColor(COLORREF clr)
{
	::SetBkColor(operator HDC(), clr);
}

bool CDrawCanvasGdi::TextDraw(LPCWSTR pszString, int iLen, int x, int y, const RECT *lprc, UINT Flags, const INT *lpDx)
{
	bool bRc = false;
	if (iLen < 0)
		iLen = lstrlen(pszString);
	if (::ExtTextOut(operator HDC(), x, y, Flags, lprc, pszString, iLen, lpDx))
		bRc = true;
	return bRc;
}

bool CDrawCanvasGdi::TextDrawOem(LPCSTR pszString, int iLen, int x, int y, const RECT *lprc, UINT Flags, const INT *lpDx)
{
	bool bRc = false;
	if (iLen < 0)
		iLen = lstrlenA(pszString);
	if (::ExtTextOutA(operator HDC(), x, y, Flags, lprc, pszString, iLen, lpDx))
		bRc = true;
	return bRc;
}

bool CDrawCanvasGdi::TextExtentPoint(LPCWSTR pszChars, UINT iLen, LPSIZE sz)
{
	bool bRc = false;
	if (::GetTextExtentPoint32(operator HDC(), pszChars, iLen, sz))
		bRc = true;
	return bRc;
}



// **********************************
//
// CDrawObjectBrushGdi
//
// **********************************
CDrawObjectBrushGdi::CDrawObjectBrushGdi(CDrawFactory* pFactory)
	: CDrawObjectBrush(pFactory)
{
}

bool CDrawObjectBrushGdi::InitBrush(COLORREF Color)
{
	if (mh_Object)
		Free();
	mh_Object = CreateSolidBrush(Color);
	_ASSERTE(mh_Object && (GetObjectType(mh_Object) == OBJ_BRUSH));
	return (mh_Object!=NULL);
}

CDrawObjectBrushGdi::operator HBRUSH() const
{
	_ASSERTE(mh_Object && (GetObjectType(mh_Object) == OBJ_BRUSH));
	return (HBRUSH)mh_Object;
}

void CDrawObjectBrushGdi::InternalFree()
{
	if (mh_Object)
		DeleteObject((HBRUSH)mh_Object);
	mh_Object = NULL;
}


// **********************************
//
// CDrawObjectPenGdi
//
// **********************************
CDrawObjectPenGdi::CDrawObjectPenGdi(CDrawFactory* pFactory)
	: CDrawObjectPen(pFactory)
{
}

bool CDrawObjectPenGdi::InitPen(COLORREF Color, UINT Width /*= 1*/)
{
	if (mh_Object)
		Free();
	mh_Object = CreatePen(PS_SOLID, Width, Color);
	_ASSERTE(mh_Object && (GetObjectType(mh_Object) == OBJ_PEN));
	return (mh_Object!=NULL);
}

CDrawObjectPenGdi::operator HPEN() const
{
	return (HPEN)mh_Object;
}

void CDrawObjectPenGdi::InternalFree()
{
	if (mh_Object)
		DeleteObject((HPEN)mh_Object);
	mh_Object = NULL;
}


// **********************************
//
// CDrawObjectFontGdi
//
// **********************************
CDrawObjectFontGdi::CDrawObjectFontGdi(CDrawFactory* pFactory)
	: CDrawObjectFont(pFactory)
{
}

bool CDrawObjectFontGdi::InitFont(const LOGFONT& Font)
{
	if (mh_Object)
		Free();
	TODO("BDF Fonts");
	mh_Object = CreateFontIndirect(&Font);
	_ASSERTE(mh_Object && (GetObjectType(mh_Object) == OBJ_FONT));
	return (mh_Object!=NULL);
}

CDrawObjectFontGdi::operator HFONT() const
{
	return (HFONT)mh_Object;
}

void CDrawObjectFontGdi::InternalFree()
{
	if (mh_Object)
		DeleteObject((HFONT)mh_Object);
	mh_Object = NULL;
}


// **********************************
//
// CDrawObjectBitmapGdi
//
// **********************************
CDrawObjectBitmapGdi::CDrawObjectBitmapGdi(CDrawFactory* pFactory)
	: CDrawObjectBitmap(pFactory)
{
}

bool CDrawObjectBitmapGdi::InitBitmap(UINT Width, UINT Height, CDrawCanvas* Compatible /*= NULL*/)
{
	if (mh_Object)
		Free();
	if (Compatible)
		mh_Object = CreateCompatibleBitmap(dynamic_cast<CDrawCanvasGdi*>(Compatible)->operator HDC(), Width, Height);
	else
		mh_Object = CreateBitmap(Width, Height, 1, 32, NULL);
	_ASSERTE(mh_Object && (GetObjectType(mh_Object) == OBJ_BITMAP));
	return (mh_Object!=NULL);
}

CDrawObjectBitmapGdi::operator HBITMAP() const
{
	return (HBITMAP)mh_Object;
}

void CDrawObjectBitmapGdi::InternalFree()
{
	if (mh_Object)
		DeleteObject((HBITMAP)mh_Object);
	mh_Object = NULL;
}
