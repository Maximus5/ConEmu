
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

#include "../common/ConEmuCheck.h"

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
		#define GetMsgT(aiMsg) InfoT->GetMsg(&guid_ConEmu,aiMsg)
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
		#define GetMsgT(aiMsg) InfoT->GetMsg(InfoT->ModuleNumber,aiMsg)
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
	//#define GetTextPtr(i) ((const char *)InfoA->SendDlgMessage(hDlg,DM_GETTEXTPTR,i,0))
	#define SETTEXT(itm,txt) lstrcpyA(itm.Data, txt)
	#define wsprintfT wsprintfA
	#define GetMsgT(aiMsg) InfoT->GetMsg(InfoT->ModuleNumber,aiMsg)
	#define FAR_T(s) s
	#define FAR_CHAR char
	#define strcpyT lstrcpyA
	#define strlenT lstrlenA
	#define FAR_PTR LONG_PTR
#endif


enum
{
	guiTitle       = 0,
	guiMacroLabel  = 1,
	guiMacroText   = 2,
	guiResultLabel = 3,
	guiResultText  = 4,
	guiOk          = 5,
	guiCancel      = 6,
};

#if FAR_UNICODE>=1867
static INT_PTR WINAPI CallGuiMacroDlg(HANDLE hDlg, FAR_INT Msg, FAR_INT Param1, FAR_PTR Param2)
#else
static LONG_PTR WINAPI CallGuiMacroDlg(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
#endif
{
	if (((Msg == DN_BTNCLICK) && (Param1 == guiOk))
	        //|| ((Msg == DM_CLOSE) && (Param1 != guiCancel))
			#if FAR_UNICODE>=1867
			|| ((Msg == DN_CONTROLINPUT) && (Param1 != guiCancel) 
			    && (((INPUT_RECORD*)Param2)->EventType == KEY_EVENT)
				&& (((INPUT_RECORD*)Param2)->Event.KeyEvent.wVirtualKeyCode == VK_RETURN))
			#else
	        || ((Msg == DN_KEY) && (Param1 != guiCancel) && (Param2 == 0x0D/*KEY_ENTER*/))
			#endif
	  )
	{
		EditorSelect es = {0};
		InfoT->SendDlgMessage(hDlg, DM_GETSELECTION, guiMacroText, (FAR_PTR)&es);
		FAR_CHAR *pszResult = NULL;
		FAR_CHAR *pszBuff = NULL;
		#ifdef FAR_UNICODE
		const FAR_CHAR *pszMacro = GetTextPtr(guiMacroText);
		#else
		LONG_PTR nMacroLen = InfoA->SendDlgMessage(hDlg,DM_GETTEXTPTR,guiMacroText,0);
		FAR_CHAR *pszMacro = (FAR_CHAR*)calloc(nMacroLen+1,sizeof(FAR_CHAR));
		InfoA->SendDlgMessage(hDlg,DM_GETTEXTPTR,guiMacroText,(FAR_PTR)pszMacro);
		#endif
		CESERVER_REQ *pIn = NULL, *pOut = NULL;

		if (!ghConEmuWndDC)
		{
			SetEnvironmentVariable(CEGUIMACRORETENVVAR, NULL);
		}
		else if (pszMacro && *pszMacro)
		{
			int nLen = strlenT(pszMacro);
			pIn = ExecuteNewCmd(CECMD_GUIMACRO, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_GUIMACRO)+nLen*sizeof(wchar_t));
#ifdef FAR_UNICODE
			lstrcpyW(pIn->GuiMacro.sMacro, pszMacro);
#else
			MultiByteToWideChar(CP_OEMCP, 0, pszMacro, nLen+1, pIn->GuiMacro.sMacro, nLen+1);
#endif
			pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);

			if (pOut && pOut->GuiMacro.nSucceeded)
			{
#ifdef FAR_UNICODE
				pszResult = pOut->GuiMacro.sMacro;
#else
				int nLen = lstrlenW(pOut->GuiMacro.sMacro)+1;
				pszBuff = (FAR_CHAR*)malloc(nLen);
				WideCharToMultiByte(CP_OEMCP, 0, pOut->GuiMacro.sMacro, nLen, pszBuff, nLen, 0,0);
				pszResult = pszBuff;
#endif
			}

			ExecuteFreeResult(pIn);
		}

		if (pszResult)
			InfoT->SendDlgMessage(hDlg, DM_ADDHISTORY, guiMacroText, (FAR_PTR)pszMacro);

#ifndef FAR_UNICODE
		free(pszMacro);
#endif

		InfoT->SendDlgMessage(hDlg, DM_SETTEXTPTR, guiResultText, (FAR_PTR)(pszResult ? pszResult : FAR_T("")));
		InfoT->SendDlgMessage(hDlg, DM_SETFOCUS, guiMacroText, 0);
		InfoT->SendDlgMessage(hDlg, DM_SETSELECTION, guiMacroText, (FAR_PTR)&es);

		if (pOut)
			ExecuteFreeResult(pOut);

		if (pszBuff)
			free(pszBuff);

		return TRUE;
	}

	return InfoT->DefDlgProc(hDlg, Msg, Param1, Param2);
}

