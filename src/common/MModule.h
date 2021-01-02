
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

#pragma once

#include <windows.h>

// Wrapper for LoadLibrary and GetProcAddress.
// Note that is used for declaring global variables, so the class
// should stay exit-time-destructors compliant.
class MModule
{
protected:
	// Warning!!! DON'T declare or use here any classes (e.g. CEStr) which utilizes heap!

	// Result of GetModuleHandle or LoadLibrary
	HMODULE moduleHandle_ = nullptr;
	
	#ifdef _DEBUG
	// Module path for debugging purposes
	wchar_t moduleName_[MAX_PATH] = L"";
	#endif

	// true if mh_Module is from GetModuleHandle so we don't need to call FreeLibrary
	bool    selfLoaded_ = false;
	
public:
	MModule();
	explicit MModule(const wchar_t* asModule);
	explicit MModule(HMODULE ahModule);

	MModule(const MModule&) = delete;
	MModule(MModule&&) noexcept;
	MModule& operator=(const MModule&) = delete;
	MModule& operator=(MModule&&) noexcept;
	
	~MModule();
public:
	void Free();
	HMODULE Load(const wchar_t* asModule);
	bool SetHandle(HMODULE hModule);
public:
	template <typename FunctionType>
	bool GetProcAddress(const char * const asFunction, FunctionType*& pfn) const
	{
		pfn = moduleHandle_ ? reinterpret_cast<FunctionType*>(::GetProcAddress(moduleHandle_, asFunction)) : nullptr;
		return (pfn != nullptr);
	};
public:
	explicit operator HMODULE() const;
	bool operator!() const;
	// Checks if module handle is not null
	bool IsValid() const;
};
