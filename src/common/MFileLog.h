
/*
Copyright (c) 2009-present Maximus5
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

#include "CEStr.h"
#include "defines.h"
#include "MSectionSimple.h"

struct CEStartupEnv;
struct MSectionSimple;
class MFileLog;

class MFileLog
{
protected:
	CEStr filePathName_;
	CEStr fileName_;
	CEStr defaultPath_;
	HANDLE logHandle_{ nullptr };
	SYSTEMTIME lastWrite_{};
	MSectionSimple lock_{ true };

	HRESULT  InitFileName(LPCWSTR asName = nullptr, DWORD anPID = 0);
public:
	MFileLog(LPCWSTR asName, LPCWSTR asDir = nullptr, DWORD anPID = 0);
	virtual ~MFileLog();
	bool IsLogOpened() const;
	void CloseLogFile();
	HRESULT CreateLogFile(LPCWSTR asName = nullptr, DWORD anPID = 0, DWORD anLevel = 0); // Returns 0 if succeeded, otherwise - GetLastError() code
	LPCWSTR GetLogFileName();
	void LogString(LPCSTR asText, bool abWriteTime = true, LPCSTR asThreadName = nullptr, bool abNewLine = true, UINT anCP = CP_ACP);
	void LogString(LPCWSTR asText, bool abWriteTime = true, LPCWSTR asThreadName = nullptr, bool abNewLine = true);
};
