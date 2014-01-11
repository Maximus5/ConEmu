
/*
Copyright (c) 2011-2014 Maximus5
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

#define DOWNLOADER_IMPORTS
#include "../ConEmuCD/Downloader.h"

struct ConEmuUpdateSettings;
class CConEmuUpdate;
class MSection;

extern CConEmuUpdate* gpUpd;

class CWinInet;

class CConEmuUpdate
{
protected:
	BOOL mb_InCheckProcedure;
	DWORD mn_CheckThreadId;
	HANDLE mh_CheckThread;
	
	//HANDLE mh_StopThread;

	bool mb_InetMode, mb_DroppedMode;
	DWORD mn_InternetContentReady, mn_PackageSize;

	struct wininet
	{
		CConEmuUpdate* pUpd;
		HMODULE hDll;
		DownloadCommand_t DownloadCommand;
		bool Init(CConEmuUpdate* apUpd);
		bool Deinit(bool bFull);
		void SetCallback(CEDownloadCommand cbk, FDownloadCallback pfnCallback, LPARAM lParam);
	} Inet;

	static void WINAPI ProgressCallback(const CEDownloadInfo* pError);
	static void WINAPI ErrorCallback(const CEDownloadInfo* pError);
	static void WINAPI LogCallback(const CEDownloadInfo* pError);
	
	BOOL mb_ManualCallMode;
	ConEmuUpdateSettings* mp_Set;
	
	bool mb_InShowLastError;
	wchar_t* ms_LastErrorInfo;
	MSection* mp_LastErrorSC;

	wchar_t* mpsz_DeleteIniFile;
	wchar_t* mpsz_DeletePackageFile;
	wchar_t* mpsz_DeleteBatchFile;
	void DeleteBadTempFiles();

	wchar_t* mpsz_PendingPackageFile;
	wchar_t* mpsz_PendingBatchFile;
	
	static DWORD WINAPI CheckThreadProc(LPVOID lpParameter);
	DWORD CheckProcInt();
	
	wchar_t* CreateTempFile(LPCWSTR asDir, LPCWSTR asFileNameTempl, HANDLE& hFile);
	wchar_t* CreateBatchFile(LPCWSTR asPackage);
	
	bool IsLocalFile(LPWSTR& asPathOrUrl);
	bool IsLocalFile(LPCWSTR& asPathOrUrl);

	bool bNeedRunElevation;
	bool NeedRunElevation();
	
	BOOL DownloadFile(LPCWSTR asSource, LPCWSTR asTarget, HANDLE hDstFile, DWORD& crc, BOOL abPackage = FALSE);
	
	void ReportError(LPCWSTR asFormat, DWORD nErrCode);
	void ReportError(LPCWSTR asFormat, LPCWSTR asArg, DWORD nErrCode);
	void ReportError(LPCWSTR asFormat, LPCWSTR asArg1, LPCWSTR asArg2, DWORD nErrCode);
	void ReportBrokenIni(LPCWSTR asSection, LPCWSTR asName, LPCWSTR asIni);

	void ReportErrorInt(wchar_t* asErrorInfo);

public:
	CConEmuUpdate();
	~CConEmuUpdate();
	
	void StartCheckProcedure(BOOL abShowMessages);
	void StopChecking();
	void ShowLastError();
	bool ShowConfirmation();
	
	static bool LocalUpdate(LPCWSTR asDownloadedPackage);
	static bool IsUpdatePackage(LPCWSTR asFilePath);

	enum UpdateStep
	{
		us_NotStarted = 0,
		us_Check = 1,
		us_ConfirmDownload = 2,
		us_Downloading = 3,
		us_ConfirmUpdate = 4,
		us_ExitAndUpdate = 5,
	};
	UpdateStep InUpdate();

	short GetUpdateProgress();

protected:
	void RequestTerminate();
	bool mb_RequestTerminate;
	UpdateStep m_UpdateStep;
	wchar_t ms_NewVersion[64], ms_CurVersion[64], ms_SkipVersion[64];
	wchar_t ms_DefaultTitle[128];
	bool QueryConfirmation(UpdateStep step, LPCWSTR asParm = NULL);
	bool QueryConfirmationInt(LPCWSTR asConfirmInfo);
	void WaitAllInstances();
	bool Check7zipInstalled();
	#if 0
	bool CanUpdateInstallation();
	#endif
	bool StartLocalUpdate(LPCWSTR asDownloadedPackage);
};
