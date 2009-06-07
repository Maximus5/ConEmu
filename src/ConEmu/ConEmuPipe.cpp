#include "header.h"

//HANDLE CConEmuPipe::hMapping=NULL;
//TCHAR  CConEmuPipe::sLastOp[64] = _T("");

WARNING("!!! Обязательно нужно сделать возможность отваливаться по таймауту!");

CConEmuPipe::CConEmuPipe(DWORD anFarPID, DWORD anTimeout/*=-1*/)
{
	mn_FarPID = anFarPID;
	mh_Pipe = NULL;
	wsprintf(ms_PipeName, CEPLUGINPIPENAME, L".", mn_FarPID);
	pIn = NULL;
	pOut = NULL;
	lpCursor = NULL;
	dwMaxDataSize = 0;
	mdw_Timeout = anTimeout;

	//MCHKHEAP
	//for (int i=0; i<MAXCMDCOUNT; i++)
	//	hEventCmd[i] = NULL;
	//hEventAlive=NULL;
	//hEventReady=NULL;
	////hMapping=NULL;
	//dwMaxDataSize = 0;
	//lpMap = NULL;
	//lpCursor = NULL;
	//sMapName[0] = 0;
	//nPID = 0;
	//MCHKHEAP
}

CConEmuPipe::~CConEmuPipe()
{
	Close();
}

void CConEmuPipe::Close()
{
	SafeCloseHandle(mh_Pipe);
	SafeFree(pIn);
	SafeFree(pOut);

	//MCHKHEAP
	//for (int i=0; i<MAXCMDCOUNT; i++)
	//	SafeCloseHandle(hEventCmd[i]);
	//SafeCloseHandle(hEventAlive);
	//SafeCloseHandle(hEventReady);
	//if (lpMap)
	//	UnmapViewOfFile(lpMap);
	//MCHKHEAP
	//if (hMapping) {
	//	SafeCloseHandle(hMapping);
	//	ms_LastOp[0] = 0;
	//	sLastOp[0] = 0;
	//}
	//MCHKHEAP

	DEBUGSTR(L"Pipe::Close()\n");
}

//#define CREATEEVENT(fmt,h) \
//		wsprintf(szEventName, fmt, dwCurProcId ); \
//		h = OpenEvent(EVENT_ALL_ACCESS, FALSE, szEventName); \
//		if (h==INVALID_HANDLE_VALUE) h=NULL;

