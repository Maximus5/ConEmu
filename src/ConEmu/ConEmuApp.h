#pragma once

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

BOOL CreateMainWindow();
BOOL CreateAppWindow();

int __stdcall _MDEBUG_TRAP(LPCSTR asFile, int anLine);

#ifdef MSGLOGGER
void DebugLogFile(LPCSTR asMessage);
void DebugLogBufSize(HANDLE h, COORD sz);
void DebugLogPos(HWND hw, int x, int y, int w, int h, LPCSTR asFunc);
void DebugLogMessage(HWND h, UINT m, WPARAM w, LPARAM l, BOOL posted, BOOL extra);
#endif
