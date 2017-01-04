
/*
Copyright (c) 2016-2017 Maximus5
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
#include <TlHelp32.h>
#include "Common.h"
#include "CmdLine.h"
#include "MArray.h"
#include "WObjects.h"

template <
	typename T,
	BOOL (WINAPI* get_first)(HANDLE h, T* lp),
	BOOL (WINAPI* get_next)(HANDLE h, T* lp)
>
class MToolHelp
{
protected:
	// Arguments for CreateToolhelp32Snapshot
	DWORD m_Flags;
	DWORD m_ByProcessId;
	// CreateToolhelp32Snapshot was called?
	bool mb_Opened;
	bool mb_Valid;
	// Enumeration was finished, all items are placed in m_Items
	bool mb_EndOfList;
	// Enumeration was started
	bool mb_Started;
	// Toolhelp handle
	HANDLE mh_Snapshot;
	// Internal variable
	T m_Item;

protected:
	MArray<T> m_Items;
	INT_PTR mn_Position;

	virtual bool Filter(T* lp)
	{
		return true;
	};

	bool getNext()
	{
		if (!mb_Opened)
		{
			if (!Open())
				return false;
		}

		if (!mb_Valid)
		{
			return false;
		}

		if (mb_EndOfList)
		{
			return false;
		}

		_ASSERTE(mh_Snapshot && (mh_Snapshot != INVALID_HANDLE_VALUE));


		for (;;)
		{
			BOOL bRc;

			if (!mb_Started)
			{
				mb_Started = true;
				ZeroStruct(m_Item);
				*((LPDWORD)&m_Item) = sizeof(T);
				bRc = get_first(mh_Snapshot, &m_Item);
			}
			else
			{
				bRc = get_next(mh_Snapshot, &m_Item);
			}

			if (!bRc)
			{
				mb_EndOfList = true;
				return false;
			}

			if (Filter(&m_Item))
			{
				break;
			}
		}

		m_Items.push_back(m_Item);
		return true;
	};

public:
	MToolHelp(DWORD dwFlags = 0, DWORD th32ProcessID = 0)
		: m_Flags(dwFlags)
		, m_ByProcessId(th32ProcessID)
		, mb_Opened(false)
		, mb_Valid(false)
		, mb_EndOfList(false)
		, mb_Started(false)
		, mh_Snapshot(NULL)
		, mn_Position(0)
	{
		ZeroStruct(m_Item);
		*((LPDWORD)&m_Item) = sizeof(T);
	};

	virtual ~MToolHelp()
	{
		Close();
	};

	void Close(bool bReset = false)
	{
		SafeCloseHandle(mh_Snapshot);

		ZeroStruct(m_Item);
		*((LPDWORD)&m_Item) = sizeof(T);

		mb_Opened = false;
		mb_Valid = false;
		mb_EndOfList = false;
		mb_Started = false;
		mn_Position = 0;

		m_Items.eraseall();
	};

	bool Open()
	{
		Close(true);

		mh_Snapshot = CreateToolhelp32Snapshot(m_Flags, m_ByProcessId);
		mb_Opened = true;
		mb_Valid = (mh_Snapshot && (mh_Snapshot != INVALID_HANDLE_VALUE));
		_ASSERTE(!mb_EndOfList && !mb_Started && (mn_Position == 0));

		return mb_Valid;
	};

	bool Open(DWORD dwFlags, DWORD th32ProcessID)
	{
		m_Flags = dwFlags;
		m_ByProcessId = th32ProcessID;
		return Open();
	};

	bool LoadAll()
	{
		while (getNext())
			;
		return (m_Items.size() > 0);
	};

	void Reset()
	{
		mn_Position = 0;
	};

	bool Next(T* lp)
	{
		if (mn_Position >= m_Items.size())
		{
			if (!getNext())
			{
				return false;
			}
			_ASSERTE(mn_Position < m_Items.size());
		}

		memmove(lp, &(m_Items[mn_Position]), sizeof(T));
		mn_Position++;
		return true;
	};

	bool Find(bool (*compare)(T* p, LPARAM lParam), LPARAM lParam, T* lp)
	{
		T item = {sizeof(T)};

		Reset();

		while (Next(&item))
		{
			if (compare(&item, lParam))
			{
				memmove(lp, &item, sizeof(T));
				return true;
			}
		}
		return false;
	};
};

class MToolHelpProcess : public MToolHelp<PROCESSENTRY32, Process32First, Process32Next>
{
public:
	MToolHelpProcess()
		: MToolHelp(TH32CS_SNAPPROCESS, 0)
	{
	};

	// Inherited
	// bool Next(PROCESSENTRY32* lp);

	bool Find(DWORD nPID, PROCESSENTRY32* Info)
	{
		struct cmp
		{
			static bool compare(PROCESSENTRY32* p, LPARAM lParam)
			{
				return (p->th32ProcessID == LODWORD(lParam));
			};
		};
		return MToolHelp::Find(cmp::compare, (LPARAM)(DWORD_PTR)nPID, Info);
	};

	bool Find(LPCWSTR asExeName, PROCESSENTRY32* Info)
	{
		struct cmp
		{
			static bool compare(PROCESSENTRY32* p, LPARAM lParam)
			{
				LPCWSTR pszName1 = PointToName(p->szExeFile);
				LPCWSTR pszName2 = (LPCWSTR)lParam;
				int iCmp = lstrcmpi(pszName1, pszName2);
				return (iCmp == 0);
			};
		};
		LPCWSTR pszName = PointToName(asExeName);
		if (!pszName || !*pszName)
			return false;
		return MToolHelp::Find(cmp::compare, (LPARAM)pszName, Info);
	};

	// Visual Studio Code spawns a lot of children processes
	// So we have a long tree of Code.exe/Code.exe/Code.exe
	// The function will find all first-level forks of current process
	bool FindForks(MArray<PROCESSENTRY32>& Forks)
	{
		CEStr lsSelfExe;
		struct cmp
		{
			LPCWSTR pszName;
			MArray<PROCESSENTRY32>* Forks;
			DWORD nParentPID;
			cmp() {};
			~cmp() {};
			static bool check(PROCESSENTRY32* p, LPARAM lParam)
			{
				cmp* pCmp = (cmp*)lParam;
				// Process only first-level children
				if (p->th32ParentProcessID != pCmp->nParentPID)
					return false; // continue
				// Compare names
				LPCWSTR pszName1 = PointToName(p->szExeFile);
				LPCWSTR pszName2 = pCmp->pszName;
				int iCmp = lstrcmpi(pszName1, pszName2);
				if (iCmp == 0)
					pCmp->Forks->push_back(*p);
				return false; // Process all processes in system...
			};
		} Cmp;
		if (!GetModulePathName(NULL, lsSelfExe))
			return false;
		Cmp.pszName = PointToName(lsSelfExe);
		Cmp.nParentPID = GetCurrentProcessId();
		Cmp.Forks = &Forks;
		PROCESSENTRY32 dummy = {};
		MToolHelp::Find(cmp::check, (LPARAM)&Cmp, &dummy);
		return (Forks.size() > 0);
	};
};

class MToolHelpModule : public MToolHelp<MODULEENTRY32, Module32First, Module32Next>
{
public:
	MToolHelpModule(DWORD th32ProcessID, bool SnapModule32 = false)
		: MToolHelp(TH32CS_SNAPMODULE|(SnapModule32 ? TH32CS_SNAPMODULE32 : 0), th32ProcessID)
	{
	};

	// Inherited
	// bool Next(MODULEENTRY32* lp);
};

class MToolHelpThread : public MToolHelp<THREADENTRY32, Thread32First, Thread32Next>
{
public:
	// Unfortunately, Thread32First/Next enumarate all thread in the system
	// We shall filter found threads by THREADENTRY32::th32OwnerProcessID
	MToolHelpThread(DWORD th32ProcessID)
		: MToolHelp(TH32CS_SNAPTHREAD, th32ProcessID)
	{
	};

	// Inherited
	// bool Next(THREADENTRY32* lp);

protected:
	virtual bool Filter(THREADENTRY32* lp)
	{
		return (lp->th32OwnerProcessID == m_ByProcessId);
	};
};
