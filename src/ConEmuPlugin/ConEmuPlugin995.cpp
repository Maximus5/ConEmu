
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
#include "..\common\common.hpp"
#include "..\common\pluginW1007.hpp" // Отличается от 995 наличием SynchoApi
#include "PluginHeader.h"

#ifdef _DEBUG
//#define SHOW_DEBUG_EVENTS
#endif

struct PluginStartupInfo *InfoW995=NULL;
struct FarStandardFunctions *FSFW995=NULL;

void WaitEndSynchro995();


void ProcessDragFrom995()
{
	if (!InfoW995 || !InfoW995->AdvControl)
		return;

	WindowInfo WInfo;				
    WInfo.Pos=0;
	InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);
	if (!WInfo.Current)
	{
		int ItemsCount=0;
		//WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);				
		OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}

	PanelInfo PInfo;
	WCHAR *szCurDir=gszDir1; szCurDir[0]=0; //(WCHAR*)calloc(0x400,sizeof(WCHAR));
	InfoW995->Control(PANEL_ACTIVE, FCTL_GETPANELINFO, NULL, (LONG_PTR)&PInfo);
	if ((PInfo.PanelType == PTYPE_FILEPANEL || PInfo.PanelType == PTYPE_TREEPANEL) && PInfo.Visible)
	{
		InfoW995->Control(PANEL_ACTIVE, FCTL_GETCURRENTDIRECTORY, 0x400, (LONG_PTR)szCurDir);
		int nDirLen=0, nDirNoSlash=0;
		if (szCurDir[0])
		{
			nDirLen=lstrlen(szCurDir);
			if (nDirLen>0)
				if (szCurDir[nDirLen-1]!=L'\\')
					nDirNoSlash=1;
		}

		// Это только предполагаемый размер, при необходимости он будет увеличен
		OutDataAlloc(sizeof(int)+PInfo.SelectedItemsNumber*((MAX_PATH+2)+sizeof(int)));

		//Maximus5 - новый формат передачи
		int nNull=0; // ItemsCount
		//WriteFile(hPipe, &nNull, sizeof(int), &cout, NULL);
		OutDataWrite(&nNull/*ItemsCount*/, sizeof(int));
		
		if (PInfo.SelectedItemsNumber<=0) {
			//if (nDirLen > 3 && szCurDir[1] == L':' && szCurDir[2] == L'\\')
			// Проверка того, что мы стоим на ".."
			if (PInfo.CurrentItem == 0 && PInfo.ItemsNumber > 0)
			{
				if (!nDirNoSlash)
					szCurDir[nDirLen-1] = 0;
				else
					nDirLen++;

				int nWholeLen = nDirLen + 1;
				OutDataWrite(&nWholeLen, sizeof(int));
				OutDataWrite(&nDirLen, sizeof(int));
				OutDataWrite(szCurDir, sizeof(WCHAR)*nDirLen);
			}
			// Fin
			OutDataWrite(&nNull/*ItemsCount*/, sizeof(int));
		} else {
			PluginPanelItem **pi = (PluginPanelItem**)calloc(PInfo.SelectedItemsNumber, sizeof(PluginPanelItem*));
			bool *bIsFull = (bool*)calloc(PInfo.SelectedItemsNumber, sizeof(bool));
			int ItemsCount=PInfo.SelectedItemsNumber, i;

			int nMaxLen=MAX_PATH+1, nWholeLen=1;

			// сначала посчитать максимальную длину буфера
			for (i=0;i<ItemsCount;i++)
			{
				size_t sz = InfoW995->Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, NULL);
				if (!sz)
					continue;
				pi[i] = (PluginPanelItem*)calloc(sz, 1); // размер возвращается в байтах
				if (!InfoW995->Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, (LONG_PTR)(pi[i]))) {
					free(pi[i]); pi[i] = NULL;
					continue;
				}
				if (i == 0 
					&& ((pi[i]->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
					&& !lstrcmpW(pi[i]->FindData.lpwszFileName, L".."))
				{
					free(pi[i]); pi[i] = NULL;
					continue;
				}

				int nLen=nDirLen+nDirNoSlash;
				if ((pi[i]->FindData.lpwszFileName[0] == L'\\' && pi[i]->FindData.lpwszFileName[1] == L'\\') ||
				    (ISALPHA(pi[i]->FindData.lpwszFileName[0]) && pi[i]->FindData.lpwszFileName[1] == L':' && pi[i]->FindData.lpwszFileName[2] == L'\\'))
				    { nLen = 0; bIsFull[i] = TRUE; } // это уже полный путь!
				nLen += lstrlenW(pi[i]->FindData.lpwszFileName);
				if (nLen>nMaxLen)
					nMaxLen = nLen;
				nWholeLen += (nLen+1);
			}
			//WriteFile(hPipe, &nWholeLen, sizeof(int), &cout, NULL);
			OutDataWrite(&nWholeLen, sizeof(int));

			WCHAR* Path=new WCHAR[nMaxLen+1];

			for (i=0;i<ItemsCount;i++)
			{
				//WCHAR Path[MAX_PATH+1];
				//ZeroMemory(Path, MAX_PATH+1);
				//Maximus5 - засада с корнем диска и возможностью overflow
				//wsprintf(Path, L"%s\\%s", szCurDir, PInfo.SelectedItems[i]->FindData.lpwszFileName);
				Path[0]=0;

				if (!pi[i] || !pi[i]->FindData.lpwszFileName) continue; //этот элемент получить не удалось

				int nLen=0;
				if (nDirLen>0 && !bIsFull[i]) {
					lstrcpy(Path, szCurDir);
					if (nDirNoSlash) {
						Path[nDirLen]=L'\\';
						Path[nDirLen+1]=0;
					}
					nLen = nDirLen+nDirNoSlash;
				}
				lstrcpy(Path+nLen, pi[i]->FindData.lpwszFileName);
				nLen += lstrlen(pi[i]->FindData.lpwszFileName);

				nLen++;
				//WriteFile(hPipe, &nLen, sizeof(int), &cout, NULL);
				OutDataWrite(&nLen, sizeof(int));
				//WriteFile(hPipe, Path, sizeof(WCHAR)*nLen, &cout, NULL);
				OutDataWrite(Path, sizeof(WCHAR)*nLen);
			}

			for (i=0;i<ItemsCount;i++)
			{
				if (pi[i]) free(pi[i]);
			}
			free ( pi ); pi = NULL;

			free(bIsFull);
			delete [] Path; Path=NULL;

			// Конец списка
			//WriteFile(hPipe, &nNull, sizeof(int), &cout, NULL);
			OutDataWrite(&nNull, sizeof(int));
		}
	}
	else
	{
		int ItemsCount=0;
		OutDataWrite(&ItemsCount, sizeof(int));
		OutDataWrite(&ItemsCount, sizeof(int)); // смена формата
	}
	//free(szCurDir);
}

