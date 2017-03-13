
/*
Copyright (c) 2009-2017 Maximus5
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
#include <windows.h>
#include "header.h"
#include "registry.h"
#include "ConEmu.h"

#ifdef _DEBUG
#define HEAPVAL //HeapValidate(GetProcessHeap(), 0, NULL);
#else
#define HEAPVAL
#endif

#ifndef __GNU__
const CLSID CLSID_DOMDocument30 = {0xf5078f32, 0xc551, 0x11d3, {0x89, 0xb9, 0x00, 0x00, 0xf8, 0x1f, 0xe2, 0x21}};
#endif

// Let save integers as 10-based numbers
#define REG__LONG  254
#define REG__ULONG 255

SettingsBase::SettingsBase(const SettingsStorage& Storage)
{
	m_Storage = Storage;
}

SettingsBase::SettingsBase()
{
	memset(&m_Storage,0,sizeof(m_Storage));
}

SettingsBase::~SettingsBase()
{
}

// Helpers: Don't change &value if it was not loaded
bool SettingsBase::Load(const wchar_t *regName, char  &value)
{
	return Load(regName, (LPBYTE)&value, 1);
}
bool SettingsBase::Load(const wchar_t *regName, BYTE  &value)
{
	return Load(regName, (LPBYTE)&value, 1);
}
bool SettingsBase::Load(const wchar_t *regName, DWORD &value)
{
	return Load(regName, (LPBYTE)&value, sizeof(DWORD));
}
bool SettingsBase::Load(const wchar_t *regName, LONG  &value)
{
	return Load(regName, (LPBYTE)&value, sizeof(DWORD));
}
bool SettingsBase::Load(const wchar_t *regName, int   &value)
{
	return Load(regName, (LPBYTE)&value, sizeof(DWORD));
}
bool SettingsBase::Load(const wchar_t *regName, UINT  &value)
{
	return Load(regName, (LPBYTE)&value, sizeof(DWORD));
}
// Don't change &value if it was not loaded
bool SettingsBase::Load(const wchar_t *regName, bool  &value)
{
	BYTE bval = 0;
	bool bRc = Load(regName, &bval, sizeof(bval));
	if (bRc) value = (bval!=0);
	return bRc;
}
// Don't change &value if it was not loaded
bool SettingsBase::Load(const wchar_t *regName, RECT &value)
{
	wchar_t szRect[80] = L"";
	// Example: "100,200,500,400" (Left,Top,Right,Bottom)
	if (!Load(regName, szRect, countof(szRect)-1))
		return false;
	RECT rc = {};
	wchar_t *psz = szRect, *pszEnd;
	for (int i = 0; i < 4; i++)
	{
		if (!isDigit(psz[0]) && (psz[0] != L'-'))
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

void SettingsBase::Save(const wchar_t *regName, const wchar_t *value)
{
	if (!value) value = L"";  // protect against NULL values
	Save(regName, (LPCBYTE)value, REG_SZ, (_tcslen(value)+1)*sizeof(wchar_t));
}

// Use strict types to prohibit unexpected template usage
void SettingsBase::Save(const wchar_t *regName, const char&  value)
{
	_Save(regName, value);
}
void SettingsBase::Save(const wchar_t *regName, const bool&  value)
{
	_Save(regName, value);
}
void SettingsBase::Save(const wchar_t *regName, const BYTE&  value)
{
	_Save(regName, value);
}
void SettingsBase::Save(const wchar_t *regName, const DWORD& value)
{
	Save(regName, (LPBYTE)(&value), REG_DWORD, sizeof(DWORD));
}
void SettingsBase::Save(const wchar_t *regName, const LONG&  value)
{
	Save(regName, (LPBYTE)(&value), REG__LONG, sizeof(int));
}
void SettingsBase::Save(const wchar_t *regName, const int&   value)
{
	_ASSERTE(sizeof(int) == 4);
	Save(regName, (LPBYTE)(&value), REG__LONG, sizeof(int));
}
void SettingsBase::Save(const wchar_t *regName, const UINT&   value)
{
	_ASSERTE(sizeof(int) == 4);
	Save(regName, (LPBYTE)(&value), REG__ULONG, sizeof(int));
}
void SettingsBase::Save(const wchar_t *regName, const RECT&  value)
{
	wchar_t szRect[80];
	_wsprintf(szRect, SKIPCOUNT(szRect) L"%i,%i,%i,%i", value.left, value.top, value.right, value.bottom);
	Save(regName, szRect);
}

// nSize in BYTES!!!
void SettingsBase::SaveMSZ(const wchar_t *regName, const wchar_t *value, DWORD nSize)
{
	if (!value || !*value)
		Delete(regName);
	else
		Save(regName, (LPBYTE)value, REG_MULTI_SZ, nSize);
}



SettingsRegistry::SettingsRegistry()
	: SettingsBase()
{
	ZeroStruct(m_Storage);
	regMy = NULL;
	lstrcpy(m_Storage.szType, CONEMU_CONFIGTYPE_REG);
}
SettingsRegistry::~SettingsRegistry()
{
	CloseKey();
}



bool SettingsRegistry::OpenKey(HKEY inHKEY, const wchar_t *regPath, uint access, BOOL abSilent /*= FALSE*/)
{
	bool res = false;

	_ASSERTE(!gpConEmu->IsResetBasicSettings() || ((access & KEY_WRITE)!=KEY_WRITE) || !lstrcmpi(regPath, L"Software\\Microsoft\\Command Processor"));

	if ((access & KEY_WRITE) == KEY_WRITE)
		res = RegCreateKeyEx(inHKEY, regPath, 0, NULL, 0, access, 0, &regMy, 0) == ERROR_SUCCESS;
	else
		res = RegOpenKeyEx(inHKEY, regPath, 0, access, &regMy) == ERROR_SUCCESS;

	return res;
}
bool SettingsRegistry::OpenKey(const wchar_t *regPath, uint access, BOOL abSilent /*= FALSE*/)
{
	return OpenKey(HKEY_CURRENT_USER, regPath, access);
}
void SettingsRegistry::CloseKey()
{
	if (regMy)
	{
		RegCloseKey(regMy);
		regMy = NULL;
	}
}



bool SettingsRegistry::Load(const wchar_t *regName, LPBYTE value, DWORD nSize)
{
	_ASSERTE(nSize>0);

	DWORD nNewSize = nSize;
	LONG lRc = RegQueryValueEx(regMy, regName, NULL, NULL, (LPBYTE)value, &nNewSize);
	if (lRc == ERROR_SUCCESS)
		return true;

	// Access denied может быть, если пытаемся _читать_ из ключа, открытого на _запись_
	_ASSERTE(lRc != ERROR_ACCESS_DENIED);

	if (lRc == ERROR_MORE_DATA && nSize == sizeof(BYTE) && nNewSize == sizeof(DWORD))
	{
		// если тип раньше был DWORD а стал - BYTE
		DWORD nData = 0;
		lRc = RegQueryValueEx(regMy, regName, NULL, NULL, (LPBYTE)&nData, &nNewSize);
		if (lRc == ERROR_SUCCESS)
		{
			*value = (BYTE)(nData & 0xFF);
			return true;
		}
	}
	return false;
}
// эта функция, если значения нет (или тип некорректный) *value НЕ трогает
bool SettingsRegistry::Load(const wchar_t *regName, wchar_t **value)
{
	DWORD len = 0;

	if (RegQueryValueExW(regMy, regName, NULL, NULL, NULL, &len) == ERROR_SUCCESS && len)
	{
		int nChLen = len/2;
		if (*value) {free(*value); *value = NULL;}
		*value = (wchar_t*)malloc((nChLen+2)*sizeof(wchar_t));

		bool lbRc = (RegQueryValueExW(regMy, regName, NULL, NULL, (LPBYTE)(*value), &len) == ERROR_SUCCESS);

		if (!lbRc)
			nChLen = 0;
		(*value)[nChLen] = 0; (*value)[nChLen+1] = 0; // На случай REG_MULTI_SZ

		return lbRc;
	}
	//else if (!*value)
	//{
	//	*value = (wchar_t*)malloc(sizeof(wchar_t)*2);
	//	(*value)[0] = 0; (*value)[1] = 0; // На случай REG_MULTI_SZ
	//}

	return false;
}
// эта функция, если значения нет (или тип некорректный) value НЕ трогает
bool SettingsRegistry::Load(const wchar_t *regName, wchar_t *value, int maxLen)
{
	_ASSERTE(maxLen>1);
	DWORD len = 0, dwType = 0;

	if (RegQueryValueExW(regMy, regName, NULL, &dwType, NULL, &len) == ERROR_SUCCESS && dwType == REG_SZ)
	{
		len = maxLen*2; // max size in BYTES

		if (RegQueryValueExW(regMy, regName, NULL, NULL, (LPBYTE)value, &len) == ERROR_SUCCESS)
		{
			if (value)
				value[maxLen-1] = 0; // на всякий случай, чтобы ASCIIZ был однозначно
			return true;
		}
	}

	return false;
}



