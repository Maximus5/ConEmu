
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


#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"

#include "ConEmu.h"
#include "ConEmuStart.h"
#include "LngData.h"
#include "LngRc.h"
#include "Options.h"
#include "../common/MJsonReader.h"
#include "../common/WFiles.h"

const wchar_t gsResourceFileName[] = L"ConEmu.l10n";

//#include "../common/MSectionSimple.h"

CLngRc* gpLng = NULL;

CLngRc::CLngRc()
{
	//mp_Lock = new MSectionSimple();
}

CLngRc::~CLngRc()
{
	//SafeDelete(mp_Lock);
}

// static
bool CLngRc::isLocalized()
{
	if (!gpLng)
		return false;
	if (gpLng->ms_Lng.IsEmpty() || gpLng->ms_l10n.IsEmpty())
		return false;
	return true;
}

// static
void CLngRc::Initialize()
{
	if (!gpLng)
		gpLng = new CLngRc();

	CLngPredefined::Initialize();
	
	// No sense to load resources now,
	// options were not initialized yet
	// -- if (gpLng) gpLng->Reload();
}

void CLngRc::Reload(bool bForce /*= false*/)
{
	bool bChanged = bForce;
	bool bExists = false;
	CEStr lsNewLng, lsNewFile;

	// TODO: There would be gpSet member in future
	lsNewLng.Set(gpConEmu->opt.Language.GetStr());

	// Language was requested?
	if (!lsNewLng.IsEmpty())
	{
		if (ms_Lng.IsEmpty() || (0 != wcscmp(lsNewLng, ms_Lng)))
		{
			bChanged = true;
		}

		// We need a file
		if (gpConEmu->opt.LanguageFile.Exists)
		{
			// It may contain environment variables
			lsNewFile = ExpandEnvStr(gpConEmu->opt.LanguageFile.GetStr());
			if (lsNewFile.IsEmpty())
				lsNewFile.Set(gpConEmu->opt.LanguageFile.GetStr());
			// Check if the file exists
			bExists = FileExists(lsNewFile);
		}
		else
		{
			lsNewFile = JoinPath(gpConEmu->ms_ConEmuExeDir, gsResourceFileName);
			if (!(bExists = FileExists(lsNewFile)))
			{
				lsNewFile = JoinPath(gpConEmu->ms_ConEmuBaseDir, gsResourceFileName);
				bExists = FileExists(lsNewFile);
			}
		}

		// File name was changed?
		if ((bExists == ms_l10n.IsEmpty())
			|| (bExists
				&& (ms_l10n.IsEmpty()
					|| (0 != wcscmp(lsNewFile, ms_l10n)))
				)
			)
		{
			bChanged = true;
		}
	}
	else if (!ms_Lng.IsEmpty())
	{
		bChanged = true;
	}

	// Check if we have to reload data
	if (bChanged)
	{
		if (!bExists
			|| !LoadResources(lsNewLng, lsNewFile))
		{
			ms_Lng.Empty();
			ms_l10n.Empty();

			Clean(m_CmnHints);
			Clean(m_MnuHints);
			Clean(m_Controls);
		}
	}
}

void CLngRc::Clean(MArray<CLngRc::LngRcItem>& arr)
{
	for (INT_PTR i = arr.size()-1; i >= 0; --i)
	{
		LngRcItem& l = arr[i];
		l.Processed = false;
		l.Localized = false;
		l.MaxLen = 0;
		SafeFree(l.Str);
	}
}

