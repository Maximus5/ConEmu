
/*
Copyright (c) 2010-present Maximus5
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

#include "../common/defines.h"
#pragma warning( disable : 4995 )
#include "../common/pluginW1761.hpp" // Отличается от 995 наличием SynchoApi
#pragma warning( default : 4995 )
#include "../common/plugin_helper.h"
#include "ConEmuBg.h"

//#define FCTL_GETPANELDIR FCTL_GETCURRENTDIRECTORY

//#define _ACTL_GETFARRECT 32

#ifdef _DEBUG
#define SHOW_DEBUG_EVENTS
#endif

struct PluginStartupInfo *InfoW995 = NULL;
struct FarStandardFunctions *FSFW995 = NULL;
static wchar_t* gszRootKeyW995  = NULL;

void GetPluginInfoW995(void *piv)
{
	PluginInfo* pi = (PluginInfo*)piv;
	//memset(pi, 0, sizeof(PluginInfo));
	_ASSERTE(pi->StructSize==0);
	pi->StructSize = sizeof(PluginInfo);
	//_ASSERTE(pi->StructSize>0 && (pi->StructSize >= sizeof(*pi)));

	static WCHAR *szMenu[1], szMenu1[255];
	szMenu[0] = szMenu1;
	lstrcpynW(szMenu1, GetMsgW(CEPluginName), 240);
	_ASSERTE(pi->StructSize == sizeof(struct PluginInfo));
	pi->Flags = (gbBackgroundEnabled?PF_PRELOAD:0) | PF_EDITOR | PF_VIEWER;
	//pi->DiskMenuStrings = NULL;
	//pi->DiskMenuNumbers = 0;
	pi->PluginMenuStrings = szMenu;
	pi->PluginMenuStringsNumber = 1;
	pi->PluginConfigStrings = szMenu;
	pi->PluginConfigStringsNumber = 1;
}

void SetStartupInfoW995(void *aInfo)
{
	INIT_FAR_PSI(::InfoW995, ::FSFW995, (PluginStartupInfo*)aInfo);
	
	_ASSERTE(FPS_SHOWSTATUSLINE == 0x00000040);
	_ASSERTE(FPS_SHOWCOLUMNTITLES == 0x00000020);

	DWORD nFarVer = 0;
	if (InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETFARVERSION, &nFarVer))
	{
		if (HIBYTE(LOWORD(nFarVer)) == 2)
		{
			gFarVersion.dwBuild = HIWORD(nFarVer);
			gFarVersion.dwVerMajor = (HIBYTE(LOWORD(nFarVer)));
			gFarVersion.dwVerMinor = (LOBYTE(LOWORD(nFarVer)));
		}
		else
		{
			_ASSERTE(HIBYTE(HIWORD(nFarVer)) == 2);
		}
	}

	int nLen = lstrlenW(InfoW995->RootKey)+16;
	if (gszRootKeyW995) free(gszRootKeyW995);
	gszRootKeyW995 = (wchar_t*)calloc(nLen,2);
	lstrcpyW(gszRootKeyW995, InfoW995->RootKey);
	WCHAR* pszSlash = gszRootKeyW995+lstrlenW(gszRootKeyW995)-1;
	if (*pszSlash != L'\\') *(++pszSlash) = L'\\';
	lstrcpyW(pszSlash+1, L"ConEmuBg\\");
}

extern BOOL gbInfoW_OK;
HANDLE WINAPI _export OpenPluginW(int OpenFrom,INT_PTR Item)
{
	if (!gbInfoW_OK)
		return INVALID_HANDLE_VALUE;

	return OpenPluginWcmn(OpenFrom, Item);
}

void ExitFARW995(void)
{
	if (InfoW995)
	{
		free(InfoW995);
		InfoW995=NULL;
	}

	if (FSFW995)
	{
		free(FSFW995);
		FSFW995=NULL;
	}

	if (gszRootKeyW995)
	{
		free(gszRootKeyW995);
		gszRootKeyW995 = NULL;
	}
}

LPCWSTR GetMsgW995(int aiMsg)
{
	if (!InfoW995 || !InfoW995->GetMsg)
		return L"";

	return InfoW995->GetMsg(InfoW995->ModuleNumber,aiMsg);
}

#define FAR_UNICODE 995
#include "Configure.h"

int ConfigureW995(int ItemNumber)
{
	if (!InfoW995)
		return false;

	return ConfigureProc(ItemNumber);
}

void SettingsLoadW995()
{
	SettingsLoadReg(gszRootKeyW995);
}
void SettingsSaveW995()
{
	SettingsSaveReg(gszRootKeyW995);
}

bool FMatchW995(LPCWSTR asMask, LPWSTR asPath)
{
	int iRc = FSFW995->ProcessName(asMask, asPath, 0, PN_CMPNAME);
	return (iRc != 0);
}

int GetMacroAreaW995()
{
	#define MCMD_GETAREA 6
	ActlKeyMacro area = {MCMD_GETAREA};
	int nArea = (int)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_KEYMACRO, &area);
	return nArea;
}
