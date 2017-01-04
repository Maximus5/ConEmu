
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

// This class is intended to unify access to resource data via JSON format,
// without any need to change caller routines, if we decide to switch parser.

#include "MJsonReader.h"
#include "MStrDup.h"

// Use json-parser by James McLaughlin
#define json_char wchar_t
#include "../modules/json-parser/json.c"

MJsonValue::MJsonValue()
	: mp_data(NULL)
	, ms_Error(NULL)
	, mb_Root(false)
{
}

MJsonValue::~MJsonValue()
{
	FreeData();
}

void MJsonValue::FreeData()
{
	if (mp_data && mb_Root)
	{
		json_value* p = (json_value*)mp_data;
		mp_data = NULL;
		json_value_free (p);
	}
	else
	{
		mp_data = NULL;
	}
	mb_Root = false;
	SafeFree(ms_Error);
}

MJsonValue::JSON_TYPE MJsonValue::getType()
{
	switch (((json_value*)mp_data)->type)
	{
	case json_none:
		return json_None;
	case json_object:
		return json_Object;
	case json_array:
		return json_Array;
	case json_integer:
		return json_Integer;
	case json_double:
		return json_Double;
	case json_string:
		return json_String;
	case json_boolean:
		return json_Boolean;
	default:
		return json_Null;
	}
}

size_t MJsonValue::getLength()
{
	switch (((json_value*)mp_data)->type)
	{
	case json_object:
		return ((json_value*)mp_data)->u.object.length;
	case json_array:
		return ((json_value*)mp_data)->u.array.length;
	case json_string:
		return ((json_value*)mp_data)->u.string.length;
	default:
		return 0;
	}
}

bool MJsonValue::getBool()
{
	return (((json_value*)mp_data)->u.boolean != 0);
}

i64 MJsonValue::getInt()
{
	return ((json_value*)mp_data)->u.integer;
}

double MJsonValue::getDouble()
{
	return ((json_value*)mp_data)->u.dbl;
}

const wchar_t* MJsonValue::getString()
{
	return ((json_value*)mp_data)->u.string.ptr;
}

const wchar_t* MJsonValue::getObjectName(size_t index)
{
	return ((json_value*)mp_data)->u.object.values[index].name;
}

bool MJsonValue::getItem(size_t index, MJsonValue& value)
{
	_ASSERTE(&value != this);

	value.FreeData();
	if (!mp_data)
		return false;
	if (index >= getLength())
		return false;

	switch (((json_value*)mp_data)->type)
	{
	case json_object:
		value.mp_data = ((json_value*)mp_data)->u.object.values[index].value;
		return true;
	case json_array:
		value.mp_data = ((json_value*)mp_data)->u.array.values[index];
		return true;
	default:
		return false;
	}
}

bool MJsonValue::getItem(const wchar_t* name, MJsonValue& value)
{
	_ASSERTE(&value != this);

	value.FreeData();
	if (!mp_data || (((json_value*)mp_data)->type != json_object))
		return false;
	if (!name || !*name)
		return false;

	for (size_t i = 0; i < ((json_value*)mp_data)->u.object.length; ++i)
	{
		if (0 == wcscmp (((json_value*)mp_data)->u.object.values[i].name, name))
		{
			value.mp_data = ((json_value*)mp_data)->u.object.values[i].value;
			return true;
		}
	}

	return false;
}

const wchar_t* MJsonValue::GetParseError()
{
	return ms_Error;
}

bool MJsonValue::ParseJson(const wchar_t* buffer)
{
	if (mp_data)
	{
		FreeData();
	}

	json_settings setting = {};
	setting.settings = json_enable_comments;
	char errMsg[json_error_max] = "";

	json_value* p = json_parse_ex(&setting, buffer, wcslen(buffer), errMsg);
	if (p == NULL)
	{
		SafeFree(ms_Error);
		size_t eLen = strlen(errMsg);
		if (eLen)
		{
			ms_Error = (wchar_t*)malloc((eLen+1)*sizeof(*ms_Error));
			if (ms_Error)
				MultiByteToWideChar(CP_ACP, 0, errMsg, -1, ms_Error, eLen+1);
		}
		return false;
	}

	mb_Root = true;
	mp_data = p;
	return true;
}
