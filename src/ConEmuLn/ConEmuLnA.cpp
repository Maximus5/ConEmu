
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
#include "../common/pluginA.hpp"
#include "../common/plugin_helper.h"
#include "ConEmuLn.h"

//#define SHOW_DEBUG_EVENTS

extern bool gbSetStartupInfoOk;

// minimal(?) FAR version 1.71 alpha 4 build 2470
int WINAPI _export GetMinFarVersion(void)
{
	// Однако, FAR2 до сборки 748 не понимал две версии плагина в одном файле
	BOOL bFar2=FALSE;

	if (!LoadFarVersion())
		bFar2 = TRUE;
	else
		bFar2 = gFarVersion.dwVerMajor>=2;

	if (bFar2)
	{
		return MAKEFARVERSION(2,0,748);
	}

	return MAKEFARVERSION(1,71,2470);
}


struct PluginStartupInfo *InfoA = NULL;
struct FarStandardFunctions *FSFA = NULL;
static wchar_t* gszRootKeyA = NULL;


#if defined(__GNUC__)
#ifdef __cplusplus
extern "C" {
#endif
	void WINAPI SetStartupInfo(const struct PluginStartupInfo *aInfo);
#ifdef __cplusplus
};
#endif
#endif


void WINAPI _export SetStartupInfo(const struct PluginStartupInfo *aInfo)
{
	gbSetStartupInfoOk = true;

	//LoadFarVersion - уже вызван в GetStartupInfo

	INIT_FAR_PSI(::InfoA, ::FSFA, aInfo);

	DWORD nFarVer = 0;
	if (InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETFARVERSION, &nFarVer))
	{
		if (HIBYTE(LOWORD(nFarVer)) == 1)
		{
			gFarVersion.dwBuild = HIWORD(nFarVer);
			gFarVersion.dwVerMajor = (HIBYTE(LOWORD(nFarVer)));
			gFarVersion.dwVerMinor = (LOBYTE(LOWORD(nFarVer)));
		}
		else
		{
			_ASSERTE(HIBYTE(HIWORD(nFarVer)) == 1);
		}
	}

	int nLen = lstrlenA(InfoA->RootKey)+16;
	if (gszRootKeyA) free(gszRootKeyA);
	gszRootKeyA = (wchar_t*)calloc(nLen,2);
	MultiByteToWideChar(CP_OEMCP,0,InfoA->RootKey,-1,gszRootKeyA,nLen);
	WCHAR* pszSlash = gszRootKeyA+lstrlenW(gszRootKeyA)-1;
	if (*pszSlash != L'\\') *(++pszSlash) = L'\\';
	lstrcpyW(pszSlash+1, L"ConEmuLn\\");

	StartPlugin(FALSE);
}

void WINAPI _export GetPluginInfo(struct PluginInfo *pi)
{
	_ASSERTE(pi->StructSize==0);
	pi->StructSize = sizeof(struct PluginInfo);
	//_ASSERTE(pi->StructSize>0 && (pi->StructSize >= sizeof(*pi)));

	static char *szMenu[1], szMenu1[255];
	szMenu[0]=szMenu1;
	lstrcpynA(szMenu1, InfoA->GetMsg(InfoA->ModuleNumber,CEPluginName), 240);
	_ASSERTE(pi->StructSize == sizeof(struct PluginInfo));
	pi->Flags = (gbBackgroundEnabled?PF_PRELOAD:0) | PF_EDITOR | PF_VIEWER;
	//pi->DiskMenuStrings = NULL;
	//pi->DiskMenuNumbers = 0;
	pi->PluginMenuStrings = szMenu;
	pi->PluginMenuStringsNumber = 1;
	pi->PluginConfigStrings = szMenu;
	pi->PluginConfigStringsNumber = 1;
	pi->CommandPrefix = NULL;
	pi->Reserved = 0;
}

void   WINAPI _export ExitFAR(void)
{
	ExitPlugin();

	if (InfoA)
	{
		free(InfoA);
		InfoA = NULL;
	}

	if (FSFA)
	{
		free(FSFA);
		FSFA = NULL;
	}

	if (gszRootKeyA)
	{
		free(gszRootKeyA);
		gszRootKeyA = NULL;
	}
}

static char *GetMsg(int MsgId)
{
	return((char*)InfoA->GetMsg(InfoA->ModuleNumber, MsgId));
}


#undef FAR_UNICODE
#include "Configure.h"

int WINAPI Configure(int ItemNumber)
{
	if (!InfoA)
		return false;

	return ConfigureProc(ItemNumber);
}

HANDLE WINAPI _export OpenPlugin(int OpenFrom,INT_PTR Item)
{
	if (InfoA == NULL)
		return INVALID_HANDLE_VALUE;

	Configure(0);
	return INVALID_HANDLE_VALUE;
}

void SettingsLoadA()
{
	SettingsLoadReg(gszRootKeyA);
}
void SettingsSaveA()
{
	SettingsSaveReg(gszRootKeyA);
}
