
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

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"

#define DEBUGSTRSCHED(s) //DEBUGSTR(s)

// Task Scheduler v2.0 (Vista and higher)
#if defined(CE_HAS_SCHEDULER_V2)
#include <taskschd.h>
#endif

// Task Scheduler v1.0 (Win2k, WinXP, Win2k3)
#include <MSTask.h>
// And some crazyness with ‘hidden features’
#include <LMCons.h>
#include <LMErr.h>
#include <Lmat.h>

#include "../common/MBSTR.h"
#include "../common/MModule.h"

#include "OptionsClass.h"
#include "version.h"


// Avoid linking to extra lib, declare guids here
#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const IID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }


// Task Scheduler helpers forward declarations

enum TaskSchedulerState
{
	tss_Failed = 0,
	tss_Unknown,
	tss_NotStarted,
	tss_Ready,
	tss_Running,
};

class CTaskSchedulerBase
{
protected:
	HRESULT hr;
	bool bCoInitialized;
	MBSTR bsTaskName;
	MBSTR bsApplication;
	MBSTR bsArguments;
	MBSTR bsDirectory;
public:
	CTaskSchedulerBase();
	virtual ~CTaskSchedulerBase();
public:
	virtual HRESULT Create(bool bAsSystem, LPCWSTR lpTaskName, LPCWSTR lpApplication, LPCWSTR lpArguments, LPCWSTR lpDirectory) = 0;
	virtual HRESULT Run() = 0;
	virtual TaskSchedulerState GetState() = 0;
public:
	void DisplaySchedulerError(LPCWSTR pszStep);
};

class CTaskScheduler1 : public CTaskSchedulerBase
{
protected:
	ITaskScheduler *pITS;
	ITask *pITask;
	IPersistFile *pIPersistFile;
	MModule* netApi;
	NET_API_STATUS (WINAPI* netScheduleJobAdd)(LPCWSTR Servername, LPBYTE Buffer, LPDWORD JobId);
	NET_API_STATUS (WINAPI* netScheduleJobDel)(LPCWSTR Servername, DWORD MinJobId, DWORD MaxJobId);
	DWORD jobId;
public:
	CTaskScheduler1();
	virtual ~CTaskScheduler1();
public:
	virtual HRESULT Create(bool bAsSystem, LPCWSTR lpTaskName, LPCWSTR lpApplication, LPCWSTR lpArguments, LPCWSTR lpDirectory) override;
	virtual HRESULT Run() override;
	virtual TaskSchedulerState GetState() override;
};

#if defined(CE_HAS_SCHEDULER_V2)
class CTaskScheduler2 : public CTaskSchedulerBase
{
protected:
	IRegisteredTask *pRegisteredTask;
	IRunningTask *pRunningTask;
	IAction *pAction;
	IExecAction *pExecAction;
	IActionCollection *pActionCollection;
	ITaskDefinition *pTask;
	ITaskSettings *pSettings;
	ITaskFolder *pRootFolder;
	ITaskService *pService;
	TASK_STATE taskState;
	bool bTaskCreated;
public:
	CTaskScheduler2();
	virtual ~CTaskScheduler2();
public:
	virtual HRESULT Create(bool bAsSystem, LPCWSTR lpTaskName, LPCWSTR lpApplication, LPCWSTR lpArguments, LPCWSTR lpDirectory) override;
	virtual HRESULT Run() override;
	virtual TaskSchedulerState GetState() override;
};
#endif


/// class CTaskSchedulerBase

CTaskSchedulerBase::CTaskSchedulerBase()
	: hr(E_UNEXPECTED)
	, bCoInitialized(false)
{
}

CTaskSchedulerBase::~CTaskSchedulerBase()
{
}

