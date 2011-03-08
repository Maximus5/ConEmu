
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
#ifdef _DEBUG
#pragma warning( disable : 4995 )
#endif
#include "..\common\pluginW1900.hpp" // Far3
#ifdef _DEBUG
#pragma warning( default : 4995 )
#endif
#include "PluginHeader.h"
#include "..\ConEmu\version.h"

#ifdef _DEBUG
//#define SHOW_DEBUG_EVENTS
#endif

GUID guid_ConEmu = { /* 4b675d80-1d4a-4ea9-8436-fdc23f2fc14b */
	0x4b675d80,
	0x1d4a,
	0x4ea9,
	{0x84, 0x36, 0xfd, 0xc2, 0x3f, 0x2f, 0xc1, 0x4b}
};
GUID guid_ConEmuPluginItems = { /* 3836ad1f-5130-4a13-93d8-15fefbdc3185 */
	0x3836ad1f,
	0x5130,
	0x4a13,
	{0x93, 0xd8, 0x15, 0xfe, 0xfb, 0xdc, 0x31, 0x85}
};
GUID guid_ConEmuPluginMenu = { /* 830d40da-ccf3-417b-b378-87f9441c4c95 */
	0x830d40da,
	0xccf3,
	0x417b,
	{0xb3, 0x78, 0x87, 0xf9, 0x44, 0x1c, 0x4c, 0x95}
};
GUID guid_ConEmuGuiMacroDlg = { /* a0484f91-a800-4e3a-abac-aed4485da79d */
	0xa0484f91,
	0xa800,
	0x4e3a,
	{0xab, 0xac, 0xae, 0xd4, 0x48, 0x5d, 0xa7, 0x9d}
};
GUID guid_ConEmuWaitEndSynchro = { /* e93fba92-d7de-4651-9be1-c9b064254f65 */
	0xe93fba92,
	0xd7de,
	0x4651,
	{0x9b, 0xe1, 0xc9, 0xb0, 0x64, 0x25, 0x4f, 0x65}
};


struct PluginStartupInfo *InfoW1900=NULL;
struct FarStandardFunctions *FSFW1900=NULL;

void WaitEndSynchroW1900();

void GetPluginInfoW1900(void *piv)
{
	PluginInfo *pi = (PluginInfo*)piv;
	memset(pi, 0, sizeof(PluginInfo));

	pi->StructSize = sizeof(struct PluginInfo);

	static wchar_t *szMenu[1], szMenu1[255];
	szMenu[0]=szMenu1; //lstrcpyW(szMenu[0], L"[&\x2560] ConEmu"); -> 0x2584
	lstrcpynW(szMenu1, GetMsgW(CEPluginName), 240);

	//static WCHAR *szMenu[1], szMenu1[255];
	//szMenu[0]=szMenu1; //lstrcpyW(szMenu[0], L"[&\x2560] ConEmu"); -> 0x2584
	//szMenu[0][1] = L'&';
	//szMenu[0][2] = 0x2560;
	// Проверить, не изменилась ли горячая клавиша плагина, и если да - пересоздать макросы
	//IsKeyChanged(TRUE); -- в FAR2 устарело, используем Synchro
	//if (gcPlugKey) szMenu1[0]=0; else lstrcpyW(szMenu1, L"[&\x2584] ");
	//lstrcpynW(szMenu1+lstrlenW(szMenu1), GetMsgW(2), 240);
	//lstrcpynW(szMenu1, GetMsgW(CEPluginName), 240);
	//_ASSERTE(pi->StructSize = sizeof(struct PluginInfo));
	pi->Flags = PF_EDITOR | PF_VIEWER | PF_DIALOG | PF_PRELOAD;
	//pi->DiskMenuStrings = NULL;
	//pi->DiskMenuNumbers = 0;
	pi->PluginMenu.Guids = &guid_ConEmuPluginMenu;
	pi->PluginMenu.Strings = szMenu;
	pi->PluginMenu.Count = 1;
	//pi->PluginConfigStrings = NULL;
	//pi->PluginConfigStringsNumber = 0;
	pi->CommandPrefix = L"ConEmu";
	//pi->Reserved = ConEmu_SysID; // 'CEMU'
}


