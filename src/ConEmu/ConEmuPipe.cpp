
/*
Copyright (c) 2009-2017 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define HIDE_USE_EXCEPTION_INFO
#include "header.h"
#include "../common/ConEmuCheck.h"
#include "../common/ConEmuPipeMode.h"
#include "ConEmuPipe.h"
#include "ConEmu.h"
#include "SetPgDebug.h"

WARNING("!!! Обязательно нужно сделать возможность отваливаться по таймауту!");

CConEmuPipe::CConEmuPipe(DWORD anFarPID, DWORD anTimeout/*=-1*/)
{
	mn_FarPID = anFarPID;
	mh_Pipe = NULL;
	_wsprintf(ms_PipeName, SKIPLEN(countof(ms_PipeName)) CEPLUGINPIPENAME, L".", mn_FarPID);
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
	mh_Pipe = ExecuteOpenPipe(ms_PipeName, szErr, gpConEmu->GetDefaultTitle());

	if (!mh_Pipe || mh_Pipe == INVALID_HANDLE_VALUE)
	{
		MBoxA(szErr);
		return FALSE;
	}

	return TRUE;
}

// Не интересуется результатом команды!
BOOL CConEmuPipe::Execute(int nCmd, LPCVOID apData, UINT anDataSize)
{
	WARNING("Если указан mdw_Timeout - создать нить и выполнять команду в ней. Ожидать нить не более и прибить ее, если пришел Timeout");
	MCHKHEAP

	if (!((nCmd >= (int)CMD_FIRST_FAR_CMD && nCmd <= (int)CMD_LAST_FAR_CMD)))
	{
		TCHAR szError[128];
		_wsprintf(szError, SKIPLEN(countof(szError)) _T("Invalid command id (%i)!\nPID=%u, TID=%u"), nCmd, GetCurrentProcessId(), GetCurrentThreadId());
		MBoxA(szError);
		return FALSE;
	}

#ifdef _DEBUG
	WCHAR szMsg[64]; _wsprintf(szMsg, SKIPLEN(countof(szMsg)) _T("Pipe:Execute(%i)\n"), nCmd);
	DEBUGSTR(szMsg);
#endif
	int nAllSize = sizeof(CESERVER_REQ_HDR)+anDataSize;
	CESERVER_REQ* pIn = ExecuteNewCmd(nCmd, nAllSize);

	if (!pIn)
	{
		_ASSERTE(pIn!=NULL);
		TCHAR szError[128];
		_wsprintf(szError, SKIPLEN(countof(szError)) _T("Pipe: Can't allocate memory (%i) bytes, Cmd = %i!"), nAllSize, nCmd);
		MBoxA(szError);
		Close();
		return FALSE;
	}

	if (apData && anDataSize)
	{
		memmove(pIn->Data, apData, anDataSize);
	}

	DWORD dwTickStart = timeGetTime();

	BYTE cbReadBuf[512];
	BOOL fSuccess;
	DWORD cbRead, dwErr;
	// Send a message to the pipe server and read the response.
	fSuccess = TransactNamedPipe(
					mh_Pipe,                // pipe handle
					pIn,                    // message to server
					pIn->hdr.cbSize,        // message length
					cbReadBuf,              // buffer to receive reply
					sizeof(cbReadBuf),      // size of read buffer
					&cbRead,                // bytes read
					NULL);                  // not overlapped
	dwErr = GetLastError();

	CSetPgDebug::debugLogCommand(pIn, FALSE, dwTickStart, timeGetTime()-dwTickStart, ms_PipeName);

	if (!fSuccess && dwErr == ERROR_BROKEN_PIPE)
	{
		// Плагин не вернул данных, но обработал команду
		Close();
		SafeFree(pIn);
		return TRUE;
	}
	else if (!fSuccess && (dwErr != ERROR_MORE_DATA))
	{
		DEBUGSTR(L" - FAILED!\n");
		TCHAR szError[128];
		_wsprintf(szError, SKIPLEN(countof(szError)) _T("Pipe: TransactNamedPipe failed, Cmd = %i, ErrCode = 0x%08X!"), nCmd, dwErr);
		MBoxA(szError);
		Close();
		SafeFree(pIn);
		return FALSE;
	}

	if (cbRead < sizeof(DWORD))
	{
		pOut = NULL;
		Close();
		SafeFree(pIn);
		return FALSE;
	}

	pOut = (CESERVER_REQ*)cbReadBuf;

	// Проверка размера
	if (pOut->hdr.cbSize <= sizeof(pOut->hdr))
	{
		_ASSERTE(pOut->hdr.cbSize == 0);
		pOut = NULL;
		Close();
		SafeFree(pIn);
		return FALSE;
	}

	if (pOut->hdr.nVersion != CESERVER_REQ_VER)
	{
		gpConEmu->ReportOldCmdVersion(pOut->hdr.nCmd, pOut->hdr.nVersion, -1, pOut->hdr.nSrcPID, pOut->hdr.hModule, pOut->hdr.nBits);
		pOut = NULL;
		Close();
		SafeFree(pIn);
		return FALSE;
	}

	nAllSize = pOut->hdr.cbSize;
	pOut = NULL;

	if (nAllSize==0)
	{
		DEBUGSTR(L" - FAILED!\n");
		DisplayLastError(L"Empty data recieved from server", 0);
		Close();
		SafeFree(pIn);
		return FALSE;
	}

	pOut = (CESERVER_REQ*)calloc(nAllSize,1);
	_ASSERTE(pOut!=NULL);
	memmove(pOut, cbReadBuf, cbRead);
	_ASSERTE(pOut->hdr.nVersion==CESERVER_REQ_VER);
	LPBYTE ptrData = ((LPBYTE)pOut)+cbRead;
	nAllSize -= cbRead;

	while (nAllSize>0)
	{
		//_tprintf(TEXT("%s\n"), chReadBuf);

		// Break if TransactNamedPipe or ReadFile is successful
		if (fSuccess)
			break;

		// Read from the pipe if there is more data in the message.
		fSuccess = ReadFile(
						mh_Pipe,    // pipe handle
						ptrData,    // buffer to receive reply
						nAllSize,   // size of buffer
						&cbRead,    // number of bytes read
						NULL);      // not overlapped

		// Exit if an error other than ERROR_MORE_DATA occurs.
		if (!fSuccess && (GetLastError() != ERROR_MORE_DATA))
			break;

		ptrData += cbRead;
		nAllSize -= cbRead;
	}

	TODO("Может возникнуть ASSERT, если консоль была закрыта в процессе чтения");
	_ASSERTE(nAllSize==0);
	SafeCloseHandle(mh_Pipe);
	SafeFree(pIn);
	lpCursor = pOut->Data;
	dwMaxDataSize = pOut->hdr.cbSize - sizeof(CESERVER_REQ_HDR);
	return TRUE;
}

LPBYTE CConEmuPipe::GetPtr(DWORD* pdwLeft/*=NULL*/)
{
	if (pdwLeft)
	{
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

	if (nRead) *nRead = 0;  // пока сбросим

	if (lpCursor == NULL)
	{
		if (nRead) *nRead=0;

		return FALSE;
	}

	MCHKHEAP

	if (!dwMaxDataSize)
		nSize = 0; else if ((lpCursor-pOut->Data+nSize)>dwMaxDataSize)

		nSize = dwMaxDataSize - (lpCursor-pOut->Data);

	MCHKHEAP

	if (nSize)
	{
		memmove(pData, lpCursor, nSize);
		MCHKHEAP
		lpCursor += nSize;
	}

	if (nRead) *nRead = nSize;

	return (nSize!=0);
}
