
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

#pragma once

#ifdef FAR_UNICODE
	#if FAR_UNICODE>=1867
		#if FARMANAGERVERSION_BUILD>=2800
			#define FAR_INT intptr_t
			#define InfoT InfoW2800
		#else
			#define FAR_INT int
			#define InfoT InfoW1900
		#endif
		#define _GetCheck(i) (int)InfoT->SendDlgMessage(hDlg,DM_GETCHECK,i,0)
		#define GetDataPtr(i) ((const wchar_t *)InfoT->SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
		#define GetTextPtr(i) ((const wchar_t *)InfoT->SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
		#define SETTEXT(itm,txt) itm.Data = txt
		#define wsprintfT wsprintfW
		#define GetMsgT GetMsgW
		#define FAR_T(s) L ## s
		#define FAR_CHAR wchar_t
		#define strcpyT lstrcpyW
		#define strlenT lstrlenW
		#define FAR_PTR void*
	#else
		#define FAR_INT int
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
	#define FAR_INT int
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
	cfgShowLines,
	cfgTypeLines,
	cfgTypeStripes,
	cfgPanel,
	cfgColorPanel,
	cfgEditor,
	cfgColorEditor,
	cfgViewer,
	cfgColorViewer,
	cfgHilight,
	cfgPlugLabel,
	cfgPlugBack,
	cfgSeparator,
	cfgOk,
	cfgCancel,
};