void SettingsRegistry::Delete(const wchar_t *regName)
{
	RegDeleteValue(regMy, regName);
}



void SettingsRegistry::Save(const wchar_t *regName, LPCBYTE value, DWORD nType, DWORD nSize)
{
	_ASSERTE(value && nSize);
	if (nType == REG__LONG || nType == REG__ULONG) nType = REG_DWORD;
	RegSetValueEx(regMy, regName, 0, nType, (LPBYTE)value, nSize);
}
//void SettingsRegistry::Save(const wchar_t *regName, const wchar_t *value)
//{
//	if (!value) value = _T("");  // сюда мог придти и NULL
//
//	RegSetValueEx(regMy, regName, NULL, REG_SZ, (LPBYTE)value, (lstrlenW(value)+1) * sizeof(wchar_t));
//}



















/* *************************** */

SettingsINI::SettingsINI(const SettingsStorage& Storage)
	: SettingsBase(Storage)
{
	mpsz_Section = NULL;
	mpsz_IniFile = NULL;
	lstrcpy(m_Storage.szType, CONEMU_CONFIGTYPE_INI);
}
SettingsINI::~SettingsINI()
{
	CloseKey();
}



bool SettingsINI::OpenKey(const wchar_t *regPath, uint access, BOOL abSilent /*= FALSE*/)
{
	_ASSERTE(!gpConEmu->IsResetBasicSettings() || ((access & KEY_WRITE)!=KEY_WRITE));

	SafeFree(mpsz_Section);

	if (!regPath || !*regPath)
	{
		mpsz_IniFile = NULL;
		return false;
	}

	if (m_Storage.pszFile && *m_Storage.pszFile)
	{
		mpsz_IniFile = m_Storage.pszFile;
	}
	else
	{
		_ASSERTE(m_Storage.pszFile && *m_Storage.pszFile);
		m_Storage.pszFile = mpsz_IniFile = gpConEmu->ConEmuIni();
	}

	if (!mpsz_IniFile || !*mpsz_IniFile)
	{
		mpsz_IniFile = NULL;
		return false;
	}

	HANDLE hFile = CreateFile(mpsz_IniFile,
		((access & KEY_WRITE) == KEY_WRITE) ? GENERIC_WRITE : GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		((access & KEY_WRITE) == KEY_WRITE) ? OPEN_ALWAYS : OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		mpsz_IniFile = NULL;
		return false;
	}

	CloseHandle(hFile);

	wchar_t* pszDup = lstrdup(regPath);
	if (!pszDup)
	{
		mpsz_IniFile = NULL;
		return FALSE;
	}
	// Замена символов
	wchar_t* psz = pszDup;
	while ((psz = wcspbrk(psz, L" ?")))
		*psz = L'_';
	psz = pszDup;
	while ((psz = wcspbrk(psz, L"/\\")))
		*psz = L' ';


	size_t cchMax = (_tcslen(pszDup)*3)+1;
	char* pszSectionA = (char*)malloc(cchMax);
	if (!pszSectionA)
	{
		mpsz_IniFile = NULL;
		SafeFree(pszDup);
		return false;
	}

	int nLen = WideCharToMultiByte(CP_UTF8, 0, pszDup, -1, pszSectionA, cchMax, 0,0);
	mpsz_Section = (wchar_t*)malloc((nLen+1)*sizeof(*mpsz_Section));
	if (!mpsz_Section)
	{
		SafeFree(pszSectionA);
		SafeFree(pszDup);
		return false;
	}
	MultiByteToWideChar(CP_ACP, 0, pszSectionA, -1, mpsz_Section, nLen+1);

	SafeFree(pszSectionA);
	SafeFree(pszDup);
	return true;
}
void SettingsINI::CloseKey()
{
	SafeFree(mpsz_Section);
}



bool SettingsINI::Load(const wchar_t *regName, LPBYTE value, DWORD nSize)
{
	_ASSERTE(nSize>0);

	return false;
}
// эта функция, если значения нет (или тип некорректный) *value НЕ трогает
bool SettingsINI::Load(const wchar_t *regName, wchar_t **value)
{
	//DWORD len = 0;


	return false;
}
// эта функция, если значения нет (или тип некорректный) value НЕ трогает
bool SettingsINI::Load(const wchar_t *regName, wchar_t *value, int maxLen)
{
	_ASSERTE(maxLen>1);
	//DWORD len = 0, dwType = 0;

	return false;
}



void SettingsINI::Delete(const wchar_t *regName)
{
	if (mpsz_IniFile && *mpsz_IniFile && mpsz_Section && *mpsz_Section && regName && *regName)
	{
		//char szName[MAX_PATH*3] = {};
		//WideCharToMultiByte(CP_UTF8, 0, regName, -1, szName, countof(szName), 0,0);
		//if (*szName)
		WritePrivateProfileString(mpsz_Section, regName, NULL, mpsz_IniFile);
	}
}



void SettingsINI::Save(const wchar_t *regName, LPCBYTE value, DWORD nType, DWORD nSize)
{
	_ASSERTE(value && nSize);
}









/* *************************** */
#if !defined(__GNUC__) || defined(__MINGW64_VERSION_MAJOR)
SettingsXML::SettingsXML(const SettingsStorage& Storage)
	: SettingsBase(Storage)
{
	mp_File = NULL; mp_Key = NULL;
	lstrcpy(m_Storage.szType, CONEMU_CONFIGTYPE_XML);
	mb_Modified = false;
	mb_DataChanged = false;
	mi_Level = 0;
	mb_Empty = false;
	mb_KeyEmpty = false;
	mn_access = 0;
}
SettingsXML::~SettingsXML()
{
	CloseStorage();
}

