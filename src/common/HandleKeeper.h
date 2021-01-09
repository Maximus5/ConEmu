
/*
Copyright (c) 2015-present Maximus5
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

enum class HandleSource : unsigned
{
	Unknown = 0,
	StdIn = 1,
	StdOut = 2,
	StdErr = 3,
	CreateConsoleScreenBuffer = 4,
	CreateFileA = 5,
	CreateFileW = 6,
};

struct HandleInformation
{
	HANDLE h;

	#ifdef _DEBUG
	union
	{
	wchar_t *file_name_w; 
	char    *file_name_a;
	void    *file_name_ptr;
	};
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	BOOL wasConnector;
	unsigned int errorCode, errorPlace;
	#endif

	// enum HandleSource
	HandleSource source     : 3;

	// ‘General’ console handle "CON"
	unsigned int is_con     : 1;
	// Capable for ReadConsoleInput
	unsigned int is_input   : 1;
	// Capable for WriteFile, WriteConsole, SetConsoleTextAttribute
	unsigned int is_output  : 1;
	unsigned int is_error   : 1;
	// Capable for [Get|Set]ConsoleMode, [Get|Set]ConsoleScreenBufferInfo, ...
	unsigned int is_ansi    : 1;

	// Invalid access rights or smth else
	unsigned int is_invalid : 1;
};

namespace HandleKeeper
{
	bool PreCreateHandle(HandleSource source, DWORD& dwDesiredAccess, DWORD& dwShareMode, const void** as_name);
	bool AllocHandleInfo(HANDLE h, HandleSource source, DWORD access = 0, const void* as_name = nullptr, HandleInformation* pInfo = nullptr);
	bool QueryHandleInfo(HANDLE h, HandleInformation& info, bool ansiOutExpected);
	void CloseHandleInfo(HANDLE h);
	void SetConnectorMode(bool isConnector);

	void ReleaseHandleStorage();

	bool IsAnsiCapable(HANDLE hFile, bool *pbIsConOut = nullptr);
	bool IsOutputHandle(HANDLE hFile);
	bool IsInputHandle(HANDLE hFile);
};