void CTaskSchedulerBase::DisplaySchedulerError(LPCWSTR pszStep)
{
	wchar_t szInfo[200] = L"";
	_wsprintf(szInfo, SKIPCOUNT(szInfo) L" Please check scheduler log.\n" L"HR=%u, TaskName=\"", (DWORD)hr);
	CEStr szErr(L"Scheduler: ", pszStep, szInfo, bsTaskName, L"\"\n", bsApplication, L" ", bsArguments);
	DisplayLastError(szErr, (DWORD)hr);
};


#if !defined(CE_HAS_SCHEDULER_V2)

WARNING("GNU <taskschd.h> does not have TaskScheduler support yet!")

#else

// *************************************
// Task Scheduler 2.0 (Vista and higher)
// *************************************

// {0F87369F-A4E5-4CFC-BD3E-73E6154572DD}
DEFINE_GUID(CLSID_TaskScheduler, 0x0f87369f, 0xa4e5, 0x4cfc, 0xbd, 0x3e, 0x73, 0xe6, 0x15, 0x45, 0x72, 0xdd);
// {4C3D624D-FD6B-49A3-B9B7-09CB3CD3F047}
DEFINE_GUID(IID_IExecAction,     0x4c3d624d, 0xfd6b, 0x49a3, 0xb9, 0xb7, 0x09, 0xcb, 0x3c, 0xd3, 0xf0, 0x47);
// {2FABA4C7-4DA9-4013-9697-20CC3FD40F85}
DEFINE_GUID(IID_ITaskService,    0x2faba4c7, 0x4da9, 0x4013, 0x96, 0x97, 0x20, 0xcc, 0x3f, 0xd4, 0x0f, 0x85);

// class CTaskScheduler2 : public CTaskSchedulerBase
// IRegisteredTask *pRegisteredTask;
// IRunningTask *pRunningTask;
// IAction *pAction;
// IExecAction *pExecAction;
// IActionCollection *pActionCollection;
// ITaskDefinition *pTask;
// ITaskSettings *pSettings;
// ITaskFolder *pRootFolder;
// ITaskService *pService;
// TASK_STATE taskState;
// bool bTaskCreated;

CTaskScheduler2::CTaskScheduler2()
	: pRegisteredTask(NULL)
	, pRunningTask(NULL)
	, pAction(NULL)
	, pExecAction(NULL)
	, pActionCollection(NULL)
	, pTask(NULL)
	, pSettings(NULL)
	, pRootFolder(NULL)
	, pService(NULL)
	, taskState((TASK_STATE)-1)
	, bTaskCreated(false)
{
}

CTaskScheduler2::~CTaskScheduler2()
{
	//Delete the task when done
	if (bTaskCreated)
	{
		hr = pRootFolder->DeleteTask(bsTaskName, NULL);
		if (FAILED(hr))
		{
			wchar_t szLog[80];
			_wsprintf(szLog, SKIPCOUNT(szLog) L"Scheduler: Failed delete created task, hr=x%08X", (DWORD)hr);
			LogString(szLog);
		}
	}

	SafeRelease(pRegisteredTask);
	SafeRelease(pRunningTask);
	SafeRelease(pAction);
	SafeRelease(pExecAction);
	SafeRelease(pActionCollection);
	SafeRelease(pTask);
	SafeRelease(pRootFolder);
	SafeRelease(pService);

	if (bCoInitialized)
	{
		CoUninitialize();
	}
}

