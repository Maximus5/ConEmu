
/*
Copyright (c) 2009-2014 Maximus5
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
#include "../common/common.hpp"
#ifdef _DEBUG
#pragma warning( disable : 4995 )
#endif
#include "../common/pluginA.hpp"
#ifdef _DEBUG
#pragma warning( default : 4995 )
#endif
#include "../common/plugin_helper.h"
#include "PluginHeader.h"
#include "PluginBackground.h"
#include "../common/farcolor1.hpp"
#include "../common/MSection.h"

#include "ConEmuPluginA.h"

//#define SHOW_DEBUG_EVENTS


struct PluginStartupInfo *InfoA=NULL;
struct FarStandardFunctions *FSFA=NULL;

LPWSTR CPluginAnsi::ToUnicode(LPCSTR asOemStr)
{
	if (!asOemStr)
		return NULL;
	if (!*asOemStr)
		return lstrdup(L"");

	int nLen = lstrlenA(asOemStr);
	wchar_t* pszUnicode = (wchar_t*)calloc((nLen+1),sizeof(*pszUnicode));
	if (!pszUnicode)
		return NULL;

	MultiByteToWideChar(CP_OEMCP, 0, asOemStr, nLen, pszUnicode, nLen);
	return pszUnicode;
}

void CPluginAnsi::ToOem(LPCWSTR asUnicode, char* rsOem, INT_PTR cchOemMax)
{
	WideCharToMultiByte(CP_OEMCP, 0, asUnicode?asUnicode:L"", -1, rsOem, (int)cchOemMax, NULL, NULL);
}

wchar_t* CPluginAnsi::GetPanelDir(GetPanelDirFlags Flags)
{
	if (!InfoA)
		return NULL;

	wchar_t* pszDir = NULL;
	PanelInfo pi = {};
	InfoA->Control(INVALID_HANDLE_VALUE, (Flags & gpdf_Active) ? FCTL_GETPANELSHORTINFO : FCTL_GETANOTHERPANELSHORTINFO, &pi);

	if ((Flags & gpdf_NoHidden) && !pi.Visible)
		return NULL;
	if ((Flags & gpdf_NoPlugin) && pi.Plugin)
		return NULL;

	if (pi.CurDir[0])
		pszDir = ToUnicode(pi.CurDir);

	return pszDir;
}


HANDLE WINAPI _export OpenPlugin(int OpenFrom,INT_PTR Item)
{
	if (InfoA == NULL)
		return INVALID_HANDLE_VALUE;

#ifdef _DEBUG
	{
		wchar_t szInfo[128]; _wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"OpenPlugin[Ansi] (%i%s, Item=0x%X, gnReqCmd=%i%s)\n",
		                               OpenFrom, (OpenFrom==OPEN_COMMANDLINE) ? L"[OPEN_COMMANDLINE]" :
		                               (OpenFrom==OPEN_PLUGINSMENU) ? L"[OPEN_PLUGINSMENU]" : L"",
		                               (DWORD)Item,
		                               (int)gnReqCommand,
		                               (gnReqCommand == (DWORD)-1) ? L"" :
		                               (gnReqCommand == CMD_REDRAWFAR) ? L"[CMD_REDRAWFAR]" :
		                               (gnReqCommand == CMD_EMENU) ? L"[CMD_EMENU]" :
		                               (gnReqCommand == CMD_SETWINDOW) ? L"[CMD_SETWINDOW]" :
		                               (gnReqCommand == CMD_POSTMACRO) ? L"[CMD_POSTMACRO]" :
		                               L"");
		OutputDebugStringW(szInfo);
	}
#endif

	if (OpenFrom == OPEN_COMMANDLINE && Item)
	{
		wchar_t* pszUnicode = CPluginAnsi::ToUnicode((char*)Item);
		Plugin()->ProcessCommandLine(pszUnicode);
		SafeFree(pszUnicode);
		return INVALID_HANDLE_VALUE;
	}

	if (gnReqCommand != (DWORD)-1)
	{
		gnPluginOpenFrom = OpenFrom;
		SHOWDBGINFO(L"*** Calling ProcessCommand\n");
		ProcessCommand(gnReqCommand, FALSE/*bReqMainThread*/, gpReqCommandData);
	}
	else
	{
		SHOWDBGINFO(L"*** Calling ShowPluginMenu\n");
		Plugin()->ShowPluginMenu();
	}

	#ifdef _DEBUG
	if (gnReqCommand != (DWORD)-1)
	{
		wchar_t szInfo[128]; _wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"*** OpenPlugin[Ansi] post gnReqCmd=%i%s\n",
		                               (int)gnReqCommand,
		                               (gnReqCommand == (DWORD)-1) ? L"" :
		                               (gnReqCommand == CMD_REDRAWFAR) ? L"CMD_REDRAWFAR" :
		                               (gnReqCommand == CMD_EMENU) ? L"CMD_EMENU" :
		                               (gnReqCommand == CMD_SETWINDOW) ? L"CMD_SETWINDOW" :
		                               (gnReqCommand == CMD_POSTMACRO) ? L"CMD_POSTMACRO" :
		                               L"");
		OutputDebugStringW(szInfo);
	}
	#endif

	return INVALID_HANDLE_VALUE;
}


extern
VOID CALLBACK ConEmuCheckTimerProc(
    HWND hwnd,         // handle to window
    UINT uMsg,         // WM_TIMER message
    UINT_PTR idEvent,  // timer identifier
    DWORD dwTime       // current system time
);

