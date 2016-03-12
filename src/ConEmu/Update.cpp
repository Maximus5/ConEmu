
/*
Copyright (c) 2009-2016 Maximus5
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
#include "Header.h"
#include <Wininet.h>
#pragma warning(disable: 4091)
#include <shlobj.h>
#pragma warning(default: 4091)
#include "Update.h"
#include "UpdateSet.h"
#include "Options.h"
#include "OptionsClass.h"
#include "ConEmu.h"
#include "TrayIcon.h"
#include "version.h"

#include <commctrl.h>
#include "AboutDlg.h"
#include "ConfirmDlg.h"
#include "../common/MSection.h"
#include "../common/WFiles.h"
#include "../common/WThreads.h"
#include "../common/WUser.h"

CConEmuUpdate* gpUpd = NULL;

#define UPDATETHREADTIMEOUT 2500

#define sectionConEmuStable  L"ConEmu_Stable_2"
#define sectionConEmuPreview L"ConEmu_Preview_2"
#define sectionConEmuDevel   L"ConEmu_Devel_2"

#define szWhatsNewLabel L"Whats new (project wiki page)"

#define szRetryVersionIniCheck \
	L"ConEmu is unable to load current version information from servers.\n" \
	L"You may either check and download new versions manually from\n" \
	CEDOWNLPAGE /* http://www.fosshub.com/ConEmu.html */ L"\n" \
	L"or let ConEmu retry the check.\n"

#define szRetryPackageDownload \
	L"ConEmu is unable to download update package.\n" \
	L"You may either download new versions manually from\n" \
	CEDOWNLPAGE /* http://www.fosshub.com/ConEmu.html */ L"\n" \
	L"or let ConEmu retry the downloading.\n"



CConEmuUpdate::CConEmuUpdate()
{
	_ASSERTE(isMainThread());

	mb_InCheckProcedure = FALSE;
	mn_CheckThreadId = 0;
	mh_CheckThread = NULL;
	mb_RequestTerminate = false;
	mp_Set = NULL;
	mps_ErrorLock = new MSectionSimple(true);
	ms_LastErrorInfo = NULL;
	mn_InShowMsgBox = 0;
	mn_ErrorInfoCount = 0;
	mn_ErrorInfoSkipCount = 0;
	mb_InetMode = false;
	mb_DroppedMode = false;
	mn_InternetContentReady = mn_PackageSize = 0;
	mpsz_DeleteIniFile = NULL;
	mpsz_DeletePackageFile = mpsz_DeleteBatchFile = NULL;
	mpsz_PendingPackageFile = mpsz_PendingBatchFile = NULL;
	m_UpdateStep = us_NotStarted;
	mb_NewVersionAvailable = false;
	ms_NewVersion[0] = ms_SkipVersion[0] = 0;
	ms_VerOnServer[0] = ms_VerOnServerRA[0] = ms_CurVerInfo[0] = 0;
	mpsz_ConfirmSource = NULL;

	wchar_t szVer4[8] = L""; lstrcpyn(szVer4, _T(MVV_4a), countof(szVer4));
	_wsprintf(ms_OurVersion, SKIPLEN(countof(ms_OurVersion)) L"%02u%02u%02u%s",
		(MVV_1%100), MVV_2, MVV_3, szVer4);

	lstrcpyn(ms_DefaultTitle, gpConEmu->GetDefaultTitle(), countof(ms_DefaultTitle));

	ZeroStruct(Inet);
	mb_ManualCallMode = FALSE;
	bNeedRunElevation = false;
}

CConEmuUpdate::~CConEmuUpdate()
{
	if (mh_CheckThread)
	{
		DWORD nWait;
		if ((nWait = WaitForSingleObject(mh_CheckThread, 0)) == WAIT_TIMEOUT)
		{
			RequestTerminate();
			nWait = WaitForSingleObject(mh_CheckThread, UPDATETHREADTIMEOUT);
		}

		if (nWait != WAIT_OBJECT_0)
		{
			_ASSERTE(FALSE && "Terminating Updater(mh_CheckThread) thread");
			apiTerminateThread(mh_CheckThread, 100);
		}

		CloseHandle(mh_CheckThread);
		mh_CheckThread = NULL;
	}

	//if (mh_StopThread)
	//{
	//	CloseHandle(mh_StopThread);
	//	mh_StopThread = NULL;
	//}

	DeleteBadTempFiles();

	Inet.Deinit(true);

	SafeFree(mpsz_ConfirmSource);

	if (((m_UpdateStep == us_ExitAndUpdate) || (m_UpdateStep == us_PostponeUpdate)) && mpsz_PendingBatchFile)
	{
		WaitAllInstances();

		wchar_t *pszCmd = lstrdup(L"cmd.exe"); // Мало ли что в ComSpec пользователь засунул...
		size_t cchParmMax = lstrlen(mpsz_PendingBatchFile)+16;
		wchar_t *pszParm = (wchar_t*)calloc(cchParmMax,sizeof(*pszParm));
		// Обязательно двойное окавычивание. cmd.exe отбрасывает кавычки,
		// и при наличии разделителей (пробелы, скобки,...) получаем проблемы
		_wsprintf(pszParm, SKIPLEN(cchParmMax) L"/c \"\"%s\"\"", mpsz_PendingBatchFile);

		// Наверное на Elevated процесс это не распространится, но для четкости - взведем флажок
		SetEnvironmentVariable(ENV_CONEMU_INUPDATE_W, ENV_CONEMU_INUPDATE_YES);

		// ghWnd уже закрыт
		INT_PTR nShellRc = (INT_PTR)ShellExecute(NULL, bNeedRunElevation ? L"runas" : L"open", pszCmd, pszParm, NULL, SW_SHOWMINIMIZED);
		if (nShellRc <= 32)
		{
			wchar_t szErrInfo[MAX_PATH*4];
			_wsprintf(szErrInfo, SKIPLEN(countof(szErrInfo))
				L"Failed to start update batch\n%s\nError code=%i", mpsz_PendingBatchFile, (int)nShellRc);
			MsgBox(szErrInfo, MB_ICONSTOP|MB_SYSTEMMODAL, L"ConEmu", NULL, false);

			DeleteFile(mpsz_PendingBatchFile);
			if (!(mp_Set && mp_Set->isUpdateLeavePackages))
				DeleteFile(mpsz_PendingPackageFile);
		}
		SafeFree(pszCmd);
		SafeFree(pszParm);
	}
	SafeFree(mpsz_PendingBatchFile);
	SafeFree(mpsz_PendingPackageFile);

	if (mp_Set)
	{
		delete mp_Set;
		mp_Set = NULL;
	}

	SafeDelete(mps_ErrorLock);
	SafeFree(ms_LastErrorInfo);
}

void CConEmuUpdate::StartCheckProcedure(BOOL abShowMessages)
{
	//DWORD nWait = WAIT_OBJECT_0;

	if (InUpdate() != us_NotStarted)
	{
		// Already in update procedure
		if ((m_UpdateStep == us_PostponeUpdate) || (m_UpdateStep == us_ExitAndUpdate))
		{
			if (gpConEmu)
			{
				// Повторно?
				gpConEmu->CallMainThread(true, RequestExitUpdate, 0);
			}
		}
		else if (abShowMessages)
		{
			MBoxError(L"Checking for updates already started");
		}
		return;
	}

	gpSet->UpdSet.dwLastUpdateCheck = GetTickCount();

	// Сразу проверим, как нужно будет запускаться
	bNeedRunElevation = NeedRunElevation();

	mb_RequestTerminate = false;
	//if (!mh_StopThread)
	//	mh_StopThread = CreateEvent(NULL, TRUE/*manual*/, FALSE, NULL);
	//ResetEvent(mh_StopThread);

	// Запомнить текущие параметры обновления
	if (!mp_Set)
		mp_Set = new ConEmuUpdateSettings;
	mp_Set->LoadFrom(&gpSet->UpdSet);

	mb_ManualCallMode = abShowMessages;

	// Don't clear it here, must be cleared by display procedure
	_ASSERTE(ms_LastErrorInfo == NULL);

	wchar_t szReason[128];
	wchar_t szErrMsg[255];

	if (!mp_Set->UpdatesAllowed(szReason))
	{
		wcscpy_c(szErrMsg, L"Updates are not enabled in ConEmu settings\r\n");
		wcscat_c(szErrMsg, szReason);
		DisplayLastError(szErrMsg, -1);
		return;
	}

	if (gpConEmu)
		gpConEmu->LogString(abShowMessages ? L"Update: starting in manual mode" : L"Update: starting in automatic mode");

	mb_InCheckProcedure = TRUE;
	mh_CheckThread = apiCreateThread(CheckThreadProc, this, &mn_CheckThreadId, "AutoUpdateThread");
	if (!mh_CheckThread)
	{
		mb_InCheckProcedure = FALSE;
		DWORD nErrCode = GetLastError();
		wcscpy_c(szErrMsg, L"ConEmu automatic update check failed!\r\n");
		if (nErrCode == ERROR_ACCESS_DENIED) wcscat_c(szErrMsg, L"Check your antivirus software\r\n");
		wcscat_c(szErrMsg, L"\r\nCreateThread(CheckThreadProc) failed\r\n");
		DisplayLastError(szErrMsg, nErrCode);
		return;
	}
	// OK
}

void CConEmuUpdate::RequestTerminate()
{
	if (gpConEmu)
		gpConEmu->LogString(L"Update: TERMINATE WAS REQUESTED");
	Inet.DownloadCommand(dc_RequestTerminate, 0, NULL);
	mb_RequestTerminate = true;
}

void CConEmuUpdate::StopChecking()
{
	if (MsgBox(L"Are you sure, stop updates checking?", MB_SYSTEMMODAL|MB_ICONQUESTION|MB_YESNO, ms_DefaultTitle, NULL, false) != IDYES)
		return;

	RequestTerminate();

	if (mh_CheckThread)
	{
		DWORD nWait;
		if ((nWait = WaitForSingleObject(mh_CheckThread, 0)) == WAIT_TIMEOUT)
		{
			RequestTerminate();
			nWait = WaitForSingleObject(mh_CheckThread, UPDATETHREADTIMEOUT);
		}

		if (nWait != WAIT_OBJECT_0)
		{
			_ASSERTE(FALSE && "Terminating Updater(mh_CheckThread) thread");
			apiTerminateThread(mh_CheckThread, 100);
		}
	}

	DeleteBadTempFiles();
	m_UpdateStep = us_NotStarted;

	if (mpsz_PendingBatchFile)
	{
		if (*mpsz_PendingBatchFile)
			DeleteFile(mpsz_PendingBatchFile);
		SafeFree(mpsz_PendingBatchFile);
	}

	if (mpsz_PendingPackageFile)
	{
		if (*mpsz_PendingPackageFile && !(mp_Set && mp_Set->isUpdateLeavePackages))
			DeleteFile(mpsz_PendingPackageFile);
		SafeFree(mpsz_PendingPackageFile);
	}
}

