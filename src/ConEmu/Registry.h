
/*
Copyright (c) 2009-2015 Maximus5
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

#if !defined(__GNUC__) || defined(__MINGW64_VERSION_MAJOR)
#include <msxml.h>
#endif

#ifndef LPCBYTE
#define LPCBYTE const BYTE*
#endif

struct SettingsBase
{
	public:
		virtual bool OpenKey(const wchar_t *regPath, uint access, BOOL abSilent = FALSE) = 0;
		virtual void CloseKey() = 0;

		virtual bool Load(const wchar_t *regName, wchar_t **value) = 0;
		virtual bool Load(const wchar_t *regName, LPBYTE value, DWORD nSize) = 0;
		virtual bool Load(const wchar_t *regName, wchar_t *value, int maxLen) = 0; // required for registry type validarion (string)

		virtual void Delete(const wchar_t *regName) = 0;

		virtual void Save(const wchar_t *regName, LPCBYTE value, DWORD nType, DWORD nSize) = 0;

		// Helpers: Don't change &value if it was not loaded
		bool Load(const wchar_t *regName, char  &value) { return Load(regName, (LPBYTE)&value, 1); }
		bool Load(const wchar_t *regName, BYTE  &value) { return Load(regName, (LPBYTE)&value, 1); }
		bool Load(const wchar_t *regName, DWORD &value) { return Load(regName, (LPBYTE)&value, sizeof(DWORD)); }
		bool Load(const wchar_t *regName, LONG  &value) { return Load(regName, (LPBYTE)&value, sizeof(DWORD)); }
		bool Load(const wchar_t *regName, int   &value) { return Load(regName, (LPBYTE)&value, sizeof(DWORD)); }
		// Don't change &value if it was not loaded
		bool Load(const wchar_t *regName, bool  &value)
		{
			BYTE bval = 0;
			bool bRc = Load(regName, &bval, sizeof(bval));
			if (bRc) value = (bval!=0);
			return bRc;
		}
		// Don't change &value if it was not loaded
		bool Load(const wchar_t *regName, RECT &value)
		{
			wchar_t szRect[80] = L"";
			// Example: "100,200,500,400" (Left,Top,Right,Bottom)
			if (!Load(regName, szRect, countof(szRect)-1))
				return false;
			RECT rc = {};
			wchar_t *psz = szRect, *pszEnd;
			for (int i = 0; i < 4; i++)
			{
				if (!isDigit(psz[0]))
					return false;
				((LONG*)&rc)[i] = wcstol(psz, &pszEnd, 10);
				if (i < 3)
				{
					if (!pszEnd || !wcschr(L",;", *pszEnd))
						return false;
					psz = pszEnd+1;
				}
			}
			if (IsRectEmpty(&rc))
				return false;
			value = rc;
			return true;
		}

		void Save(const wchar_t *regName, const wchar_t *value)
		{
			if (!value) value = L"";  // protect against NULL values
			Save(regName, (LPCBYTE)value, REG_SZ, (_tcslen(value)+1)*sizeof(wchar_t));
		};

		// Use strict types to prohibit unexpected template usage
		void Save(const wchar_t *regName, const char&  value) { _Save(regName, value); }
		void Save(const wchar_t *regName, const bool&  value) { _Save(regName, value); }
		void Save(const wchar_t *regName, const BYTE&  value) { _Save(regName, value); }
		void Save(const wchar_t *regName, const DWORD& value) { _Save(regName, value); }
		void Save(const wchar_t *regName, const LONG&  value) { _Save(regName, value); }
		void Save(const wchar_t *regName, const int&   value) { _Save(regName, value); }
		void Save(const wchar_t *regName, const RECT&  value)
		{
			wchar_t szRect[80];
			_wsprintf(szRect, SKIPCOUNT(szRect) L"%i,%i,%i,%i", value.left, value.top, value.right, value.bottom);
			_Save(regName, szRect);
		}

		// nSize in BYTES!!!
		void SaveMSZ(const wchar_t *regName, const wchar_t *value, DWORD nSize)
		{
			if (!value || !*value)
				Delete(regName);
			else
				Save(regName, (LPBYTE)value, REG_MULTI_SZ, nSize);
		};

	protected:
		// bool, dword, rect, etc.
		template <class T> void _Save(const wchar_t *regName, T value)
		{
			DWORD len = sizeof(T);
			Save(regName, (LPBYTE)(&value), (len == sizeof(DWORD)) ? REG_DWORD : REG_BINARY, len);
		}

	public:
		//wchar_t Type[16];
		SettingsStorage m_Storage;
	protected:
		SettingsBase(const SettingsStorage& Storage) {m_Storage = Storage;};
		SettingsBase() {memset(&m_Storage,0,sizeof(m_Storage));};
	public:
		virtual ~SettingsBase() {};
};

struct SettingsRegistry : public SettingsBase
{
	public:
		HKEY regMy;

		bool OpenKey(HKEY inHKEY, const wchar_t *regPath, uint access, BOOL abSilent = FALSE);
		virtual bool OpenKey(const wchar_t *regPath, uint access, BOOL abSilent = FALSE) override;
		virtual void CloseKey() override;

		virtual bool Load(const wchar_t *regName, wchar_t **value) override;
		virtual bool Load(const wchar_t *regName, LPBYTE value, DWORD nSize) override;
		virtual bool Load(const wchar_t *regName, wchar_t *value, int maxLen) override; // нада, для проверки валидности типа реестра

		virtual void Delete(const wchar_t *regName) override;

		//virtual void Save(const wchar_t *regName, const wchar_t *value) override; // value = _T(""); // сюда мог придти и NULL
		virtual void Save(const wchar_t *regName, LPCBYTE value, DWORD nType, DWORD nSize) override;

	public:
		SettingsRegistry();
		virtual ~SettingsRegistry();
};

struct SettingsINI : public SettingsBase
{
	protected:
		wchar_t* mpsz_Section;
		LPCWSTR  mpsz_IniFile;
	public:

		virtual bool OpenKey(const wchar_t *regPath, uint access, BOOL abSilent = FALSE) override;
		virtual void CloseKey() override;

		virtual bool Load(const wchar_t *regName, wchar_t **value) override;
		virtual bool Load(const wchar_t *regName, LPBYTE value, DWORD nSize) override;
		virtual bool Load(const wchar_t *regName, wchar_t *value, int maxLen) override; // нада, для проверки валидности типа реестра

		virtual void Delete(const wchar_t *regName) override;

		//virtual void Save(const wchar_t *regName, const wchar_t *value) override; // value = _T(""); // сюда мог придти и NULL
		virtual void Save(const wchar_t *regName, LPCBYTE value, DWORD nType, DWORD nSize) override;

	public:
		SettingsINI(const SettingsStorage& Storage);
		virtual ~SettingsINI();
};

#if !defined(__GNUC__) || defined(__MINGW64_VERSION_MAJOR)
struct SettingsXML : public SettingsBase
{
	protected:
		IXMLDOMDocument* mp_File;
		IXMLDOMNode* mp_Key;
		bool mb_Modified; // Save was called at least once (data may be was not changed)
		bool mb_DataChanged; // Data was changed
		void SetDataChanged();
		int mi_Level;
		bool mb_Empty;
		bool mb_KeyEmpty;
		uint mn_access;
		//wchar_t ms_LevelPrefix[64];
		//BSTR mbs_LevelPrefix, mbs_LevelSuffix;
		static IXMLDOMDocument* CreateDomDocument(wchar_t*& pszErr);
		bool OpenStorage(uint access, wchar_t*& pszErr);
		void CloseStorage();
		void TouchKey(IXMLDOMNode* apKey);
	public:
		static bool IsXmlAllowed();
		virtual bool OpenKey(const wchar_t *regPath, uint access, BOOL abSilent = FALSE) override;
		virtual void CloseKey() override;

		virtual bool Load(const wchar_t *regName, wchar_t **value) override;
		virtual bool Load(const wchar_t *regName, LPBYTE value, DWORD nSize) override;
		virtual bool Load(const wchar_t *regName, wchar_t *value, int maxLen) override; // нада, для проверки валидности типа реестра

		virtual void Delete(const wchar_t *regName) override;

		//virtual void Save(const wchar_t *regName, const wchar_t *value) override; // value = _T(""); // сюда мог придти и NULL
		virtual void Save(const wchar_t *regName, LPCBYTE value, DWORD nType, DWORD nSize) override;

	protected:
		IXMLDOMNode* FindItem(IXMLDOMNode* apFrom, const wchar_t* asType, const wchar_t* asName, bool abAllowCreate);
		bool SetAttr(IXMLDOMNode* apNode, const wchar_t* asName, const wchar_t* asValue);
		bool SetAttr(IXMLDOMNode* apNode, IXMLDOMNamedNodeMap* apAttrs, const wchar_t* asName, const wchar_t* asValue);

		bool SetMultiLine(IXMLDOMNode* apNode, const wchar_t* asValue, long nAllLen);
		void ClearChildrenTail(IXMLDOMNode* apNode, IXMLDOMNode* apFirstClear);

		BSTR GetAttr(IXMLDOMNode* apNode, const wchar_t* asName);
		BSTR GetAttr(IXMLDOMNode* apNode, IXMLDOMNamedNodeMap* apAttrs, const wchar_t* asName);

		void AppendIndent(IXMLDOMNode* apFrom, int nLevel);
		void AppendNewLine(IXMLDOMNode* apFrom);
		void AppendText(IXMLDOMNode* apFrom, BSTR asText);

	public:
		SettingsXML(const SettingsStorage& Storage);
		virtual ~SettingsXML();
};
#endif
