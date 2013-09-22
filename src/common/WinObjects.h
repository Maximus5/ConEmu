
/*
Copyright (c) 2009-2013 Maximus5
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

#include <windows.h>
#include <wchar.h>
#include <CommCtrl.h>
#include "common.hpp"
#include "MSecurity.h"
#include "ConEmuCheck.h"


// GCC fixes
#ifndef KEY_WOW64_32KEY
#define KEY_WOW64_32KEY 0x0200
#endif
#ifndef KEY_WOW64_64KEY
#define KEY_WOW64_64KEY 0x0100
#endif

// WinAPI wrappers
BOOL apiSetForegroundWindow(HWND ahWnd);
BOOL apiShowWindow(HWND ahWnd, int anCmdShow);
BOOL apiShowWindowAsync(HWND ahWnd, int anCmdShow);
void getWindowInfo(HWND ahWnd, wchar_t (&rsInfo)[1024]);

typedef BOOL (WINAPI* IsWindow_t)(HWND hWnd);
extern IsWindow_t Is_Window;
BOOL isWindow(HWND hWnd);

typedef BOOL (WINAPI* AttachConsole_t)(DWORD dwProcessId);


// Some WinAPI related functions
wchar_t* GetShortFileNameEx(LPCWSTR asLong, BOOL abFavorLength=TRUE);
BOOL FileCompare(LPCWSTR asFilePath1, LPCWSTR asFilePath2);
BOOL FileExists(LPCWSTR asFilePath, DWORD* pnSize = NULL);
BOOL FileExistsSearch(wchar_t* rsFilePath, size_t cchPathMax);
BOOL DirectoryExists(LPCWSTR asPath);
BOOL MyCreateDirectory(wchar_t* asPath);
BOOL IsFilePath(LPCWSTR asFilePath);
BOOL IsUserAdmin();
BOOL GetLogonSID (HANDLE hToken, wchar_t **ppszSID);
bool IsWine();
bool IsDbcs();
bool IsWindows64();
int GetProcessBits(DWORD nPID, HANDLE hProcess = NULL);
BOOL CheckCallbackPtr(HMODULE hModule, size_t ProcCount, FARPROC* CallBack, BOOL abCheckModuleInfo, BOOL abAllowNTDLL = FALSE);
bool IsModuleValid(HMODULE module);
typedef struct tagPROCESSENTRY32W PROCESSENTRY32W;
bool GetProcessInfo(DWORD nPID, PROCESSENTRY32W* Info);
bool isTerminalMode();
bool IsFarExe(LPCWSTR asModuleName);

void RemoveOldComSpecC();
const wchar_t* PointToName(const wchar_t* asFullPath);
const char* PointToName(const char* asFileOrPath);
const wchar_t* PointToExt(const wchar_t* asFullPath);
const wchar_t* Unquote(wchar_t* asParm);
wchar_t* ExpandMacroValues(LPCWSTR pszFormat, LPCWSTR* pszValues, size_t nValCount);
wchar_t* ExpandEnvStr(LPCWSTR pszCommand);
wchar_t* GetFullPathNameEx(LPCWSTR asPath);

BOOL IsExecutable(LPCWSTR aszFilePathName, wchar_t** rsExpandedVars = NULL);
BOOL IsNeedCmd(BOOL bRootCmd, LPCWSTR asCmdLine, LPCWSTR* rsArguments, BOOL *rbNeedCutStartEndQuot,
			   wchar_t (&szExe)[MAX_PATH+1],
			   BOOL& rbRootIsCmdExe, BOOL& rbAlwaysConfirmExit, BOOL& rbAutoDisableConfirmExit);

//BOOL FindConEmuBaseDir(wchar_t (&rsConEmuBaseDir)[MAX_PATH+1], wchar_t (&rsConEmuExe)[MAX_PATH+1]);


COORD MyGetLargestConsoleWindowSize(HANDLE hConsoleOutput);

struct CE_CONSOLE_HISTORY_INFO
{
	UINT  cbSize;
	UINT  HistoryBufferSize;
	UINT  NumberOfHistoryBuffers;
	DWORD dwFlags;
};

struct CE_HANDLE_INFO
{
	HANDLE hStd;
	DWORD  nMode;
};

struct CEStartupEnv
{
	size_t cbSize;
	STARTUPINFOW si;
	DWORD OsBits;
	HWND  hConWnd;
	DWORD nConVisible;
	OSVERSIONINFOEXW os;
	CE_CONSOLE_HISTORY_INFO hi;
	CE_HANDLE_INFO hIn, hOut, hErr;
	LPCWSTR pszCmdLine;
	LPCWSTR pszExecMod;
	LPCWSTR pszWorkDir;
	LPCWSTR pszPathEnv;
	size_t  cchPathLen;
	BOOL    bIsWine; // »нформационно!
	BOOL    bIsReactOS;
	BOOL    bIsDbcs;
	UINT    nAnsiCP, nOEMCP;
	UINT    nMonitorsCount;
	RECT    rcMonitor[16], rcMonitorWork[16];
	// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Console\TrueTypeFont
	LPCWSTR pszRegConFonts; // "Index/CP ~t Name ~t Index/CP ..."
};

//CEStartupEnv* LoadStartupEnv();

#ifndef CONEMU_MINIMAL
HANDLE DuplicateProcessHandle(DWORD anTargetPID);
void FindComspec(ConEmuComspec* pOpt); // используетс€ в GUI при загрузке настроек
void UpdateComspec(ConEmuComspec* pOpt, bool DontModifyPath = false);
void SetEnvVarExpanded(LPCWSTR asName, LPCWSTR asValue);
wchar_t* GetEnvVar(LPCWSTR VarName, DWORD cchDefaultMax = 2000);
#endif
LPCWSTR GetComspecFromEnvVar(wchar_t* pszComspec, DWORD cchMax, ComSpecBits Bits = csb_SameOS);
wchar_t* GetComspec(const ConEmuComspec* pOpt);

bool IsExportEnvVarAllowed(LPCWSTR szName);
void ApplyExportEnvVar(LPCWSTR asEnvNameVal);

#ifndef CONEMU_MINIMAL
bool CopyToClipboard(LPCWSTR asText);
#endif

//------------------------------------------------------------------------
///| Section |////////////////////////////////////////////////////////////
//------------------------------------------------------------------------
class MSectionLock;

class MSection
{
	protected:
		CRITICAL_SECTION m_cs;
		CRITICAL_SECTION m_lock_cs;
		DWORD mn_TID; // устанавливаетс€ только после EnterCriticalSection
		HANDLE mh_ExclusiveThread;
#ifdef _DEBUG
		DWORD mn_UnlockedExclusiveTID;
#endif
		int mn_Locked;
		BOOL mb_Exclusive;
		HANDLE mh_ReleaseEvent;
		friend class MSectionLock;
		DWORD mn_LockedTID[10];
		int   mn_LockedCount[10];
		void Process_Lock();
		void Process_Unlock();
	public:
		MSection();
		~MSection();
	public:
		void ThreadTerminated(DWORD dwTID);
		bool isLockedExclusive();
	protected:
		void AddRef(DWORD dwTID);
		int ReleaseRef(DWORD dwTID);
		void WaitUnlocked(DWORD dwTID, DWORD anTimeout);
		bool MyEnterCriticalSection(DWORD anTimeout);
		BOOL Lock(BOOL abExclusive, DWORD anTimeout=-1/*, BOOL abRelockExclusive=FALSE*/);
		void Unlock(BOOL abExclusive);
};