void ProcessDragTo995()
{
	if (!InfoW995 || !InfoW995->AdvControl)
		return;

	WindowInfo WInfo;				
    WInfo.Pos=0;
	InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);
	if (!WInfo.Current)
	{
		//InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_FREEWINDOWINFO, (void*)&WInfo);
		int ItemsCount=0;
		//WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);				
		if (gpCmdRet==NULL)
			OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}
	//InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_FREEWINDOWINFO, (void*)&WInfo);
	
	PanelInfo PAInfo, PPInfo;
	ForwardedPanelInfo *pfpi=NULL;
	int nStructSize = sizeof(ForwardedPanelInfo)+4; // потом увеличим на длину строк
	//ZeroMemory(&fpi, sizeof(fpi));
	BOOL lbAOK=FALSE, lbPOK=FALSE;
	WCHAR *szPDir=gszDir1; szPDir[0]=0; //(WCHAR*)calloc(0x400,sizeof(WCHAR));
	WCHAR *szADir=gszDir2; szADir[0]=0; //(WCHAR*)calloc(0x400,sizeof(WCHAR));
	
	//if (!(lbAOK=InfoW995->Control(PANEL_ACTIVE, FCTL_GETPANELSHORTINFO, &PAInfo)))
	lbAOK=InfoW995->Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&PAInfo) &&
		  InfoW995->Control(PANEL_ACTIVE, FCTL_GETCURRENTDIRECTORY, 0x400, (LONG_PTR)szADir);
	if (lbAOK && szADir)
		nStructSize += (lstrlen(szADir))*sizeof(WCHAR);

	lbPOK=InfoW995->Control(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&PPInfo) &&
		  InfoW995->Control(PANEL_PASSIVE, FCTL_GETCURRENTDIRECTORY, 0x400, (LONG_PTR)szPDir);
	if (lbPOK && szPDir)
		nStructSize += (lstrlen(szPDir))*sizeof(WCHAR); // Именно WCHAR! не TCHAR

	pfpi = (ForwardedPanelInfo*)calloc(nStructSize,1);
	if (!pfpi) {
		int ItemsCount=0;
		//WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);				
		if (gpCmdRet==NULL)
			OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}


	pfpi->ActivePathShift = sizeof(ForwardedPanelInfo);
	pfpi->pszActivePath = (WCHAR*)(((char*)pfpi)+pfpi->ActivePathShift);

	pfpi->PassivePathShift = pfpi->ActivePathShift+2; // если ActivePath заполнится - увеличим

	if (lbAOK)
	{
		pfpi->ActiveRect=PAInfo.PanelRect;
		if (!PAInfo.Plugin && (PAInfo.PanelType == PTYPE_FILEPANEL || PAInfo.PanelType == PTYPE_TREEPANEL) && PAInfo.Visible)
		{
			if (szADir[0]) {
				lstrcpyW(pfpi->pszActivePath, szADir);
				pfpi->PassivePathShift += lstrlenW(pfpi->pszActivePath)*2;
			}
		}
	}

	pfpi->pszPassivePath = (WCHAR*)(((char*)pfpi)+pfpi->PassivePathShift);
	if (lbPOK)
	{
		pfpi->PassiveRect=PPInfo.PanelRect;
		if (!PPInfo.Plugin && (PPInfo.PanelType == PTYPE_FILEPANEL || PPInfo.PanelType == PTYPE_TREEPANEL) && PPInfo.Visible)
		{
			if (szPDir[0])
				lstrcpyW(pfpi->pszPassivePath, szPDir);
		}
	}

	// Собственно, пересылка информации
	//WriteFile(hPipe, &nStructSize, sizeof(nStructSize), &cout, NULL);
	//WriteFile(hPipe, pfpi, nStructSize, &cout, NULL);
	if (gpCmdRet==NULL)
			OutDataAlloc(nStructSize+4);
	OutDataWrite(&nStructSize, sizeof(nStructSize));
	OutDataWrite(pfpi, nStructSize);

	free(pfpi); pfpi=NULL;
}

