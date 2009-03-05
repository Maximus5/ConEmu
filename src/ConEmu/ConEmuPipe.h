#pragma once

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
   LPBYTE GetPtr();
};
