#include "header.h"
#include "../common/ConEmuCheck.h"

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

	DEBUGSTR(L"Pipe::Close()\n");
}


BOOL CConEmuPipe::Init(LPCTSTR asOp, BOOL abSilent)
{
	wchar_t szErr[MAX_PATH*2];
	mh_Pipe = ExecuteOpenPipe(ms_PipeName, szErr, L"ConEmu");
	if (!mh_Pipe || mh_Pipe == INVALID_HANDLE_VALUE) {
		MBoxA(szErr);
		return FALSE;
	}
	
	return TRUE;
	
	//DWORD dwErr = 0;
	//
	//// Try to open a named pipe; wait for it, if necessary. 
	//while (1) 
	//{ 
	//	mh_Pipe = CreateFile( 
	//		ms_PipeName,    // pipe name 
	//		GENERIC_READ |  // read and write access 
	//		GENERIC_WRITE, 
	//		0,              // no sharing 
	//		NULL,           // default security attributes
	//		OPEN_EXISTING,  // opens existing pipe 
	//		0,              // default attributes 
	//		NULL);          // no template file 
	//
	//	// Break if the pipe handle is valid. 
	//	if (mh_Pipe != INVALID_HANDLE_VALUE) 
	//		break; 
	//
	//	// Exit if an error other than ERROR_PIPE_BUSY occurs. 
	//	dwErr = GetLastError();
	//	if (dwErr != ERROR_PIPE_BUSY) 
	//	{
	//		if (!abSilent) {
	//			WCHAR szMsg[128];
	//			swprintf(szMsg, _T("Can't open plugin pipe! Plugin is not installed?\r\nFAR PID: %i, ErrCode: 0x%08X"), mn_FarPID, dwErr);
	//			MBoxA(szMsg);
	//		}
	//		return FALSE;
	//	}
	//
	//	// All pipe instances are busy, so wait for 1 second.
	//	if (!WaitNamedPipe(ms_PipeName, 1000) ) 
	//	{
	//		if (!abSilent) {
	//			WCHAR szMsg[128];
	//			swprintf(szMsg, _T("Plugin pipe timeout!\r\nFAR PID: %i"), mn_FarPID);
	//			MBoxA(szMsg);
	//		}
	//		return FALSE;
	//	}
	//} 
	//
	//// The pipe connected; change to message-read mode. 
	//DWORD dwMode = PIPE_READMODE_MESSAGE; 
	//BOOL fSuccess = SetNamedPipeHandleState( 
	//	mh_Pipe,    // pipe handle 
	//	&dwMode,  // new pipe mode 
	//	NULL,     // don't set maximum bytes 
	//	NULL);    // don't set maximum time 
	//if (!fSuccess) 
	//{
	//	SafeCloseHandle(mh_Pipe);
	//	return FALSE;
	//}
	//
	//return TRUE;
}

// Не интересуется результатом команды!
BOOL CConEmuPipe::Execute(int nCmd, LPCVOID apData, UINT anDataSize)
{
	WARNING("Если указан mdw_Timeout - создать нить и выполнять команду в ней. Ожидать нить не более и прибить ее, если пришел Timeout");

	MCHKHEAP
	if (nCmd<0 || nCmd>MAXCMDCOUNT) {
		TCHAR szError[128];
		swprintf(szError, _T("Invalid command id (%i)!"), nCmd);
		MBoxA(szError);
		return FALSE;
	}


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
	nAllSize = pOut->hdr.nSize;
	pOut = NULL;
    
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
