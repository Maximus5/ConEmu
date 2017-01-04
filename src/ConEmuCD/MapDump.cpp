
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

#include "../common/Common.h"
#include "../common/CEStr.h"
#include "../common/CmdLine.h"
#include "../common/MStrDup.h"
#include <stdlib.h>
#include "ConEmuC.h"
#include "MapDump.h"

enum MapDumpEnum
{
	mde_Unknown = 0,
	mde_GuiMapping, // struct ConEmuGuiMapping :: CEGUIINFOMAPNAME L"ConEmuGuiInfoMapping.%u" ( % == dwGuiProcessId )
	mde_ConMapping, // struct CESERVER_CONSOLE_MAPPING_HDR :: CECONMAPNAME L"ConEmuFileMapping.%08X" ( % == (DWORD)ghConWnd )
	mde_AppMapping, // struct CESERVER_CONSOLE_APP_MAPPING :: CECONAPPMAPNAME L"ConEmuAppMapping.%08X" ( % == (DWORD)ghConWnd )
	mde_All,
};

static void DumpMember(const WORD& dwData, LPCWSTR szName, LPCWSTR szIndent)
{
	wchar_t szData[32]; CEStr lsLine;
	lsLine = lstrmerge(szIndent, szName, L": ", _ultow(dwData, szData, 10), L"\r\n");
	_wprintf(lsLine);
}

static void DumpMember(const DWORD& dwData, LPCWSTR szName, LPCWSTR szIndent)
{
	wchar_t szData[32]; CEStr lsLine;
	lsLine = lstrmerge(szIndent, szName, L": ", _ultow(dwData, szData, 10), L"\r\n");
	_wprintf(lsLine);
}

static void DumpMember(const int& iData, LPCWSTR szName, LPCWSTR szIndent)
{
	wchar_t szData[32]; CEStr lsLine;
	lsLine = lstrmerge(szIndent, szName, L": ", _ltow(iData, szData, 10), L"\r\n");
	_wprintf(lsLine);
}

static void DumpMember(const wchar_t* pszData, LPCWSTR szName, LPCWSTR szIndent)
{
	CEStr lsLine;
	lsLine = lstrmerge(szIndent, szName, L": `", pszData, L"`\r\n");
	_wprintf(lsLine);
}

static void DumpMember(const u64& llData, LPCWSTR szName, LPCWSTR szIndent)
{
	wchar_t szData[64]; CEStr lsLine;
	_wsprintf(szData, SKIPCOUNT(szData) L"0x%p", (LPVOID)llData);
	lsLine = lstrmerge(szIndent, szName, L": ", szData, L"\r\n");
	_wprintf(lsLine);
}

static void DumpMember(const HWND2& hWnd, LPCWSTR szName, LPCWSTR szIndent)
{
	wchar_t szData[32]; CEStr lsLine;
	_wsprintf(szData, SKIPCOUNT(szData) L"0x%08X", (DWORD)hWnd);
	lsLine = lstrmerge(szIndent, szName, L": ", szData, L"\r\n");
	_wprintf(lsLine);
}

static void DumpMember(const CONSOLE_SCREEN_BUFFER_INFO& cs, LPCWSTR szName, LPCWSTR szIndent)
{
	wchar_t szText[255];
	_wsprintf(szText, SKIPCOUNT(szText) L"%s%s: Size={%i,%i} Cursor={%i,%i} Attr=0x%02X Wnd={%i,%i}-{%i,%i} Max={%i,%i}\r\n",
		szIndent, szName, cs.dwSize.X, cs.dwSize.Y, cs.dwCursorPosition.X, cs.dwCursorPosition.Y,
		cs.wAttributes, cs.srWindow.Left, cs.srWindow.Top, cs.srWindow.Right, cs.srWindow.Bottom,
		cs.dwMaximumWindowSize.X, cs.dwMaximumWindowSize.Y);
	_wprintf(szText);
}

static void DumpMember(const COORD& cr, LPCWSTR szName, LPCWSTR szIndent)
{
	wchar_t szText[80];
	_wsprintf(szText, SKIPCOUNT(szText) L"%s%s: {%i,%i}\r\n",
		szIndent, szName, cr.X, cr.Y);
	_wprintf(szText);
}