#ifndef CONEMU_MINIMAL
class MSectionThread
{
	protected:
		MSection* mp_S;
		DWORD mn_TID;
	public:
		MSectionThread(MSection* apS);
		~MSectionThread();
};
#endif

class MSectionLock
{
	protected:
		MSection* mp_S;
		BOOL mb_Locked, mb_Exclusive;
	public:
		BOOL Lock(MSection* apS, BOOL abExclusive=FALSE, DWORD anTimeout=-1);
		BOOL RelockExclusive(DWORD anTimeout=-1);
		void Unlock();
		BOOL isLocked(BOOL abExclusiveOnly=FALSE);
	public:
		MSectionLock();
		~MSectionLock();
};


#ifndef CONEMU_MINIMAL
class MSectionLockSimple
{
	protected:
		CRITICAL_SECTION* mp_S;
		#ifdef _DEBUG
		DWORD mn_LockTID, mn_LockTick;
		#endif
		bool mb_Locked;
	public:
		BOOL Lock(CRITICAL_SECTION* apS, DWORD anTimeout=-1);
		void Unlock();
		BOOL isLocked();
	public:
		MSectionLockSimple();
		~MSectionLockSimple();
};
#endif


#ifndef CONEMU_MINIMAL
/* Console Handles */
class MConHandle
{
	private:
		wchar_t   ms_Name[10];
		HANDLE    mh_Handle;
		MSection  mcs_Handle;
		BOOL      mb_OpenFailed;
		DWORD     mn_LastError;
		DWORD     mn_StdMode;
		HANDLE*   mpp_OutBuffer; // ”станавливаетс€ при SetConsoleActiveScreenBuffer

