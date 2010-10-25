
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
#include "ConEmuLn.h"

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

	int nLen = lstrlenA(InfoA->RootKey)+16;
	if (gszRootKey) free(gszRootKey);
	gszRootKey = (wchar_t*)calloc(nLen,2);
	MultiByteToWideChar(CP_OEMCP,0,InfoA->RootKey,-1,gszRootKey,nLen);
	WCHAR* pszSlash = gszRootKey+lstrlenW(gszRootKey)-1;
	if (*pszSlash != L'\\') *(++pszSlash) = L'\\';
	lstrcpyW(pszSlash+1, L"ConEmuLn\\");

	StartPlugin(FALSE);
}

void WINAPI _export GetPluginInfo(struct PluginInfo *pi)
{
    static char *szMenu[1], szMenu1[255];
	szMenu[0]=szMenu1;

	lstrcpynA(szMenu1, InfoA->GetMsg(InfoA->ModuleNumber,CEPluginName), 240);

	pi->StructSize = sizeof(struct PluginInfo);
	pi->Flags = PF_PRELOAD;
	pi->DiskMenuStrings = NULL;
	pi->DiskMenuNumbers = 0;
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

	if (InfoA) {
		free(InfoA);
		InfoA=NULL;
	}
	if (FSFA) {
		free(FSFA);
		FSFA=NULL;
	}
}

static char *GetMsg(int MsgId)
{
    return((char*)InfoA->GetMsg(InfoA->ModuleNumber, MsgId));
}

//LONG_PTR WINAPI ConfigDlgProcA(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
//{
//	if (Msg == DN_BTNCLICK) {
//		if (Param1 == FPCDVideoExtsReset) {
//			InfoA->SendDlgMessage(hDlg, DM_SETTEXTPTR, FPCDVideoExtsData, (LONG_PTR)VIDEO_EXTS);
//			InfoA->SendDlgMessage(hDlg, DM_SETFOCUS, FPCDVideoExtsData, 0);
//			return TRUE;
//		} else if (Param1 == FPCDAudioExtsReset) {
//			InfoA->SendDlgMessage(hDlg, DM_SETTEXTPTR, FPCDAudioExtsData, (LONG_PTR)AUDIO_EXTS);
//			InfoA->SendDlgMessage(hDlg, DM_SETFOCUS, FPCDAudioExtsData, 0);
//			return TRUE;
//		} else if (Param1 == FPCDPictureExtsReset) {
//			InfoA->SendDlgMessage(hDlg, DM_SETTEXTPTR, FPCDPictureExtsData, (LONG_PTR)PICTURE_EXTS);
//			InfoA->SendDlgMessage(hDlg, DM_SETFOCUS, FPCDPictureExtsData, 0);
//			return TRUE;
//		}
//	}
//	return InfoA->DefDlgProc(hDlg, Msg, Param1, Param2);
//}

//#ifdef _UNICODE
//    #define _tcsscanf swscanf
//    #define SETMENUTEXT(itm,txt) itm.Text = txt;
//    #define F757NA 0,
//    #define _GetCheck(i) (int)InfoA->SendDlgMessage(hDlg,DM_GETCHECK,i,0)
//    #define GetDataPtr(i) ((const TCHAR *)InfoA->SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
//    #define SETTEXT(itm,txt) itm.PtrData = txt
//    #define SETTEXTPRINT(itm,fmt,arg) wsprintf(pszBuf, fmt, arg); SETTEXT(itm,pszBuf); pszBuf+=lstrlen(pszBuf)+2;
//    #define _tcstoi _wtoi
//#else
//    #define _tcsscanf sscanf
//    #define SETMENUTEXT(itm,txt) lstrcpy(itm.Text, txt);
//    #define F757NA
    //#define _GetCheck(i) items[i].Selected
    //#define GetDataPtr(i) items[i].Data
//#define SETTEXT(itm,txt) lstrcpyA(itm.Data, txt)
//    #define SETTEXTPRINT(itm,fmt,arg) wsprintf(itm.Data, fmt, arg)
//    #define _tcstoi atoi
//#endif

#undef FAR_UNICODE
#include "Configure.h"

int WINAPI Configure(int ItemNumber)
{
	return ConfigureProc(ItemNumber);
    //int height = 11;
    //
	//char szColorMask[16]; lstrcpyA(szColorMask, "0xXXXXXX");
	//char szColor[16];
	//
    //FarDialogItem items[] = {
    //    {DI_DOUBLEBOX,  3,  1,  38, height - 2},        //CEPluginName
    //
	//	{DI_CHECKBOX,   5,  3,  0,  0,          true},    //CEPluginEnable
	//
    //    {DI_TEXT,       5,  5,  0,  0,          false},    //CEColorLabel
	//	{DI_FIXEDIT,   29, 5,  36,  0,          false, {(DWORD_PTR)szColorMask}, DIF_MASKEDIT},
	//
	//	{DI_TEXT,       0,  7,  0,  0,          false, {(DWORD_PTR)0}, DIF_SEPARATOR},
	//
    //    {DI_BUTTON,     0,  8, 0,  0,          true,   {(DWORD_PTR)true},           DIF_CENTERGROUP,    true},     //CEBtnOK
    //    {DI_BUTTON,     0,  8, 0,  0,          true,   {(DWORD_PTR)false},          DIF_CENTERGROUP,    false},    //CEBtnCancel
    //};
    //
    //SETTEXT(items[0], GetMsg(CEPluginName));
    //SETTEXT(items[1], GetMsg(CEPluginEnable));
	//items[1].Selected = gbBackgroundEnabled;
    //SETTEXT(items[2], GetMsg(CEColorLabel));
	//wsprintfA(szColor, "%06X", (0xFFFFFF & gcrLinesColor));
    //SETTEXT(items[3], szColor);
	//SETTEXT(items[5], GetMsg(CEBtnOK));
	//SETTEXT(items[6], GetMsg(CEBtnCancel));
	//
    //int dialog_res = 0;
    //
	//dialog_res = InfoA->DialogEx(InfoA->ModuleNumber,-1,-1,42,height, NULL/*"Configure"*/, items, countof(items), 0, 0,
	//	NULL/*ConfigDlgProcA*/, NULL);
	//
    //if (dialog_res != -1 && dialog_res != 6)
    //{
	//	HKEY hkey = NULL;
	//	if (!RegCreateKeyExW(HKEY_CURRENT_USER, gszRootKey, 0, 0, 0, KEY_ALL_ACCESS, 0, &hkey, NULL))
	//	{
	//		BYTE cVal; DWORD nVal = gcrLinesColor;
	//		gbBackgroundEnabled = cVal = _GetCheck(1);
	//		RegSetValueExW(hkey, L"PluginEnabled", 0, REG_BINARY, &cVal, sizeof(cVal));
	//		char* psz = GetDataPtr(3);
	//		char* endptr = NULL;
	//		gcrLinesColor = nVal = strtoul(psz, &endptr, 16);
	//		RegSetValueExW(hkey, L"LinesColor", 0, REG_DWORD, (LPBYTE)&nVal, sizeof(nVal));
	//		RegCloseKey(hkey);
	//
	//		StartPlugin(TRUE);
	//	}
    //}
    //
    //return(true);
}

HANDLE WINAPI _export OpenPlugin(int OpenFrom,INT_PTR Item)
{
	if (InfoA == NULL)
		return INVALID_HANDLE_VALUE;

	Configure(0);

	return INVALID_HANDLE_VALUE;
}