void ProcessDragFromW1900()
{
	if (!InfoW1900 || !InfoW1900->AdvControl)
		return;

	WindowInfo WInfo = {sizeof(WindowInfo)};
	WInfo.Pos = 0;
	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
	InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETWINDOWINFO, (void*)&WInfo);

	if (!(WInfo.Flags & WIF_CURRENT))
	{
		int ItemsCount=0;
		//WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);
		OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}

	PanelInfo PInfo;
	WCHAR *szCurDir=gszDir1; szCurDir[0]=0; //(WCHAR*)calloc(0x400,sizeof(WCHAR));
	InfoW1900->Control(PANEL_ACTIVE, FCTL_GETPANELINFO, NULL, (LONG_PTR)&PInfo);

	if ((PInfo.PanelType == PTYPE_FILEPANEL || PInfo.PanelType == PTYPE_TREEPANEL) 
		&& (PInfo.Flags & PFLAGS_VISIBLE))
	{
		InfoW1900->Control(PANEL_ACTIVE, FCTL_GETPANELDIR, 0x400, (LONG_PTR)szCurDir);
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

		if (PInfo.SelectedItemsNumber<=0)
		{
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
		}
		else
		{
			PluginPanelItem **pi = (PluginPanelItem**)calloc(PInfo.SelectedItemsNumber, sizeof(PluginPanelItem*));
			bool *bIsFull = (bool*)calloc(PInfo.SelectedItemsNumber, sizeof(bool));
			int ItemsCount=PInfo.SelectedItemsNumber, i;
			int nMaxLen=MAX_PATH+1, nWholeLen=1;

			// сначала посчитать максимальную длину буфера
			for(i=0; i<ItemsCount; i++)
			{
				size_t sz = InfoW1900->Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, NULL);

				if (!sz)
					continue;

				pi[i] = (PluginPanelItem*)calloc(sz, 1); // размер возвращается в байтах

				if (!InfoW1900->Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, (LONG_PTR)(pi[i])))
				{
					free(pi[i]); pi[i] = NULL;
					continue;
				}

				if (i == 0
				        && ((pi[i]->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
				        && !lstrcmpW(pi[i]->FileName, L".."))
				{
					free(pi[i]); pi[i] = NULL;
					continue;
				}

				int nLen=nDirLen+nDirNoSlash;

				if ((pi[i]->FileName[0] == L'\\' && pi[i]->FileName[1] == L'\\') ||
				        (ISALPHA(pi[i]->FileName[0]) && pi[i]->FileName[1] == L':' && pi[i]->FileName[2] == L'\\'))
					{ nLen = 0; bIsFull[i] = TRUE; } // это уже полный путь!

				nLen += lstrlenW(pi[i]->FileName);

				if (nLen>nMaxLen)
					nMaxLen = nLen;

				nWholeLen += (nLen+1);
			}

			//WriteFile(hPipe, &nWholeLen, sizeof(int), &cout, NULL);
			OutDataWrite(&nWholeLen, sizeof(int));
			WCHAR* Path=new WCHAR[nMaxLen+1];

			for(i=0; i<ItemsCount; i++)
			{
				//WCHAR Path[MAX_PATH+1];
				//ZeroMemory(Path, MAX_PATH+1);
				//Maximus5 - засада с корнем диска и возможностью overflow
				//StringCchPrintf(Path, countof(Path), L"%s\\%s", szCurDir, PInfo.SelectedItems[i]->FileName);
				Path[0]=0;

				if (!pi[i] || !pi[i]->FileName) continue;  //этот элемент получить не удалось

				int nLen=0;

				if (nDirLen>0 && !bIsFull[i])
				{
					lstrcpy(Path, szCurDir);

					if (nDirNoSlash)
					{
						Path[nDirLen]=L'\\';
						Path[nDirLen+1]=0;
					}

					nLen = nDirLen+nDirNoSlash;
				}

				lstrcpy(Path+nLen, pi[i]->FileName);
				nLen += lstrlen(pi[i]->FileName);
				nLen++;
				//WriteFile(hPipe, &nLen, sizeof(int), &cout, NULL);
				OutDataWrite(&nLen, sizeof(int));
				//WriteFile(hPipe, Path, sizeof(WCHAR)*nLen, &cout, NULL);
				OutDataWrite(Path, sizeof(WCHAR)*nLen);
			}

			for(i=0; i<ItemsCount; i++)
			{
				if (pi[i]) free(pi[i]);
			}

			free(pi); pi = NULL;
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

void ProcessDragToW1900()
{
	if (!InfoW1900 || !InfoW1900->AdvControl)
		return;

	WindowInfo WInfo = {sizeof(WindowInfo)};
	WInfo.Pos = 0;
	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
	InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETWINDOWINFO, (void*)&WInfo);

	if (!(WInfo.Flags & WIF_CURRENT))
	{
		//InfoW1900->AdvControl(&guid_ConEmu, ACTL_FREEWINDOWINFO, (void*)&WInfo);
		int ItemsCount=0;

		//WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);
		if (gpCmdRet==NULL)
			OutDataAlloc(sizeof(ItemsCount));

		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}

	//InfoW1900->AdvControl(&guid_ConEmu, ACTL_FREEWINDOWINFO, (void*)&WInfo);
	PanelInfo PAInfo, PPInfo;
	ForwardedPanelInfo *pfpi=NULL;
	int nStructSize = sizeof(ForwardedPanelInfo)+4; // потом увеличим на длину строк
	//ZeroMemory(&fpi, sizeof(fpi));
	BOOL lbAOK=FALSE, lbPOK=FALSE;
	WCHAR *szPDir=gszDir1; szPDir[0]=0; //(WCHAR*)calloc(0x400,sizeof(WCHAR));
	WCHAR *szADir=gszDir2; szADir[0]=0; //(WCHAR*)calloc(0x400,sizeof(WCHAR));
	//if (!(lbAOK=InfoW1900->Control(PANEL_ACTIVE, FCTL_GETPANELSHORTINFO, &PAInfo)))
	lbAOK=InfoW1900->Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&PAInfo) &&
	      InfoW1900->Control(PANEL_ACTIVE, FCTL_GETPANELDIR, 0x400, (LONG_PTR)szADir);

	if (lbAOK && szADir)
		nStructSize += (lstrlen(szADir))*sizeof(WCHAR);

	lbPOK=InfoW1900->Control(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&PPInfo) &&
	      InfoW1900->Control(PANEL_PASSIVE, FCTL_GETPANELDIR, 0x400, (LONG_PTR)szPDir);

	if (lbPOK && szPDir)
		nStructSize += (lstrlen(szPDir))*sizeof(WCHAR); // Именно WCHAR! не TCHAR

	pfpi = (ForwardedPanelInfo*)calloc(nStructSize,1);

	if (!pfpi)
	{
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

		if (!(PAInfo.Flags & PFLAGS_PLUGIN)
			&& (PAInfo.PanelType == PTYPE_FILEPANEL || PAInfo.PanelType == PTYPE_TREEPANEL) 
			&& (PAInfo.Flags & PFLAGS_VISIBLE))
		{
			if (szADir[0])
			{
				lstrcpyW(pfpi->pszActivePath, szADir);
				pfpi->PassivePathShift += lstrlenW(pfpi->pszActivePath)*2;
			}
		}
	}

	pfpi->pszPassivePath = (WCHAR*)(((char*)pfpi)+pfpi->PassivePathShift);

	if (lbPOK)
	{
		pfpi->PassiveRect=PPInfo.PanelRect;

		if (!(PPInfo.Flags & PFLAGS_PLUGIN)
			&& (PPInfo.PanelType == PTYPE_FILEPANEL || PPInfo.PanelType == PTYPE_TREEPANEL) 
			&& (PPInfo.Flags & PFLAGS_VISIBLE))
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

void SetStartupInfoW1900(void *aInfo)
{
	::InfoW1900 = (PluginStartupInfo*)calloc(sizeof(PluginStartupInfo),1);
	::FSFW1900 = (FarStandardFunctions*)calloc(sizeof(FarStandardFunctions),1);

	if (::InfoW1900 == NULL || ::FSFW1900 == NULL)
		return;

	*::InfoW1900 = *((struct PluginStartupInfo*)aInfo);
	*::FSFW1900 = *((struct PluginStartupInfo*)aInfo)->FSF;
	::InfoW1900->FSF = ::FSFW1900;

	DWORD nFarVer = 0;
	if (InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETFARVERSION, &nFarVer))
	{
		if (HIBYTE(HIWORD(nFarVer)) == 3)
		{
			gFarVersion.dwBuild = LOWORD(nFarVer);
			gFarVersion.dwVerMajor = (HIBYTE(HIWORD(nFarVer)));
			gFarVersion.dwVerMinor = (LOBYTE(HIWORD(nFarVer)));
			InitRootKey();
		}
		else
		{
			_ASSERTE(HIBYTE(HIWORD(nFarVer)) == 3);
		}
	}
	//lstrcpynW(gszRootKey, InfoW1900->RootKey, MAX_PATH);
	//WCHAR* pszSlash = gszRootKey+lstrlenW(gszRootKey)-1;

	//if (*pszSlash == L'\\') *(pszSlash--) = 0;

	//while(pszSlash>gszRootKey && *pszSlash!=L'\\') pszSlash--;

	//*pszSlash = 0;
	/*if (!FarHwnd)
		InitHWND((HWND)InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETFARHWND, 0));*/
}

DWORD GetEditorModifiedStateW1900()
{
	EditorInfo ei;
	InfoW1900->EditorControl(-1/*Active editor*/, ECTL_GETINFO, 0, (INT_PTR)&ei);
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
int ProcessEditorInputW1900(LPCVOID aRec)
{
	if (!InfoW1900)
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


bool UpdateConEmuTabsW1900(int anEvent, bool losingFocus, bool editorSave, void* Param/*=NULL*/)
{
	if (!InfoW1900 || !InfoW1900->AdvControl || gbIgnoreUpdateTabs)
		return false;

	BOOL lbCh = FALSE;
	WindowInfo WInfo = {sizeof(WindowInfo)};
	wchar_t szWNameBuffer[CONEMUTABMAX];
	//WInfo.Name = szWNameBuffer;
	//WInfo.NameSize = CONEMUTABMAX;
	int windowCount = (int)InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETWINDOWCOUNT, NULL);
	lbCh = (lastWindowCount != windowCount);

	if (!CreateTabs(windowCount))
		return false;

	//EditorInfo ei = {0};
	//if (editorSave)
	//{
	//	InfoW1900->EditorControl(-1/*Active editor*/, ECTL_GETINFO, &ei);
	//}
	ViewerInfo vi = {sizeof(ViewerInfo)};

	if (anEvent == 206)
	{
		if (Param)
			vi.ViewerID = *(int*)Param;

		InfoW1900->ViewerControl(-1/*Active viewer*/, VCTL_GETINFO, 0, (INT_PTR)&vi);
	}

	int tabCount = 0;
	BOOL lbActiveFound = FALSE;

	for(int i = 0; i < windowCount; i++)
	{
		WInfo.Pos = i;
		_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
		InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETWINDOWINFO, (void*)&WInfo);

		if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
		{
			WInfo.Pos = i;
			WInfo.Name = szWNameBuffer;
			WInfo.NameSize = CONEMUTABMAX;
			InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETWINDOWINFO, (void*)&WInfo);
			WARNING("Для получения имени нужно пользовать ECTL_GETFILENAME");

			if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
			{
				if ((WInfo.Flags & WIF_CURRENT)) lbActiveFound = TRUE;

				TODO("Определение ИД редактора/вьювера");
				lbCh |= AddTab(tabCount, losingFocus, editorSave,
				               WInfo.Type, WInfo.Name, /*editorSave ? ei.FileName :*/ NULL,
				               (WInfo.Flags & WIF_CURRENT), (WInfo.Flags & WIF_MODIFIED), 0);
				//if (WInfo.Type == WTYPE_EDITOR && WInfo.Current) //2009-08-17
				//	lastModifiedStateW = WInfo.Modified;
			}

			//InfoW1900->AdvControl(&guid_ConEmu, ACTL_FREEWINDOWINFO, (void*)&WInfo);
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

		_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
		if (InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETWINDOWINFO, (void*)&WInfo))
		{
			if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER)
			{
				WInfo.Pos = -1;
				WInfo.Name = szWNameBuffer;
				WInfo.NameSize = CONEMUTABMAX;
				InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETWINDOWINFO, (void*)&WInfo);

				if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER)
				{
					tabCount = 0;
					TODO("Определение ИД Редактора/вьювера");
					lbCh |= AddTab(tabCount, losingFocus, editorSave,
					               WInfo.Type, WInfo.Name, /*editorSave ? ei.FileName :*/ NULL,
					               (WInfo.Flags & WIF_CURRENT), (WInfo.Flags & WIF_MODIFIED), 0);
				}
			}
			else if (WInfo.Type == WTYPE_PANELS)
			{
				gpTabs->Tabs.CurrentType = WInfo.Type;
			}
		}

		//wchar_t* pszEditorFileName = NULL;
		//EditorInfo ei = {0};
		//ViewerInfo vi = {sizeof(ViewerInfo)};
		//BOOL bEditor = FALSE, bViewer = FALSE;
		//bViewer = InfoW1900->ViewerControl(VCTL_GETINFO, &vi);
		//if (InfoW1900->EditorControl(ECTL_GETINFO, &ei)) {
		//	int nLen = InfoW1900->EditorControl(ECTL_GETFILENAME, NULL);
		//	if (nLen > 0) {
		//		wchar_t* pszEditorFileName = (wchar_t*)calloc(nLen+1,2);
		//		if (pszEditorFileName) {
		//			if (InfoW1900->EditorControl(ECTL_GETFILENAME, pszEditorFileName)) {
		//				bEditor = true;
		//			}
		//		}
		//	}
		//}
		//if (bEditor && bViewer) {
		//	// Попробуем получить информацию об активном окне, но это может привести к блокировке некоторых диалогов ФАР2?
		//	WInfo.Pos = -1;
		//	InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETWINDOWINFO, (void*)&WInfo);
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

	// 101224 - сразу запомнить количество!
	gpTabs->Tabs.nTabCount = tabCount;
	//// 2009-08-17
	//if (gbHandleOneRedraw && gbHandleOneRedrawCh && lbCh) {
	//	gbHandleOneRedraw = false;
	//	gbHandleOneRedrawCh = false;
	//}