void SetStartupInfoW995(void *aInfo)
{
	::InfoW995 = (PluginStartupInfo*)calloc(sizeof(PluginStartupInfo),1);
	::FSFW995 = (FarStandardFunctions*)calloc(sizeof(FarStandardFunctions),1);
	if (::InfoW995 == NULL || ::FSFW995 == NULL)
		return;
	*::InfoW995 = *((struct PluginStartupInfo*)aInfo);
	*::FSFW995 = *((struct PluginStartupInfo*)aInfo)->FSF;
	::InfoW995->FSF = ::FSFW995;

	lstrcpynW(gszRootKey, InfoW995->RootKey, MAX_PATH);
	WCHAR* pszSlash = gszRootKey+lstrlenW(gszRootKey)-1;
	if (*pszSlash == L'\\') *(pszSlash--) = 0;
	while (pszSlash>gszRootKey && *pszSlash!=L'\\') pszSlash--;
	*pszSlash = 0;

	
	/*if (!FarHwnd)
		InitHWND((HWND)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETFARHWND, 0));*/
}

DWORD GetEditorModifiedState995()
{
	EditorInfo ei;
	InfoW995->EditorControl(ECTL_GETINFO, &ei);

	#ifdef SHOW_DEBUG_EVENTS
		char szDbg[255];
		wsprintfA(szDbg, "Editor:State=%i\n", ei.CurState);
		OutputDebugStringA(szDbg);
	#endif

	// Если он сохранен, то уже НЕ модифицирован
	DWORD currentModifiedState = ((ei.CurState & (ECSTATE_MODIFIED|ECSTATE_SAVED)) == ECSTATE_MODIFIED) ? 1 : 0;

	return currentModifiedState;
}

//extern int lastModifiedStateW;

// watch non-modified -> modified editor status change
int ProcessEditorInputW995(LPCVOID aRec)
{
	if (!InfoW995)
		return 0;

	const INPUT_RECORD *Rec = (const INPUT_RECORD*)aRec;
	// only key events with virtual codes > 0 are likely to cause status change (?)
	if (!gbRequestUpdateTabs && (Rec->EventType & 0xFF) == KEY_EVENT 
		&& (Rec->Event.KeyEvent.wVirtualKeyCode || Rec->Event.KeyEvent.wVirtualScanCode || Rec->Event.KeyEvent.uChar.UnicodeChar)
		&& Rec->Event.KeyEvent.bKeyDown)
	{
		//if (!gbRequestUpdateTabs)
		gbNeedPostEditCheck = TRUE;
	}

	return 0;
}


bool UpdateConEmuTabsW995(int anEvent, bool losingFocus, bool editorSave, void* Param/*=NULL*/)
{
	if (!InfoW995 || !InfoW995->AdvControl || gbIgnoreUpdateTabs)
		return false;

    BOOL lbCh = FALSE;
	WindowInfo WInfo = {0};
	wchar_t szWNameBuffer[CONEMUTABMAX];
	//WInfo.Name = szWNameBuffer;
	//WInfo.NameSize = CONEMUTABMAX;

	int windowCount = (int)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETWINDOWCOUNT, NULL);
	lbCh = (lastWindowCount != windowCount);
	
	if (!CreateTabs ( windowCount ))
		return false;

	//EditorInfo ei = {0};
	//if (editorSave)
	//{
	//	InfoW995->EditorControl(ECTL_GETINFO, &ei);
	//}

	ViewerInfo vi = {sizeof(ViewerInfo)};
	if (anEvent == 206)
	{
		if (Param)
			vi.ViewerID = *(int*)Param;
		InfoW995->ViewerControl(VCTL_GETINFO, &vi);
	}

	int tabCount = 0;
	BOOL lbActiveFound = FALSE;
	for (int i = 0; i < windowCount; i++)
	{
		WInfo.Pos = i;

		InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);

		if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
		{
			WInfo.Pos = i;
			WInfo.Name = szWNameBuffer;
			WInfo.NameSize = CONEMUTABMAX;

			InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);

			WARNING("Для получения имени нужно пользовать ECTL_GETFILENAME");
			if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
			{
				if (WInfo.Current) lbActiveFound = TRUE;
				TODO("Определение ИД редактора/вьювера");
				lbCh |= AddTab(tabCount, losingFocus, editorSave, 
					WInfo.Type, WInfo.Name, /*editorSave ? ei.FileName :*/ NULL, 
					WInfo.Current, WInfo.Modified, 0);
				//if (WInfo.Type == WTYPE_EDITOR && WInfo.Current) //2009-08-17
				//	lastModifiedStateW = WInfo.Modified;
			}
			//InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_FREEWINDOWINFO, (void*)&WInfo);
		}
	}

	// Viewer в FAR 2 build 9xx не попадает в список окон при событии VE_GOTFOCUS
	if (!losingFocus && !editorSave && tabCount == 0 && anEvent == 206)
	{
		lbActiveFound = TRUE;
		lbCh |= AddTab(tabCount, losingFocus, editorSave, 
			WTYPE_VIEWER, vi.FileName, NULL, 
			1, 0, vi.ViewerID);
	}


	// Скорее всего это модальный редактор (или вьювер?)
	if (!lbActiveFound && !losingFocus)
	{
		WInfo.Pos = -1;

		if (InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo))
		{
			if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER)
			{
				WInfo.Pos = -1;
				WInfo.Name = szWNameBuffer;
				WInfo.NameSize = CONEMUTABMAX;

				InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);

				if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER)
				{
					tabCount = 0;
					TODO("Определение ИД Редактора/вьювера");
					lbCh |= AddTab(tabCount, losingFocus, editorSave, 
						WInfo.Type, WInfo.Name, /*editorSave ? ei.FileName :*/ NULL, 
						WInfo.Current, WInfo.Modified, 0);
				}
			}
			else
			{
				gpTabs->Tabs.CurrentType = WInfo.Type;
			}
		}

		//wchar_t* pszEditorFileName = NULL;
		//EditorInfo ei = {0};
		//ViewerInfo vi = {sizeof(ViewerInfo)};
		//BOOL bEditor = FALSE, bViewer = FALSE;

		//bViewer = InfoW995->ViewerControl(VCTL_GETINFO, &vi);

		//if (InfoW995->EditorControl(ECTL_GETINFO, &ei)) {
		//	int nLen = InfoW995->EditorControl(ECTL_GETFILENAME, NULL);
		//	if (nLen > 0) {
		//		wchar_t* pszEditorFileName = (wchar_t*)calloc(nLen+1,2);
		//		if (pszEditorFileName) {
		//			if (InfoW995->EditorControl(ECTL_GETFILENAME, pszEditorFileName)) {
		//				bEditor = true;
		//			}
		//		}
		//	}
		//}

		//if (bEditor && bViewer) {
		//	// Попробуем получить информацию об активном окне, но это может привести к блокировке некоторых диалогов ФАР2?
		//	WInfo.Pos = -1;
		//	InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);
		//}

		//if (bEditor) {
		//	tabCount = 0;
		//	lbCh |= AddTab(tabCount, losingFocus, editorSave, 
		//		WTYPE_EDITOR, pszEditorFileName, NULL, 
		//		1, (ei.CurState & (ECSTATE_MODIFIED|ECSTATE_SAVED)) == ECSTATE_MODIFIED);
		//} else if (bViewer) {
		//	tabCount = 0;
		//	lbCh |= AddTab(tabCount, losingFocus, editorSave, 
		//		WTYPE_VIEWER, vi.FileName, NULL, 
		//		1, 0);
		//}

		//if (pszEditorFileName) free(pszEditorFileName);
	}
	
	//// 2009-08-17
	//if (gbHandleOneRedraw && gbHandleOneRedrawCh && lbCh) {
	//	gbHandleOneRedraw = false;
	//	gbHandleOneRedrawCh = false;
	//}
	