bool CLngRc::LoadResources(LPCWSTR asLanguage, LPCWSTR asFile)
{
	bool  bOk = false;
	CEStr lsJsonData;
	DWORD jsonDataSize = 0, nErrCode = 0;
	MJsonValue* jsonFile = NULL;
	MJsonValue jsonSection;
	DWORD nStartTick = 0, nLoadTick = 0, nFinTick = 0, nDelTick = 0;
	int iRc;
	struct { LPCWSTR pszSection; MArray<LngRcItem>* arr; int idDiff; }
	sections[] = {
		{ L"cmnhints", &m_CmnHints, 0 },
		{ L"mnuhints", &m_MnuHints, IDM__MIN_MNU_ITEM_ID },
		{ L"controls", &m_Controls, 0 },
		{ L"strings",  &m_Strings, 0 },
	};

	if (gpSet->isLogging())
	{
		CEStr lsLog(L"Loading resources: Lng=`", asLanguage, L"` File=`", asFile, L"`");
		gpConEmu->LogString(lsLog);
	}

	iRc = ReadTextFile(asFile, 1<<24 /*16Mb max*/, lsJsonData.ms_Val, jsonDataSize, nErrCode);
	if (iRc != 0)
	{
		// TODO: Log error
		goto wrap;
	}
	
	nStartTick = GetTickCount();
	jsonFile = new MJsonValue();
	if (!jsonFile->ParseJson(lsJsonData))
	{
		// TODO: Log error
		CEStr lsErrMsg(
			L"Language resources loading failed!\r\n"
			L"File: ", asFile, L"\r\n"
			L"Error: ", jsonFile->GetParseError());
		gpConEmu->LogString(lsErrMsg.ms_Val);
		DisplayLastError(lsErrMsg.ms_Val, (DWORD)-1, MB_ICONSTOP);
		goto wrap;
	}
	nLoadTick = GetTickCount();

	// Remember language parameters
	ms_Lng.Set(asLanguage);
	ms_l10n.Set(asFile);

	// Allocate intial array size
	m_CmnHints.alloc(4096);
	m_MnuHints.alloc(512);
	m_Controls.alloc(4096);
	m_Strings.alloc(lng_NextId);

	// Process sections
	for (size_t i = 0; i < countof(sections); i++)
	{
		if (jsonFile->getItem(sections[i].pszSection, jsonSection) && (jsonSection.getType() == MJsonValue::json_Object))
			bOk |= LoadSection(&jsonSection, *(sections[i].arr), sections[i].idDiff);
		else
			Clean(*(sections[i].arr));
	}

	nFinTick = GetTickCount();

wrap:
	SafeDelete(jsonFile);
	nDelTick = GetTickCount();
	if (bOk)
	{
		wchar_t szLog[120];
		_wsprintf(szLog, SKIPCOUNT(szLog) L"Loading resources: duration (ms): Parse: %u; Internal: %u; Delete: %u", (nLoadTick - nStartTick), (nFinTick - nLoadTick), (nDelTick - nFinTick));
		gpConEmu->LogString(szLog);
	}
	return bOk;
}

