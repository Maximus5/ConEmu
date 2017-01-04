
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
		BOOL Execute(int nCmd, LPCVOID apData=NULL, UINT anDataSize=0);
		BOOL Read(LPVOID pData, DWORD nSize, DWORD* nRead);
		LPBYTE GetPtr(DWORD* pdwLeft=NULL);
};
