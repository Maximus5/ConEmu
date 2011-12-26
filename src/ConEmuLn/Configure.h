
/*
Copyright (c) 2010-2011 Maximus5
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

#pragma once

#ifdef FAR_UNICODE
	#if FAR_UNICODE>=1867
		#define InfoT InfoW1900
		#define _GetCheck(i) (int)InfoW1900->SendDlgMessage(hDlg,DM_GETCHECK,i,0)
		#define GetDataPtr(i) ((const wchar_t *)InfoW1900->SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
		#define GetTextPtr(i) ((const wchar_t *)InfoW1900->SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
		#define SETTEXT(itm,txt) itm.Data = txt
		#define wsprintfT wsprintfW
		#define GetMsgT GetMsgW
		#define FAR_T(s) L ## s
		#define FAR_CHAR wchar_t
		#define strcpyT lstrcpyW
		#define strlenT lstrlenW
		#define FAR_PTR void*
	#else
		#define InfoT InfoW995
		#define _GetCheck(i) (int)InfoW995->SendDlgMessage(hDlg,DM_GETCHECK,i,0)
		#define GetDataPtr(i) ((const wchar_t *)InfoW995->SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
		#define GetTextPtr(i) ((const wchar_t *)InfoW995->SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
		#define SETTEXT(itm,txt) itm.PtrData = txt
		#define wsprintfT wsprintfW
		#define GetMsgT GetMsgW
		#define FAR_T(s) L ## s
		#define FAR_CHAR wchar_t
		#define strcpyT lstrcpyW
		#define strlenT lstrlenW
		#define FAR_PTR LONG_PTR
	#endif
#else
	#define InfoT InfoA
	#define _GetCheck(i) items[i].Selected
	#define GetDataPtr(i) items[i].Data
	#define SETTEXT(itm,txt) lstrcpyA(itm.Data, txt)
	#define wsprintfT wsprintfA
	#define GetMsgT GetMsg
	#define FAR_T(s) s
	#define FAR_CHAR char
	#define strcpyT lstrcpyA
	#define strlenT lstrlenA
	#define FAR_PTR LONG_PTR
#endif


enum
{
	cfgTitle       = 0,
	cfgShowLines   = 1,
	cfgColorLabel  = 2,
	cfgColor       = 3,
	cfgTypeLines   = 4,
	cfgTypeStripes = 5,
	cfgHilight     = 6,
	cfgPlugLabel   = 7,
	cfgPlugBack    = 8,
	cfgSeparator   = 9,
	cfgOk          = 10,
	cfgCancel      = 11,
};

#if FAR_UNICODE>=1867
static INT_PTR WINAPI ConfigDlgProc(HANDLE hDlg, int Msg, int Param1, FAR_PTR Param2)
#else
static LONG_PTR WINAPI ConfigDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
#endif
{
	if (Msg == DN_BTNCLICK)
	{
		if ((Param1 == cfgShowLines) || (Param1 == cfgTypeLines) || (Param1 == cfgTypeStripes) || (Param1 == cfgHilight))
		{
			if (Param1 == cfgShowLines)
				gbBackgroundEnabled = (int)Param2; //-V205
			else if (Param1 == cfgTypeLines)
				giHilightType = 0;
			else if (Param1 == cfgTypeStripes)
				giHilightType = 1;
			else if (Param1 == cfgHilight)
				gbHilightPlugins = (int)Param2; //-V205

			// Обновить или отключить
			StartPlugin(TRUE /*НЕ считывать параметры из реестра*/);
		}
	}
	else if (Msg == DN_EDITCHANGE)
	{
		if ((Param1 == cfgColor) || (Param1 == cfgPlugBack))
		{
			FarDialogItem* pItem = (FarDialogItem*)Param2;
			FAR_CHAR* endptr = NULL;
			COLORREF clr = 0;
#ifdef FAR_UNICODE
	#if FAR_UNICODE>=1867
			clr = wcstoul(pItem->Data, &endptr, 16);
	#else
			clr = wcstoul(pItem->PtrData, &endptr, 16);
	#endif
#else
			clr = strtoul(pItem->Data, &endptr, 16);
#endif

			if (Param1 == cfgColor)
				gcrLinesColor = clr;
			else if (Param1 == cfgPlugBack)
				gcrHilightPlugBack = clr;

			// Обновить или отключить
			StartPlugin(TRUE /*НЕ считывать параметры из реестра*/);
		}
	}

	return InfoT->DefDlgProc(hDlg, Msg, Param1, Param2);
}

