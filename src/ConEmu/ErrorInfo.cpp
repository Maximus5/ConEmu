
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


#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"
#include "../common/MStrSafe.h"
#include "ErrorInfo.h"

ErrorInfo::ErrorInfo()
	: error_(false), errorCode_(0)
{
}

ErrorInfo::ErrorInfo(const wchar_t* error)
	: what_(error), errorCode_(0), error_(true)
{
}

ErrorInfo::ErrorInfo(const wchar_t* format, unsigned errCode)
	: errorCode_(errCode), error_(true)
{
	if (IsStrNotEmpty(format))
	{
		const size_t cchMax = 32 + _tcslen(format);
		if (what_.GetBuffer(cchMax))
		{
			swprintf_c(what_.data(), what_.GetMaxCount(), format, errCode);
		}
	}
}

ErrorInfo::ErrorInfo(const wchar_t* format, const wchar_t* arg, const unsigned errCode)
	: errorCode_(errCode), error_(true)
{
	if (IsStrNotEmpty(format))
	{
		const size_t cchMax = 32 + _tcslen(format) + (arg ? _tcslen(arg) : 10);
		if (what_.GetBuffer(cchMax))
		{
			swprintf_c(what_.data(), what_.GetMaxCount(), format, arg ? arg : L"<null>", errCode);
		}
	}
}

ErrorInfo::ErrorInfo(const wchar_t* format, const wchar_t* arg1, const wchar_t* arg2, unsigned errCode)
	: errorCode_(errCode), error_(true)
{
	if (IsStrNotEmpty(format))
	{
		const size_t cchMax = 32 + _tcslen(format) + (arg1 ? _tcslen(arg1) : 10) + (arg2 ? _tcslen(arg2) : 10);
		if (what_.GetBuffer(cchMax))
		{
			swprintf_c(what_.data(), what_.GetMaxCount(), format, arg1 ? arg1 : L"<null>", arg2 ? arg2 : L"<null>", errCode);
		}
	}
}

ErrorInfo::~ErrorInfo()
{
}

const wchar_t* ErrorInfo::What() const
{
	return what_.c_str(L"<Unknown error>");
}

bool ErrorInfo::HasError() const
{
	return error_;
}

void ErrorInfo::ClearError()
{
	what_.Release();
	error_ = false;
}
