
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
#include "HkFunc.h"

HkFunc hkFunc;

void WINAPI OnHooksUnloaded()
{
	hkFunc.State = HkFunc::sUnloaded;
}

HkFunc::HkFunc()
	: State(sNotChecked)
{
}

bool HkFunc::Init(LPCWSTR asModule, HMODULE hModule)
{
	if ((State != sNotChecked) && (State != sUnloaded) && (State != sConEmuHk))
		return (State == sHooked || State == sConEmuHk);

	BOOL bTrampolined = FALSE;
	typedef BOOL (WINAPI* RequestTrampolines_t)(LPCWSTR asModule, HMODULE hModule);
	RequestTrampolines_t fnRequestTrampolines;

	// WARNING! Heap may be not initialized in this step!

	HMODULE hConEmuHk = GetModuleHandle(WIN3264TEST(L"ConEmuHk.dll",L"ConEmuHk64.dll"));
	if (hConEmuHk != NULL)
	{
		fnRequestTrampolines = (RequestTrampolines_t)GetProcAddress(hConEmuHk, "RequestTrampolines");
		if (!fnRequestTrampolines)
		{
			// Old build?
			_ASSERTE(fnRequestTrampolines!=NULL);
		}
		else
		{
			bTrampolined = fnRequestTrampolines(asModule, hModule);
		}
	}

	State = bTrampolined ? sHooked : sNotHooked;
	return (State == sHooked || State == sConEmuHk);
}

// We are working in the ConEmuHk.dll itself
void HkFunc::SetInternalMode()
{
	_ASSERTEX(State == sNotChecked);

	State = sConEmuHk;
}

bool HkFunc::isConEmuHk()
{
	return (State == sConEmuHk);
}