IXMLDOMDocument* SettingsXML::CreateDomDocument(wchar_t*& pszErr)
{
	HRESULT hr;
	IXMLDOMDocument* pFile = NULL;
	static HMODULE hMsXml3 = NULL;
	typedef HRESULT (__stdcall* DllGetClassObject_t)(REFCLSID rclsid, REFIID riid, LPVOID *ppv);
	static DllGetClassObject_t lpfnGetClassObject = NULL;
	wchar_t szDllErr[128] = {};

	_ASSERTE(pszErr==NULL);

	hr = CoInitialize(NULL);

	// Если в прошлый раз обломались, и загрузили "msxml3.dll" - то и не дергаться
	if (hMsXml3 && (hMsXml3 != (HMODULE)INVALID_HANDLE_VALUE))
		hr = REGDB_E_CLASSNOTREG;
	else
		hr = CoCreateInstance(CLSID_DOMDocument30, NULL, CLSCTX_INPROC_SERVER, //-V519
							  IID_IXMLDOMDocument, (void**)&pFile);

	// Если msxml3.dll (Msxml2.DOMDocument.3.0) не зарегистрирована - будет такая ошибка
	if (FAILED(hr)) // (hr == REGDB_E_CLASSNOTREG)
	{
		HRESULT hFact = 0;
		// Попробовать грузануть ее ручками
		if (!hMsXml3)
		{
			wchar_t szDll[MAX_PATH+16];
			struct FindPlaces { LPCWSTR sDir, sSlash; } findPlaces[] = {
				{gpConEmu->ms_ConEmuExeDir,  L"\\"},
				{gpConEmu->ms_ConEmuBaseDir, L"\\"},
				{L"", L""}, {NULL}};

			for (FindPlaces* fp = findPlaces; fp->sDir; fp++)
			{
				_wsprintf(szDll, SKIPLEN(countof(szDll)) L"%s%smsxml3.dll", fp->sDir, fp->sSlash);
				hMsXml3 = LoadLibrary(szDll);
				hFact = hMsXml3 ? 0 : (HRESULT)GetLastError();
				if (hMsXml3)
					break;
			//if (!hMsXml3
			//	&& (((DWORD)hFact) == ERROR_MOD_NOT_FOUND
			//		|| ((DWORD)hFact) == ERROR_BAD_EXE_FORMAT
			//		|| ((DWORD)hFact) == ERROR_FILE_NOT_FOUND))
			}

			//_wsprintf(szDll, SKIPLEN(countof(szDll)) L"%s\\msxml3.dll", gpConEmu->ms_ConEmuExeDir);
			//hMsXml3 = LoadLibrary(szDll);
			//hFact = hMsXml3 ? 0 : (HRESULT)GetLastError();
			//if (!hMsXml3
			//	&& (((DWORD)hFact) == ERROR_MOD_NOT_FOUND
			//		|| ((DWORD)hFact) == ERROR_BAD_EXE_FORMAT
			//		|| ((DWORD)hFact) == ERROR_FILE_NOT_FOUND))
			//{
			//	_wsprintf(szDll, SKIPLEN(countof(szDll)) L"%s\\msxml3.dll", gpConEmu->ms_ConEmuBaseDir);
			//	hMsXml3 = LoadLibrary(szDll);
			//	hFact = hMsXml3 ? 0 : (HRESULT)GetLastError();
			//}

			if (!hMsXml3)
			{
				hMsXml3 = (HMODULE)INVALID_HANDLE_VALUE;
				_wsprintf(szDllErr, SKIPLEN(countof(szDllErr)) L"\nLoadLibrary(\"msxml3.dll\") failed\nErrCode=0x%08X", (DWORD)hFact);
			}
		}

		if (hMsXml3 && (hMsXml3 != (HMODULE)INVALID_HANDLE_VALUE))
		{
			if (!lpfnGetClassObject)
				lpfnGetClassObject = (DllGetClassObject_t)GetProcAddress(hMsXml3, "DllGetClassObject");

			if (!lpfnGetClassObject)
			{
				hFact = (HRESULT)GetLastError();
				_wsprintf(szDllErr, SKIPLEN(countof(szDllErr)) L"\nGetProcAddress(\"DllGetClassObject\") failed\nErrCode=0x%08X", (DWORD)hFact);
			}
			else
			{
				IClassFactory* pFact = NULL;
				hFact = lpfnGetClassObject(CLSID_DOMDocument30, IID_IClassFactory, (void**)&pFact);
				if (SUCCEEDED(hFact) && pFact)
				{
					hFact = pFact->CreateInstance(NULL, IID_IXMLDOMDocument, (void**)&pFile);
					if (SUCCEEDED(hFact) && pFile)
						hr = hFact;
					else
						_wsprintf(szDllErr, SKIPLEN(countof(szDllErr)) L"\nCreateInstance(IID_IXMLDOMDocument) failed\nErrCode=0x%08X", (DWORD)hFact);
					pFact->Release();
				}
				else
				{
					_wsprintf(szDllErr, SKIPLEN(countof(szDllErr)) L"\nGetClassObject(CLSID_DOMDocument30) failed\nErrCode=0x%08X", (DWORD)hFact);
				}
			}
		}
	}

	if (FAILED(hr) || !pFile)
	{
		wchar_t szErr[512];
		_wsprintf(szErr, SKIPCOUNT(szErr)
				  L"XML setting file can not be used!\r\n"
				  L"Dynamic libraries 'msxml3.dll'/'msxml3r.dll' were not found!\r\n\r\n"
				  L"Can't create IID_IXMLDOMDocument!\r\n"
				  L"ErrCode=0x%08X %s", (DWORD)hr, szDllErr);
	  	pszErr = lstrdup(szErr);
		return NULL;
	}

	return pFile;
}

bool SettingsXML::IsXmlAllowed()
{
	bool bAllowed = false;
	wchar_t* pszErr = NULL;
	IXMLDOMDocument* pFile = CreateDomDocument(pszErr);

	if (pFile)
	{
		bAllowed = true;
		pFile->Release();
	}
	else if (pszErr)
	{
		static bool bWarned = false;
		if (!bWarned)
		{
			// Don't buzz more than one time
			bWarned = true;
			MBoxError(pszErr);
		}

	}

	SafeFree(pszErr);
	return bAllowed;
}

bool SettingsXML::OpenStorage(uint access, wchar_t*& pszErr)
{
	bool bRc = false;
	bool bNeedReopen = (mp_File == NULL);
	LPCWSTR pszXmlFile;
	HRESULT hr = S_OK;
	VARIANT_BOOL bSuccess;
	IXMLDOMParseError* pErr = NULL;
	VARIANT vt; VariantInit(&vt);
	bool bAllowCreate = (access & KEY_WRITE) == KEY_WRITE;

	_ASSERTE(pszErr == NULL);

	if (m_Storage.pszFile && *m_Storage.pszFile)
	{
		pszXmlFile = m_Storage.pszFile;
	}
	else
	{
		// Must be already initialized
		_ASSERTE(m_Storage.pszFile && *m_Storage.pszFile);
		pszXmlFile = gpConEmu->ConEmuXml();
		if (!pszXmlFile || !*pszXmlFile)
		{
			goto wrap;
		}
	}

	m_Storage.pszFile = pszXmlFile;

	// Changed access type?
	if (bNeedReopen || (mn_access != access))
	{
		DWORD dwAccess = GENERIC_READ;
		DWORD dwMode = OPEN_EXISTING;

		if ((access & KEY_WRITE) == KEY_WRITE)
		{
			dwAccess |= GENERIC_WRITE;
			dwMode = OPEN_ALWAYS;
		}

		HANDLE hFile = CreateFile(pszXmlFile, dwAccess, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, dwMode, 0, 0);

		// XML-файл отсутсвует
		if (!hFile || (hFile == INVALID_HANDLE_VALUE))
		{
			goto wrap;
		}
		else
		{
			BY_HANDLE_FILE_INFORMATION bfi = {0};
			if (GetFileInformationByHandle(hFile, &bfi))
			{
				mb_Empty = (bfi.nFileSizeHigh == 0 && bfi.nFileSizeLow == 0);
				if (!mb_Empty)
				{
					// UTF-8 BOM? Xml DOM does not allows BOM
					if (bfi.nFileSizeHigh == 0 && bfi.nFileSizeLow == 3)
					{
						BYTE bom[3] = {}; DWORD nRead = 0;
						if (ReadFile(hFile, bom, sizeof(bom), &nRead, NULL) && (nRead == sizeof(bom))
							&& (bom[0]==0xEF && bom[1]==0xBB && bom[2]==0xBF))
						{
							mb_Empty = true;
						}
					}
				}
			}
			CloseHandle(hFile); hFile = NULL;
			if (mb_Empty && bAllowCreate)
			{
				bNeedReopen = true;
				hFile = CreateFile(pszXmlFile, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 0, 0);
				if (!hFile || (hFile == INVALID_HANDLE_VALUE))
				{
					goto wrap;
				}
				else
				{
					LPCSTR pszDefault = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
						"<key name=\"Software\">\r\n"
						"\t<key name=\"ConEmu\">\r\n"
						"\t</key>\r\n"
						"</key>\r\n";
					DWORD nLen = lstrlenA(pszDefault);
					WriteFile(hFile, pszDefault, nLen, &nLen, NULL);
					CloseHandle(hFile);
				}
				hFile = NULL;
			}
		}
	}

	if (!bNeedReopen)
	{
		bRc = true;
		goto wrap;
	}

	if (mb_Empty && !bAllowCreate)
		goto wrap;

	CloseStorage();

	SAFETRY
	{
		_ASSERTE(mp_File == NULL);
		mp_File = CreateDomDocument(pszErr);

		if (FAILED(hr) || !mp_File)
		{
			goto wrap;
		}

		hr = mp_File->put_preserveWhiteSpace(VARIANT_TRUE);
		hr = mp_File->put_async(VARIANT_FALSE);

		// Загрузить xml-ку
		bSuccess = VARIANT_FALSE;
		vt.vt = VT_BSTR; vt.bstrVal = ::SysAllocString(pszXmlFile);
		hr = mp_File->load(vt, &bSuccess);
		VariantClear(&vt);

		if ((hr != S_OK) || !bSuccess)
		{
			BSTR bsXmlDescr = NULL;
			wchar_t* pszOurDescr = NULL;
			wchar_t szErr[100] = L"";
			long errorCode = 0; // Contains the error code of the last parse error. Read-only.
			long line = 0; // Specifies the line number that contains the error. Read-only.
			long linepos  = 0; // Contains the character position within the line where the error occurred. Read-only.

			hr = mp_File->get_parseError(&pErr);

			if (pErr)
			{
				DEBUGTEST(HRESULT hrd=)
				pErr->get_errorCode(&errorCode);

				DEBUGTEST(hrd=)
				pErr->get_line(&line);

				DEBUGTEST(hrd=)
				pErr->get_linepos(&linepos);

				_wsprintf(szErr, SKIPCOUNT(szErr) L"\r\n\r\n"
					L"Line=%i, Pos=%i, XmlCode=0x%08X, HR=0x%08X\r\n", line, linepos, (DWORD)errorCode, (DWORD)hr);

				// For some errors service returns weird error descriptions
				if (errorCode == 0xC00CE558)
				{
					pszOurDescr = lstrdup(L"Root node doesn't exists");
				}
				else
				{
					pErr->get_reason(&bsXmlDescr);
				}
			}

			pszErr = lstrmerge(
				((access & KEY_WRITE) != KEY_WRITE)
					? L"Failed to load configuration file!\r\n"
					: L"Failed to open configuration file for writing!\r\n",
				pszXmlFile, szErr, bsXmlDescr, pszOurDescr);

			if (bsXmlDescr)
				SysFreeString(bsXmlDescr);
			SafeFree(pszOurDescr);

			goto wrap;
		}

		mn_access = access;
		bRc = true;

	} SAFECATCH
	{
		pszErr = lstrmerge(L"Exception in SettingsXML::OpenStorage\r\n", pszXmlFile);
		bRc = false;
	}

wrap:
	SafeRelease(pErr);
	if (!bRc)
		CloseStorage();
	return bRc;
}

