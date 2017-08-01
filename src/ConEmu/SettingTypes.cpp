
/*
Copyright (c) 2014-2017 Maximus5
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

#include "Header.h"
#include "SettingTypes.h"
#include "resource.h"

const wchar_t* CESize::AsString() const
{
	switch (Style)
	{
	case ss_Pixels:
		_wsprintf(TempSZ, SKIPLEN(countof(TempSZ)) L"%ipx", Value);
		break;
	case ss_Percents:
		_wsprintf(TempSZ, SKIPLEN(countof(TempSZ)) L"%i%%", Value);
		break;
	//case ss_Standard:
	default:
		_wsprintf(TempSZ, SKIPLEN(countof(TempSZ)) L"%i", Value);
	}
	return TempSZ;
}

bool CESize::IsValid(bool IsWidth) const
{
	bool bValid;
	switch (Style)
	{
	case ss_Percents:
		bValid = (Value >= 1 && Value <= 100);
		break;
	case ss_Pixels:
		// Treat width/height as values for font size 4x2 (minimal)
		if (IsWidth)
			bValid = (Value >= (MIN_CON_WIDTH*4));
		else
			bValid = (Value >= (MIN_CON_HEIGHT*2));
		break;
	default:
		if (IsWidth)
			bValid = (Value >= MIN_CON_WIDTH);
		else
			bValid = (Value >= MIN_CON_HEIGHT);
	}
	return bValid;
}

bool CESize::Set(bool IsWidth, CESizeStyle NewStyle, int NewValue)
{
	if (NewStyle == ss_Standard)
	{
		int nDef = IsWidth ? 80 : 25;
		int nMax = IsWidth ? 1000 : 500;
		if (NewValue <= 0) NewValue = nDef; else if (NewValue > nMax) NewValue = nMax;
	}
	else if (NewStyle == ss_Percents)
	{
		int nDef = IsWidth ? 50 : 30;
		int nMax = 100;
		if (NewValue <= 0) NewValue = nDef; else if (NewValue > nMax) NewValue = nMax;
	}

	if (!NewValue)
	{
		// Size can't be empty
		_ASSERTE(NewValue);
		// Fail
		return false;
	}

	Value = NewValue;
	Style = NewStyle;
	return true;
}

void CESize::SetFromRaw(bool IsWidth, DWORD aRaw)
{
	CESize v; v.Raw = aRaw;
	if (v.Style == ss_Standard || v.Style == ss_Pixels || v.Style == ss_Percents)
	{
		this->Set(IsWidth, v.Style, v.Value);
	}
}

bool CESize::SetFromString(bool IsWidth, const wchar_t* sValue)
{
	if (!sValue || !*sValue)
		return false;
	wchar_t* pszEnd = NULL;
	// Try to convert
	int NewValue = wcstol(sValue, &pszEnd, 10);
	if (!NewValue)
		return false;

	CESizeStyle NewStyle = ss_Standard;
	if (pszEnd)
	{
		switch (*SkipNonPrintable(pszEnd))
		{
		case L'%':
			NewStyle = ss_Percents; break;
		case L'p':
			NewStyle = ss_Pixels; break;
		}
	}
	// Done
	return Set(IsWidth, NewStyle, NewValue);
}