		static const int HANDLE_BUFFER_SIZE = RELEASEDEBUGTEST(0x100,0x1000);   // Must be a power of 2
		struct Event {
			DWORD TID;
			HANDLE h;
			enum EventType {
				e_Empty,
				e_GetHandle,
				e_CloseHandle,
				e_CloseHandleStd,
				e_CreateHandle,
				e_CreateHandleStd,
				e_GetHandlePtr,
			} evt;
			DEBUGTEST(DWORD time;)
		};
		Event m_log[HANDLE_BUFFER_SIZE];
		LONG m_logidx;
		void LogHandle(UINT evt, HANDLE h);

	public:
		operator const HANDLE();

	public:
		void Close();
		void SetBufferPtr(HANDLE* ppOutBuffer);

	public:
		MConHandle(LPCWSTR asName);
		~MConHandle();
};
#endif


template <class T>
class MFileMapping
{
	protected:
		HANDLE mh_Mapping;
		T* mp_Data; //WARNING!!! ƒоступ может быть только на чтение!
		BOOL mb_WriteAllowed;
		DWORD mn_Size;
		wchar_t ms_MapName[128];
		DWORD mn_LastError;
		wchar_t ms_Error[MAX_PATH*2];
	public:
		T* Ptr()
		{
			return mp_Data;
		};
		operator T*()
		{
			return mp_Data;
		};
		bool IsValid()
		{
			return (mp_Data!=NULL);
		};
		LPCWSTR GetErrorText()
		{
			return ms_Error;
		};
		#ifndef CONEMU_MINIMAL
		bool SetFrom(const T* pSrc, int nSize=-1)
		{
			if (!IsValid() || !nSize)
				return false;

			if (nSize == -1)
				nSize = sizeof(T);
			if (nSize < 0)
				return false;

			bool lbChanged = (memcmp(mp_Data, pSrc, nSize)!=0); //-V106
			memmove(mp_Data, pSrc, nSize); //-V106
			return lbChanged;
		}
		bool GetTo(T* pDst, int nSize=-1)
		{
			if (!IsValid() || !nSize) return false;

			if (nSize<0) nSize = sizeof(T);

			//потому, что T может быть описан как const - (void*)
			memmove((void*)pDst, mp_Data, nSize); //-V106
			return true;
		}
		#endif
	public:
		LPCWSTR InitName(const wchar_t *aszTemplate,DWORD Parm1=0,DWORD Parm2=0)
		{
			if (mh_Mapping) CloseMap();

			//#ifdef CONEMU_MINIMAL
			//			_ASSERTE(Parm2==0);
			msprintf(ms_MapName, countof(ms_MapName), aszTemplate, Parm1, Parm2);
			//#else
			//			msprintf(ms_MapName, SKIPLEN(countof(ms_MapName)) aszTemplate, Parm1, Parm2);
			//#endif
			return ms_MapName;
		};
		void ClosePtr()
		{
			if (mp_Data)
			{
				UnmapViewOfFile((void*)mp_Data);
				mp_Data = NULL;
			}
		};
		void CloseMap()
		{
			if (mp_Data) ClosePtr();

			if (mh_Mapping)
			{
				CloseHandle(mh_Mapping);
				mh_Mapping = NULL;
			}

			mh_Mapping = NULL; mb_WriteAllowed = FALSE; mp_Data = NULL;
			mn_Size = -1; mn_LastError = 0;
		};
	protected:
		// mn_Size и nSize используетс€ фактически только при CreateFileMapping или операци€х копировани€
		T* InternalOpenCreate(BOOL abCreate,BOOL abReadWrite,int nSize)
		{
			if (mh_Mapping) CloseMap();

			mn_LastError = 0;
			ms_Error[0] = 0;
			_ASSERTE(mh_Mapping==NULL && mp_Data==NULL);
			_ASSERTE(nSize==-1 || nSize>=sizeof(T));

			if (ms_MapName[0] == 0)
			{
				_ASSERTE(ms_MapName[0]!=0);
				wcscpy_c(ms_Error, L"Internal error. Mapping file name was not specified.");
				return NULL;
			}
			else
			{
				mn_Size = (nSize<=0) ? sizeof(T) : nSize; //-V105 //-V103
				mb_WriteAllowed = abCreate || abReadWrite;

				if (abCreate)
				{
					mh_Mapping = CreateFileMapping(INVALID_HANDLE_VALUE,
					                               LocalSecurity(), PAGE_READWRITE, 0, mn_Size, ms_MapName);
				}
				else
				{
					mh_Mapping = OpenFileMapping(FILE_MAP_READ, FALSE, ms_MapName);
				}

				if (!mh_Mapping)
				{
					mn_LastError = GetLastError();
					msprintf(ms_Error, countof(ms_Error), L"Can't %s console data file mapping. ErrCode=0x%08X\n%s",
					          abCreate ? L"create" : L"open", mn_LastError, ms_MapName);
				}
				else
				{
					DWORD nFlags = mb_WriteAllowed ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
					mp_Data = (T*)MapViewOfFile(mh_Mapping, nFlags,0,0,0);

					if (!mp_Data)
					{
						mn_LastError = GetLastError();
						msprintf(ms_Error, countof(ms_Error), L"Can't map console info (%s). ErrCode=0x%08X\n%s",
						          mb_WriteAllowed ? L"ReadWrite" : L"Read" ,mn_LastError, ms_MapName);
					}
				}
			}

			return mp_Data;
		};
	public:
		#ifndef CONEMU_MINIMAL
		T* Create(int nSize=-1)
		{
			_ASSERTE(nSize==-1 || nSize>=sizeof(T));
			return InternalOpenCreate(TRUE/*abCreate*/,TRUE/*abReadWrite*/,nSize);
		};
		#endif
		T* Open(BOOL abReadWrite=FALSE/*FALSE - только Read*/,int nSize=-1)
		{
			_ASSERTE(nSize==-1 || nSize>=sizeof(T));
			return InternalOpenCreate(FALSE/*abCreate*/,abReadWrite,nSize);
		};
		const T* ReopenForRead()
		{
			if (mh_Mapping)
			{
				if (mp_Data) ClosePtr();

				mb_WriteAllowed = FALSE;

				DWORD nFlags = FILE_MAP_READ;
				mp_Data = (T*)MapViewOfFile(mh_Mapping, nFlags,0,0,0);

				if (!mp_Data)
				{
					mn_LastError = GetLastError();
					msprintf(ms_Error, countof(ms_Error), L"Can't map console info (%s). ErrCode=0x%08X\n%s",
						mb_WriteAllowed ? L"ReadWrite" : L"Read" ,mn_LastError, ms_MapName);
				}
			}
			return mp_Data;
		};
	public:
		MFileMapping()
		{
			mh_Mapping = NULL; mb_WriteAllowed = FALSE; mp_Data = NULL;
			mn_Size = -1; ms_MapName[0] = 0;
			ms_Error[0] = 0;
			mn_LastError = 0;
		};
		~MFileMapping()
		{
			if (mh_Mapping) CloseMap();
		};
};

