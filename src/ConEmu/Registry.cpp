
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

#include <windows.h>
#include "header.h"
#include "registry.h"


const CLSID CLSID_DOMDocument30 = {0xf5078f32, 0xc551, 0x11d3, {0x89, 0xb9, 0x00, 0x00, 0xf8, 0x1f, 0xe2, 0x21}};



SettingsRegistry::SettingsRegistry()
{
	regMy = NULL;
}
SettingsRegistry::~SettingsRegistry()
{
	CloseKey();
}



bool SettingsRegistry::OpenKey(HKEY inHKEY, const wchar_t *regPath, uint access)
{
	bool res = false;
	if (access == KEY_READ)
		res = RegOpenKeyEx(inHKEY, regPath, 0, KEY_READ, &regMy) == ERROR_SUCCESS;
	else
		res = RegCreateKeyEx(inHKEY, regPath, 0, NULL, 0, access, 0, &regMy, 0) == ERROR_SUCCESS;
	return res;
}
bool SettingsRegistry::OpenKey(const wchar_t *regPath, uint access)
{
	return OpenKey(HKEY_CURRENT_USER, regPath, access);
}
void SettingsRegistry::CloseKey()
{
	if (regMy) {
		RegCloseKey(regMy);
		regMy = NULL;
	}
}



bool SettingsRegistry::Load(const wchar_t *regName, LPBYTE value, DWORD nSize)
{
	_ASSERTE(nSize>0);
	if (RegQueryValueEx(regMy, regName, NULL, NULL, (LPBYTE)value, &nSize) == ERROR_SUCCESS)
		return true;
	return false;
}
bool SettingsRegistry::Load(const wchar_t *regName, wchar_t **value, LPDWORD pnSize /*= NULL*/)
{
	DWORD len = 0;
	if (*value) {free(*value); *value = NULL;}
	if (RegQueryValueExW(regMy, regName, NULL, NULL, NULL, &len) == ERROR_SUCCESS && len) {
		int nChLen = len/2;
		*value = (wchar_t*)malloc((nChLen+2)*sizeof(wchar_t));
		(*value)[nChLen] = 0; (*value)[nChLen+1] = 0;
		if (RegQueryValueExW(regMy, regName, NULL, NULL, (LPBYTE)(*value), &len) == ERROR_SUCCESS) {
			if (pnSize) *pnSize = len+1;
			return true;
		}
	} else {
		*value = (wchar_t*)malloc(sizeof(wchar_t)*2);
	}
	(*value)[0] = 0; (*value)[1] = 0; // На случай REG_MULTI_SZ
	return false;
}



void SettingsRegistry::Delete(const wchar_t *regName)
{
	RegDeleteValue(regMy, regName);
}



void SettingsRegistry::Save(const wchar_t *regName, LPCBYTE value, DWORD nType, DWORD nSize)
{
	_ASSERTE(value && nSize);
	RegSetValueEx(regMy, regName, NULL, nType, (LPBYTE)value, nSize); 
}
void SettingsRegistry::Save(const wchar_t *regName, const wchar_t *value)
{
	if (!value) value = _T(""); // сюда мог придти и NULL
	RegSetValueEx(regMy, regName, NULL, REG_SZ, (LPBYTE)value, (lstrlenW(value)+1) * sizeof(wchar_t));
}









/* *************************** */
SettingsXML::SettingsXML()
{
	mp_File = NULL; mp_Key = NULL;
}
SettingsXML::~SettingsXML()
{
	CloseKey();
}

