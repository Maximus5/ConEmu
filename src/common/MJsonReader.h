
/*
Copyright (c) 2016-2017 Maximus5
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

// This class is intended to unify access to resource data via JSON format,
// without any need to change caller routines, if we decide to switch parser.

#include "defines.h"

class MJsonValue
{
private:
	void* mp_data;

	wchar_t* ms_Error;

	bool mb_Root;
	void FreeData();
public:
	enum JSON_TYPE
	{
	   json_None,
	   json_Object,
	   json_Array,
	   json_Integer,
	   json_Double,
	   json_String,
	   json_Boolean,
	   json_Null
	};

public:
	MJsonValue();
	~MJsonValue();

	JSON_TYPE getType();
	size_t getLength();
	bool getBool();
	i64 getInt();
	double getDouble();
	const wchar_t* getString();
	const wchar_t* getObjectName(size_t index);
	bool getItem(size_t index, MJsonValue& value);
	bool getItem(const wchar_t* name, MJsonValue& value);

public:
	bool ParseJson(const wchar_t* buffer);
	const wchar_t* GetParseError();
};
