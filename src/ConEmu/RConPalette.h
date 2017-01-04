
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

#include "../common/RgnDetect.h"

class CRealConsole;

class CRConPalette
{
public:
	CRealConsole* mp_RCon;
	CharAttr m_TableOrg[0x100];
	CharAttr m_TableExt[0x100];
	COLORREF m_Colors[32];

protected:
	bool mb_Initialized;
	bool mb_VividColors;
	bool mb_ExtendFonts;
	BYTE mn_FontNormalColor, mn_FontBoldColor, mn_FontItalicColor;

public:
	CRConPalette(CRealConsole* apRCon);
	virtual ~CRConPalette();

public:
	// Methods
	void UpdateColorTable(COLORREF *apColors/*[32]*/,
		bool bVividColors,
		bool bExtendFonts, BYTE nFontNormalColor, BYTE nFontBoldColor, BYTE nFontItalicColor);

};
