
/*
Copyright (c) 2015-present Maximus5
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

HANDLE apiCreateThread(LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, LPDWORD lpThreadId, LPCSTR asThreadNameFormat = NULL, int anFormatArg = 0);
BOOL apiTerminateThreadEx(HANDLE hThread, DWORD dwExitCode, LPCSTR asFile, int anLine);
#define apiTerminateThread(hThread,dwExitCode) apiTerminateThreadEx(hThread,dwExitCode,__FILE__,__LINE__)
bool wasTerminateThreadCalled();

#define MS_VC_THREADNAME_EXCEPTION 0x406D1388

#if defined(_MSC_VER) && !defined(CONEMU_MINIMAL)
void SetThreadName(DWORD dwThreadID, char* threadName);
#else
#define SetThreadName(dwThreadID, threadName)
#endif

class MThread
{
public:
	MThread();
	MThread(LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, LPCSTR asThreadNameFormat = NULL, int anFormatArg = 0);
	virtual ~MThread();

	MThread(const MThread&) = delete;
	MThread& operator=(const MThread&) = delete;
	MThread(MThread&& src);
	MThread& operator=(MThread&& src);

	bool Running() const;
	operator bool() const;
	operator HANDLE() const;
	DWORD GetExitCode() const;

protected:
	void Close();

	LPTHREAD_START_ROUTINE pfnThread = nullptr;
	LPVOID threadArg = nullptr;
	HANDLE hThread = nullptr;
	DWORD threadId = 0;
	mutable bool wasTerminated = false;
	mutable DWORD exitCode = STILL_ACTIVE;
};