LRESULT CConEmuUpdate::ShowLastError(LPARAM apObj)
{
	_ASSERTE(gpUpd == (CConEmuUpdate*)apObj);
	// Now we store error text in member var protected by CS
	CEStr szError; // (wchar_t*)apErrText;
	{
		MSectionLockSimple CS;
		if (CS.Lock(gpUpd->mps_ErrorLock, 5000))
		{
			szError.Set(gpUpd->ms_LastErrorInfo);
			SafeFree(gpUpd->ms_LastErrorInfo);
		}
		else
		{
			szError.Set(L"Updater error was occurred but we failed to lock it");
		}
	}

	if (!szError.IsEmpty())
	{
		InterlockedIncrement(&gpUpd->mn_InShowMsgBox);

		MsgBox(szError, MB_ICONINFORMATION, gpConEmu?gpConEmu->GetDefaultTitle():L"ConEmu Update");

		InterlockedDecrement(&gpUpd->mn_InShowMsgBox);
	}

	return 0;
}

LRESULT CConEmuUpdate::QueryConfirmationCallback(LPARAM lParam)
{
	if (!gpUpd || gpUpd->mb_RequestTerminate)
		return 0;
	return gpUpd->QueryConfirmation((UpdateStep)LOWORD(lParam));
}

// Worker thread
DWORD CConEmuUpdate::CheckThreadProc(LPVOID lpParameter)
{
	DWORD nRc = 0;
	CConEmuUpdate *pUpdate = (CConEmuUpdate*)lpParameter;
	_ASSERTE(pUpdate!=NULL && pUpdate->mn_CheckThreadId==GetCurrentThreadId());

	SAFETRY
	{
		nRc = pUpdate->CheckProcInt();
	}
	SAFECATCH
	{
		pUpdate->ReportError(L"Exception in CheckProcInt", 0);
	}

	pUpdate->mb_InCheckProcedure = FALSE;
	// May be null, if update package was dropped on ConEmu icon
	if (gpConEmu && ghWnd)
	{
		gpConEmu->UpdateProgress();
	}
	return nRc;
}

#if 0
bool CConEmuUpdate::CanUpdateInstallation()
{
	if (UpdateDownloadSetup() == 1)
	{
		// Если через Setupper - то msi сам разберется и ругнется когда надо
		return true;
	}

	// Раз дошли сюда - значит ConEmu был просто "распакован"

	if (IsUserAdmin())
	{
		// ConEmu запущен "Под администратором", проверки не нужны
		return true;
	}

	wchar_t szTestFile[MAX_PATH*2];
	wcscpy_c(szTestFile, gpConEmu->ms_ConEmuExeDir);
	wcscat_c(szTestFile, L"\\ConEmuUpdate.check");

	HANDLE hFile = CreateFile(szTestFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_TEMPORARY, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD nErr = GetLastError();
		wcscpy_c(szTestFile, L"Can't update installation folder!\r\n");
		wcscat_c(szTestFile, gpConEmu->ms_ConEmuExeDir);
		DisplayLastError(szTestFile, nErr);
		return false;
	}
	CloseHandle(hFile);
	DeleteFile(szTestFile);

	// OK
	return true;
}
#endif

// static
bool CConEmuUpdate::LocalUpdate(LPCWSTR asDownloadedPackage)
{
	bool bOk = false;

	if (!gpUpd)
		gpUpd = new CConEmuUpdate;

	if (gpUpd)
		bOk = gpUpd->StartLocalUpdate(asDownloadedPackage);

	return bOk;
}

// static
bool CConEmuUpdate::IsUpdatePackage(LPCWSTR asFilePath)
{
	bool bIsUpdatePackage = false;

	if (asFilePath && *asFilePath && lstrlen(asFilePath) <= MAX_PATH)
	{
		wchar_t szName[MAX_PATH+1];
		LPCWSTR pszName = PointToName(asFilePath);
		if (pszName && *pszName)
		{
			lstrcpyn(szName, pszName, countof(szName));
			CharLowerBuff(szName, lstrlen(szName));
			LPCWSTR pszExt = PointToExt(szName);
			if (pszExt)
			{
				if ((wcsncmp(szName, L"conemupack.", 11) == 0)
					&& (wcscmp(pszExt, L".7z") == 0))
				{
					bIsUpdatePackage = true;
				}
				else if ((wcsncmp(szName, L"conemusetup.", 12) == 0)
					&& (wcscmp(pszExt, L".exe") == 0))
				{
					bIsUpdatePackage = true;
				}
			}
		}
	}

	return bIsUpdatePackage;
}

bool CConEmuUpdate::StartLocalUpdate(LPCWSTR asDownloadedPackage)
{
	bool bRc = false;
	LPCWSTR pszName, pszExt;
	HANDLE hTarget = NULL;
	wchar_t *pszLocalPackage = NULL, *pszBatchFile = NULL;
	DWORD nLocalCRC = 0;
	BOOL lbDownloadRc = FALSE, lbExecuteRc = FALSE;
	int iConfirmUpdate = -1;

	LPCWSTR pszPackPref = L"conemupack.";
	size_t lnPackPref = _tcslen(pszPackPref);
	LPCWSTR pszSetupPref = L"conemusetup.";
	size_t lnSetupPref = _tcslen(pszSetupPref);

	_ASSERTE(gpConEmu && isMainThread());

	if (InUpdate() != us_NotStarted)
	{
		MBoxError(L"Checking for updates already started");
		goto wrap;
	}

	if (mb_InCheckProcedure)
	{
		Assert(mb_InCheckProcedure==FALSE);
		goto wrap;
	}

	DeleteBadTempFiles();
	Inet.Deinit(true);

	pszName = PointToName(asDownloadedPackage);
	pszExt = PointToExt(pszName);
	if (!pszName || !*pszName || !pszExt || !*pszExt)
	{
		AssertMsg(L"Invalid asDownloadedPackage");
		goto wrap;
	}

	// Запомнить текущие параметры обновления
	if (!mp_Set)
		mp_Set = new ConEmuUpdateSettings;
	mp_Set->LoadFrom(&gpSet->UpdSet);

	mb_ManualCallMode = TRUE;

	mb_RequestTerminate = false;

	// Don't clear it here, must be cleared by display procedure
	_ASSERTE(ms_LastErrorInfo == NULL);

	ms_NewVersion[0] = 0;

	if ((lstrcmpni(pszName, pszPackPref, lnPackPref) == 0)
		&& (lstrcmpi(pszExt, L".7z") == 0)
		&& (((pszExt - pszName) - lnPackPref + 1) < sizeof(ms_NewVersion)))
	{
		// Check it was NOT installed with "Setupper"
		if (mp_Set->UpdateDownloadSetup() == 1)
		{
			DontEnable de;
			LPCWSTR pszConfirm = L"ConEmu was installed with setup!\nAre you sure to update installation with 7zip?";
			int iBtn = MsgBox(pszConfirm, MB_ICONEXCLAMATION|MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_YESNO|MB_DEFBUTTON2, ms_DefaultTitle, NULL, false);
			if (iBtn != IDYES)
			{
				goto wrap;
			}
		}

		if (!Check7zipInstalled())
			goto wrap; // Error already reported

		// Forcing usage of 7zip package!
		mp_Set->isUpdateDownloadSetup = 2;

		//if (!CanUpdateInstallation())
		//{
		//	// Значит 7zip обломается при попытке распаковки
		//	goto wrap;
		//}

		// OK
		size_t nLen = (pszExt - pszName) - lnPackPref;
		wmemmove(ms_NewVersion, pszName+lnPackPref, nLen);
		ms_NewVersion[nLen] = 0;
	}
	else if ((lstrcmpni(pszName, pszSetupPref, lnSetupPref) == 0)
		&& (lstrcmpi(pszExt, L".exe") == 0)
		&& (((pszExt - pszName) - lnSetupPref + 1) < sizeof(ms_NewVersion)))
	{
		// Must be installed with "Setupper"
		if (mp_Set->UpdateDownloadSetup() != 1)
		{
			MBoxError(L"ConEmu was not installed with setup! Can't update!");
			goto wrap;
		}

		// OK
		size_t nLen = (pszExt - pszName) - lnSetupPref;
		wmemmove(ms_NewVersion, pszName+lnSetupPref, nLen);
		ms_NewVersion[nLen] = 0;
	}
	else
	{
		AssertMsg(L"Invalid asDownloadedPackage (2)");
		goto wrap;
	}


	// Сразу проверим, как нужно будет запускаться
	bNeedRunElevation = NeedRunElevation();

	// StartLocalUpdate - запуск обновления из локального пакета

	mb_InetMode = false;
	mb_DroppedMode = true;



	pszLocalPackage = CreateTempFile(mp_Set->szUpdateDownloadPath, PointToName(asDownloadedPackage), hTarget);
	CloseHandle(hTarget);
	if (!pszLocalPackage)
		goto wrap;

	lbDownloadRc = DownloadFile(asDownloadedPackage, pszLocalPackage, nLocalCRC, TRUE);
	if (!lbDownloadRc)
		goto wrap;


	if (mb_RequestTerminate)
		goto wrap;

	pszBatchFile = CreateBatchFile(pszLocalPackage);
	if (!pszBatchFile)
		goto wrap;

	iConfirmUpdate = gpConEmu->CallMainThread(true, QueryConfirmationCallback, us_ConfirmUpdate);
	if ((iConfirmUpdate <= 0) || (iConfirmUpdate == IDCANCEL))
	{
		goto wrap;
	}

	Assert(mb_ManualCallMode==TRUE);
	Assert(mpsz_PendingBatchFile==NULL);

	mpsz_PendingPackageFile = pszLocalPackage;
	pszLocalPackage = NULL;
	mpsz_PendingBatchFile = pszBatchFile;
	pszBatchFile = NULL;
	if (iConfirmUpdate == IDYES)
	{
		m_UpdateStep = us_ExitAndUpdate;
		if (gpConEmu) gpConEmu->CallMainThread(false, RequestExitUpdate, 0);
	}
	else
	{
		m_UpdateStep = us_PostponeUpdate;
		if (gpConEmu) gpConEmu->UpdateProgress();
	}
	lbExecuteRc = TRUE;

wrap:
	_ASSERTE(mpsz_DeletePackageFile==NULL);
	mpsz_DeletePackageFile = NULL;
	if (pszLocalPackage)
	{
		if (*pszLocalPackage && (!lbDownloadRc || (!lbExecuteRc && !mp_Set->isUpdateLeavePackages)))
			mpsz_DeletePackageFile = pszLocalPackage;
			//DeleteFile(pszLocalPackage);
		else
			SafeFree(pszLocalPackage);
	}

	_ASSERTE(mpsz_DeleteBatchFile==NULL);
	mpsz_DeleteBatchFile = NULL;
	if (pszBatchFile)
	{
		if (*pszBatchFile && !lbExecuteRc)
			mpsz_DeleteBatchFile = pszBatchFile;
			//DeleteFile(pszBatchFile);
		else
			SafeFree(pszBatchFile);
	}

	if (!lbExecuteRc)
	{
		m_UpdateStep = us_NotStarted;
		mb_DroppedMode = false;
	}

	return bRc;
}

