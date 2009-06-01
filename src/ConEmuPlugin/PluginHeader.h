#pragma once

#if !defined(_MSC_VER)
#include <wchar.h>
#include <tchar.h>
#endif

#define SafeCloseHandle(h) { if ((h)!=NULL) { HANDLE hh = (h); (h) = NULL; if (hh!=INVALID_HANDLE_VALUE) CloseHandle(hh); } }
#ifdef _DEBUG
#define OUTPUTDEBUGSTRING(m) OutputDebugString(m)
#else
#define OUTPUTDEBUGSTRING(m)
#endif

#define ISALPHA(c) ((((c) >= (BYTE)'c') && ((c) <= (BYTE)'z')) || (((c) >= (BYTE)'C') && ((c) <= (BYTE)'Z')))

extern int lastModifiedStateW;
extern WCHAR gszDir1[CONEMUTABMAX], gszDir2[CONEMUTABMAX], gszRootKey[MAX_PATH];
extern int maxTabCount, lastWindowCount;
extern ConEmuTab* tabs; //(ConEmuTab*) calloc(maxTabCount, sizeof(ConEmuTab));
extern HWND ConEmuHwnd;
extern HWND FarHwnd;
extern FarVersion gFarVersion;
extern int lastModifiedStateW;
extern HANDLE hEventCmd[MAXCMDCOUNT];
extern HANDLE hThread;
extern WCHAR gcPlugKey;

BOOL CreateTabs(int windowCount);

BOOL AddTab(int &tabCount, bool losingFocus, bool editorSave, 
			int Type, LPCWSTR Name, LPCWSTR FileName, int Current, int Modified);

void SendTabs(int tabCount, BOOL abFillDataOnly=FALSE);

void InitHWND(HWND ahFarHwnd);

int ProcessEditorInputW789(LPCVOID Rec);
int ProcessEditorInputW757(LPCVOID Rec);
int ProcessEditorEventW789(int Event, void *Param);
int ProcessEditorEventW757(int Event, void *Param);
int ProcessViewerEventW789(int Event, void *Param);
int ProcessViewerEventW757(int Event, void *Param);
void StopThread(void);
void ExitFARW789(void);
void ExitFARW757(void);
void UpdateConEmuTabsW789(int event, bool losingFocus, bool editorSave);
void UpdateConEmuTabsW757(int event, bool losingFocus, bool editorSave);
void SetStartupInfoW789(void *aInfo);
void SetStartupInfoW757(void *aInfo);
void ProcessDragFrom789();
void ProcessDragFrom757();
void ProcessDragFromA();
void ProcessDragTo789();
void ProcessDragTo757();
void ProcessDragToA();
void SetWindowA(int nTab);
void SetWindow789(int nTab);
void SetWindow757(int nTab);

void CloseTabs();

HWND AtoH(WCHAR *Str, int Len);
void UpdateConEmuTabsW(int event, bool losingFocus, bool editorSave);

BOOL LoadFarVersion();

BOOL OutDataAlloc(DWORD anSize); // необязательно
BOOL OutDataWrite(LPVOID apData, DWORD anSize);

void CheckMacro(BOOL abAllowAPI);
BOOL IsKeyChanged(BOOL abAllowReload);
int ShowMessage(int aiMsg, int aiButtons);
int ShowMessageA(int aiMsg, int aiButtons);
int ShowMessage789(int aiMsg, int aiButtons);
int ShowMessage757(int aiMsg, int aiButtons);
void ReloadMacroA();
void ReloadMacro789();
void ReloadMacro757();
void PostMacro(wchar_t* asMacro);
void PostMacroA(char* asMacro);
void PostMacro789(wchar_t* asMacro);
void PostMacro757(wchar_t* asMacro);
LPCWSTR GetMsgW(int aiMsg);
LPCWSTR GetMsg757(int aiMsg);
LPCWSTR GetMsg789(int aiMsg);

extern DWORD gnReqCommand;
extern LPVOID gpReqCommandData;
void ProcessCommand(DWORD nCmd, BOOL bReqMainThread, LPVOID pCommandData);
BOOL CheckPlugKey();
void NotifyChangeKey();

DWORD WINAPI ServerThread(LPVOID lpvParam);
void ServerThreadCommand(HANDLE hPipe);
