
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
#ifdef _DEBUG
#pragma warning( disable : 4995 )
#endif
#include "../common/pluginW2800.hpp" // Far3
#ifdef _DEBUG
#pragma warning( default : 4995 )
#endif
#include "../common/plugin_helper.h"
#include "ConEmuLn.h"
#include "../ConEmu/version.h"

//#define FCTL_GETPANELDIR FCTL_GETCURRENTDIRECTORY

//#define _ACTL_GETFARRECT 32

#ifdef _DEBUG
#define SHOW_DEBUG_EVENTS
#endif

struct PluginStartupInfo *InfoW2800 = NULL;
struct FarStandardFunctions *FSFW2800 = NULL;

GUID guid_ConEmuLn = { /* e71f78e4-585c-4ca7-9508-71d1966f7b1e */
    0xe71f78e4,
    0x585c,
    0x4ca7,
    {0x95, 0x08, 0x71, 0xd1, 0x96, 0x6f, 0x7b, 0x1e}
  };
GUID guid_ConEmuLnCfgDlg = { /* d381193b-8196-4240-942c-e1589da0dc00 */
    0xd381193b,
    0x8196,
    0x4240,
    {0x94, 0x2c, 0xe1, 0x58, 0x9d, 0xa0, 0xdc, 0x00}
  };
GUID guid_ConEmuLnPluginMenu = { /* d119555c-1291-4ae7-ad9b-f8a4df454c98 */
	0xd119555c,
	0x1291,
	0x4ae7,
	{0xad, 0x9b, 0xf8, 0xa4, 0xdf, 0x45, 0x4c, 0x98}
};
GUID guid_ConEmuLnPluginConfig = { /* e3fc38bf-e634-4340-9933-77267c1a8a1b */
    0xe3fc38bf,
    0xe634,
    0x4340,
    {0x99, 0x33, 0x77, 0x26, 0x7c, 0x1a, 0x8a, 0x1b}
  };


void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	_ASSERTE(Info->StructSize >= (size_t)((LPBYTE)(&Info->Instance) - (LPBYTE)(Info)));

	if (gFarVersion.dwBuild >= FAR_Y2_VER)
		Info->MinFarVersion = FARMANAGERVERSION;
	else
		Info->MinFarVersion = MAKEFARVERSION(FARMANAGERVERSION_MAJOR,FARMANAGERVERSION_MINOR, FARMANAGERVERSION_REVISION, 2578, FARMANAGERVERSION_STAGE);

	// Build: YYMMDDX (YY - две цифры года, MM - месяц, DD - день, X - 0 и выше-номер подсборки)
	Info->Version = MAKEFARVERSION(MVV_1,MVV_2,MVV_3,((MVV_1 % 100)*100000) + (MVV_2*1000) + (MVV_3*10) + (MVV_4 % 10),VS_RELEASE);
	
	Info->Guid = guid_ConEmuLn;
	Info->Title = L"ConEmu Underlines";
	Info->Description = L"Paint underlined background in the ConEmu window";
	Info->Author = L"ConEmu.Maximus5@gmail.com";
}

void GetPluginInfoW2800(void *piv)
{
	PluginInfo *pi = (PluginInfo*)piv;
	//memset(pi, 0, sizeof(PluginInfo));
	//pi->StructSize = sizeof(struct PluginInfo);
	_ASSERTE(pi->StructSize>0 && ((size_t)pi->StructSize >= sizeof(*pi)/*(size_t)(((LPBYTE)&pi->MacroFunctionNumber) - (LPBYTE)pi))*/));

	static wchar_t *szMenu[1], szMenu1[255];
	szMenu[0] = szMenu1;
	lstrcpynW(szMenu1, GetMsgW(CEPluginName), 240);

	pi->Flags = (gbBackgroundEnabled?PF_PRELOAD:0) | PF_EDITOR | PF_VIEWER;
	pi->PluginMenu.Guids = &guid_ConEmuLnPluginMenu;
	pi->PluginMenu.Strings = szMenu;
	pi->PluginMenu.Count = 1;
	pi->PluginConfig.Guids = &guid_ConEmuLnPluginConfig;
	pi->PluginConfig.Strings = szMenu;
	pi->PluginConfig.Count = 1;
}

