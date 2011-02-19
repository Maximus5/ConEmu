
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
    if( dos_header->e_magic == 'ZM' )
    {
        nt_header = (IMAGE_NT_HEADERS*)((char*)Module + dos_header->e_lfanew);
        if( nt_header->Signature != 0x004550 )
            return false;
        else
            ExportDir = (DWORD)(nt_header->OptionalHeader.
                                         DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].
                                         VirtualAddress);
    }
    if (!ExportDir)
        return false;

    IMAGE_SECTION_HEADER* section = (IMAGE_SECTION_HEADER*)IMAGE_FIRST_SECTION (nt_header);

    int s = 0;
    for(s = 0; s < nt_header->FileHeader.NumberOfSections; s++)
    {
        if((section[s].VirtualAddress == ExportDir) ||
           (section[s].VirtualAddress <= ExportDir &&
           (section[s].Misc.VirtualSize + section[s].VirtualAddress > ExportDir)))
        {
            int nDiff = 0;//section[s].VirtualAddress - section[s].PointerToRawData;
            IMAGE_EXPORT_DIRECTORY* Export = (IMAGE_EXPORT_DIRECTORY*)((char*)Module + (ExportDir-nDiff));
            DWORD* Name = (DWORD*)((char*)Module + Export->AddressOfNames);
            DWORD* Ptr  = (DWORD*)((char*)Module + Export->AddressOfFunctions);

            DWORD old_protect;
            if (!VirtualProtect( Ptr, Export->NumberOfFunctions * sizeof( DWORD ), PAGE_READWRITE, &old_protect ))
                return false;

            for (DWORD i = 0; i < Export->NumberOfNames; i++)
            {
                for (int j = 0; Funcs[j].Name; j++)
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
            VirtualProtect( Ptr, Export->NumberOfFunctions * sizeof( DWORD ), old_protect, &old_protect );
        }
    }
    return true;
}