void ProcessDragFromA()
{
	if (InfoA == NULL)
		return;

	WindowInfo WInfo;
	WInfo.Pos = 0;
	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
	InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);

	if (!WInfo.Current)
	{
		int ItemsCount=0;
		OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}

	PanelInfo PInfo = {};
	InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &PInfo);

	if ((PInfo.PanelType == PTYPE_FILEPANEL || PInfo.PanelType == PTYPE_TREEPANEL) && PInfo.Visible)
	{
		int nDirLen=0, nDirNoSlash=0;

		if (PInfo.CurDir && *PInfo.CurDir)
		{
			nDirLen=lstrlenA(PInfo.CurDir);

			if (nDirLen>0)
				if (PInfo.CurDir[nDirLen-1]!='\\')
					nDirNoSlash=1;
		}

		// Это только предполагаемый размер, при необходимости он будет увеличен
		OutDataAlloc(sizeof(int)+PInfo.SelectedItemsNumber*((MAX_PATH+2)+sizeof(int))); //-V119 //-V107
		//Maximus5 - новый формат передачи
		int nNull=0;
		//WriteFile(hPipe, &nNull/*ItemsCount*/, sizeof(int), &cout, NULL);
		OutDataWrite(&nNull/*ItemsCount*/, sizeof(int));

		if (PInfo.SelectedItemsNumber<=0)
		{
			// Проверка того, что мы стоим на ".."
			if (PInfo.CurrentItem == 0 && PInfo.ItemsNumber > 0)
			{
				if (!nDirNoSlash)
					PInfo.CurDir[nDirLen-1] = 0;
				else
					nDirLen++;

				WCHAR *szCurDir = new WCHAR[nDirLen];
				MultiByteToWideChar(CP_OEMCP, 0, PInfo.CurDir, nDirLen, szCurDir, nDirLen);
				int nWholeLen = nDirLen + 1;
				OutDataWrite(&nWholeLen, (DWORD)sizeof(int));
				OutDataWrite(&nDirLen, (DWORD)sizeof(int));
				OutDataWrite(szCurDir, (DWORD)sizeof(WCHAR)*nDirLen);
				delete [] szCurDir; szCurDir=NULL;
			}

			// Fin
			OutDataWrite(&nNull/*ItemsCount*/, sizeof(int));
		}
		else
		{
			int ItemsCount=PInfo.SelectedItemsNumber, i;
			bool *bIsFull = (bool*)calloc(PInfo.SelectedItemsNumber, sizeof(bool));
			int nMaxLen=MAX_PATH+1, nWholeLen=1;

			// сначала посчитать максимальную длину буфера
			for (i=0; i<ItemsCount; i++)
			{
				int nLen=nDirLen+nDirNoSlash;

				if ((PInfo.SelectedItems[i].FindData.cFileName[0] == '\\' && PInfo.SelectedItems[i].FindData.cFileName[1] == '\\') || //-V108
				        (ISALPHA(PInfo.SelectedItems[i].FindData.cFileName[0]) && PInfo.SelectedItems[i].FindData.cFileName[1] == ':' && PInfo.SelectedItems[i].FindData.cFileName[2] == '\\')) //-V108
					{ nLen = 0; bIsFull[i] = TRUE; } // это уже полный путь! //-V108

				nLen += lstrlenA(PInfo.SelectedItems[i].FindData.cFileName); //-V108

				if (nLen>nMaxLen)
					nMaxLen = nLen;

				nWholeLen += (nLen+1);
			}

			nMaxLen += nDirLen;

			//WriteFile(hPipe, &nWholeLen, sizeof(int), &cout, NULL);
			OutDataWrite(&nWholeLen, sizeof(int));
			WCHAR* Path = new WCHAR[nMaxLen+1];

			for (i=0; i<ItemsCount; i++)
			{
				if (i == 0
				        && ((PInfo.SelectedItems[i].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
				        && !lstrcmpA(PInfo.SelectedItems[i].FindData.cFileName, ".."))
				{
					continue;
				}

				//WCHAR Path[MAX_PATH+1];
				//ZeroMemory(Path, MAX_PATH+1);
				//Maximus5 - засада с корнем диска и возможностью overflow
				//StringCchPrintf(Path, countof(Path), L"%s\\%s", PInfo.CurDir, PInfo.SelectedItems[i]->FindData.cFileName);
				Path[0]=0;
				//begin
				int nLen=0;

				if (nDirLen>0 && !bIsFull[i])
				{
					MultiByteToWideChar(CP_OEMCP/*???*/,0,
					                    PInfo.CurDir, nDirLen+1, Path, nDirLen+1);

					if (nDirNoSlash)
					{
						Path[nDirLen]=L'\\';
						Path[nDirLen+1]=0;
					}

					nLen = nDirLen+nDirNoSlash;
				}

				int nFLen = lstrlenA(PInfo.SelectedItems[i].FindData.cFileName);
				MultiByteToWideChar(CP_OEMCP/*???*/, 0,
				                    PInfo.SelectedItems[i].FindData.cFileName,nFLen+1,Path+nLen,nFLen+1);
				nLen += nFLen;
				//end
				nLen++;
				//WriteFile(hPipe, &nLen, sizeof(int), &cout, NULL);
				OutDataWrite(&nLen, (DWORD)sizeof(int));
				//WriteFile(hPipe, Path, sizeof(WCHAR)*nLen, &cout, NULL);
				OutDataWrite(Path, (DWORD)sizeof(WCHAR)*nLen);
			}

			free(bIsFull);
			delete [] Path; Path=NULL;
			// Конец списка
			//WriteFile(hPipe, &nNull/*ItemsCount*/, sizeof(int), &cout, NULL);
			OutDataWrite(&nNull/*ItemsCount*/, sizeof(int));
		}
	}
	else
	{
		int ItemsCount=0;
		OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
	}
}
void ProcessDragToA()
{
	if (InfoA == NULL)
		return;

	WindowInfo WInfo = {};
	//WInfo.Pos = 0;
	WInfo.Pos = -1; // попробуем работать в диалогах и редакторе
	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
	InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);

	if (!WInfo.Current)
	{
		int ItemsCount=0;
		if (gpCmdRet==NULL)
			OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}

	int nStructSize;

	if ((WInfo.Type == WTYPE_DIALOG) || (WInfo.Type == WTYPE_EDITOR))
	{
		// разрешить дроп в виде текста
		ForwardedPanelInfo DlgInfo = {};
		DlgInfo.NoFarConsole = TRUE;
		nStructSize = sizeof(DlgInfo);
		if (gpCmdRet==NULL)
			OutDataAlloc(nStructSize+sizeof(nStructSize));
		OutDataWrite(&nStructSize, sizeof(nStructSize));
		OutDataWrite(&DlgInfo, nStructSize);
		return;
	}
	else if (WInfo.Type != WTYPE_PANELS)
	{
		// Иначе - дроп не разрешен
		int ItemsCount=0;
		if (gpCmdRet==NULL)
			OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}

	PanelInfo PAInfo = {}, PPInfo = {};
	ForwardedPanelInfo *pfpi = NULL;
	nStructSize = sizeof(ForwardedPanelInfo)+4; // потом увеличим на длину строк
	//ZeroMemory(&fpi, sizeof(fpi));
	BOOL lbAOK = FALSE, lbPOK = FALSE;

	//Maximus5 - к сожалению, В FAR2 FCTL_GETPANELSHORTINFO не возвращает CurDir :-(

	if (!(lbAOK=InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETPANELSHORTINFO, &PAInfo)))
		lbAOK=InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &PAInfo);

	if (lbAOK && PAInfo.CurDir)
		nStructSize += (lstrlenA(PAInfo.CurDir))*sizeof(WCHAR);

	if (!(lbPOK=InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETANOTHERPANELSHORTINFO, &PPInfo)))
		lbPOK=InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETANOTHERPANELINFO, &PPInfo);

	if (lbPOK && PPInfo.CurDir)
		nStructSize += (lstrlenA(PPInfo.CurDir))*sizeof(WCHAR); // Именно WCHAR! не TCHAR

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

		if (!PAInfo.Plugin && (PAInfo.PanelType == PTYPE_FILEPANEL || PAInfo.PanelType == PTYPE_TREEPANEL) && PAInfo.Visible)
		{
			if (PAInfo.CurDir != NULL)
			{
				int nLen = lstrlenA(PAInfo.CurDir)+1;
				MultiByteToWideChar(CP_OEMCP/*??? проверить*/,0,
				                    PAInfo.CurDir, nLen, pfpi->pszActivePath, nLen);
				//lstrcpyW(pfpi->pszActivePath, PAInfo.CurDir);
				pfpi->PassivePathShift += (nLen-1)*2;
			}
		}
	}

	pfpi->pszPassivePath = (WCHAR*)(((char*)pfpi)+pfpi->PassivePathShift);

	if (lbPOK)
	{
		pfpi->PassiveRect=PPInfo.PanelRect;

		if (!PPInfo.Plugin && (PPInfo.PanelType == PTYPE_FILEPANEL || PPInfo.PanelType == PTYPE_TREEPANEL) && PPInfo.Visible)
		{
			if (PPInfo.CurDir != NULL)
			{
				int nLen = lstrlenA(PPInfo.CurDir)+1;
				MultiByteToWideChar(CP_OEMCP/*??? проверить*/,0,
				                    PPInfo.CurDir, nLen, pfpi->pszPassivePath, nLen);
				//lstrcpyW(pfpi->pszPassivePath, PPInfo.CurDir);
			}
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


#if defined(__GNUC__)
#ifdef __cplusplus
extern "C" {
#endif
	void WINAPI SetStartupInfo(const struct PluginStartupInfo *aInfo);
#ifdef __cplusplus
};
#endif
#endif

void WINAPI _export SetStartupInfo(const struct PluginStartupInfo *aInfo)
{
	//LoadFarVersion - уже вызван в GetStartupInfo

	INIT_FAR_PSI(::InfoA, ::FSFA, aInfo);

	DWORD nFarVer = 0;
	if (InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETFARVERSION, &nFarVer))
	{
		if (HIBYTE(LOWORD(nFarVer)) == 1)
		{
			gFarVersion.dwBuild = HIWORD(nFarVer);
			gFarVersion.dwVerMajor = (HIBYTE(LOWORD(nFarVer)));
			gFarVersion.dwVerMinor = (LOBYTE(LOWORD(nFarVer)));
		}
		else
		{
			_ASSERTE(HIBYTE(HIWORD(nFarVer)) == 1);
		}
	}

	MultiByteToWideChar(CP_OEMCP,0,InfoA->RootKey,lstrlenA(InfoA->RootKey)+1,gszRootKey,countof(gszRootKey)-1);
	gszRootKey[countof(gszRootKey)-1] = 0;
	WCHAR* pszSlash = gszRootKey+lstrlenW(gszRootKey)-1;
	if (*pszSlash == L'\\') *(pszSlash--) = 0;
	while(pszSlash>gszRootKey && *pszSlash!=L'\\') pszSlash--;
	*pszSlash = 0;

	CommonPluginStartup();
	//// Устарело. активация через [Read/Peek]ConsoleInput
	////CheckMacro(TRUE);
	//
	//gbBgPluginsAllowed = TRUE;
	//if (gpConMapInfo) //2010-03-04 Имеет смысл только при запуске из-под ConEmu
	//{
	//	CheckResources(TRUE);
	//	LogCreateProcessCheck((LPCWSTR)-1);
	//}
}

//extern WCHAR gcPlugKey; // Для ANSI far он инициализируется как (char)

void WINAPI _export GetPluginInfo(struct PluginInfo *pi)
{
	_ASSERTE(pi->StructSize==0);
	pi->StructSize = sizeof(struct PluginInfo);
	//_ASSERTE(pi->StructSize>0 && (pi->StructSize >= sizeof(*pi)));

	static char *szMenu[1], szMenu1[255];
	szMenu[0]=szMenu1;
	// Устарело. активация через [Read/Peek]ConsoleInput
	//// Проверить, не изменилась ли горячая клавиша плагина, и если да - пересоздать макросы
	//IsKeyChanged(TRUE);
	//if (gcPlugKey) szMenu1[0]=0; else lstrcpyA(szMenu1, "[&\xDC] "); // а тут действительно OEM
	lstrcpynA(szMenu1/*+lstrlenA(szMenu1)*/, InfoA->GetMsg(InfoA->ModuleNumber,CEPluginName), 240);
	_ASSERTE(pi->StructSize == sizeof(struct PluginInfo));
	pi->Flags = PF_EDITOR | PF_VIEWER | PF_DIALOG | PF_PRELOAD;
	pi->DiskMenuStrings = NULL;
	pi->DiskMenuNumbers = 0;
	pi->PluginMenuStrings = szMenu;
	pi->PluginMenuStringsNumber = 1;
	pi->PluginConfigStrings = NULL;
	pi->PluginConfigStringsNumber = 0;
	pi->CommandPrefix = "ConEmu";
	pi->Reserved = 0;
}

DWORD CPluginAnsi::GetEditorModifiedState()
{
	EditorInfo ei;
	InfoA->EditorControl(ECTL_GETINFO, &ei);
#ifdef SHOW_DEBUG_EVENTS
	char szDbg[255];
	wsprintfA(szDbg, "Editor:State=%i\n", ei.CurState);
	OutputDebugStringA(szDbg);
#endif
	// Если он сохранен, то уже НЕ модифицирован
	DWORD currentModifiedState = ((ei.CurState & (ECSTATE_MODIFIED|ECSTATE_SAVED)) == ECSTATE_MODIFIED) ? 1 : 0;
	return currentModifiedState;
}

// watch non-modified -> modified editor status change
int WINAPI _export ProcessEditorInput(const INPUT_RECORD *Rec)
{
	if (/*!ghConEmuWndDC ||*/ !InfoA)  // иногда событие от QuickView приходит ДО инициализации плагина
		return 0; // Даже если мы не под эмулятором - просто запомним текущее состояние

	// only key events with virtual codes > 0 are likely to cause status change (?)

	if (!gbRequestUpdateTabs && (Rec->EventType & 0xFF) == KEY_EVENT
	        && (Rec->Event.KeyEvent.wVirtualKeyCode || Rec->Event.KeyEvent.wVirtualScanCode || Rec->Event.KeyEvent.uChar.AsciiChar)
	        && Rec->Event.KeyEvent.bKeyDown)
	{
#ifdef SHOW_DEBUG_EVENTS
		char szDbg[255]; wsprintfA(szDbg, "ProcessEditorInput(E=%i, VK=%i, SC=%i, CH=%i, Down=%i)\n", Rec->EventType, Rec->Event.KeyEvent.wVirtualKeyCode, Rec->Event.KeyEvent.wVirtualScanCode, Rec->Event.KeyEvent.uChar.AsciiChar, Rec->Event.KeyEvent.bKeyDown);
		OutputDebugStringA(szDbg);
#endif
		gbNeedPostEditCheck = TRUE;
	}

	return 0;
}

int WINAPI _export ProcessEditorEvent(int Event, void *Param)
{
	if (!gbRequestUpdateTabs)
	{
		if (Event == EE_READ || Event == EE_CLOSE || Event == EE_GOTFOCUS || Event == EE_KILLFOCUS || Event == EE_SAVE)
		{
			gbRequestUpdateTabs = TRUE;
			//} else if (Event == EE_REDRAW && gbHandleOneRedraw) {
			//	gbHandleOneRedraw = false; gbRequestUpdateTabs = TRUE;
		}
	}

	if (gpTabs && Event == EE_CLOSE && gpTabs->Tabs.nTabCount
	        && gpTabs->Tabs.tabs[0].Type != WTYPE_PANELS)
		gbClosingModalViewerEditor = TRUE;

	if (gpBgPlugin && (Event != EE_REDRAW))
	{
		gpBgPlugin->OnMainThreadActivated(Event, -1);
	}

	return 0;
}

int WINAPI _export ProcessViewerEvent(int Event, void *Param)
{
	if (!gbRequestUpdateTabs &&
	        (Event == VE_CLOSE || Event == VE_GOTFOCUS || Event == VE_KILLFOCUS))
		gbRequestUpdateTabs = TRUE;

	if (gpTabs && Event == VE_CLOSE && gpTabs->Tabs.nTabCount
	        && gpTabs->Tabs.tabs[0].Type != WTYPE_PANELS)
		gbClosingModalViewerEditor = TRUE;

	if (gpBgPlugin)
	{
		gpBgPlugin->OnMainThreadActivated(-1, Event);
	}

	return 0;
}

extern MSection *csTabs;

bool UpdateConEmuTabsA(int anEvent, bool losingFocus, bool editorSave, void *Param)
{
	if (!InfoA || gbIgnoreUpdateTabs)
		return false;

	if (ghConEmuWndDC && FarHwnd)
		CheckResources(FALSE);

	MSectionLock SC; SC.Lock(csTabs);
	BOOL lbCh = FALSE, lbDummy = FALSE;
	WindowInfo WInfo;
	WCHAR* pszName = gszDir1; pszName[0] = 0; //(WCHAR*)calloc(CONEMUTABMAX, sizeof(WCHAR));
	int windowCount = (int)InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWCOUNT, NULL);
	if ((windowCount == 0) && !gpFarInfo->bFarPanelAllowed)
	{
		windowCount = 1; lbDummy = TRUE;
	}
	lbCh = (lastWindowCount != windowCount);

	if (!CreateTabs(windowCount))
		return false;

	int tabCount = 0;

	if (lbDummy)
	{
		AddTab(tabCount, 0, false, false, WTYPE_PANELS, NULL, NULL, 1, 0, 0, 0);
		return (lbCh != FALSE);
	}

	//EditorInfo ei = {0}; BOOL bEditorRetrieved = FALSE;
	//WCHAR* pszFileName = NULL;
	//if (editorSave)
	//{
	//	InfoA->EditorControl(ECTL_GETINFO, &ei);
	//	bEditorRetrieved = TRUE;
	//	//pszFileName = (WCHAR*)calloc(CONEMUTABMAX, sizeof(WCHAR));
	//	pszFileName = gszDir2; pszFileName[0] = 0;
	//	if (ei.FileName)
	//		MultiByteToWideChar(CP_OEMCP, 0, ei.FileName, lstrlenA(ei.FileName)+1, pszFileName, CONEMUTABMAX);
	//}
	//ViewerInfo vi = {sizeof(ViewerInfo)};
	//if (anEvent == 206) {
	//	if (Param)
	//		vi.ViewerID = *(int*)Param;
	//	InfoA->ViewerControl(VCTL_GETINFO, &vi);
	//	pszFileName = gszDir2; pszFileName[0] = 0;
	//	if (vi.FileName)
	//		MultiByteToWideChar(CP_OEMCP, 0, vi.FileName, lstrlenA(vi.FileName)+1, pszFileName, CONEMUTABMAX);
	//}
	BOOL lbActiveFound = FALSE;

	for(int i = 0; i < windowCount; i++)
	{
		WInfo.Pos = i;
		_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
		InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);

		if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
		{
			InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);

			if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
			{
#ifdef SHOW_DEBUG_EVENTS
				char szDbg[255]; wsprintfA(szDbg, "Window %i (Type=%i, Modified=%i)\n", i, WInfo.Type, WInfo.Modified);
				OutputDebugStringA(szDbg);
#endif

				if (WInfo.Current) lbActiveFound = TRUE;

				MultiByteToWideChar(CP_OEMCP, 0, WInfo.Name, lstrlenA(WInfo.Name)+1, pszName, CONEMUTABMAX);
				TODO("Определение ИД редактора/вьювера");
				lbCh |= AddTab(tabCount, -1, losingFocus, editorSave,
				               WInfo.Type, pszName, /*editorSave ? pszFileName :*/ NULL,
				               WInfo.Current, WInfo.Modified, 0, 0);
				//if (WInfo.Type == WTYPE_EDITOR && WInfo.Current) //2009-08-17
				//	lastModifiedStateW = WInfo.Modified;
			}
		}
	}

	// Скорее всего это модальный редактор (или вьювер?)
	if (!lbActiveFound && !losingFocus)
	{
		WInfo.Pos = -1;
		_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
		InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);

		if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER)
		{
			WInfo.Pos = -1;
			InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);

			if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER)
			{
				tabCount = 0;
				MultiByteToWideChar(CP_OEMCP, 0, WInfo.Name, lstrlenA(WInfo.Name)+1, pszName, CONEMUTABMAX);
				TODO("Определение ИД редактора/вьювера");
				lbCh |= AddTab(tabCount, -1, losingFocus, editorSave,
				               WInfo.Type, pszName, /*editorSave ? pszFileName :*/ NULL,
				               WInfo.Current, WInfo.Modified, 0, 0);
			}
		}
		else if (WInfo.Type == WTYPE_PANELS)
		{
			gpTabs->Tabs.CurrentType = gnCurrentWindowType = WInfo.Type;
		}
	}

	// 101224 - сразу запомнить количество!
	gpTabs->Tabs.nTabCount = tabCount;
	//SendTabs(tabCount, lbCh && (gnReqCommand==(DWORD)-1));
	return (lbCh != FALSE);
}