HRESULT CTaskScheduler2::Create(bool bAsSystem, LPCWSTR lpTaskName, LPCWSTR lpApplication, LPCWSTR lpArguments, LPCWSTR lpDirectory)
{
	bsTaskName = lpTaskName;
	bsApplication = lpApplication;
	bsArguments = lpArguments;
	bsDirectory = lpDirectory;

	if (gpSet->isLogging())
	{
		CEStr lsLog(L"Scheduler: Creating Task: `", lpTaskName, L"`; Exe: `", lpApplication, L"`; Args: ", lpArguments, L"; Dir: `", lpDirectory, L"`");
		LogString(lsLog);
	}

	MBSTR bsIndefinitely(L"PT0S");
	MBSTR bsRoot(L"\\");

	// No need, using TASK_LOGON_INTERACTIVE_TOKEN now
	// -- VARIANT vtUsersSID = {VT_BSTR}; vtUsersSID.bstrVal = SysAllocString(L"S-1-5-32-545"); // Well Known SID for "\\Builtin\Users" group
	MBSTR bsSystemSID(L"S-1-5-18");
	VARIANT vtSystemSID = {VT_BSTR}; vtSystemSID.bstrVal = bsSystemSID;
	MBSTR bsZeroStr(L"");
	VARIANT vtZeroStr = {VT_BSTR}; vtZeroStr.bstrVal = bsZeroStr;
	VARIANT vtEmpty = {VT_EMPTY};

	//  ------------------------------------------------------
	//  Initialize COM.
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		DisplaySchedulerError(L"CoInitializeEx failed.");
		goto wrap;
	}
	bCoInitialized = true;

	//  Set general COM security levels.
	hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);
	if (FAILED(hr))
	{
		DisplaySchedulerError(L"CoInitializeSecurity failed.");
		goto wrap;
	}

	//  ------------------------------------------------------
	//  Create an instance of the Task Service.
	hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
	if (FAILED(hr))
	{
		DisplaySchedulerError(L"Failed to CoCreate an instance of the TaskService class.");
		goto wrap;
	}

	//  Connect to the task service.
	hr = pService->Connect(vtEmpty, vtEmpty, vtEmpty, vtEmpty);
	if (FAILED(hr))
	{
		DisplaySchedulerError(L"ITaskService::Connect failed.");
		goto wrap;
	}

	//  ------------------------------------------------------
	//  Get the pointer to the root task folder.  This folder will hold the
	//  new task that is registered.
	hr = pService->GetFolder(bsRoot, &pRootFolder);
	if( FAILED(hr) )
	{
		DisplaySchedulerError(L"Cannot get Root Folder pointer.");
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
		DisplaySchedulerError(L"Failed to create an instance of the TaskService class.");
		goto wrap;
	}

	SafeRelease(pService);  // COM clean up.  Pointer is no longer used.

	//  ------------------------------------------------------
	//  Ensure there will be no execution time limit
	hr = pTask->get_Settings(&pSettings);
	if (FAILED(hr))
	{
		DisplaySchedulerError(L"Cannot get task settings.");
		goto wrap;
	}
	hr = pSettings->put_ExecutionTimeLimit(bsIndefinitely);
	if (FAILED(hr))
	{
		DisplaySchedulerError(L"Cannot set task execution time limit.");
		goto wrap;
	}

	//  ------------------------------------------------------
	//  Add an Action to the task.

	//  Get the task action collection pointer.
	hr = pTask->get_Actions(&pActionCollection);
	if (FAILED(hr))
	{
		DisplaySchedulerError(L"Cannot get task collection pointer.");
		goto wrap;
	}

	//  Create the action, specifying that it is an executable action.
	hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
	if (FAILED(hr))
	{
		DisplaySchedulerError(L"pActionCollection->Create failed.");
		goto wrap;
	}
	SafeRelease(pActionCollection);  // COM clean up.  Pointer is no longer used.

	hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
	if( FAILED(hr) )
	{
		DisplaySchedulerError(L"pAction->QueryInterface failed.");
		goto wrap;
	}
	SafeRelease(pAction);

	//  Set the path of the executable to the user supplied executable.
	hr = pExecAction->put_Path(bsApplication);
	if (FAILED(hr))
	{
		DisplaySchedulerError(L"Cannot set path of executable.");
		goto wrap;
	}

	hr = pExecAction->put_Arguments(bsArguments);
	if (FAILED(hr))
	{
		DisplaySchedulerError(L"Cannot set arguments of executable.");
		goto wrap;
	}

	if (lpDirectory)
	{
		hr = pExecAction->put_WorkingDirectory(bsDirectory);
		if (FAILED(hr))
		{
			DisplaySchedulerError(L"Cannot set working directory of executable.");
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
		DisplaySchedulerError(L"Error registering the task instance.");
		goto wrap;
	}
	else
	{
		CEStr lsLog(L"Scheduler: Task `", bsTaskName, L"` created successfully");
		LogString(lsLog);
	}

	bTaskCreated = true;

wrap:
	return hr;
}