bool SettingsXML::OpenKey(const wchar_t *regPath, uint access)
{
	bool lbRc = false;
	HRESULT hr = S_OK;
	wchar_t szErr[512]; szErr[0] = 0;
	wchar_t szName[MAX_PATH];
	const wchar_t* psz = NULL;
	VARIANT_BOOL bSuccess;
	IXMLDOMParseError* pErr = NULL;
	//IXMLDOMNodeList* pList = NULL;
	IXMLDOMNode* pKey = NULL;
	IXMLDOMNode* pChild = NULL;
	VARIANT vt; VariantInit(&vt);
	bool bAllowCreate = (access & KEY_WRITE) == KEY_WRITE;
	
	CloseKey(); // на всякий
	
	if (!regPath || !*regPath)
		return false;
	
	__try {
		hr = CoInitialize(NULL); 
		hr = CoCreateInstance(CLSID_DOMDocument30, NULL, CLSCTX_INPROC_SERVER, 
		       IID_IXMLDOMDocument, (void**)&mp_File);
		if (FAILED(hr) || !mp_File) {
			wsprintf(szErr, L"Can't create IID_IXMLDOMDocument!\nErrCode=0x%08X", (DWORD)hr);
			goto wrap;
		}
		hr = mp_File->put_preserveWhiteSpace(VARIANT_TRUE);
		hr = mp_File->put_async(VARIANT_FALSE);
		
		// Загрузить xml-ку
		bSuccess = VARIANT_FALSE;
		vt.vt = VT_BSTR; vt.bstrVal = ::SysAllocString(gConEmu.ms_ConEmuXml);
		hr = mp_File->load(vt, &bSuccess);
		VariantClear(&vt);
		if (FAILED(hr) || !bSuccess) {
			wsprintf(szErr, L"Failed to load ConEmu.xml!\nHR=0x%08X\n", (DWORD)hr);
			hr = mp_File->get_parseError(&pErr);
			if (pErr) {
				long errorCode = 0; // Contains the error code of the last parse error. Read-only.
				long line = 0; // Specifies the line number that contains the error. Read-only.
				long linepos  = 0; // Contains the character position within the line where the error occurred. Read-only.
 				hr = pErr->get_errorCode(&errorCode);
 				hr = pErr->get_line(&line);
 				hr = pErr->get_linepos(&linepos);
 				wsprintf(szErr+lstrlen(szErr), L"XmlErrCode=%i, Line=%i, Pos=%i", errorCode, line, linepos);
			}
			goto wrap;
		}
		
		hr = mp_File->QueryInterface(IID_IXMLDOMNode, (void **)&pKey);
		if (FAILED(hr)) {
			wsprintf(szErr, L"XML: Root node not found!\nErrCode=0x%08X", (DWORD)hr);
			goto wrap;
		}
		
		while (*regPath) {
			// Получить следующий токен
			psz = wcschr(regPath, L'\\');
			if (!psz) psz = regPath + lstrlen(regPath);
			lstrcpyn(szName, regPath, psz-regPath+1);
			
			// Найти в структуре XML
			pChild = FindItem(pKey, L"key", szName, bAllowCreate);
			pKey->Release();
			pKey = pChild; pChild = NULL;
			
			if (!pKey) {
				if (bAllowCreate)
					wsprintf(szErr, L"XML: key <%s> not found!", szName);
				else
					wsprintf(szErr, L"XML: Can't create key <%s>!", szName);
				goto wrap;
			}
			
			if (*psz == L'\\') {
				regPath = psz + 1;
			} else {
				break;
			}
		}

		// Нашли, запомнили	
		mp_Key = pKey; pKey = NULL;
		
		lbRc = true;
	
	}__except(EXCEPTION_EXECUTE_HANDLER){
		lstrcpy(szErr, L"Exception in SettingsXML::OpenKey");
		lbRc = false;
	}

wrap:

	if (pErr) { pErr->Release(); pErr = NULL; }
	if (pChild) { pChild->Release(); pChild = NULL; }
	if (pKey) { pKey->Release(); pKey = NULL; }
	
	if (!lbRc && szErr[0]) {
		MBoxA(szErr);
	}
	return lbRc;
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

	// Получить все дочерние элементы нужного типа
	bsText = ::SysAllocString(asType);
	hr = apFrom->selectNodes(bsText, &pList);
	::SysFreeString(bsText); bsText = NULL;

	if (SUCCEEDED(hr) && pList) {
		hr = pList->reset();
		while ((hr = pList->nextNode(&pIXMLDOMNode)) == S_OK && pIXMLDOMNode) {
			hr = pIXMLDOMNode->get_attributes(&pAttrs);
			if (SUCCEEDED(hr)) {
				hr = pAttrs->getNamedItem(L"name", &pName);
				pAttrs->Release(); pAttrs = NULL;
				
				if (SUCCEEDED(hr)) {
					hr = pName->get_text(&bsText);
					pName->Release(); pName = NULL;
					
					if (lstrcmpi(bsText, asName) == 0) {
						::SysFreeString(bsText); bsText = NULL;
						pChild = pIXMLDOMNode; pIXMLDOMNode = NULL;
						break;
					}
					::SysFreeString(bsText); bsText = NULL;
				}
			}
			
			pIXMLDOMNode->Release(); pIXMLDOMNode = NULL;
		}
		pList->Release(); pList = NULL;
	}
	
	
	if (!pChild && abAllowCreate) {
		bsText = ::SysAllocString(asType);
		VARIANT vtType; vtType.vt = VT_I4; vtType.lVal = NODE_ELEMENT;
		hr = mp_File->createNode(vtType, bsText, L"", &pChild);
		::SysFreeString(bsText); bsText = NULL;

		if (SUCCEEDED(hr) && pChild) {
			hr = pChild->get_attributes(&pAttrs);
			if (FAILED(hr)) {
				pChild->Release(); pChild = NULL;
			} else {
				hr = mp_File->createAttribute(L"name", &pIXMLDOMAttribute);
		        if(SUCCEEDED(hr) && pIXMLDOMAttribute)
		        {
					VARIANT vtName; vtName.vt = VT_BSTR; vtName.bstrVal = ::SysAllocString(asName);
            		hr = pIXMLDOMAttribute->put_nodeValue(vtName);
					VariantClear(&vtName);
            		hr = pAttrs->setNamedItem(pIXMLDOMAttribute, &pIXMLDOMNode);
            		if(SUCCEEDED(hr) && pIXMLDOMNode)
		            {
		               pIXMLDOMNode->Release();
		               pIXMLDOMNode = NULL;
		            }
				}
				hr = apFrom->appendChild(pChild, &pIXMLDOMNode);
				pChild->Release(); pChild = NULL;
				if (FAILED(hr)) {
					pAttrs->Release(); pAttrs = NULL;
				} else {
					pChild = pIXMLDOMNode;
					pIXMLDOMNode = NULL;
				}
			}
		}
	}
	
	return pChild;
}

void SettingsXML::CloseKey()
{
	if (mp_Key) { mp_Key->Release(); mp_Key = NULL; }
	if (mp_File) { mp_File->Release(); mp_File = NULL; }
}

bool SettingsXML::Load(const wchar_t *regName, wchar_t **value, LPDWORD pnSize /*= NULL*/)
{
	return false;
}
bool SettingsXML::Load(const wchar_t *regName, LPBYTE value, DWORD nSize)
{
	return false;
}

void SettingsXML::Delete(const wchar_t *regName)
{
}

void SettingsXML::Save(const wchar_t *regName, const wchar_t *value)
{
// value = _T(""); // сюда мог придти и NULL
}
void SettingsXML::Save(const wchar_t *regName, LPCBYTE value, DWORD nType, DWORD nSize)
{
}