int WINAPI _export ProcessDialogEvent(int Event, void *Param)
{
	return FALSE; // разрешение обработки фаром/другими плагинами
}

void   WINAPI _export ExitFAR(void)
{
	ShutdownPluginStep(L"ExitFAR");

	ExitFarCmn();
	//ShutdownHooks();
	//StopThread();

	if (InfoA)
	{
		free(InfoA);
		InfoA=NULL;
	}

	if (FSFA)
	{
		free(FSFA);
		FSFA=NULL;
	}

	gbExitFarCalled = TRUE;

	ShutdownPluginStep(L"ExitFAR - done");
}

int ShowMessageA(LPCSTR asMsg, int aiButtons, bool bWarning)
{
	if (!InfoA || !InfoA->Message)
		return -1;

	return InfoA->Message(InfoA->ModuleNumber,
                          FMSG_ALLINONE|(aiButtons?0:FMSG_MB_OK)|(bWarning ? FMSG_WARNING : 0), NULL,
	                     (const char * const *)asMsg, 0, aiButtons);
}

int ShowMessageA(int aiMsg, int aiButtons)
{
	if (!InfoA || !InfoA->Message)
		return -1;

	return ShowMessageA(InfoA->GetMsg(InfoA->ModuleNumber,aiMsg), aiButtons, true);
}

