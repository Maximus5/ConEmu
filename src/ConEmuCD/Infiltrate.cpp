
/*
Copyright (c) 2009-2017 Maximus5
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

/*
This module contains static machine codes for calling LoadLibrary
These codes was produced by cl.exe with special compiler switches
Look at InfiltrateProc for source
*/

#include <windows.h>
#include "Infiltrate.h"
#include "../common/defines.h"

#define USE_STATIC_CODES

#ifndef USE_STATIC_CODES

// This code must be compiled with
// turn off Basic Runtime Checks (disable /RTC)
// and Buffer Security Checks (/GS-). 

#ifndef __GNUC__
#ifdef __MSVC_RUNTIME_CHECKS
	#pragma message("error: must be compiled with turn off Basic Runtime Checks and Buffer Security Checks")
#else
	#pragma message("warning: must be compiled with turn off Basic Runtime Checks and Buffer Security Checks")
#endif
#endif

static DWORD WINAPI InfiltrateProc(LPVOID _pInjDat)
{
	((struct InfiltrateArg*)_pInjDat)->_SetLastError(0);
	((struct InfiltrateArg*)_pInjDat)->hInst = ((struct InfiltrateArg*)_pInjDat)->_LoadLibraryW(((struct InfiltrateArg*)_pInjDat)->szConEmuHk);
	((struct InfiltrateArg*)_pInjDat)->ErrCode = ((struct InfiltrateArg*)_pInjDat)->_GetLastError();
	//ErrCode - DWORD_PTR просто для выравнивания на x64
	return (DWORD)((struct InfiltrateArg*)_pInjDat)->ErrCode;
}
static void InfiltrateEnd() {} // Mark the end
#endif

size_t GetInfiltrateProc(void** ppCode)
{
	size_t cbProcSize;
#ifdef USE_STATIC_CODES
	#ifndef _WIN64
	static BYTE Infiltrate_x86[] = {
		0x55, 0x8b, 0xec, 0x83, 0xec, 0x40, 0x53, 0x56, 0x57, 0x6a, 0x00, 0x8b, 0x45, 0x08, 0x8b, 0x48,
		0x04, 0xff, 0xd1, 0x8b, 0x45, 0x08, 0x83, 0xc0, 0x14, 0x50, 0x8b, 0x4d, 0x08, 0x8b, 0x51, 0x08,
		0xff, 0xd2, 0x8b, 0x4d, 0x08, 0x89, 0x41, 0x0c, 0x8b, 0x45, 0x08, 0x8b, 0x08, 0xff, 0xd1, 0x8b,
		0x55, 0x08, 0x89, 0x42, 0x10, 0x8b, 0x45, 0x08, 0x8b, 0x40, 0x10, 0x5f, 0x5e, 0x5b, 0x8b, 0xe5,
		0x5d, 0xc2, 0x04, 0x00
	};
	cbProcSize = countof(Infiltrate_x86);
	if (ppCode)
		*ppCode = (void*)Infiltrate_x86;
	#else
	static BYTE Infiltrate_x64[] = {
		0x48, 0x89, 0x4c, 0x24, 0x08, 0x48, 0x83, 0xec, 0x28, 0x33, 0xc9, 0x48, 0x8b, 0x44, 0x24, 0x30,
		0xff, 0x50, 0x08, 0x48, 0x8b, 0x4c, 0x24, 0x30, 0x48, 0x83, 0xc1, 0x28, 0x48, 0x8b, 0x44, 0x24,
		0x30, 0xff, 0x50, 0x10, 0x4c, 0x8b, 0xd8, 0x48, 0x8b, 0x44, 0x24, 0x30, 0x4c, 0x89, 0x58, 0x18,
		0x48, 0x8b, 0x44, 0x24, 0x30, 0xff, 0x10, 0x8b, 0xc8, 0x48, 0x8b, 0x44, 0x24, 0x30, 0x48, 0x89,
		0x48, 0x20, 0x48, 0x8b, 0x44, 0x24, 0x30, 0x8b, 0x40, 0x20, 0x48, 0x83, 0xc4, 0x28, 0xc3
	};
	cbProcSize = ARRAYSIZE(Infiltrate_x64);
	if (ppCode)
		*ppCode = (void*)Infiltrate_x64;
	#endif
#else
	cbProcSize = ((LPBYTE)InfiltrateEnd) - ((LPBYTE)InfiltrateProc);
	if (ppCode)
		*ppCode = (void*)InfiltrateProc;
#endif
	return cbProcSize;
}