void CConEmuUpdate::GetVersionsFromIni(LPCWSTR pszUpdateVerLocation, wchar_t (&szServer)[100], wchar_t (&szServerRA)[100], wchar_t (&szInfo)[100])
{
	wchar_t szTest[64]; // Дописать stable/preview/alpha
	bool bDetected = false, bNewer;

	wcscpy_c(szInfo, ms_OurVersion);

	struct {
		LPCWSTR szSect, szPref, szName;
	} Vers[] = {
		{sectionConEmuStable,        CV_Stable L":\t",  L" " CV_STABLE },
		{sectionConEmuPreview, L"\n" CV_Preview L":\t", L" " CV_PREVIEW},
		{sectionConEmuDevel,   L"\n" CV_Devel L":\t",   L" " CV_DEVEL  }
	};

	szServer[0] = 0;
	szServerRA[0] = 0;

	for (size_t i = 0; i < countof(Vers); i++)
	{
		wcscat_c(szServer, Vers[i].szPref);
		if (GetPrivateProfileString(Vers[i].szSect, L"version", L"", szTest, countof(szTest), pszUpdateVerLocation))
		{
			bNewer = (lstrcmp(szTest, ms_OurVersion) >= 0);
			if (!bDetected && bNewer)
			{
				bDetected = true;
				wcscat_c(szInfo, Vers[i].szName);
			}
			szTest[10] = 0;
			wcscat_c(szServer, szTest);
			wcscat_c(szServerRA, L"    ");
			wcscat_c(szServerRA, szTest);
			wcscat_c(szServerRA, Vers[i].szName);
			if (bNewer)
			{
				wcscat_c(szServer, (lstrcmp(szTest, ms_OurVersion) > 0) ? L" (newer)" : L" (equal)");
				wcscat_c(szServerRA, (lstrcmp(szTest, ms_OurVersion) > 0) ? L" (newer)" : L" (equal)");
			}
			wcscat_c(szServerRA, L"\n");
		}
		else
		{
			wcscat_c(szServer, L"<Not found>");
		}
	}

	if (szServerRA[0] == 0)
	{
		wcscpy_c(szServerRA, L"<Not found>");
	}
}

DWORD CConEmuUpdate::CheckProcInt()
{
	BOOL lbDownloadRc = FALSE, lbExecuteRc = FALSE;
	wchar_t *pszLocalPackage = NULL, *pszBatchFile = NULL;
	wchar_t *pszSource, *pszEnd;
	DWORD nSrcLen, nSrcCRC, nLocalCRC = 0;
	bool lbSourceLocal;
	int iConfirmUpdate = -1;

	#if 0
	// Under Windows7 we may get a debug warning
	// if conhost.exe was not found yet for all
	// started consoles...
	if (!mb_ManualCallMode)
		Sleep(2500);
	#endif

	_ASSERTE(m_UpdateStep==us_NotStarted);
	m_UpdateStep = us_Check;

	DeleteBadTempFiles();

	if (gpConEmu)
		gpConEmu->LogString(L"Update: Initializing internet");

	// This implies Inet.Deinit(false) too
	if (!Inet.Init(this))
	{
		if (gpConEmu)
			gpConEmu->LogString(L"Update: Internet failed");
		goto wrap;
	}

	// Download and parse latest version information
	while (!LoadVersionInfoFromServer())
	{
		LRESULT lRetryRc = (!gpConEmu || !mb_ManualCallMode || mb_RequestTerminate)
			? IDCANCEL
			: gpConEmu->CallMainThread(true/*bSync*/, CConEmuUpdate::QueryRetryVersionCheck, (LPARAM)szRetryVersionIniCheck);

		// Error must be already shown or logged, clear it
		if (ms_LastErrorInfo)
		{
			MSectionLockSimple CS;
			if (CS.Lock(mps_ErrorLock, 5000))
			{
				SafeFree(ms_LastErrorInfo);
			}
		}

		if (lRetryRc != IDRETRY)
		{
			goto wrap;
		}
	}

	if ((lstrcmpi(ms_NewVersion, ms_OurVersion) <= 0)
		// Если пользователь отказался от обновления в этом сеансе - не предлагать ту же версию при ежечасных проверках
		|| (!mb_ManualCallMode && (lstrcmp(ms_NewVersion, ms_SkipVersion) == 0)))
	{
		// Новых версий нет
		mb_NewVersionAvailable = false;

		if (mb_ManualCallMode)
		{
			// No newer %s version is available
			gpConEmu->CallMainThread(true, QueryConfirmationCallback, us_NotStarted);
		}

		goto wrap;
	}

	pszSource = ms_SourceFull.ms_Val;
	nSrcLen = wcstoul(pszSource, &pszEnd, 10);
	if (!nSrcLen || !pszEnd || *pszEnd != L',' || *(pszEnd+1) != L'x')
	{
		ReportError(L"Invalid format in version description (size)\n%s", (LPCWSTR)ms_SourceFull, 0);
		goto wrap;
	}
	mn_PackageSize = nSrcLen;
	pszSource = pszEnd+2;
	nSrcCRC = wcstoul(pszSource, &pszEnd, 16);
	if (!nSrcCRC || !pszEnd || *pszEnd != L',')
	{
		ReportError(L"Invalid format in version description (CRC32)\n%s", (LPCWSTR)ms_SourceFull, 0);
		goto wrap;
	}
	pszSource = pszEnd+1;
	lbSourceLocal = IsLocalFile(pszSource);

	if (mb_RequestTerminate)
		goto wrap;

	// It returns true, if updating with "exe" installer.
	if (!Check7zipInstalled())
		goto wrap; // Error already reported

	SafeFree(mpsz_ConfirmSource);
	mpsz_ConfirmSource = lstrdup(pszSource);

	if (gpConEmu && !gpConEmu->CallMainThread(true, QueryConfirmationCallback, us_ConfirmDownload))
	{
		// Если пользователь отказался от обновления в этом сеансе - не предлагать ту же версию при ежечасных проверках
		wcscpy_c(ms_SkipVersion, ms_NewVersion);
		goto wrap;
	}

	mn_InternetContentReady = 0;
	m_UpdateStep = us_Downloading;
	// May be null, if update package was dropped on ConEmu icon
	if (gpConEmu && ghWnd)
	{
		gpConEmu->UpdateProgress();
	}

	// Download the package
	while (!mb_RequestTerminate)
	{
		HANDLE hTarget = NULL;
		LARGE_INTEGER liSize = {};
		lbDownloadRc = FALSE;

		if (gpConEmu)
		{
			CEStr lsInfo(L"Update: Downloading package: ", pszSource);
			gpConEmu->LogString(lsInfo);
		}

		_ASSERTE(!ms_TemplFilename.IsEmpty());
		pszLocalPackage = CreateTempFile(mp_Set->szUpdateDownloadPath, ms_TemplFilename, hTarget);
		CloseHandle(hTarget);
		if (pszLocalPackage)
		{
			// liSize returns only .LowPart yet
			lbDownloadRc = DownloadFile(pszSource, pszLocalPackage, nLocalCRC, TRUE, &liSize);
			if (lbDownloadRc)
			{
				wchar_t szInfo[2048];
				if (liSize.HighPart || liSize.LowPart != nSrcLen)
				{
					_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"%s\nRequired size=%u, local size=%u", pszSource, nSrcLen, liSize.LowPart);
					ReportError(L"Downloaded file does not match\n%s\n%s", pszLocalPackage, szInfo, 0);
					lbDownloadRc = FALSE;
				}
				else if (nLocalCRC != nSrcCRC)
				{
					_wsprintf(szInfo, SKIPLEN(countof(szInfo)) L"Required CRC32=x%08X, local CRC32=x%08X", nSrcCRC, nLocalCRC);
					ReportError(L"Invalid local file\n%s\n%s", pszLocalPackage, szInfo, 0);
					lbDownloadRc = FALSE;
				}
			}
		}

		// Succeeded
		if (lbDownloadRc)
			break;

		// Now we show an error even in automatic mode because
		// the download was started after user confirmation
		LRESULT lRetryRc = (!gpConEmu || mb_RequestTerminate)
			? IDCANCEL
			: gpConEmu->CallMainThread(true/*bSync*/, CConEmuUpdate::QueryRetryVersionCheck, (LPARAM)szRetryPackageDownload);

		// Error must be already shown or logged, clear it
		if (ms_LastErrorInfo)
		{
			MSectionLockSimple CS;
			if (CS.Lock(mps_ErrorLock, 5000))
			{
				SafeFree(ms_LastErrorInfo);
			}
		}

		if (lRetryRc != IDRETRY)
		{
			goto wrap;
		}
	}

	Inet.Deinit(true);

	if (mb_RequestTerminate)
		goto wrap;

	if (gpConEmu)
		gpConEmu->LogString(L"Update: Creating update script");

	pszBatchFile = CreateBatchFile(pszLocalPackage);
	if (!pszBatchFile)
		goto wrap;

	iConfirmUpdate = gpConEmu->CallMainThread(true, QueryConfirmationCallback, us_ConfirmUpdate);
	if ((iConfirmUpdate <= 0) || (iConfirmUpdate == IDCANCEL))
	{
		// Если пользователь отказался от обновления в этом сеансе - не предлагать ту же версию при ежечасных проверках
		wcscpy_c(ms_SkipVersion, ms_NewVersion);
		goto wrap;
	}
	mpsz_PendingPackageFile = pszLocalPackage;
	pszLocalPackage = NULL;
	mpsz_PendingBatchFile = pszBatchFile;
	pszBatchFile = NULL;
	if (gpConEmu)
		gpConEmu->LogString(L"Update: Ready to update on exit");
	if (iConfirmUpdate == IDYES)
	{
		m_UpdateStep = us_ExitAndUpdate;
		if (gpConEmu) gpConEmu->CallMainThread(false, RequestExitUpdate, 0);
	}
	else
	{
		m_UpdateStep = us_PostponeUpdate;
		if (gpConEmu) gpConEmu->UpdateProgress();
	}
	lbExecuteRc = TRUE;

