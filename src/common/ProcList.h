
/*
Copyright (c) 2014-2017 Maximus5
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

#include "MArray.h"
#include "WObjects.h"

struct CEStr;

struct ProcessInfoBase
{
	DWORD ProcessID;
};

struct ProcessInfoLite : public ProcessInfoBase
{
};

struct ProcessInfoLite : public ProcessInfoBase
{
	DWORD   ParentPID;
	bool    IsMainSrv; // Root ConEmuC
	bool    IsConHost; // conhost.exe (Win7 и выше)
	bool    IsFar, IsFarPlugin;
	bool    IsTelnet;  // может быть включен ВМЕСТЕ с IsFar, если удалось подцепится к фару через сетевой пайп
	bool    IsNtvdm;   // 16bit приложения
	bool    IsCmd;     // значит фар выполняет команду
	bool    NameChecked, RetryName;
	bool    Alive, inConsole;
	wchar_t Name[64]; // чтобы полная инфа об ошибке влезала
	CEStr* pActiveDir;
	CEStr* pPassiveDir;
};

template<class T>
class MProcList
{
public:
	MArray<T> m_Processes;

public:
	MProcList()
	{
	};

	~MProcList()
	{
	};
protected:
	static MSection* mp_Section;
public:
	static MSection* Section();
};
