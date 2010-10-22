
#pragma once

#ifdef FAR_UNICODE
	#define InfoT InfoW995
	#define _GetCheck(i) (int)InfoW995->SendDlgMessage(hDlg,DM_GETCHECK,i,0)
	#define GetDataPtr(i) ((const wchar_t *)InfoW995->SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
	#define SETTEXT(itm,txt) itm.PtrData = txt
	#define wsprintfT wsprintfW
	#define GetMsgT GetMsgW
	#define FAR_T(s) L ## s
	#define FAR_CHAR wchar_t
	#define strcpyT lstrcpyW
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
#endif


enum {
	cfgTitle       = 0,
	cfgShowLines   = 1,
	cfgColorLabel  = 2,
	cfgColor       = 3,
	cfgSeparator   = 4,
	cfgOk          = 5,
	cfgCancel      = 6,
};

static LONG_PTR WINAPI ConfigDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
{
	if (Msg == DN_BTNCLICK)
	{
		if (Param1 == cfgShowLines)
		{
			gbBackgroundEnabled = (int)Param2;
			// Обновить или отключить 
		    StartPlugin(TRUE /*НЕ считывать параметры из реестра*/);
		}
	}
	else if (Msg == DN_EDITCHANGE)
	{
		if (Param1 == cfgColor)
		{
			FarDialogItem* pItem = (FarDialogItem*)Param2;
			FAR_CHAR* endptr = NULL;
			#ifdef FAR_UNICODE
				gcrLinesColor = wcstoul(pItem->PtrData, &endptr, 16);
			#else
				gcrLinesColor = strtoul(pItem->Data, &endptr, 16);
			#endif
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

    int height = 11;

	FAR_CHAR szColorMask[16]; strcpyT(szColorMask, FAR_T("0xXXXXXX"));
	FAR_CHAR szColor[16];

    FarDialogItem items[] =
    {
        {DI_DOUBLEBOX, 3,  1,  38, height - 2}, //cfgTitle

		{DI_CHECKBOX,  5,  3,  0,  0, true},    //cfgShowLines

        {DI_TEXT,      5,  5,  0,  0, false},   //cfgColorLabel, cfgColor
		{DI_FIXEDIT,  29, 5,  36,  0, false, {(DWORD_PTR)szColorMask}, DIF_MASKEDIT},

		{DI_TEXT,      0,  7,  0,  0, false, {(DWORD_PTR)0},           DIF_SEPARATOR},

        {DI_BUTTON,    0,  8,  0,  0, true,  {(DWORD_PTR)true},        DIF_CENTERGROUP, true},  //cfgOk
        {DI_BUTTON,    0,  8,  0,  0, true,  {(DWORD_PTR)false},       DIF_CENTERGROUP, false}, //cfgCancel
    };
    
    SETTEXT(items[cfgTitle], GetMsgT(CEPluginName));
    SETTEXT(items[cfgShowLines], GetMsgT(CEPluginEnable));
	items[cfgShowLines].Selected = gbBackgroundEnabled;
    SETTEXT(items[cfgColorLabel], GetMsgT(CEColorLabel));
	wsprintfT(szColor, FAR_T("%06X"), (0xFFFFFF & gcrLinesColor));
    SETTEXT(items[cfgColor], szColor);
	SETTEXT(items[cfgOk], GetMsgT(CEBtnOK));
	SETTEXT(items[cfgCancel], GetMsgT(CEBtnCancel));

    int dialog_res = 0;
    
    // Запомнить текущие значения, чтобы восстановить их если Esc нажат
	BOOL bCurBackgroundEnabled = gbBackgroundEnabled;
	COLORREF crCurLinesColor = gcrLinesColor;
    

    #ifdef FAR_UNICODE
		HANDLE hDlg = InfoT->DialogInit ( InfoT->ModuleNumber, -1, -1, 42, height,
			NULL/*L"Configure"*/, items, countof(items), 0, 0/*Flags*/, ConfigDlgProc, 0/*DlgProcParam*/ );
	    dialog_res = InfoT->DialogRun ( hDlg );
	#else
		dialog_res = InfoT->DialogEx(InfoT->ModuleNumber,-1,-1,42,height,
			NULL/*"Configure"*/, items, countof(items), 0, 0, ConfigDlgProc, NULL);
	#endif

    if (dialog_res != -1 && dialog_res != cfgCancel)
    {
		HKEY hkey = NULL;
		if (!RegCreateKeyExW(HKEY_CURRENT_USER, gszRootKey, 0, 0, 0, KEY_ALL_ACCESS, 0, &hkey, NULL))
		{
			BYTE cVal; DWORD nVal = gcrLinesColor;
			
			gbBackgroundEnabled = cVal = _GetCheck(cfgShowLines);
			RegSetValueExW(hkey, L"PluginEnabled", 0, REG_BINARY, &cVal, sizeof(cVal));
			
			const FAR_CHAR* psz;
			FAR_CHAR* endptr = NULL;
			#ifdef FAR_UNICODE
				psz = GetDataPtr(cfgColor);
				gcrLinesColor = nVal = wcstoul(psz, &endptr, 16);
			#else
				psz = GetDataPtr(cfgColor);
				gcrLinesColor = nVal = strtoul(psz, &endptr, 16);
			#endif
			RegSetValueExW(hkey, L"LinesColor", 0, REG_DWORD, (LPBYTE)&nVal, sizeof(nVal));
			
			RegCloseKey(hkey);
		}
    }
    else
    {
		gbBackgroundEnabled = bCurBackgroundEnabled;
		gcrLinesColor = crCurLinesColor;
    }

	// Обновить или отключить 
    StartPlugin(TRUE /*НЕ считывать параметры из реестра*/);

    #ifdef FAR_UNICODE
	InfoT->DialogFree ( hDlg );
	#endif
    
    return(true);
}
