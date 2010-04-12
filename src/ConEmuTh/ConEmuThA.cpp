
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
#include "..\common\pluginA.hpp"
#include "ConEmuTh.h"

//#define SHOW_DEBUG_EVENTS

// minimal(?) FAR version 1.71 alpha 4 build 2470
int WINAPI _export GetMinFarVersion(void)
{
    // Однако, FAR2 до сборки 748 не понимал две версии плагина в одном файле
    BOOL bFar2=FALSE;
	if (!LoadFarVersion())
		bFar2 = TRUE;
	else
		bFar2 = gFarVersion.dwVerMajor>=2;

    if (bFar2) {
	    return MAKEFARVERSION(2,0,748);
    }
	return MAKEFARVERSION(1,71,2470);
}


struct PluginStartupInfo *InfoA=NULL;
struct FarStandardFunctions *FSFA=NULL;


HANDLE WINAPI _export OpenPlugin(int OpenFrom,INT_PTR Item)
{
	if (InfoA == NULL)
		return INVALID_HANDLE_VALUE;


	return INVALID_HANDLE_VALUE;
}

#if defined(__GNUC__)
#ifdef __cplusplus
extern "C"{
#endif
	void WINAPI SetStartupInfo(const struct PluginStartupInfo *aInfo);
#ifdef __cplusplus
};
#endif
#endif

void WINAPI _export SetStartupInfo(const struct PluginStartupInfo *aInfo)
{
    //LoadFarVersion - уже вызван в GetStartupInfo
    
	::InfoA = (PluginStartupInfo*)calloc(sizeof(PluginStartupInfo),1);
	::FSFA = (FarStandardFunctions*)calloc(sizeof(FarStandardFunctions),1);
	if (::InfoA == NULL || ::FSFA == NULL)
		return;
	*::InfoA = *aInfo;
	*::FSFA = *aInfo->FSF;
	::InfoA->FSF = ::FSFA;
	
	MultiByteToWideChar(CP_ACP,0,InfoA->RootKey,lstrlenA(InfoA->RootKey)+1,gszRootKey,MAX_PATH);
	WCHAR* pszSlash = gszRootKey+lstrlenW(gszRootKey)-1;
	if (*pszSlash == L'\\') *(pszSlash--) = 0;
	while (pszSlash>gszRootKey && *pszSlash!=L'\\') pszSlash--;
	*pszSlash = 0;
}

void WINAPI _export GetPluginInfo(struct PluginInfo *pi)
{
    static char *szMenu[1], szMenu1[255];
	szMenu[0]=szMenu1;

	lstrcpynA(szMenu1, InfoA->GetMsg(InfoA->ModuleNumber,0), 240);

	pi->StructSize = sizeof(struct PluginInfo);
	pi->Flags = 0; // PF_PRELOAD; //TODO: Поставить Preload, если нужно будет восстанавливать при старте
	pi->DiskMenuStrings = NULL;
	pi->DiskMenuNumbers = 0;
	pi->PluginMenuStrings = szMenu;
	pi->PluginMenuStringsNumber = 1;
	pi->PluginConfigStrings = NULL;
	pi->PluginConfigStringsNumber = 0;
	pi->CommandPrefix = NULL;
	pi->Reserved = 0;	
}

void   WINAPI _export ExitFAR(void)
{
	StopThread();

	if (InfoA) {
		free(InfoA);
		InfoA=NULL;
	}
	if (FSFA) {
		free(FSFA);
		FSFA=NULL;
	}
}

int ShowMessageA(LPCSTR asMsg, int aiButtons)
{
	if (!InfoA || !InfoA->Message)
		return -1;
	return InfoA->Message(InfoA->ModuleNumber, FMSG_ALLINONE, NULL, 
		(const char * const *)asMsg, 0, aiButtons);
}

int ShowMessageA(int aiMsg, int aiButtons)
{
	if (!InfoA || !InfoA->Message || !InfoA->GetMsg)
		return -1;
	return ShowMessageA(
		(LPCSTR)InfoA->GetMsg(InfoA->ModuleNumber,aiMsg), aiButtons);
}

// Warning, напрямую НЕ вызывать. Пользоваться "общей" PostMacro
void PostMacroA(char* asMacro)
{
	if (!InfoA || !InfoA->AdvControl) return;

	ActlKeyMacro mcr;
	mcr.Command = MCMD_POSTMACROSTRING;
	mcr.Param.PlainText.Flags = 0; // По умолчанию - вывод на экран разрешен
	if (*asMacro == '@' && asMacro[1] && asMacro[1] != ' ') {
		mcr.Param.PlainText.Flags |= KSFLAGS_DISABLEOUTPUT;
		asMacro ++;
	}
	mcr.Param.PlainText.SequenceText = asMacro;
	InfoA->AdvControl(InfoA->ModuleNumber, ACTL_KEYMACRO, (void*)&mcr);
}

