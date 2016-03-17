
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
#define SHOWDEBUGSTR

#include "Header.h"

#if !defined(__GNUC__) || defined(__MINGW32__)
#include <shobjidl.h>
#include <propkey.h>
#include <taskschd.h>
#include "../common/MBSTR.h"
#endif
#include "../common/TokenHelper.h"
#include "../common/WSession.h"

#include "OptionsClass.h"
#include "version.h"

//#define DEBUGSTRMOVE(s) //DEBUGSTR(s)


BOOL CreateProcessRestricted(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
							 LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
							 BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment,
							 LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
							 LPDWORD pdwLastError)
{
	BOOL lbRc = FALSE;
	HANDLE hToken = NULL, hTokenRest = NULL;

	if (OpenProcessToken(GetCurrentProcess(),
	                    TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY | TOKEN_EXECUTE,
	                    &hToken))
	{
		enum WellKnownAuthorities
		{
			NullAuthority = 0, WorldAuthority, LocalAuthority, CreatorAuthority,
			NonUniqueAuthority, NtAuthority, MandatoryLabelAuthority = 16
		};
		SID *pAdmSid = (SID*)calloc(sizeof(SID)+sizeof(DWORD)*2,1);
		pAdmSid->Revision = SID_REVISION;
		pAdmSid->SubAuthorityCount = 2;
		pAdmSid->IdentifierAuthority.Value[5] = NtAuthority;
		pAdmSid->SubAuthority[0] = SECURITY_BUILTIN_DOMAIN_RID;
		pAdmSid->SubAuthority[1] = DOMAIN_ALIAS_RID_ADMINS;
		SID_AND_ATTRIBUTES sidsToDisable[] =
		{
			{pAdmSid}
		};

		#ifdef __GNUC__
		HMODULE hAdvApi = GetModuleHandle(L"AdvApi32.dll");
		CreateRestrictedToken_t CreateRestrictedToken = (CreateRestrictedToken_t)GetProcAddress(hAdvApi, "CreateRestrictedToken");
		#endif

		if (CreateRestrictedToken(hToken, DISABLE_MAX_PRIVILEGE,
		                         countof(sidsToDisable), sidsToDisable,
		                         0, NULL, 0, NULL, &hTokenRest))
		{
			if (CreateProcessAsUserW(hTokenRest, lpApplicationName, lpCommandLine,
							 lpProcessAttributes, lpThreadAttributes,
							 bInheritHandles, dwCreationFlags, lpEnvironment,
							 lpCurrentDirectory, lpStartupInfo, lpProcessInformation))
			{
				lbRc = TRUE;
			}
			else
			{
				if (pdwLastError) *pdwLastError = GetLastError();
			}

			CloseHandle(hTokenRest); hTokenRest = NULL;
		}
		else
		{
			if (pdwLastError) *pdwLastError = GetLastError();
		}

		free(pAdmSid);
		CloseHandle(hToken); hToken = NULL;
	}
	else
	{
		if (pdwLastError) *pdwLastError = GetLastError();
	}

	return lbRc;
}

