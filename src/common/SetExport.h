
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

// This portion of code inherited from ConMan

struct ExportFunc
{
	const char* Name;
	void*       OldAddress;
	void*       NewAddress;
};

static bool ChangeExports( const ExportFunc* Funcs, HMODULE Module )
{
    if( !Module )
        return false;
    
    //DebugString(_T("SetExports Module:"));
    //TCHAR _tmp[500];
    //lltoa((long)Module, _tmp);
    //DebugString(_tmp);

    DWORD ExportDir = 0;
    IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)Module;
    IMAGE_NT_HEADERS* nt_header = 0;
    if ( dos_header->e_magic == IMAGE_DOS_SIGNATURE/*'ZM'*/ )
    {
        nt_header = (IMAGE_NT_HEADERS*)((char*)Module + dos_header->e_lfanew); //-V104
        if ( nt_header->Signature != 0x004550 )
            return false;
        else
            ExportDir = (DWORD)(nt_header->OptionalHeader.
                                         DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].
                                         VirtualAddress);
    }
    #ifdef _DEBUG
    else
    {
    	_ASSERTE(dos_header->e_magic == IMAGE_DOS_SIGNATURE);
    }
    #endif
    if (!ExportDir)
        return false;

    IMAGE_SECTION_HEADER* section = (IMAGE_SECTION_HEADER*)IMAGE_FIRST_SECTION (nt_header); //-V220 //-V104

    int s = 0;
    for (s = 0; s < nt_header->FileHeader.NumberOfSections; s++)
    {
        if((section[s].VirtualAddress == ExportDir) ||
           (section[s].VirtualAddress <= ExportDir &&
           (section[s].Misc.VirtualSize + section[s].VirtualAddress > ExportDir)))
        {
            int nDiff = 0;//section[s].VirtualAddress - section[s].PointerToRawData;
            IMAGE_EXPORT_DIRECTORY* Export = (IMAGE_EXPORT_DIRECTORY*)((char*)Module + (ExportDir-nDiff)); //-V104
            #ifdef _DEBUG
            DWORD* Name = (DWORD*)((char*)Module + Export->AddressOfNames); //-V104
            #endif
            DWORD* Ptr  = (DWORD*)((char*)Module + Export->AddressOfFunctions); //-V104

            DWORD old_protect;
            if (!VirtualProtect( Ptr, Export->NumberOfFunctions * sizeof( DWORD ), PAGE_READWRITE, &old_protect )) //-V104
                return false;

            for (size_t i = 0; i < Export->NumberOfNames; i++) //-V104
            {
                for (size_t j = 0; Funcs[j].Name; j++)
				{
					DWORD PtrFunc = (DWORD)((DWORD_PTR)(Funcs[j].OldAddress) - (DWORD_PTR)Module);
					if (Funcs[j].NewAddress
						//&& !lstrcmpiA(Funcs[j].Name, (char*)Module + (DWORD)Name[i])
						&& PtrFunc == Ptr[i]
						)
                    {
                        Ptr[i] = (DWORD)((DWORD_PTR)(Funcs[j].NewAddress) - (DWORD_PTR)Module);
                        //DebugString( _T("Hooked export:") );
                        //DebugString( ToTchar( (char*)Module + ((DWORD)Name[i]) ) );
                        break;
                    }
				}
            }

            //DebugString(_T("done..."));
            VirtualProtect( Ptr, Export->NumberOfFunctions * sizeof( DWORD ), old_protect, &old_protect ); //-V104
        }
    }
    return true;
}