#ifdef _DEBUG
	//WCHAR szDbg[128]; wsprintfW(szDbg, L"Event: %i, count %i\n", anEvent, tabCount);
	//OutputDebugStringW(szDbg);
#endif

	SendTabs(tabCount, lbCh && (gnReqCommand==(DWORD)-1));

	return (lbCh != FALSE);
}

void ExitFARW995(void)
{
	WaitEndSynchro995();

	if (InfoW995)
	{
		free(InfoW995);
		InfoW995=NULL;
	}
	if (FSFW995)
	{
		free(FSFW995);
		FSFW995=NULL;
	}
}

int ShowMessage995(int aiMsg, int aiButtons)
{
	if (!InfoW995 || !InfoW995->Message || !InfoW995->GetMsg)
		return -1;
	return InfoW995->Message(InfoW995->ModuleNumber, FMSG_ALLINONE|FMSG_MB_OK|FMSG_WARNING, NULL, 
		(const wchar_t * const *)InfoW995->GetMsg(InfoW995->ModuleNumber,aiMsg), 0, aiButtons);
}

LPCWSTR GetMsg995(int aiMsg)
{
	if (!InfoW995 || !InfoW995->GetMsg)
		return L"";
	return InfoW995->GetMsg(InfoW995->ModuleNumber,aiMsg);
}

//void ReloadMacro995()
//{
//	if (!InfoW995 || !InfoW995->AdvControl)
//		return;
//
//	ActlKeyMacro command;
//	command.Command=MCMD_LOADALL;
//	InfoW995->AdvControl(InfoW995->ModuleNumber,ACTL_KEYMACRO,&command);
//}

void SetWindow995(int nTab)
{
	if (!InfoW995 || !InfoW995->AdvControl)
		return;

	if (InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_SETCURRENTWINDOW, (void*)nTab))
		InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_COMMIT, 0);
}

