/*
Copyright (c) 2016-present Maximus5
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
#include "defines.h"
#include "MModule.h"

/* LoadLibrary/FreeLibrary/GetProcAddress encapsulation */

MModule::MModule() // NOLINT(modernize-use-equals-default)
{
}

MModule::MModule(const wchar_t* asModule)
{
	Load(asModule);
}

MModule::MModule(const HMODULE ahModule)
	: mh_Module(ahModule)
{
	_ASSERTE(mb_Loaded == false);
}

MModule::~MModule()
{
	Free();
}

HMODULE MModule::Load(const wchar_t* asModule)
{
	_ASSERTE(mh_Module == nullptr && !mb_Loaded);

	Free(); // JIC

#ifdef _DEBUG
	lstrcpyn(ms_Module, asModule ? asModule : L"", countof(ms_Module));
#endif

	mh_Module = LoadLibrary(asModule);
	mb_Loaded = (mh_Module != nullptr);
	return mh_Module;
}

void MModule::SetHandle(HMODULE hModule)
{
	_ASSERTE(mh_Module == nullptr && !mb_Loaded);

	Free(); // JIC

	mh_Module = hModule;
	mb_Loaded = false;
}

void MModule::Free()
{
	if (mb_Loaded && mh_Module)
	{
		// ReSharper disable once CppLocalVariableMayBeConst
		HMODULE hLib = mh_Module;
		mh_Module = nullptr;
		FreeLibrary(hLib);
	}

	mh_Module = nullptr;
#ifdef _DEBUG
	ms_Module[0] = 0;
#endif
	mb_Loaded = false;
}

MModule::operator HMODULE() const
{
	return mh_Module;
}

bool MModule::operator!() const
{
	return (mh_Module == nullptr);
}

bool MModule::IsLoaded() const
{
	return (mh_Module != nullptr);
}