#ifdef _DEBUG
	//WCHAR szDbg[128]; StringCchPrintf(szDbg, countof(szDbg), L"Event: %i, count %i\n", anEvent, tabCount);
	//OutputDebugStringW(szDbg);
#endif
	//SendTabs(tabCount, lbCh && (gnReqCommand==(DWORD)-1));
	return (lbCh != FALSE);
}

void ExitFARW1900(void)
{
	WaitEndSynchroW1900();

	if (InfoW1900)
	{
		free(InfoW1900);
		InfoW1900=NULL;
	}

	if (FSFW1900)
	{
		free(FSFW1900);
		FSFW1900=NULL;
	}
}

int ShowMessageW1900(int aiMsg, int aiButtons)
{
	if (!InfoW1900 || !InfoW1900->Message || !InfoW1900->GetMsg)
		return -1;

	return InfoW1900->Message(&guid_ConEmu, FMSG_ALLINONE1900|FMSG_MB_OK|FMSG_WARNING, NULL,
	                         (const wchar_t * const *)InfoW1900->GetMsg(&guid_ConEmu,aiMsg), 0, aiButtons);
}

LPCWSTR GetMsgW1900(int aiMsg)
{
	if (!InfoW1900 || !InfoW1900->GetMsg)
		return L"";

	return InfoW1900->GetMsg(&guid_ConEmu,aiMsg);
}

