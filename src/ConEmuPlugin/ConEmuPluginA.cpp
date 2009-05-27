#include <windows.h>
#include "..\common\common.hpp"
#include "..\common\pluginA.hpp"
#include "PluginHeader.h"


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


HANDLE WINAPI _export OpenPlugin(int OpenFrom,INT_PTR Item)
{
	if (InfoA == NULL)
		return INVALID_HANDLE_VALUE;

	if (gnReqCommand != (DWORD)-1) {
		ProcessCommand(gnReqCommand, FALSE/*bReqMainThread*/);
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

void UpdateConEmuTabsA(int event, bool losingFocus, bool editorSave);

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

		// Это только предполагаемый размер, при необходимости он будет увеличен
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

	WindowInfo WInfo;				
    WInfo.Pos=0;
	InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);
	if (!WInfo.Current)
	{
		int ItemsCount=0;
		//WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);				
		OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}
	PanelInfo PAInfo, PPInfo;
	ForwardedPanelInfo *pfpi=NULL;
	int nStructSize = sizeof(ForwardedPanelInfo)+4; // потом увеличим на длину строк
	//ZeroMemory(&fpi, sizeof(fpi));
	BOOL lbAOK=FALSE, lbPOK=FALSE;
	
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
	if (!pfpi) {
		int ItemsCount=0;
		//WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);				
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

	// Собственно, пересылка информации
	//WriteFile(hPipe, &nStructSize, sizeof(nStructSize), &cout, NULL);
	//WriteFile(hPipe, pfpi, nStructSize, &cout, NULL);
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
}

extern WCHAR gcPlugKey; // Для ANSI far он инициализируется как (char)

void WINAPI _export GetPluginInfo(struct PluginInfo *pi)
{
    static char *szMenu[1], szMenu1[255];
	szMenu[0]=szMenu1;

	// Проверить, не изменилась ли горячая клавиша плагина, и если да - пересоздать макросы
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
	if (!ConEmuHwnd || !InfoA) // иногда событие от QuickView приходит ДО инициализации плагина
		return 0; // Если мы не под эмулятором - ничего
	// only key events with virtual codes > 0 are likely to cause status change (?)
	if (Rec->EventType == KEY_EVENT && Rec->Event.KeyEvent.wVirtualKeyCode > 0  && Rec->Event.KeyEvent.bKeyDown)
	{
		EditorInfo ei;
		InfoA->EditorControl(ECTL_GETINFO, &ei);
		int currentModifiedState = ei.CurState == ECSTATE_MODIFIED ? 1 : 0;
		if (lastModifiedStateW != currentModifiedState)
		{
			UpdateConEmuTabsA(0, false, false);
			lastModifiedStateW = currentModifiedState;
		}
	}
	return 0;
}

int WINAPI _export ProcessEditorEvent(int Event, void *Param)
{
	if (!ConEmuHwnd || !InfoA) // иногда событие от QuickView приходит ДО инициализации плагина
		return 0; // Если мы не под эмулятором - ничего
	switch (Event)
	{
	case EE_CLOSE:
	case EE_GOTFOCUS:
	case EE_KILLFOCUS:
	case EE_SAVE:
	//case EE_READ:
		{
			UpdateConEmuTabsA(Event, Event == EE_KILLFOCUS, Event == EE_SAVE);
			break;
		}
	}
	return 0;
}

int WINAPI _export ProcessViewerEvent(int Event, void *Param)
{
	if (!ConEmuHwnd || !InfoA) // иногда событие от QuickView приходит ДО инициализации плагина
		return 0; // Если мы не под эмулятором - ничего
	switch (Event)
	{
	case VE_CLOSE:
	//case VE_READ:
	case VE_KILLFOCUS:
	case VE_GOTFOCUS:
		{
			UpdateConEmuTabsA(Event, Event == VE_KILLFOCUS, false);
		}
	}

	return 0;
}

void UpdateConEmuTabsA(int event, bool losingFocus, bool editorSave)
{
	if (!InfoA) return;

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

	int tabCount = 1;
	for (int i = 0; i < windowCount; i++)
	{
		WInfo.Pos = i;
		InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);
		if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
		{
			MultiByteToWideChar(CP_OEMCP, 0, WInfo.Name, lstrlenA(WInfo.Name)+1, pszName, CONEMUTABMAX);
			lbCh |= AddTab(tabCount, losingFocus, editorSave, 
				WInfo.Type, pszName, editorSave ? pszFileName : NULL, 
				WInfo.Current, WInfo.Modified);
		}
	}

	SendTabs(tabCount);
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
