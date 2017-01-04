
/*
Copyright (c) 2016-2017 Maximus5
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

#include "Header.h"

#include "Font.h"
#include "FontMgr.h"
#include "FontPtr.h"

bool operator== (const CFont &a, const CFont &b)
{
	return a.Equal(&b);
}

bool operator!= (const CFont &a, const CFont &b)
{
	return !(a.Equal(&b));
}

CFont::CFont()
	: iType(CEFONT_NONE)
	, pData(NULL)
	, mb_Monospace(false)
	, mb_RasterFont(false)
	, mp_otm(NULL)
{
	ZeroStruct(m_tm);

	gpFontMgr->FontPtrRegister(this);
}

CFont::CFont(HFONT ahGdiFont)
	: iType(CEFONT_GDI)
	, hFont(ahGdiFont)
	, mb_Monospace(false)
	, mb_RasterFont(false)
	, mp_otm(NULL)
{
	ZeroStruct(m_tm);

	gpFontMgr->FontPtrRegister(this);
}

CFont::CFont(CustomFont* apFont)
	: iType(CEFONT_CUSTOM)
	, pCustomFont(apFont)
	, mb_Monospace(true) // *.bdf are always treated as monospace
	, mb_RasterFont(false)
	, mp_otm(NULL)
{
	ZeroStruct(m_tm);

	gpFontMgr->FontPtrRegister(this);
}

CFont::~CFont()
{
	gpFontMgr->FontPtrUnregister(this);
	Delete();
	SafeFree(mp_otm);
}

void CFont::FinalRelease()
{
	CFont* p = this;
	delete p;
}

bool CFont::Equal(const CFont* p) const
{
	if (!p || (iType != p->iType))
		return false;
	switch (iType)
	{
	case CEFONT_GDI:
		return (hFont == p->hFont);
	case CEFONT_CUSTOM:
		return (pCustomFont == p->pCustomFont);
	}
	_ASSERT(iType==CEFONT_GDI || iType==CEFONT_CUSTOM);
	return false;
}

bool CFont::IsSet() const
{
	if (!this)
	{
		_ASSERTE(this);
		return false;
	}

	switch (iType)
	{
	case CEFONT_NONE:
		return false;
	case CEFONT_GDI:
		return (hFont != NULL);
	case CEFONT_CUSTOM:
		return (pCustomFont != NULL);
	}

	_ASSERT(iType==CEFONT_GDI || iType==CEFONT_CUSTOM);
	return false;
}

void CFont::ResetFontWidth()
{
	m_CharWidth.clear();
}

bool CFont::Delete()
{
	BOOL result = FALSE;
	switch (iType)
	{
	case CEFONT_NONE:
		return false;
	case CEFONT_GDI:
		if (hFont)
		{
			result = DeleteObject(hFont);
			hFont = NULL;
		}
		break;
	case CEFONT_CUSTOM:
		if (pCustomFont)
		{
			result = TRUE;
			WARNING("Who own the pCustomFont?");
			// don't delete pCustomFont! Instance is owned by OptionsClass!
			pCustomFont = NULL;
		}
		break;
	default:
		_ASSERT(iType==CEFONT_GDI || iType==CEFONT_CUSTOM);
	}
	return (result != FALSE);
}