//#pragma message("warning: добавить аргумент abSilent, вдруг плагин еще не загружен?")
BOOL CConEmuPipe::Init(LPCTSTR asOp, BOOL abSilent)
{
	DWORD dwErr = 0;

	// Try to open a named pipe; wait for it, if necessary. 
	while (1) 
	{ 
		mh_Pipe = CreateFile( 
			ms_PipeName,    // pipe name 
			GENERIC_READ |  // read and write access 
			GENERIC_WRITE, 
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			0,              // default attributes 
			NULL);          // no template file 

		// Break if the pipe handle is valid. 
		if (mh_Pipe != INVALID_HANDLE_VALUE) 
			break; 

		// Exit if an error other than ERROR_PIPE_BUSY occurs. 
		dwErr = GetLastError();
		if (dwErr != ERROR_PIPE_BUSY) 
		{
			if (!abSilent) {
				WCHAR szMsg[128];
				swprintf(szMsg, _T("Can't open plugin pipe! Plugin is not installed?\r\nFAR PID: %i, ErrCode: 0x%08X"), mn_FarPID, dwErr);
				MBoxA(szMsg);
			}
			return FALSE;
		}

		// All pipe instances are busy, so wait for 1 second.
		if (!WaitNamedPipe(ms_PipeName, 1000) ) 
		{
			if (!abSilent) {
				WCHAR szMsg[128];
				swprintf(szMsg, _T("Plugin pipe timeout!\r\nFAR PID: %i"), mn_FarPID);
				MBoxA(szMsg);
			}
			return FALSE;
		}
	} 

	// The pipe connected; change to message-read mode. 
	DWORD dwMode = PIPE_READMODE_MESSAGE; 
	BOOL fSuccess = SetNamedPipeHandleState( 
		mh_Pipe,    // pipe handle 
		&dwMode,  // new pipe mode 
		NULL,     // don't set maximum bytes 
		NULL);    // don't set maximum time 
	if (!fSuccess) 
	{
		SafeCloseHandle(mh_Pipe);
		return FALSE;
	}

	return TRUE;

	//MCHKHEAP

	//#ifdef _DEBUG
	//if (ghWnd) {
	//	DWORD dwPID=0, dwTID = GetWindowThreadProcessId(ghWnd, &dwPID);
	//	DWORD dwCur = GetCurrentThreadId();
	//	_ASSERT(dwCur == dwTID);
	//}
	//#endif

	//if (hMapping) {
	//	if (!abSilent) {
	//		TCHAR szMsg[0x400];
	//		wsprintf(szMsg, _T("Can't start '%s'\nLast operation '%s' was not finished!"), asOp, sLastOp);
	//		MBoxA(szMsg);
	//	}
	//	return FALSE;
	//}

	//DWORD dwCurProcId = gConEmu.ActiveCon()->RCon()->GetFarPID();
 //   if (!gConEmu.isFar() && !dwCurProcId) {
	//    gConEmu.DnDstep(_T("Pipe: FAR not active"));
	//	DEBUGSTR(L"Pipe::FAR not active\n");
	//    return FALSE;
	//}
	//
	//MCHKHEAP
	//
	//lstrcpyn(ms_LastOp, asOp, 64);

	//// Сформируем ИМЯ сразу, а то вдруг процесс переключится?
	//nPID = dwCurProcId;
	//wsprintf(sMapName, CONEMUMAPPING, dwCurProcId);
	//
	//WCHAR szEventName[64];

	//CREATEEVENT(CONEMUALIVE, hEventAlive);
	//CREATEEVENT(CONEMUREADY, hEventReady);
	//if (!hEventAlive || !hEventReady) {
	//	DEBUGSTR(L"ConEmu plugins is not installed\n");
	//	if (!abSilent) {
	//		WCHAR szMsg[128];
	//		swprintf(szMsg, _T("ConEmu plugin was not installed!\r\nFAR PID: %i"), nPID);
	//		MBoxA(szMsg);
	//	}
	//	return FALSE;
	//}

	//CREATEEVENT(CONEMUDRAGFROM, hEventCmd[CMD_DRAGFROM]);
	//CREATEEVENT(CONEMUDRAGTO, hEventCmd[CMD_DRAGTO]);
	//CREATEEVENT(CONEMUREQTABS, hEventCmd[CMD_REQTABS]);
	//CREATEEVENT(CONEMUSETWINDOW, hEventCmd[CMD_SETWINDOW]);
	//CREATEEVENT(CONEMUPOSTMACRO, hEventCmd[CMD_POSTMACRO]);
	//CREATEEVENT(CONEMUDEFFONT, hEventCmd[CMD_DEFFONT]);
	//CREATEEVENT(CONEMULANGCHANGE, hEventCmd[CMD_LANGCHANGE]);
	//CREATEEVENT(CONEMUEXIT, hEventCmd[CMD_EXIT]);

	//MCHKHEAP

	//if (!hEventAlive || !hEventReady || 
	//	!hEventCmd[CMD_DRAGFROM] || !hEventCmd[CMD_DRAGTO] || 
	//	!hEventCmd[CMD_REQTABS] || !hEventCmd[CMD_SETWINDOW] ||
	//	!hEventCmd[CMD_POSTMACRO] || !hEventCmd[CMD_DEFFONT] || 
	//	!hEventCmd[CMD_LANGCHANGE] || !hEventCmd[CMD_EXIT] )
	//{
	//	DEBUGSTR(L"Create event failed\n");
	//	if (!abSilent)
	//		MBoxA(_T("CreateEvent failed"));
	//	return FALSE;
	//}

	//DEBUGSTR(L"Pipe:Initialized\n");

	//return TRUE;
}