// Warning, напрямую НЕ вызывать. Пользоваться "общей" PostMacro
void PostMacro995(wchar_t* asMacro)
{
	if (!InfoW995 || !InfoW995->AdvControl)
		return;

	ActlKeyMacro mcr;
	mcr.Command = MCMD_POSTMACROSTRING;
	mcr.Param.PlainText.Flags = 0; // По умолчанию - вывод на экран разрешен
	if (*asMacro == L'@' && asMacro[1] && asMacro[1] != L' ') {
		mcr.Param.PlainText.Flags |= KSFLAGS_DISABLEOUTPUT;
		asMacro ++;
	}
	mcr.Param.PlainText.SequenceText = asMacro;
	InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_KEYMACRO, (void*)&mcr);

	//FAR BUGBUG: Макрос не запускается на исполнение, пока мышкой не дернем :(
	//  Это чаще всего проявляется при вызове меню по RClick
	//  Если курсор на другой панели, то RClick сразу по пассивной
	//  не вызывает отрисовку :(
	
	// Перенесено в "общую" PostMacro

	////if (!mcr.Param.PlainText.Flags) {
	//INPUT_RECORD ir[2] = {{MOUSE_EVENT},{MOUSE_EVENT}};
	//if (isPressed(VK_CAPITAL))
	//	ir[0].Event.MouseEvent.dwControlKeyState |= CAPSLOCK_ON;
	//if (isPressed(VK_NUMLOCK))
	//	ir[0].Event.MouseEvent.dwControlKeyState |= NUMLOCK_ON;
	//if (isPressed(VK_SCROLL))
	//	ir[0].Event.MouseEvent.dwControlKeyState |= SCROLLLOCK_ON;
	//ir[0].Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
	//ir[1].Event.MouseEvent.dwControlKeyState = ir[0].Event.MouseEvent.dwControlKeyState;
	//ir[1].Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
	//ir[1].Event.MouseEvent.dwMousePosition.X = 1;
	//ir[1].Event.MouseEvent.dwMousePosition.Y = 1;
	//
	////2010-01-29 попробуем STD_OUTPUT
	////if (!ghConIn) {
	////	ghConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
	////		0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	////	if (ghConIn == INVALID_HANDLE_VALUE) {
	////		#ifdef _DEBUG
	////		DWORD dwErr = GetLastError();
	////		_ASSERTE(ghConIn!=INVALID_HANDLE_VALUE);
	////		#endif
	////		ghConIn = NULL;
	////		return;
	////	}
	////}
	//HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	//DWORD cbWritten = 0;
	//#ifdef _DEBUG
	//BOOL fSuccess = 
	//#endif
	//WriteConsoleInput(hIn/*ghConIn*/, ir, 1, &cbWritten);
	//_ASSERTE(fSuccess && cbWritten==1);
	////}
	////InfoW995->AdvControl(InfoW995->ModuleNumber,ACTL_REDRAWALL,NULL);
}

int ShowPluginMenu995()
{
	if (!InfoW995)
		return -1;

	FarMenuItemEx items[] = {
		{ConEmuHwnd ? MIF_SELECTED : MIF_DISABLE,  InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuEditOutput)},
		{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuViewOutput)},
		{MIF_SEPARATOR},
		{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuShowHideTabs)},
		{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuNextTab)},
		{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuPrevTab)},
		{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuCommitTab)},
		{MIF_SEPARATOR},
		{ConEmuHwnd||IsTerminalMode() ? MIF_DISABLE : MIF_SELECTED,  InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuAttach)},
		{MIF_SEPARATOR},
		//#ifdef _DEBUG
		//{0, L"&~. Raise exception"},
		//#endif
		{IsDebuggerPresent()||IsTerminalMode() ? MIF_DISABLE : 0,    InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuDebug)},
	};
	int nCount = sizeof(items)/sizeof(items[0]);

	int nRc = InfoW995->Menu(InfoW995->ModuleNumber, -1,-1, 0, 
		FMENU_USEEXT|FMENU_AUTOHIGHLIGHT|FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
		InfoW995->GetMsg(InfoW995->ModuleNumber,CEPluginName),
		NULL, NULL, NULL, NULL, (FarMenuItem*)items, nCount);
		
	//#ifdef _DEBUG
	//if (nRc == (nCount - 2))
	//{
	//	// Вызвать исключение для проверки отладчика
	//	LPVOID ptrSrc;
	//	wchar_t szDst[MAX_PATH];
	//	ptrSrc = NULL;
	//	memmove(szDst, ptrSrc, sizeof(szDst));
	//}
	//#endif

	return nRc;
}

BOOL EditOutput995(LPCWSTR asFileName, BOOL abView)
{
	if (!InfoW995)
		return FALSE;

	BOOL lbRc = FALSE;
	if (!abView) {
		int iRc =
			InfoW995->Editor(asFileName, InfoW995->GetMsg(InfoW995->ModuleNumber,CEConsoleOutput), 0,0,-1,-1, 
			EF_NONMODAL|EF_IMMEDIATERETURN|EF_DELETEONLYFILEONCLOSE|EF_ENABLE_F6|EF_DISABLEHISTORY,
			0, 1, 1200);
		lbRc = (iRc != EEC_OPEN_ERROR);
	} else {
		#ifdef _DEBUG
		int iRc =
		#endif
			InfoW995->Viewer(asFileName, InfoW995->GetMsg(InfoW995->ModuleNumber,CEConsoleOutput), 0,0,-1,-1, 
			VF_NONMODAL|VF_IMMEDIATERETURN|VF_DELETEONLYFILEONCLOSE|VF_ENABLE_F6|VF_DISABLEHISTORY,
			1200);
		lbRc = TRUE;
	}

	return lbRc;
}

BOOL ExecuteSynchro995()
{
	if (!InfoW995)
		return FALSE;

	if (IS_SYNCHRO_ALLOWED)
	{
		if (gbSynchroProhibited)
		{
			_ASSERT(gbSynchroProhibited==false);
			return FALSE;
		}

		// получается более 2-х, если фар в данный момент чем-то занят (сканирует каталог?)
		//_ASSERTE(gnSynchroCount<=3);

		gnSynchroCount++;
		InfoW995->AdvControl ( InfoW995->ModuleNumber, ACTL_SYNCHRO, NULL);
		return TRUE;
	}
	
	return FALSE;
}

static HANDLE ghSyncDlg = NULL;

