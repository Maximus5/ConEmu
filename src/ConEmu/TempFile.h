
/*
Copyright (c) 2021-present Maximus5
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

#include "../common/defines.h"
#include "../common/CEStr.h"
#include "../common/MHandle.h"

#include "ErrorInfo.h"

class MHandle;

class TempFile final
{
	CEStr path_;
	MHandle fileHandle_;

public:
	TempFile();
	TempFile(LPCWSTR asDir, LPCWSTR asFileNameTempl);
	TempFile(TempFile&&) = default;
	TempFile& operator=(TempFile&&) = default;

	TempFile(const TempFile&) = delete;
	TempFile& operator=(const TempFile&) = delete;

	~TempFile();

	bool CreateTempFile(LPCWSTR asDir, LPCWSTR asFileNameTempl, ErrorInfo& ec);

	bool IsValid() const;
	void Release(bool deleteFile = true);
	void CloseFile();
	HANDLE GetHandle() const;
	const wchar_t* GetFilePath() const;
};