//void ReloadMacroA()
//{
//	if (!InfoA || !InfoA->AdvControl)
//		return;
//
//	ActlKeyMacro command;
//	command.Command=MCMD_LOADALL;
//	InfoA->AdvControl(InfoA->ModuleNumber,ACTL_KEYMACRO,&command);
//}

void CPluginAnsi::SetWindow(int nTab)
{
	if (!InfoA || !InfoA->AdvControl)
		return;

	if (InfoA->AdvControl(InfoA->ModuleNumber, ACTL_SETCURRENTWINDOW, (void*)nTab)) //-V204
		InfoA->AdvControl(InfoA->ModuleNumber, ACTL_COMMIT, 0);
}

// Warning, напрямую НЕ вызывать. Пользоваться "общей" PostMacro
void PostMacroA(char* asMacro, INPUT_RECORD* apRec)
{
	if (!InfoA || !InfoA->AdvControl) return;

	ActlKeyMacro mcr;
	mcr.Command = MCMD_POSTMACROSTRING;
	mcr.Param.PlainText.Flags = 0; // По умолчанию - вывод на экран разрешен

	while ((asMacro[0] == '@' || asMacro[0] == '^') && asMacro[1] && asMacro[1] != ' ')
	{
		switch (*asMacro)
		{
		case '@':
			mcr.Param.PlainText.Flags |= KSFLAGS_DISABLEOUTPUT;
			break;
		case '^':
			mcr.Param.PlainText.Flags |= KSFLAGS_NOSENDKEYSTOPLUGINS;
			break;
		}
		asMacro++;
	}

	mcr.Param.PlainText.SequenceText = asMacro;
	InfoA->AdvControl(InfoA->ModuleNumber, ACTL_KEYMACRO, (void*)&mcr);
}

