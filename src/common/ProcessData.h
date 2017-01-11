// This is extraction of Far Process List plugin.

#include "WObjects.h"

class CProcessData
{
protected:

	struct UNICODE_STRING
	{
		USHORT  Length; USHORT  MaximumLength; PWSTR  Buffer;
	};

	struct ModuleData
	{
		LIST_ENTRY              InLoadOrderModuleList;
		LIST_ENTRY              InMemoryOrderModuleList;
		LIST_ENTRY              InInitializationOrderModuleList;
		PVOID                   BaseAddress;
		PVOID                   EntryPoint;
		ULONG                   SizeOfImage;
		UNICODE_STRING          FullDllName;
		UNICODE_STRING          BaseDllName;
		ULONG                   Flags;
		SHORT                   LoadCount;
		SHORT                   TlsIndex;
		LIST_ENTRY              HashTableEntry;
		ULONG                   TimeDateStamp;
	};

	struct RTL_DRIVE_LETTER_CURDIR
	{
		USHORT                  Flags;
		USHORT                  Length;
		ULONG                   TimeStamp;
		UNICODE_STRING          DosPath;
	};

	struct PROCESS_PARAMETERS
	{
		ULONG                   MaximumLength;
		ULONG                   Length;
		ULONG                   Flags;
		ULONG                   DebugFlags;
		PVOID                   ConsoleHandle;
		ULONG                   ConsoleFlags;
		HANDLE                  StdInputHandle;
		HANDLE                  StdOutputHandle;
		HANDLE                  StdErrorHandle;
		UNICODE_STRING          CurrentDirectoryPath;
		HANDLE                  CurrentDirectoryHandle;
		UNICODE_STRING          DllPath;
		UNICODE_STRING          ImagePathName;
		UNICODE_STRING          CommandLine;
		PVOID                   EnvironmentBlock;
		ULONG                   StartingPositionLeft;
		ULONG                   StartingPositionTop;
		ULONG                   Width;
		ULONG                   Height;
		ULONG                   CharWidth;
		ULONG                   CharHeight;
		ULONG                   ConsoleTextAttributes;
		ULONG                   WindowFlags;
		ULONG                   ShowWindowFlags;
		UNICODE_STRING          WindowTitle;
		UNICODE_STRING          DesktopName;
		UNICODE_STRING          ShellInfo;
		UNICODE_STRING          RuntimeData;
		RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
	};

	struct PEB_LDR_DATA
	{
		ULONG                   Length;
		BOOLEAN                 Initialized;
		PVOID                   SsHandle;
		LIST_ENTRY              InLoadOrderModuleList;
		LIST_ENTRY              InMemoryOrderModuleList;
		LIST_ENTRY              InInitializationOrderModuleList;
	};

