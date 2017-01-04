
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


#pragma once

#include <windows.h>

#include "../common/CEStr.h"
#include "../common/wcwidth.h"
#include "CustomFonts.h"
#include "FontInfo.h"
#include "FontPtr.h"
#include "RefRelease.h"

enum CEFONT_TYPE
{
	CEFONT_NONE   = 0,
	CEFONT_GDI    = 1,
	CEFONT_CUSTOM = 2,
};

class CFontMgr;

#ifdef HAS_CPP11
#include <unordered_map>
typedef std::unordered_map<ucs32,WORD> char_width_map;
#else
#include <hash_map>
typedef stdext::hash_map<ucs32,WORD> char_width_map;
#endif

class CFont : public CRefRelease
{
public:
	CEFONT_TYPE iType;

	union
	{
		HFONT hFont;
		CustomFont* pCustomFont;
		LPVOID pData;
	};

	bool mb_Monospace;  // Is the font looks like monospaced
	bool mb_RasterFont; // Specially for [Raster Fonts WxH]
	LPOUTLINETEXTMETRIC mp_otm;
	TEXTMETRIC m_tm;
	CLogFont m_LF;

	CEStr ms_FontError;

	char_width_map m_CharWidth;
	//TODO: std::unordered_map<ucs32,ABC> m_CharABC;

public:
	CFont();
	CFont(HFONT ahGdiFont);
	CFont(CustomFont* apFont);

protected:
	virtual ~CFont();
	virtual void FinalRelease() override;

public:
	// Methods
	bool Equal(const CFont* p) const;
	bool IsSet() const;
	void ResetFontWidth();

protected:
	friend class CFontMgr;
	friend class CFontPtr;
	// Members
	bool Delete();

};

bool operator== (const CFont &a, const CFont &b);
bool operator!= (const CFont &a, const CFont &b);


#if 0

struct CEFONT
{
	int iType;
	union
	{
		HFONT hFont;
		CustomFont* pCustomFont;
	};

	CEFONT() : iType(CEFONT_NONE), hFont(NULL) {}
	CEFONT(HFONT hFont) : iType(CEFONT_GDI), hFont(hFont) {}

	BOOL IsSet() const
	{
		switch (iType)
		{
		case CEFONT_NONE:
			return FALSE;
		case CEFONT_GDI:
			return hFont != NULL;
		case CEFONT_CUSTOM:
			return pCustomFont != NULL;
		}
		_ASSERT(0);
		return FALSE;
	}

	BOOL Delete()
	{
		BOOL result = FALSE;
		switch (iType)
		{
		case CEFONT_NONE:
			return FALSE;
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
				// delete pCustomFont; // instance is owned by OptionsClass
				pCustomFont = NULL;
			}
			break;
		default:
			_ASSERT(0);
		}
		return result;
	}
};

// used in CSettings::RecreateFont
bool operator== (const CEFONT &a, const CEFONT &b);
bool operator!= (const CEFONT &a, const CEFONT &b);

#endif