wrap:
	_ASSERTE(mpsz_DeletePackageFile==NULL);
	mpsz_DeletePackageFile = NULL;
	if (pszLocalPackage)
	{
		if (*pszLocalPackage && (!lbDownloadRc || (!lbExecuteRc && !mp_Set->isUpdateLeavePackages)))
			mpsz_DeletePackageFile = pszLocalPackage;
			//DeleteFile(pszLocalPackage);
		else
			SafeFree(pszLocalPackage);
	}

	_ASSERTE(mpsz_DeleteBatchFile==NULL);
	mpsz_DeleteBatchFile = NULL;
	if (pszBatchFile)
	{
		if (*pszBatchFile && !lbExecuteRc)
			mpsz_DeleteBatchFile = pszBatchFile;
			//DeleteFile(pszBatchFile);
		else
			SafeFree(pszBatchFile);
	}

	if (!lbExecuteRc)
		m_UpdateStep = us_NotStarted;

	Inet.Deinit(true);

	mb_InCheckProcedure = FALSE;
	return 0;
}

// Load information about current releases
bool CConEmuUpdate::LoadVersionInfoFromServer()
{
	bool bOk = false;

	// E.g. https://conemu.github.io/version.ini
	LPCWSTR pszUpdateVerLocationSet = mp_Set->UpdateVerLocation();
	LPCWSTR pszUpdateVerLocation = NULL;
	wchar_t szSection[64], szItem[64];

	// Logging
	if (gpConEmu)
	{
		CEStr lsInfo(L"Update: Loading versions from server: ", pszUpdateVerLocationSet);
		gpConEmu->LogString(lsInfo);
	}

	ms_VersionUrlCheck.Set(pszUpdateVerLocationSet);

	// Erase previous temp file if any
	if (!ms_TempUpdateVerLocation.IsEmpty())
	{
		DeleteFile(ms_TempUpdateVerLocation);
		ms_TempUpdateVerLocation.Empty();
	}

	// We need to download information? Or it it is located in the intranet?
	if (IsLocalFile(pszUpdateVerLocationSet))
	{
		pszUpdateVerLocation = pszUpdateVerLocationSet;
	}
	else
	{
		HANDLE hInfo = NULL;
		BOOL bInfoRc;
		DWORD crc;

		// Create temporary file
		ms_TempUpdateVerLocation = CreateTempFile(mp_Set->szUpdateDownloadPath/*L"%TEMP%"*/, L"ConEmuVersion.ini", hInfo);
		CloseHandle(hInfo);
		if (ms_TempUpdateVerLocation.IsEmpty())
			goto wrap;
		pszUpdateVerLocation = (LPCWSTR)ms_TempUpdateVerLocation;

		// And download file from server
		bInfoRc = DownloadFile(pszUpdateVerLocationSet, pszUpdateVerLocation, crc);
		if (!bInfoRc)
		{
			goto wrap;
		}
	}

	// Logging
	if (gpConEmu)
	{
		CEStr lsInfo(L"Update: Checking version information: ", pszUpdateVerLocation);
		gpConEmu->LogString(lsInfo);
	}

	// What version user prefers?
	_wcscpy_c(szSection, countof(szSection),
		(mp_Set->isUpdateUseBuilds==1) ? sectionConEmuStable :
		(mp_Set->isUpdateUseBuilds==3) ? sectionConEmuPreview
		: sectionConEmuDevel);
	_wcscpy_c(szItem, countof(szItem), (mp_Set->UpdateDownloadSetup()==1) ? L"location_exe" : L"location_arc");

	// Check the ini-file
	if (!GetPrivateProfileString(szSection, L"version", L"", ms_NewVersion, countof(ms_NewVersion), pszUpdateVerLocation) || !*ms_NewVersion)
	{
		ReportBrokenIni(szSection, L"version", pszUpdateVerLocationSet, pszUpdateVerLocation);
		goto wrap;
	}

	// URL may not contain file name at all, compile it (predefined)
	if (mp_Set->UpdateDownloadSetup()==1)
		ms_TemplFilename = lstrmerge(L"ConEmuSetup.", ms_NewVersion, L".exe");
	else
		ms_TemplFilename = lstrmerge(L"ConEmuPack.", ms_NewVersion, L".7z");

	ms_SourceFull.Empty();

	if (!GetPrivateProfileString(szSection, szItem, L"", ms_SourceFull.GetBuffer(1024), 1024, pszUpdateVerLocation)
		|| ms_SourceFull.IsEmpty())
	{
		ReportBrokenIni(szSection, szItem, pszUpdateVerLocationSet, pszUpdateVerLocation);
		goto wrap;
	}

	GetVersionsFromIni(pszUpdateVerLocation, ms_VerOnServer, ms_VerOnServerRA, ms_CurVerInfo);

	bOk = true;
wrap:
	return bOk;
}

wchar_t* CConEmuUpdate::CreateVersionOnServerInfo(bool abRightAligned, LPCWSTR asSuffix /*= NULL*/)
{
	wchar_t* pszMsg = lstrmerge(
		L"Versions on server\n",
		(LPCWSTR)ms_VersionUrlCheck,
		L"\n",
		abRightAligned ? ms_VerOnServerRA : ms_VerOnServer,
		asSuffix);
	return pszMsg;
}

LRESULT CConEmuUpdate::QueryRetryVersionCheck(LPARAM lParam)
{
	LRESULT lRc = IDCANCEL;
	CEStr szError;
	LPCWSTR pszMessage = (LPCWSTR)lParam;

	// if the termination was requested
	if (!gpUpd || !gpConEmu || gpUpd->mb_RequestTerminate)
	{
		goto wrap;
	}

	// to avoid infinite recursion
	if (gpUpd->mn_InShowMsgBox > 0)
	{
		InterlockedIncrement(&gpUpd->mn_ErrorInfoSkipCount);
		goto wrap;
	}

	InterlockedIncrement(&gpUpd->mn_ErrorInfoCount);

	_ASSERTE(isMainThread());

	// Concatenate error details if any
	if (gpUpd->ms_LastErrorInfo)
	{
		MSectionLockSimple CS;
		if (CS.Lock(gpUpd->mps_ErrorLock, 5000))
		{
			szError = lstrmerge(pszMessage, L"\n\n", gpUpd->ms_LastErrorInfo);
			SafeFree(gpUpd->ms_LastErrorInfo);
		}
		else
		{
			szError = lstrmerge(pszMessage, L"\n\n", L"Updater error was occurred but we failed to lock it");
		}
	}
	else
	{
		szError = pszMessage;
	}

	// Now show the error
	if (!szError.IsEmpty())
	{
		InterlockedIncrement(&gpUpd->mn_InShowMsgBox);

		lRc = MsgBox(szError, MB_ICONEXCLAMATION|MB_RETRYCANCEL, gpConEmu?gpConEmu->GetDefaultTitle():L"ConEmu Update");

		InterlockedDecrement(&gpUpd->mn_InShowMsgBox);
	}

wrap:
	return lRc;
}