//void ReloadMacroW1900()
//{
//	if (!InfoW1900 || !InfoW1900->AdvControl)
//		return;
//
//	ActlKeyMacro command;
//	command.Command=MCMD_LOADALL;
//	InfoW1900->AdvControl(&guid_ConEmu,ACTL_KEYMACRO,&command);
//}

void SetWindowW1900(int nTab)
{
	if (!InfoW1900 || !InfoW1900->AdvControl)
		return;

	if (InfoW1900->AdvControl(&guid_ConEmu, ACTL_SETCURRENTWINDOW, (void*)nTab))
		InfoW1900->AdvControl(&guid_ConEmu, ACTL_COMMIT, 0);
}

// Warning, напрямую НЕ вызывать. Пользоваться "общей" PostMacro
void PostMacroW1900(wchar_t* asMacro)
{
	if (!InfoW1900 || !InfoW1900->AdvControl)
		return;

	MacroSendMacroText mcr = {sizeof(MacroSendMacroText)};
	//mcr.Flags = 0; // По умолчанию - вывод на экран разрешен

	if (*asMacro == L'@' && asMacro[1] && asMacro[1] != L' ')
	{
		mcr.Flags |= KMFLAGS_DISABLEOUTPUT;
		asMacro ++;
	}

	mcr.SequenceText = asMacro;
	InfoW1900->MacroControl(INVALID_HANDLE_VALUE, MCTL_SENDSTRING, 0, (INT_PTR)&mcr);
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
	////InfoW1900->AdvControl(&guid_ConEmu,ACTL_REDRAWALL,NULL);
}