void WaitEndSynchro995()
{
	if ((gnSynchroCount == 0) || !(IS_SYNCHRO_ALLOWED))
		return;

	FarDialogItem items[] = 
	{
        {DI_DOUBLEBOX,  3,  1,  51, 3, false, 0, 0, false, GetMsg995(CEPluginName)},

        {DI_BUTTON,     0,  2,  0,  0, true,  0, DIF_CENTERGROUP, true, GetMsg995(CEStopSynchroWaiting)},
	};

	ghSyncDlg = InfoW995->DialogInit(InfoW995->ModuleNumber, -1,-1, 55, 5, NULL, items, countof(items), 0, 0, NULL, 0);
	if (ghSyncDlg == INVALID_HANDLE_VALUE)
	{
		// Видимо, Фар в состоянии выхода (финальная выгрузка всех плагинов)
		// В этом случае, по идее, Synchro вызываться более не должно
		gnSynchroCount = 0; // так что просто сбросим счетчик
	}
	else
	{
		InfoW995->DialogRun(ghSyncDlg);
		InfoW995->DialogFree(ghSyncDlg);
	}
	ghSyncDlg = NULL;
}

void StopWaitEndSynchro995()
{
	if (ghSyncDlg)
	{
		InfoW995->SendDlgMessage(ghSyncDlg, DM_CLOSE, -1, 0);
	}
}


// Param должен быть выделен в куче. Память освобождается в ProcessSynchroEventW.
//BOOL CallSynchro995(SynchroArg *Param, DWORD nTimeout /*= 10000*/)
//{
//	if (!InfoW995 || !Param)
//		return FALSE;
//
//	if (gFarVersion.dwVerMajor>1 && (gFarVersion.dwVerMinor>0 || gFarVersion.dwBuild>=1006)) {
//		// Функция всегда возвращает 0
//		if (Param->hEvent)
//			ResetEvent(Param->hEvent);
//
//		//Param->Processed = FALSE;
//
//		InfoW995->AdvControl ( InfoW995->ModuleNumber, ACTL_SYNCHRO, Param);
//
//		HANDLE hEvents[2] = {ghServerTerminateEvent, Param->hEvent};
//		int nCount = Param->hEvent ? 2 : 1;
//		_ASSERTE(Param->hEvent != NULL);
//
//		DWORD nWait = 100;
//		nWait = WaitForMultipleObjects(nCount, hEvents, FALSE, nTimeout);
//		if (nWait != WAIT_OBJECT_0 && nWait != (WAIT_OBJECT_0+1)) {
//			_ASSERTE(nWait==WAIT_OBJECT_0);
//			if (nWait == (WAIT_OBJECT_0+1)) {
//				// Таймаут, эту команду плагин должен пропустить, когда фар таки соберется ее выполнить
//				Param->Obsolete = TRUE;
//			}
//		}
//
//		return (nWait == 0);
//	}
//
//	return FALSE;
//}

BOOL IsMacroActive995()
{
	if (!InfoW995) return FALSE;

	ActlKeyMacro akm = {MCMD_GETSTATE};
	INT_PTR liRc = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_KEYMACRO, &akm);
	if (liRc == MACROSTATE_NOMACRO)
		return FALSE;
	return TRUE;
}


void RedrawAll995()
{
	if (!InfoW995) return;
	InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_REDRAWALL, NULL);
}

bool LoadPlugin995(wchar_t* pszPluginPath)
{
	if (!InfoW995) return false;
	InfoW995->PluginsControl(INVALID_HANDLE_VALUE,PCTL_LOADPLUGIN,PLT_PATH,(LONG_PTR)pszPluginPath);
	return true;
}

bool RunExternalProgramW(wchar_t* pszCommand, wchar_t* pszCurDir);

bool RunExternalProgram995(wchar_t* pszCommand)
{
	wchar_t strTemp[MAX_PATH+1];
	
	if (!pszCommand || !*pszCommand)
	{
		lstrcpy(strTemp, L"cmd");
		if (!InfoW995->InputBox(L"ConEmu", L"Start console program", L"ConEmu.CreateProcess", 
			strTemp, strTemp, MAX_PATH, NULL, FIB_BUTTONS))
			return false;
		pszCommand = strTemp;
	}

	wchar_t *pszCurDir = NULL;
	size_t len;
	if ((len = InfoW995->Control(INVALID_HANDLE_VALUE, FCTL_GETCURRENTDIRECTORY, 0, 0)) != 0) {
		if ((pszCurDir = (wchar_t*)malloc(len*2)) != NULL) {
			if (!InfoW995->Control(INVALID_HANDLE_VALUE, FCTL_GETCURRENTDIRECTORY, (int)len, (LONG_PTR)pszCurDir)) {
				free(pszCurDir); pszCurDir = NULL;
			}
		}
	}
	if (!pszCurDir) {
		pszCurDir = (wchar_t*)malloc(10);
		lstrcpy(pszCurDir, L"C:\\");
	}

	InfoW995->Control(INVALID_HANDLE_VALUE,FCTL_GETUSERSCREEN,0,0);

	RunExternalProgramW(pszCommand, pszCurDir);

	InfoW995->Control(INVALID_HANDLE_VALUE,FCTL_SETUSERSCREEN,0,0);
	InfoW995->AdvControl(InfoW995->ModuleNumber,ACTL_REDRAWALL,0);

	free(pszCurDir); pszCurDir = NULL;
	return TRUE;
}


bool ProcessCommandLine995(wchar_t* pszCommand)
{
	if (!InfoW995 || !FSFW995) return false;

	if (FSFW995->LStrnicmp(pszCommand, L"run:", 4)==0) {
		RunExternalProgram995(pszCommand+4);
		return true;
	}

	return false;
}