int CPluginAnsi::ShowPluginMenu(ConEmuPluginMenuItem* apItems, int Count)
{
	if (!InfoA)
		return -1;

	//FarMenuItemEx items[] =
	//{
	//	{MIF_USETEXTPTR|(ghConEmuWndDC ? MIF_SELECTED : MIF_DISABLE)},
	//	{MIF_USETEXTPTR|(ghConEmuWndDC ? 0 : MIF_DISABLE)},
	//	{MIF_SEPARATOR},
	//	{MIF_USETEXTPTR|(ghConEmuWndDC ? 0 : MIF_DISABLE)},
	//	{MIF_USETEXTPTR|(ghConEmuWndDC ? 0 : MIF_DISABLE)},
	//	{MIF_USETEXTPTR|(ghConEmuWndDC ? 0 : MIF_DISABLE)},
	//	{MIF_USETEXTPTR|(ghConEmuWndDC ? 0 : MIF_DISABLE)},
	//	{MIF_SEPARATOR},
	//	{MIF_USETEXTPTR|0},
	//	{MIF_SEPARATOR},
	//	{MIF_USETEXTPTR|(ghConEmuWndDC||IsTerminalMode() ? MIF_DISABLE : MIF_SELECTED)},
	//	{MIF_SEPARATOR},
	//	//#ifdef _DEBUG
	//	//		{MIF_USETEXTPTR},
	//	//#endif
	//	{MIF_USETEXTPTR|(IsDebuggerPresent()||IsTerminalMode() ? MIF_DISABLE : 0)}
	//};
	//items[0].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuEditOutput);
	//items[1].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuViewOutput);
	//items[3].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuShowHideTabs);
	//items[4].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuNextTab);
	//items[5].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuPrevTab);
	//items[6].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuCommitTab);
	//items[8].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuGuiMacro);
	//items[10].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuAttach);
	////#ifdef _DEBUG
	////items[10].Text.TextPtr = "&~. Raise exception";
	////items[11].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuDebug);
	////#else
	//items[12].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuDebug);
	////#endif
	//int nCount = sizeof(items)/sizeof(items[0]);

	FarMenuItemEx* items = (FarMenuItemEx*)calloc(Count, sizeof(*items));
	for (int i = 0; i < Count; i++)
	{
		if (apItems[i].Separator)
		{
			items[i].Flags = MIF_SEPARATOR;
			continue;
		}
		items[i].Flags	= (apItems[i].Disabled ? MIF_DISABLE : 0)
						| (apItems[i].Selected ? MIF_SELECTED : 0)
						| (apItems[i].Checked  ? MIF_CHECKED : 0)
						;
		if (apItems[i].MsgText)
		{
			WideCharToMultiByte(CP_OEMCP, 0, apItems[i].MsgText, -1, items[i].Text.Text, countof(items[i].Text.Text), 0,0);
		}
		else
		{
			items[i].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber, apItems[i].MsgID);
			items[i].Flags |= MIF_USETEXTPTR;
		}
	}

	int nRc = InfoA->Menu(InfoA->ModuleNumber, -1,-1, 0,
	                      FMENU_USEEXT|FMENU_AUTOHIGHLIGHT|FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
	                      InfoA->GetMsg(InfoA->ModuleNumber,CEPluginName),
	                      NULL, NULL, NULL, NULL, (FarMenuItem*)items, Count);
	SafeFree(items);
	return nRc;
}