int ShowPluginMenuW1900()
{
	if (!InfoW1900)
		return -1;

	FarMenuItem items[] =
	{
		{ConEmuHwnd ? MIF_SELECTED : MIF_DISABLE,  InfoW1900->GetMsg(&guid_ConEmu,CEMenuEditOutput)},
		{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW1900->GetMsg(&guid_ConEmu,CEMenuViewOutput)},
		{MIF_SEPARATOR},
		{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW1900->GetMsg(&guid_ConEmu,CEMenuShowHideTabs)},
		{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW1900->GetMsg(&guid_ConEmu,CEMenuNextTab)},
		{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW1900->GetMsg(&guid_ConEmu,CEMenuPrevTab)},
		{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW1900->GetMsg(&guid_ConEmu,CEMenuCommitTab)},
		{MIF_SEPARATOR},
		{0,                                        InfoW1900->GetMsg(&guid_ConEmu,CEMenuGuiMacro)},
		{MIF_SEPARATOR},
		{ConEmuHwnd||IsTerminalMode() ? MIF_DISABLE : MIF_SELECTED,  InfoW1900->GetMsg(&guid_ConEmu,CEMenuAttach)},
		{MIF_SEPARATOR},
		//#ifdef _DEBUG
		//{0, L"&~. Raise exception"},
		//#endif
		{IsDebuggerPresent()||IsTerminalMode() ? MIF_DISABLE : 0,    InfoW1900->GetMsg(&guid_ConEmu,CEMenuDebug)},
	};
	int nCount = sizeof(items)/sizeof(items[0]);
	int nRc = InfoW1900->Menu(&guid_ConEmu, -1,-1, 0,
	                         FMENU_AUTOHIGHLIGHT|FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
	                         InfoW1900->GetMsg(&guid_ConEmu,CEPluginName),
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

BOOL EditOutputW1900(LPCWSTR asFileName, BOOL abView)
{
	if (!InfoW1900)
		return FALSE;

	BOOL lbRc = FALSE;

	if (!abView)
	{
		int iRc =
		    InfoW1900->Editor(asFileName, InfoW1900->GetMsg(&guid_ConEmu,CEConsoleOutput), 0,0,-1,-1,
		                     EF_NONMODAL|EF_IMMEDIATERETURN|EF_DELETEONLYFILEONCLOSE|EF_ENABLE_F6|EF_DISABLEHISTORY,
		                     0, 1, 1200);
		lbRc = (iRc != EEC_OPEN_ERROR);
	}
	else
	{
#ifdef _DEBUG
		int iRc =
#endif
		    InfoW1900->Viewer(asFileName, InfoW1900->GetMsg(&guid_ConEmu,CEConsoleOutput), 0,0,-1,-1,
		                     VF_NONMODAL|VF_IMMEDIATERETURN|VF_DELETEONLYFILEONCLOSE|VF_ENABLE_F6|VF_DISABLEHISTORY,
		                     1200);
		lbRc = TRUE;
	}

	return lbRc;
}

BOOL ExecuteSynchroW1900()
{
	if (!InfoW1900)
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
		InfoW1900->AdvControl(&guid_ConEmu, ACTL_SYNCHRO, NULL);
		return TRUE;
	}

	return FALSE;
}

static HANDLE ghSyncDlg = NULL;

void WaitEndSynchroW1900()
{
	if ((gnSynchroCount == 0) || !(IS_SYNCHRO_ALLOWED))
		return;

	FarDialogItem items[] =
	{
		{DI_DOUBLEBOX,  3,  1,  51, 3, false, 0, 0, 0, 0, GetMsgW1900(CEPluginName)},

		{DI_BUTTON,     0,  2,  0,  0, true,  0, 0, DIF_CENTERGROUP|DIF_DEFAULTBUTTON, 0, GetMsgW1900(CEStopSynchroWaiting)},
	};
	
	GUID ConEmuWaitEndSynchro = { /* d0f369dc-2800-4833-a858-43dd1c115370 */
		    0xd0f369dc,
		    0x2800,
		    0x4833,
		    {0xa8, 0x58, 0x43, 0xdd, 0x1c, 0x11, 0x53, 0x70}
		  };
	
	ghSyncDlg = InfoW1900->DialogInit(&guid_ConEmu, &guid_ConEmuWaitEndSynchro,
			-1,-1, 55, 5, NULL, items, countof(items), 0, 0, NULL, 0);

	if (ghSyncDlg == INVALID_HANDLE_VALUE)
	{
		// Видимо, Фар в состоянии выхода (финальная выгрузка всех плагинов)
		// В этом случае, по идее, Synchro вызываться более не должно
		gnSynchroCount = 0; // так что просто сбросим счетчик
	}
	else
	{
		InfoW1900->DialogRun(ghSyncDlg);
		InfoW1900->DialogFree(ghSyncDlg);
	}

	ghSyncDlg = NULL;
}

void StopWaitEndSynchroW1900()
{
	if (ghSyncDlg)
	{
		InfoW1900->SendDlgMessage(ghSyncDlg, DM_CLOSE, -1, 0);
	}
}


// Param должен быть выделен в куче. Память освобождается в ProcessSynchroEventW.
//BOOL CallSynchroW1900(SynchroArg *Param, DWORD nTimeout /*= 10000*/)
//{
//	if (!InfoW1900 || !Param)
//		return FALSE;
//
//	if (gFarVersion.dwVerMajor>1 && (gFarVersion.dwVerMinor>0 || gFarVersion.dwBuild>=1006)) {
//		// Функция всегда возвращает 0
//		if (Param->hEvent)
//			ResetEvent(Param->hEvent);
//
//		//Param->Processed = FALSE;
//
//		InfoW1900->AdvControl ( &guid_ConEmu, ACTL_SYNCHRO, Param);
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

BOOL IsMacroActiveW1900()
{
	if (!InfoW1900) return FALSE;

	INT_PTR liRc = InfoW1900->MacroControl(INVALID_HANDLE_VALUE, MCTL_GETSTATE, 0, 0);

	if (liRc == MACROSTATE_NOMACRO)
		return FALSE;

	return TRUE;
}


void RedrawAllW1900()
{
	if (!InfoW1900) return;

	InfoW1900->AdvControl(&guid_ConEmu, ACTL_REDRAWALL, NULL);
}

bool LoadPluginW1900(wchar_t* pszPluginPath)
{
	if (!InfoW1900) return false;

	InfoW1900->PluginsControl(INVALID_HANDLE_VALUE,PCTL_LOADPLUGIN,PLT_PATH,(LONG_PTR)pszPluginPath);
	return true;
}

bool RunExternalProgramW(wchar_t* pszCommand, wchar_t* pszCurDir);

bool RunExternalProgramW1900(wchar_t* pszCommand)
{
	wchar_t strTemp[MAX_PATH+1];

	if (!pszCommand || !*pszCommand)
	{
		lstrcpy(strTemp, L"cmd");

		if (!InfoW1900->InputBox(&guid_ConEmu, L"ConEmu", L"Start console program", L"ConEmu.CreateProcess",
		                       strTemp, strTemp, MAX_PATH, NULL, FIB_BUTTONS))
			return false;

		pszCommand = strTemp;
	}

	wchar_t *pszCurDir = NULL;
	size_t len;

	if ((len = InfoW1900->Control(INVALID_HANDLE_VALUE, FCTL_GETPANELDIR, 0, 0)) != 0)
	{
		if ((pszCurDir = (wchar_t*)malloc(len*2)) != NULL)
		{
			if (!InfoW1900->Control(INVALID_HANDLE_VALUE, FCTL_GETPANELDIR, (int)len, (LONG_PTR)pszCurDir))
			{
				free(pszCurDir); pszCurDir = NULL;
			}
		}
	}

	if (!pszCurDir)
	{
		pszCurDir = (wchar_t*)malloc(10);
		lstrcpy(pszCurDir, L"C:\\");
	}

	InfoW1900->Control(INVALID_HANDLE_VALUE,FCTL_GETUSERSCREEN,0,0);
	RunExternalProgramW(pszCommand, pszCurDir);
	InfoW1900->Control(INVALID_HANDLE_VALUE,FCTL_SETUSERSCREEN,0,0);
	InfoW1900->AdvControl(&guid_ConEmu,ACTL_REDRAWALL,0);
	free(pszCurDir); pszCurDir = NULL;
	return TRUE;
}


bool ProcessCommandLineW1900(wchar_t* pszCommand)
{
	if (!InfoW1900 || !FSFW1900) return false;

	if (FSFW1900->LStrnicmp(pszCommand, L"run:", 4)==0)
	{
		RunExternalProgramW1900(pszCommand+4);
		return true;
	}

	return false;
}

static void FarPanel2CePanel(PanelInfo* pFar, CEFAR_SHORT_PANEL_INFO* pCE)
{
	pCE->PanelType = pFar->PanelType;
	pCE->Plugin = ((pFar->Flags & PFLAGS_PLUGIN) == PFLAGS_PLUGIN);
	pCE->PanelRect = pFar->PanelRect;
	pCE->ItemsNumber = pFar->ItemsNumber;
	pCE->SelectedItemsNumber = pFar->SelectedItemsNumber;
	pCE->CurrentItem = pFar->CurrentItem;
	pCE->TopPanelItem = pFar->TopPanelItem;
	pCE->Visible = ((pFar->Flags & PFLAGS_VISIBLE) == PFLAGS_VISIBLE);
	pCE->Focus = ((pFar->Flags & PFLAGS_FOCUS) == PFLAGS_FOCUS);
	pCE->ViewMode = pFar->ViewMode;
	pCE->ShortNames = ((pFar->Flags & PFLAGS_ALTERNATIVENAMES) == PFLAGS_ALTERNATIVENAMES);
	pCE->SortMode = pFar->SortMode;
	pCE->Flags = pFar->Flags;
}

BOOL ReloadFarInfoW1900(/*BOOL abFull*/)
{
	if (!InfoW1900 || !FSFW1900) return FALSE;

	if (!gpFarInfo)
	{
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

	if (IsDebuggerPresent())
	{
		if (ldwDbgMode != ldwConsoleMode)
		{
			wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"Far.ConEmuW: ConsoleMode(STD_INPUT_HANDLE)=0x%08X\n", ldwConsoleMode);
			OutputDebugStringW(szDbg);
			ldwDbgMode = ldwConsoleMode;
		}
	}

#endif
	gpFarInfo->nFarConsoleMode = ldwConsoleMode;
	INT_PTR nColorSize = InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETARRAYCOLOR, NULL);

	if (nColorSize <= (INT_PTR)sizeof(gpFarInfo->nFarColors))
	{
		nColorSize = InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETARRAYCOLOR, gpFarInfo->nFarColors);
	}
	else
	{
		_ASSERTE(nColorSize <= sizeof(gpFarInfo->nFarColors));
		BYTE* ptr = (BYTE*)calloc(nColorSize,1);

		if (!ptr)
		{
			memset(gpFarInfo->nFarColors, 7, sizeof(gpFarInfo->nFarColors));
		}
		else
		{
			nColorSize = InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETARRAYCOLOR, ptr);
			memmove(gpFarInfo->nFarColors, ptr, sizeof(gpFarInfo->nFarColors));
			free(ptr);
		}
	}

	gpFarInfo->nFarInterfaceSettings =
	    (DWORD)InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETINTERFACESETTINGS, 0);
	gpFarInfo->nFarPanelSettings =
	    (DWORD)InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETPANELSETTINGS, 0);
	gpFarInfo->nFarConfirmationSettings =
	    (DWORD)InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETCONFIRMATIONS, 0);
	gpFarInfo->bFarPanelAllowed = InfoW1900->Control(PANEL_NONE, FCTL_CHECKPANELSEXIST, 0, 0);
	gpFarInfo->bFarPanelInfoFilled = FALSE;
	gpFarInfo->bFarLeftPanel = FALSE;
	gpFarInfo->bFarRightPanel = FALSE;
	// -- пока, во избежание глюков в FAR при неожиданных запросах информации о панелях
	//if (FALSE == (gpFarInfo->bFarPanelAllowed)) {
	//	gpConMapInfo->bFarLeftPanel = FALSE;
	//	gpConMapInfo->bFarRightPanel = FALSE;
	//} else {
	//	PanelInfo piA = {0}, piP = {0};
	//	BOOL lbActive  = InfoW1900->Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&piA);
	//	BOOL lbPassive = InfoW1900->Control(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&piP);
	//	if (!lbActive && !lbPassive)
	//	{
	//		gpConMapInfo->bFarLeftPanel = FALSE;
	//		gpConMapInfo->bFarRightPanel = FALSE;
	//	} else {
	//		PanelInfo *ppiL = NULL;
	//		PanelInfo *ppiR = NULL;
	//		if (lbActive) {
	//			if (piA.Flags & PFLAGS_PANELLEFT) ppiL = &piA; else ppiR = &piA;
	//		}
	//		if (lbPassive) {
	//			if (piP.Flags & PFLAGS_PANELLEFT) ppiL = &piP; else ppiR = &piP;
	//		}
	//		gpConMapInfo->bFarLeftPanel = ppiL!=NULL;
	//		gpConMapInfo->bFarRightPanel = ppiR!=NULL;
	//		if (ppiL) FarPanel2CePanel(ppiL, &(gpConMapInfo->FarLeftPanel));
	//		if (ppiR) FarPanel2CePanel(ppiR, &(gpConMapInfo->FarRightPanel));
	//	}
	//}
	return TRUE;
}