static void CallGuiMacroProc()
{
	if (!InfoT)
		return;

	int height = 10;
	FarDialogItem items[] =
	{
		{DI_DOUBLEBOX, 3,  1,  72, height - 2}, //guiTitle

		{DI_TEXT,      5,  2,  0,  0},   //guiMacroLabel, guiMacroText
		#if FAR_UNICODE>=1867
		{DI_EDIT,      5,  3,  70, 0, {0}, FAR_T("ConEmuGuiMacro"), NULL, DIF_HISTORY|DIF_FOCUS},
		#else
		{DI_EDIT,      5,  3,  70, 0, true, {(DWORD_PTR)FAR_T("ConEmuGuiMacro")}, DIF_HISTORY},
		#endif

		{DI_TEXT,      5,  4,  0,  0},   //guiResultLabel, guiResultText
		{DI_EDIT,      5,  5,  70, 0},

		#if FAR_UNICODE>=1867
		{DI_BUTTON,    0,  7,  0,  0, {0},  0,0,       DIF_CENTERGROUP|DIF_DEFAULTBUTTON},  //guiOk
		{DI_BUTTON,    0,  7,  0,  0, {0},  0,0,       DIF_CENTERGROUP}, //guiCancel
		#else
		{DI_BUTTON,    0,  7,  0,  0, false,  {(DWORD_PTR)false},       DIF_CENTERGROUP, true},  //guiOk
		{DI_BUTTON,    0,  7,  0,  0, false,  {(DWORD_PTR)false},       DIF_CENTERGROUP, false}, //guiCancel
		#endif
	};
	SETTEXT(items[guiTitle], GetMsgT(CEGuiMacroTitle));
	SETTEXT(items[guiMacroLabel], GetMsgT(CEGuiMacroCommand));
	//SETTEXT(items[guiMacroText], szMacro);
	SETTEXT(items[guiResultLabel], GetMsgT(CEGuiMacroResult));
	//SETTEXT(items[guiResultText], szResult);
	SETTEXT(items[guiOk], GetMsgT(CEGuiMacroExecute));
	SETTEXT(items[guiCancel], GetMsgT(CEGuiMacroCancel));
	FAR_INT dialog_res = 0;
#ifdef FAR_UNICODE
	HANDLE hDlg = NULL;
	#if FAR_UNICODE>=1867
		hDlg = InfoT->DialogInit(&guid_ConEmu, &guid_ConEmuGuiMacroDlg,
								 -1, -1, 76, height,
								 NULL/*L"Configure"*/, items, countof(items), 
								 0, 0/*Flags*/, CallGuiMacroDlg, 0);
	#else
		hDlg = InfoT->DialogInit(InfoT->ModuleNumber, -1, -1, 76, height,
								 NULL/*L"Configure"*/, items, countof(items), 0, 0/*Flags*/, CallGuiMacroDlg, 0/*DlgProcParam*/);
	#endif
	dialog_res = InfoT->DialogRun(hDlg);
	InfoT->DialogFree(hDlg);
#else
	dialog_res = InfoT->DialogEx(InfoT->ModuleNumber,-1,-1,76,height,
	                             NULL/*"Configure"*/, items, countof(items), 0, 0, CallGuiMacroDlg, NULL);
#endif
}
