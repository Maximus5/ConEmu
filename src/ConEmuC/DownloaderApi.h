
/*
Copyright (c) 2013-2017 Maximus5
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

#include "../common/MStrDup.h"

#define DOWNLOADTIMEOUT             30000
#define DOWNLOADCLOSEHANDLETIMEOUT  5000
#define DOWNLOADOPERATIONTIMEOUT    120000

enum CEDownloadArgType
{
	at_None = 0,
	at_Uint = 1,
	at_Str  = 2,
};

struct CEDownloadErrorArg
{
	union {
	LPCWSTR   strArg;
	DWORD_PTR uintArg;
	};
	CEDownloadArgType argType;
};

struct CEDownloadInfo
{
	size_t	strSize;
	LPARAM  lParam;
	LPCWSTR strFormat;
	size_t  argCount;
	CEDownloadErrorArg Args[3];

	// For simplification of logging
	wchar_t* GetFormatted(bool bAppendNewLine) const
	{
		if (!strFormat)
			return NULL;
		// Format string length
		size_t cchTotal = lstrlen(strFormat);
		if (bAppendNewLine && strFormat[cchTotal-1] == L'\n')
			bAppendNewLine = false;
		// If no arguments - just return copy of format string
		if (argCount == 0)
			return lstrmerge(strFormat, bAppendNewLine?L"\n":NULL);
		cchTotal += 1 + (bAppendNewLine?1:0);
		// Calculate size of additional (arguments) length
		for (size_t i = 0; i < argCount; i++)
		{
			if (Args[i].argType == at_Uint)
			{
				cchTotal += 32;
			}
			else if (Args[i].argType == at_Str)
			{
				cchTotal += (Args[i].strArg ? lstrlen(Args[i].strArg) : 0);
			}
			else
			{
				_ASSERTE(Args[i].argType == at_Uint || Args[i].argType == at_Str);
				return NULL;
			}
		}
		wchar_t* pszAll = (wchar_t*)calloc(cchTotal, sizeof(*pszAll));
		if (!pszAll)
			return FALSE;
		// Does not matter, what type of arguments put into msprintf
		msprintf(pszAll, cchTotal, strFormat, Args[0].strArg, Args[1].strArg, Args[2].strArg);
		if (bAppendNewLine) _wcscat_c(pszAll, cchTotal, L"\n");
		// Done
		return pszAll;
	};
};

typedef void (WINAPI* FDownloadCallback)(const CEDownloadInfo* pError);

#define CEDLOG_MARK_ERROR L"Error: "
#define CEDLOG_MARK_PROGR L"Progr: "
#define CEDLOG_MARK_INFO  L"Info:  "

// For internal use only!
enum CEDownloadCommand
{
	// Callbacks. Must be sequental!
	dc_ErrCallback = 0,     // [0]=FDownloadCallback, [1]=lParam :: CEDLOG_MARK_ERROR
	dc_ProgressCallback,    // [0]=FDownloadCallback, [1]=lParam :: CEDLOG_MARK_PROGR
	dc_LogCallback,         // [0]=FDownloadCallback, [1]=lParam :: CEDLOG_MARK_INFO
	// Commands
	dc_Init,
	dc_DownloadFile,        // {IN}  - [0]="http", [1]="DestLocalFilePath", [2]=abShowAllErrors
	                        // {OUT} - [0]=SizeInBytes, [1] = CRC32
	dc_DownloadData,        // [0]="http" -- not implemented yet
	dc_Reset,
	dc_Deinit,
	dc_SetProxy,            // [0]="Server:Port", [1]="User", [2]="Password"
	dc_SetLogin,            // [0]="User", [1]="Password"
	dc_RequestTerminate,    // Without args
	dc_SetAsync,            // [0]=TRUE-Async, FALSE-Sync
	dc_SetTimeout,          // [0]=type (0-operation, 1-receive), [1]=ms
	dc_SetAgent,            // [0]="ConEmu Update"
	dc_SetCmdString,        // [0]="curl -L %1 -o %2"
};
