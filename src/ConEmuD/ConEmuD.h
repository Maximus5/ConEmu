
#pragma once

#if defined(__GNUC__)
//#include "assert.h"
#define _ASSERTE(x)
#define _ASSERT(x)
#else
#include <crtdbg.h>
#endif

#ifndef EVENT_CONSOLE_CARET
#define EVENT_CONSOLE_CARET             0x4001
#define EVENT_CONSOLE_UPDATE_REGION     0x4002
#define EVENT_CONSOLE_UPDATE_SIMPLE     0x4003
#define EVENT_CONSOLE_UPDATE_SCROLL     0x4004
#define EVENT_CONSOLE_LAYOUT            0x4005
#define EVENT_CONSOLE_START_APPLICATION 0x4006
#define EVENT_CONSOLE_END_APPLICATION   0x4007
#endif

#define SafeCloseHandle(h) { if ((h)!=NULL) { HANDLE hh = (h); (h) = NULL; if (hh!=INVALID_HANDLE_VALUE) CloseHandle(hh); } }

DWORD WINAPI InstanceThread(LPVOID);
DWORD WINAPI ServerThread(LPVOID lpvParam);
DWORD WINAPI InputThread(LPVOID lpvParam);
BOOL GetAnswerToRequest(CESERVER_REQ& in, CESERVER_REQ** out); 
DWORD WINAPI WinEventThread(LPVOID lpvParam);
void WINAPI WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
void CheckCursorPos();
void SendConsoleChanges(CESERVER_REQ* pOut);
CESERVER_REQ* CreateConsoleInfo(CESERVER_CHAR* pCharOnly, int bCharAttrBuff);
BOOL ReloadConsoleInfo(BOOL abSkipCursorCharCheck=FALSE); // возвращает TRUE в случае изменений
void ReloadFullConsoleInfo(CESERVER_CHAR* pCharOnly=NULL); // В том числе перечитывает содержимое
DWORD WINAPI RefreshThread(LPVOID lpvParam); // Нить, перечитывающая содержимое консоли
DWORD ReadConsoleData(CESERVER_CHAR** pCheck = NULL, BOOL* pbDataChanged = NULL); //((LPRECT)1) или реальный LPRECT
void SetConsoleFontSizeTo(HWND inConWnd, int inSizeX, int inSizeY);
int ParseCommandLine(LPCWSTR asCmdLine, wchar_t** psNewCmd); // Разбор параметров командной строки
int NextArg(LPCWSTR &asCmdLine, wchar_t* rsArg/*[MAX_PATH+1]*/);
int ServerInit(); // Создать необходимые события и нити
void ServerDone(int aiRc);
int ComspecInit();
void ComspecDone(int aiRc);
void Help();
BOOL SetConsoleSize(USHORT BufferHeight, COORD crNewSize, SMALL_RECT rNewRect, LPCSTR asLabel = NULL);
void CreateLogSizeFile();
void LogSize(COORD* pcrSize, LPCSTR pszLabel);
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType);
int GetProcessCount(DWORD **rpdwPID);
SHORT CorrectTopVisible(int nY);
void CorrectVisibleRect(CONSOLE_SCREEN_BUFFER_INFO* pSbi);
WARNING("Вместо GetConsoleScreenBufferInfo нужно использовать MyGetConsoleScreenBufferInfo!");
BOOL MyGetConsoleScreenBufferInfo(HANDLE ahConOut, PCONSOLE_SCREEN_BUFFER_INFO apsc);
void ExitWaitForKey(WORD vkKey, LPCWSTR asConfirm, BOOL abNewLine);



#define CERR_GETCOMMANDLINE 100
#define CERR_CARGUMENT 101
#define CERR_CMDEXENOTFOUND 102
#define CERR_NOTENOUGHMEM1 103
#define CERR_CREATESERVERTHREAD 104
#define CERR_CREATEPROCESS 105
#define CERR_WINEVENTTHREAD 106
#define CERR_CONINFAILED 107
#define CERR_GETCONSOLEWINDOW 108
#define CERR_EXITEVENT 109
#define CERR_GLOBALUPDATE 110
#define CERR_WINHOOKNOTCREATED 111
#define CERR_CREATEINPUTTHREAD 112
#define CERR_CONOUTFAILED 113
#define CERR_PROCESSTIMEOUT 114
#define CERR_REFRESHEVENT 115
#define CERR_CREATEREFRESHTHREAD 116
#define CERR_CMDLINE 117
#define CERR_HELPREQUESTED 118
#define CERR_ATTACHFAILED 119
#define CERR_CMDLINEEMPTY 120