#ifndef CONEMU_MINIMAL
class MEvent
{
	protected:
		WCHAR  ms_EventName[MAX_PATH];
		HANDLE mh_Event;
		DWORD  mn_LastError;
	public:
		MEvent();
		~MEvent();
		void InitName(const wchar_t *aszTemplate, DWORD Parm1=0);
	public:
		void   Close();
		HANDLE Open();
		DWORD  Wait(DWORD dwMilliseconds, BOOL abAutoOpen=TRUE);
		HANDLE GetHandle();
};
#endif


#ifndef CONEMU_MINIMAL
class MSetter
{
	protected:
		enum tag_MSETTERTYPE
		{
			st_BOOL,
			st_bool,
			st_DWORD,
		} type;

		union
		{
			struct
			{
				// st_BOOL
				BOOL *mp_BOOLVal;
			};
			struct
			{
				// st_bool
				bool *mp_boolVal;
			};
			struct
			{
				// st_DWORD
				DWORD *mdw_DwordVal; DWORD mdw_OldDwordValue, mdw_NewDwordValue;
			};
			DWORD DataPtr[4];
		};
	public:
		MSetter(BOOL* st);
		MSetter(bool* st);
		MSetter(DWORD* st, DWORD setValue);
		~MSetter();

		void Unlock();
};
#endif