static void FarPanel2CePanel(PanelInfo* pFar, CEFAR_SHORT_PANEL_INFO* pCE)
{
	pCE->PanelType = pFar->PanelType;
	pCE->Plugin = pFar->Plugin;
	pCE->PanelRect = pFar->PanelRect;
	pCE->ItemsNumber = pFar->ItemsNumber;
	pCE->SelectedItemsNumber = pFar->SelectedItemsNumber;
	pCE->CurrentItem = pFar->CurrentItem;
	pCE->TopPanelItem = pFar->TopPanelItem;
	pCE->Visible = pFar->Visible;
	pCE->Focus = pFar->Focus;
	pCE->ViewMode = pFar->ViewMode;
	pCE->ShortNames = pFar->ShortNames;
	pCE->SortMode = pFar->SortMode;
	pCE->Flags = pFar->Flags;
}

BOOL ReloadFarInfo995(BOOL abFull)
{
	if (!InfoW995 || !FSFW995) return FALSE;
	
	if (!gpFarInfo) {
		_ASSERTE(gpFarInfo!=NULL);
		return FALSE;
	}

	// Заполнить gpFarInfo->
	//BYTE nFarColors[0x100]; // Массив цветов фара
	//DWORD nFarInterfaceSettings;
	//DWORD nFarPanelSettings;
	//DWORD nFarConfirmationSettings;
	//BOOL  bFarPanelAllowed, bFarLeftPanel, bFarRightPanel;   // FCTL_CHECKPANELSEXIST, FCTL_GETPANELSHORTINFO,...
	//CEFAR_SHORT_PANEL_INFO FarLeftPanel, FarRightPanel;

	DWORD ldwConsoleMode = 0;	GetConsoleMode(/*ghConIn*/GetStdHandle(STD_INPUT_HANDLE), &ldwConsoleMode);
	#ifdef _DEBUG
	static DWORD ldwDbgMode = 0;
	if (IsDebuggerPresent()) {
		if (ldwDbgMode != ldwConsoleMode) {
			wchar_t szDbg[128]; wsprintfW(szDbg, L"Far.ConEmuW: ConsoleMode(STD_INPUT_HANDLE)=0x%08X\n", ldwConsoleMode);
			OutputDebugStringW(szDbg);
			ldwDbgMode = ldwConsoleMode;
		}
	}
	#endif
	gpFarInfo->nFarConsoleMode = ldwConsoleMode;
	
	INT_PTR nColorSize = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETARRAYCOLOR, NULL);
	if (nColorSize <= (INT_PTR)sizeof(gpFarInfo->nFarColors)) {
		nColorSize = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETARRAYCOLOR, gpFarInfo->nFarColors);
	} else {
		_ASSERTE(nColorSize <= sizeof(gpFarInfo->nFarColors));
		BYTE* ptr = (BYTE*)calloc(nColorSize,1);
		if (!ptr) {
			memset(gpFarInfo->nFarColors, 7, sizeof(gpFarInfo->nFarColors));
		} else {
			nColorSize = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETARRAYCOLOR, ptr);
			memmove(gpFarInfo->nFarColors, ptr, sizeof(gpFarInfo->nFarColors));
			free(ptr);
		}
	}
	
	gpFarInfo->nFarInterfaceSettings =
		(DWORD)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETINTERFACESETTINGS, 0);
	gpFarInfo->nFarPanelSettings =
		(DWORD)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETPANELSETTINGS, 0);
	gpFarInfo->nFarConfirmationSettings =
		(DWORD)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETCONFIRMATIONS, 0);

	gpFarInfo->bFarPanelAllowed = InfoW995->Control(PANEL_NONE, FCTL_CHECKPANELSEXIST, 0, 0);
	gpFarInfo->bFarPanelInfoFilled = FALSE;
	gpFarInfo->bFarLeftPanel = FALSE;
	gpFarInfo->bFarRightPanel = FALSE;

	// -- пока, во избежание глюков в FAR при неожиданных запросах информации о панелях
	//if (FALSE == (gpFarInfo->bFarPanelAllowed)) {
	//	gpConsoleInfo->bFarLeftPanel = FALSE;
	//	gpConsoleInfo->bFarRightPanel = FALSE;
	//} else {
	//	PanelInfo piA = {0}, piP = {0};
	//	BOOL lbActive  = InfoW995->Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&piA);
	//	BOOL lbPassive = InfoW995->Control(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&piP);
	//	if (!lbActive && !lbPassive)
	//	{
	//		gpConsoleInfo->bFarLeftPanel = FALSE;
	//		gpConsoleInfo->bFarRightPanel = FALSE;
	//	} else {
	//		PanelInfo *ppiL = NULL;
	//		PanelInfo *ppiR = NULL;
	//		if (lbActive) {
	//			if (piA.Flags & PFLAGS_PANELLEFT) ppiL = &piA; else ppiR = &piA;
	//		}
	//		if (lbPassive) {
	//			if (piP.Flags & PFLAGS_PANELLEFT) ppiL = &piP; else ppiR = &piP;
	//		}
	//		gpConsoleInfo->bFarLeftPanel = ppiL!=NULL;
	//		gpConsoleInfo->bFarRightPanel = ppiR!=NULL;
	//		if (ppiL) FarPanel2CePanel(ppiL, &(gpConsoleInfo->FarLeftPanel));
	//		if (ppiR) FarPanel2CePanel(ppiR, &(gpConsoleInfo->FarRightPanel));
	//	}
	//}

	return TRUE;
}