void ExecuteQuitFarW1900()
{
	if (!InfoW1900 || !InfoW1900->AdvControl)
	{
		PostMessage(FarHwnd, WM_CLOSE, 0, 0);
		return;
	}

	InfoW1900->AdvControl(&guid_ConEmu, ACTL_QUIT, NULL);
}

BOOL CheckBufferEnabledW1900()
{
	if (!InfoW1900 || !InfoW1900->AdvControl)
		return FALSE;

	static int siEnabled = 0;

	// Чтобы проверку выполнять только один раз.
	// Т.к. буфер может быть реально сброшен, а фар его все-еще умеет.
	if (siEnabled)
	{
		return (siEnabled == 1);
	}

	SMALL_RECT rcFar = {0};

	if (InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETFARRECT, &rcFar))
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

static void CopyPanelInfoW(PanelInfo* pInfo, PaintBackgroundArg::BkPanelInfo* pBk)
{
	pBk->bVisible = ((pInfo->Flags & PFLAGS_VISIBLE) == PFLAGS_VISIBLE);
	pBk->bFocused = ((pInfo->Flags & PFLAGS_FOCUS) == PFLAGS_FOCUS);
	pBk->bPlugin = ((pInfo->Flags & PFLAGS_PLUGIN) == PFLAGS_PLUGIN);
	HANDLE hPanel = (pBk->bFocused) ? PANEL_ACTIVE : PANEL_PASSIVE;
	InfoW1900->Control(hPanel, FCTL_GETPANELDIR /* == FCTL_GETPANELDIR == 25*/, BkPanelInfo_CurDirMax, (LONG_PTR)pBk->szCurDir);

	if (gFarVersion.dwBuild >= 1657)
	{
		InfoW1900->Control(hPanel, FCTL_GETPANELFORMAT, BkPanelInfo_FormatMax, (LONG_PTR)pBk->szFormat);
		InfoW1900->Control(hPanel, FCTL_GETPANELHOSTFILE, BkPanelInfo_HostFileMax, (LONG_PTR)pBk->szHostFile);
	}
	else
	{
		lstrcpyW(pBk->szFormat, (pInfo->Flags & PFLAGS_PLUGIN) ? L"Plugin" : L"");
		pBk->szHostFile[0] = 0;
	}

	pBk->rcPanelRect = pInfo->PanelRect;
}