	struct PEB
	{
		BOOLEAN                 InheritedAddressSpace;
		BOOLEAN                 ReadImageFileExecOptions;
		BOOLEAN                 BeingDebugged;
		BOOLEAN                 Spare;
		HANDLE                  Mutant;
		PVOID                   ImageBaseAddress;
		PEB_LDR_DATA            *LoaderData;
		PROCESS_PARAMETERS      *ProcessParameters;
		PVOID                   SubSystemData;
		PVOID                   ProcessHeap;
		PVOID                   FastPebLock;
	//  PPEBLOCKROUTINE         FastPebLockRoutine;
		PVOID         FastPebLockRoutine;
	//  PPEBLOCKROUTINE         FastPebUnlockRoutine;
		PVOID         FastPebUnlockRoutine;
		ULONG                   EnvironmentUpdateCount;
		PVOID                  *KernelCallbackTable;
		PVOID                   EventLogSection;
		PVOID                   EventLog;
	//  PPEB_FREE_BLOCK         FreeList;
		PVOID         FreeList;
		ULONG                   TlsExpansionCounter;
		PVOID                   TlsBitmap;
		ULONG                   TlsBitmapBits[0x2];
		PVOID                   ReadOnlySharedMemoryBase;
		PVOID                   ReadOnlySharedMemoryHeap;
		PVOID                   *ReadOnlyStaticServerData;
		PVOID                   AnsiCodePageData;
		PVOID                   OemCodePageData;
		PVOID                   UnicodeCaseTableData;
		ULONG                   NumberOfProcessors;
		ULONG                   NtGlobalFlag;
		BYTE                    Spare2[0x4];
		LARGE_INTEGER           CriticalSectionTimeout;
		ULONG                   HeapSegmentReserve;
		ULONG                   HeapSegmentCommit;
		ULONG                   HeapDeCommitTotalFreeThreshold;
		ULONG                   HeapDeCommitFreeBlockThreshold;
		ULONG                   NumberOfHeaps;
		ULONG                   MaximumNumberOfHeaps;
		PVOID                  **ProcessHeaps;
		PVOID                   GdiSharedHandleTable;
		PVOID                   ProcessStarterHelper;
		PVOID                   GdiDCAttributeList;
		PVOID                   LoaderLock;
		ULONG                   OSMajorVersion;
		ULONG                   OSMinorVersion;
		ULONG                   OSBuildNumber;
		ULONG                   OSPlatformId;
		ULONG                   ImageSubSystem;
		ULONG                   ImageSubSystemMajorVersion;
		ULONG                   ImageSubSystemMinorVersion;
		ULONG                   GdiHandleBuffer[0x22];
		ULONG                   PostProcessInitRoutine;
		ULONG                   TlsExpansionBitmap;
		BYTE                    TlsExpansionBitmapBits[0x80];
		ULONG                   SessionId;
	};

	typedef struct _PROCESS_BASIC_INFORMATION
	{
		PVOID Reserved1;
		PEB   *PebBaseAddress;
		PVOID Reserved2[2];
		ULONG_PTR UniqueProcessId;
		PVOID Reserved3;
	} PROCESS_BASIC_INFORMATION;

	typedef enum _PROCESSINFOCLASS
	{
		ProcessBasicInformation = 0,
		ProcessWow64Information = 26
	} PROCESSINFOCLASS;

	#ifndef STATUS_NOT_IMPLEMENTED
	#define STATUS_NOT_IMPLEMENTED           ((LONG)0xC0000002L)
	#endif
	#ifndef STATUS_INFO_LENGTH_MISMATCH
	#define STATUS_INFO_LENGTH_MISMATCH      ((LONG)0xC0000004L)
	#endif

	static LONG WINAPI fNtQueryInformationProcess(
	    HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG)
	{ return STATUS_NOT_IMPLEMENTED; }

	BOOL GetInternalProcessData(HANDLE hProcess, ModuleData* Data, PROCESS_PARAMETERS* &pProcessParams, char*&pEnd, bool bFirstModule=false)
	{
		DWORD ret;
		// From ntddk.h
		PROCESS_BASIC_INFORMATION processInfo;

		typedef LONG (WINAPI *PNtQueryInformationProcess)(HANDLE,PROCESSINFOCLASS,PVOID,ULONG,PULONG);
		static PNtQueryInformationProcess pNtQueryInformationProcess = NULL;

		if (!pNtQueryInformationProcess)
		{
			HMODULE h;
			if ((h = GetModuleHandle(L"ntdll")) != NULL)
			{
				pNtQueryInformationProcess = (PNtQueryInformationProcess)GetProcAddress(h, "NtQueryInformationProcess");
			}

			if (!pNtQueryInformationProcess)
				pNtQueryInformationProcess = fNtQueryInformationProcess;
		}

		if (pNtQueryInformationProcess(hProcess, ProcessBasicInformation, &processInfo, sizeof(processInfo), &ret))
			return FALSE;

		char *p4;
		//FindModule, obtained from PSAPI.DLL
		PVOID hModule;
		PEB peb;
		PEB_LDR_DATA pld;

		if (ReadProcessMemory(hProcess, processInfo.PebBaseAddress, &peb, sizeof(peb), 0) &&
		        ReadProcessMemory(hProcess, peb.LoaderData, &pld, sizeof(pld), 0))
		{
			//pEnd = (void *)((void *)peb.LoaderData+((void *)&pld.InMemoryOrderModuleList-(void *)&pld));
			hModule = peb.ImageBaseAddress;
			pProcessParams = peb.ProcessParameters;
			pEnd = (char *)peb.LoaderData+sizeof(pld)-sizeof(LIST_ENTRY)*2;
			p4 = (char *)pld.InMemoryOrderModuleList.Flink;

			while (p4)
			{
				if (p4==pEnd || !ReadProcessMemory(hProcess, p4-sizeof(PVOID)*2, Data, sizeof(*Data), 0))
					return FALSE;

				if (bFirstModule)
					return TRUE;

				if (Data->BaseAddress==hModule) break;

				p4 = (char *)Data->InMemoryOrderModuleList.Flink;
			}
		}

		return TRUE;
	}