#ifndef CONEMU_MINIMAL
struct CEStartupEnv;

class MFileLog
{
	protected:
		wchar_t* ms_FilePathName;
		wchar_t* ms_FileName;
		wchar_t* ms_DefPath;
		HANDLE   mh_LogFile;
		HRESULT  InitFileName(LPCWSTR asName = NULL, DWORD anPID = 0);
	protected:
		CRITICAL_SECTION mcs_Lock;
		static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
	public:
		MFileLog(LPCWSTR asName, LPCWSTR asDir = NULL, DWORD anPID = 0);
		~MFileLog();
		void CloseLogFile();
		HRESULT CreateLogFile(LPCWSTR asName = NULL, DWORD anPID = 0, DWORD anLevel = 0); // Returns 0 if succeeded, otherwise - GetLastError() code
		LPCWSTR GetLogFileName();
		void LogString(LPCSTR asText, bool abWriteTime = true, LPCSTR asThreadName = NULL, bool abNewLine = true, UINT anCP = CP_ACP);
		void LogString(LPCWSTR asText, bool abWriteTime = true, LPCWSTR asThreadName = NULL, bool abNewLine = true);
		void LogStartEnv(CEStartupEnv* apStartEnv);
};
#endif

#ifndef CONEMU_MINIMAL
//  ласс отключени€ редиректора системных библиотек.
class MWow64Disable
{
	protected:
		typedef BOOL (WINAPI* Wow64DisableWow64FsRedirection_t)(PVOID* OldValue);
		typedef BOOL (WINAPI* Wow64RevertWow64FsRedirection_t)(PVOID OldValue);
		Wow64DisableWow64FsRedirection_t _Wow64DisableWow64FsRedirection;
		Wow64RevertWow64FsRedirection_t _Wow64RevertWow64FsRedirection;

		BOOL mb_Disabled;
		PVOID m_OldValue;
	public:
		void Disable()
		{
			if (!mb_Disabled && _Wow64DisableWow64FsRedirection)
			{
				mb_Disabled = _Wow64DisableWow64FsRedirection(&m_OldValue);
			}
		};
		void Restore()
		{
			if (mb_Disabled)
			{
				mb_Disabled = FALSE;

				if (_Wow64RevertWow64FsRedirection)
					_Wow64RevertWow64FsRedirection(m_OldValue);
			}
		};
	public:
		MWow64Disable()
		{
			mb_Disabled = FALSE; m_OldValue = NULL;
			HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");

			if (hKernel)
			{
				_Wow64DisableWow64FsRedirection = (Wow64DisableWow64FsRedirection_t)GetProcAddress(hKernel, "Wow64DisableWow64FsRedirection");
				_Wow64RevertWow64FsRedirection = (Wow64RevertWow64FsRedirection_t)GetProcAddress(hKernel, "Wow64RevertWow64FsRedirection");
			}
			else
			{
				_Wow64DisableWow64FsRedirection = NULL;
				_Wow64RevertWow64FsRedirection = NULL;
			}
		};
		~MWow64Disable()
		{
			Restore();
		};
};
#endif

#ifndef CONEMU_MINIMAL
// ќбертка дл€ таймера
class CTimer
{
	protected:
		HWND mh_Wnd;
		UINT_PTR mn_TimerId;
		UINT mn_Elapse;
		bool mb_Started;
	public:
		bool IsStarted();
		void Start(UINT anElapse = 0);
		void Stop();
		void Restart(UINT anElapse = 0);
	public:
		CTimer();
		~CTimer();
		void Init(HWND ahWnd, UINT_PTR anTimerID, UINT anElapse);
};
#endif

#ifndef CONEMU_MINIMAL
class CToolTip
{
public:
	CToolTip();
	virtual ~CToolTip();
public:
	void ShowTip(HWND ahOwner, HWND ahControl, LPCWSTR asText, BOOL abBalloon, POINT pt, HINSTANCE hInstance = NULL/*g_hInstance*/);
	void HideTip();
	//virtual bool OnMenuSelected(HMENU hMenu, WORD nID, WORD nFlags) { return false; };
private:
	HWND mh_Tip, mh_Ball;
	wchar_t *mpsz_LastTip;
	int mn_LastTipCchMax;
	TOOLINFO mti_Tip, mti_Ball;
	int mb_LastTipBalloon;
};
#endif