int ShowPluginMenuA()
{
	if (!InfoA)
		return -1;

	return -1;

	//FarMenuItemEx items[] = {
	//	{MIF_USETEXTPTR|(ConEmuHwnd ? MIF_SELECTED : MIF_DISABLE)},
	//	{MIF_USETEXTPTR|(ConEmuHwnd ? 0 : MIF_DISABLE)},
	//	{MIF_SEPARATOR},
	//	{MIF_USETEXTPTR|(ConEmuHwnd ? 0 : MIF_DISABLE)},
	//	{MIF_USETEXTPTR|(ConEmuHwnd ? 0 : MIF_DISABLE)},
	//	{MIF_USETEXTPTR|(ConEmuHwnd ? 0 : MIF_DISABLE)},
	//	{MIF_USETEXTPTR|(ConEmuHwnd ? 0 : MIF_DISABLE)},
	//	{MIF_SEPARATOR},
	//	{MIF_USETEXTPTR|(ConEmuHwnd||IsTerminalMode() ? MIF_DISABLE : MIF_SELECTED)},
	//	{MIF_SEPARATOR},
	//	{MIF_USETEXTPTR|(IsDebuggerPresent()||IsTerminalMode() ? MIF_DISABLE : 0)}
	//};
	//items[0].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,3);
	//items[1].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,4);
	//items[3].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,6);
	//items[4].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,7);
	//items[5].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,8);
	//items[6].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,9);
	//items[8].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,13);
	//items[10].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,14);

	//int nCount = sizeof(items)/sizeof(items[0]);

	//int nRc = InfoA->Menu(InfoA->ModuleNumber, -1,-1, 0, 
	//	FMENU_USEEXT|FMENU_AUTOHIGHLIGHT|FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
	//	InfoA->GetMsg(InfoA->ModuleNumber,2),
	//	NULL, NULL, NULL, NULL, (FarMenuItem*)items, nCount);

	//return nRc;
}

void GetMsgA(int aiMsg, wchar_t* rsMsg/*MAX_PATH*/)
{
	if (!rsMsg || !InfoA)
		return;
	LPCSTR pszMsg = InfoA->GetMsg(InfoA->ModuleNumber,aiMsg);
	if (pszMsg && *pszMsg) {
		int nLen = (int)lstrlenA(pszMsg);
		if (nLen>=MAX_PATH) nLen = MAX_PATH - 1;
		nLen = MultiByteToWideChar(CP_OEMCP, 0, pszMsg, nLen, rsMsg, MAX_PATH-1);
		if (nLen>=0) rsMsg[nLen] = 0;
	} else {
		rsMsg[0] = 0;
	}
}

BOOL IsMacroActiveA()
{
	if (!InfoA) return FALSE;

	ActlKeyMacro akm = {MCMD_GETSTATE};
	INT_PTR liRc = InfoA->AdvControl(InfoA->ModuleNumber, ACTL_KEYMACRO, &akm);
	if (liRc == MACROSTATE_NOMACRO)
		return FALSE;
	return TRUE;
}

void LoadPanelItemInfoA(CeFullPanelInfo* pi, int nItem)
{
	TODO("CeFullPanelInfo* LoadPanelInfoA()");
}

CeFullPanelInfo* LoadPanelInfoA(BOOL abActive)
{
	TODO("CeFullPanelInfo* LoadPanelInfoA()");
	return NULL;
}

void ReloadPanelsInfoA()
{
	TODO("void ReloadPanelsInfoA()");
}

BOOL IsLeftPanelActiveA()
{
	TODO("IsLeftPanelActiveA");
	return FALSE;
}

bool CheckWindowsA()
{
	if (!InfoA || !InfoA->AdvControl) return false;
	
	bool lbPanelsActive = false;
	int nCount = (int)InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWCOUNT, NULL);
	WindowInfo wi = {0};
	char szTypeName[64];
	char szName[MAX_PATH*2];
	char szInfo[MAX_PATH*4];

	OutputDebugStringW(L"\n\n");
	// Pos: Номер окна, о котором нужно узнать информацию. Нумерация идет с 0. Pos = -1 вернет информацию о текущем окне. 
	for (int i=-1; i <= nCount; i++) {
		memset(&wi, 0, sizeof(wi));
		wi.Pos = i;
		wi.TypeName[0] = 0;
		wi.Name[0] = 0;
		int iRc = InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWINFO, (LPVOID)&wi);
		if (iRc) {
			wsprintfA(szInfo, "%s%i: {%s-%s} %s\n",
				wi.Current ? "*" : "",
				i,
				(wi.Type==WTYPE_PANELS) ? "WTYPE_PANELS" :
				(wi.Type==WTYPE_VIEWER) ? "WTYPE_VIEWER" :
				(wi.Type==WTYPE_EDITOR) ? "WTYPE_EDITOR" :
				(wi.Type==WTYPE_DIALOG) ? "WTYPE_DIALOG" :
				(wi.Type==WTYPE_VMENU)  ? "WTYPE_VMENU"  :
				(wi.Type==WTYPE_HELP)   ? "WTYPE_HELP"   : "Unknown",
				szTypeName, szName);
		} else {
			wsprintfA(szInfo, "%i: <window absent>\n", i);
		}
		OutputDebugStringA(szInfo);
	}
	
	return lbPanelsActive;
}