HRESULT CTaskScheduler2::Run()
{
	if (gpSet->isLogging())
	{
		CEStr lsLog(L"Scheduler: Starting Task: `", bsTaskName, L"`");
		LogString(lsLog);
	}

	VARIANT vtEmpty = {VT_EMPTY};
	//  ------------------------------------------------------
	//  Run the task
	//LONG nSessionId = (LONG)apiGetConsoleSessionID();
	_ASSERTE(pRunningTask==NULL);
	//RunEx fails to run "As System" properly...
	//hr = pRegisteredTask->RunEx(vtEmpty, (nSessionId > 0) ? TASK_RUN_USE_SESSION_ID : 0, nSessionId, NULL, &pRunningTask);
	hr = pRegisteredTask->Run(vtEmpty, &pRunningTask);
	if (FAILED(hr))
	{
		DisplaySchedulerError(L"Failed to start the task instance.");
	}
	else
	{
		wchar_t szLog[100];
		_wsprintf(szLog, SKIPCOUNT(szLog) L"Scheduler: Run result: 0x%08X", (DWORD)hr);
		LogString(szLog);

		GetState();
	}
	return hr;
}

TaskSchedulerState CTaskScheduler2::GetState()
{
	wchar_t szLog[80];
	HRESULT hrRun;

	hrRun = pRunningTask ? pRunningTask->get_State(&taskState) : E_POINTER;
	if (FAILED(hrRun))
	{
		if (hrRun == SCHED_E_TASK_NOT_RUNNING)
			wcscpy_c(szLog, L"Scheduler: Current task state: SCHED_E_TASK_NOT_RUNNING");
		else
			_wsprintf(szLog, SKIPCOUNT(szLog) L"Scheduler: Failed to get task state: hr=x%08X", (DWORD)hrRun);
		LogString(szLog);
		return tss_Failed;
	}

	TaskSchedulerState state = tss_Unknown;
	wcscpy_c(szLog, L"Scheduler: Current task state: ");

	switch (taskState)
	{
	case TASK_STATE_RUNNING:
		wcscat_c(szLog, L"TASK_STATE_RUNNING");
		state = tss_Running;
		break;
	case TASK_STATE_READY:
		wcscat_c(szLog, L"TASK_STATE_READY");
		state = tss_Ready;
		break;
	case TASK_STATE_QUEUED:
		wcscat_c(szLog, L"TASK_STATE_QUEUED");
		break;
	case TASK_STATE_DISABLED:
		wcscat_c(szLog, L"TASK_STATE_DISABLED");
		break;
	case TASK_STATE_UNKNOWN:
		wcscat_c(szLog, L"TASK_STATE_UNKNOWN");
		break;
	default:
		_wsprintf(szLog+wcslen(szLog), SKIPLEN(countof(szLog)-wcslen(szLog)) L"%u", (DWORD)taskState);
	}

	LogString(szLog);
	return state;
}

#endif // #if !defined(CE_HAS_SCHEDULER_V2) .. #else ..



// ******************************
// Task Scheduler 1.0 (pre-Vista)
// ******************************

// {148BD52A-A2AB-11CE-B11F-00AA00530503}
DEFINE_GUID(CLSID_CTaskScheduler, 0x148BD52A, 0xA2AB, 0x11CE, 0xB1, 0x1F, 0x00, 0xAA, 0x00, 0x53, 0x05, 0x03);
// {148BD527-A2AB-11CE-B11F-00AA00530503}
DEFINE_GUID(IID_ITaskScheduler, 0x148BD527L, 0xA2AB, 0x11CE, 0xB1, 0x1F, 0x00, 0xAA, 0x00, 0x53, 0x05, 0x03);
// {148BD520-A2AB-11CE-B11F-00AA00530503}
DEFINE_GUID(CLSID_CTask, 0x148BD520, 0xA2AB, 0x11CE, 0xB1, 0x1F, 0x00, 0xAA, 0x00, 0x53, 0x05, 0x03);
// {148BD524-A2AB-11CE-B11F-00AA00530503}
DEFINE_GUID(IID_ITask, 0x148BD524L, 0xA2AB, 0x11CE, 0xB1, 0x1F, 0x00, 0xAA, 0x00, 0x53, 0x05, 0x03);