wchar_t* CConEmuUpdate::CreateBatchFile(LPCWSTR asPackage)
{
	BOOL lbRc = FALSE;
	HANDLE hBatch = NULL;
	wchar_t* pszBatch = NULL;
	wchar_t* pszCommand = NULL;
	BOOL lbWrite;
	DWORD nLen, nWritten;
	char szOem[4096];
	LPCWSTR pszFormat = NULL;
	size_t cchCmdMax = 0;

	wchar_t szPID[16]; _wsprintf(szPID, SKIPLEN(countof(szPID)) L"%u", GetCurrentProcessId());
	// Setupper bitness will be corrected in mp_Set->UpdateExeCmdLine
	wchar_t szCPU[4]; wcscpy_c(szCPU, WIN3264TEST(L"x86",L"x64"));

	if (!gpConEmu)
	{
		ReportError(L"CreateBatchFile failed, gpConEmu==NULL", 0); goto wrap;
	}

	pszBatch = CreateTempFile(mp_Set->szUpdateDownloadPath, L"ConEmuUpdate.cmd", hBatch);
	if (!pszBatch)
		goto wrap;

	#define WRITE_BATCH_A(s) \
		nLen = lstrlenA(s); \
		lbWrite = WriteFile(hBatch, s, nLen, &nWritten, NULL); \
		if (!lbWrite || (nLen != nWritten)) { ReportError(L"WriteBatch failed, code=%u", GetLastError()); goto wrap; }

	#define WRITE_BATCH_W(s) \
		nLen = WideCharToMultiByte(CP_OEMCP, 0, s, -1, szOem, countof(szOem), NULL, NULL); \
		if (!nLen) { ReportError(L"WideCharToMultiByte failed, len=%i", lstrlen(s)); goto wrap; } \
		WRITE_BATCH_A(szOem);

	WRITE_BATCH_A("@echo off\r\n");

	// "set ConEmuInUpdate=YES"
	WRITE_BATCH_W(L"\r\nset " ENV_CONEMU_INUPDATE_W L"=" ENV_CONEMU_INUPDATE_YES L"\r\n");

	WRITE_BATCH_A("cd /d \"");
	WRITE_BATCH_W(gpConEmu->ms_ConEmuExeDir);
	WRITE_BATCH_A("\\\"\r\necho Current folder\r\ncd\r\necho .\r\n\r\necho Starting update...\r\n");

	// Формат.
	pszFormat = (mp_Set->UpdateDownloadSetup()==1) ? mp_Set->UpdateExeCmdLine(szCPU) : mp_Set->UpdateArcCmdLine();

	// Замена %1 и т.п.
	for (int s = 0; s < 2; s++)
	{
		// На первом шаге - считаем требуемый размер под pszCommand, на втором - формируем команду
		if (s)
		{
			if (!cchCmdMax)
			{
				ReportError(L"Invalid %s update command (%s)", (mp_Set->UpdateDownloadSetup()==1) ? L"exe" : L"arc", pszFormat, 0);
				goto wrap;
			}
			pszCommand = (wchar_t*)malloc((cchCmdMax+1)*sizeof(wchar_t));
		}
		wchar_t* pDst = pszCommand;
		LPCWSTR pszMacro;
		for (LPCWSTR pSrc = pszFormat; *pSrc; pSrc++)
		{
			switch (*pSrc)
			{
			case L'%':
				pSrc++;
				switch (*pSrc)
				{
				case L'%':
					pszMacro = L"%";
					break;
				// "%1"-archive or setup file, "%2"-ConEmu.exe folder, "%3"-x86/x64, "%4"-ConEmu PID
				case L'1':
					pszMacro = asPackage;
					break;
				case L'2':
					pszMacro = gpConEmu->ms_ConEmuExeDir;
					break;
				case L'3':
					pszMacro = szCPU;
					break;
				case L'4':
					pszMacro = szPID;
					break;
				default:
					// Недопустимый управляющий символ, это может быть переменная окружения
					pszMacro = NULL;
					pSrc--;
					if (s)
						*(pDst++) = L'%';
					else
						cchCmdMax++;
				}

				if (pszMacro)
				{
					size_t cchLen = _tcslen(pszMacro);
					if (s)
					{
						_wcscpy_c(pDst, cchLen+1, pszMacro);
						pDst += cchLen;
					}
					else
					{
						cchCmdMax += cchLen;
					}
				}
				break;
			default:
				if (s)
					*(pDst++) = *pSrc;
				else
					cchCmdMax++;
			}
		}
		if (s)
			*pDst = 0;
	}

	// Выполнить команду обновления
	WRITE_BATCH_A("echo ");
	WRITE_BATCH_W(pszCommand);
	WRITE_BATCH_A("\r\ncall ");
	WRITE_BATCH_W(pszCommand);
	WRITE_BATCH_A("\r\nif errorlevel 1 goto err\r\n");

	// Если юзер просил что-то выполнить после распаковки установки
	if (mp_Set->szUpdatePostUpdateCmd && *mp_Set->szUpdatePostUpdateCmd)
	{
		WRITE_BATCH_A("\r\n");
		WRITE_BATCH_W(mp_Set->szUpdatePostUpdateCmd);
		WRITE_BATCH_A("\r\n");
	}

	// Сброс переменной окружения: "set ConEmuInUpdate="
	WRITE_BATCH_W(L"\r\nset " ENV_CONEMU_INUPDATE_W L"=\r\n");

	// Перезапуск ConEmu
	WRITE_BATCH_A("\r\necho Starting ConEmu...\r\nstart \"ConEmu\" \"");
	WRITE_BATCH_W(gpConEmu->ms_ConEmuExe);
	WRITE_BATCH_A("\" ");
	if (bNeedRunElevation)
	{
		WRITE_BATCH_A("/demote /cmd \"");
		WRITE_BATCH_W(gpConEmu->ms_ConEmuExe);
		WRITE_BATCH_A("\" ");
	}
	if (gpConEmu->mpsz_ConEmuArgs)
	{
		WRITE_BATCH_W(gpConEmu->mpsz_ConEmuArgs);
	}
	// Fin
	WRITE_BATCH_A("\r\ngoto fin\r\n");

	// Сообщение об ошибке?
	WRITE_BATCH_A("\r\n:err\r\n");
	WRITE_BATCH_A((mp_Set->UpdateDownloadSetup()==1) ? "echo \7Installation failed\7" : "echo \7Extraction failed\7\r\n");
	WRITE_BATCH_A("\r\npause\r\n:fin\r\n");

	// Грохнуть пакет обновления
	if (!mp_Set->isUpdateLeavePackages)
	{
		WRITE_BATCH_A("del \"");
		WRITE_BATCH_W(asPackage);
		WRITE_BATCH_A("\"\r\n");
	}

	// Грохнуть сам батч и позвать "exit" чтобы в консоли
	// не появлялось "Batch not found" при попытке выполнить следующую строку файла
	WRITE_BATCH_A("del \"%~0\" & exit\r\n");

	//// Для отладки
	//WRITE_BATCH_A("\r\npause\r\n");

	// Succeeded
	lbRc = TRUE;
wrap:
	SafeFree(pszCommand);

	if (!lbRc)
	{
		SafeFree(pszBatch);
	}

	if (hBatch && hBatch != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hBatch);
	}
	return pszBatch;
}

CConEmuUpdate::UpdateStep CConEmuUpdate::InUpdate()
{
	if (!this)
	{
		_ASSERTE(this);
		return us_NotStarted;
	}

	DWORD nWait = WAIT_OBJECT_0;

	if (mh_CheckThread)
		nWait = WaitForSingleObject(mh_CheckThread, 0);

	switch (m_UpdateStep)
	{
	case us_Check:
	case us_ConfirmDownload:
	case us_ConfirmUpdate:
		if (nWait == WAIT_OBJECT_0)
			m_UpdateStep = us_NotStarted;
		break;
	case us_PostponeUpdate:
	case us_ExitAndUpdate:
		// Тут у нас нить уже имеет право завершиться
		break;
	default:
		;
	}

	return m_UpdateStep;
}

// asDir is allowed to have environment variables, e.g. "%TEMP%\\ConEmu"
wchar_t* CConEmuUpdate::CreateTempFile(LPCWSTR asDir, LPCWSTR asFileNameTempl, HANDLE& hFile)
{
	wchar_t szFile[MAX_PATH*2+2];
	wchar_t szName[128];

	if (!asDir || !*asDir)
		asDir = L"%TEMP%\\ConEmu";
	if (!asFileNameTempl || !*asFileNameTempl)
		asFileNameTempl = L"ConEmu.tmp";

	if (wcschr(asDir, L'%'))
	{
		DWORD nExp = ExpandEnvironmentStrings(asDir, szFile, MAX_PATH);
		if (!nExp || (nExp >= MAX_PATH))
		{
			ReportError(L"CreateTempFile.ExpandEnvironmentStrings(%s) failed, code=%u", asDir, GetLastError());
			return NULL;
		}
	}
	else
	{
		lstrcpyn(szFile, asDir, MAX_PATH);
	}

	// Checking %TEMP% for valid path
	LPCWSTR pszColon1, pszColon2;
	if ((pszColon1 = wcschr(szFile, L':')) != NULL)
	{
		if ((pszColon2 = wcschr(pszColon1+1, L':')) != NULL)
		{
			ReportError(L"Invalid download path (%%TEMP%% variable?)\n%s", szFile, 0);
			return NULL;
		}
	}

	int nLen = lstrlen(szFile);
	if (nLen <= 0)
	{
		ReportError(L"CreateTempFile.asDir(%s) failed, path is null", asDir, 0);
		return NULL;
	}
	if (szFile[nLen-1] != L'\\')
	{
		szFile[nLen++] = L'\\'; szFile[nLen] = 0;
	}
	wchar_t* pszFilePart = szFile + nLen;

	wchar_t* pszDirectory = lstrdup(szFile);


	LPCWSTR pszName = PointToName(asFileNameTempl);
	_ASSERTE(pszName == asFileNameTempl);
	if (!pszName || !*pszName || (*pszName == L'.'))
	{
		_ASSERTE(pszName && *pszName && (*pszName != L'.'));
		pszName = L"ConEmu";
	}

	LPCWSTR pszExt = PointToExt(pszName);
	if (pszExt == NULL)
	{
		_ASSERTE(pszExt != NULL);
		pszExt = L".tmp";
	}

	lstrcpyn(szName, pszName, countof(szName)-16);
	wchar_t* psz = wcsrchr(szName, L'.');
	if (psz)
		*psz = 0;

	wchar_t* pszResult = NULL;
	DWORD dwErr = 0;

	for (UINT i = 0; i <= 9999; i++)
	{
		_wcscpy_c(pszFilePart, MAX_PATH, szName);
		if (i)
			_wsprintf(pszFilePart+_tcslen(pszFilePart), SKIPLEN(16) L"(%u)", i);
		_wcscat_c(pszFilePart, MAX_PATH, pszExt);

		SECURITY_ATTRIBUTES sec = {sizeof(sec), NULL, TRUE};
		hFile = CreateFile(szFile, GENERIC_WRITE, FILE_SHARE_READ, &sec, CREATE_NEW, FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_TEMPORARY, NULL);
		//ERROR_PATH_NOT_FOUND?
		if (!hFile || (hFile == INVALID_HANDLE_VALUE))
		{
			dwErr = GetLastError();
			// на первом обломе - попытаться создать директорию, может ее просто нет?
			if ((dwErr == ERROR_PATH_NOT_FOUND) && (i == 0))
			{
				if (!MyCreateDirectory(pszDirectory))
				{
					ReportError(L"CreateTempFile.asDir(%s) failed", asDir, 0);
					goto wrap;
				}
			}
		}

		if (hFile && hFile != INVALID_HANDLE_VALUE)
		{
			psz = lstrdup(szFile);
			if (!psz)
			{
				CloseHandle(hFile); hFile = NULL;
				ReportError(L"Can't allocate memory (%i bytes)", lstrlen(szFile));
			}
			pszResult = psz;
			goto wrap;
		}
	}

	ReportError(L"Can't create temp file(%s), code=%u", szFile, dwErr);
	hFile = NULL;
wrap:
	SafeFree(pszDirectory);
	return pszResult;
}

// This checks if file is located on local drive
// (has "file://" prefix, or "\\server\share\..." or "X:\path\...")
// and set asPathOrUrl back to local path (if prefix was specified)
bool CConEmuUpdate::IsLocalFile(LPCWSTR& asPathOrUrl)
{
	LPWSTR psz = (LPWSTR)asPathOrUrl;
	bool lbLocal = IsLocalFile(psz);
	asPathOrUrl = psz;
	return lbLocal;
}

// This checks if file is located on local drive
// (has "file://" prefix, or "\\server\share\..." or "X:\path\...")
// and set asPathOrUrl back to local path (if prefix was specified)
// Function DOES NOT modify the contents of buffer pointed by asPathOrUrl!
bool CConEmuUpdate::IsLocalFile(LPWSTR& asPathOrUrl)
{
	if (!asPathOrUrl || !*asPathOrUrl)
	{
		_ASSERTE(asPathOrUrl && *asPathOrUrl);
		return true;
	}

	if (asPathOrUrl[0] == L'\\' && asPathOrUrl[1] == L'\\')
		return true; // network or UNC
	if (asPathOrUrl[1] == L':')
		return true; // Local drive

	wchar_t szPrefix[8]; // "file:"
	lstrcpyn(szPrefix, asPathOrUrl, countof(szPrefix));
	if (lstrcmpi(szPrefix, L"file://") == 0)
	{
		asPathOrUrl += 7;
		if (asPathOrUrl[0] == L'/' && isDriveLetter(asPathOrUrl[1]) && asPathOrUrl[2] == L':')
			asPathOrUrl++;
		return true; // "file:" protocol
	}

	return false;
}