// Не интересуется результатом команды!
BOOL CConEmuPipe::Execute(int nCmd, LPVOID apData, UINT anDataSize)
{
	WARNING("Если указан mdw_Timeout - создать нить и выполнять команду в ней. Ожидать нить не более и прибить ее, если пришел Timeout");

	MCHKHEAP
	if (nCmd<0 || nCmd>MAXCMDCOUNT) {
		TCHAR szError[128];
		swprintf(szError, _T("Invalid command id (%i)!"), nCmd);
		MBoxA(szError);
		return FALSE;
	}
	//if (!hEventCmd[nCmd]) {
	//	TCHAR szError[128];
	//	swprintf(szError, _T("Command %i was not created by plugin!"), nCmd);
	//	MBoxA(szError);
	//	return FALSE;
	//}

	#ifdef _DEBUG
	WCHAR szMsg[64]; swprintf(szMsg, _T("Pipe:Execute(%i)\n"), nCmd);
	DEBUGSTR(szMsg);
	#endif

	int nAllSize = sizeof(CESERVER_REQ_HDR)+anDataSize;
	CESERVER_REQ* pIn = (CESERVER_REQ*)calloc(nAllSize,1);
	_ASSERTE(pIn!=NULL);
	if (!pIn) {
		TCHAR szError[128];
		swprintf(szError, _T("Pipe: Can't allocate memory (%i) bytes, Cmd = %i!"), nAllSize, nCmd);
		MBoxA(szError);
		Close();
		return FALSE;
	}

	pIn->hdr.nCmd = nCmd;
	pIn->hdr.nSrcThreadId = GetCurrentThreadId();
	pIn->hdr.nSize = nAllSize;
	pIn->hdr.nVersion = CESERVER_REQ_VER;
	if (apData && anDataSize) {
		memmove(pIn->Data, apData, anDataSize);
	}

    BYTE cbReadBuf[512];
    BOOL fSuccess;
    DWORD cbRead, dwErr;

    // Send a message to the pipe server and read the response. 
    fSuccess = TransactNamedPipe( 
      mh_Pipe,                // pipe handle 
      pIn,                    // message to server
      pIn->hdr.nSize,             // message length 
      cbReadBuf,              // buffer to receive reply
      sizeof(cbReadBuf),      // size of read buffer
      &cbRead,                // bytes read
      NULL);                  // not overlapped 
	dwErr = GetLastError();

	if (!fSuccess && dwErr == ERROR_BROKEN_PIPE) {
		// Плагин не вернул данных, но обработал команду
		Close();
		return TRUE;
	} else
    if (!fSuccess && (dwErr != ERROR_MORE_DATA)) 
    {
		DEBUGSTR(L" - FAILED!\n");
		TCHAR szError[128];
		swprintf(szError, _T("Pipe: TransactNamedPipe failed, Cmd = %i, ErrCode = 0x%08X!"), nCmd, dwErr);
		MBoxA(szError);
        Close();
        return FALSE;
    }

	// Информационно!
	pOut = (CESERVER_REQ*)cbReadBuf;
	if (pOut->hdr.nVersion != CESERVER_REQ_VER) {
		gConEmu.ShowOldCmdVersion(pOut->hdr.nCmd, pOut->hdr.nVersion);
		pOut = NULL;
		Close();
		return FALSE;
	}
	pOut = NULL;


    nAllSize = *((DWORD*)cbReadBuf);
    if (nAllSize==0) {
       DEBUGSTR(L" - FAILED!\n");
       DisplayLastError(L"Empty data recieved from server", 0);
       Close();
       return FALSE;
    }
    pOut = (CESERVER_REQ*)calloc(nAllSize,1);
    _ASSERTE(pOut!=NULL);
    memmove(pOut, cbReadBuf, cbRead);
    _ASSERTE(pOut->hdr.nVersion==CESERVER_REQ_VER);

    LPBYTE ptrData = ((LPBYTE)pOut)+cbRead;
    nAllSize -= cbRead;

    while(nAllSize>0)
    { 
      //_tprintf(TEXT("%s\n"), chReadBuf);

      // Break if TransactNamedPipe or ReadFile is successful
      if(fSuccess)
         break;

      // Read from the pipe if there is more data in the message.
      fSuccess = ReadFile( 
         mh_Pipe,    // pipe handle 
         ptrData,    // buffer to receive reply 
         nAllSize,   // size of buffer 
         &cbRead,    // number of bytes read 
         NULL);      // not overlapped 

      // Exit if an error other than ERROR_MORE_DATA occurs.
      if( !fSuccess && (GetLastError() != ERROR_MORE_DATA)) 
         break;
      ptrData += cbRead;
      nAllSize -= cbRead;
    }

    TODO("Может возникнуть ASSERT, если консоль была закрыта в процессе чтения");
    _ASSERTE(nAllSize==0);

    SafeCloseHandle(mh_Pipe);
	SafeFree(pIn);

	lpCursor = pOut->Data;
	dwMaxDataSize = pOut->hdr.nSize - sizeof(CESERVER_REQ_HDR);

	//ResetEvent(hEventAlive);
	//ResetEvent(hEventReady);
	//SetEvent(hEventCmd[nCmd]);
	//MCHKHEAP
	return TRUE;
}

