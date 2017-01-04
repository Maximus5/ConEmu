
/*
Copyright (c) 2012-2017 Maximus5
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

#pragma once

#include "WThreads.h"

#define CONEMUINOUTBUFSIZE 4096 

struct ConEmuInOutPipe
{
	HANDLE hStd;
	HANDLE hRead, hWrite;

	BOOL   bIsOut;

	DWORD  nLastErr, nErrStep;

	BOOL   bTerminated;

	BOOL   bSuccess;
	char   chBuf[CONEMUINOUTBUFSIZE];
	DWORD  dwRead, dwWritten;

	HANDLE hThread;
	DWORD  nThreadID;

	bool CEIO_Initialize(HANDLE ahStd, bool bOut)
	{
		SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
		BOOL bRc;

		bIsOut = bOut;

		bRc = CreatePipe(&hRead, &hWrite, &sa, 0);
		if (!bRc)
		{
			nLastErr = GetLastError();
			nErrStep = 1;
			goto wrap;
		}

		bRc = SetHandleInformation(bOut ? hWrite : hRead, HANDLE_FLAG_INHERIT, 0);
		if (!bRc)
		{
			nLastErr = GetLastError();
			nErrStep = 2;
			goto wrap;
		}

	wrap:
		return (bRc != FALSE);
	};

	DWORD CEIO_ThreadProcExec()
	{
		//BOOL bSuccess = FALSE;
		//char  cBuf[CONEMUINOUTBUFSIZE];
		//DWORD dwRead, dwWritten;

		while (!bTerminated)
		{
			if (bIsOut)
			{
				bSuccess = ReadFile(hWrite, chBuf, CONEMUINOUTBUFSIZE, &dwRead, NULL);
			}
			else
			{
				// ReadConsoleInputW?
				bSuccess = ReadFile(hStd, chBuf, CONEMUINOUTBUFSIZE, &dwRead, NULL);
			}

			if (!bSuccess || !dwRead)
			{
				nLastErr = GetLastError();
				nErrStep = 4;
				Sleep(10);
				continue;
			}

			if (bTerminated)
			{
				break;
			}
			
			if (bIsOut)
			{
				bSuccess = WriteFile(hStd, chBuf, dwRead, &dwWritten, NULL);
			}
			else
			{
				bSuccess = WriteFile(hWrite, chBuf, dwRead, &dwWritten, NULL);
			}

			if (!bSuccess || !dwWritten)
			{
				nLastErr = GetLastError();
				nErrStep = 5;
				continue;
			}
		}

		return 0;
	};

	static DWORD WINAPI CEIO_ThreadProc(LPVOID lpParameter)
	{
		ConEmuInOutPipe* p = (ConEmuInOutPipe*)lpParameter;
		return p->CEIO_ThreadProcExec();
	};

	bool CEIO_RunThread()
	{
		if (!hThread)
		{
			hThread = apiCreateThread(CEIO_ThreadProc, (PVOID)this, &nThreadID, "CEIO_ThreadProc");
			nLastErr = GetLastError();
			nErrStep = 3;
		}
		else
		{
			_ASSERTEX(hThread==NULL);
		}
		return (hThread!=NULL);
	};

	void CEIO_Terminate()
	{
		bTerminated = true;

		if (hThread)
		{
			TODO("Try to stop correctly");
			apiTerminateThread(hThread, 100);

			SafeCloseHandle(hThread);
		}

		SafeCloseHandle(hRead);
		SafeCloseHandle(hWrite);
	}
};