#if FAR_UNICODE>=1867
static INT_PTR WINAPI ConfigDlgProc(HANDLE hDlg, FAR_INT Msg, FAR_INT Param1, FAR_PTR Param2)
#else
static LONG_PTR WINAPI ConfigDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
#endif
{
	if (Msg == DN_BTNCLICK)
	{
		if ((Param1 == cfgShowLines)
			|| (Param1 == cfgPanel)
			|| (Param1 == cfgEditor)
			|| (Param1 == cfgViewer)
			|| (Param1 == cfgTypeLines) || (Param1 == cfgTypeStripes)
			|| (Param1 == cfgHilight))
		{
			switch (LODWORD(Param1))
			{
			case cfgShowLines:
				gbBackgroundEnabled = LODWORD(Param2); break;
			case cfgPanel:
				gbLinesColorPanel = LODWORD(Param2); break;
			case cfgEditor:
				gbLinesColorEditor = LODWORD(Param2); break;
			case cfgViewer:
				gbLinesColorViewer = LODWORD(Param2); break;
			case cfgTypeLines:
				giHilightType = 0; break;
			case cfgTypeStripes:
				giHilightType = 1; break;
			case cfgHilight:
				gbHilightPlugins = LODWORD(Param2); break;
			}

			// Обновить или отключить
			StartPlugin(TRUE /*НЕ считывать параметры из реестра*/);
		}
	}
	else if (Msg == DN_EDITCHANGE)
	{
		if ((Param1 == cfgColorPanel)
			|| (Param1 == cfgColorEditor)
			|| (Param1 == cfgColorViewer)
			|| (Param1 == cfgPlugBack))
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

			switch (LODWORD(Param1))
			{
			case cfgColorPanel:
				gcrLinesColorPanel = clr; break;
			case cfgColorEditor:
				gcrLinesColorEditor = clr; break;
			case cfgColorViewer:
				gcrLinesColorViewer = clr; break;
			case cfgPlugBack:
				gcrHilightPlugBack = clr; break;
			}

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

	int height = 18;
	FAR_CHAR szColorMask[16]; strcpyT(szColorMask, FAR_T("0xXXXXXX"));
	FAR_CHAR szColorP[16], szColorE[16], szColorV[16], szBack[16];
	FarDialogItem items[] =
	{
		{DI_DOUBLEBOX, 3,  1,  38, height - 2}, //cfgTitle

		#if FAR_UNICODE>=1867
		{DI_CHECKBOX,  5,  3,  0,  0, {0}, NULL, NULL, DIF_FOCUS},    //cfgShowLines
		#else
		{DI_CHECKBOX,  5,  3,  0,  0, true},    //cfgShowLines
		#endif

		#if FAR_UNICODE>=1867
		{DI_RADIOBUTTON,  5,  5,  0,  0, {(DWORD_PTR)0}, NULL, NULL, DIF_GROUP},    //cfgTypeLines
		#else
		{DI_RADIOBUTTON,  5,  5,  0,  0, 0, {(DWORD_PTR)0}, DIF_GROUP},    //cfgTypeLines
		#endif
		{DI_RADIOBUTTON,  5,  5,  0,  0},    //cfgTypeStripes


		{DI_CHECKBOX,  5,  7,  0,  0},   //cfgPanel
		#if FAR_UNICODE>=1867
		{DI_FIXEDIT,  29,  7,  36, 0, {(DWORD_PTR)0}, NULL, szColorMask, DIF_MASKEDIT}, // cfgColorPanel
		#else
		{DI_FIXEDIT,  29,  7,  36, 0, false, {(DWORD_PTR)szColorMask}, DIF_MASKEDIT}, // cfgColorPanel
		#endif

		{DI_CHECKBOX,  5,  8,  0,  0},   //cfgEditor
		#if FAR_UNICODE>=1867
		{DI_FIXEDIT,  29,  8,  36, 0, {(DWORD_PTR)0}, NULL, szColorMask, DIF_MASKEDIT}, // cfgColorEditor
		#else
		{DI_FIXEDIT,  29,  8,  36, 0, false, {(DWORD_PTR)szColorMask}, DIF_MASKEDIT}, // cfgColorEditor
		#endif

		{DI_CHECKBOX,  5,  9,  0,  0},   //cfgViewer
		#if FAR_UNICODE>=1867
		{DI_FIXEDIT,  29,  9,  36, 0, {(DWORD_PTR)0}, NULL, szColorMask, DIF_MASKEDIT}, // cfgColorViewer
		#else
		{DI_FIXEDIT,  29,  9,  36, 0, false, {(DWORD_PTR)szColorMask}, DIF_MASKEDIT}, // cfgColorViewer
		#endif


		{DI_CHECKBOX,  5, 11,  0,  0},    //cfgHilight
		{DI_TEXT,      5, 12,  0,  0},   //cfgPlugLabel, cfgPlugBack
		#if FAR_UNICODE>=1867
		{DI_FIXEDIT,  29, 12,  36, 0, {0}, NULL, szColorMask, DIF_MASKEDIT},
		{DI_TEXT,      0, 14,  0,  0, {(DWORD_PTR)0}, NULL, NULL, DIF_SEPARATOR},
		#else
		{DI_FIXEDIT,  29, 12,  36, 0, false, {(DWORD_PTR)szColorMask}, DIF_MASKEDIT},
		{DI_TEXT,      0, 14,  0,  0, false, {(DWORD_PTR)0}, DIF_SEPARATOR},
		#endif

		#if FAR_UNICODE>=1867
		{DI_BUTTON,    0, 15,  0,  0, {(DWORD_PTR)0}, NULL, NULL, DIF_CENTERGROUP|DIF_DEFAULTBUTTON},  //cfgOk
		{DI_BUTTON,    0, 15,  0,  0, {(DWORD_PTR)0}, NULL, NULL, DIF_CENTERGROUP}, //cfgCancel
		#else
		{DI_BUTTON,    0, 15,  0,  0, false,  {(DWORD_PTR)true},        DIF_CENTERGROUP, true},  //cfgOk
		{DI_BUTTON,    0, 15,  0,  0, false,  {(DWORD_PTR)false},       DIF_CENTERGROUP, false}, //cfgCancel
		#endif
	};
	SETTEXT(items[cfgTitle], GetMsgT(CEPluginName));
	SETTEXT(items[cfgShowLines], GetMsgT(CEPluginEnable));
	items[cfgShowLines].Selected = gbBackgroundEnabled;

	SETTEXT(items[cfgPanel], GetMsgT(CEColorPanelLabel));
	items[cfgPanel].Selected = gbLinesColorPanel;
	wsprintfT(szColorP, FAR_T("%06X"), (0xFFFFFF & gcrLinesColorPanel));
	SETTEXT(items[cfgColorPanel], szColorP);

	SETTEXT(items[cfgEditor], GetMsgT(CEColorEditorLabel));
	items[cfgEditor].Selected = gbLinesColorEditor;
	wsprintfT(szColorE, FAR_T("%06X"), (0xFFFFFF & gcrLinesColorEditor));
	SETTEXT(items[cfgColorEditor], szColorE);

	SETTEXT(items[cfgViewer], GetMsgT(CEColorViewerLabel));
	items[cfgViewer].Selected = gbLinesColorViewer;
	wsprintfT(szColorV, FAR_T("%06X"), (0xFFFFFF & gcrLinesColorViewer));
	SETTEXT(items[cfgColorViewer], szColorV);

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
	FAR_INT dialog_res = 0;
	// Запомнить текущие значения, чтобы восстановить их если Esc нажат
	BOOL bCurBackgroundEnabled = gbBackgroundEnabled;
	BOOL bLinesColorPanel = gbLinesColorPanel;
	COLORREF crLinesColorPanel = gcrLinesColorPanel;
	BOOL bLinesColorEditor = gbLinesColorEditor;
	COLORREF crLinesColorEditor = gcrLinesColorEditor;
	BOOL bLinesColorViewer = gbLinesColorViewer;
	COLORREF crLinesColorViewer = gcrLinesColorViewer;
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
		#ifdef FAR_UNICODE
			#define GET_COLOR(id,v) \
				psz = GetDataPtr(id); \
				endptr = NULL; \
				v = nVal = wcstoul(psz, &endptr, 16);
		#else
			#define GET_COLOR(id,v) \
				psz = GetDataPtr(id); \
				endptr = NULL; \
				v = nVal = strtoul(psz, &endptr, 16);
		#endif

		const FAR_CHAR* psz;
		FAR_CHAR* endptr;
		BYTE cVal; DWORD nVal;

		gbBackgroundEnabled = cVal = _GetCheck(cfgShowLines);

		gbLinesColorPanel = cVal = _GetCheck(cfgPanel);
		GET_COLOR(cfgColorPanel, gcrLinesColorPanel);
		gbLinesColorEditor = cVal = _GetCheck(cfgEditor);
		GET_COLOR(cfgColorEditor, gcrLinesColorEditor);
		gbLinesColorViewer = cVal = _GetCheck(cfgViewer);
		GET_COLOR(cfgColorViewer, gcrLinesColorViewer);

		gbHilightPlugins = cVal = _GetCheck(cfgHilight);
		GET_COLOR(cfgPlugBack, gcrHilightPlugBack);

		SettingsSave();
	}
	else
	{
		gbBackgroundEnabled = bCurBackgroundEnabled;
		gbLinesColorPanel = bLinesColorPanel;
		gcrLinesColorPanel = crLinesColorPanel;
		gbLinesColorEditor = bLinesColorEditor;
		gcrLinesColorEditor = crLinesColorEditor;
		gbLinesColorViewer = bLinesColorViewer;
		gcrLinesColorViewer = crLinesColorViewer;
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