BOOL CConEmuUpdate::DownloadFile(LPCWSTR asSource, LPCWSTR asTarget, DWORD& crc, BOOL abPackage /*= FALSE*/, LARGE_INTEGER* rpSize /*= NULL*/)
{
	BOOL lbRc = FALSE, lbRead = FALSE, lbWrite = FALSE;
	CEDownloadErrorArg args[3] = {};

	MCHKHEAP;

	mn_InternetContentReady = 0;

	if (gpConEmu)
	{
		CEStr lsMsg(L"Update: Downloading ", asSource, L" to ", asTarget);
		gpConEmu->LogString(lsMsg);
	}

	// This implies Inet.Deinit(false) too
	if (!Inet.Init(this))
	{
		// Ошибка уже в стеке
		goto wrap;
	}

	mb_InetMode = !IsLocalFile(asSource);
	if (mb_InetMode)
	{
		mb_DroppedMode = false;

		if (mp_Set->isUpdateInetTool && mp_Set->szUpdateInetTool && *mp_Set->szUpdateInetTool)
		{
			args[0].argType = at_Str;args[0].strArg = mp_Set->szUpdateInetTool;
			Inet.DownloadCommand(dc_SetCmdString, 1, args);
		}
		else if (mp_Set->isUpdateUseProxy)
		{
			args[0].argType = at_Str;args[0].strArg = mp_Set->szUpdateProxy;
			args[1].argType = at_Str;args[1].strArg = mp_Set->szUpdateProxyUser;
			args[2].argType = at_Str;args[2].strArg = mp_Set->szUpdateProxyPassword;
			Inet.DownloadCommand(dc_SetProxy, 3, args);
		}
	}

	Inet.SetCallback(dc_ProgressCallback, ProgressCallback, (LPARAM)this);

	args[0].argType = at_Str;  args[0].strArg = asSource;
	args[1].argType = at_Str;  args[1].strArg = asTarget;
	args[2].argType = at_Uint; args[2].uintArg = (mb_ManualCallMode || abPackage);

	MCHKHEAP;

	lbRc = Inet.DownloadCommand(dc_DownloadFile, 3, args);
	if (lbRc)
	{
		if (rpSize)
		{
			rpSize->LowPart = args[0].uintArg;
			rpSize->HighPart = 0; // Files sizes larger than 4GB are not supported (doesn't matter for our update package)
		}
		crc = args[1].uintArg;
	}
	else
	{
		ReportError(L"File download was failed\n%s\nErrCode=0x%08X", asSource, args[0].uintArg);
	}
wrap:
	Inet.SetCallback(dc_ProgressCallback, NULL, 0);
	MCHKHEAP;
	return lbRc;
}

