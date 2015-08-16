
/*
Copyright (c) 2015 Maximus5
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


#include "common.hpp"
#include "HkFunc.h"

HkFunc hkFunc;

HkFunc::HkFunc()
	: State(sNotChecked)
	, hConEmuHk(NULL)
	, fnGetTrampoline(NULL)
	, fnGetLargestConsoleWindowSize(NULL)
	, fnSetConsoleScreenBufferSize(NULL)
	, fnGetConsoleWindow(NULL)
	, fnVirtualAlloc(NULL)
	, fnCreateProcess(NULL)
{
}

WARNING("DANGER zone. If ConEmuHk unloads calling saved pointers may cause crashes");

bool HkFunc::Init()
{
	if (State == sNotChecked)
	{
		hConEmuHk = GetModuleHandle(WIN3264TEST(L"ConEmuHk.dll",L"ConEmuHk64.dll"));
		if (hConEmuHk != NULL)
		{
			fnGetTrampoline = (GetTrampoline_t)GetProcAddress(hConEmuHk, "GetTrampoline");
			if (!fnGetTrampoline)
			{
				// Old build?
				_ASSERTE(fnGetTrampoline!=NULL);
				hConEmuHk = NULL;
			}
		}

		State = (hConEmuHk != NULL) ? sHooked : sNotHooked;
	}

	return ((State == sHooked || State == sConEmuHk) && (fnGetTrampoline != NULL));
}

// We are working in the ConEmuHk.dll itself
void HkFunc::SetInternalMode(HMODULE hSelf, FARPROC pfnGetTrampoline)
{
	_ASSERTEX(State == sNotChecked && pfnGetTrampoline);

	hConEmuHk = hSelf;
	fnGetTrampoline = (GetTrampoline_t)pfnGetTrampoline;

	State = sConEmuHk;
}

void HkFunc::OnHooksUnloaded()
{
	State = sUnloaded;
	fnGetTrampoline = NULL;
	fnGetLargestConsoleWindowSize = NULL;
	fnSetConsoleScreenBufferSize = NULL;
	fnGetConsoleWindow = NULL;
	fnVirtualAlloc = NULL;
	fnCreateProcess = NULL;
}

bool HkFunc::isConEmuHk()
{
	return (State == sConEmuHk);
}

COORD HkFunc::getLargestConsoleWindowSize(HANDLE hConsoleOutput)
{
	if (Init())
	{
		if (!fnGetLargestConsoleWindowSize)
			fnGetLargestConsoleWindowSize = (GetLargestConsoleWindowSize_t)fnGetTrampoline("GetLargestConsoleWindowSize");
		if (fnGetLargestConsoleWindowSize)
			return fnGetLargestConsoleWindowSize(hConsoleOutput);
	}

	return GetLargestConsoleWindowSize(hConsoleOutput);
}

BOOL HkFunc::setConsoleScreenBufferSize(HANDLE hConsoleOutput, COORD dwSize)
{
	if (Init())
	{
		if (!fnSetConsoleScreenBufferSize)
			fnSetConsoleScreenBufferSize = (SetConsoleScreenBufferSize_t)fnGetTrampoline("SetConsoleScreenBufferSize");
		if (fnSetConsoleScreenBufferSize)
			return fnSetConsoleScreenBufferSize(hConsoleOutput, dwSize);
	}

	return SetConsoleScreenBufferSize(hConsoleOutput, dwSize);
}

HWND HkFunc::getConsoleWindow()
{
	if (Init())
	{
		if (!fnGetConsoleWindow)
			fnGetConsoleWindow = (GetConsoleWindow_t)fnGetTrampoline("GetConsoleWindow");
		if (fnGetConsoleWindow)
			return fnGetConsoleWindow();
	}

	return GetConsoleWindow();
}

LPVOID HkFunc::virtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
	if (Init())
	{
		if (!fnVirtualAlloc)
			fnVirtualAlloc = (VirtualAlloc_t)fnGetTrampoline("VirtualAlloc");
		if (fnVirtualAlloc)
			return fnVirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
	}

	return VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
}

BOOL HkFunc::createProcess(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	if (Init())
	{
		if (!fnCreateProcess)
			fnCreateProcess = (CreateProcessW_t)fnGetTrampoline("CreateProcessW");
		if (fnCreateProcess)
			return fnCreateProcess(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	}

	return CreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
}