bool SettingsXML::OpenKey(const wchar_t *regPath, uint access, BOOL abSilent /*= FALSE*/)
{
	// That may occur if Basic settings and "Export" button was pressed
	_ASSERTE(!gpConEmu->IsResetBasicSettings() || ((access & KEY_WRITE)!=KEY_WRITE));

	bool lbRc = false;
	HRESULT hr = S_OK;
	wchar_t* pszErr = NULL;
	wchar_t szName[MAX_PATH];
	const wchar_t* psz = NULL;
	IXMLDOMNode* pKey = NULL;
	IXMLDOMNode* pChild = NULL;
	bool bAllowCreate = (access & KEY_WRITE) == KEY_WRITE;

	CloseKey(); // на всякий

	if (!regPath || !*regPath)
	{
		return false;
	}

	if (!OpenStorage(access, pszErr))
	{
		goto wrap;
	}

	SAFETRY
	{
		_ASSERTE(mp_File != NULL);

		hr = mp_File->QueryInterface(IID_IXMLDOMNode, (void **)&pKey);

		if (FAILED(hr))
		{
			wchar_t szRootError[80];
			_wsprintf(szRootError, SKIPLEN(countof(szRootError))
				L"XML: Root node not found! ErrCode=0x%08X\r\n", (DWORD)hr);
			pszErr = lstrmerge(szRootError, m_Storage.pszFile, L"\r\n", regPath);
			goto wrap;
		}

		mi_Level = 0;

		while (*regPath)
		{
			// Получить следующий токен
			psz = wcschr(regPath, L'\\');

			if (!psz) psz = regPath + _tcslen(regPath);

			lstrcpyn(szName, regPath, psz-regPath+1);
			// Найти в структуре XML
			pChild = FindItem(pKey, L"key", szName, bAllowCreate);
			pKey->Release();
			pKey = pChild; pChild = NULL;
			mi_Level++;

			if (!pKey)
			{
				if (bAllowCreate)
				{
					pszErr = lstrmerge(L"XML: Can't create key <", szName, L">!");
				}
				else
				{
					//_wsprintf(szErr, SKIPLEN(countof(szErr)) L"XML: key <%s> not found!", szName);
					// Don't show error - use default settings
					SafeFree(pszErr);
				}

				goto wrap;
			}

			if (*psz == L'\\')
			{
				regPath = psz + 1;
			}
			else
			{
				break;
			}
		}

		// Нашли, запомнили
		mp_Key = pKey; pKey = NULL;

		#if 0
		if (mp_Key)
		{
			SYSTEMTIME st; wchar_t szTime[32];
			GetLocalTime(&st);
			_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%04i-%02i-%02i %02i:%02i:%02i",
				st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
			SetAttr(mp_Key, L"modified", szTime);
			SetAttr(mp_Key, L"build", gpConEmu->ms_ConEmuBuild);
		}
		#endif

		lbRc = true;

	} SAFECATCH
	{
		pszErr = lstrmerge(L"Exception in SettingsXML::OpenKey\r\n", m_Storage.pszFile, L"\r\n", regPath);
		lbRc = false;
	}
wrap:
	SafeRelease(pChild);
	SafeRelease(pKey);

	if (!lbRc && (pszErr && *pszErr) && !abSilent)
	{
		// Don't show error message box as a child of ghWnd
		// otherwise it may be closed unexpectedly on exit
		MsgBox(pszErr, MB_ICONSTOP, NULL, NULL);
	}
	SafeFree(pszErr);

	return lbRc;
}

void SettingsXML::SetDataChanged()
{
	mb_DataChanged = true;
}

