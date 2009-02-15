#pragma once

#ifdef _DEBUG
	#define CONEMUALIVETIMEOUT INFINITE
	#define CONEMUREADYTIMEOUT INFINITE
#else
	#define CONEMUALIVETIMEOUT 1000 // Живость плагина ждем секунду
	#define CONEMUREADYTIMEOUT 10000 // А на выполнение команды - 10s max
#endif

class CConEmuPipe
{
public:
   HANDLE hEventCmd[MAXCMDCOUNT], hEventAlive, hEventReady, hMapping;
   LPBYTE lpMap, lpCursor;
   DWORD  dwMaxDataSize, nPID;
   WCHAR  sMapName[MAX_PATH];
public:
   CConEmuPipe();
   ~CConEmuPipe();
   
   BOOL Init();
   BOOL Read(LPVOID pData, DWORD nSize, DWORD* nRead);
};
