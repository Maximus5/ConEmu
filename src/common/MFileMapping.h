
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

#include <windows.h>

template <class T>
class MFileMapping
{
protected:
	HANDLE mh_Mapping;
	T* mp_Data; //WARNING!!! Доступ может быть только на чтение!
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
	// mn_Size и nSize используется фактически только при CreateFileMapping или операциях копирования
	T* InternalOpenCreate(BOOL abCreate,BOOL abReadWrite,int nSize)
	{
		if (mh_Mapping) CloseMap();

		mn_LastError = 0;
		ms_Error[0] = 0;
		_ASSERTE(mh_Mapping==NULL && mp_Data==NULL);
		_ASSERTE(nSize==-1 || nSize>=(INT_PTR)sizeof(T));

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

			DWORD nFlags = mb_WriteAllowed ? (FILE_MAP_READ|FILE_MAP_WRITE) : FILE_MAP_READ;

			if (abCreate)
			{
				mh_Mapping = CreateFileMapping(INVALID_HANDLE_VALUE,
				                               LocalSecurity(), PAGE_READWRITE, 0, mn_Size, ms_MapName);
			}
			else
			{
				mh_Mapping = OpenFileMapping(nFlags, FALSE, ms_MapName);
			}

			if (!mh_Mapping)
			{
				mn_LastError = GetLastError();
				msprintf(ms_Error, countof(ms_Error), L"Can't %s console data file mapping. ErrCode=0x%08X\n%s",
				          abCreate ? L"create" : L"open", mn_LastError, ms_MapName);
			}
			else
			{
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
		_ASSERTE(nSize==-1 || nSize>=(INT_PTR)sizeof(T));
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