BOOL CreateProcessInteractive(DWORD anSessionId, LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
							 LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
							 BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment,
							 LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
							 LPDWORD pdwLastError /*= NULL*/)
{
	BOOL lbRc = FALSE;
	HANDLE hToken = NULL, hTokenRest = NULL;
	wchar_t szLog[128];
	DWORD nErrCode = 0, nCurSession = (DWORD)-1, nNewSessionId = (DWORD)-1, nRetSize = 0;

	if (!OpenProcessToken(GetCurrentProcess(),
	                    TOKEN_DUPLICATE | TOKEN_ADJUST_SESSIONID | TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY | TOKEN_EXECUTE,
	                    &hToken))
	{
		nErrCode = GetLastError();
		_wsprintf(szLog, SKIPCOUNT(szLog) L"Failed to OpenProcessToken, code=%u", nErrCode);
		if (pdwLastError) *pdwLastError = nErrCode;
	}
	else
	{
		if (!GetTokenInformation(hToken, TokenSessionId, &nCurSession, sizeof(nCurSession), &nRetSize))
			_wsprintf(szLog, SKIPCOUNT(szLog) L"Failed to query TokenSessionId, code=%u", GetLastError());
		else
			_wsprintf(szLog, SKIPCOUNT(szLog) L"Current TokenSessionId is %u", nCurSession);
		LogString(szLog);
		nNewSessionId = (anSessionId == (DWORD)-1) ? apiGetConsoleSessionID() : anSessionId;

		if (!DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &hTokenRest))
		{
			_wsprintf(szLog, SKIPCOUNT(szLog) L"Failed to duplicate current token, code=%u", GetLastError());
			LogString(szLog);
		}
		else
		{
			CAdjustProcessToken tcbName;
			if (tcbName.Enable(1, SE_TCB_NAME) != 1)
			{
				_wsprintf(szLog, SKIPCOUNT(szLog) L"Failed to adjust SE_TCB_NAME privilege, code=%u", GetLastError());
				LogString(szLog);
			}

			if (!SetTokenInformation(hTokenRest, TokenSessionId, &nNewSessionId, sizeof(nNewSessionId)))
			{
				nErrCode = GetLastError();
				_wsprintf(szLog, SKIPCOUNT(szLog) L"Failed to adjust TokenSessionId to %u, code=%u", nNewSessionId, nErrCode);
				LogString(szLog);
				if (pdwLastError) *pdwLastError = nErrCode;
			}
			else
			{
				tcbName.Release();

				if (!GetTokenInformation(hTokenRest, TokenSessionId, &nCurSession, sizeof(nCurSession), &nRetSize))
					_wsprintf(szLog, SKIPCOUNT(szLog) L"Failed to query new TokenSessionId, code=%u", GetLastError());
				else
					_wsprintf(szLog, SKIPCOUNT(szLog) L"New TokenSessionId is %u", nCurSession);
				LogString(szLog);

				if (CreateProcessAsUserW(hTokenRest, lpApplicationName, lpCommandLine,
								 lpProcessAttributes, lpThreadAttributes,
								 bInheritHandles, dwCreationFlags, lpEnvironment,
								 lpCurrentDirectory, lpStartupInfo, lpProcessInformation))
				{
					lbRc = TRUE;
				}
				else
				{
					nErrCode = GetLastError();
					_wsprintf(szLog, SKIPCOUNT(szLog) L"CreateProcessAsUserW failed, code=%u", nErrCode);
					LogString(szLog);
					if (pdwLastError) *pdwLastError = nErrCode;
				}

			}

			CloseHandle(hTokenRest); hTokenRest = NULL;
		}

		CloseHandle(hToken); hToken = NULL;
	}

	return lbRc;
}

#if !defined(__GNUC__)
static void DisplayShedulerError(LPCWSTR pszStep, HRESULT hr, LPCWSTR bsTaskName, LPCWSTR lpCommandLine)
{
	wchar_t szInfo[200] = L"";
	_wsprintf(szInfo, SKIPCOUNT(szInfo) L" Please check sheduler log.\n" L"HR=%u, TaskName=", (DWORD)hr);
	CEStr szErr(pszStep, szInfo, bsTaskName, L"\n", lpCommandLine);
	DisplayLastError(szErr, (DWORD)hr);
}
#endif