LPBYTE CConEmuPipe::GetPtr(DWORD* pdwLeft/*=NULL*/)
{
	if (pdwLeft) {
		if (!dwMaxDataSize)
			*pdwLeft = 0;
		else
			*pdwLeft = dwMaxDataSize - (lpCursor-pOut->Data);
	}
	return lpCursor;
}

BOOL CConEmuPipe::Read(LPVOID pData, DWORD nSize, DWORD* nRead)
{
	MCHKHEAP
	if (nRead) *nRead = 0; // пока сбросим

	//if (hMapping==NULL) {
	//	// Нужно инициализировать FileMapping
	//	DEBUGSTR(L"Starts reading result\n");
	//	
	//	lstrcpy(sLastOp, ms_LastOp);
	//	
	//	
	//	MCHKHEAP
	//	hMapping = OpenFileMapping(FILE_MAP_ALL_ACCESS, // Read/write permission. 
	//		FALSE,                             // Do not inherit the name
	//		sMapName);            // of the mapping object. 
	//	if (hMapping == NULL) {
	//		hMapping = INVALID_HANDLE_VALUE;
	//		// Нету такого
	//		MessageBox(ghWnd, _T("Can't open filemapping"), sMapName, MB_ICONSTOP);
	//		return FALSE;
	//	}
	//	lpMap = (LPBYTE)MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0,0,0);
	//	if (lpMap==NULL) {
	//		CloseHandle(hMapping); hMapping = INVALID_HANDLE_VALUE;
	//		MessageBox(ghWnd, _T("Can't map view of file"), sMapName, MB_ICONSTOP);
	//		return FALSE;
	//	}
	//	MCHKHEAP
	//	dwMaxDataSize = *((DWORD*)lpMap);
	//	lpCursor = lpMap+4;
	//}
	//if (hMapping==INVALID_HANDLE_VALUE || !hMapping) {
	//	if (nRead) *nRead=0;
	//	return FALSE;
	//}

	if (lpCursor == NULL) {
		if (nRead) *nRead=0;
		return FALSE;
	}

	MCHKHEAP
	if (!dwMaxDataSize)
		nSize = 0; else
	if ((lpCursor-pOut->Data+nSize)>dwMaxDataSize)
		nSize = dwMaxDataSize - (lpCursor-pOut->Data);

	MCHKHEAP
	if (nSize) {
		memmove(pData, lpCursor, nSize);
		MCHKHEAP
		lpCursor += nSize;
	}
	if (nRead) *nRead = nSize;

	return (nSize!=0);
}
