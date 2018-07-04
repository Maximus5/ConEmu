
/*
Copyright (c) 2013-present Maximus5
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

#include "defines.h"

// CEStr
struct CEStr
{
// #CEStr Make ms_Val protected
public:
	wchar_t *ms_Val = nullptr;
private:
	INT_PTR mn_MaxCount = 0; // Including termination \0

private:
	LPCWSTR AttachInt(wchar_t*& asPtr);

public:
	operator LPCWSTR() const;
	operator bool() const;
	LPCWSTR c_str(LPCWSTR asNullSubstitute = NULL) const;
	LPCWSTR Right(INT_PTR cchMaxCount) const;
	LPCWSTR Mid(INT_PTR cchOffset) const;

	int Compare(LPCWSTR asText, bool abCaseSensitive = false) const;
	bool operator==(const wchar_t* asStr) const;

	CEStr& operator=(CEStr&& asStr);
	CEStr& operator=(const CEStr& asStr);
	CEStr& operator=(wchar_t*&& asPtr);
	CEStr& operator=(const wchar_t* asPtr);

	INT_PTR GetLen() const;
	INT_PTR GetMaxCount();

	wchar_t* GetBuffer(INT_PTR cchMaxLen);
	wchar_t* Detach();
	LPCWSTR  Attach(wchar_t* RVAL_REF asPtr);
	LPCWSTR  Append(const wchar_t* asStr1, const wchar_t* asStr2 = NULL, const wchar_t* asStr3 = NULL, const wchar_t* asStr4 = NULL, const wchar_t* asStr5 = NULL, const wchar_t* asStr6 = NULL, const wchar_t* asStr7 = NULL, const wchar_t* asStr8 = NULL);
	void Clear();
	void Empty();
	bool IsEmpty() const;
	LPCWSTR Set(LPCWSTR asNewValue, INT_PTR anChars = -1);
	void SetAt(INT_PTR nIdx, wchar_t wc);

	CEStr();
	CEStr(CEStr&& asStr);
	CEStr(const CEStr& asStr);
	CEStr(wchar_t*&& asPtr);
	CEStr(const wchar_t* asStr1, const wchar_t* asStr2 = NULL, const wchar_t* asStr3 = NULL, const wchar_t* asStr4 = NULL, const wchar_t* asStr5 = NULL, const wchar_t* asStr6 = NULL, const wchar_t* asStr7 = NULL, const wchar_t* asStr8 = NULL, const wchar_t* asStr9 = NULL);
	~CEStr();
};

// Minimalistic storage for ANSI/UTF8 strings
struct CEStrA
{
public:
	CEStrA();
	CEStrA(const char* asPtr);
	CEStrA(char*&& asPtr);

	CEStrA(const CEStrA& src);
	CEStrA(CEStrA&& src);

	CEStrA& operator=(const char* asPtr);
	CEStrA& operator=(char*&& asPtr);

	CEStrA& operator=(const CEStrA& src);
	CEStrA& operator=(CEStrA&& src);

	operator const char*() const;
	operator bool() const;
	const char* c_str(const char* asNullSubstitute = NULL) const;
	INT_PTR length() const;
	void clear();

	char* getbuffer(INT_PTR cchMaxLen);
	char* release();

public:
	char* ms_Val = nullptr;
};
