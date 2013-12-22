
/*
Copyright (c) 2013 Maximus5
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

int StartDefTermHooker(DWORD nForePID, HANDLE& hProcess, DWORD& nResult, LPCWSTR asConEmuBaseDir, DWORD& nErrCode)
{
	int iRc = -1;
	wchar_t szCmdLine[MAX_PATH*3];
	int nBits;
	PROCESS_INFORMATION pi = {};
	STARTUPINFO si = {sizeof(si)};
	BOOL bStarted = FALSE;

	nErrCode = 0;

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|SYNCHRONIZE, FALSE, nForePID);
	if (!hProcess)
	{
		// Failed to hook
		nErrCode = GetLastError();
		goto wrap;
	}

	// Need to be hooked
	nBits = GetProcessBits(nForePID, hProcess);
	switch (nBits)
	{
	case 32:
		msprintf(szCmdLine, countof(szCmdLine), L"\"%s\\%s\" /DEFTRM=%u",
			asConEmuBaseDir, L"ConEmuC.exe", nForePID);
		break;
	case 64:
		msprintf(szCmdLine, countof(szCmdLine), L"\"%s\\%s\" /DEFTRM=%u",
			asConEmuBaseDir, L"ConEmuC64.exe", nForePID);
		break;
	default:
		szCmdLine[0] = 0;
	}
	if (!*szCmdLine)
	{
		// Unsupported bitness?
		CloseHandle(hProcess);
		iRc = -2;
		goto wrap;
	}

	// Run hooker
	si.dwFlags = STARTF_USESHOWWINDOW;
	bStarted = CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	if (!bStarted)
	{
		//DisplayLastError(L"Failed to start hooking application!\nDefault terminal feature will not be available!");
		nErrCode = GetLastError();
		CloseHandle(hProcess);
		iRc = -3;
		goto wrap;
	}
	CloseHandle(pi.hThread);
	// Waiting for result
	WaitForSingleObject(pi.hProcess, INFINITE);
	GetExitCodeProcess(pi.hProcess, &nResult);
	CloseHandle(pi.hProcess);

	iRc = 0;
wrap:
	return iRc;
}
