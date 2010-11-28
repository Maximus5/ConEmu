
/*
Copyright (c) 2010 Maximus5
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

#include <windows.h>
#include "..\common\pluginW1007.hpp" // Отличается от 995 наличием SynchoApi
#include "ConEmuLn.h"

#define FCTL_GETPANELDIR FCTL_GETCURRENTDIRECTORY

#define _ACTL_GETFARRECT 32

#ifdef _DEBUG
#define SHOW_DEBUG_EVENTS
#endif

struct PluginStartupInfo *InfoW995=NULL;
struct FarStandardFunctions *FSFW995=NULL;


void SetStartupInfoW995(void *aInfo)
{
	::InfoW995 = (PluginStartupInfo*)calloc(sizeof(PluginStartupInfo),1);
	::FSFW995 = (FarStandardFunctions*)calloc(sizeof(FarStandardFunctions),1);
	if (::InfoW995 == NULL || ::FSFW995 == NULL)
		return;
	*::InfoW995 = *((struct PluginStartupInfo*)aInfo);
	*::FSFW995 = *((struct PluginStartupInfo*)aInfo)->FSF;
	::InfoW995->FSF = ::FSFW995;

	int nLen = lstrlenW(InfoW995->RootKey)+16;
	if (gszRootKey) free(gszRootKey);
	gszRootKey = (wchar_t*)calloc(nLen,2);
	lstrcpyW(gszRootKey, InfoW995->RootKey);
	WCHAR* pszSlash = gszRootKey+lstrlenW(gszRootKey)-1;
	if (*pszSlash != L'\\') *(++pszSlash) = L'\\';
	lstrcpyW(pszSlash+1, L"ConEmuTh\\");
}

void ExitFARW995(void)
{
	if (InfoW995) {
		free(InfoW995);
		InfoW995=NULL;
	}
	if (FSFW995) {
		free(FSFW995);
		FSFW995=NULL;
	}
}

LPCWSTR GetMsgW995(int aiMsg)
{
	if (!InfoW995 || !InfoW995->GetMsg)
		return L"";
	return InfoW995->GetMsg(InfoW995->ModuleNumber,aiMsg);
}

#define FAR_UNICODE
#include "Configure.h"

int ConfigureW995(int ItemNumber)
{
	if (!InfoW995)
		return false;

	return ConfigureProc(ItemNumber);
}