bool CLngRc::LoadSection(MJsonValue* pJson, MArray<LngRcItem>& arr, int idDiff)
{
	bool bRc = false;
	MJsonValue jRes, jItem;

	_ASSERTE(!ms_Lng.IsEmpty());

	for (INT_PTR i = arr.size()-1; i >= 0; --i)
	{
		LngRcItem& l = arr[i];
		l.Processed = false;
		l.Localized = false;
	}

	size_t iCount = pJson->getLength();

	for (size_t i = 0; i < iCount; i++)
	{
		if (!pJson->getItem(i, jRes) || (jRes.getType() != MJsonValue::json_Object))
			continue;

		// Now, jRes contains something like this:
	    // {
	    //  "en": "Decrease window height (check ‘Resize with arrows’)",
	    //  "ru": [ "Decrease window height ", "(check ‘Resize with arrows’)" ],
	    //  "id": 2046
		// }
		LPCWSTR lsLoc = NULL;
		i64 id = -1;
		size_t childCount = jRes.getLength();

		for (INT_PTR c = (childCount - 1); c >= 0; --c)
		{
			LPCWSTR pszName = jRes.getObjectName(c);
			if (!pszName || !*pszName)
			{
				_ASSERTE(FALSE && "Invalid object name!");
				return false;
			}

			// "id" field must be LAST!
			if (wcscmp(pszName, L"id") == 0)
			{
				if (!jRes.getItem(c, jItem) || (jItem.getType() != MJsonValue::json_Integer))
				{
					_ASSERTE(FALSE && "Invalid 'id' field");
					return false;
				}
				id = jItem.getInt();
				if ((id <= idDiff) || ((id - idDiff) > 0xFFFF))
				{
					_ASSERTE(FALSE && "Invalid 'id' value");
					return false;
				}

			} // "id"

			// "en" field must be FIRST!
			else if ((wcscmp(pszName, ms_Lng) == 0)
				|| (wcscmp(pszName, L"en") == 0)
				)
			{
				if (id == -1)
				{
					_ASSERTE(FALSE && "Wrong format, 'id' not found!");
					return false;
				}

				if (!jRes.getItem(c, jItem)
					|| ((jItem.getType() != MJsonValue::json_String)
						&& (jItem.getType() != MJsonValue::json_Array))
					)
				{
					_ASSERTE(FALSE && "Invalid 'lng' field");
					return false;
				}

				switch (jItem.getType())
				{
				case MJsonValue::json_String:
					if (!SetResource(arr, (id - idDiff), jItem.getString(), true))
					{
						// Already asserted
						return false;
					}
					bRc = true;
					break;
				case MJsonValue::json_Array:
					if (!SetResource(arr, (id - idDiff), &jItem))
					{
						// Already asserted
						return false;
					}
					bRc = true;
					break;
				default:
					_ASSERTE(FALSE && "Unsupported object type!")
					return false;
				} // switch (jItem.getType())

				// proper lng string found and processed, go to next resource
				break; // for (size_t c = 0; c < childCount; c++)

			} // ms_Lng || "en"

		} // for (size_t c = 0; c < childCount; c++)

	} // for (size_t i = 0; i < iCount; i++)

	return bRc;
}

// Set resource item
bool CLngRc::SetResource(MArray<LngRcItem>& arr, int idx, LPCWSTR asValue, bool bLocalized)
{
	if (idx < 0)
	{
		_ASSERTE(idx >= 0);
		return false;
	}

	_ASSERTE(!bLocalized || (asValue && *asValue));

	if (idx >= arr.size())
	{
		LngRcItem dummy = {};
		arr.set_at(idx, dummy);
	}

	bool bOk = false;
	LngRcItem& item = arr[idx];

	// Caching: no resource was found for that id
	if (!asValue || !*asValue)
	{
		if (item.Str)
			item.Str[0] = 0;
		item.Processed = true;
		item.Localized = false;
		return true;
	}

	size_t iLen = wcslen(asValue);
	if (iLen >= (u16)-1)
	{
		// Too long string?
		_ASSERTE(iLen < (u16)-1);
	}
	else
	{
		if (item.Str && (item.MaxLen >= iLen))
		{
			_wcscpy_c(item.Str, item.MaxLen, asValue);
		}
		else
		{
			//TODO: thread-safe
			SafeFree(item.Str);
			item.MaxLen = iLen;
			item.Str = lstrdup(asValue);
		}
		bOk = (item.Str != NULL);
	}

	item.Processed = bOk;
	item.Localized = (bOk && bLocalized);

	return bOk;
}

// Concatenate strings from array and set resource item
bool CLngRc::SetResource(MArray<LngRcItem>& arr, int idx, MJsonValue* pJson)
{
	CEStr lsValue;
	MJsonValue jStr;

	// [ "Decrease window height ", "(check ‘Resize with arrows’)" ]
	size_t iCount = pJson->getLength();

	for (size_t i = 0; i < iCount; i++)
	{
		if (!pJson->getItem(i, jStr) || (jStr.getType() != MJsonValue::json_String))
		{
			_ASSERTE(FALSE && "String format failure");
			return false;
		}
		lstrmerge(&lsValue.ms_Val, jStr.getString());
	}

	if (lsValue.IsEmpty())
	{
		_ASSERTE(FALSE && "Empty resource string (array)");
		return false;
	}

	return SetResource(arr, idx, lsValue.ms_Val, true);
}