	void GetOpenProcessData(HANDLE hProcess, wchar_t* pProcessName, DWORD cbProcessName,
	                          wchar_t* pFullPath, DWORD cbFullPath, wchar_t* pCommandLine, DWORD cbCommandLine)
	{
		ModuleData Data={};
		char *pEnd;
		PROCESS_PARAMETERS* pProcessParams = 0;

		if (GetInternalProcessData(hProcess, &Data, pProcessParams, pEnd))
		{
			WCHAR szProcessName[MAX_PATH];
			memset(szProcessName, 0, sizeof(szProcessName));

			if (pProcessName)
			{
				SIZE_T sz = sizeof(szProcessName);//min(sizeof(szProcessName), Data.BaseDllName.MaximumLength*2);

				if (Data.BaseDllName.Buffer && ReadProcessMemory(hProcess, Data.BaseDllName.Buffer, szProcessName, sz,0))
					lstrcpyn(pProcessName, szProcessName, cbProcessName);
				else
					*pProcessName = 0;
			}

			if (pFullPath)
			{
				SIZE_T sz = sizeof(szProcessName);//min(sizeof(szProcessName), Data.FullDllName.MaximumLength*2);

				if (Data.FullDllName.Buffer && ReadProcessMemory(hProcess, Data.FullDllName.Buffer, szProcessName, sz,0))
					lstrcpyn(pFullPath, szProcessName, cbFullPath);
				else
					*pFullPath = 0;
			}

			if (pCommandLine)
			{
				*pCommandLine = 0;

				UNICODE_STRING pCmd;

				if (ReadProcessMemory(hProcess, &pProcessParams->CommandLine, &pCmd, sizeof(pCmd), 0))
				{
					SIZE_T sz = min(cbCommandLine, (ULONG)pCmd.Length/sizeof(WCHAR) + 1);
					WCHAR* sCommandLine = (WCHAR*)malloc(sz*sizeof(WCHAR));
					if (sCommandLine)
					{
						if (ReadProcessMemory(hProcess, pCmd.Buffer, sCommandLine, (sz-1)*sizeof(WCHAR),0))
						{
							sCommandLine[sz-1] = 0;
							lstrcpyn(pCommandLine, sCommandLine, cbCommandLine);
						}

						free(sCommandLine);
					}
				}
			}
		}
	};

protected:
	wchar_t szSubKey[1024];
	PPERF_DATA_BLOCK mp_ProcPerfData; // Для поиска имени процесса
	DWORD  mn_ProcPerfDataSize;
	DWORD  dwProcessIdTitle;

	// look backwards for the counter number
	static int getcounter(wchar_t* p)
	{
		wchar_t* p2;

		for (p2 = p-2; isDigit(*p2); p2--)
			;

		wchar_t *pe = NULL;
		int n = wcstol(p2+1, &pe, 10);
		return n;
	};

	bool LoadPerfData()
	{
		// Already loaded?
		if (mp_ProcPerfData != NULL)
			return true;

		#define INITIAL_SIZE        512000
		#define EXTEND_SIZE         256000

		dwProcessIdTitle = 0;

		LONG rc;
		LANGID lid = MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL);
		_wsprintf(szSubKey, SKIPLEN(countof(szSubKey)) L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Perflib\\%03X", lid);
		HKEY hKeyNames = NULL;
		if (0 != (rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_READ, &hKeyNames)))
			return false;