bool CPluginAnsi::OpenEditor(LPCWSTR asFileName, bool abView, bool abDeleteTempFile, bool abDetectCP /*= false*/, int anStartLine /*= 0*/, int anStartChar /*= 1*/)
{
	if (!InfoA)
		return false;

	bool lbRc;
	int iRc;

	char szFileName[MAX_PATH] = "";
	ToOem(asFileName, szFileName, countof(szFileName));
	LPCSTR pszTitle = abDeleteTempFile ? InfoA->GetMsg(InfoA->ModuleNumber,CEConsoleOutput) : NULL;

	if (!abView)
	{
		iRc = InfoA->Editor(szFileName, pszTitle, 0,0,-1,-1,
		                     EF_NONMODAL|EF_IMMEDIATERETURN
		                     |(abDeleteTempFile ? (EF_DELETEONLYFILEONCLOSE|EF_DISABLEHISTORY) : 0)
		                     |EF_ENABLE_F6,
		                     anStartLine, anStartChar);
		lbRc = (iRc != EEC_OPEN_ERROR);
	}
	else
	{
		iRc = InfoA->Viewer(szFileName, pszTitle, 0,0,-1,-1,
		                     VF_NONMODAL|VF_IMMEDIATERETURN
		                     |(abDeleteTempFile ? (VF_DELETEONLYFILEONCLOSE|VF_DISABLEHISTORY) : 0)
		                     |VF_ENABLE_F6);
		lbRc = (iRc != 0);
	}

	return lbRc;
}


void GetMsgA(int aiMsg, wchar_t (&rsMsg)[MAX_PATH])
{
	if (!rsMsg || !InfoA)
		return;

	LPCSTR pszMsg = InfoA->GetMsg(InfoA->ModuleNumber,aiMsg);

	if (pszMsg && *pszMsg)
	{
		int nLen = (int)lstrlenA(pszMsg);

		if (nLen>=MAX_PATH) nLen = MAX_PATH - 1;

		nLen = MultiByteToWideChar(CP_OEMCP, 0, pszMsg, nLen, rsMsg, MAX_PATH-1);

		if (nLen>=0) rsMsg[nLen] = 0;
	}
	else
	{
		rsMsg[0] = 0;
	}
}

