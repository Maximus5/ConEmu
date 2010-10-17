
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

#define _GetCheck(i) (int)InfoW995->SendDlgMessage(hDlg,DM_GETCHECK,i,0)
#define GetDataPtr(i) ((const wchar_t *)InfoW995->SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
#define SETTEXT(itm,txt) itm.PtrData = txt

int ConfigureW995(int ItemNumber)
{
	if (!InfoW995)
		return false;

    int height = 11;

	wchar_t szColorMask[16]; lstrcpyW(szColorMask, L"0xXXXXXX");
	wchar_t szColor[16];

    FarDialogItem items[] = {
        {DI_DOUBLEBOX,  3,  1,  38, height - 2},        //CEPluginName

		{DI_CHECKBOX,   5,  3,  0,  0,          true},    //CEPluginEnable

        {DI_TEXT,       5,  5,  0,  0,          false},    //CEColorLabel
		{DI_FIXEDIT,   29, 5,  36,  0,          false, {(DWORD_PTR)szColorMask}, DIF_MASKEDIT},

		{DI_TEXT,       0,  7,  0,  0,          false, {(DWORD_PTR)0}, DIF_SEPARATOR},

        {DI_BUTTON,     0,  8, 0,  0,          true,   {(DWORD_PTR)true},           DIF_CENTERGROUP,    true},     //CEBtnOK
        {DI_BUTTON,     0,  8, 0,  0,          true,   {(DWORD_PTR)false},          DIF_CENTERGROUP,    false},    //CEBtnCancel
    };
    
    SETTEXT(items[0], GetMsgW995(CEPluginName));
    SETTEXT(items[1], GetMsgW995(CEPluginEnable));
	items[1].Selected = gbBackgroundEnabled;
    SETTEXT(items[2], GetMsgW995(CEColorLabel));
	wsprintfW(szColor, L"%06X", (0xFFFFFF & gcrLinesColor));
    SETTEXT(items[3], szColor);
	SETTEXT(items[5], GetMsgW995(CEBtnOK));
	SETTEXT(items[6], GetMsgW995(CEBtnCancel));

    int dialog_res = 0;

	HANDLE hDlg = InfoW995->DialogInit ( InfoW995->ModuleNumber, -1, -1, 42, height,
		NULL/*L"Configure"*/, items, countof(items), 0, 0/*Flags*/, NULL/*ConfigDlgProcW*/, 0/*DlgProcParam*/ );


    dialog_res = InfoW995->DialogRun ( hDlg );

    if (dialog_res != -1 && dialog_res != 6)
    {
		HKEY hkey = NULL;
		if (!RegCreateKeyExW(HKEY_CURRENT_USER, gszRootKey, 0, 0, 0, KEY_ALL_ACCESS, 0, &hkey, NULL))
		{
			BYTE cVal; DWORD nVal = gcrLinesColor;
			gbBackgroundEnabled = cVal = _GetCheck(1);
			RegSetValueExW(hkey, L"PluginEnabled", 0, REG_BINARY, &cVal, sizeof(cVal));
			const wchar_t* psz = GetDataPtr(3);
			wchar_t* endptr = NULL;
			gcrLinesColor = nVal = wcstol(psz, &endptr, 16);
			RegSetValueExW(hkey, L"LinesColor", 0, REG_DWORD, (LPBYTE)&nVal, sizeof(nVal));
			RegCloseKey(hkey);

			StartPlugin(TRUE);
		}
    }

	InfoW995->DialogFree ( hDlg );
    
    return(true);
}