		szSubKey[0] = 0;

		// Get the buffer size for the counter names
		DWORD dwType = 0, dwSize = 0;

		if (0 == (rc = RegQueryValueEx(hKeyNames, L"Counters", 0, &dwType, NULL, &dwSize)) && dwSize)
		{
			// Allocate the counter names buffer
			wchar_t* buf = (wchar_t*)malloc(dwSize*sizeof(wchar_t));
			if (buf)
			{
				// read the counter names from the registry
				if (0 == (rc = RegQueryValueEx(hKeyNames, L"Counters", 0, &dwType, (BYTE*)buf, &dwSize)))
				{
					// now loop thru the counter names looking for the following counters:
					//      1.  "Process"           process name
					//      2.  "ID Process"        process id
					// the buffer contains multiple null terminated strings and then
					// finally null terminated at the end.  the strings are in pairs of
					// counter number and counter name.
					wchar_t* buf_end = (wchar_t*)(((char*)buf)+dwSize);
					for (wchar_t* p = buf; p < buf_end && *p; p += lstrlen(p)+1)
					{
						if (lstrcmpi(p, L"Process")==0)
							_wsprintf(szSubKey, SKIPLEN(countof(szSubKey)) L"%i", getcounter(p));
						else if (!dwProcessIdTitle && lstrcmpi(p, L"ID Process")==0)
							dwProcessIdTitle = getcounter(p);
					}
				}
				free(buf);
			}
		}
		RegCloseKey(hKeyNames);

		if (*szSubKey)
		{
			if (!mp_ProcPerfData)
			{
				mn_ProcPerfDataSize = INITIAL_SIZE;
				mp_ProcPerfData = (PPERF_DATA_BLOCK)malloc(mn_ProcPerfDataSize);
			}

			while (1)
			{
				dwSize = mn_ProcPerfDataSize;

				while ((rc = RegQueryValueEx(HKEY_PERFORMANCE_DATA, szSubKey, 0, &dwType, (LPBYTE)mp_ProcPerfData, &dwSize))
						==ERROR_LOCK_FAILED)  ; //Just retry

				// check for success and valid perf data block signature
				if ((rc == ERROR_SUCCESS) &&
						!memcmp(mp_ProcPerfData->Signature, L"PERF", 8))
				{
					break;
				}

				if (rc == ERROR_MORE_DATA)
				{
					dwSize += EXTEND_SIZE;
					void* pNew = malloc(dwSize);
					if (!pNew)
					{
						free(mp_ProcPerfData);
						mp_ProcPerfData = NULL;
						return false;
					}
					mp_ProcPerfData = (PPERF_DATA_BLOCK)pNew;
					mn_ProcPerfDataSize = dwSize;
				}
				else if (rc < 0x100000)  // ??? sometimes we receive garbage in rc
				{
					free(mp_ProcPerfData);
					mp_ProcPerfData = NULL;
					return false;
				}
			}
		}