void SetStartupInfoW2800(void *aInfo)
{
	INIT_FAR_PSI(::InfoW2800, ::FSFW2800, (PluginStartupInfo*)aInfo);

	VersionInfo FarVer = {0};
	if (InfoW2800->AdvControl(&guid_ConEmuLn, ACTL_GETFARMANAGERVERSION, 0, &FarVer))
	{
		if (FarVer.Major == 3)
		{
			gFarVersion.dwBuild = FarVer.Build;
			_ASSERTE(FarVer.Major<=0xFFFF && FarVer.Minor<=0xFFFF)
			gFarVersion.dwVerMajor = (WORD)FarVer.Major;
			gFarVersion.dwVerMinor = (WORD)FarVer.Minor;
			gFarVersion.Bis = (FarVer.Stage==VS_BIS);
		}
		else
		{
			_ASSERTE(FarVer.Major == 3);
		}
	}
	
	//int nLen = lstrlenW(InfoW2800->RootKey)+16;

	//if (gszRootKey) free(gszRootKey);

	//gszRootKey = (wchar_t*)calloc(nLen,2);
	//lstrcpyW(gszRootKey, InfoW2800->RootKey);
	//WCHAR* pszSlash = gszRootKey+lstrlenW(gszRootKey)-1;

	//if (*pszSlash != L'\\') *(++pszSlash) = L'\\';

	//lstrcpyW(pszSlash+1, L"ConEmuTh\\");
}

extern BOOL gbInfoW_OK;
HANDLE WINAPI OpenW2800(const void* apInfo)
{
	const struct OpenInfo *Info = (const struct OpenInfo*)apInfo;

	if (!gbInfoW_OK)
		return NULL;

	return OpenPluginWcmn(Info->OpenFrom, Info->Data);
}

void ExitFARW2800(void)
{
	if (InfoW2800)
	{
		free(InfoW2800);
		InfoW2800=NULL;
	}

	if (FSFW2800)
	{
		free(FSFW2800);
		FSFW2800=NULL;
	}
}

LPCWSTR GetMsgW2800(int aiMsg)
{
	if (!InfoW2800 || !InfoW2800->GetMsg)
		return L"";

	return InfoW2800->GetMsg(&guid_ConEmuLn,aiMsg);
}

#define FAR_UNICODE 1867
#include "Configure.h"

int ConfigureW2800(int ItemNumber)
{
	if (!InfoW2800)
		return false;

	return ConfigureProc(ItemNumber);
}

void SettingsLoadW2800()
{
	FarSettingsCreate sc = {sizeof(FarSettingsCreate), guid_ConEmuLn, INVALID_HANDLE_VALUE};
	FarSettingsItem fsi = {sizeof(fsi)};
	if (InfoW2800->SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc))
	{
		//BYTE cVal; DWORD nVal;

		for (ConEmuLnSettings *p = gSettings; p->pszValueName; p++)
		{
			if (p->nValueType == REG_BINARY)
			{
				_ASSERTE(p->nValueSize == 1);
				//cVal = (BYTE)*(BOOL*)p->pValue;
				fsi.Name = p->pszValueName;
				fsi.Type = FST_DATA;
				//fsi.Data.Size = 1; 
				//fsi.Data.Data = &cVal;
				if (InfoW2800->SettingsControl(sc.Handle, SCTL_GET, 0, &fsi) && (fsi.Data.Size == sizeof(BYTE)))
					*((BOOL*)p->pValue) = (*((BYTE*)fsi.Data.Data) != 0);
			}
			else if (p->nValueType == REG_DWORD)
			{
				_ASSERTE(p->nValueSize == 4);
				fsi.Name = p->pszValueName;
				fsi.Type = FST_DATA;
				//fsi.Data.Size = 4;
				//fsi.Data.Data = &nVal;
				if (InfoW2800->SettingsControl(sc.Handle, SCTL_GET, 0, &fsi) && (fsi.Data.Size == sizeof(DWORD)))
					*((DWORD*)p->pValue) = *((DWORD*)fsi.Data.Data);
			}
		}

		InfoW2800->SettingsControl(sc.Handle, SCTL_FREE, 0, 0);
	}
}
void SettingsSaveW2800()
{
	if (!InfoW2800)
		return;

	FarSettingsCreate sc = {sizeof(FarSettingsCreate), guid_ConEmuLn, INVALID_HANDLE_VALUE};
	FarSettingsItem fsi = {sizeof(fsi)};
	if (InfoW2800->SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc))
	{
		BYTE cVal;
		for (ConEmuLnSettings *p = gSettings; p->pszValueName; p++)
		{
			if (p->nValueType == REG_BINARY)
			{
				_ASSERTE(p->nValueSize == 1);
				cVal = (BYTE)*(BOOL*)p->pValue;
				fsi.Name = p->pszValueName;
				fsi.Type = FST_DATA;
				fsi.Data.Size = 1; 
				fsi.Data.Data = &cVal;
				InfoW2800->SettingsControl(sc.Handle, SCTL_SET, 0, &fsi);
			}
			else if (p->nValueType == REG_DWORD)
			{
				_ASSERTE(p->nValueSize == 4);
				fsi.Name = p->pszValueName;
				fsi.Type = FST_DATA;
				fsi.Data.Size = 4;
				fsi.Data.Data = p->pValue;
				InfoW2800->SettingsControl(sc.Handle, SCTL_SET, 0, &fsi);
			}
		}

		InfoW2800->SettingsControl(sc.Handle, SCTL_FREE, 0, 0);
	}
}