// class CTaskScheduler1 : public CTaskSchedulerBase
// ITaskScheduler *pITS;
// ITask *pITask;
// IPersistFile *pIPersistFile;
// MModule* netApi;
// NET_API_STATUS (WINAPI* netScheduleJobAdd)(LPCWSTR Servername, LPBYTE Buffer, LPDWORD JobId);
// NET_API_STATUS (WINAPI* netScheduleJobDel)(LPCWSTR Servername, DWORD MinJobId, DWORD MaxJobId);
// DWORD jobId;
CTaskScheduler1::CTaskScheduler1()
	: pITS(NULL)
	, pITask(NULL)
	, pIPersistFile(NULL)
	, netApi(NULL)
	, netScheduleJobAdd(NULL)
	, netScheduleJobDel(NULL)
	, jobId((DWORD)-1)
{
}

CTaskScheduler1::~CTaskScheduler1()
{
	SafeRelease(pIPersistFile);
	SafeRelease(pITask);
	SafeRelease(pITS);

	if (jobId != (DWORD)-1)
	{
		netScheduleJobDel(NULL, jobId, jobId);
	}

	SafeDelete(netApi);

	if (bCoInitialized)
	{
		CoUninitialize();
	}
}

HRESULT CTaskScheduler1::Create(bool bAsSystem, LPCWSTR lpTaskName, LPCWSTR lpApplication, LPCWSTR lpArguments, LPCWSTR lpDirectory)
{
	// ‘Official’ Task Scheduler 1.0 can't run process interactively, so it's tricky
	// -- bsTaskName = lpTaskName;
	bsTaskName = L"<NetSchedulerTask>"; // To be changed after netScheduleJobAdd

	bsApplication = lpApplication;
	bsArguments = lpArguments;
	bsDirectory = lpDirectory;

	if (!bAsSystem)
	{
		DisplaySchedulerError(L"TaskScheduler v1.0 must not be user to run process demoted!");
		goto wrap;
	}

	if (gpSet->isLogging())
	{
		CEStr lsLog(L"Scheduler: Creating Task: `", lpTaskName, L"`; Exe: `", lpApplication, L"`; Args: ", lpArguments, L"; Dir: `", lpDirectory, L"`");
		LogString(lsLog);
	}

	netApi = new MModule(L"Netapi32.dll");
	if (!netApi
		|| !netApi->GetProcAddress("NetScheduleJobAdd", netScheduleJobAdd)
		|| !netApi->GetProcAddress("NetScheduleJobDel", netScheduleJobDel)
		)
	{
		DisplaySchedulerError(L"Failed to initialize Netapi32 library!");
		goto wrap;
	}

	//  ------------------------------------------------------
	//  Initialize COM.
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		DisplaySchedulerError(L"CoInitializeEx failed.");
		goto wrap;
	}
	bCoInitialized = true;

	//  ------------------------------------------------------
	//  Create an instance of the Task Service (will be used to run Task).
	hr = CoCreateInstance(CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskScheduler, (void**)&pITS);
	if (FAILED(hr))
	{
		DisplaySchedulerError(L"Failed to CoCreate an instance of the TaskService class.");
		goto wrap;
	}

	_ASSERTE(bAsSystem);
	//  ------------------------------------------------------
	{
		// Running the command interactively is tricky
		AT_INFO at = {};
		CEStr lsCmdLine(L"\"", lpApplication, L"\" ", lpArguments);
		at.Command = lsCmdLine.ms_Val;
		DWORD jobId = (DWORD)-1;
		NET_API_STATUS rc = netScheduleJobAdd(NULL, (LPBYTE)&at, &jobId);
		wchar_t szLog[120], szName[20];
		if (rc != NERR_Success)
		{
			_wsprintf(szLog, SKIPCOUNT(szLog) L"NetScheduleJobAdd failed, rc=%u", rc);
			hr = (HRESULT)-1;
			DisplaySchedulerError(szLog);
			goto wrap;
		}

		// Successfully created, but the name is ‘unknown’, only JobID
		_wsprintf(szName, SKIPCOUNT(szName) L"At%u", jobId);
		bsTaskName = szName;
		_wsprintf(szLog, SKIPCOUNT(szLog) L"Scheduler: NetScheduleJobAdd succeeded, jobId=%u", jobId);
		LogString(szLog);

		// Check if we can access the task via Task Scheduler 1.0 interface (to Run it)
		SafeRelease(pITask);
		hr = pITS ? pITS->Activate(bsTaskName, IID_ITask, (IUnknown**)&pITask) : E_POINTER;
		if (FAILED(hr) || !pITask)
		{
			// Prepare error message
			_wsprintf(szLog, SKIPCOUNT(szLog) L"NetScheduleJobAdd succeeded, jobId=%u, but ITask is not found", jobId);
			// Delete the job immediately
			netScheduleJobDel(NULL, jobId, jobId);
			jobId = (DWORD)-1;
			// Warn user
			DisplaySchedulerError(szLog);
			goto wrap;
		}

		// Done, interactive System task was created
	}
	#if 0
	else // !bSystem
	{
		hr = pITS->NewWorkItem(bsTaskName, CLSID_CTask, IID_ITask, (IUnknown**)&pITask);
		if (FAILED(hr))
		{
			DisplaySchedulerError(L"Failed to create new work item.");
			goto wrap;
		}

		//  ------------------------------------------------------
		// Account info

		#if 0
		{
			// MSBUG? Failed to start LocalSystem+Interactive. Always started NonInteractively.
			_ASSERTE(FALSE && "Force logged user account!");

			hr = pITask->SetAccountInformation(MBSTR(L""), NULL);
			if (FAILED(hr))
			{
				DisplaySchedulerError(L"Cannot set account to ‘LocalSystem’.");
				goto wrap;
			}

			hr = pITask->SetFlags(TASK_FLAG_INTERACTIVE);
			if (FAILED(hr))
			{
				DisplaySchedulerError(L"Cannot set flags to ‘Interactive’.");
				goto wrap;
			}
		}
		#endif

		{
			//hr = pITask->SetAccountInformation(MBSTR(L""), NULL);

			hr = pITask->SetFlags(TASK_FLAG_RUN_ONLY_IF_LOGGED_ON);
			if (FAILED(hr))
			{
				DisplaySchedulerError(L"Cannot set flags to ‘OnlyIfLoggedOn’.");
				goto wrap;
			}
		}

		//  Set the path of the executable to the user supplied executable.
		hr = pITask->SetApplicationName(bsApplication);
		if (FAILED(hr))
		{
			DisplaySchedulerError(L"Cannot set path of executable.");
			goto wrap;
		}

		hr = pITask->SetParameters(bsArguments);
		if (FAILED(hr))
		{
			DisplaySchedulerError(L"Cannot set arguments of executable.");
			goto wrap;
		}

		if (lpDirectory)
		{
			hr = pITask->SetWorkingDirectory(bsDirectory);
			if (FAILED(hr))
			{
				DisplaySchedulerError(L"Cannot set working directory of executable.");
				goto wrap;
			}
		}

		// Create the task in the scheduler
		hr = pITask->QueryInterface(IID_IPersistFile, (void**)&pIPersistFile);
		if (FAILED(hr))
		{
			DisplaySchedulerError(L"QueryInterface(IID_IPersistFile) failed.");
			goto wrap;
		}

		SafeRelease(pITask);

		// This would create `*.job` file in the Scheduler folder
		hr = pIPersistFile->Save(NULL, TRUE);
		if (FAILED(hr))
		{
			DisplaySchedulerError(L"Failed to save task definition.");
			goto wrap;
		}
	}
	#endif

	// All done
