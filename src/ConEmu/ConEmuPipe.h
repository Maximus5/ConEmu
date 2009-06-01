#pragma once

class CConEmuPipe
{
//protected:
//   HANDLE hEventCmd[MAXCMDCOUNT];
//public:
//   HANDLE hEventAlive, hEventReady;
//   static HANDLE hMapping;
//   static TCHAR  sLastOp[64];
//   TCHAR  ms_LastOp[64];
//   LPBYTE lpMap, lpCursor;
//   DWORD  dwMaxDataSize, nPID;
//   WCHAR  sMapName[MAX_PATH];
protected:
	DWORD mn_FarPID;
	WCHAR ms_PipeName[MAX_PATH];
	HANDLE mh_Pipe;
	CESERVER_REQ *pIn, *pOut;
	LPBYTE lpCursor;
	DWORD dwMaxDataSize;
	DWORD mdw_Timeout;
public:
   CConEmuPipe(DWORD anFarPID, DWORD anTimeout=-1);
   ~CConEmuPipe();
   void Close();
   BOOL Init(LPCTSTR asOp, BOOL abSilent=FALSE);
   BOOL Execute(int nCmd, LPVOID apData=NULL, UINT anDataSize=0);
   BOOL Read(LPVOID pData, DWORD nSize, DWORD* nRead);
   LPBYTE GetPtr(DWORD* pdwLeft=NULL);
};