bool CPluginAnsi::IsMacroActive()
{
	if (!InfoA || !FarHwnd)
		return false;

	ActlKeyMacro akm = {MCMD_GETSTATE};
	INT_PTR liRc = InfoA->AdvControl(InfoA->ModuleNumber, ACTL_KEYMACRO, &akm);

	if (liRc == MACROSTATE_NOMACRO)
		return false;

	return true;
}

int CPluginAnsi::GetMacroArea()
{
	return 1; // в Far 1.7x не поддерживается
}

void CPluginAnsi::RedrawAll()
{
	if (!InfoA || !FarHwnd)
		return;

	InfoA->AdvControl(InfoA->ModuleNumber, ACTL_REDRAWALL, NULL);
}

bool CPluginAnsi::InputBox(LPCWSTR Title, LPCWSTR SubTitle, LPCWSTR HistoryName, LPCWSTR SrcText, wchar_t*& DestText)
{
	_ASSERTE(DestText==NULL);
	if (!InfoA)
		return false;

	char strTemp[MAX_PATH+1] = "", aTitle[64] = "", aSubTitle[128] = "", aHistoryName[64] = "", aSrcText[128];
	ToOem(Title, aTitle, countof(aTitle));
	ToOem(SubTitle, aSubTitle, countof(aSubTitle));
	ToOem(HistoryName, aHistoryName, countof(aHistoryName));
	ToOem(SrcText, aSrcText, countof(aSrcText));

	if (!InfoA->InputBox(aTitle, aSubTitle, aHistoryName, aSrcText, strTemp, countof(strTemp), NULL, FIB_BUTTONS))
		return false;
	DestText = ToUnicode(strTemp);
	return true;
}

void CPluginAnsi::ShowUserScreen(bool bUserScreen)
{
	if (!InfoA)
		return;

	if (bUserScreen)
		InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETUSERSCREEN, 0);
	else
		InfoA->Control(INVALID_HANDLE_VALUE, FCTL_SETUSERSCREEN, 0);
}


//static void FarPanel2CePanel(PanelInfo* pFar, CEFAR_SHORT_PANEL_INFO* pCE)
//{
//	pCE->PanelType = pFar->PanelType;
//	pCE->Plugin = pFar->Plugin;
//	pCE->PanelRect = pFar->PanelRect;
//	pCE->ItemsNumber = pFar->ItemsNumber;
//	pCE->SelectedItemsNumber = pFar->SelectedItemsNumber;
//	pCE->CurrentItem = pFar->CurrentItem;
//	pCE->TopPanelItem = pFar->TopPanelItem;
//	pCE->Visible = pFar->Visible;
//	pCE->Focus = pFar->Focus;
//	pCE->ViewMode = pFar->ViewMode;
//	pCE->ShortNames = pFar->ShortNames;
//	pCE->SortMode = pFar->SortMode;
//	pCE->Flags = pFar->Flags;
//}

void LoadFarColorsA(BYTE (&nFarColors)[col_LastIndex])
{
	BYTE FarConsoleColors[0x100];
	INT_PTR nColorSize = InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETARRAYCOLOR, FarConsoleColors);
#ifdef _DEBUG
	INT_PTR nDefColorSize = COL_LASTPALETTECOLOR;
	_ASSERTE(nColorSize==nDefColorSize);
#endif
	UNREFERENCED_PARAMETER(nColorSize);
	nFarColors[col_PanelText] = FarConsoleColors[COL_PANELTEXT];
	nFarColors[col_PanelSelectedCursor] = FarConsoleColors[COL_PANELSELECTEDCURSOR];
	nFarColors[col_PanelSelectedText] = FarConsoleColors[COL_PANELSELECTEDTEXT];
	nFarColors[col_PanelCursor] = FarConsoleColors[COL_PANELCURSOR];
	nFarColors[col_PanelColumnTitle] = FarConsoleColors[COL_PANELCOLUMNTITLE];
	nFarColors[col_PanelBox] = FarConsoleColors[COL_PANELBOX];
	nFarColors[col_HMenuText] = FarConsoleColors[COL_HMENUTEXT];
	nFarColors[col_WarnDialogBox] = FarConsoleColors[COL_WARNDIALOGBOX];
	nFarColors[col_DialogBox] = FarConsoleColors[COL_DIALOGBOX];
	nFarColors[col_CommandLineUserScreen] = FarConsoleColors[COL_COMMANDLINEUSERSCREEN];
	nFarColors[col_PanelScreensNumber] = FarConsoleColors[COL_PANELSCREENSNUMBER];
	nFarColors[col_KeyBarNum] = FarConsoleColors[COL_KEYBARNUM];
}

static void LoadFarSettingsA(CEFarInterfaceSettings* pInterface, CEFarPanelSettings* pPanel)
{
	DWORD nSet;

	nSet = (DWORD)InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETINTERFACESETTINGS, 0);
	if (pInterface)
	{
		pInterface->Raw = nSet;
		_ASSERTE((pInterface->AlwaysShowMenuBar != 0) == ((nSet & FIS_ALWAYSSHOWMENUBAR) != 0));
		_ASSERTE((pInterface->ShowKeyBar != 0) == ((nSet & FIS_SHOWKEYBAR) != 0));
	}

	nSet = (DWORD)InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETPANELSETTINGS, 0);
	if (pPanel)
	{
		pPanel->Raw = nSet;
		_ASSERTE((pPanel->ShowColumnTitles != 0) == ((nSet & FPS_SHOWCOLUMNTITLES) != 0));
		_ASSERTE((pPanel->ShowStatusLine != 0) == ((nSet & FPS_SHOWSTATUSLINE) != 0));
		_ASSERTE((pPanel->ShowSortModeLetter != 0) == ((nSet & FPS_SHOWSORTMODELETTER) != 0));
	}
}