void FillUpdateBackgroundW1900(struct PaintBackgroundArg* pFar)
{
	if (!InfoW1900 || !InfoW1900->AdvControl)
		return;

	INT_PTR nColorSize = InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETARRAYCOLOR, NULL);

	if (nColorSize <= (INT_PTR)sizeof(pFar->nFarColors))
	{
		nColorSize = InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETARRAYCOLOR, pFar->nFarColors);
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
			nColorSize = InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETARRAYCOLOR, ptr);
			memmove(pFar->nFarColors, ptr, sizeof(pFar->nFarColors));
			free(ptr);
		}
	}

	pFar->nFarInterfaceSettings =
	    (DWORD)InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETINTERFACESETTINGS, 0);
	pFar->nFarPanelSettings =
	    (DWORD)InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETPANELSETTINGS, 0);
	pFar->bPanelsAllowed = (0 != InfoW1900->Control(INVALID_HANDLE_VALUE, FCTL_CHECKPANELSEXIST, 0, 0));

	if (pFar->bPanelsAllowed)
	{
		PanelInfo pasv = {0}, actv = {0};
		InfoW1900->Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&actv);
		InfoW1900->Control(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pasv);
		PanelInfo* pLeft = (actv.Flags & PFLAGS_PANELLEFT) ? &actv : &pasv;
		PanelInfo* pRight = (actv.Flags & PFLAGS_PANELLEFT) ? &pasv : &actv;
		CopyPanelInfoW(pLeft, &pFar->LeftPanel);
		CopyPanelInfoW(pRight, &pFar->RightPanel);
	}

	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO scbi = {sizeof(CONSOLE_SCREEN_BUFFER_INFO)};
	GetConsoleScreenBufferInfo(hCon, &scbi);

	if (CheckBufferEnabledW1900())
	{
		SMALL_RECT rc = {0};
		InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETFARRECT, &rc);
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

int GetActiveWindowTypeW1900()
{
	if (!InfoW1900 || !InfoW1900->AdvControl)
		return -1;

	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);

	INT_PTR nArea = InfoW1900->MacroControl(INVALID_HANDLE_VALUE, MCTL_GETAREA, 0, 0);

	switch(nArea)
	{
		case MACROAREA_SHELL:
		case MACROAREA_INFOPANEL:
		case MACROAREA_QVIEWPANEL:
		case MACROAREA_TREEPANEL:
			return WTYPE_PANELS;
		case MACROAREA_VIEWER:
			return WTYPE_VIEWER;
		case MACROAREA_EDITOR:
			return WTYPE_EDITOR;
		case MACROAREA_DIALOG:
		case MACROAREA_SEARCH:
		case MACROAREA_DISKS:
		case MACROAREA_FINDFOLDER:
		case MACROAREA_AUTOCOMPLETION:
			return WTYPE_DIALOG;
		case MACROAREA_HELP:
			return WTYPE_HELP;
		case MACROAREA_MAINMENU:
		case MACROAREA_MENU:
		case MACROAREA_USERMENU:
			return WTYPE_VMENU;
		case MACROAREA_OTHER: // Grabber
			return -1;
	}

	// Сюда мы попасть не должны, все макрообласти должны быть учтены в switch
	_ASSERTE(nArea==MACROAREA_SHELL);
	return -1;
}

//static LONG_PTR WINAPI CallGuiMacroDlg(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);
//HANDLE CreateGuiMacroDlg(int height, FarDialogItem *items, int nCount)
//{
//	static const GUID ConEmuCallGuiMacro = { /* 21f61504-b40f-45e3-9d27-84db16bc9c22 */
//		    0x21f61504,
//		    0xb40f,
//		    0x45e3,
//		    {0x9d, 0x27, 0x84, 0xdb, 0x16, 0xbc, 0x9c, 0x22}
//		  };
//
//	HANDLE hDlg = InfoW1900->DialogInitW1900(&guid_ConEmu, ConEmuCallGuiMacro,
//									-1, -1, 76, height,
//									NULL/*L"Configure"*/, items, nCount, 
//									0, 0/*Flags*/, (FARWINDOWPROC)CallGuiMacroDlg, 0);
//	return hDlg;
//}