static void DumpConsoleInfo(const ConEmuConsoleInfo& con, LPCWSTR szIndent)
{
	_wprintf(szIndent);

	wchar_t szFlags[64] = L"";
	if (con.Flags & ccf_Active)
		wcscat_c(szFlags, L"|Active");
	if (con.Flags & ccf_Visible)
		wcscat_c(szFlags, L"|Visible");
	if (con.Flags & ccf_ChildGui)
		wcscat_c(szFlags, L"|ChildGui");
	if (!*szFlags)
		wcscat_c(szFlags, L"|None");

	wchar_t szLine[128];
	_wsprintf(szLine, SKIPCOUNT(szLine) L" ConWnd=0x%08X DCWnd=0x%08X ChildGui=0x%08X SrvPID=%u %s\r\n",
		(DWORD)con.Console, (DWORD)con.DCWindow, (DWORD)con.ChildGui, con.ServerPID, szFlags+1);
	_wprintf(szLine);
}

struct FlagList
{
	DWORD flag;
	LPCWSTR name;
};

static void DumpFlags(const DWORD Flags, FlagList* flags, size_t flagsCount, LPCWSTR szName, LPCWSTR szIndent)
{
	CEStr lsLine;
	lsLine = lstrmerge(szIndent, szName, L": ");

	LPCWSTR pszDelim = NULL;
	for (size_t i = 0; i < flagsCount; i++)
	{
		if (Flags & flags[i].flag)
		{
			lstrmerge(&lsLine.ms_Val, pszDelim, flags[i].name);
			if (!pszDelim) pszDelim = L"|";
		}
	}
	lstrmerge(&lsLine.ms_Val, L"\r\n");
	_wprintf(lsLine);
}

static void DumpConsoleFlags(const ConEmuConsoleFlags& Flags, LPCWSTR szName, LPCWSTR szIndent)
{
	FlagList flags[] = {
		{ CECF_DosBox,          L"DosBox" },
		{ CECF_UseTrueColor,    L"UseTrueColor" },
		{ CECF_ProcessAnsi,     L"ProcessAnsi" },
		{ CECF_UseClink_1,      L"UseClink_1" },
		{ CECF_UseClink_2,      L"UseClink_2" },
		{ CECF_SleepInBackg,    L"SleepInBackg" },
		{ CECF_BlockChildDbg,   L"BlockChildDbg" },
		{ CECF_SuppressBells,   L"SuppressBells" },
		{ CECF_ConExcHandler,   L"ConExcHandler" },
		{ CECF_ProcessNewCon,   L"ProcessNewCon" },
		{ CECF_ProcessCmdStart, L"ProcessCmdStart" },
		{ CECF_RealConVisible,  L"RealConVisible" },
		{ CECF_ProcessCtrlZ,    L"ProcessCtrlZ" },
		{ CECF_RetardNAPanes,   L"RetardNAPanes" },
	};

	DumpFlags(Flags, flags, countof(flags), szName, szIndent);
}

static void DumpAppFlags(const CEActiveAppFlags& Flags, LPCWSTR szName, LPCWSTR szIndent)
{
	FlagList flags[] = {
		{ caf_Cygwin1,          L"Cygwin1" },
		{ caf_Msys1,            L"MSys1" },
		{ caf_Msys2,            L"MSys2" },
		{ caf_Clink,            L"Clink" },
	};

	DumpFlags(Flags, flags, countof(flags), szName, szIndent);
}


