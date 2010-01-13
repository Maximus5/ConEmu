
/*
Copyright (c) 2009-2010 Maximus5
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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

#include <msxml2.h>

class SettingsBase
{
public:
	virtual bool OpenKey(const wchar_t *regPath, uint access) = 0;
	virtual void CloseKey() = 0;

	virtual bool Load(const wchar_t *regName, wchar_t **value, LPDWORD pnSize = NULL) = 0;
	virtual bool Load(const wchar_t *regName, LPBYTE value, DWORD nSize) = 0;
	template <class T> bool Load(const wchar_t *regName, T &value)
	{
		DWORD len = sizeof(T);
		bool lbRc = Load(regName, (LPBYTE)&(value), len);
		return lbRc;
	}
	
	virtual void Delete(const wchar_t *regName) = 0;
	
	virtual void Save(const wchar_t *regName, const wchar_t *value) = 0; // value = _T(""); // сюда мог придти и NULL
	virtual void Save(const wchar_t *regName, LPCBYTE value, DWORD nType, DWORD nSize) = 0;
	
	void SaveMSZ(const wchar_t *regName, const wchar_t *value, DWORD nSize) // size in BYTES!!!
	{
		if (!value || !*value)
			Delete(regName);
		else
			Save(regName, (LPBYTE)value, REG_MULTI_SZ, nSize); 
	}
	void Save(const wchar_t *regName, wchar_t *value)
	{	// нада, чтобы вниз в template не провалился
		Save(regName, (const wchar_t*)value);
	}
	// bool, dword, rect, etc.
	template <class T> void Save(const wchar_t *regName, T value)
	{
		DWORD len = sizeof(T);
		Save(regName, (LPBYTE)(&value), (len == 4) ? REG_DWORD : REG_BINARY, len); 
	}
	
public:
	SettingsBase() {};
	virtual ~SettingsBase() {};
};

class SettingsRegistry : public SettingsBase
{
public:
	HKEY regMy;

	bool OpenKey(HKEY inHKEY, const wchar_t *regPath, uint access);
	virtual bool OpenKey(const wchar_t *regPath, uint access);
	virtual void CloseKey();

	virtual bool Load(const wchar_t *regName, wchar_t **value, LPDWORD pnSize = NULL);
	virtual bool Load(const wchar_t *regName, LPBYTE value, DWORD nSize);
	
	virtual void Delete(const wchar_t *regName);
	
	virtual void Save(const wchar_t *regName, const wchar_t *value); // value = _T(""); // сюда мог придти и NULL
	virtual void Save(const wchar_t *regName, LPCBYTE value, DWORD nType, DWORD nSize);

public:
	SettingsRegistry();
	virtual ~SettingsRegistry();
};

class SettingsXML : public SettingsBase
{
protected:
	IXMLDOMDocument* mp_File;
	IXMLDOMNode* mp_Key;
public:
	virtual bool OpenKey(const wchar_t *regPath, uint access);
	virtual void CloseKey();

	virtual bool Load(const wchar_t *regName, wchar_t **value, LPDWORD pnSize = NULL);
	virtual bool Load(const wchar_t *regName, LPBYTE value, DWORD nSize);
	
	virtual void Delete(const wchar_t *regName);
	
	virtual void Save(const wchar_t *regName, const wchar_t *value); // value = _T(""); // сюда мог придти и NULL
	virtual void Save(const wchar_t *regName, LPCBYTE value, DWORD nType, DWORD nSize);
	
protected:
	IXMLDOMNode* FindItem(IXMLDOMNode* apFrom, const wchar_t* asType, const wchar_t* asName, bool abAllowCreate);
	
public:
	SettingsXML();
	virtual ~SettingsXML();
};
