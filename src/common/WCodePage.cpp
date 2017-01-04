
/*
Copyright (c) 2015-2017 Maximus5
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
#include "Common.h"
#include "WCodePage.h"

void CpCvt::ResetBuffer()
{
	buf[blen=0] = 0;
}

CpCvtResult CpCvt::SetCP(UINT anCP)
{
	if (bInitialized && (nCP == anCP))
		return ccr_OK;

	CpCvtResult rc = (blen == 0) ? ccr_OK : ccr_BadBuffer;
	ResetBuffer();
	nCP = anCP;
	bInitialized = TRUE;

	if (! ((nCP == 42) || (nCP == 65000) || (nCP >= 57002 && nCP <= 57011) || (nCP >= 50220 && nCP <= 50229)) )
		nCvtFlags = MB_ERR_INVALID_CHARS;
	else
		nCvtFlags = 0;

	if (!GetCPInfoEx(anCP, 0, &CP))
	{
		ZeroStruct(CP);
		CP.MaxCharSize = 1;
		CP.DefaultChar[0] = '?';
		CP.UnicodeDefaultChar = 0xFFFD;
		return ccr_BadCodepage;
	}

	return ccr_OK;
}

CpCvtResult CpCvt::Convert(char c, wchar_t& wc)
{
	CpCvtResult rc = ccr_Dummy;
	int iRc;

	_ASSERTE((blen + 2) <= countof(buf));

	if (nCP == CP_UTF8)
	{
		BYTE bt = (BYTE)c;

		// Standalone char?
		if (!(bt & 0x80))
		{
			if (blen > 0)
			{
				wrc[0] = wc = CP.UnicodeDefaultChar; // (wchar_t)0xFFFD;
				wrc[1] = (wchar_t)c;
				wpair = true;
				rc = ccr_BadTail;
				ResetBuffer();
			}
			else
			{
				wc = (wchar_t)c;
				wpair = false;
				rc = ccr_OK;
			}
			goto wrap;
		}
		
		// Continuation byte (10xxxxxx)?
		if (blen && ((bt & 0xC0/*11xxxxxx*/) != 0x80/*10xxxxxx*/))
		{
			wrc[0] = wc = CP.UnicodeDefaultChar; // (wchar_t)0xFFFD;
			wpair = false;
			rc = ccr_BadUnicode;
			ResetBuffer();
			//goto wrap; -- start new sequence
		}

		// Sequence start?
		if (!blen)
		{
			_ASSERTE(CP.MaxCharSize==4);
			if ((bt & 0xE0/*11100000*/) == 0xC0/*110xxxxx*/)
				bmax = 2;
			else if ((bt & 0xF0/*11110000*/) == 0xE0/*1110xxxx*/)
				bmax = 3;
			else if ((bt & 0xF8/*11111000*/) == 0xF0/*11110xxx*/)
				bmax = 4;
			else
			{
				// Invalid UTF-8 start character
				if (rc == ccr_BadUnicode)
				{
					wrc[1] = CP.UnicodeDefaultChar; // (wchar_t)0xFFFD;
					wpair = true;
					rc = ccr_DoubleBad;
				}
				else
				{
					_ASSERTE(rc == ccr_Dummy);
					wc = CP.UnicodeDefaultChar; // (wchar_t)0xFFFD;
					wpair = false;
					rc = ccr_BadUnicode;
				}
				goto wrap;
			}
		}

		// Is UTF-8 sequence ready?
		if ((blen+1) < bmax)
		{
			buf[blen++] = c; buf[blen] = 0;
			if (rc == ccr_Dummy)
			{
				wc = CP.UnicodeDefaultChar; // (wchar_t)0xFFFD;
				rc = ccr_Pending;
			}
			goto wrap;
		}

	} // end of (nCP == CP_UTF8)
	else
	{
		bmax = CP.MaxCharSize;
	}

	buf[blen++] = c; buf[blen] = 0;

	iRc = MultiByteToWideChar(nCP, nCvtFlags, buf, blen, wrc, 2);
	switch (iRc)
	{
	case 1:
		wc = wrc[0];
		wpair = false;
		rc = ccr_OK;
		ResetBuffer();
		break;

	case 2:
		_ASSERTE(IS_HIGH_SURROGATE(wrc[0]));
		_ASSERTE(IS_LOW_SURROGATE(wrc[1]));
		wc = wrc[0];
		wpair = true;
		rc = ccr_Surrogate;
		ResetBuffer();
		break;

	default:
		wc = CP.UnicodeDefaultChar; // (wchar_t)0xFFFD;
		if (blen < bmax)
		{
			rc = ccr_Pending;
		}
		else
		{
			rc = ccr_BadUnicode;
			ResetBuffer();
		}
	}

wrap:
	return rc;
}

CpCvtResult CpCvt::GetTail(wchar_t& wc)
{
	if (!wpair)
	{
		wc = (wchar_t)0xFFFD;
		return ccr_BadUnicode;
	}

	//_ASSERTE(IS_HIGH_SURROGATE(wrc[0]));
	//_ASSERTE(IS_LOW_SURROGATE(wrc[1]));
	wc = wrc[1];
	return ccr_Surrogate;
}