/// The function starts new process using Windows Task Scheduler
/// This allows to run process ‘Demoted’ (bAsSystem == false)
/// or under ‘System’ account (bAsSystem == true)
static BOOL CreateProcessScheduled(bool bAsSystem, LPWSTR lpCommandLine,
						   LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
						   BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment,
						   LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
						   LPDWORD pdwLastError)
{
	if (!lpCommandLine || !*lpCommandLine)
	{
		DisplayLastError(L"CreateProcessDemoted failed, lpCommandLine is empty", -1);
		return FALSE;
	}

	// This issue is not actual anymore, because we call put_ExecutionTimeLimit(‘Infinite’)
	// Issue 1897: Created task stopped after 72 hour, so use "/bypass"
	CEStr szExe;
	if (!GetModuleFileName(NULL, szExe.GetBuffer(MAX_PATH), MAX_PATH) || szExe.IsEmpty())
	{
		DisplayLastError(L"GetModuleFileName(NULL) failed");
		return FALSE;
	}
	CEStr szCommand(
		gpSetCls->isAdvLogging ? L"-log " : NULL,
		bAsSystem ? L"-interactive " : L"-bypass ",
		lpCommandLine);
	LPCWSTR pszCmdArgs = szCommand;

	BOOL lbRc = FALSE;

#if defined(__GNUC__)

	DisplayLastError(L"GNU <taskschd.h> does not have TaskScheduler support yet!", (DWORD)-1);

#else

	// Task не выносит окна созданных задач "наверх"
	HWND hPrevEmu = FindWindowEx(NULL, NULL, VirtualConsoleClassMain, NULL);
	HWND hCreated = NULL;


	// Really working method: Run cmd-line via task scheduler
	// But there is one caveat: Task scheduler may be disable on PC!

	wchar_t szUniqTaskName[128];
	wchar_t szVer4[8] = L""; lstrcpyn(szVer4, _T(MVV_4a), countof(szVer4));
	_wsprintf(szUniqTaskName, SKIPLEN(countof(szUniqTaskName)) L"ConEmu %02u%02u%02u%s starter ParentPID=%u", (MVV_1%100),MVV_2,MVV_3,szVer4, GetCurrentProcessId());

	BSTR bsTaskName = SysAllocString(szUniqTaskName);
	BSTR bsExecutablePath = SysAllocString(szExe);
	BSTR bsArgs = SysAllocString(pszCmdArgs ? pszCmdArgs : L"");
	BSTR bsDir = lpCurrentDirectory ? SysAllocString(lpCurrentDirectory) : NULL;
	BSTR bsRoot = SysAllocString(L"\\");

	// No need, using TASK_LOGON_INTERACTIVE_TOKEN now
	// -- VARIANT vtUsersSID = {VT_BSTR}; vtUsersSID.bstrVal = SysAllocString(L"S-1-5-32-545"); // Well Known SID for "\\Builtin\Users" group
	MBSTR szSystemSID(L"S-1-5-18");
	VARIANT vtSystemSID = {VT_BSTR}; vtSystemSID.bstrVal = szSystemSID;
	VARIANT vtZeroStr = {VT_BSTR}; vtZeroStr.bstrVal = SysAllocString(L"");
	VARIANT vtEmpty = {VT_EMPTY};

	const IID CLSID_TaskScheduler = {0x0f87369f, 0xa4e5, 0x4cfc, {0xbd, 0x3e, 0x73, 0xe6, 0x15, 0x45, 0x72, 0xdd}};
	const IID IID_IExecAction     = {0x4c3d624d, 0xfd6b, 0x49a3, {0xb9, 0xb7, 0x09, 0xcb, 0x3c, 0xd3, 0xf0, 0x47}};
	const IID IID_ITaskService    = {0x2faba4c7, 0x4da9, 0x4013, {0x96, 0x97, 0x20, 0xcc, 0x3f, 0xd4, 0x0f, 0x85}};

	IRegisteredTask *pRegisteredTask = NULL;
	IRunningTask *pRunningTask = NULL;
	IAction *pAction = NULL;
	IExecAction *pExecAction = NULL;
	IActionCollection *pActionCollection = NULL;
	ITaskDefinition *pTask = NULL;
	ITaskSettings *pSettings = NULL;
	ITaskFolder *pRootFolder = NULL;
	ITaskService *pService = NULL;
	HRESULT hr;
	TASK_STATE taskState, prevTaskState;
	bool bCoInitialized = false;
	DWORD nTickStart, nDuration;
	const DWORD nMaxWindowWait = 30000;
	const DWORD nMaxSystemWait = 30000;
	MBSTR szIndefinitely(L"PT0S");

	//  ------------------------------------------------------
	//  Initialize COM.
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"CoInitializeEx failed.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}
	bCoInitialized = true;

	//  Set general COM security levels.
	hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"CoInitializeSecurity failed.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	//  ------------------------------------------------------
	//  Create an instance of the Task Service.
	hr = CoCreateInstance( CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService );
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Failed to CoCreate an instance of the TaskService class.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	//  Connect to the task service.
	hr = pService->Connect(vtEmpty, vtEmpty, vtEmpty, vtEmpty);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"ITaskService::Connect failed.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	//  ------------------------------------------------------
	//  Get the pointer to the root task folder.  This folder will hold the
	//  new task that is registered.
	hr = pService->GetFolder(bsRoot, &pRootFolder);
	if( FAILED(hr) )
	{
		DisplayShedulerError(L"Cannot get Root Folder pointer.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	//  Check if the same task already exists. If the same task exists, remove it.
	hr = pRootFolder->DeleteTask(bsTaskName, 0);
	// We are creating "unique" task name, admitting it can't exists already, so ignore deletion error
	UNREFERENCED_PARAMETER(hr);

	//  Create the task builder object to create the task.
	hr = pService->NewTask(0, &pTask);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Failed to create an instance of the TaskService class.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	SafeRelease(pService);  // COM clean up.  Pointer is no longer used.

	//  ------------------------------------------------------
	//  Ensure there will be no execution time limit
	hr = pTask->get_Settings(&pSettings);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Cannot get task settings.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}
	hr = pSettings->put_ExecutionTimeLimit(szIndefinitely);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Cannot set task execution time limit.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	//  ------------------------------------------------------
	//  Add an Action to the task.

	//  Get the task action collection pointer.
	hr = pTask->get_Actions(&pActionCollection);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Cannot get task collection pointer.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	//  Create the action, specifying that it is an executable action.
	hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"pActionCollection->Create failed.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}
	SafeRelease(pActionCollection);  // COM clean up.  Pointer is no longer used.

	hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
	if( FAILED(hr) )
	{
		DisplayShedulerError(L"pAction->QueryInterface failed.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}
	SafeRelease(pAction);

	//  Set the path of the executable to the user supplied executable.
	hr = pExecAction->put_Path(bsExecutablePath);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Cannot set path of executable.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	hr = pExecAction->put_Arguments(bsArgs);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Cannot set arguments of executable.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	if (bsDir)
	{
		hr = pExecAction->put_WorkingDirectory(bsDir);
		if (FAILED(hr))
		{
			DisplayShedulerError(L"Cannot set working directory of executable.", hr, bsTaskName, lpCommandLine);
			goto wrap;
		}
	}

	//  ------------------------------------------------------
	//  Save the task in the root folder.
	hr = pRootFolder->RegisterTaskDefinition(bsTaskName, pTask, TASK_CREATE,
		//vtUsersSID, vtEmpty, TASK_LOGON_GROUP, // gh-571 - this may start process as wrong user
		bAsSystem ? vtSystemSID : vtEmpty, vtEmpty, bAsSystem ? TASK_LOGON_SERVICE_ACCOUNT : TASK_LOGON_INTERACTIVE_TOKEN,
		vtZeroStr, &pRegisteredTask);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Error registering the task instance.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	prevTaskState = (TASK_STATE)-1;

	//  ------------------------------------------------------
	//  Run the task
	hr = pRegisteredTask->Run(vtEmpty, &pRunningTask);
	if (FAILED(hr))
	{
		DisplayShedulerError(L"Error starting the task instance.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}

	HRESULT hrRun; hrRun = pRunningTask->get_State(&prevTaskState);

	//  ------------------------------------------------------
	// Success! Task successfully started. But our task may end
	// promptly because it just bypassing the command line
	lbRc = TRUE;

	if (bAsSystem)
	{
		nTickStart = GetTickCount();
		nDuration = 0;
		_ASSERTE(hCreated==NULL);
		hCreated = NULL;

		while (nDuration <= nMaxSystemWait/*30000*/)
		{
			hrRun = pRunningTask->get_State(&taskState);
			if (FAILED(hr))
				break;
			if (taskState == TASK_STATE_RUNNING)
				break; // OK, started
			if (taskState == TASK_STATE_READY)
				break; // Already finished?

			Sleep(100);
			nDuration = (GetTickCount() - nTickStart);
		}
	}
	else
	{
		// Success! Program was started.
		// Give 30 seconds for new window appears and bring it to front
		LPCWSTR pszExeName = PointToName(szExe);
		if (lstrcmpi(pszExeName, L"ConEmu.exe") == 0 || lstrcmpi(pszExeName, L"ConEmu64.exe") == 0)
		{
			nTickStart = GetTickCount();
			nDuration = 0;
			_ASSERTE(hCreated==NULL);
			hCreated = NULL;

			while (nDuration <= nMaxWindowWait/*30000*/)
			{
				HWND hTop = NULL;
				while ((hTop = FindWindowEx(NULL, hTop, VirtualConsoleClassMain, NULL)) != NULL)
				{
					if (!IsWindowVisible(hTop) || (hTop == hPrevEmu))
						continue;

					hCreated = hTop;
					SetForegroundWindow(hCreated);
					break;
				}

				if (hCreated != NULL)
				{
					// Window found, activated
					break;
				}

				Sleep(100);
				nDuration = (GetTickCount() - nTickStart);
			}
		}
		// Window activation end
	}

	//Delete the task when done
	hr = pRootFolder->DeleteTask(bsTaskName, NULL);
	if( FAILED(hr) )
	{
		DisplayShedulerError(L"CoInitializeEx failed.", hr, bsTaskName, lpCommandLine);
		goto wrap;
	}
	// Task successfully deleted

wrap:
	// Clean up
	SysFreeString(bsTaskName);
	SysFreeString(bsExecutablePath);
	SysFreeString(bsArgs);
	if (bsDir) SysFreeString(bsDir);
	SysFreeString(bsRoot);
	//VariantClear(&vtUsersSID);
	VariantClear(&vtZeroStr);
	SafeRelease(pRegisteredTask);
	SafeRelease(pRunningTask);
	SafeRelease(pAction);
	SafeRelease(pExecAction);
	SafeRelease(pActionCollection);
	SafeRelease(pTask);
	SafeRelease(pRootFolder);
	SafeRelease(pService);
	// Finalize
	if (bCoInitialized)
		CoUninitialize();
	if (pdwLastError) *pdwLastError = hr;
	// End of Task-scheduler mode
#endif

	return lbRc;
}

BOOL CreateProcessSystem(LPWSTR lpCommandLine,
						 LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
						 BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment,
						 LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
						 LPDWORD pdwLastError /*= NULL*/)
{
	BOOL lbRc;

	lbRc = CreateProcessScheduled(true, lpCommandLine,
						   lpProcessAttributes, lpThreadAttributes,
						   bInheritHandles, dwCreationFlags, lpEnvironment,
						   lpCurrentDirectory, lpStartupInfo, lpProcessInformation,
						   pdwLastError);

	return lbRc;
}

BOOL CreateProcessDemoted(LPWSTR lpCommandLine,
						  LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
						  BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment,
						  LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
						  LPDWORD pdwLastError)
{
	BOOL lbRc;

	lbRc = CreateProcessScheduled(false, lpCommandLine,
						   lpProcessAttributes, lpThreadAttributes,
						   bInheritHandles, dwCreationFlags, lpEnvironment,
						   lpCurrentDirectory, lpStartupInfo, lpProcessInformation,
						   NULL);

	// If all methods fails - try to execute "as is"?
	if (!lbRc)
	{
		lbRc = CreateProcess(NULL, lpCommandLine,
							 lpProcessAttributes, lpThreadAttributes,
							 bInheritHandles, dwCreationFlags, lpEnvironment,
							 lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
		if (pdwLastError) *pdwLastError = GetLastError();
	}

	return lbRc;
}