wrap:
	return hr;
}

HRESULT CTaskScheduler1::Run()
{
	SafeRelease(pITask);
	hr = pITS ? pITS->Activate(bsTaskName, IID_ITask, (IUnknown**)&pITask) : E_POINTER;
	if (FAILED(hr))
	{
		DisplaySchedulerError(L"Failed to get interface for the task.");
		goto wrap;
	}

	if (gpSet->isLogging())
	{
		CEStr lsLog(L"Scheduler: Starting Task: `", bsTaskName, L"`");
		LogString(lsLog);
	}

	hr = pITask->Run();
	if (FAILED(hr))
	{
		DisplaySchedulerError(L"Failed to run the task.");
	}
	else
	{
		wchar_t szLog[80];
		_wsprintf(szLog, SKIPCOUNT(szLog) L"Scheduler: Run result: 0x%08X", (DWORD)hr);
		LogString(szLog);

		GetState();
	}

wrap:
	return hr;
}

TaskSchedulerState CTaskScheduler1::GetState()
{
	wchar_t szLog[80];
	HRESULT hrStatus = E_UNEXPECTED;
	HRESULT hrRun = pITask ? pITask->GetStatus(&hrStatus) : E_POINTER;

	if (FAILED(hrRun))
	{
		_wsprintf(szLog, SKIPCOUNT(szLog) L"Scheduler: Failed to get task state, hr=x%08X", (DWORD)hrRun);
		LogString(szLog);
		return tss_Failed;
	}

	TaskSchedulerState state = tss_Unknown;
	wcscpy_c(szLog, L"Scheduler: Current task state: ");

	switch ((DWORD)hrStatus)
	{
	case SCHED_S_TASK_RUNNING:
		wcscat_c(szLog, L"SCHED_S_TASK_RUNNING");
		state = tss_Running;
		break;
	case SCHED_S_TASK_READY:
		wcscat_c(szLog, L"SCHED_S_TASK_READY");
		state = tss_Ready;
		break;
	case SCHED_S_TASK_NOT_SCHEDULED:
		wcscat_c(szLog, L"SCHED_S_TASK_NOT_SCHEDULED");
		break;
	case SCHED_S_TASK_HAS_NOT_RUN:
		wcscat_c(szLog, L"SCHED_S_TASK_HAS_NOT_RUN");
		break;
	case SCHED_S_TASK_DISABLED:
		wcscat_c(szLog, L"SCHED_S_TASK_DISABLED");
		break;
	case SCHED_S_TASK_NO_MORE_RUNS:
		wcscat_c(szLog, L"SCHED_S_TASK_NO_MORE_RUNS");
		break;
	case SCHED_S_TASK_NO_VALID_TRIGGERS:
		wcscat_c(szLog, L"SCHED_S_TASK_NO_VALID_TRIGGERS");
		break;
	default:
		_wsprintf(szLog+wcslen(szLog), SKIPLEN(countof(szLog)-wcslen(szLog)) L"0x%08X", (DWORD)hrStatus);
	}

	LogString(szLog);
	return state;
}

