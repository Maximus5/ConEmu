
/*
Copyright (c) 2009-2010 Maximus5
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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

typedef struct HookItem
{
	// Calling function must set only this 3 fields
	// Theese fields must be valid for lifetime
    const void*    NewAddress;
    const char*    Name;
    const wchar_t* DllName;
    
    
    // Next fields are for internal use only!
    void*   OldAddress;    // original function from hDll
	HMODULE hDll;          // handle of DllName
    #ifdef _DEBUG
	BOOL    ReplacedInExe; // debug information only
	#endif
	BOOL    InExeOnly;     // replace only in Exe module (FAR sort functions)
	
	// Stored for internal use in GetOriginalAddress
	// Some other dll's may modify procaddress (Anamorphosis, etc.)
	void*   ExeOldAddress; // function address from executable module (may be replaced by other libraries)
} HookItem;

extern const HookItem LibraryHooks[6];

void* GetOriginalAddress( void* OurFunction, void* DefaultFunction, BOOL abAllowModified );

bool InitHooks( HookItem* apHooks, BOOL abInExeOnly = FALSE );

// All *aszExcludedModules must be valid all time
bool SetAllHooks( HMODULE ahOurDll, const wchar_t* aszExcludedModules = NULL );

void UnsetAllHooks( );