void CConEmuUpdate::ReportErrorInt(wchar_t* asErrorInfo)
{
	MCHKHEAP;

	if (gpConEmu)
	{
		CEStr lsLog(L"Update: ", asErrorInfo);
		wchar_t* ptr = lsLog.ms_Val;
		while ((ptr = wcspbrk(ptr, L"\r\n")) != NULL) *ptr = L' ';
		gpConEmu->LogString(lsLog);
	}

	// Append error
	{
		wchar_t szTime[40] = L""; SYSTEMTIME st = {}; GetLocalTime(&st);
		_wsprintf(szTime, SKIPLEN(countof(szTime)) L"\r\n%u-%02u-%02u %u:%02u:%02u.%03u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

		MSectionLockSimple CS;
		if (CS.Lock(mps_ErrorLock, 5000))
		{
			wchar_t* pszNew = lstrmerge(asErrorInfo, szTime, ms_LastErrorInfo ? L"\r\n\r\n" : NULL, ms_LastErrorInfo);
			if (pszNew)
			{
				SafeFree(asErrorInfo);
				SafeFree(ms_LastErrorInfo);
				ms_LastErrorInfo = pszNew;
			}
			else
			{
				SafeFree(ms_LastErrorInfo);
				ms_LastErrorInfo = asErrorInfo;
			}
		}
		else
		{
			_ASSERTE(FALSE && "Can't lock ms_LastErrorInfo");
			ms_LastErrorInfo = asErrorInfo;
		}
	}

	// Show error message boxes in appropriate places only
	return;
#if 0
	MCHKHEAP;

	if (mn_InShowMsgBox > 0)
	{
		InterlockedIncrement(&mn_ErrorInfoSkipCount);
		return; // to avoid infinite recursion
	}

	InterlockedIncrement(&mn_ErrorInfoCount);

	// This will call back ShowLastError
	if (gpConEmu)
		gpConEmu->CallMainThread(false, ShowLastError, (LPARAM)this);
	else
		SafeFree(asErrorInfo);

	MCHKHEAP;
#endif
}

void CConEmuUpdate::ReportError(LPCWSTR asFormat, DWORD nErrCode)
{
	if (!asFormat)
		return;

	size_t cchMax = _tcslen(asFormat) + 32;
	wchar_t* pszErrInfo = (wchar_t*)malloc(cchMax*sizeof(wchar_t));
	if (pszErrInfo)
	{
		_wsprintf(pszErrInfo, SKIPLEN(cchMax) asFormat, nErrCode);
		ReportErrorInt(pszErrInfo);
	}
}

void CConEmuUpdate::ReportError(LPCWSTR asFormat, LPCWSTR asArg, DWORD nErrCode)
{
	if (!asFormat || !asArg)
		return;

	size_t cchMax = _tcslen(asFormat) + _tcslen(asArg) + 32;
	wchar_t* pszErrInfo = (wchar_t*)malloc(cchMax*sizeof(wchar_t));
	if (pszErrInfo)
	{
		_wsprintf(pszErrInfo, SKIPLEN(cchMax) asFormat, asArg, nErrCode);
		ReportErrorInt(pszErrInfo);
	}
}

void CConEmuUpdate::ReportError(LPCWSTR asFormat, LPCWSTR asArg1, LPCWSTR asArg2, DWORD nErrCode)
{
	if (!asFormat || !asArg1 || !asArg2)
		return;

	size_t cchMax = _tcslen(asFormat) + _tcslen(asArg1) + _tcslen(asArg2) + 32;
	wchar_t* pszErrInfo = (wchar_t*)malloc(cchMax*sizeof(wchar_t));
	if (pszErrInfo)
	{
		_wsprintf(pszErrInfo, SKIPLEN(cchMax) asFormat, asArg1, asArg2, nErrCode);
		ReportErrorInt(pszErrInfo);
	}
}

LRESULT CConEmuUpdate::RequestExitUpdate(LPARAM)
{
	if (!gpUpd)
		return 0;

	CConEmuUpdate::UpdateStep step = gpUpd->InUpdate();

	// Awaiting for exit?
	if ((step != us_ExitAndUpdate) && (step != us_PostponeUpdate))
	{
		_ASSERTE(step == us_ExitAndUpdate);
	}
	// May be null, if update package was dropped on ConEmu icon
	else if (ghWnd)
	{
		gpConEmu->UpdateProgress();
		gpConEmu->PostScClose();
	}

	return 0;
}

void CConEmuUpdate::ReportBrokenIni(LPCWSTR asSection, LPCWSTR asName, LPCWSTR asIniUrl, LPCWSTR asIniLocal)
{
	DWORD nErr = GetLastError();

	CEStr lsInfo(
		L"[", asSection, L"] \"", asName, L"\""
		);

	CEStr lsIni(
		L"URL: ", asIniUrl, L"\n"
		L"File: ", asIniLocal
		);

	ReportError(
		L"Version update information is broken (not found)\n"
		L"%s\n\n"
		L"%s\n\n"
		L"Error code=%u",
		(LPCWSTR)lsInfo, (LPCWSTR)lsIni, nErr);
}

// If we are installed in "C:\Program Files\..." or any other restricted location.
bool CConEmuUpdate::NeedRunElevation()
{
	if (!gpConEmu)
		return false;

	//TODO: В каких случаях нужен "runas"
	//TODO: Vista+: (если сейчас НЕ "Admin") && (установка в %ProgramFiles%)
	//TODO: WinXP-: (установка в %ProgramFiles%) && (нет доступа в %ProgramFiles%)

	DWORD dwErr = 0;
	wchar_t szTestFile[MAX_PATH+20];
	wcscpy_c(szTestFile, gpConEmu->ms_ConEmuExeDir);
	wcscat_c(szTestFile, L"\\");

	if (gOSVer.dwMajorVersion >= 6)
	{
		if (IsUserAdmin())
			return false; // Уже под админом (Vista+)
		// куда мы установлены? Если НЕ в %ProgramFiles%, то для распаковки совсем не нужно Elevation требовать
		int nFolderIdl[] = {
			CSIDL_PROGRAM_FILES,
			CSIDL_PROGRAM_FILES_COMMON,
			#ifdef _WIN64
			CSIDL_PROGRAM_FILESX86,
			CSIDL_PROGRAM_FILES_COMMONX86,
			#endif
		};
		size_t nLen;
		wchar_t szSystem[MAX_PATH+2];
		for (size_t i = 0; i < countof(nFolderIdl); i++)
		{
			for (size_t j = 0; j <= 1; j++)
			{
				if ((S_OK == SHGetFolderPath(NULL, nFolderIdl[i], NULL, j ? SHGFP_TYPE_DEFAULT : SHGFP_TYPE_CURRENT, szSystem))
					&& (nLen = _tcslen(szSystem)))
				{
					if (szSystem[nLen-1] != L'\\')
					{
						szSystem[nLen++] = L'\\'; szSystem[nLen] = 0;
					}

					if (lstrcmpni(szTestFile, szSystem, nLen) == 0)
					{
						return true; // installed in the ProgramFiles
					}
				}
			}
		}
		// Issue 651: Проверим возможность создания/изменения файлов в любом случае
		//// Скорее всего не надо
		//return false;
	}

	// XP и ниже
	wcscat_c(szTestFile, L"ConEmuUpdate.flag");
	HANDLE hFile = CreateFile(szTestFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		dwErr = GetLastError();
		return true;
	}
	CloseHandle(hFile);
	DeleteFile(szTestFile);

	// RunAs не нужен
	return false;
}

void CConEmuUpdate::DeleteBadTempFiles()
{
	if (!ms_TempUpdateVerLocation.IsEmpty())
	{
		DeleteFile(ms_TempUpdateVerLocation);
		ms_TempUpdateVerLocation.Empty();
	}
	if (mpsz_DeletePackageFile)
	{
		DeleteFile(mpsz_DeletePackageFile);
		SafeFree(mpsz_DeletePackageFile);
	}
	if (mpsz_DeleteBatchFile)
	{
		DeleteFile(mpsz_DeleteBatchFile);
		SafeFree(mpsz_DeleteBatchFile);
	}
}

bool CConEmuUpdate::Check7zipInstalled()
{
	if (mp_Set->UpdateDownloadSetup() == 1)
		return true; // Инсталлер, архиватор не требуется!

	LPCWSTR pszCmd = mp_Set->UpdateArcCmdLine();
	CEStr sz7zip; sz7zip.GetBuffer(MAX_PATH);
	if (NextArg(&pszCmd, sz7zip) != 0)
	{
		ReportError(L"Invalid update command\nGoto 'Update' page and check 7-zip command", 0);
		return false;
	}

	if (FileExistsSearch((LPCWSTR)sz7zip, sz7zip, false))
		return true;

	WARNING("TODO: Suggest to download 7zip");

	ReportError(L"7zip or WinRar not found! Not installed?\n%s\nGoto 'Update' page and check 7-zip command", sz7zip, 0);
	return false;
}

int CConEmuUpdate::QueryConfirmation(CConEmuUpdate::UpdateStep step)
{
	if (mb_RequestTerminate)
	{
		return 0;
	}

	int lRc = 0;
	wchar_t* pszMsg = NULL;
	size_t cchMax;

	_ASSERTE(isMainThread());

	switch (step)
	{
	case us_ConfirmDownload:
		{
			mb_NewVersionAvailable = true;
			m_UpdateStep = step;

			if (mb_ManualCallMode == 2)
			{
				lRc = TRUE;
			}
			else if (mp_Set->isUpdateConfirmDownload || mb_ManualCallMode)
			{
				lRc = QueryConfirmationDownload();
			}
			else
			{
				cchMax = 200;
				pszMsg = (wchar_t*)malloc(cchMax*sizeof(*pszMsg));

				_wsprintf(pszMsg, SKIPLEN(cchMax) L"New %s version available: %s\nClick here to download",
					(mp_Set->isUpdateUseBuilds==1) ? CV_STABLE : (mp_Set->isUpdateUseBuilds==3) ? CV_PREVIEW : CV_DEVEL,
					ms_NewVersion);
				Icon.ShowTrayIcon(pszMsg, tsa_Source_Updater);

				lRc = 0;
			}
		}
		break;

	case us_ConfirmUpdate:
		m_UpdateStep = step;
		lRc = QueryConfirmationUpdate();
		break;

	default:
		lRc = false;
		_ASSERTE(mb_NewVersionAvailable == false);
		if (step > us_Check)
		{
			_ASSERTE(step<=us_Check);
			break;
		}

		QueryConfirmationNoNewVer();
	}

	SafeFree(pszMsg);

	return lRc;
}

int CConEmuUpdate::QueryConfirmationDownload()
{
	if ((mn_InShowMsgBox > 0) || !gpConEmu)
		return 0; // Если отображается ошибка - не звать

	DontEnable de;
	InterlockedIncrement(&mn_InShowMsgBox);

	int iBtn = -1;
	HRESULT hr;
	wchar_t szNewVer[120];
	wchar_t szWWW[MAX_PATH];
	TASKDIALOGCONFIG tsk = {sizeof(tsk)};
	TASKDIALOG_BUTTON btns[3] = {};
	LPCWSTR pszServerPath = L"";
	size_t cchMax;
	wchar_t* pszMsg = NULL;
	wchar_t* pszDup = NULL;
	wchar_t* pszFile = NULL;
	wchar_t* pszBtn1 = NULL;
	wchar_t* pszBtn2 = NULL;


	if (mpsz_ConfirmSource)
	{
		pszDup = lstrdup(mpsz_ConfirmSource);
		if (pszDup)
		{
			pszFile = wcsrchr(pszDup, L'/');
			if (!pszFile) pszFile = wcsrchr(pszDup, L'\\'); // Local file?
		}
		if (pszFile)
		{
			pszFile[1] = 0;
			pszFile = (mpsz_ConfirmSource + (pszFile - pszDup + 1));
			pszServerPath = pszDup;
		}
		else
		{
			pszServerPath = mpsz_ConfirmSource;
		}
	}

	if (gOSVer.dwMajorVersion < 6)
		goto MsgOnly;

	tsk.hInstance = g_hInstance;
	if (hClassIcon)
		tsk.hMainIcon = hClassIcon;
	else
		tsk.pszMainIcon = MAKEINTRESOURCE(IDI_ICON1);

	_wsprintf(szNewVer, SKIPLEN(countof(szNewVer)) L"New %s version available: %s",
		(mp_Set->isUpdateUseBuilds==1) ? CV_STABLE : (mp_Set->isUpdateUseBuilds==3) ? CV_PREVIEW : CV_DEVEL, ms_NewVersion);
	tsk.pszMainInstruction = szNewVer;

	_wsprintf(szWWW, SKIPLEN(countof(szWWW)) L"<a href=\"%s\">%s</a>", gsWhatsNew, szWhatsNewLabel);
	tsk.pszFooter = szWWW;

	pszMsg = CreateVersionOnServerInfo(true);
	tsk.pszContent = pszMsg;

	pszBtn1 = lstrmerge(L"Download\n", pszServerPath, pszFile ? L"\n" : NULL, pszFile);
	btns[0].nButtonID  = IDYES;    btns[0].pszButtonText = pszBtn1;
	pszBtn2 = lstrmerge(L"Visit download page\n", gsDownlPage);
	btns[1].nButtonID  = IDNO;     btns[1].pszButtonText = pszBtn2;
	btns[2].nButtonID  = IDCANCEL; btns[2].pszButtonText = L"Cancel\nSkip this version";

	tsk.dwFlags        = (hClassIcon?TDF_USE_HICON_MAIN:0)|TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_ENABLE_HYPERLINKS;
	tsk.pszWindowTitle = ms_DefaultTitle;
	tsk.pButtons       = btns;
	tsk.cButtons       = countof(btns);

	tsk.hwndParent     = ghWnd;

	hr = TaskDialog(&tsk, &iBtn, NULL, NULL);
	if (hr == S_OK)
	{
		if (iBtn == IDNO)
			ConEmuAbout::OnInfo_DownloadPage();
		goto wrap;
	}

MsgOnly:

	SafeFree(pszMsg);
	cchMax = 300 + (mpsz_ConfirmSource ? _tcslen(mpsz_ConfirmSource) : 0);
	pszMsg = (wchar_t*)malloc(cchMax*sizeof(*pszMsg));

	_wsprintf(pszMsg, SKIPLEN(cchMax) L"New %s version available: %s\n\nVersions on server\n%s\n\n%s\n%s\n\nDownload?",
		(mp_Set->isUpdateUseBuilds==1) ? CV_STABLE : (mp_Set->isUpdateUseBuilds==3) ? CV_PREVIEW : CV_DEVEL,
		ms_NewVersion,
		ms_VerOnServer,
		pszServerPath,
		pszFile ? pszFile : L"");


	iBtn = MsgBox(pszMsg, MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONQUESTION|MB_YESNO, ms_DefaultTitle, NULL, false);

wrap:
	InterlockedDecrement(&mn_InShowMsgBox);
	SafeFree(pszMsg);
	SafeFree(pszDup);
	SafeFree(pszBtn1);
	SafeFree(pszBtn2);
	return (iBtn == IDYES);
}

int CConEmuUpdate::QueryConfirmationUpdate()
{
	if ((mn_InShowMsgBox > 0) || !gpConEmu)
		return 0; // Если отображается ошибка - не звать

	DontEnable de;
	InterlockedIncrement(&mn_InShowMsgBox);

	int iBtn = -1;
	HRESULT hr;
	wchar_t szWWW[MAX_PATH];
	TASKDIALOGCONFIG tsk = {sizeof(tsk)};
	TASKDIALOG_BUTTON btns[3] = {};
	wchar_t szMsg[200];


	if (gOSVer.dwMajorVersion < 6)
		goto MsgOnly;

	tsk.hInstance = g_hInstance;
	if (hClassIcon)
		tsk.hMainIcon = hClassIcon;
	else
		tsk.pszMainIcon = MAKEINTRESOURCE(IDI_ICON1);

	tsk.pszMainInstruction = L"Close ConEmu window and update?";

	_wsprintf(szWWW, SKIPLEN(countof(szWWW)) L"<a href=\"%s\">%s</a>", gsWhatsNew, szWhatsNewLabel);
	tsk.pszFooter = szWWW;

	_wsprintf(szMsg, SKIPLEN(countof(szMsg)) L"Close and update\nto %s version %s%s",
		mb_DroppedMode ? L"dropped" : (mp_Set->isUpdateUseBuilds==1) ? L"new " CV_STABLE
		: (mp_Set->isUpdateUseBuilds==3) ? L"new " CV_PREVIEW : L"new " CV_DEVEL, ms_NewVersion,
		(mp_Set->UpdateDownloadSetup() == 1) ? (mp_Set->isSetup64 ? L".x64" : L".x86") : L" (7-Zip)");
	btns[0].nButtonID  = IDYES;    btns[0].pszButtonText = szMsg;
	btns[1].nButtonID  = IDNO; btns[1].pszButtonText = L"Postpone\nupdate will be started when you close ConEmu window";
	btns[2].nButtonID  = IDCANCEL; btns[2].pszButtonText = L"Cancel";

	tsk.dwFlags        = (hClassIcon?TDF_USE_HICON_MAIN:0)|TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_ENABLE_HYPERLINKS;
	tsk.pszWindowTitle = ms_DefaultTitle;
	tsk.pButtons       = btns;
	tsk.cButtons       = countof(btns);

	tsk.hwndParent     = ghWnd;

	hr = TaskDialog(&tsk, &iBtn, NULL, NULL);
	if (hr == S_OK)
		goto wrap;

MsgOnly:

	_wsprintf(szMsg, SKIPLEN(countof(szMsg))
		L"Do you want to close ConEmu and\n"
		L"update to %s version %s?\n\n"
		L"Press <No> to postpone.",
		mb_DroppedMode ? L"dropped" : (mp_Set->isUpdateUseBuilds==1) ? L"new " CV_STABLE
		: (mp_Set->isUpdateUseBuilds==3) ? L"new " CV_PREVIEW : L"new " CV_DEVEL, ms_NewVersion);

	iBtn = MsgBox(szMsg, MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONQUESTION|MB_YESNOCANCEL, ms_DefaultTitle, NULL, false);

wrap:
	InterlockedDecrement(&mn_InShowMsgBox);
	return iBtn;
}

int CConEmuUpdate::QueryConfirmationNoNewVer()
{
	if ((mn_InShowMsgBox > 0) || !gpConEmu)
		return 0; // Если отображается ошибка - не звать

	DontEnable de;
	InterlockedIncrement(&mn_InShowMsgBox);

	int iBtn = -1;
	HRESULT hr;
	CEStr szMsg, szInfo;
	wchar_t szCurVer[120];
	wchar_t szWWW[MAX_PATH];
	TASKDIALOGCONFIG tsk = {sizeof(tsk)};
	TASKDIALOG_BUTTON btns[2] = {};
	LPCWSTR pszServerPath = L"";
	wchar_t* pszBtn2 = NULL;

	_wsprintf(szCurVer, SKIPLEN(countof(szCurVer)) L"No newer %s version is available",
		(mp_Set->isUpdateUseBuilds==1) ? CV_STABLE : (mp_Set->isUpdateUseBuilds==3) ? CV_PREVIEW : CV_DEVEL);

	if (gOSVer.dwMajorVersion < 6)
		goto MsgOnly;

	tsk.hInstance = g_hInstance;
	if (hClassIcon)
		tsk.hMainIcon = hClassIcon;
	else
		tsk.pszMainIcon = MAKEINTRESOURCE(IDI_ICON1);

	tsk.pszMainInstruction = szCurVer;

	_wsprintf(szWWW, SKIPLEN(countof(szWWW)) L"<a href=\"%s\">%s</a>", gsWhatsNew, szWhatsNewLabel);
	tsk.pszFooter = szWWW;

	szInfo = CreateVersionOnServerInfo(true);
	szMsg = lstrmerge(
		L"Your current ConEmu version is ", ms_CurVerInfo, L"\n\n",
		(LPCWSTR)szInfo);
	tsk.pszContent = szMsg.ms_Val;

	btns[0].nButtonID  = IDOK;    btns[0].pszButtonText = L"OK";
	pszBtn2 = lstrmerge(L"Visit download page\n", gsDownlPage);
	btns[1].nButtonID  = IDNO;    btns[1].pszButtonText = pszBtn2;

	tsk.dwFlags        = (hClassIcon?TDF_USE_HICON_MAIN:0)|TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_ENABLE_HYPERLINKS;
	tsk.pszWindowTitle = ms_DefaultTitle;
	tsk.pButtons       = btns;
	tsk.cButtons       = countof(btns);

	tsk.hwndParent     = ghWnd;

	hr = TaskDialog(&tsk, &iBtn, NULL, NULL);
	if (hr == S_OK)
	{
		if (iBtn == IDNO)
			ConEmuAbout::OnInfo_DownloadPage();
		goto wrap;
	}

MsgOnly:
	szInfo = CreateVersionOnServerInfo(false, L"\n\n");
	szMsg = lstrmerge(
		L"Your current ConEmu version is ", ms_CurVerInfo, L"\n\n",
		(LPCWSTR)szInfo,
		(LPCWSTR)szCurVer);

	iBtn = MsgBox(szMsg, MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONINFORMATION|MB_OK, ms_DefaultTitle, NULL, false);

wrap:
	InterlockedDecrement(&mn_InShowMsgBox);
	SafeFree(pszBtn2);
	return 0;
}

short CConEmuUpdate::GetUpdateProgress()
{
	if (mb_RequestTerminate)
		return -1;

	switch (InUpdate())
	{
	case us_NotStarted:
	case us_PostponeUpdate:
		return -1;
	case us_Check:
		return mb_ManualCallMode ? 1 : -1;
	case us_ConfirmDownload:
		return UPD_PROGRESS_CONFIRM_DOWNLOAD;
	case us_Downloading:
		if (mn_PackageSize > 0)
		{
			int nValue = (mn_InternetContentReady * 88 / mn_PackageSize);
			if (nValue < 0)
				nValue = 0;
			else if (nValue > 88)
				nValue = 88;
			return nValue+UPD_PROGRESS_DOWNLOAD_START;
		}
		return UPD_PROGRESS_DOWNLOAD_START;
	case us_ConfirmUpdate:
		return UPD_PROGRESS_CONFIRM_UPDATE;
	case us_ExitAndUpdate:
		return UPD_PROGRESS_EXIT_AND_UPDATE;
	}

	return -1;
}

wchar_t* CConEmuUpdate::GetCurVerInfo()
{
	wchar_t* pszVerInfo = NULL;

	if (lstrcmpi(ms_NewVersion, ms_OurVersion) > 0)
	{
		pszVerInfo = lstrmerge(L"Obsolete, recommended update to ", ms_NewVersion,
			(mp_Set->isUpdateUseBuilds==1) ? L" " CV_STABLE : (mp_Set->isUpdateUseBuilds==3) ? L" " CV_PREVIEW : L" " CV_DEVEL);
	}
	else if (ms_CurVerInfo[0])
		pszVerInfo = lstrdup(ms_CurVerInfo);
	else
		pszVerInfo = lstrdup(ms_OurVersion);

	return pszVerInfo;
}

void CConEmuUpdate::WaitAllInstances()
{
	while (true)
	{
		bool bStillExists = false;
		wchar_t szMessage[255];
		wcscpy_c(szMessage, L"Please, close all ConEmu instances before continue");

		HWND hFind = FindWindowEx(NULL, NULL, VirtualConsoleClassMain, NULL);
		if (hFind)
		{
			bStillExists = true;
			DWORD nPID; GetWindowThreadProcessId(hFind, &nPID);
			_wsprintf(szMessage+_tcslen(szMessage), SKIPLEN(64) L"\nConEmu still running, PID=%u", nPID);
		}

		if (!bStillExists)
		{
			TODO("Можно бы проехаться по всем модулям запущенных процессов на предмет блокировки файлов в папках ConEmu");
		}

		// Если никого запущенного не нашли - выходим из цикла
		if (!bStillExists)
			return;

		// Ругнуться
		int nBtn = MsgBox(szMessage, MB_ICONEXCLAMATION|MB_OKCANCEL, ms_DefaultTitle, NULL, false);
		if (nBtn == IDCANCEL)
			return; // "Cancel" - means stop checking
	}
}

bool CConEmuUpdate::wininet::Init(CConEmuUpdate* apUpd)
{
	bool bRc = false;
	wchar_t* pszLib = NULL;
	HMODULE lhDll = NULL;
	DownloadCommand_t lDownloadCommand = NULL;

	//  Already initialized?
	if (hDll)
	{
		_ASSERTE(DownloadCommand!=NULL);
		// But check if local objects are created...
		if (!DownloadCommand(dc_Init, 0, NULL))
		{
			DWORD nErr = GetLastError(); // No use?
			pUpd->ReportError(L"Failed to re-initialize gpInet, code=%u", nErr);
			return false;
		}
		bRc = true;
		goto wrap;
	}

	pUpd = apUpd;

	pszLib = lstrmerge(gpConEmu->ms_ConEmuBaseDir, WIN3264TEST(L"\\ConEmuCD.dll",L"\\ConEmuCD64.dll"));
	lhDll = pszLib ? LoadLibrary(pszLib) : NULL;
	if (!lhDll)
	{
		_ASSERTE(lhDll!=NULL);
		lhDll = LoadLibrary(WIN3264TEST(L"ConEmuCD.dll",L"ConEmuCD64.dll"));
	}
	if (!lhDll)
	{
		DWORD nErr = GetLastError();
		pUpd->ReportError(L"Required library '%s' was not loaded, code=%u", WIN3264TEST(L"ConEmuCD.dll",L"ConEmuCD64.dll"), nErr);
		goto wrap;
	}

	lDownloadCommand = (DownloadCommand_t)GetProcAddress(lhDll, "DownloadCommand");
	if (!lDownloadCommand)
	{
		pUpd->ReportError(L"Exported function DownloadCommand in '%s' not found! Check your installation!",
			WIN3264TEST(L"ConEmuCD.dll",L"ConEmuCD64.dll"), GetLastError());
		goto wrap;
	}

	if (!lDownloadCommand(dc_Init, 0, NULL))
	{
		DWORD nErr = GetLastError(); // No use?
		pUpd->ReportError(L"Failed to initialize gpInet, code=%u", nErr);
		goto wrap;
	}

	hDll = lhDll;
	DownloadCommand = lDownloadCommand;

	bRc = true;
wrap:
	if (bRc)
	{
		SetCallback(dc_ErrCallback, ErrorCallback, (LPARAM)apUpd);
		SetCallback(dc_LogCallback, LogCallback, (LPARAM)apUpd);
	}
	SafeFree(pszLib);
	if (lhDll && !bRc)
	{
		FreeLibrary(lhDll);
	}
	return bRc;
}
bool CConEmuUpdate::wininet::Deinit(bool bFull)
{
	// if bFull - release allocated object too
	if (!DownloadCommand)
		return false;
	if (!DownloadCommand(bFull ? dc_Deinit : dc_Reset, 0, NULL))
		return false;
	return true;
}
void CConEmuUpdate::wininet::SetCallback(CEDownloadCommand cbk, FDownloadCallback pfnCallback, LPARAM lParam)
{
	CEDownloadErrorArg args[2];
	args[0].argType = at_Uint;  args[0].uintArg = (DWORD_PTR)pfnCallback;
	args[1].argType = at_Uint;  args[1].uintArg = lParam;
	_ASSERTE(dc_ErrCallback<=cbk && cbk<=dc_LogCallback);
	DownloadCommand(cbk, 2, args);
}

void CConEmuUpdate::ProgressCallback(const CEDownloadInfo* pError)
{
	_ASSERTE(pError && pError->lParam==(LPARAM)gpUpd);
	if (!pError || pError->argCount < 1 || pError->Args[0].argType != at_Uint)
	{
		_ASSERTE(pError->argCount >= 1 && pError->Args[0].argType == at_Uint);
		return;
	}
	CConEmuUpdate* pUpd = (CConEmuUpdate*)pError->lParam;
	if (!pUpd)
		return;
	pUpd->mn_InternetContentReady = pError->Args[0].uintArg; // How many bytes were read
	if (gpConEmu && ghWnd)
	{
		gpConEmu->UpdateProgress();
	}
}
void CConEmuUpdate::ErrorCallback(const CEDownloadInfo* pError)
{
	_ASSERTE(pError && pError->lParam==(LPARAM)gpUpd);
	if (!pError)
		return;
	CConEmuUpdate* pUpd = (CConEmuUpdate*)pError->lParam;
	if (!pUpd)
		return;
	wchar_t* pszText = pError->GetFormatted(false);
	pUpd->ReportErrorInt(pszText);
}
void CConEmuUpdate::LogCallback(const CEDownloadInfo* pError)
{
	_ASSERTE(pError && pError->lParam==(LPARAM)gpUpd);
	if (!pError)
		return;
	wchar_t* pszText = pError->GetFormatted(false);
	if (pszText)
	{
		CEStr lsLog(L"Update: ", pszText);
		gpConEmu->LogString(lsLog);
		free(pszText);
	}
}