static int ConfigureProc(int ItemNumber)
{
	if (!InfoT)
		return false;

	int height = 15;
	FAR_CHAR szColorMask[16]; strcpyT(szColorMask, FAR_T("0xXXXXXX"));
	FAR_CHAR szColor[16], szBack[16];
	FarDialogItem items[] =
	{
		{DI_DOUBLEBOX, 3,  1,  38, height - 2}, //cfgTitle

		#if FAR_UNICODE>=1867
		{DI_CHECKBOX,  5,  3,  0,  0, {0}, NULL, NULL, DIF_FOCUS},    //cfgShowLines
		#else
		{DI_CHECKBOX,  5,  3,  0,  0, true},    //cfgShowLines
		#endif

		{DI_TEXT,      5,  5,  0,  0},   //cfgColorLabel, cfgColor
		#if FAR_UNICODE>=1867
		{DI_FIXEDIT,  29,  5,  36, 0, {(DWORD_PTR)0}, NULL, szColorMask, DIF_MASKEDIT},
		#else
		{DI_FIXEDIT,  29,  5,  36, 0, false, {(DWORD_PTR)szColorMask}, DIF_MASKEDIT},
		#endif
		
		#if FAR_UNICODE>=1867
		{DI_RADIOBUTTON,  5,  6,  0,  0, {(DWORD_PTR)0}, NULL, NULL, DIF_GROUP},    //cfgTypeLines
		#else
		{DI_RADIOBUTTON,  5,  6,  0,  0, 0, {(DWORD_PTR)0}, DIF_GROUP},    //cfgTypeLines
		#endif
		{DI_RADIOBUTTON,  5,  6,  0,  0},    //cfgTypeStripes

		{DI_CHECKBOX,  5,  8,  0,  0},    //cfgHilight
		{DI_TEXT,      5,  9,  0,  0},   //cfgPlugLabel, cfgPlugBack
		#if FAR_UNICODE>=1867
		{DI_FIXEDIT,  29,  9,  36, 0, {0}, NULL, szColorMask, DIF_MASKEDIT},
		{DI_TEXT,      0, 11,  0,  0, {(DWORD_PTR)0}, NULL, NULL, DIF_SEPARATOR},
		#else
		{DI_FIXEDIT,  29,  9,  36, 0, false, {(DWORD_PTR)szColorMask}, DIF_MASKEDIT},
		{DI_TEXT,      0, 11,  0,  0, false, {(DWORD_PTR)0}, DIF_SEPARATOR},
		#endif

		#if FAR_UNICODE>=1867
		{DI_BUTTON,    0, 12,  0,  0, {(DWORD_PTR)0}, NULL, NULL, DIF_CENTERGROUP|DIF_DEFAULTBUTTON},  //cfgOk
		{DI_BUTTON,    0, 12,  0,  0, {(DWORD_PTR)0}, NULL, NULL, DIF_CENTERGROUP}, //cfgCancel
		#else
		{DI_BUTTON,    0, 12,  0,  0, false,  {(DWORD_PTR)true},        DIF_CENTERGROUP, true},  //cfgOk
		{DI_BUTTON,    0, 12,  0,  0, false,  {(DWORD_PTR)false},       DIF_CENTERGROUP, false}, //cfgCancel
		#endif
	};
	SETTEXT(items[cfgTitle], GetMsgT(CEPluginName));
	SETTEXT(items[cfgShowLines], GetMsgT(CEPluginEnable));
	items[cfgShowLines].Selected = gbBackgroundEnabled;
	SETTEXT(items[cfgColorLabel], GetMsgT(CEColorLabel));
	wsprintfT(szColor, FAR_T("%06X"), (0xFFFFFF & gcrLinesColor));
	SETTEXT(items[cfgColor], szColor);
	SETTEXT(items[cfgTypeLines], GetMsgT(CEColorTypeLines));
	items[cfgTypeStripes].X1 += strlenT(GetMsgT(CEColorTypeLines)) + 4;
	SETTEXT(items[cfgTypeStripes], GetMsgT(CEColorTypeStripes));
	items[(giHilightType == 0) ? cfgTypeLines : cfgTypeStripes].Selected = TRUE;
	SETTEXT(items[cfgHilight], GetMsgT(CEHilightPlugins));
	items[cfgHilight].Selected = gbHilightPlugins;
	SETTEXT(items[cfgPlugLabel], GetMsgT(CEPluginColor));
	wsprintfT(szBack, FAR_T("%06X"), (0xFFFFFF & gcrHilightPlugBack));
	SETTEXT(items[cfgPlugBack], szBack);
	SETTEXT(items[cfgOk], GetMsgT(CEBtnOK));
	SETTEXT(items[cfgCancel], GetMsgT(CEBtnCancel));
	int dialog_res = 0;
	// Запомнить текущие значения, чтобы восстановить их если Esc нажат
	BOOL bCurBackgroundEnabled = gbBackgroundEnabled;
	COLORREF crCurLinesColor = gcrLinesColor;
	BOOL bCurHilightPlugins = gbHilightPlugins;
	COLORREF crCurHilightPlugBack = gcrHilightPlugBack;
#ifdef FAR_UNICODE
	HANDLE hDlg = NULL;
	#if FAR_UNICODE>=1867
		hDlg = InfoT->DialogInit(&guid_ConEmuLn, &guid_ConEmuLnCfgDlg,
								 -1, -1, 42, height,
								 NULL/*L"Configure"*/, items, countof(items), 
								 0, 0/*Flags*/, ConfigDlgProc, 0);
	#else
		hDlg = InfoT->DialogInit(InfoT->ModuleNumber, -1, -1, 42, height,
								 NULL/*L"Configure"*/, items, countof(items), 0, 0/*Flags*/, ConfigDlgProc, 0/*DlgProcParam*/);
	#endif
	dialog_res = InfoT->DialogRun(hDlg);
#else
	dialog_res = InfoT->DialogEx(InfoT->ModuleNumber,-1,-1,42,height,
	                             NULL/*"Configure"*/, items, countof(items), 0, 0, ConfigDlgProc, 0);
#endif

	if (dialog_res != -1 && dialog_res != cfgCancel)
	{
		//HKEY hkey = NULL;

		//if (!RegCreateKeyExW(HKEY_CURRENT_USER, gszRootKey, 0, 0, 0, KEY_ALL_ACCESS, 0, &hkey, NULL))
		//{
		BYTE cVal; DWORD nVal = gcrLinesColor;
		gbBackgroundEnabled = cVal = _GetCheck(cfgShowLines);
		//RegSetValueExW(hkey, L"PluginEnabled", 0, REG_BINARY, &cVal, sizeof(cVal));
		const FAR_CHAR* psz;
		FAR_CHAR* endptr = NULL;
#ifdef FAR_UNICODE
		psz = GetDataPtr(cfgColor);
		gcrLinesColor = nVal = wcstoul(psz, &endptr, 16);
#else
		psz = GetDataPtr(cfgColor);
		gcrLinesColor = nVal = strtoul(psz, &endptr, 16);
#endif
		//RegSetValueExW(hkey, L"LinesColor", 0, REG_DWORD, (LPBYTE)&nVal, sizeof(nVal));
		gbHilightPlugins = cVal = _GetCheck(cfgHilight);
		//RegSetValueExW(hkey, L"HilightPlugins", 0, REG_BINARY, &cVal, sizeof(cVal));
		endptr = NULL;
#ifdef FAR_UNICODE
		psz = GetDataPtr(cfgPlugBack);
		gcrHilightPlugBack = nVal = wcstoul(psz, &endptr, 16);
#else
		psz = GetDataPtr(cfgPlugBack);
		gcrHilightPlugBack = nVal = strtoul(psz, &endptr, 16);
#endif
		//RegSetValueExW(hkey, L"HilightPlugBack", 0, REG_DWORD, (LPBYTE)&nVal, sizeof(nVal));
		//RegCloseKey(hkey);
		//}
		SettingsSave();
	}
	else
	{
		gbBackgroundEnabled = bCurBackgroundEnabled;
		gcrLinesColor = crCurLinesColor;
		gbHilightPlugins = bCurHilightPlugins;
		gcrHilightPlugBack = crCurHilightPlugBack;
	}

	// Обновить или отключить
	StartPlugin(TRUE /*НЕ считывать параметры из реестра*/);
#ifdef FAR_UNICODE
	InfoT->DialogFree(hDlg);
#endif
	return(true);
}