		return (mp_ProcPerfData != NULL);
	};

	bool FindProcessName(DWORD anPID, wchar_t* pProcessName, DWORD cbProcessName)
	{
		bool lbFound = false;

		if (LoadPerfData())
		{
			PPERF_OBJECT_TYPE pObj = (PPERF_OBJECT_TYPE)(((DWORD_PTR)mp_ProcPerfData) + mp_ProcPerfData->HeaderLength);
			PPERF_INSTANCE_DEFINITION pInst = (PPERF_INSTANCE_DEFINITION)(((DWORD_PTR)pObj) + pObj->DefinitionLength);
			// loop thru the performance counter definition records looking
			// for the process id counter and then save its offset
			PPERF_COUNTER_DEFINITION pCounterDef =
				(PPERF_COUNTER_DEFINITION)(((DWORD_PTR)pObj) + pObj->HeaderLength);
			PPERF_COUNTER_DEFINITION pDataEnd =
				(PPERF_COUNTER_DEFINITION)(((DWORD_PTR)mp_ProcPerfData) + mn_ProcPerfDataSize - sizeof(PERF_COUNTER_DEFINITION));

			DWORD dwProcessIdCounter = 0;

			for (DWORD iNC = pObj->NumCounters; iNC && (pCounterDef <= pDataEnd); --iNC)
			{
				if (pCounterDef->CounterNameTitleIndex == dwProcessIdTitle)
				{
					dwProcessIdCounter = pCounterDef->CounterOffset;

					for (LONG iNI = pObj->NumInstances; iNI > 0; --iNI)
					{
						PPERF_COUNTER_BLOCK pCounter = (PPERF_COUNTER_BLOCK)((DWORD_PTR)pInst + pInst->ByteLength);
						DWORD dwProcessId = *((LPDWORD)((DWORD_PTR)pCounter + dwProcessIdCounter));

						if (dwProcessId == anPID)
						{
							if (pProcessName && (cbProcessName > 5))
							{
								lstrcpyn(pProcessName, (LPCWSTR)((DWORD_PTR)pInst + pInst->NameOffset), cbProcessName - 5);
								if (dwProcessId>8)
									lstrcat(pProcessName, L".exe");
							}
							lbFound = true;
							break;
						}

						pInst = (PPERF_INSTANCE_DEFINITION)((DWORD_PTR)pCounter + pCounter->ByteLength);
					}

					break;
				}
				pCounterDef++;
			}
		}

		return lbFound;
	};

protected:
	HMODULE mh_Kernel;
	HMODULE mh_Psapi;
	typedef DWORD (WINAPI* FGetModuleFileNameEx)(HANDLE hProcess,HMODULE hModule,LPWSTR lpFilename,DWORD nSize);
	BOOL mb_Is64BitOs;
	typedef BOOL (WINAPI* IsWow64Process_t)(HANDLE hProcess, PBOOL Wow64Process);
	IsWow64Process_t IsWow64Process_f;
	typedef BOOL (WINAPI* QueryFullProcessImageNameW_t)(HANDLE hProcess, DWORD dwFlags, LPWSTR lpExeName, PDWORD lpdwSize);

protected:
	bool queryFullProcessImageName(HANDLE hProcess, DWORD dwFlags, LPWSTR lpExeName, DWORD dwSize)
	{
		QueryFullProcessImageNameW_t QueryFullProcessImageNameW_f = (QueryFullProcessImageNameW_t)(mh_Kernel ? GetProcAddress(mh_Kernel, "QueryFullProcessImageNameW") : NULL);
		if (!QueryFullProcessImageNameW_f)
			return false;
		if (!QueryFullProcessImageNameW_f(hProcess, dwFlags, lpExeName, &dwSize))
			return false;
		return true;
	};

	DWORD getModuleFileNameEx(HANDLE hProcess, HMODULE hModule, LPWSTR lpFilename, DWORD nSize)
	{
		DWORD nRc = 0;

		FGetModuleFileNameEx GetModuleFileNameEx = (FGetModuleFileNameEx)(mh_Kernel ? GetProcAddress(mh_Kernel, "GetModuleFileNameExW") : NULL);
		if (GetModuleFileNameEx == NULL)
		{
			if (!mh_Psapi)
				mh_Psapi = LoadLibrary(L"psapi.dll");
			if (mh_Psapi)
				GetModuleFileNameEx = (FGetModuleFileNameEx)GetProcAddress(mh_Psapi, "GetModuleFileNameExW");
		}

		if (GetModuleFileNameEx)
		{
			nRc = GetModuleFileNameEx(hProcess, hModule, lpFilename, nSize);
		}

		return nRc;
	};

public:

	CProcessData()
	{
		szSubKey[0] = 0;
		mp_ProcPerfData = NULL;
		mn_ProcPerfDataSize = 0;
    	dwProcessIdTitle = 0;

		mh_Psapi = NULL;

		mh_Kernel = GetModuleHandleW(L"kernel32.dll");
		IsWow64Process_f = (IsWow64Process_t)(mh_Kernel ? GetProcAddress(mh_Kernel, "IsWow64Process") : NULL);

		mb_Is64BitOs = IsWindows64();
	};

	~CProcessData()
	{
		if (mp_ProcPerfData)
		{
			free(mp_ProcPerfData);
			mp_ProcPerfData = NULL;
		}
		if (mh_Psapi)
		{
			FreeLibrary(mh_Psapi);
		}
	};

