
/*
Copyright (c) 2016-present Maximus5
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

#include "../common/Common.h"

enum class ConsoleMainMode : int
{
	Normal = 0,
	AltServer = 1,
	Comspec = 2,
	GuiMacro = 3,  // could be used by integrators
};

#if defined(DLL_CONEMUCD_EXPORTS)

#if defined(__GNUC__)
extern "C"
{
#endif

	int __stdcall ConsoleMain2(ConsoleMainMode anWorkMode);

	int __stdcall ConsoleMain3(ConsoleMainMode anWorkMode, LPCWSTR asCmdLine);

	int __stdcall GuiMacro(LPCWSTR asInstance, LPCWSTR asMacro, BSTR* bsResult = nullptr);

	BOOL WINAPI HandlerRoutine(DWORD dwCtrlType);

	int WINAPI RequestLocalServer(/*[IN/OUT]*/RequestLocalServerParm* Parm);

#if defined(__GNUC__)
}
#endif

#endif


#define FN_CONEMUCD_CONSOLE_MAIN_2_NAME "ConsoleMain2"
typedef int (__stdcall* ConsoleMain2_t)(ConsoleMainMode anWorkMode);

#define FN_CONEMUCD_CONSOLE_MAIN_3_NAME "ConsoleMain3"
typedef int (__stdcall* ConsoleMain3_t)(ConsoleMainMode anWorkMode, LPCWSTR asCmdLine);

#define FN_CONEMUCD_HANDLER_ROUTINE_NAME "HandlerRoutine"
// type is PHANDLER_ROUTINE

#define FN_CONEMUCD_REQUEST_LOCAL_SERVER_NAME "PrivateEntry"
// type is RequestLocalServer_t

#define FN_CONEMUCD_GUIMACRO_NAME "GuiMacro"
/// <summary>
/// 
/// </summary>
typedef int(__stdcall* GuiMacro_t)(LPCWSTR asInstance, LPCWSTR asMacro, BSTR* bsResult /*= nullptr*/);