void ExecuteQuitFar995()
{
	if (!InfoW995 || !InfoW995->AdvControl) {
		PostMessage(FarHwnd, WM_CLOSE, 0, 0);
		return;
	}
	
	InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_QUIT, NULL);
}

BOOL CheckBufferEnabled995()
{
	if (!InfoW995 || !InfoW995->AdvControl)
		return FALSE;
	static int siEnabled = 0;
	
	// Чтобы проверку выполнять только один раз.
	// Т.к. буфер может быть реально сброшен, а фар его все-еще умеет.
	if (siEnabled)
	{
		return (siEnabled == 1);
	}

	SMALL_RECT rcFar = {0};
	if (InfoW995->AdvControl(InfoW995->ModuleNumber, 32/*ACTL_GETFARRECT*/, &rcFar))
	{
		if (rcFar.Top > 0 && rcFar.Bottom > rcFar.Top)
		{
			siEnabled = 1;
			return TRUE;
		}
	}
	siEnabled = -1;

	return FALSE;
}

static void CopyPanelInfoW(PanelInfo* pInfo, UpdateBackgroundArg::BkPanelInfo* pBk)
{
	pBk->bVisible = pInfo->Visible;
	pBk->bFocused = pInfo->Focus;
	pBk->bPlugin = pInfo->Plugin;
	HANDLE hPanel = (pInfo->Focus) ? PANEL_ACTIVE : PANEL_PASSIVE;
	InfoW995->Control(hPanel, FCTL_GETCURRENTDIRECTORY /* == FCTL_GETPANELDIR == 25*/, countof(pBk->szCurDir), (LONG_PTR)&pBk->szCurDir);
	if (gFarVersion.dwBuild >= 1657)
	{
		InfoW995->Control(hPanel, 32/*FCTL_GETPANELFORMAT*/, countof(pBk->szCurDir), (LONG_PTR)&pBk->szFormat);
		InfoW995->Control(hPanel, 33/*FCTL_GETPANELHOSTFILE*/, countof(pBk->szCurDir), (LONG_PTR)&pBk->szHostFile);
	}
	else
	{
		lstrcpyW(pBk->szFormat, pInfo->Plugin ? L"Plugin" : L"");
		pBk->szHostFile[0] = 0;
	}
	pBk->rcPanelRect = pInfo->PanelRect;
}

void FillUpdateBackground995(struct UpdateBackgroundArg* pFar)
{
	if (!InfoW995 || !InfoW995->AdvControl)
		return;

	INT_PTR nColorSize = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETARRAYCOLOR, NULL);
	if (nColorSize <= (INT_PTR)sizeof(pFar->nFarColors))
	{
		nColorSize = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETARRAYCOLOR, pFar->nFarColors);
	}
	else
	{
		_ASSERTE(nColorSize <= sizeof(pFar->nFarColors));
		BYTE* ptr = (BYTE*)calloc(nColorSize,1);
		if (!ptr)
		{
			memset(pFar->nFarColors, 7, sizeof(pFar->nFarColors));
		}
		else
		{
			nColorSize = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETARRAYCOLOR, ptr);
			memmove(pFar->nFarColors, ptr, sizeof(pFar->nFarColors));
			free(ptr);
		}
	}
	
	pFar->nFarInterfaceSettings =
		(DWORD)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETINTERFACESETTINGS, 0);
	pFar->nFarPanelSettings =
		(DWORD)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETPANELSETTINGS, 0);

	pFar->bPanelsAllowed = (0 != InfoW995->Control(INVALID_HANDLE_VALUE, FCTL_CHECKPANELSEXIST, 0, 0));

	if (pFar->bPanelsAllowed)
	{
		PanelInfo pasv = {0}, actv = {0};
		InfoW995->Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&actv);
		InfoW995->Control(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pasv);

		PanelInfo* pLeft = (actv.Flags & PFLAGS_PANELLEFT) ? &actv : &pasv;
		PanelInfo* pRight = (actv.Flags & PFLAGS_PANELLEFT) ? &pasv : &actv;

		CopyPanelInfoW(pLeft, &pFar->LeftPanel);
		CopyPanelInfoW(pRight, &pFar->RightPanel);
	}

	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO scbi = {sizeof(CONSOLE_SCREEN_BUFFER_INFO)};
	GetConsoleScreenBufferInfo(hCon, &scbi);
	if (CheckBufferEnabled995())
	{
		SMALL_RECT rc = {0};
		InfoW995->AdvControl(InfoW995->ModuleNumber, 32/*ACTL_GETFARRECT*/, &rc);
		pFar->rcConWorkspace.left = rc.Left;
		pFar->rcConWorkspace.top = rc.Top;
		pFar->rcConWorkspace.right = rc.Right;
		pFar->rcConWorkspace.bottom = rc.Bottom;
	}
	else
	{
		pFar->rcConWorkspace.left = pFar->rcConWorkspace.top = 0;
		pFar->rcConWorkspace.right = scbi.dwSize.X - 1;
		pFar->rcConWorkspace.bottom = scbi.dwSize.Y - 1;
		//pFar->conSize = scbi.dwSize;
	}
	pFar->conCursor = scbi.dwCursorPosition;
	CONSOLE_CURSOR_INFO crsr = {0};
	GetConsoleCursorInfo(hCon, &crsr);
	if (!crsr.bVisible || crsr.dwSize == 0)
	{
		pFar->conCursor.X = pFar->conCursor.Y = -1;
	}
}