void SettingsXML::TouchKey(IXMLDOMNode* apKey)
{
	SYSTEMTIME st; wchar_t szTime[32];
	GetLocalTime(&st);
	_wsprintf(szTime, SKIPLEN(countof(szTime)) L"%04i-%02i-%02i %02i:%02i:%02i",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	SetAttr(apKey, L"modified", szTime);
	SetAttr(apKey, L"build", gpConEmu->ms_ConEmuBuild);
}

void SettingsXML::CloseStorage()
{
	HRESULT hr = S_OK;
	HANDLE hFile = NULL;
	bool bCanSave = false;

	CloseKey();

	if (mb_Modified && mp_File)
	{
		// Путь к файлу проинициализирован в OpenKey
		_ASSERTE(m_Storage.pszFile && *m_Storage.pszFile);
		LPCWSTR pszXmlFile = m_Storage.pszFile;

		if (pszXmlFile)
		{
			hFile = CreateFile(pszXmlFile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
			                   NULL, OPEN_EXISTING, 0, 0);

			// XML-файл отсутсвует, или ошибка доступа
			if (hFile == INVALID_HANDLE_VALUE)
			{
				DWORD dwErrCode = GetLastError();
				wchar_t szErr[MAX_PATH*2];
				_wsprintf(szErr, SKIPLEN(countof(szErr)) L"Can't open file for writing!\n%s\nErrCode=0x%08X",
				          pszXmlFile, dwErrCode);
				MBoxA(szErr);
			}
			else
			{
				CloseHandle(hFile); hFile = NULL;
				bCanSave = true;
			}

			if (bCanSave)
			{
				VARIANT vt; vt.vt = VT_BSTR; vt.bstrVal = ::SysAllocString(pszXmlFile);
				hr = mp_File->save(vt);
				VariantClear(&vt);
			}
		}
	}

	SafeRelease(mp_File);

	mb_Modified = false;
	mb_DataChanged = false;
	mb_Empty = false;
	mn_access = 0;
}

void SettingsXML::CloseKey()
{
	if (mp_Key && mb_DataChanged)
		TouchKey(mp_Key);
	SafeRelease(mp_Key);
	mb_KeyEmpty = false;
	mi_Level = 0;
}

BSTR SettingsXML::GetAttr(IXMLDOMNode* apNode, const wchar_t* asName)
{
	HRESULT hr = S_OK;
	BSTR bsValue = NULL;
	IXMLDOMNamedNodeMap* pAttrs = NULL;
	hr = apNode->get_attributes(&pAttrs);

	if (SUCCEEDED(hr) && pAttrs)
	{
		bsValue = GetAttr(apNode, pAttrs, asName);
		pAttrs->Release(); pAttrs = NULL;
	}

	return bsValue;
}

BSTR SettingsXML::GetAttr(IXMLDOMNode* apNode, IXMLDOMNamedNodeMap* apAttrs, const wchar_t* asName)
{
	HRESULT hr = S_OK;
	IXMLDOMNode *pValue = NULL;
	BSTR bsText = NULL;
	bsText = ::SysAllocString(asName);
	hr = apAttrs->getNamedItem(bsText, &pValue);
	::SysFreeString(bsText); bsText = NULL;

	if (SUCCEEDED(hr) && pValue)
	{
		hr = pValue->get_text(&bsText);
		pValue->Release(); pValue = NULL;
	}

	return bsText;
}

bool SettingsXML::SetAttr(IXMLDOMNode* apNode, const wchar_t* asName, const wchar_t* asValue)
{
	bool lbRc = false;
	HRESULT hr = S_OK;
	IXMLDOMNamedNodeMap* pAttrs = NULL;
	hr = apNode->get_attributes(&pAttrs);

	if (SUCCEEDED(hr) && pAttrs)
	{
		lbRc = SetAttr(apNode, pAttrs, asName, asValue);
		pAttrs->Release(); pAttrs = NULL;
	}

	return lbRc;
}

bool SettingsXML::SetAttr(IXMLDOMNode* apNode, IXMLDOMNamedNodeMap* apAttrs, const wchar_t* asName, const wchar_t* asValue)
{
	bool lbRc = false;
	HRESULT hr = S_OK;
	IXMLDOMNode *pValue = NULL;
	IXMLDOMAttribute *pIXMLDOMAttribute = NULL;
	BSTR bsText = NULL;
	BSTR bsCurValue = NULL;
	bsText = ::SysAllocString(asName);
	hr = apAttrs->getNamedItem(bsText, &pValue);

	BSTR bsValue = ::SysAllocString(asValue);
	wchar_t* pszEsc = wcschr(bsValue, (wchar_t)27);
	if (pszEsc != NULL)
	{
		_ASSERTE(wcschr(bsValue, (wchar_t)27) == NULL); // У DOM сносит крышу, если писать "ESC" в значение
		while ((pszEsc = wcschr(bsValue, (wchar_t)27)) != NULL)
		{
			*pszEsc = L'?';
		}
	}

	if (FAILED(hr) || !pValue)
	{
		hr = mp_File->createAttribute(bsText, &pIXMLDOMAttribute);
		_ASSERTE(hr == S_OK);

		if (SUCCEEDED(hr) && pIXMLDOMAttribute)
		{
			VARIANT vtValue; vtValue.vt = VT_BSTR; vtValue.bstrVal = bsValue;
			hr = pIXMLDOMAttribute->put_nodeValue(vtValue);
			_ASSERTE(hr == S_OK);
			hr = apAttrs->setNamedItem(pIXMLDOMAttribute, &pValue); //-V519
			_ASSERTE(hr == S_OK);
			lbRc = SUCCEEDED(hr);
			SetDataChanged();
		}
	}
	else if (SUCCEEDED(hr) && pValue)
	{
		// Для проверки mb_DataChanged
		hr = pValue->get_text(&bsCurValue);
		if (SUCCEEDED(hr) && bsCurValue && bsValue
			&& (wcscmp(bsValue, bsCurValue) == 0))
		{
			// Not changed
			lbRc = true;
		}
		else
		{
			// Change the value
			hr = pValue->put_text(bsValue);
			_ASSERTE(hr == S_OK);
			lbRc = SUCCEEDED(hr);
			SetDataChanged();
		}
	}

	::SysFreeString(bsText); bsText = NULL;
	::SysFreeString(bsValue); bsValue = NULL;
	if (bsCurValue) { ::SysFreeString(bsCurValue); bsCurValue = NULL; }

	if (pValue) { pValue->Release(); pValue = NULL; }

	if (pIXMLDOMAttribute) { pIXMLDOMAttribute->Release(); pIXMLDOMAttribute = NULL; }

	return lbRc;
}

// Indenting XML keys using DOM
void SettingsXML::AppendIndent(IXMLDOMNode* apFrom, int nLevel)
{
	if (nLevel<=0) return;

	int nMax = min(32,nLevel); //-V112
	wchar_t szIndent[34];

	for(int i=0; i<nMax; i++) szIndent[i] = L'\t';

	szIndent[nMax] = 0;
	BSTR bsText = ::SysAllocString(szIndent);
	AppendText(apFrom, bsText);
	SysFreeString(bsText);
}

void SettingsXML::AppendNewLine(IXMLDOMNode* apFrom)
{
	BSTR bsText = SysAllocString(L"\r\n");
	AppendText(apFrom, bsText);
	SysFreeString(bsText);
}

void SettingsXML::AppendText(IXMLDOMNode* apFrom, BSTR asText)
{
	if (!asText || !*asText)
		return;

	VARIANT vtType;
	IXMLDOMNode* pChild = NULL;
	IXMLDOMNode *pIXMLDOMNode = NULL;
	vtType.vt = VT_I4; vtType.lVal = NODE_TEXT;
	HRESULT hr = mp_File->createNode(vtType, L"", L"", &pChild);

	if (SUCCEEDED(hr) && pChild)
	{
		hr = pChild->put_text(asText);
		hr = apFrom->appendChild(pChild, &pIXMLDOMNode); //-V519
		pChild->Release(); pChild = NULL;

		if (SUCCEEDED(hr) && pIXMLDOMNode)
		{
			pIXMLDOMNode->Release(); pIXMLDOMNode = NULL;
		}
	}
}

IXMLDOMNode* SettingsXML::FindItem(IXMLDOMNode* apFrom, const wchar_t* asType, const wchar_t* asName, bool abAllowCreate)
{
	HRESULT hr = S_OK;
	IXMLDOMNodeList* pList = NULL;
	IXMLDOMNode* pChild = NULL;
	IXMLDOMNamedNodeMap* pAttrs = NULL;
	IXMLDOMAttribute *pIXMLDOMAttribute = NULL;
	IXMLDOMNode *pIXMLDOMNode = NULL;
	IXMLDOMNode *pName = NULL;
	BSTR bsText = NULL;
	BSTR bsCheck = NULL;
	DOMNodeType nodeTypeCheck = NODE_INVALID;
	BOOL lbEmpty = TRUE;
	int iLastIndent = 1;

	// Получить все дочерние элементы нужного типа
	if (apFrom == NULL)
	{
		hr = S_FALSE;
	}
	else
	{
		long lFound = 0;
		// key[@name="abc"], but it is case-sensitive, and may fail in theory
		bsText = lstrmerge(asType, L"[@name=\"", asName, L"\"]");
		hr = apFrom->selectNodes(bsText, &pList);
		if (SUCCEEDED(hr) && pList)
		{
			hr = pList->get_length(&lFound);
			if (FAILED(hr) || (lFound < 1))
			{
				SafeRelease(pList);
			}
		}
		SafeFree(bsText);
		// May be case-insensitive search will be succeeded?
		// However, it is very slow
		if (!pList)
		{
			bsText = ::SysAllocString(asType);
			hr = apFrom->selectNodes(bsText, &pList);
			::SysFreeString(bsText); bsText = NULL;
		}
	}

	if (SUCCEEDED(hr) && pList)
	{
		hr = pList->reset();

		while ((hr = pList->nextNode(&pIXMLDOMNode)) == S_OK && pIXMLDOMNode)
		{
			lbEmpty = FALSE;
			hr = pIXMLDOMNode->get_attributes(&pAttrs);

			if (SUCCEEDED(hr) && pAttrs)
			{
				bsText = GetAttr(pIXMLDOMNode, pAttrs, L"name");

				if (bsText)
				{
					if (lstrcmpi(bsText, asName) == 0)
					{
						::SysFreeString(bsText); bsText = NULL;
						pChild = pIXMLDOMNode; pIXMLDOMNode = NULL;
						break;
					}

					::SysFreeString(bsText); bsText = NULL;
				}
			}

			pIXMLDOMNode->Release(); pIXMLDOMNode = NULL;
		}

		pList->Release();
		//pList = NULL; -- для отладки
	}

	if (lbEmpty && abAllowCreate && (asType[0] == L'k'))
	{
		bsText = ::SysAllocString(L"value");
		hr = apFrom->selectNodes(bsText, &pList);
		::SysFreeString(bsText); bsText = NULL;
		if (SUCCEEDED(hr) && pList)
		{
			hr = pList->reset();
			if ((hr = pList->nextNode(&pIXMLDOMNode)) == S_OK && pIXMLDOMNode)
			{
				lbEmpty = FALSE;
				pIXMLDOMNode->Release(); pIXMLDOMNode = NULL;
			}
			pList->Release();
			//pList = NULL; -- для отладки
		}
	}

	if (!pChild && abAllowCreate)
	{
		if (asType[0] == L'k')
		{
			hr = apFrom->get_lastChild(&pChild);
			if (SUCCEEDED(hr) && pChild)
			{
				hr = pChild->get_nodeType(&nodeTypeCheck);
				if (SUCCEEDED(hr) && (nodeTypeCheck == NODE_TEXT))
				{
					hr = pChild->get_text(&bsCheck);
					if (SUCCEEDED(hr) && bsCheck)
					{
						iLastIndent = 0;
						LPCWSTR pszTabs = bsCheck;
						while (*pszTabs)
						{
							if (*(pszTabs++) == L'\t')
								iLastIndent++;
						}
						::SysFreeString(bsCheck); bsCheck = NULL;
					}
				}
			}
			SafeRelease(pChild);
		}

		VARIANT vtType; vtType.vt = VT_I4;
		vtType.lVal = NODE_ELEMENT;
		bsText = ::SysAllocString(asType);
		hr = mp_File->createNode(vtType, bsText, L"", &pChild);
		::SysFreeString(bsText); bsText = NULL;

		if (SUCCEEDED(hr) && pChild)
		{
			if (SetAttr(pChild, L"name", asName))
			{
				if (asType[0] == L'k')
				{
					AppendNewLine(pChild);
					mb_KeyEmpty = true;
					TouchKey(pChild);
				}

				if (asType[0] == L'k')
				{
					//if (mb_KeyEmpty)
					//AppendIndent(apFrom, lbEmpty ? (mi_Level-1) : mi_Level);
					AppendIndent(apFrom, (mi_Level-iLastIndent));
				}
				else if (mb_KeyEmpty)
				{
					AppendIndent(apFrom, !lbEmpty ? (mi_Level-1) : mi_Level);
				}
				else
				{
					AppendIndent(apFrom, 1);
				}
				hr = apFrom->appendChild(pChild, &pIXMLDOMNode);
				pChild->Release(); pChild = NULL;

				if (FAILED(hr))
				{
					pAttrs->Release(); pAttrs = NULL;
				}
				else
				{
					pChild = pIXMLDOMNode;
					pIXMLDOMNode = NULL;
				}

				AppendNewLine(apFrom);
				AppendIndent(apFrom, mi_Level-1);
				if ((asType[0] != L'k') && mb_KeyEmpty)
					mb_KeyEmpty = false;
			}
			else
			{
				pChild->Release(); pChild = NULL;
			}
		}
	}

	return pChild;
}

// эта функция, если значения нет (или тип некорректный) *value НЕ трогает
bool SettingsXML::Load(const wchar_t *regName, wchar_t **value)
{
	bool lbRc = false;
	HRESULT hr = S_OK;
	IXMLDOMNode* pChild = NULL;
	IXMLDOMNamedNodeMap* pAttrs = NULL;
	IXMLDOMAttribute *pIXMLDOMAttribute = NULL;
	IXMLDOMNode *pNode = NULL;
	IXMLDOMNodeList* pList = NULL;
	BSTR bsType = NULL;
	BSTR bsData = NULL;
	size_t nLen = 0;

	//if (*value) {free(*value); *value = NULL;}

	if (mp_Key)
		pChild = FindItem(mp_Key, L"value", regName, false);

	if (!pChild)
		return false;

	hr = pChild->get_attributes(&pAttrs);

	if (SUCCEEDED(hr) && pAttrs)
	{
		bsType = GetAttr(pChild, pAttrs, L"type");
	}

	if (SUCCEEDED(hr) && bsType)
	{
		if (!lstrcmpi(bsType, L"multi"))
		{
			// Тут значения хранятся так:
			//<value name="CmdLineHistory" type="multi">
			//	<line data="C:\Far\Far.exe"/>
			//	<line data="cmd"/>
			//</value>
			wchar_t *pszData = NULL, *pszCur = NULL;
			size_t nMaxLen = 0, nCurLen = 0;
			long nCount = 0;

			if (pAttrs) { pAttrs->Release(); pAttrs = NULL; }

			// Получить все дочерние элементы нужного типа
			bsData = ::SysAllocString(L"line");
			hr = pChild->selectNodes(bsData, &pList);
			::SysFreeString(bsData); bsData = NULL;

			if (SUCCEEDED(hr) && pList)
			{
				hr = pList->get_length(&nCount);

				if (SUCCEEDED(hr) && (nCount > 0))
				{
					HEAPVAL;
					nMaxLen = ((MAX_PATH+1) * nCount) + 1;
					pszData = (wchar_t*)malloc(nMaxLen * sizeof(wchar_t));
					pszCur = pszData;
					pszCur[0] = 0; pszCur[1] = 0;
					nCurLen = 2; // сразу посчитать DoubleZero
					HEAPVAL;
				}
			}

			if (SUCCEEDED(hr) && pList)
			{
				hr = pList->reset();

				while ((hr = pList->nextNode(&pNode)) == S_OK && pNode)
				{
					bsData = GetAttr(pNode, L"data");
					pNode->Release(); pNode = NULL;

					if (SUCCEEDED(hr) && bsData)
					{
						nLen = _tcslen(bsData) + 1;

						if ((nCurLen + nLen) > nMaxLen)
						{
							// Нужно пересоздать!
							nMaxLen = nCurLen + nLen + MAX_PATH + 1;
							wchar_t *psz = (wchar_t*)malloc(nMaxLen * sizeof(wchar_t));
							_ASSERTE(psz);

							if (!psz) break;  // Не удалось выделить память!

							wmemmove(psz, pszData, nCurLen);
							pszCur = psz + (pszCur - pszData);
							HEAPVAL;
							free(pszData);
							pszData = psz;
							HEAPVAL;
						}

						lstrcpy(pszCur, bsData);
						pszCur += nLen; // указатель - на место для следующей строки
						nCurLen += nLen;
						*pszCur = 0; // ASCIIZZ
						HEAPVAL;
						::SysFreeString(bsData); bsData = NULL;
					}
				}

				pList->Release(); pList = NULL;
			}

			// значит что-то прочитать удалось
			if (pszData)
			{
				if (*value) {free(*value); *value = NULL;}
				*value = pszData;
				lbRc = true;
			}
		}
		else if (!lstrcmpi(bsType, L"string"))
		{
			bsData = GetAttr(pChild, pAttrs, L"data");

			if (SUCCEEDED(hr) && bsData)
			{
				nLen = _tcslen(bsData);
				if (!*value || (_tcslen(*value) <= nLen))
				{
					*value = (wchar_t*)realloc(*value, (nLen+2)*sizeof(wchar_t));
				}
				if (*value)
				{
					lstrcpy(*value, bsData);
					(*value)[nLen] = 0; // уже должен быть после lstrcpy
					(*value)[nLen+1] = 0; // ASCIIZZ
					lbRc = true;
				}
			}
		}

		// Все остальные типы - не интересуют. Нам нужны только строки
	}

	if (bsType) { ::SysFreeString(bsType); bsType = NULL; }

	if (bsData) { ::SysFreeString(bsData); bsData = NULL; }

	if (pChild) { pChild->Release(); pChild = NULL; }

	if (pAttrs) { pAttrs->Release(); pAttrs = NULL; }

	//if (!lbRc)
	//{
	//	_ASSERTE(*value == NULL);
	//	*value = (wchar_t*)malloc(sizeof(wchar_t)*2);
	//	(*value)[0] = 0; (*value)[1] = 0; // На случай REG_MULTI_SZ
	//}

	return lbRc;
}
// эта функция, если значения нет (или тип некорректный) value НЕ трогает
bool SettingsXML::Load(const wchar_t *regName, wchar_t *value, int maxLen)
{
	_ASSERTE(maxLen>1);
	bool lbRc = false;
	wchar_t* pszValue = NULL;

	if (Load(regName, &pszValue))
	{
		if (pszValue)
			lstrcpyn(value, pszValue, maxLen);
		else
			value[0] = 0;

		lbRc = true;
	}

	if (pszValue) free(pszValue);

	return lbRc;
}
bool SettingsXML::Load(const wchar_t *regName, LPBYTE value, DWORD nSize)
{
	bool lbRc = false;
	HRESULT hr = S_OK;
	IXMLDOMNode* pChild = NULL;
	IXMLDOMNamedNodeMap* pAttrs = NULL;
	IXMLDOMAttribute *pIXMLDOMAttribute = NULL;
	IXMLDOMNode *pNode = NULL;
	BSTR bsType = NULL;
	BSTR bsData = NULL;

	if (!value || !nSize)
		return false;

	if (mp_Key)
		pChild = FindItem(mp_Key, L"value", regName, false);

	if (!pChild)
		return false;

	hr = pChild->get_attributes(&pAttrs);

	if (SUCCEEDED(hr) && pAttrs)
	{
		bsType = GetAttr(pChild, pAttrs, L"type");
	}

	if (SUCCEEDED(hr) && bsType)
	{
		bsData = GetAttr(pChild, pAttrs, L"data");
	}

	if (SUCCEEDED(hr) && bsData)
	{
		if (!lstrcmpi(bsType, L"string"))
		{
			#ifdef _DEBUG
			DWORD nLen = _tcslen(bsData) + 1;
			#endif
			DWORD nMaxLen = nSize / 2;
			lstrcpyn((wchar_t*)value, bsData, nMaxLen);
			lbRc = true;
		}
		else if (!lstrcmpi(bsType, L"ulong"))
		{
			wchar_t* pszEnd = NULL;
			DWORD lVal = wcstoul(bsData, &pszEnd, 10);

			if (nSize > 4) nSize = 4;

			if (pszEnd && pszEnd != bsData)
			{
				memmove(value, &lVal, nSize);
				lbRc = true;
			}
		}
		else if (!lstrcmpi(bsType, L"long"))
		{
			wchar_t* pszEnd = NULL;
			int lVal = wcstol(bsData, &pszEnd, 10);

			if (nSize > 4) nSize = 4;

			if (pszEnd && pszEnd != bsData)
			{
				memmove(value, &lVal, nSize);
				lbRc = true;
			}
		}
		else if (!lstrcmpi(bsType, L"dword"))
		{
			wchar_t* pszEnd = NULL;
			DWORD lVal = wcstoul(bsData, &pszEnd, 16);

			if (nSize > 4) nSize = 4;

			if (pszEnd && pszEnd != bsData)
			{
				memmove(value, &lVal, nSize);
				lbRc = true;
			}
		}
		else if (!lstrcmpi(bsType, L"hex"))
		{
			wchar_t* pszCur = bsData;
			wchar_t* pszEnd = NULL;
			LPBYTE pCur = value;
			wchar_t cHex;
			DWORD lVal = 0;
			lbRc = true;

			while (*pszCur && nSize)
			{
				lVal = 0;
				cHex = *(pszCur++);

				if (cHex >= L'0' && cHex <= L'9')
				{
					lVal = cHex - L'0';
				}
				else if (cHex >= L'a' && cHex <= L'f')
				{
					lVal = cHex - L'a' + 10;
				}
				else if (cHex >= L'A' && cHex <= L'F')
				{
					lVal = cHex - L'A' + 10;
				}
				else
				{
					lbRc = false; break;
				}

				cHex = *(pszCur++);

				if (cHex && cHex != L',')
				{
					lVal = lVal << 4;

					if (cHex >= L'0' && cHex <= L'9')
					{
						lVal |= cHex - L'0';
					}
					else if (cHex >= L'a' && cHex <= L'f')
					{
						lVal |= cHex - L'a' + 10;
					}
					else if (cHex >= L'A' && cHex <= L'F')
					{
						lVal |= cHex - L'A' + 10;
					}
					else
					{
						lbRc = false; break;
					}

					cHex = *(pszCur++);
				}

				*pCur = (BYTE)lVal;
				pCur++; nSize--;

				if (cHex != L',')
				{
					break;
				}
			}

			while (nSize--)  // очистить хвост
				*(pCur++) = 0;
		}
	}

	// Остальные типы (строки) - не интересуют

	if (bsType) { ::SysFreeString(bsType); bsType = NULL; }

	if (bsData) { ::SysFreeString(bsData); bsData = NULL; }

	if (pChild) { pChild->Release(); pChild = NULL; }

	if (pAttrs) { pAttrs->Release(); pAttrs = NULL; }

	return lbRc;
}

// Очистка содержимого параметра REG_MULTI_SZ
void SettingsXML::Delete(const wchar_t *regName)
{
	Save(regName, NULL, REG_MULTI_SZ, 0);
}

bool SettingsXML::SetMultiLine(IXMLDOMNode* apNode, const wchar_t* asValue, long nAllLen)
{
	bool bRc = false;
	HRESULT hr;
	VARIANT_BOOL bHasChild = VARIANT_FALSE;
	DOMNodeType  nodeType = NODE_INVALID;
	BSTR bsNodeType = ::SysAllocString(L"line");
	VARIANT vtType; vtType.vt = VT_I4; vtType.lVal = NODE_ELEMENT;
	IXMLDOMNode* pNode = NULL;
	IXMLDOMNode* pNodeRmv = NULL;
	long nLen = 0;
	const wchar_t* psz = asValue;
	bool bNewNodeCreate = false;
	int iAddIndent = 1;
	bool bAddFirstNewLine = false;

	hr = apNode->get_firstChild(&pNode);
	if (pNode == NULL)
	{
		bAddFirstNewLine = true;
		iAddIndent = mi_Level + 1;
	}

	// Обновляем существующие
	while ((psz && *psz && nAllLen > 0) && (SUCCEEDED(hr) && pNode))
	{
		hr = pNode->get_nodeType(&nodeType);

		if (SUCCEEDED(hr) && (nodeType == NODE_ELEMENT))
		{
			if (!SetAttr(pNode, L"data", psz))
				goto wrap;

			nLen = _tcslen(psz)+1;
			psz += nLen;
			nAllLen -= nLen;
		}

		hr = pNode->get_nextSibling(&pNode);
	}

	// Добавляем что осталось
	while (psz && *psz && nAllLen > 0)
	{
		hr = mp_File->createNode(vtType, bsNodeType, L"", &pNode);

		if (FAILED(hr) || !pNode)
			goto wrap;

		if (!SetAttr(pNode, L"data", psz))
			goto wrap;

		if (bAddFirstNewLine)
		{
			AppendNewLine(apNode);
			bAddFirstNewLine = false;
		}
		AppendIndent(apNode, iAddIndent);

		hr = apNode->appendChild(pNode, &pNodeRmv);

		AppendNewLine(apNode);
		bNewNodeCreate = true;
		iAddIndent = mi_Level + 1;

		SafeRelease(pNode);
		SafeRelease(pNodeRmv);

		if (FAILED(hr))
			goto wrap;

		nLen = _tcslen(psz)+1;
		psz += nLen;
		nAllLen -= nLen;
	}

	// Очистить хвост (если элементов стало меньше)
	if (pNode)
	{
		ClearChildrenTail(apNode, pNode);
		AppendNewLine(apNode);
		bNewNodeCreate = true;
	}

	if (bNewNodeCreate)
	{
		AppendIndent(apNode, mi_Level);
	}

	bRc = true;
wrap:
	_ASSERTE(nAllLen <= 1);
	::SysFreeString(bsNodeType);
	return bRc;
}

void SettingsXML::ClearChildrenTail(IXMLDOMNode* apNode, IXMLDOMNode* apFirstClear)
{
	HRESULT hr;
	IXMLDOMNode* pNext = NULL;
	IXMLDOMNode *pNodeRmv = NULL;
	DOMNodeType  nodeType = NODE_INVALID;

	#ifdef _DEBUG
	BSTR bsDebug = NULL;
	if (pNext)
	{
		pNext->get_nodeType(&nodeType);
		pNext->get_xml(&bsDebug);
	}
	if (bsDebug) ::SysFreeString(bsDebug); bsDebug = NULL;
	#endif

	while (((hr = apFirstClear->get_nextSibling(&pNext)) == S_OK) && pNext)
	{
		pNext->get_nodeType(&nodeType);

		#ifdef _DEBUG
		pNext->get_xml(&bsDebug);
		if (bsDebug) ::SysFreeString(bsDebug); bsDebug = NULL;
		#endif

		hr = apNode->removeChild(pNext, &pNodeRmv);
		if (SUCCEEDED(hr) && (nodeType != NODE_TEXT))
			SetDataChanged();

		SafeRelease(pNodeRmv);
		SafeRelease(pNext);
	}

	apFirstClear->get_nodeType(&nodeType);
	hr = apNode->removeChild(apFirstClear, &pNodeRmv);
	if (SUCCEEDED(hr) && (nodeType != NODE_TEXT))
		SetDataChanged();

	SafeRelease(pNodeRmv);
	SafeRelease(apFirstClear);
}

//void SettingsXML::Save(const wchar_t *regName, const wchar_t *value)
//{
//	if (!value) value = L"";  // сюда мог придти и NULL
//
//	Save(regName, (LPCBYTE)value, REG_SZ, (_tcslen(value)+1)*sizeof(wchar_t));
//}
void SettingsXML::Save(const wchar_t *regName, LPCBYTE value, DWORD nType, DWORD nSize)
{
	HRESULT hr = S_OK;
	IXMLDOMNamedNodeMap* pAttrs = NULL;
	IXMLDOMNodeList* pList = NULL;
	IXMLDOMAttribute *pIXMLDOMAttribute = NULL;
	IXMLDOMNode *pNode = NULL;
	IXMLDOMNode* pChild = NULL;
	IXMLDOMNode *pNodeRmv = NULL;
	BSTR bsValue = NULL;
	BSTR bsType = NULL;
	bool bNeedSetType = false;
	// nType:
	// REG_DWORD:    сохранение числа в 16-ричном или 10-чном формате, в зависимости от того, что сейчас указано в xml ("dword"/"ulong"/"long")
	// REG_BINARY:   строго в hex (FF,FF,...)
	// REG_SZ:       ASCIIZ строка, можно проконтролировать, чтобы nSize/2 не был меньше длины строки
	// REG_MULTI_SZ: ASCIIZZ. При формировании <list...> нужно убедиться, что мы не вылезли за пределы nSize
	pChild = FindItem(mp_Key, L"value", regName, true); // создать, если его еще нету

	if (!pChild)
		goto wrap;

	hr = pChild->get_attributes(&pAttrs);

	if (FAILED(hr) || !pAttrs)
		goto wrap;

	bsType = GetAttr(pChild, pAttrs, L"type");

	switch (nType)
	{
		case REG__LONG:
		case REG__ULONG:
		case REG_DWORD:
		{
			wchar_t szValue[32];

			if (bsType && (bsType[0] == L'u' || bsType[0] == L'U'))
			{
				_wsprintf(szValue, SKIPLEN(countof(szValue)) L"%u", *(LPDWORD)value);
			}
			else if (bsType && (bsType[0] == L'l' || bsType[0] == L'L'))
			{
				_wsprintf(szValue, SKIPLEN(countof(szValue)) L"%i", *(int*)value);
			}
			else
			{
				LPCWSTR pszTypeName;
				if (nType == REG_DWORD)
				{
					_wsprintf(szValue, SKIPLEN(countof(szValue)) L"%08x", *(LPDWORD)value);
					pszTypeName = L"dword";
				}
				else if (nType == REG__ULONG)
				{
					_wsprintf(szValue, SKIPLEN(countof(szValue)) L"%u", *(UINT*)value);
					pszTypeName = L"ulong";
				}
				else
				{
					_wsprintf(szValue, SKIPLEN(countof(szValue)) L"%i", *(int*)value);
					pszTypeName = L"long";
				}

				if (!bsType || lstrcmp(bsType, pszTypeName))
				{
					// add or refresh the type
					::SysFreeString(bsType);
					bsType = ::SysAllocString(pszTypeName);
					bNeedSetType = true;
				}
			}

			bsValue = ::SysAllocString(szValue);
		} break;
		case REG_BINARY:
		{
			if (nSize == 1 && bsType && (bsType[0] == L'u' || bsType[0] == L'U'))
			{
				wchar_t szValue[4];
				BYTE bt = *value;
				_wsprintf(szValue, SKIPLEN(countof(szValue)) L"%u", (DWORD)bt);
				bsValue = ::SysAllocString(szValue);
			}
			else if (nSize == 1 && bsType && (bsType[0] == L'l' || bsType[0] == L'L'))
			{
				wchar_t szValue[4];
				char bt = *value;
				_wsprintf(szValue, SKIPLEN(countof(szValue)) L"%i", (int)bt);
				bsValue = ::SysAllocString(szValue);
			}
			else
			{
				DWORD nLen = nSize*2 + (nSize-1); // по 2 символа на байт + ',' между ними
				bsValue = ::SysAllocStringLen(NULL, nLen);
				nLen ++; // Чтобы далее не добавлять WCHAR на '\0'
				wchar_t* psz = (wchar_t*)bsValue;
				LPCBYTE  ptr = value;

				while (nSize)
				{
					_wsprintf(psz, SKIPLEN(nLen-(psz-bsValue)) L"%02x", (DWORD)*ptr);
					ptr++; nSize--; psz+=2;

					if (nSize)
						*(psz++) = L',';
				}

				if (bsType && lstrcmp(bsType, L"hex"))
				{
					// Допустим только "hex"
					::SysFreeString(bsType); bsType = NULL;
				}

				if (!bsType)
				{
					// нужно добавить/установить тип
					bsType = ::SysAllocString(L"hex"); bNeedSetType = true;
				}
			}
		} break;
		case REG_SZ:
		{
			wchar_t* psz = (wchar_t*)value;
			bsValue = ::SysAllocString(psz);

			if (bsType && lstrcmp(bsType, L"string"))
			{
				// Допустим только "string"
				::SysFreeString(bsType); bsType = NULL;
			}

			if (!bsType)
			{
				// нужно добавить/установить тип
				bsType = ::SysAllocString(L"string"); bNeedSetType = true;
			}
		} break;
		case REG_MULTI_SZ:
		{
			if (bsType && lstrcmp(bsType, L"multi"))
			{
				// Допустим только "multi"
				::SysFreeString(bsType); bsType = NULL;
			}

			if (!bsType)
			{
				// нужно добавить/установить тип
				bsType = ::SysAllocString(L"multi"); bNeedSetType = true;
			}
		} break;
		default:
			goto wrap; // не поддерживается
	}

	if (bNeedSetType)
	{
		_ASSERTE(bsType!=NULL);
		SetAttr(pChild, pAttrs, L"type", bsType);
		::SysFreeString(bsType); bsType = NULL;
	}

	// Теперь собственно значение
	if (nType != REG_MULTI_SZ)
	{
		_ASSERTE(bsValue != NULL);
		SetAttr(pChild, pAttrs, L"data", bsValue);
		::SysFreeString(bsValue); bsValue = NULL;
	}
	else     // Тут нужно формировать список элементов <list>
	{
		// Если ранее был параметр "data" - удалить его из списка атрибутов
		hr = pAttrs->getNamedItem(L"data", &pNode);
		if (SUCCEEDED(hr) && pNode)
		{
			hr = pChild->removeChild(pNode, &pNodeRmv);
			pNode->Release(); pNode = NULL;
			SetDataChanged();

			if (pNodeRmv) { pNodeRmv->Release(); pNodeRmv = NULL; }
		}

		long nAllLen = nSize/2; // длина в wchar_t
		wchar_t* psz = (wchar_t*)value;
		SetMultiLine(pChild, psz, nAllLen);
	}

	mb_Modified = true;
wrap:

	if (pIXMLDOMAttribute) { pIXMLDOMAttribute->Release(); pIXMLDOMAttribute = NULL; }

	if (pNode) { pNode->Release(); pNode = NULL; }

	if (pNodeRmv) { pNodeRmv->Release(); pNodeRmv = NULL; }

	if (pChild) { pChild->Release(); pChild = NULL; }

	if (pAttrs) { pAttrs->Release(); pAttrs = NULL; }

	if (bsValue) { ::SysFreeString(bsValue); bsValue = NULL; }

	if (bsType) { ::SysFreeString(bsType); bsType = NULL; }
}
#endif
