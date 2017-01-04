
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

#include "../common/PipeServer.h"

template <class T> struct PipeServer;

class CRealConsole;

class CRealServer
{
public:
	CRealServer();
	~CRealServer();
	void Init(CRealConsole* apRCon);
	bool Start();
	void Stop(bool abDeinitialize=false);
protected:
	CRealConsole* mp_RCon;

	friend class CGuiServer;

	//static DWORD WINAPI RConServerThread(LPVOID lpvParam);
	//HANDLE mh_RConServerThreads[MAX_SERVER_THREADS], mh_ActiveRConServerThread;
	//DWORD  mn_RConServerThreadsId[MAX_SERVER_THREADS];
	//HANDLE mh_ServerSemaphore;
	HANDLE mh_GuiAttached;
	//void SetBufferHeightMode(BOOL abBufferHeight, BOOL abIgnoreLock=FALSE);
	//BOOL mb_BuferModeChangeLocked; -> mp_RBuf

	//void ServerThreadCommand(HANDLE hPipe);
	PipeServer<CESERVER_REQ>* mp_RConServer;
	bool mb_RealForcedTermination;
	static BOOL WINAPI ServerCommand(LPVOID pInst, CESERVER_REQ* pIn, CESERVER_REQ* &ppReply, DWORD &pcbReplySize, DWORD &pcbMaxReplySize, LPARAM lParam);
	static BOOL WINAPI ServerThreadReady(LPVOID pInst, LPARAM lParam);
	static void WINAPI ServerCommandFree(CESERVER_REQ* pReply, LPARAM lParam);
	CESERVER_REQ* cmdStartStop(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	//CESERVER_REQ* cmdGetGuiHwnd(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdTabsChanged(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdGetOutputFile(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdGuiMacro(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdLangChange(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdTabsCmd(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdResources(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdSetForeground(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdFlashWindow(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdRegPanelView(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdSetBackground(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdActivateCon(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdOnCreateProc(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	//CESERVER_REQ* cmdNewConsole(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdOnPeekReadInput(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdOnSetConsoleKeyShortcuts(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdLockDc(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdGetAllTabs(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdGetAllPanels(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdActivateTab(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdRenameTab(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdSetProgress(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdExportEnvVarAll(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdStartXTerm(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdPortableStarted(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdSshAgentStarted(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdQueryPalette(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdGetTaskCmd(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	CESERVER_REQ* cmdIsAnsiExecAllowed(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
	//CESERVER_REQ* cmdAssert(LPVOID pInst, CESERVER_REQ* pIn, UINT nDataSize);
};