LPCWSTR CLngRc::getControl(LONG id, CEStr& lsText, LPCWSTR asDefault /*= NULL*/)
{
	if (!gpLng)
	{
		_ASSERTE(gpLng != NULL);
		return asDefault;
	}
	if (!id || (id > (u16)-1))
	{
		_ASSERTE(FALSE && "Control ID out of range");
		return asDefault;
	}

	if (gpLng->GetResource(gpLng->m_Controls, id, lsText))
	{
		return lsText.ms_Val;
	}

	return asDefault;
}

bool CLngRc::GetResource(MArray<LngRcItem>& arr, int idx, CEStr& lsText)
{
	bool bFound = false;

	if ((idx >= 0) && (idx < arr.size()))
	{
		const LngRcItem& item = arr[idx];

		if (item.Processed && (item.Str && *item.Str))
		{
			lsText.Set(item.Str);
			bFound = true;
		}
	}

	return bFound;
}

bool CLngRc::getHint(UINT id, LPWSTR lpBuffer, size_t nBufferMax)
{
	if (!gpLng)
	{
		_ASSERTE(gpLng != NULL);
		return loadString(id, lpBuffer, nBufferMax);
	}

	// IDM__MIN_MNU_ITEM_ID
	UINT idDiff = (id < IDM__MIN_MNU_ITEM_ID) ? 0 : IDM__MIN_MNU_ITEM_ID;
	INT_PTR idx = (id - idDiff);
	_ASSERTE(idx >= 0);

	MArray<LngRcItem>& arr = (id < IDM__MIN_MNU_ITEM_ID) ? gpLng->m_CmnHints : gpLng->m_MnuHints;

	if (arr.size() > idx)
	{
		const LngRcItem& item = arr[idx];

		if (item.Processed)
		{
			if (item.Str && *item.Str)
			{
				lstrcpyn(lpBuffer, item.Str, nBufferMax);
				// Succeeded
				return true;
			}
			// No such resource
			goto wrap;
		}
	}

	// Use binary search to find resource
	if (loadString(id, lpBuffer, nBufferMax))
	{
		// Succeeded
		return true;
	}

	// Don't try to load it again
	gpLng->SetResource(arr, idx, NULL, false);

wrap:
	if (lpBuffer && nBufferMax)
		lpBuffer[0] = 0;
	return false;
}

LPCWSTR CLngRc::getRsrc(UINT id, CEStr* lpText /*= NULL*/)
{
	LPCWSTR pszRsrc = NULL;
	if (!gpLng)
	{
		pszRsrc = CLngPredefined::getRsrc(id);
	}
	else
	{
		INT_PTR idx = (INT_PTR)id;
		MArray<LngRcItem>& arr = gpLng->m_Strings;

		if ((idx >= 0) && (arr.size() > idx))
		{
			const LngRcItem& item = arr[idx];

			if (item.Processed)
			{
				if (item.Str && *item.Str)
				{
					// Succeeded
					pszRsrc = item.Str;
					goto wrap;
				}
				// No such resource
				goto wrap;
			}
		}

		// Use binary search to find resource
		pszRsrc = CLngPredefined::getRsrc(id);
	}

wrap:
	if (lpText)
		lpText->Set(pszRsrc);
	// Don't return NULL-s ever
	return (pszRsrc ? pszRsrc : L"");
}

// static
bool CLngRc::loadString(UINT id, LPWSTR lpBuffer, size_t nBufferMax)
{
	LPCWSTR pszPredefined = CLngPredefined::getHint(id);
	if (pszPredefined == NULL)
		return false;
	lstrcpyn(lpBuffer, pszPredefined, nBufferMax);
	return true;
}
