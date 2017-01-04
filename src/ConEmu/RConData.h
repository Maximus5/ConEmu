
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

#include "RefRelease.h"
#include "../common/MFileMapping.h"
#include "../common/ConsoleAnnotation.h"
#include "../common/RgnDetect.h" // struct CharAttr

class CRealConsole;
class CRConData;
class CRConDataGuard;

struct ConsoleLinePtr
{
	const wchar_t* pChar;
	const WORD* pAttr; // [may be NULL] - attributes from WinAPI
	const CharAttr* pAttrEx; // [may be NULL] - extended attributes
	int nLen;

	void clear();
	bool get(int index, wchar_t& chr, CharAttr& atr);
};

class CRConData : public CRefRelease
{
protected:
	CRealConsole* mp_RCon;

public:
	// Max buffers size (cells!)
	size_t     nMaxCells;
	UINT       nWidth, nHeight;
	wchar_t*   pConChar;
	WORD*      pConAttr;
	CHAR_INFO* pDataCmp;
	CONSOLE_SCREEN_BUFFER_INFO m_sbi;
protected:
	bool       bExternal;
	wchar_t*   pszBlock1;
	CharAttr*  pcaBlock1;

public:
	static bool Allocate(CRConDataGuard& data, CRealConsole* apRCon, size_t anMaxCells);
	static bool Allocate(CRConDataGuard& data, CRealConsole* apRCon, wchar_t* apszBlock1, CharAttr* apcaBlock1, const CONSOLE_SCREEN_BUFFER_INFO& asbi, const COORD& acrSize);

public:
	bool isValid(bool bCheckSizes, size_t anCellsRequired);
	UINT GetConsoleData(wchar_t* rpChar, CharAttr* rpAttr, UINT anWidth, UINT anHeight,
		wchar_t wSetChar, CharAttr lcaDef, CharAttr *lcaTable, CharAttr *lcaTableExt,
		bool bFade, bool bExtendColors, BYTE nExtendColorIdx, bool bExtendFonts);
	bool GetConsoleLine(int nLine, ConsoleLinePtr& rpLine) const;
	bool FindPanels(bool& bLeftPanel, RECT& rLeftPanel, RECT& rLeftPanelFull, bool& bRightPanel, RECT& rRightPanel, RECT& rRightPanelFull);
	short CheckProgressInConsole(UINT nCursorLine);

protected:
	CRConData(CRealConsole* apRCon);
	CRConData(CRealConsole* apRCon, wchar_t* apszBlock1, CharAttr* apcaBlock1, const CONSOLE_SCREEN_BUFFER_INFO& asbi, const COORD& acrSize);
	virtual ~CRConData();
	virtual void FinalRelease() override;
};

class CRConDataGuard : public CRefGuard<CRConData>
{
public:
	bool isValid(size_t anCellsRequired = 0);
};