/// The function starts new process using Windows Task Scheduler
/// This allows to run process ‘Demoted’ (bAsSystem == false)
/// or under ‘System’ account (bAsSystem == true)
BOOL CreateProcessScheduled(bool bAsSystem, DWORD anSessionId, LPWSTR lpCommandLine,
						   LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes,
						   BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment,
						   LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation,
						   LPDWORD pdwLastError /*= NULL*/)
{
	if (!lpCommandLine || !*lpCommandLine)
	{
		DisplayLastError(L"CreateProcessDemoted failed, lpCommandLine is empty", -1);
		return FALSE;
	}

	// This issue is not actual anymore, because we call put_ExecutionTimeLimit(‘Infinite’)
	// -- Issue 1897: Created task stopped after 72 hour, so use "/bypass"
	CEStr szExe;
	if (!GetModuleFileName(NULL, szExe.GetBuffer(MAX_PATH), MAX_PATH) || szExe.IsEmpty())
	{
		DisplayLastError(L"GetModuleFileName(NULL) failed");
		return FALSE;
	}
	wchar_t szInteractive[32] = L" ";
	if (bAsSystem)
		msprintf(szInteractive, countof(szInteractive), L"-interactive:%u ", anSessionId); // with trailing space
	CEStr szCommand(
		gpSet->isLogging() ? L"-log " : NULL,
		lpCurrentDirectory ? L"-dir \"" : NULL, lpCurrentDirectory, lpCurrentDirectory ? L"\" " : NULL,
		bAsSystem ? szInteractive : L"-apparent ",
		lpCommandLine);
	LPCWSTR pszCmdArgs = szCommand;

	BOOL lbRc = FALSE;

	// Task Scheduler do not bring created tasks OnTop
	HWND hPrevEmu = FindWindowEx(NULL, NULL, VirtualConsoleClassMain, NULL);
	HWND hCreated = NULL;

	DWORD nTickStart, nDuration;
	const DWORD nMaxWindowWait = 30000;
	const DWORD nMaxSystemWait = 30000;

	// The only really working method: Run cmd-line via task scheduler
	// But there is one caveat: Task scheduler may be disabled on PC!

	wchar_t szUniqTaskName[128];
	wchar_t szVer4[8] = L""; lstrcpyn(szVer4, _T(MVV_4a), countof(szVer4));
	_wsprintf(szUniqTaskName, SKIPLEN(countof(szUniqTaskName)) L"ConEmu %02u%02u%02u%s starter ParentPID=%u", (MVV_1%100),MVV_2,MVV_3,szVer4, GetCurrentProcessId());

	HRESULT hr;
	TaskSchedulerState state;

	CTaskSchedulerBase* pSchd = NULL;

	if (IsWin6())
	{
		#if !defined(CE_HAS_SCHEDULER_V2)

		WARNING("GNU <taskschd.h> does not have TaskScheduler support yet!");
		DisplayLastError(L"Scheduler: GNU <taskschd.h> does not have TaskScheduler support yet!", (DWORD)-1);
		goto wrap;

		#else

		pSchd = new CTaskScheduler2();

		#endif
	}
    else
	{
		pSchd = new CTaskScheduler1();
	}

	if (!pSchd)
	{
		DisplayLastError(L"Scheduler: Failed to create Task Scheduler interface (memory allocation).", GetLastError());
		goto wrap;
	}

	if (FAILED(hr = pSchd->Create(bAsSystem, szUniqTaskName, szExe, szCommand, lpCurrentDirectory)))
	{
		// Already warned
		goto wrap;
	}

	if (FAILED(hr = pSchd->Run()))
	{
		// Already warned
		goto wrap;
	}

	state = pSchd->GetState();

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
			if ((state = pSchd->GetState()) == tss_Failed)
				break;
			if (state == tss_Ready)
				break; // OK, started
			if (state == tss_Running)
				break; // Already finished?

			Sleep(100);
			nDuration = (GetTickCount() - nTickStart);
		}
	}
	else
	{
		// Success! Program was started.
		// Give 30 seconds for new window appears and bring it to front
		if (IsConEmuGui(szExe)) // "ConEmu.exe" or "ConEmu64.exe"
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
					if ((hTop == hPrevEmu)
						|| !IsWindowVisible(hTop)
						|| IsIconic(hTop))
					{
						continue;
					}

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

	// Done

wrap:
	SafeDelete(pSchd);
	if (pdwLastError) *pdwLastError = hr;
	return lbRc;
}
