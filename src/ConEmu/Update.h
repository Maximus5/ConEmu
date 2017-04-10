
/*
Copyright (c) 2011-2017 Maximus5
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
#include "../ConEmuCD/DownloaderCall.h"

#define UPD_PROGRESS_CONFIRM_DOWNLOAD  5
#define UPD_PROGRESS_DOWNLOAD_START   10
#define UPD_PROGRESS_CONFIRM_UPDATE   98
#define UPD_PROGRESS_EXIT_AND_UPDATE  99

struct ConEmuUpdateSettings;
class CConEmuUpdate;
class MSection;
struct MSectionSimple;

extern CConEmuUpdate* gpUpd;

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

	UINT mb_ManualCallMode; // FALSE, TRUE, 2 (click on TSA icon)
	ConEmuUpdateSettings* mp_Set;

	long mn_InShowMsgBox;
	long mn_ErrorInfoCount;
	long mn_ErrorInfoSkipCount;

	MSectionSimple* mps_ErrorLock;
	wchar_t* ms_LastErrorInfo;

	wchar_t* mpsz_DeleteIniFile;
	wchar_t* mpsz_DeletePackageFile;
	wchar_t* mpsz_DeleteBatchFile;
	void DeleteBadTempFiles();

	wchar_t* mpsz_PendingPackageFile;
	wchar_t* mpsz_PendingBatchFile;

	CEStr ms_TempUpdateVerLocation;

	CEStr ms_TemplFilename;
	CEStr ms_SourceFull;
	CEStr ms_VersionUrlCheck;

	static DWORD WINAPI CheckThreadProc(LPVOID lpParameter);
	DWORD CheckProcInt();
	void GetVersionsFromIni(LPCWSTR pszUpdateVerLocation, wchar_t (&szServer)[100], wchar_t (&szServerRA)[100], wchar_t (&szInfo)[100]);

	wchar_t* CreateTempFile(LPCWSTR asDir, LPCWSTR asFileNameTempl, HANDLE& hFile);
	wchar_t* CreateBatchFile(LPCWSTR asPackage);

	bool IsLocalFile(LPWSTR& asPathOrUrl);
	bool IsLocalFile(LPCWSTR& asPathOrUrl);

	bool bNeedRunElevation;

	BOOL DownloadFile(LPCWSTR asSource, LPCWSTR asTarget, DWORD& crc, BOOL abPackage = FALSE, LARGE_INTEGER* rpSize = NULL);

	void ReportError(LPCWSTR asFormat, DWORD nErrCode);
	void ReportError(LPCWSTR asFormat, LPCWSTR asArg, DWORD nErrCode);
	void ReportError(LPCWSTR asFormat, LPCWSTR asArg1, LPCWSTR asArg2, DWORD nErrCode);
	void ReportBrokenIni(LPCWSTR asSection, LPCWSTR asName, LPCWSTR asIniUrl, LPCWSTR asIniLocal);

	void ReportErrorInt(wchar_t* asErrorInfo);

public:
	CConEmuUpdate();
	~CConEmuUpdate();

	void StartCheckProcedure(UINT abShowMessages);
	void StopChecking();

	static bool LocalUpdate(LPCWSTR asDownloadedPackage);
	static bool IsUpdatePackage(LPCWSTR asFilePath);
	static bool NeedRunElevation();

	enum UpdateStep
	{
		us_NotStarted = 0,
		us_Check,
		us_ConfirmDownload,
		us_Downloading,
		us_ConfirmUpdate,
		us_PostponeUpdate,
		us_ExitAndUpdate,
	};
	UpdateStep InUpdate();

	short GetUpdateProgress();

	wchar_t* GetCurVerInfo();

protected:
	void RequestTerminate();
	bool mb_RequestTerminate;
	UpdateStep m_UpdateStep;
	bool mb_NewVersionAvailable;
	wchar_t ms_NewVersion[64], ms_OurVersion[64], ms_SkipVersion[64];
	wchar_t ms_VerOnServer[100]; // Information about available server versions
	wchar_t ms_VerOnServerRA[100]; // Information about available server versions (right aligned)
	wchar_t ms_CurVerInfo[100];  // Version + stable/preview/alpha
	wchar_t ms_DefaultTitle[128];
	wchar_t* mpsz_ConfirmSource;
	static LRESULT QueryConfirmationCallback(LPARAM lParam);
	static LRESULT RequestExitUpdate(LPARAM);
	static LRESULT ShowLastError(LPARAM apObj);
	int QueryConfirmation(UpdateStep step);
	int QueryConfirmationDownload();
	int QueryConfirmationUpdate();
	int QueryConfirmationNoNewVer();
	static LRESULT QueryRetryVersionCheck(LPARAM lParam);
	void WaitAllInstances();
	bool Check7zipInstalled();
	#if 0
	bool CanUpdateInstallation();
	#endif
	bool StartLocalUpdate(LPCWSTR asDownloadedPackage);
	bool LoadVersionInfoFromServer();
	bool LoadPackageFromServer();
	wchar_t* CreateVersionOnServerInfo(bool abRightAligned, LPCWSTR asSuffix = NULL);
};
