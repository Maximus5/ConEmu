
/*
Copyright (c) 2016 Maximus5
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

#include "FontCache.h"
#include "FontMgr.h"

CFontCache::CFontCache(CVirtualConsole* apVCon)
	: mp_VCon(apVCon)
{
}

CFontCache::~CFontCache()
{
}

// Создать шрифт для отображения символов в диалоге плагина UCharMap
bool CFontCache::CreateOtherFont(const wchar_t* asFontName, CFontPtr& rpFont)
{
	CLogFont otherLF(asFontName, LogFont.lfHeight, 0, FW_NORMAL, DEFAULT_CHARSET, LogFont.lfQuality);
	HFONT hf = CreateFontIndirect(&otherLF);
	if (hf)
		rpFont = new CFont(hf);
	else
		rpFont.Release();
	return (rpFont.IsSet());
}

// pVCon may be NULL
bool CFontCache::QueryFont(CEFontStyles fontStyle, LPCWSTR asText, int cchLen, CFontPtr& rpFont)
{
	TODO("Take into account per-VCon font size zoom value");

	TODO("Optimization: Perhaps, we may not create all set of fonts, but");

	if ((fontStyle >= fnt_Normal) && (fontStyle < MAX_FONT_STYLES))
	{
		_ASSERTE(MAX_FONT_STYLES == (CEFontStyles)8);
		if (m_Font[fontStyle].IsSet())
		{
			rpFont = m_Font[fontStyle].Ptr();
		}
		else
		{
			_ASSERTE(FALSE && "Try to recteate the font?");
			rpFont.Release();
		}
		_ASSERTE(rpFont.IsSet()); // Fonts must be created already!
	}
	else if ((fontStyle == fnt_Alternative) && m_Font2.IsSet())
	{
		rpFont = m_Font2.Ptr();
	}
	else if ((fontStyle == fnt_UCharMap) && mp_VCon)
	{
		mp_VCon->GetUCharMapFontPtr(rpFont);
	}
	else
	{
		rpFont.Release();
	}

	if (!rpFont.IsSet())
	{
		rpFont = m_Font[fnt_Normal].Ptr();
		_ASSERTE(rpFont.IsSet()); // Fonts must be created already!
	}

	return (rpFont.IsSet());
}
