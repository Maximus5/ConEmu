
/*
Copyright (c) 2013 Maximus5
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

#define DOWNLOADTIMEOUT     30000
#define DOWNLOADTIMEOUTMAX  180000

struct CEDownloadErrorArg
{
	union {
	LPCWSTR   strArg;
	DWORD_PTR uintArg;
	};
	bool isString;
};

struct CEDownloadInfo
{
	size_t	strSize;
	LPARAM  lParam;
	LPCWSTR strFormat;
	size_t  argCount;
	CEDownloadErrorArg Args[3];
};

typedef void (WINAPI* FDownloadCallback)(const CEDownloadInfo* pError);

enum CEDownloadCommand
{
	dc_Init           = 1,
	dc_Reset          = 2,
	dc_Finalize       = 3,
	dc_SetProxy       = 4, // [0]="Server:Port", [1]="User", [2]="Password"
	dc_SetErrCallback = 5, // [0]=FDownloadCallback, [1]=lParam
	dc_SetProgress    = 6, // [0]=FDownloadCallback, [1]=lParam
	dc_SetLogLevel    = 7, // [0]=LogLevelNo
	dc_DownloadFile   = 8, // {IN}  - [0]="http", [1]="DestLocalFilePath", [2]=HANDLE(hDstFile), [3]=abShowAllErrors
	                       // {OUT} - [0]=SizeInBytes, [1] = CRC32
	dc_DownloadData   = 9, // [0]="http" -- not implemented yet
};

#if defined(__GNUC__)
extern "C"
#endif
DWORD_PTR DownloadCommand(CEDownloadCommand cmd, int argc, CEDownloadErrorArg* argv);
