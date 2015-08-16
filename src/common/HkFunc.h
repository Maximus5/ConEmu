
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


#pragma once

#include <windows.h>

struct HkFunc
{
public:
	HkFunc();

public:
	COORD  getLargestConsoleWindowSize(HANDLE hConsoleOutput);
	BOOL   setConsoleScreenBufferSize(HANDLE hConsoleOutput, COORD dwSize);
	HWND   getConsoleWindow();
	LPVOID virtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
	BOOL   createProcess(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

public:
	void  SetInternalMode(HMODULE hSelf, FARPROC pfnGetTrampoline);
	void  OnHooksUnloaded();
	bool  isConEmuHk();

protected:
	enum {
		sNotChecked = 0,
		sNotHooked,
		sHooked,
		sConEmuHk,
		sUnloaded,
	} State;
	HMODULE hConEmuHk;
	typedef FARPROC (WINAPI* GetTrampoline_t)(LPCSTR pszName);
	GetTrampoline_t fnGetTrampoline;

	bool Init();

	typedef COORD(WINAPI* GetLargestConsoleWindowSize_t)(HANDLE hConsoleOutput);
	GetLargestConsoleWindowSize_t fnGetLargestConsoleWindowSize;
	typedef BOOL(WINAPI* SetConsoleScreenBufferSize_t)(HANDLE hConsoleOutput, COORD dwSize);
	SetConsoleScreenBufferSize_t fnSetConsoleScreenBufferSize;
	typedef HWND (WINAPI* GetConsoleWindow_t)();
	GetConsoleWindow_t fnGetConsoleWindow;
	typedef LPVOID(WINAPI* VirtualAlloc_t)(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
	VirtualAlloc_t fnVirtualAlloc;
	typedef BOOL (WINAPI* CreateProcessW_t)(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
	CreateProcessW_t fnCreateProcess;
};

extern HkFunc hkFunc;
