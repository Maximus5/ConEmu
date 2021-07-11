
/*
Copyright (c) 2011-present Maximus5
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
#include "../common/MSectionSimple.h"
#include "TempFile.h"
#include "UpdateSet.h"

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
	bool mb_InCheckProcedure{ false };
	DWORD mn_CheckThreadId{ 0 };
	HANDLE mh_CheckThread{ nullptr };

	//HANDLE mh_StopThread;

	bool mb_InetMode{ false };
	bool mb_DroppedMode{ false };
	DWORD mn_InternetContentReady{ 0 };
	DWORD mn_PackageSize{ 0 };

	struct wininet
	{
		CConEmuUpdate* pUpd{ nullptr };
		HMODULE hDll{ nullptr };
		DownloadCommand_t DownloadCommand{ nullptr };

		bool Init(CConEmuUpdate* apUpd);
		bool Deinit(bool bFull);
		void SetCallback(CEDownloadCommand cbk, FDownloadCallback pfnCallback, LPARAM lParam);
	};
	wininet Inet{};

	static void WINAPI ProgressCallback(const CEDownloadInfo* pError);
	static void WINAPI ErrorCallback(const CEDownloadInfo* pError);
	static void WINAPI LogCallback(const CEDownloadInfo* pError);

	UpdateCallMode m_ManualCallMode{ UpdateCallMode::Automatic };
	ConEmuUpdateSettings* mp_Set{ nullptr };

	long mn_InShowMsgBox{ 0 };
	long mn_ErrorInfoCount{ 0 };
	long mn_ErrorInfoSkipCount{ 0 };

	MSectionSimple ms_ErrorLock{true};
	CEStr ms_LastErrorInfo;

	TempFile msz_DeletePackageFile;
	TempFile msz_DeleteBatchFile;
	void DeleteBadTempFiles();

	TempFile msz_PendingPackageFile;
	TempFile msz_PendingBatchFile;

	TempFile ms_TempUpdateVerLocation;

	CEStr ms_TemplFilename;
	CEStr ms_SourceFull;
	CEStr ms_VersionUrlCheck;

	static DWORD WINAPI CheckThreadProc(LPVOID lpParameter);
	DWORD CheckProcInt();
	void GetVersionsFromIni(LPCWSTR pszUpdateVerLocation, wchar_t (&szServer)[100], wchar_t (&szServerRA)[100], wchar_t (&szInfo)[100]);

	TempFile CreateBatchFile(LPCWSTR asPackage);

	bool IsLocalFile(LPWSTR& asPathOrUrl);
	bool IsLocalFile(LPCWSTR& asPathOrUrl);

	bool bNeedRunElevation{ false };

	BOOL DownloadFile(LPCWSTR asSource, LPCWSTR asTarget, DWORD& crc, BOOL abPackage = FALSE, LARGE_INTEGER* rpSize = nullptr);

	void ReportError(LPCWSTR asFormat, DWORD nErrCode);
	void ReportError(LPCWSTR asFormat, LPCWSTR asArg, DWORD nErrCode);
	void ReportError(LPCWSTR asFormat, LPCWSTR asArg1, LPCWSTR asArg2, DWORD nErrCode);
	void ReportBrokenIni(LPCWSTR asSection, LPCWSTR asName, LPCWSTR asIniUrl, LPCWSTR asIniLocal);

	void ReportErrorInt(const ErrorInfo& error);

public:
	CConEmuUpdate();
	~CConEmuUpdate();

	void StartCheckProcedure(UpdateCallMode callMode);
	void StopChecking();

	static bool LocalUpdate(LPCWSTR asDownloadedPackage);
	static bool IsUpdatePackage(LPCWSTR asFilePath);
	static bool NeedRunElevation();

	enum class UpdateStep
	{
		NotStarted = 0,
		Check,
		ConfirmDownload,
		Downloading,
		ConfirmUpdate,
		PostponeUpdate,
		ExitAndUpdate,
	};
	UpdateStep InUpdate();

	short GetUpdateProgress();

	CEStr GetCurVerInfo();

protected:
	bool mb_RequestTerminate{ false };
	UpdateStep m_UpdateStep{ UpdateStep::NotStarted };
	bool mb_NewVersionAvailable{ false };
	wchar_t ms_NewVersion[64] = L"";
	wchar_t ms_OurVersion[64] = L"";
	wchar_t ms_SkipVersion[64] = L"";
	wchar_t ms_VerOnServer[100] = L""; // Information about available server versions
	wchar_t ms_VerOnServerRA[100] = L""; // Information about available server versions (right aligned)
	wchar_t ms_CurVerInfo[100] = L"";  // Version + stable/preview/alpha
	wchar_t ms_DefaultTitle[128] = L"";
	CEStr msz_ConfirmSource;
	
	void RequestTerminate();
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
	CEStr CreateVersionOnServerInfo(bool abRightAligned, LPCWSTR asSuffix = nullptr);
};