static void DumpStructPtr(void* ptrData, MapDumpEnum type, LPCWSTR szIndent)
{
	wchar_t szText[255];
	#define dumpMember(n) DumpMember(p->n, _CRT_WIDE(#n), szIndent)
	switch (type)
	{
	case mde_GuiMapping:
		{
			_wprintf(L"{struct ConEmuGuiMapping}\r\n");
			ConEmuGuiMapping* p = (ConEmuGuiMapping*)ptrData;
			dumpMember(cbSize);
			dumpMember(nProtocolVersion);
			dumpMember(nChangeNum);
			dumpMember(hGuiWnd);
			dumpMember(nGuiPID);
			dumpMember(nLoggingType);
			dumpMember(sConEmuExe);
			dumpMember(sConEmuArgs);
			dumpMember(bUseInjects);
			DumpConsoleFlags(p->Flags, L"Flags", szIndent);
			dumpMember(bGuiActive);
			dumpMember(dwActiveTick);
			// ConEmuConsoleInfo Consoles[MAX_CONSOLE_COUNT];
			_wprintf(szIndent); _wprintf(L"Consoles\r\n");
			for (size_t i = 0; i < countof(p->Consoles); i++)
			{
				if (!p->Consoles[i].ServerPID && !p->Consoles[i].Console && !p->Consoles[i].DCWindow && !p->Consoles[i].ChildGui)
					continue;
				_wsprintf(szText, SKIPCOUNT(szText) L"%s  [%2u]:", szIndent, i);
				DumpConsoleInfo(p->Consoles[i], szText);
			}
			// ConEmuMainFont MainFont;
			// ConEmuComspec ComSpec;
			dumpMember(AppID);
		} break;
	case mde_ConMapping:
		{
			_wprintf(L"{struct CESERVER_CONSOLE_MAPPING_HDR}\r\n");
			CESERVER_CONSOLE_MAPPING_HDR* p = (CESERVER_CONSOLE_MAPPING_HDR*)ptrData;
			dumpMember(cbSize);
			dumpMember(nProtocolVersion);
			dumpMember(nLogLevel);
			dumpMember(crMaxConSize);
			dumpMember(bDataReady);
			dumpMember(hConWnd);
			dumpMember(nServerPID);
			dumpMember(nAltServerPID);
			dumpMember(nGuiPID);
			dumpMember(nActiveFarPID);
			dumpMember(nServerInShutdown);
			dumpMember(sConEmuExe);
			dumpMember(hConEmuRoot);
			dumpMember(hConEmuWndDc);
			dumpMember(hConEmuWndBack);
			dumpMember(nLoggingType);
			dumpMember(bUseInjects);
			DumpConsoleFlags(p->Flags, L"Flags", szIndent);
			dumpMember(AnsiLog.Enabled);
			dumpMember(AnsiLog.Path);
			dumpMember(bLockVisibleArea);
			dumpMember(crLockedVisible);
			dumpMember(rbsAllowed);
		} break;
	case mde_AppMapping:
		{
			_wprintf(L"{struct CESERVER_CONSOLE_APP_MAPPING}\r\n");
			CESERVER_CONSOLE_APP_MAPPING* p = (CESERVER_CONSOLE_APP_MAPPING*)ptrData;
			dumpMember(cbSize);
			dumpMember(nProtocolVersion);
			dumpMember(nReadConsoleInputPID);
			dumpMember(nReadConsolePID);
			dumpMember(nLastReadInputPID);
			// WORD nPreReadRowID[2]
			_wsprintf(szText, SKIPCOUNT(szText) L"%snPreReadRowID[]: [%u, %u]\r\n", szIndent, (DWORD)p->nPreReadRowID[0], (DWORD)p->nPreReadRowID[1]);
			_wprintf(szText);
			dumpMember(csbiPreRead);
			DumpAppFlags(p->nActiveAppFlags, L"nActiveAppFlags", szIndent);
		} break;
	}
}

static MapDumpEnum GetMapTypeByName(LPCWSTR asMappingName, DWORD& strSize)
{
	if (!asMappingName || !*asMappingName)
		return mde_Unknown;

	if (wcsncmp(CEGUIINFOMAPNAME, asMappingName, wcslen(CEGUIINFOMAPNAME)-2) == 0)
	{
		strSize = sizeof(ConEmuGuiMapping);
		return mde_GuiMapping;
	}

	if (wcsncmp(CECONAPPMAPNAME, asMappingName, wcslen(CECONAPPMAPNAME)-4) == 0)
	{
		strSize = sizeof(CESERVER_CONSOLE_APP_MAPPING);
		return mde_AppMapping;
	}

	if (wcsncmp(CECONMAPNAME, asMappingName, wcslen(CECONMAPNAME)-4) == 0)
	{
		strSize = sizeof(CESERVER_CONSOLE_MAPPING_HDR);
		return mde_ConMapping;
	}

	if (isDigit(asMappingName[0]))
	{
		wchar_t* pszEnd = NULL;
		strSize = wcstoul(asMappingName, &pszEnd, 10);
		if (strSize && pszEnd && !*pszEnd)
		{
			return mde_All;
		}
	}

	return mde_Unknown;
}