BOOL ReloadFarInfoA(/*BOOL abFull*/)
{
	if (!InfoA || !FSFA) return FALSE;

	if (!gpFarInfo)
	{
		_ASSERTE(gpFarInfo!=NULL);
		return FALSE;
	}

	// Заполнить gpFarInfo->
	//BYTE nFarColors[col_LastIndex]; // Массив цветов фара
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
			wchar_t szDbg[128]; _wsprintf(szDbg, SKIPLEN(countof(szDbg)) L"Far.ConEmuA: ConsoleMode(STD_INPUT_HANDLE)=0x%08X\n", ldwConsoleMode);
			OutputDebugStringW(szDbg);
			ldwDbgMode = ldwConsoleMode;
		}
	}

#endif
	gpFarInfo->nFarConsoleMode = ldwConsoleMode;

	LoadFarColorsA(gpFarInfo->nFarColors);

	//_ASSERTE(FPS_SHOWCOLUMNTITLES==0x20 && FPS_SHOWSTATUSLINE==0x40); //-V112
	LoadFarSettingsA(&gpFarInfo->FarInterfaceSettings, &gpFarInfo->FarPanelSettings);

	//gpFarInfo->nFarConfirmationSettings =
	//    (DWORD)InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETCONFIRMATIONS, 0);

	gpFarInfo->bMacroActive = IsMacroActive();
	gpFarInfo->nMacroArea = fma_Unknown; // в Far 1.7x не поддерживается

	gpFarInfo->bFarPanelAllowed = InfoA->Control(INVALID_HANDLE_VALUE, FCTL_CHECKPANELSEXIST, 0);
	gpFarInfo->bFarPanelInfoFilled = FALSE;
	gpFarInfo->bFarLeftPanel = FALSE;
	gpFarInfo->bFarRightPanel = FALSE;
	// -- пока, во избежание глюков в FAR при неожиданных запросах информации о панелях
	//if (FALSE == (gpFarInfo->bFarPanelAllowed)) {
	//	gpConMapInfo->bFarLeftPanel = FALSE;
	//	gpConMapInfo->bFarRightPanel = FALSE;
	//} else {
	//	PanelInfo piA = {}, piP = {};
	//	BOOL lbActive  = InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETPANELSHORTINFO, &piA);
	//	BOOL lbPassive = InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETANOTHERPANELSHORTINFO, &piP);
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

static void CopyPanelInfo(PanelInfo* pInfo, PaintBackgroundArg::BkPanelInfo* pBk)
{
	pBk->bVisible = pInfo->Visible;
	pBk->bFocused = pInfo->Focus;
	pBk->bPlugin = pInfo->Plugin;
	pBk->nPanelType = pInfo->PanelType;
	MultiByteToWideChar(CP_OEMCP, 0, pInfo->CurDir, -1, pBk->szCurDir, BkPanelInfo_CurDirMax);
	lstrcpyW(pBk->szFormat, pInfo->Plugin ? L"Plugin" : L"");
	pBk->szHostFile[0] = 0;
	pBk->rcPanelRect = pInfo->PanelRect;
}

void FillUpdateBackgroundA(struct PaintBackgroundArg* pFar)
{
	if (!InfoA || !InfoA->AdvControl)
		return;

	LoadFarColorsA(pFar->nFarColors);

	LoadFarSettingsA(&pFar->FarInterfaceSettings, &pFar->FarPanelSettings);

	pFar->bPanelsAllowed = (0 != InfoA->Control(INVALID_HANDLE_VALUE, FCTL_CHECKPANELSEXIST, 0));

	if (pFar->bPanelsAllowed)
	{
		PanelInfo pasv = {}, actv = {};
		InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETPANELSHORTINFO, &actv);
		InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETANOTHERPANELSHORTINFO, &pasv);
		PanelInfo* pLeft = (actv.Flags & PFLAGS_PANELLEFT) ? &actv : &pasv;
		PanelInfo* pRight = (actv.Flags & PFLAGS_PANELLEFT) ? &pasv : &actv;
		CopyPanelInfo(pLeft, &pFar->LeftPanel);
		CopyPanelInfo(pRight, &pFar->RightPanel);
	}

	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO scbi = {};
	GetConsoleScreenBufferInfo(hCon, &scbi);
	pFar->rcConWorkspace.left = pFar->rcConWorkspace.top = 0;
	pFar->rcConWorkspace.right = scbi.dwSize.X - 1;
	pFar->rcConWorkspace.bottom = scbi.dwSize.Y - 1;
	//pFar->conSize = scbi.dwSize;
	pFar->conCursor = scbi.dwCursorPosition;
	CONSOLE_CURSOR_INFO crsr = {0};
	GetConsoleCursorInfo(hCon, &crsr);

	if (!crsr.bVisible || crsr.dwSize == 0)
	{
		pFar->conCursor.X = pFar->conCursor.Y = -1;
	}
}

int CPluginAnsi::GetActiveWindowType()
{
	if (!InfoA || !InfoA->AdvControl)
		return -1;

	WindowInfo WInfo = {-1};
	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
	InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);
	return WInfo.Type;
}

LPCWSTR CPluginAnsi::GetWindowTypeName(int WindowType)
{
	LPCWSTR pszCurType;
	switch (WindowType)
	{
		case WTYPE_PANELS: pszCurType = L"WTYPE_PANELS"; break;
		case WTYPE_VIEWER: pszCurType = L"WTYPE_VIEWER"; break;
		case WTYPE_EDITOR: pszCurType = L"WTYPE_EDITOR"; break;
		case WTYPE_DIALOG: pszCurType = L"WTYPE_DIALOG"; break;
		case WTYPE_VMENU:  pszCurType = L"WTYPE_VMENU"; break;
		case WTYPE_HELP:   pszCurType = L"WTYPE_HELP"; break;
		default:           pszCurType = L"Unknown";
	}
	return pszCurType;
}

#undef FAR_UNICODE
#include "Dialogs.h"
void GuiMacroDlgA()
{
	CallGuiMacroProc();
}
