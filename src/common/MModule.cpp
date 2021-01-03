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

#include <tuple>

/* LoadLibrary/FreeLibrary/GetProcAddress encapsulation */

MModule::MModule() // NOLINT(modernize-use-equals-default)
{
}

MModule::MModule(const wchar_t* asModule)
{
	Load(asModule);
}

MModule::MModule(const HMODULE ahModule)
	: moduleHandle_(ahModule)
{
	_ASSERTE(selfLoaded_ == false);
}

MModule::MModule(MModule&& src) noexcept
{
	moduleHandle_ = src.moduleHandle_;
	#ifdef _DEBUG
	wcscpy_s(moduleName_, src.moduleName_);
	#endif
	selfLoaded_ = src.selfLoaded_;
	src.selfLoaded_ = false;
}

MModule& MModule::operator=(MModule&& src) noexcept
{
	if (moduleHandle_ == src.moduleHandle_)
	{
		_ASSERTE((moduleHandle_ == nullptr) && "Already has this HMODULE");
		return *this;
	}
	
	Free();

	moduleHandle_ = src.moduleHandle_;
	#ifdef _DEBUG
	wcscpy_s(moduleName_, src.moduleName_);
	#endif
	selfLoaded_ = src.selfLoaded_;
	src.selfLoaded_ = false;
	return *this;
}

MModule::~MModule()
{
	Free();
}

HMODULE MModule::Load(const wchar_t* asModule)
{
	_ASSERTE(moduleHandle_ == nullptr && !selfLoaded_);

	Free(); // JIC

	#ifdef _DEBUG
	std::ignore = lstrcpyn(moduleName_, asModule ? asModule : L"", countof(moduleName_));
	#endif

	if (!asModule || !*asModule)
	{
		_ASSERTE(asModule && *asModule);
		return nullptr;
	}

	moduleHandle_ = LoadLibraryW(asModule);
	selfLoaded_ = (moduleHandle_ != nullptr);
	lastError_ = selfLoaded_ ? 0 : GetLastError();
	return moduleHandle_;
}

bool MModule::SetHandle(HMODULE hModule)
{
	_ASSERTE(moduleHandle_ == nullptr && !selfLoaded_);

	Free(); // JIC

	moduleHandle_ = hModule;
	selfLoaded_ = false;

	return IsValid();
}

void MModule::Free()
{
	if (selfLoaded_ && moduleHandle_)
	{
		// ReSharper disable once CppLocalVariableMayBeConst
		HMODULE hLib = moduleHandle_;
		moduleHandle_ = nullptr;
		FreeLibrary(hLib);
		lastError_ = GetLastError();
	}

	moduleHandle_ = nullptr;
#ifdef _DEBUG
	moduleName_[0] = L'\0';
#endif
	selfLoaded_ = false;
}

MModule::operator HMODULE() const
{
	return moduleHandle_;
}

bool MModule::operator!() const
{
	return !IsValid();
}

bool MModule::IsValid() const
{
	return (moduleHandle_ != nullptr && moduleHandle_ != INVALID_HANDLE_VALUE);
}