template <typename T>
int ProcessMapping(MFileMapping<T>& fileMap, MapDumpEnum type, LPCWSTR pszMapName, DWORD strSize)
{
	CEStr lsMsg;

	if (!fileMap.Open(FALSE, strSize))
	{
		lsMsg = lstrmerge(L"Failed to open mapping: `", pszMapName, L"`\r\n", fileMap.GetErrorText(), L"\r\n");
		_wprintf(lsMsg);
		return CERR_UNKNOWN_MAP_NAME;
	}

	lsMsg = lstrmerge(L"Mapping: `", pszMapName, L"`\r\n");
	_wprintf(lsMsg);
	if (fileMap.Ptr()->cbSize != strSize)
	{
		wchar_t szReq[32] = L"", szGet[32] = L"";
		lsMsg = lstrmerge(L"### WARNING ###",
			L" ReqSize=", _ultow(strSize, szReq, 10),
			L" MapSize=", _ultow(fileMap.Ptr()->cbSize, szGet, 10),
			L"\r\n");
		_wprintf(lsMsg);
	}

	DumpStructPtr(fileMap.Ptr(), type, L"  ");

	return 0;
}

int DumpStructData(LPCWSTR asMappingName)
{
	CEStr lsMsg;

	if (!asMappingName || !*asMappingName || wcschr(asMappingName, L'%'))
	{
		lsMsg = lstrmerge(L"Invalid mapping name: `", asMappingName, L"`\r\n");
		_wprintf(lsMsg);
		return CERR_UNKNOWN_MAP_NAME;
	}

	DWORD strSize = 0;
	MapDumpEnum type = GetMapTypeByName(asMappingName, strSize);
	if ((type == mde_Unknown) || !strSize)
	{
		_ASSERTE(type == mde_Unknown);
		lsMsg = lstrmerge(L"Unknown mapping name: `", asMappingName, L"`\r\n");
		_wprintf(lsMsg);
		return CERR_UNKNOWN_MAP_NAME;
	}

	if (type == mde_All)
	{
		MFileMapping<ConEmuGuiMapping> guiMap;
		LPCWSTR pszGuiName = guiMap.InitName(CEGUIINFOMAPNAME, strSize/*dwGuiPID*/);
		int iRc = ProcessMapping(guiMap, mde_GuiMapping, pszGuiName, sizeof(ConEmuGuiMapping));
		if (iRc != 0)
			return iRc;
		ConEmuGuiMapping* p = guiMap.Ptr();
		if (p)
		{
			wchar_t szMapName[128];
			for (size_t i = 0; i < countof(p->Consoles); i++)
			{
				if (p->Consoles[i].Console)
				{
					_wsprintf(szMapName, SKIPCOUNT(szMapName) CECONMAPNAME, (DWORD)p->Consoles[i].Console);
					DumpStructData(szMapName);
					_wsprintf(szMapName, SKIPCOUNT(szMapName) CECONAPPMAPNAME, (DWORD)p->Consoles[i].Console);
					DumpStructData(szMapName);
				}
			}
		}
		return 0;
	}

	// Type has no matter, DumpStructPtr will cast pointer properly
	MFileMapping<CESERVER_REQ_HDR> fileMap;
	LPCWSTR pszName = fileMap.InitName(asMappingName);
	return ProcessMapping(fileMap, type, pszName, strSize);
}

int DoDumpStruct(LPCWSTR asCmdLine)
{
	int iRc = 0;
	if (!asCmdLine || !*asCmdLine)
		return CERR_UNKNOWN_MAP_NAME;
	CEStr lsName;
	while (0 == NextArg(&asCmdLine, lsName))
	{
		iRc = DumpStructData(lsName);
	}
	return iRc;
}