public:
	bool GetProcessName(DWORD anPID, wchar_t* pProcessName, DWORD cbProcessName, wchar_t* pProcessPath, DWORD cbProcessPath, int* pnProcessBits)
	{
		bool lbFound = false;

		#if defined(_DEBUG)
		DWORD nErr = 0, nErr1 = (DWORD)-1, nErr2 = (DWORD)-1, nErr3 = (DWORD)-1;
		#endif

		HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, anPID);
		if (h == NULL)
		{
			#if defined(_DEBUG)
			nErr = nErr1 = GetLastError();
			#endif

			h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, anPID);
			if (h == NULL)
			{
				#if defined(_DEBUG)
				nErr = nErr2 = GetLastError();
				#endif

				// Last chance for Vista+
				// May be we can get full path by this handle, but IsWow64Process would be successful
				if (IsWin6())
				{
					// PROCESS_QUERY_LIMITED_INFORMATION not defined in GCC
					h = OpenProcess(0x1000/*PROCESS_QUERY_LIMITED_INFORMATION*/, FALSE, anPID);

					#ifdef _DEBUG
					if (h == NULL)
						nErr = nErr3 = GetLastError();
					#endif
				}
			}
		}

		#ifdef _DEBUG
		if (h == NULL)
		{
			// Most usually this will be "Access denied", check others?
			_ASSERTE(nErr == ERROR_ACCESS_DENIED);
		}
		#endif

		if (h == INVALID_HANDLE_VALUE)
		{
			_ASSERTE(h!=INVALID_HANDLE_VALUE); // Unexpected here!
			h = NULL;
		}

		if (pProcessName)
			*pProcessName = 0;
		if (pProcessPath)
			*pProcessPath = 0;

		// If process handle was opened successfully, we don't need to scan performance counters...
		if (h != NULL)
		{
			lbFound = true;
		}
		else
		{
			lbFound = FindProcessName(anPID, pProcessName, cbProcessName);
		}

		// Let's try to load full path and bitness
		if (h != NULL)
		{
			if (pnProcessBits)
			{
				*pnProcessBits = mb_Is64BitOs ? 0 : 32;

				if (mb_Is64BitOs && IsWow64Process_f)
				{
					BOOL lbWow64 = FALSE;
					if (IsWow64Process_f(h, &lbWow64))
					{
						*pnProcessBits = lbWow64 ? 32 : 64;
					}
				}
			}

			if (pProcessPath || (pProcessName && !*pProcessName))
			{
				DWORD cchMax = max(MAX_PATH*2,cbProcessPath);
				wchar_t* pszPath = (wchar_t*)malloc(cchMax*sizeof(wchar_t));

				if (pszPath)
				{
					*pszPath = 0;

					if (*pszPath == 0)
					{
						if (!queryFullProcessImageName(h, 0, pszPath, cchMax))
							*pszPath = 0;
					}

					if (*pszPath == 0)
					{
						if (!getModuleFileNameEx(h, NULL, pszPath, cchMax))
							*pszPath = 0;
					}

					if (*pszPath == 0)
					{
						GetOpenProcessData(h, NULL, 0, pszPath, cchMax, NULL, 0);
					}

					if (*pszPath != 0)
					{
						if (pProcessPath)
							lstrcpyn(pProcessPath, pszPath, cbProcessPath);
						if (pProcessName && !*pProcessName)
							lstrcpyn(pProcessName, PointToName(pszPath), cbProcessName);
					}
					/*
					else if (pProcessPath && pProcessName && *pProcessName)
					{
						lstrcpyn(pProcessPath, pProcessName, cbProcessPath);
					}
					*/

					free(pszPath);
				}
			}
		}

		if (!lbFound)
		{
			if (pProcessName)
				*pProcessName = 0;
			if (pProcessPath)
				*pProcessPath = 0;
		}

		SafeCloseHandle(h);

		return lbFound;
	};
};
