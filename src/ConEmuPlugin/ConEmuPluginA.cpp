#include <windows.h>
#include "..\common\common.hpp"
#include "..\common\pluginA.hpp"
#include "PluginHeader.h"

//#define SHOW_DEBUG_EVENTS

// minimal(?) FAR version 1.71 alpha 4 build 2470
int WINAPI _export GetMinFarVersion(void)
{
    // ќднако, FAR2 до сборки 748 не понимал две версии плагина в одном файле
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

	if (gnReqCommand != (DWORD)-1) {
		gnPluginOpenFrom = OpenFrom;
		ProcessCommand(gnReqCommand, FALSE/*bReqMainThread*/, gpReqCommandData);
	} else {
		if (!gbCmdCallObsolete)
			ShowPluginMenu();
		else
			gbCmdCallObsolete = FALSE;
	}
	return INVALID_HANDLE_VALUE;
}


extern 
VOID CALLBACK ConEmuCheckTimerProc(
  HWND hwnd,         // handle to window
  UINT uMsg,         // WM_TIMER message
  UINT_PTR idEvent,  // timer identifier
  DWORD dwTime       // current system time
);

void UpdateConEmuTabsA(int event, bool losingFocus, bool editorSave, void *Param=NULL);

void ProcessDragFromA()
{
	if (InfoA == NULL)
		return;

	WindowInfo WInfo;
    WInfo.Pos=0;
	InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);
	if (!WInfo.Current)
	{
		int ItemsCount=0;
		OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}

	PanelInfo PInfo;
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

		// Ёто только предполагаемый размер, при необходимости он будет увеличен
		OutDataAlloc(sizeof(int)+PInfo.SelectedItemsNumber*((MAX_PATH+2)+sizeof(int)));

		//Maximus5 - новый формат передачи
		int nNull=0;
		//WriteFile(hPipe, &nNull/*ItemsCount*/, sizeof(int), &cout, NULL);
		OutDataWrite(&nNull/*ItemsCount*/, sizeof(int));
		
		if (PInfo.SelectedItemsNumber>0)
		{
			int ItemsCount=PInfo.SelectedItemsNumber, i;
			bool *bIsFull = (bool*)calloc(PInfo.SelectedItemsNumber, sizeof(bool));

			int nMaxLen=MAX_PATH+1, nWholeLen=1;

			// сначала посчитать максимальную длину буфера
			for (i=0;i<ItemsCount;i++)
			{
				int nLen=nDirLen+nDirNoSlash;
				if ((PInfo.SelectedItems[i].FindData.cFileName[0] == '\\' && PInfo.SelectedItems[i].FindData.cFileName[1] == '\\') ||
				    (ISALPHA(PInfo.SelectedItems[i].FindData.cFileName[0]) && PInfo.SelectedItems[i].FindData.cFileName[1] == ':' && PInfo.SelectedItems[i].FindData.cFileName[2] == '\\'))
				    { nLen = 0; bIsFull[i] = TRUE; } // это уже полный путь!
				nLen += lstrlenA(PInfo.SelectedItems[i].FindData.cFileName);
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
				//wsprintf(Path, L"%s\\%s", PInfo.CurDir, PInfo.SelectedItems[i]->FindData.cFileName);
				Path[0]=0;

				//begin	
				int nLen=0;
				if (nDirLen>0 && !bIsFull[i]) {
					MultiByteToWideChar(CP_OEMCP/*???*/,0,
						PInfo.CurDir, nDirLen+1, Path, nDirLen+1);
					if (nDirNoSlash) {
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
				OutDataWrite(&nLen, sizeof(int));
				//WriteFile(hPipe, Path, sizeof(WCHAR)*nLen, &cout, NULL);
				OutDataWrite(Path, sizeof(WCHAR)*nLen);
			}

			free(bIsFull);
			delete [] Path; Path=NULL;

			//  онец списка
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

	WindowInfo WInfo;				
    WInfo.Pos=0;
	InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);
	if (!WInfo.Current)
	{
		int ItemsCount=0;
		//WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);				
		if (gpCmdRet==NULL)
			OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}
	PanelInfo PAInfo, PPInfo;
	ForwardedPanelInfo *pfpi=NULL;
	int nStructSize = sizeof(ForwardedPanelInfo)+4; // потом увеличим на длину строк
	//ZeroMemory(&fpi, sizeof(fpi));
	BOOL lbAOK=FALSE, lbPOK=FALSE;
	
    //Maximus5 - к сожалению, ¬ FAR2 FCTL_GETPANELSHORTINFO не возвращает CurDir :-(

    if (!(lbAOK=InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETPANELSHORTINFO, &PAInfo)))
        lbAOK=InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &PAInfo);
	if (lbAOK && PAInfo.CurDir)
		nStructSize += (lstrlenA(PAInfo.CurDir))*sizeof(WCHAR);

    if (!(lbPOK=InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETANOTHERPANELSHORTINFO, &PPInfo)))
        lbPOK=InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETANOTHERPANELINFO, &PPInfo);
	if (lbPOK && PPInfo.CurDir)
		nStructSize += (lstrlenA(PPInfo.CurDir))*sizeof(WCHAR); // »менно WCHAR! не TCHAR

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

	pfpi->PassivePathShift = pfpi->ActivePathShift+2; // если ActivePath заполнитс€ - увеличим

	if (lbAOK)
	{
		pfpi->ActiveRect=PAInfo.PanelRect;
		if (!PAInfo.Plugin && (PAInfo.PanelType == PTYPE_FILEPANEL || PAInfo.PanelType == PTYPE_TREEPANEL) && PAInfo.Visible)
		{
			if (PAInfo.CurDir != NULL) {
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

	// —обственно, пересылка информации
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

	CheckMacro(TRUE);

	CheckResources();
}

extern WCHAR gcPlugKey; // ƒл€ ANSI far он инициализируетс€ как (char)

void WINAPI _export GetPluginInfo(struct PluginInfo *pi)
{
    static char *szMenu[1], szMenu1[255];
	szMenu[0]=szMenu1;

	// ѕроверить, не изменилась ли гор€ча€ клавиша плагина, и если да - пересоздать макросы
	IsKeyChanged(TRUE);

	if (gcPlugKey) szMenu1[0]=0; else lstrcpyA(szMenu1, "[&\xDC] "); // а тут действительно OEM
	lstrcpynA(szMenu1+lstrlenA(szMenu1), InfoA->GetMsg(InfoA->ModuleNumber,2), 240);

	pi->StructSize = sizeof(struct PluginInfo);
	pi->Flags = PF_EDITOR | PF_VIEWER | PF_DIALOG | PF_PRELOAD;
	pi->DiskMenuStrings = NULL;
	pi->DiskMenuNumbers = 0;
	pi->PluginMenuStrings = szMenu;
	pi->PluginMenuStringsNumber = 1;
	pi->PluginConfigStrings = NULL;
	pi->PluginConfigStringsNumber = 0;
	pi->CommandPrefix = NULL;
	pi->Reserved = 0;	
}

// watch non-modified -> modified editor status change
int WINAPI _export ProcessEditorInput(const INPUT_RECORD *Rec)
{
	if (/*!ConEmuHwnd ||*/ !InfoA) // иногда событие от QuickView приходит ƒќ инициализации плагина
		return 0; // ƒаже если мы не под эмул€тором - просто запомним текущее состо€ние
	// only key events with virtual codes > 0 are likely to cause status change (?)



	if ((Rec->EventType & 0xFF) == KEY_EVENT 
		&& (Rec->Event.KeyEvent.wVirtualKeyCode || Rec->Event.KeyEvent.wVirtualScanCode || Rec->Event.KeyEvent.uChar.AsciiChar)
		&& Rec->Event.KeyEvent.bKeyDown)
	{
#ifdef SHOW_DEBUG_EVENTS
		char szDbg[255]; wsprintfA(szDbg, "ProcessEditorInput(E=%i, VK=%i, SC=%i, CH=%i, Down=%i)\n", 
			Rec->EventType, Rec->Event.KeyEvent.wVirtualKeyCode, 
			Rec->Event.KeyEvent.wVirtualScanCode, Rec->Event.KeyEvent.uChar.AsciiChar,
			Rec->Event.KeyEvent.bKeyDown);
		OutputDebugStringA(szDbg);
#endif

		EditorInfo ei;
		InfoA->EditorControl(ECTL_GETINFO, &ei);
#ifdef SHOW_DEBUG_EVENTS
		wsprintfA(szDbg, "Editor:State=%i\n", ei.CurState);
		OutputDebugStringA(szDbg);
#endif
		int currentModifiedState = ei.CurState == ECSTATE_MODIFIED ? 1 : 0;
		if (lastModifiedStateW != currentModifiedState)
		{
			UpdateConEmuTabsA(0, false, false);
			lastModifiedStateW = currentModifiedState;
		} else {
			gbHandleOneRedraw = true;
		}
	}
	return 0;
}

int WINAPI _export ProcessEditorEvent(int Event, void *Param)
{
	if (/*!ConEmuHwnd ||*/ !InfoA) // иногда событие от QuickView приходит ƒќ инициализации плагина
		return 0; // ƒаже если мы не под эмул€тором - просто запомним текущее состо€ние
	switch (Event)
	{
	case EE_READ: // в этот момент количество окон еще не изменилось
		gbHandleOneRedraw = true;
		return 0;
	case EE_REDRAW:
		if (!gbHandleOneRedraw)
			return 0;
		gbHandleOneRedraw = false;
		OUTPUTDEBUGSTRING(L"EE_REDRAW(first)\n");
		break;
	case EE_CLOSE:
	case EE_GOTFOCUS:
	case EE_KILLFOCUS:
	case EE_SAVE:
		break;
	default:
		return 0;
	}
	UpdateConEmuTabsA(Event+100, Event == EE_KILLFOCUS, Event == EE_SAVE);
	return 0;
}

int WINAPI _export ProcessViewerEvent(int Event, void *Param)
{
	if (/*!ConEmuHwnd ||*/ !InfoA) // иногда событие от QuickView приходит ƒќ инициализации плагина
		return 0; // ƒаже если мы не под эмул€тором - просто запомним текущее состо€ние
	switch (Event)
	{
	case VE_CLOSE:
	//case VE_READ:
	case VE_KILLFOCUS:
	case VE_GOTFOCUS:
		{
			UpdateConEmuTabsA(Event+200, Event == VE_KILLFOCUS, false, Param);
		}
	}

	return 0;
}

void UpdateConEmuTabsA(int event, bool losingFocus, bool editorSave, void *Param/*=NULL*/)
{
	if (!InfoA) return;

	CheckResources();

    BOOL lbCh = FALSE;
	WindowInfo WInfo;
	WCHAR* pszName = gszDir1; pszName[0] = 0; //(WCHAR*)calloc(CONEMUTABMAX, sizeof(WCHAR));

	int windowCount = (int)InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWCOUNT, NULL);
	lbCh = (lastWindowCount != windowCount);
	
	if (!CreateTabs ( windowCount ))
		return;

	EditorInfo ei;
	WCHAR* pszFileName = NULL;
	if (editorSave)
	{
		InfoA->EditorControl(ECTL_GETINFO, &ei);
		//pszFileName = (WCHAR*)calloc(CONEMUTABMAX, sizeof(WCHAR));
		pszFileName = gszDir2; pszFileName[0] = 0;
		if (ei.FileName)
			MultiByteToWideChar(CP_OEMCP, 0, ei.FileName, lstrlenA(ei.FileName)+1, pszFileName, CONEMUTABMAX);
	}

	ViewerInfo vi = {sizeof(ViewerInfo)};
	if (event == 206) {
		if (Param)
			vi.ViewerID = *(int*)Param;
		InfoA->ViewerControl(VCTL_GETINFO, &vi);
		pszFileName = gszDir2; pszFileName[0] = 0;
		if (vi.FileName)
			MultiByteToWideChar(CP_OEMCP, 0, vi.FileName, lstrlenA(vi.FileName)+1, pszFileName, CONEMUTABMAX);
	}

	int tabCount = 0;
	for (int i = 0; i < windowCount; i++)
	{
		WInfo.Pos = i;
		InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);
		if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
		{
#ifdef SHOW_DEBUG_EVENTS
			char szDbg[255]; wsprintfA(szDbg, "Window %i (Type=%i, Modified=%i)\n", i, WInfo.Type, WInfo.Modified);
			OutputDebugStringA(szDbg);
#endif
			MultiByteToWideChar(CP_OEMCP, 0, WInfo.Name, lstrlenA(WInfo.Name)+1, pszName, CONEMUTABMAX);
			lbCh |= AddTab(tabCount, losingFocus, editorSave, 
				WInfo.Type, pszName, editorSave ? pszFileName : NULL, 
				WInfo.Current, WInfo.Modified);
		}
	}

	// Viewer в FAR 2 build 9xx не попадает в список окон при событии VE_GOTFOCUS
	if (!losingFocus && !editorSave && tabCount == 0 && event == 206) {
		lbCh |= AddTab(tabCount, losingFocus, editorSave, 
			WTYPE_VIEWER, pszFileName, NULL, 
			1, 0);
	}

	SendTabs(tabCount, FALSE, lbCh);
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

int ShowMessageA(int aiMsg, int aiButtons)
{
	if (!InfoA || !InfoA->Message)
		return -1;
	return InfoA->Message(InfoA->ModuleNumber, FMSG_ALLINONE, NULL, 
		(const char * const *)InfoA->GetMsg(InfoA->ModuleNumber,aiMsg), 0, aiButtons);
}

void ReloadMacroA()
{
	if (!InfoA || !InfoA->AdvControl)
		return;

	ActlKeyMacro command;
	command.Command=MCMD_LOADALL;
	InfoA->AdvControl(InfoA->ModuleNumber,ACTL_KEYMACRO,&command);
}

void SetWindowA(int nTab)
{
	if (!InfoA || !InfoA->AdvControl)
		return;

	if (InfoA->AdvControl(InfoA->ModuleNumber, ACTL_SETCURRENTWINDOW, (void*)nTab))
		InfoA->AdvControl(InfoA->ModuleNumber, ACTL_COMMIT, 0);
}

void PostMacroA(char* asMacro)
{
	if (!InfoA || !InfoA->AdvControl) return;

	ActlKeyMacro mcr;
	mcr.Command = MCMD_POSTMACROSTRING;
	mcr.Param.PlainText.SequenceText = asMacro;
	mcr.Param.PlainText.Flags = KSFLAGS_DISABLEOUTPUT;
	InfoA->AdvControl(InfoA->ModuleNumber, ACTL_KEYMACRO, (void*)&mcr);
}

int ShowPluginMenuA()
{
	if (!InfoA)
		return -1;

	FarMenuItem items[] = {
		{"", 1, 0, 0},
		{"", 0, 0, 0},
		{"", 0, 0, 1},
		{"", 0, 0, 0},
		{"", 0, 0, 0},
		{"", 0, 0, 0},
		{"", 0, 0, 0}
	};
	lstrcpyA(items[0].Text, InfoA->GetMsg(InfoA->ModuleNumber,3));
	lstrcpyA(items[1].Text, InfoA->GetMsg(InfoA->ModuleNumber,4));
	lstrcpyA(items[3].Text, InfoA->GetMsg(InfoA->ModuleNumber,6));
	lstrcpyA(items[4].Text, InfoA->GetMsg(InfoA->ModuleNumber,7));
	lstrcpyA(items[5].Text, InfoA->GetMsg(InfoA->ModuleNumber,8));
	lstrcpyA(items[6].Text, InfoA->GetMsg(InfoA->ModuleNumber,9));
	int nCount = sizeof(items)/sizeof(items[0]);

	int nRc = InfoA->Menu(InfoA->ModuleNumber, -1,-1, 0, 
		FMENU_AUTOHIGHLIGHT|FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
		InfoA->GetMsg(InfoA->ModuleNumber,2),
		NULL, NULL, NULL, NULL, items, nCount);

	return nRc;
}

BOOL EditOutputA(LPCWSTR asFileName, BOOL abView)
{
	if (!InfoA)
		return FALSE;

	char szAnsi[MAX_PATH+1];
	if (!WideCharToMultiByte(CP_ACP, 0, asFileName, -1, szAnsi, MAX_PATH+1, 0,0))
		return FALSE;

	BOOL lbRc = FALSE;
	if (!abView) {
		int iRc =
		InfoA->Editor(szAnsi, InfoA->GetMsg(InfoA->ModuleNumber,5), 0,0,-1,-1, 
			EF_NONMODAL|EF_IMMEDIATERETURN|EF_DELETEONLYFILEONCLOSE|EF_ENABLE_F6|EF_DISABLEHISTORY,
			0, 1);
		lbRc = (iRc != EEC_OPEN_ERROR);
	} else {
		#ifdef _DEBUG
		int iRc =
		#endif
			InfoA->Viewer(szAnsi, InfoA->GetMsg(InfoA->ModuleNumber,5), 0,0,-1,-1, 
			VF_NONMODAL|VF_IMMEDIATERETURN|VF_DELETEONLYFILEONCLOSE|VF_ENABLE_F6|VF_DISABLEHISTORY);
		lbRc = TRUE;
	}

	return lbRc;
}

void GetMsgA(int aiMsg, wchar_t* rsMsg/*MAX_PATH*/)
{
	if (!rsMsg || !InfoA)
		return;
	LPCSTR pszMsg = InfoA->GetMsg(InfoA->ModuleNumber,aiMsg);
	if (pszMsg && *pszMsg) {
		int nLen = strlen(pszMsg);
		if (nLen>=MAX_PATH) nLen = MAX_PATH - 1;
		nLen = MultiByteToWideChar(CP_OEMCP, 0, pszMsg, nLen, rsMsg, MAX_PATH-1);
		if (nLen>=0) rsMsg[nLen] = 0;
	} else {
		rsMsg[0] = 0;
	}
}