#define FAR_UNICODE 1867
#include "Dialogs.h"
void GuiMacroDlgW1900()
{
	CallGuiMacroProc();
}

//GUID ConEmuGuid = { /* 374471b3-db1e-4276-bf9e-c486fcce4553 */
//						    0x374471b3,
//						    0xdb1e,
//						    0x4276,
//						    {0xbf, 0x9e, 0xc4, 0x86, 0xfc, 0xce, 0x45, 0x53}
//				  };

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	//static wchar_t szTitle[16]; _wcscpy_c(szTitle, L"ConEmu");
	//static wchar_t szDescr[64]; _wcscpy_c(szTitle, L"ConEmu support for Far Manager");
	//static wchar_t szAuthr[64]; _wcscpy_c(szTitle, L"ConEmu.Maximus5@gmail.com");
	
	Info->StructSize = sizeof(GlobalInfo);
	Info->MinFarVersion = FARMANAGERVERSION;
	//#define D(N) (1##N-100)
	// YYMMDDX (YY - две цифры года, MM - месяц, DD - день, X - 0 и выше-номер подсборки)
	Info->Version = ((MVV_1 % 100)*100000) + (MVV_2*1000) + (MVV_3*10) + (MVV_4 % 10);
	Info->Guid = guid_ConEmu;
	Info->Title = L"ConEmu";
	Info->Description = L"ConEmu support for Far Manager";
	Info->Author = L"ConEmu.Maximus5@gmail.com";
}

extern BOOL gbInfoW_OK;
HANDLE WINAPI OpenW(const struct OpenInfo *Info)
{
	if (!gbInfoW_OK)
		return INVALID_HANDLE_VALUE;
	
	return OpenPluginWcmn(Info->OpenFrom, Info->Data);
}

#ifdef _DEBUG
extern CONSOLE_SCREEN_BUFFER_INFO csbi;
extern int gnPage;
extern bool gbShowAttrsOnly;
extern void ShowConDump(wchar_t* pszText);
extern void ShowConPacket(CESERVER_REQ* pReq);
HANDLE WINAPI OpenFilePluginW1900(const wchar_t *Name,const unsigned char *Data,int DataSize,int OpMode)
{
	//Name==NULL, когда Shift-F1
	if (!InfoW1900) return INVALID_HANDLE_VALUE;

	if (OpMode || Name == NULL) return INVALID_HANDLE_VALUE;  // только из панелей в обычном режиме

	const wchar_t* pszDot = wcsrchr(Name, L'.');

	if (!pszDot) return INVALID_HANDLE_VALUE;

	if (lstrcmpi(pszDot, L".con")) return INVALID_HANDLE_VALUE;

	if (DataSize < sizeof(CESERVER_REQ_HDR)) return INVALID_HANDLE_VALUE;

	HANDLE hFile = CreateFile(Name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		InfoW1900->Message(&guid_ConEmu, FMSG_ALLINONE1900|FMSG_MB_OK|FMSG_WARNING,
			NULL, (const wchar_t* const*)L"ConEmu plugin\nCan't open file!", 0,0);
		return INVALID_HANDLE_VALUE;
	}

	DWORD dwSizeLow, dwSizeHigh = 0;
	dwSizeLow = GetFileSize(hFile, &dwSizeHigh);

	if (dwSizeHigh)
	{
		InfoW1900->Message(&guid_ConEmu, FMSG_ALLINONE1900|FMSG_MB_OK|FMSG_WARNING,
			NULL, (const wchar_t* const*)L"ConEmu plugin\nFile too large!", 0,0);
		CloseHandle(hFile);
		return INVALID_HANDLE_VALUE;
	}

	wchar_t* pszData = (wchar_t*)calloc(dwSizeLow+2,1);

	if (!pszData) return INVALID_HANDLE_VALUE;

	if (!ReadFile(hFile, pszData, dwSizeLow, &dwSizeHigh, 0) || dwSizeHigh != dwSizeLow)
	{
		InfoW1900->Message(&guid_ConEmu, FMSG_ALLINONE1900|FMSG_MB_OK|FMSG_WARNING,
			NULL, (const wchar_t* const*)L"ConEmu plugin\nCan't read file!", 0,0);
		return INVALID_HANDLE_VALUE;
	}

	CloseHandle(hFile);
	memset(&csbi, 0, sizeof(csbi));
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	HANDLE h = InfoW1900->SaveScreen(0,0,-1,-1);
	CESERVER_REQ* pReq = (CESERVER_REQ*)pszData;

	if (pReq->hdr.cbSize == dwSizeLow)
	{
		if (pReq->hdr.nVersion != CESERVER_REQ_VER && pReq->hdr.nVersion < 40)
		{
			InfoW1900->Message(&guid_ConEmu, FMSG_ALLINONE1900|FMSG_MB_OK|FMSG_WARNING,
				NULL, (const wchar_t* const*)L"ConEmu plugin\nUnknown version of packet", 0,0);
		}
		else
		{
			ShowConPacket(pReq);
		}
	}
	else if ((*(DWORD*)pszData) >= 0x200020)
	{
		ShowConDump(pszData);
	}

	InfoW1900->RestoreScreen(NULL);
	InfoW1900->RestoreScreen(h);
	InfoW1900->Text(0,0,0,0);
	//InfoW1900->Control(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);
	//InfoW1900->Control(PANEL_PASSIVE, FCTL_REDRAWPANEL, 0, 0);
	//INPUT_RECORD r = {WINDOW_BUFFER_SIZE_EVENT};
	//r.Event.WindowBufferSizeEvent.dwSize = csbi.dwSize;
	//WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &r, 1, &dwSizeLow);
	free(pszData);
	return INVALID_HANDLE_VALUE;
}
#endif
