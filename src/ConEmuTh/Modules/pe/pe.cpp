
#include <windows.h>
#include <crtdbg.h>
#include "../ThumbSDK.h"


typedef __int32 i32;
typedef __int64 i64;
typedef unsigned __int8 u8;
typedef unsigned __int16 u16;
typedef DWORD u32;

HMODULE ghModule;


#define PGE_INVALID_FRAME        0x1001
#define PGE_ERROR_BASE           0x80000000
#define PGE_DLL_NOT_FOUND        0x80001001
#define PGE_FILE_NOT_FOUND       0x80001003
#define PGE_NOT_ENOUGH_MEMORY    0x80001004
#define PGE_INVALID_CONTEXT      0x80001005
#define PGE_FUNCTION_NOT_FOUND   0x80001006
#define PGE_WIN32_ERROR          0x80001007
#define PGE_UNKNOWN_COLORSPACE   0x80001008
#define PGE_UNSUPPORTED_PITCH    0x80001009
#define PGE_INVALID_PAGEDATA     0x8000100A
#define PGE_OLD_PICVIEW          0x8000100B
#define PGE_BITBLT_FAILED        0x8000100C
#define PGE_INVALID_VERSION      0x8000100D
#define PGE_INVALID_IMGSIZE      0x8000100E
#define PGE_UNSUPPORTEDFORMAT    0x8000100F




#define CALLOC(n) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n)
#define FREE(p) HeapFree(GetProcessHeap(), 0, p)
#define STRING2(x) #x
#define STRING(x) STRING2(x)
#define FILE_LINE __FILE__ "(" STRING(__LINE__) "): "
#ifdef HIDE_TODO
#define TODO(s) 
#define WARNING(s) 
#else
#define TODO(s) __pragma(message (FILE_LINE "TODO: " s))
#define WARNING(s) __pragma(message (FILE_LINE "warning: " s))
#endif
#define PRAGMA_ERROR(s) __pragma(message (FILE_LINE "error: " s))

enum tag_PeStrMagics {
	eGdiStr_Data = 0x1005,
};


struct PEData
{
	DWORD nMagic;
	wchar_t szInfo[255];
	wchar_t szVersion[128];
	wchar_t szProduct[128];
	wchar_t szCompany[128]; // или company, или copyright?
	
	GDIPlusData() {
		nMagic = eGdiStr_Data;
	};
	
	void Close() {
		FREE(this);
	};
};

PEData *gpCurData = NULL;

PBYTE g_pMappedFileBase = 0;
ULARGE_INTEGER g_FileSize = {{0,0}};
PIMAGE_NT_HEADERS32 gpNTHeader32 = NULL;
PIMAGE_NT_HEADERS64 gpNTHeader64 = NULL;
bool g_bIs64Bit = false;
//PDWORD g_pCVHeader = 0;
//PIMAGE_COFF_SYMBOLS_HEADER g_pCOFFHeader = 0;
//COFFSymbolTable * g_pCOFFSymbolTable = 0;


bool DumpExeFilePE( PIMAGE_DOS_HEADER dosHeader, PIMAGE_NT_HEADERS32 pNTHeader );
bool DumpExeFileVX( PIMAGE_DOS_HEADER dosHeader, PIMAGE_VXD_HEADER pVXDHeader );


// MakePtr is a macro that allows you to easily add to values (including
// pointers) together without dealing with C's pointer arithmetic.  It
// essentially treats the last two parameters as DWORDs.  The first
// parameter is used to typecast the result to the appropriate pointer type.
#define MakePtr( cast, ptr, addValue ) (cast)( (DWORD_PTR)(ptr) + (DWORD_PTR)(addValue))

#define GetImgDirEntryRVA( pNTHdr, IDE ) \
	(pNTHdr->OptionalHeader.DataDirectory[IDE].VirtualAddress)

#define GetImgDirEntrySize( pNTHdr, IDE ) \
	(pNTHdr->OptionalHeader.DataDirectory[IDE].Size)

#define ARRAY_SIZE( x ) (sizeof(x) / sizeof(x[0]))

#define IMAGE_SIZEOF_ROM_OPTIONAL_HEADER 56
#define IMAGE_SIZEOF_NT_OPTIONAL32_HEADER    224
#define IMAGE_SIZEOF_NT_OPTIONAL64_HEADER    240

//
// Like GetPtrFromRVA, but works with addresses that already include the
// default image base
//
template <class T> LPVOID GetPtrFromVA( PVOID ptr, T* pNTHeader, PBYTE pImageBase ) // 'T' = PIMAGE_NT_HEADERS 
{
	// Yes, under Win64, we really are lopping off the high 32 bits of a 64 bit
	// value.  We'll knowingly believe that the two pointers are within the
	// same load module, and as such, are RVAs
	DWORD rva = PtrToLong( (PBYTE)ptr - pNTHeader->OptionalHeader.ImageBase );
	
	return GetPtrFromRVA( rva, pNTHeader, pImageBase );
}

//
// Given a section name, look it up in the section table and return a
// pointer to its IMAGE_SECTION_HEADER
//
template <class T> PIMAGE_SECTION_HEADER GetSectionHeader(PSTR name, T* pNTHeader)	// 'T' == PIMAGE_NT_HEADERS
{
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pNTHeader);
    unsigned i;
    
    for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++, section++ )
    {
        if ( 0 == strncmp((char *)section->Name,name,IMAGE_SIZEOF_SHORT_NAME) )
            return section;
    }
    
    return 0;
}


static const TCHAR cszLanguage[] = _T("Lang");
static const TCHAR cszString[] = _T("String value");
static const TCHAR cszType[] = _T("Type");
static const TCHAR cszInfo[] = _T("Information");

typedef const BYTE *LPCBYTE;

// The predefined resource types
char *SzResourceTypes[] = {
"???_0",
"CURSOR",
"BITMAP",
"ICON",
"MENU",
"DIALOG",
"STRING",
"FONTDIR",
"FONT",
"ACCELERATORS",
"RCDATA",
"MESSAGETABLE",
"GROUP_CURSOR",
"???_13",
"GROUP_ICON",
"???_15",
"VERSION",
"DLGINCLUDE",
"???_18",
"PLUGPLAY",
"VXD",
"ANICURSOR",
"ANIICON",
"HTML",
"MANIFEST"
};

DWORD_FLAG_DESCRIPTIONSW VersionInfoFlags[] = {
	{VS_FF_DEBUG, L"VS_FF_DEBUG"},
	{VS_FF_INFOINFERRED, L"VS_FF_INFOINFERRED"},
	{VS_FF_PATCHED, L"VS_FF_PATCHED"},
	{VS_FF_PRERELEASE, L"VS_FF_PRERELEASE"},
	{VS_FF_PRIVATEBUILD, L"VS_FF_PRIVATEBUILD"},
	{VS_FF_SPECIALBUILD, L"VS_FF_SPECIALBUILD"},
	{0}
};

DWORD_FLAG_DESCRIPTIONSW VersionInfoFileOS[] = {
	{VOS_DOS, L"VOS_DOS"},
	{VOS_NT, L"VOS_NT"},
	{VOS__WINDOWS16, L"VOS__WINDOWS16"},
	{VOS__WINDOWS32, L"VOS__WINDOWS32"},
	{VOS_OS216, L"VOS_OS216"},
	{VOS_OS232, L"VOS_OS232"},
	{VOS__PM16, L"VOS__PM16"},
	{VOS__PM32, L"VOS__PM32"},
	{VOS_WINCE, L"VOS_WINCE"},
	{0}
};

DWORD_FLAG_DESCRIPTIONSW VersionInfoDrvSubtype[] = {
	{VFT2_DRV_PRINTER, L"VFT2_DRV_PRINTER"},
	{VFT2_DRV_KEYBOARD, L"VFT2_DRV_KEYBOARD"},
	{VFT2_DRV_LANGUAGE, L"VFT2_DRV_LANGUAGE"},
	{VFT2_DRV_DISPLAY, L"VFT2_DRV_DISPLAY"},
	{VFT2_DRV_NETWORK, L"VFT2_DRV_NETWORK"},
	{VFT2_DRV_SYSTEM, L"VFT2_DRV_SYSTEM"},
	{VFT2_DRV_INSTALLABLE, L"VFT2_DRV_INSTALLABLE"},
	{VFT2_DRV_SOUND, L"VFT2_DRV_SOUND"},
	{VFT2_DRV_COMM, L"VFT2_DRV_COMM"},
	{VFT2_DRV_INPUTMETHOD, L"VFT2_DRV_INPUTMETHOD"},
	{VFT2_DRV_VERSIONED_PRINTER, L"VFT2_DRV_VERSIONED_PRINTER"},
	{0}
};

DWORD_FLAG_DESCRIPTIONSW VersionInfoFontSubtype[] = {
	{VFT2_FONT_RASTER, L"VFT2_FONT_RASTER"},
	{VFT2_FONT_VECTOR, L"VFT2_FONT_VECTOR"},
	{VFT2_FONT_TRUETYPE, L"VFT2_FONT_TRUETYPE"},
	{0}
};

struct String { 
	WORD   wLength; 
	WORD   wValueLength; 
	WORD   wType; 
	WCHAR  szKey[1]; 
	WORD   Padding[ANYSIZE_ARRAY]; 
	WORD   Value[1]; 
};
struct StringA { 
	WORD   wLength; 
	WORD   wValueLength; 
	char   szKey[1]; 
	char   Padding[ANYSIZE_ARRAY]; 
	char   Value[1]; 
};

struct StringTable { 
	WORD   wLength; 
	WORD   wValueLength; 
	WORD   wType; 
	WCHAR  szKey[9];
	WORD   Padding[ANYSIZE_ARRAY];
	String Children[ANYSIZE_ARRAY];
};
struct StringTableA { 
	WORD    wLength; 
	WORD    wType; 
	char    szKey[9];
	char    Padding[ANYSIZE_ARRAY];
	StringA Children[ANYSIZE_ARRAY];
};

struct StringFileInfo { 
	WORD        wLength; 
	WORD        wValueLength; 
	WORD        wType; 
	WCHAR       szKey[ANYSIZE_ARRAY]; 
	WORD        Padding[ANYSIZE_ARRAY]; 
	StringTable Children[ANYSIZE_ARRAY]; 
};
struct StringFileInfoA { 
	WORD         wLength; 
	WORD         wType; 
	char         szKey[ANYSIZE_ARRAY]; 
	char         Padding[ANYSIZE_ARRAY]; 
	StringTableA Children[ANYSIZE_ARRAY]; 
};

struct Var { 
	WORD  wLength; 
	WORD  wValueLength; 
	WORD  wType; 
	WCHAR szKey[ANYSIZE_ARRAY]; 
	WORD  Padding[ANYSIZE_ARRAY]; 
	DWORD Value[ANYSIZE_ARRAY]; 
};

struct VarFileInfo { 
	WORD  wLength; 
	WORD  wValueLength; 
	WORD  wType; 
	WCHAR szKey[ANYSIZE_ARRAY]; 
	WORD  Padding[ANYSIZE_ARRAY]; 
	Var   Children[ANYSIZE_ARRAY]; 
};

//PIMAGE_RESOURCE_DIRECTORY_ENTRY g_pStrResEntries = 0;
//PIMAGE_RESOURCE_DIRECTORY_ENTRY g_pDlgResEntries = 0;
//DWORD g_cStrResEntries = 0;
//DWORD g_cDlgResEntries = 0;


// Get an ASCII string representing a resource type
void GetOS2ResourceTypeName(DWORD type, PSTR buffer, UINT cBytes)
{
	if ((type & 0x8000) == 0x8000) {
		GetResourceTypeName(type & 0x7FFF, buffer, cBytes-10);
		strcat(buffer, " [16bit]");
	} else {
		GetResourceTypeName(type, buffer, cBytes);
	}
}

typedef struct tag_OS2RC_TNAMEINFO {
	USHORT rnOffset;
	USHORT rnLength;
	UINT   rnID;
	USHORT rnHandle;
	USHORT rnUsage;
} OS2RC_TNAMEINFO, *POS2RC_TNAMEINFO;

typedef struct tag_OS2RC_TYPEINFO {
	USHORT rtTypeID;
	USHORT rtResourceCount;
	UINT   rtReserved;
	OS2RC_TNAMEINFO rtNameInfo[1];
} OS2RC_TYPEINFO, *POS2RC_TYPEINFO;

MPanelItem* CreateResource(DWORD rootType, LPVOID ptrRes, DWORD resSize,
						   LPCSTR asID, LPCSTR langID, DWORD stringIdBase, DWORD anLangId);


void DumpNEResourceTable(PIMAGE_DOS_HEADER dosHeader, LPBYTE pResourceTable)
{
	PBYTE pImageBase = (PBYTE)dosHeader;

	//MPanelItem* pChild = pRoot->AddFolder(_T("Resource Table"));
	//pChild->AddText(_T("<Resource Table>\n"));

	// минимальный размер
	size_t nReqSize = sizeof(OS2RC_TYPEINFO)+12;
	if (IsBadReadPtr(pResourceTable, nReqSize)) {
		//pChild->printf(_T("!!! Can't read memory at offset:  0x%08X\n"),
		//	(DWORD)(pResourceTable - pImageBase));
		return;
	}

	//
	USHORT rscAlignShift = *(USHORT*)pResourceTable;
	OS2RC_TYPEINFO* pTypeInfo = (OS2RC_TYPEINFO*)(pResourceTable+2);
	char szTypeName[128], szResName[256];
	UINT nResLength = 0, nResOffset = 0;
	LPBYTE pNames;

	// —начала нужно найти начало имен
	pTypeInfo = (OS2RC_TYPEINFO*)(pResourceTable+2);
	while (pTypeInfo->rtTypeID) {
		OS2RC_TNAMEINFO* pResName = pTypeInfo->rtNameInfo;

		// Next resource type
		pTypeInfo = (OS2RC_TYPEINFO*)(pResName+pTypeInfo->rtResourceCount);
		if (IsBadReadPtr(pTypeInfo, 2)) {
			pChild->printf(_T("!!! Can't read memory at offset:  0x%08X\n"),
				(DWORD)(((LPBYTE)pTypeInfo) - pImageBase));
			return;
		}
	}
	pNames = ((LPBYTE)pTypeInfo)+2;

	// “еперь, собственно ресурсы
	pTypeInfo = (OS2RC_TYPEINFO*)(pResourceTable+2);
	while (pTypeInfo->rtTypeID) {
		szTypeName[0] = 0;
		GetOS2ResourceTypeName(pTypeInfo->rtTypeID, szTypeName, sizeof(szTypeName));

		MPanelItem* pType = pChild->AddFolder(szTypeName);
		pType->printf("  <%s>:\n", szTypeName);

		pType->printf(_T("    Resource count:   %i\n"), pTypeInfo->rtResourceCount);

		OS2RC_TNAMEINFO* pResName = pTypeInfo->rtNameInfo;
		for (USHORT i = pTypeInfo->rtResourceCount; i--; pResName++) {
			nResLength = pResName->rnLength * (1 << rscAlignShift);
			nResOffset = pResName->rnOffset * (1 << rscAlignShift);

			szResName[0] = 0;
			if (pNames) {
				if (IsBadReadPtr(pNames, 1)) {
					pChild->printf(_T("!!! Can't read memory at offset:  0x%08X\n"),
						(DWORD)(pNames - pImageBase));
					pNames = NULL;
				} else if (IsBadReadPtr(pNames, 1+(*pNames))) {
					pChild->printf(_T("!!! Can't read memory at offset:  0x%08X\n"),
						(DWORD)(pNames - pImageBase));
					pNames = NULL;
				} else if (*pNames) {
					memmove(szResName, pNames+1, *pNames);
					szResName[*pNames] = 0;
					pNames += (*pNames)+1;
				} else {
					pNames++;
				}
			}
			if (szResName[0]) {
				sprintf(szResName+strlen(szResName), ".0x%08X", pResName->rnID);
			} else {
				sprintf(szResName, "ResID=0x%08X", pResName->rnID);
			}

			MPanelItem* pRes = NULL;
			if (nResLength && nResOffset) {
				pRes = CreateResource(pType, pTypeInfo->rtTypeID, 
					pImageBase+nResOffset, nResLength,
					szResName, NULL, 0, 0);
			} else {
				pRes = pType->AddFile(szResName, nResOffset ? nResLength : 0);
			}
			pType->printf("    <%s>\n", szResName);
			pType->printf("      Resource Name:    %s\n", szResName);
			pType->printf("      Resource ID:      0x%08X\n", pResName->rnID);
			pType->printf("      Resource length:  %u bytes\n", nResLength);
			pType->printf("      Resource offset:  0x%08X\n", nResOffset);
			pType->printf("      Handle(reserved): 0x%04X\n", (DWORD)pResName->rnHandle);
			pType->printf("      Usage(reserved):  0x%04X\n", (DWORD)pResName->rnUsage);
			//if (nResLength && nResOffset) {
			//	pRes->SetData(pImageBase+nResOffset, nResLength);
			//}
		}


		// Next resource type
		pTypeInfo = (OS2RC_TYPEINFO*)pResName;
	}
}

bool DumpExeFileNE( PIMAGE_DOS_HEADER dosHeader, PIMAGE_OS2_HEADER pOS2Header )
{
	PBYTE pImageBase = (PBYTE)dosHeader;

	//pRoot->Root()->AddFlags(_T("16BIT"));

	//MPanelItem* pChild = pRoot->AddFolder(_T("OS2 Header"));
	//pChild->AddText(_T("<OS2 Header>\n"));

	//MPanelItem* pDos = pRoot->AddFile(_T("DOS_Header"), sizeof(*dosHeader));
	//pDos->SetData((const BYTE*)dosHeader, sizeof(*dosHeader));

	//MPanelItem* pOS2 = pRoot->AddFile(_T("OS2_Header"), sizeof(*pOS2Header));
	//pOS2->SetData((const BYTE*)pOS2Header, sizeof(*pOS2Header));


	if (pOS2Header->ne_magic != IMAGE_OS2_SIGNATURE) {
		//pChild->AddText(_T("  IMAGE_OS2_SIGNATURE_LE signature not supported\n"));
		return false;
	}

	//pChild->AddText(_T("  Signature:         IMAGE_OS2_SIGNATURE\n"));  
	//
	//pChild->printf(_T("  Version number:                     %02u\n"), (UINT)pOS2Header->ne_ver);
	//pChild->printf(_T("  Revision number:                    %02u\n"), (UINT)pOS2Header->ne_rev);
	//pChild->printf(_T("  Offset of Entry Table:              %04u\n"), (UINT)pOS2Header->ne_enttab);
	//pChild->printf(_T("  Number of bytes in Entry Table:     %04u\n"), (UINT)pOS2Header->ne_cbenttab);
	//pChild->printf(_T("  Checksum of whole file:             %08u\n"), (UINT)pOS2Header->ne_crc);
	//pChild->printf(_T("  Flag word:                          %04u\n"), (UINT)pOS2Header->ne_flags);
	//pChild->printf(_T("  Automatic data segment number:      %04u\n"), (UINT)pOS2Header->ne_autodata);
	//pChild->printf(_T("  Initial heap allocation:            %04u\n"), (UINT)pOS2Header->ne_heap);
	//pChild->printf(_T("  Initial stack allocation:           %04u\n"), (UINT)pOS2Header->ne_stack);
	//pChild->printf(_T("  Initial CS:IP setting:              %08u\n"), (UINT)pOS2Header->ne_csip);
	//pChild->printf(_T("  Initial SS:SP setting:              %08u\n"), (UINT)pOS2Header->ne_sssp);
	//pChild->printf(_T("  Count of file segments:             %04u\n"), (UINT)pOS2Header->ne_cseg);
	//pChild->printf(_T("  Entries in Module Reference Table:  %04u\n"), (UINT)pOS2Header->ne_cmod);
	//pChild->printf(_T("  Size of non-resident name table:    %04u\n"), (UINT)pOS2Header->ne_cbnrestab);
	//pChild->printf(_T("  Offset of Segment Table:            %04u\n"), (UINT)pOS2Header->ne_segtab);
	//pChild->printf(_T("  Offset of Resource Table:           %04u\n"), (UINT)pOS2Header->ne_rsrctab);
	//pChild->printf(_T("  Offset of resident name table:      %04u\n"), (UINT)pOS2Header->ne_restab);
	//pChild->printf(_T("  Offset of Module Reference Table:   %04u\n"), (UINT)pOS2Header->ne_modtab);
	//pChild->printf(_T("  Offset of Imported Names Table:     %04u\n"), (UINT)pOS2Header->ne_imptab);
	//pChild->printf(_T("  Offset of Non-resident Names Table: %08u\n"), (UINT)pOS2Header->ne_nrestab);
	//pChild->printf(_T("  Count of movable entries:           %04u\n"), (UINT)pOS2Header->ne_cmovent);
	//pChild->printf(_T("  Segment alignment shift count:      %04u\n"), (UINT)pOS2Header->ne_align);
	//pChild->printf(_T("  Count of resource segments:         %04u\n"), (UINT)pOS2Header->ne_cres);
	//pChild->printf(_T("  Target Operating system:            %02u\n"), (UINT)pOS2Header->ne_exetyp);
	//pChild->printf(_T("  Other .EXE flags:                   %02u\n"), (UINT)pOS2Header->ne_flagsothers);
	//pChild->printf(_T("  offset to return thunks:            %04u\n"), (UINT)pOS2Header->ne_pretthunks);
	//pChild->printf(_T("  offset to segment ref. bytes:       %04u\n"), (UINT)pOS2Header->ne_psegrefbytes);
	//pChild->printf(_T("  Minimum code swap area size:        %04u\n"), (UINT)pOS2Header->ne_swaparea);
	//pChild->printf(_T("  Expected Windows version number:    %04u\n"), (UINT)pOS2Header->ne_expver);
	//
	//pChild->AddText(_T("\n"));

	if (pOS2Header->ne_rsrctab) {
		LPBYTE pResourceTable = (((LPBYTE)pOS2Header)+pOS2Header->ne_rsrctab);
		DumpNEResourceTable(dosHeader, pResourceTable);
		//MPanelItem* pChild = pRoot->AddFolder(_T("Resource Table"));
		//pChild->AddText(_T("<Resource Table>\n"));
	}

	return true;
}


DWORD GetOffsetToDataFromResEntry( 	PBYTE pResourceBase,
									PIMAGE_RESOURCE_DIRECTORY_ENTRY pResEntry )
{
	// The IMAGE_RESOURCE_DIRECTORY_ENTRY is gonna point to a single
	// IMAGE_RESOURCE_DIRECTORY, which in turn will point to the
	// IMAGE_RESOURCE_DIRECTORY_ENTRY, which in turn will point
	// to the IMAGE_RESOURCE_DATA_ENTRY that we're really after.  In
	// other words, traverse down a level.

	PIMAGE_RESOURCE_DIRECTORY pStupidResDir;
	pStupidResDir = (PIMAGE_RESOURCE_DIRECTORY)(pResourceBase + pResEntry->OffsetToDirectory);

    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry =
	    	(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pStupidResDir + 1);// PTR MATH

	PIMAGE_RESOURCE_DATA_ENTRY pResDataEntry =
			(PIMAGE_RESOURCE_DATA_ENTRY)(pResourceBase + pResDirEntry->OffsetToData);

	return pResDataEntry->OffsetToData;
}


// Get an ASCII string representing a resource type
void GetResourceTypeName(DWORD type, PSTR buffer, UINT cBytes)
{
    if ( type <= (WORD)RT_MANIFEST )
        strncpy(buffer, SzResourceTypes[type], cBytes);
    else
        sprintf(buffer, "0x%X", type);
}

//
// If a resource entry has a string name (rather than an ID), go find
// the string and convert it from unicode to ascii.
//
void GetResourceNameFromId
(
    DWORD id, PBYTE pResourceBase, PSTR buffer, UINT cBytes
)
{
    PIMAGE_RESOURCE_DIR_STRING_U prdsu;

    // If it's a regular ID, just format it.
    if ( !(id & IMAGE_RESOURCE_NAME_IS_STRING) )
    {
        sprintf(buffer, "0x%04X", id);
        return;
    }
    
    id &= 0x7FFFFFFF;
    prdsu = MakePtr(PIMAGE_RESOURCE_DIR_STRING_U, pResourceBase, id);

    // prdsu->Length is the number of unicode characters
    WideCharToMultiByte(CP_ACP, 0, prdsu->NameString, prdsu->Length,
                        buffer, cBytes, 0, 0);
    buffer[ min(cBytes-1,prdsu->Length) ] = 0;  // Null terminate it!!!
}


#pragma pack( push )
#pragma pack( 1 )

typedef struct
{
	BYTE        bWidth;          // Width, in pixels, of the image
	BYTE        bHeight;         // Height, in pixels, of the image
	BYTE        bColorCount;     // Number of colors in image (0 if >=8bpp)
	BYTE        bReserved;       // Reserved ( must be 0)
	WORD        wPlanes;         // Color Planes
	WORD        wBitCount;       // Bits per pixel
	DWORD       dwBytesInRes;    // How many bytes in this resource?
	DWORD       dwImageOffset;   // Where in the file is this image?
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct
{
	WORD           idReserved;   // Reserved (must be 0)
	WORD           idType;       // Resource Type (1 for icons, 2 for cursors)
	WORD           idCount;      // How many images?
	ICONDIRENTRY   idEntries[1]; // An entry for each image (idCount of 'em)
} ICONDIR, *LPICONDIR;

// DLL and EXE Files
// Icons can also be stored in .DLL and .EXE files. The structures used to store icon images in .EXE and .DLL files differ only slightly from those used in .ICO files.
// Analogous to the ICONDIR data in the ICO file is the RT_GROUP_ICON resource. In fact, one RT_GROUP_ICON resource is created for each ICO file bound to the EXE or DLL with the resource compiler/linker. The RT_GROUP_ICON resource is simply a GRPICONDIR structure:
// #pragmas are used here to insure that the structure's
// packing in memory matches the packing of the EXE or DLL.
typedef struct
{
	BYTE   bWidth;               // Width, in pixels, of the image
	BYTE   bHeight;              // Height, in pixels, of the image
	BYTE   bColorCount;          // Number of colors in image (0 if >=8bpp)
	BYTE   bReserved;            // Reserved
	WORD   wPlanes;              // Color Planes
	WORD   wBitCount;            // Bits per pixel
	DWORD   dwBytesInRes;         // how many bytes in this resource?
	WORD   nID;                  // the ID
} GRPICONDIRENTRY, *LPGRPICONDIRENTRY;

typedef struct 
{
	WORD            idReserved;   // Reserved (must be 0)
	WORD            idType;       // Resource type (1 for icons, 2 for cursors)
	WORD            idCount;      // How many images?
	GRPICONDIRENTRY   idEntries[1]; // The entries for each image
} GRPICONDIR, *LPGRPICONDIR;

#pragma pack( pop )

void ResourceParseFlags(DWORD dwFlags, wchar_t* pszFlags, DWORD_FLAG_DESCRIPTIONSW* pFlags)
{
	BOOL bFirst = TRUE;
	wchar_t* pszStart = pszFlags;
	wsprintfW(pszFlags, L"0x%XL", dwFlags);
	pszFlags += lstrlenW(pszFlags);
	*pszFlags = 0;

	while (pFlags->flag)
	{
		if ( (dwFlags & pFlags->flag) == pFlags->flag ) {
			if (bFirst) {
				bFirst = FALSE;
				while ((pszFlags - pszStart) < 8) {
					*(pszFlags++) = L' '; *pszFlags = 0;
				}
				wcscpy(pszFlags, L" // "); pszFlags += wcslen(pszFlags);
			} else {
				*(pszFlags++) = L'|';
			}
			wcscpy(pszFlags, pFlags->name);
			pszFlags += wcslen(pszFlags);
		}
		pFlags++;
	}
	*pszFlags = 0;
}

void ParseVersionInfoFixed(char* fileNameBuffer, MPanelItem *&pChild, LPVOID ptrRes, DWORD &resSize, LPBYTE &ptrBuf, BOOL &bIsCopy, VS_FIXEDFILEINFO* pVer, wchar_t* &psz)
{
	wchar_t szMask[255], szFlags[255], szOS[255], szFileType[64], szFileSubType[64];
	ResourceParseFlags(pVer->dwFileFlagsMask, szMask, VersionInfoFlags);
	ResourceParseFlags(pVer->dwFileFlags, szFlags, VersionInfoFlags);
	ResourceParseFlags(pVer->dwFileOS, szOS, VersionInfoFileOS);
	szFileType[0] = 0;
	wsprintfW(szFileSubType, L"0x%XL", pVer->dwFileSubtype);
	switch (pVer->dwFileType) {
		case VFT_APP: wcscpy(szFileType, L"     // VFT_APP"); break;
		case VFT_DLL: wcscpy(szFileType, L"     // VFT_DLL"); break;
		case VFT_DRV: wcscpy(szFileType, L"     // VFT_DRV"); 
			{
				wchar_t* pszStart = szFileSubType;
				wchar_t* pszFlags = pszStart + lstrlenW(pszStart);
				while ((pszFlags - pszStart) < 8) {
					*(pszFlags++) = L' '; *pszFlags = 0;
				}
				for (int k=0; VersionInfoDrvSubtype[k].flag; k++) {
					if (pVer->dwFileSubtype == VersionInfoDrvSubtype[k].flag) {
						wcscat(szFileSubType, L" // "); wcscat(szFileSubType, VersionInfoDrvSubtype[k].name);
						break;
					}
				}
			}
			break;
		case VFT_FONT: wcscpy(szFileType, L"     // VFT_FONT");
			{
				wchar_t* pszStart = szFileSubType;
				wchar_t* pszFlags = pszStart + lstrlenW(pszStart);
				while ((pszFlags - pszStart) < 8) {
					*(pszFlags++) = L' '; *pszFlags = 0;
				}
				for (int k=0; VersionInfoFontSubtype[k].flag; k++) {
					if (pVer->dwFileSubtype == VersionInfoFontSubtype[k].flag) {
						wcscat(szFileSubType, L" // "); wcscat(szFileSubType, VersionInfoFontSubtype[k].name);
						break;
					}
				}
			}
			break;
		case VFT_VXD: wcscpy(szFileType, L"     // VFT_VXD"); break;
		case VFT_STATIC_LIB: wcscpy(szFileType, L"     // VFT_STATIC_LIB"); break;
	}

	wsprintfW(psz,
		L"#include <windows.h>\n\n"
		L"VS_VERSION_INFO VERSIONINFO\n"
		L" FILEVERSION    %u,%u,%u,%u\n"
		L" PRODUCTVERSION %u,%u,%u,%u\n"
		L" FILEFLAGSMASK  %s\n"
		L" FILEFLAGS      %s\n"
		L" FILEOS         %s\n"
		L" FILETYPE       0x%XL%s\n"
		L" FILESUBTYPE    %s\n"
		L"BEGIN\n"
		,
		HIWORD(pVer->dwFileVersionMS), LOWORD(pVer->dwFileVersionMS),
		HIWORD(pVer->dwFileVersionLS), LOWORD(pVer->dwFileVersionLS),
		HIWORD(pVer->dwProductVersionMS), LOWORD(pVer->dwProductVersionMS),
		HIWORD(pVer->dwProductVersionLS), LOWORD(pVer->dwProductVersionLS),
		szMask, szFlags,
		szOS,
		pVer->dwFileType, szFileType,
		szFileSubType
		);
	psz += wcslen(psz);
}

#define ALIGN_TOKEN(p) p = (LPWORD)( ((((DWORD_PTR)p) + 3) >> 2) << 2 );

void ParseVersionInfoVariableString(char* fileNameBuffer, MPanelItem *&pChild, LPVOID ptrRes, DWORD &resSize, LPBYTE &ptrBuf, BOOL &bIsCopy, LPWORD pToken, wchar_t* &psz)
{
	StringFileInfo *pSFI = (StringFileInfo*)pToken;
	LPWORD pEnd = (LPWORD)(((LPBYTE)ptrRes)+resSize);
	if (pToken < pEnd && *pToken > sizeof(StringFileInfo)) {
		LPWORD pEnd1 = (LPWORD)(((LPBYTE)pToken)+*pToken);
		if (pEnd < pEnd1)
			pEnd1 = pEnd;
		wcscat(psz, L"    BLOCK \"");
		wcscat(psz, pSFI->szKey);
		wcscat(psz, L"\"\n");
		if (pSFI->wType != 1) {
			wcscat(psz, L"    // Warning! Binary data in StringFileInfo\n");
		}
		{
			wcscat(psz, L"    BEGIN\n");
			psz += wcslen(psz);
			// Padding - Contains as many zero words as necessary to align the Children member on a 32-bit boundary.
			pToken = (LPWORD)(pSFI->szKey+wcslen(pSFI->szKey)+1);
			//while (*pToken == 0 && pToken < pEnd1) pToken++;
			ALIGN_TOKEN(pToken);
			if ((((LPBYTE)pToken)+sizeof(StringTable)) <= (LPBYTE)pEnd1) {
				StringTable *pST = (StringTable*)pToken;
				LPWORD pEnd2 = (LPWORD)(((LPBYTE)pToken)+*pToken);
				if (pEnd1 < pEnd2)
					pEnd2 = pEnd1;
				// Specifies an 8-digit hexadecimal number stored as a Unicode string. 
				// The four most significant digits represent the language identifier. 
				// The four least significant digits represent the code page for which 
				// the data is formatted. Each Microsoft Standard Language identifier contains 
				// two parts: the low-order 10 bits specify the major language, 
				// and the high-order 6 bits specify the sublanguage.
				wcscat(psz, L"        BLOCK \"");
				psz += wcslen(psz);
				wmemmove(psz, pST->szKey, 8);
				psz += 8; *psz = 0; wcscat(psz, L"\"\n");
				wcscat(psz, L"        BEGIN\n");
				pToken = (LPWORD)(pST->szKey+8);
				//while (*pToken == 0 && pToken < pEnd2) pToken++;
				ALIGN_TOKEN(pToken);
				while ((((LPBYTE)pToken)+sizeof(String)) <= (LPBYTE)pEnd2) {
					String *pS = (String*)pToken;
					if (pS->wLength == 0) break; // Invalid?
					LPWORD pNext = (LPWORD)(((LPBYTE)pToken)+pS->wLength);
					wcscat(psz, L"            VALUE \""); psz += wcslen(psz);
					wcscat(psz, pS->szKey);
					wcscat(psz, L"\", "); psz += wcslen(psz);
					// ¬ыровн€ть текст в результирующем .rc
					for (int k = lstrlenW(pS->szKey); k < 17; k++) *(psz++) = L' ';
					*(psz++) = L'"'; *psz = 0;
					pToken = (LPWORD)(pS->szKey+wcslen(pS->szKey)+1);
					//while (*pToken == 0 && pToken < pEnd2) pToken++;
					ALIGN_TOKEN(pToken);
					int nLenLeft = pS->wValueLength;
					while (pToken < pEnd2 && nLenLeft>0) {
						switch (*pToken) {
							case 0:
								*(psz++) = L'\\'; *(psz++) = L'0'; break;
							case L'\r':
								*(psz++) = L'\\'; *(psz++) = L'r'; break;
							case L'\n':
								*(psz++) = L'\\'; *(psz++) = L'n'; break;
							case L'\t':
								*(psz++) = L'\\'; *(psz++) = L't'; break;
							default:
								*(psz++) = *pToken;
						}
						pToken++; nLenLeft--;
					}
					*psz = 0;
					//if (pToken < pEnd2 && pS->wValueLength) {
					//	// ¬ообще-то тут бы провести замены \r\n\t"
					//	wcscat(psz, (LPCWSTR)pToken);
					//}
					wcscat(psz, L"\"\n"); psz += wcslen(psz);

					// Next value
					pToken = pNext;
					if (pToken < pEnd2 && *pToken == 0)
					{
						wcscat(psz, L"            // Zero-length item found\n"); psz += wcslen(psz);
						while (pToken < pEnd2 && *pToken == 0) pToken ++;
					}
				}
				wcscat(psz, L"        END\n");
			}
			//
			wcscat(psz, L"    END\n");
		}
	}
}
void ParseVersionInfoVariableStringA(char* fileNameBuffer, MPanelItem *&pChild, LPVOID ptrRes, DWORD &resSize, LPBYTE &ptrBuf, BOOL &bIsCopy, char* pToken, wchar_t* &psz)
{
	wchar_t szTemp[MAX_PATH*2+1], *pwsz = NULL;
	StringFileInfoA *pSFI = (StringFileInfoA*)pToken;
	char* pEnd = (char*)(((LPBYTE)ptrRes)+resSize);
	if (pToken < pEnd && *pToken > sizeof(StringFileInfoA)) {
		char* pEnd1 = (char*)(((LPBYTE)pToken)+*((WORD*)pToken));
		if (pEnd < pEnd1)
			pEnd1 = pEnd;
		wcscat(psz, L"    BLOCK \"");
		//wcscat(psz, pSFI->szKey);
		psz += wcslen(psz);
		MultiByteToWideChar(CP_ACP,0,pSFI->szKey,-1,psz,64);
		//for (int x=0; x<8; x++) {
		//	*psz++ = pSFI->szKey[x] ? pSFI->szKey[x] : L' ';
		//}
		wcscat(psz, L"\"\n");
		//if (pSFI->wType != 1) {
		//	wcscat(psz, L"    // Warning! Binary data in StringFileInfo\n");
		//}
		{
			wcscat(psz, L"    BEGIN\n");
			psz += wcslen(psz);
			// Padding - Contains as many zero words as necessary to align the Children member on a 32-bit boundary.
			pToken = (char*)(pSFI->szKey+strlen(pSFI->szKey)+1);
			//while (*pToken == 0 && pToken < pEnd1) pToken++;
			//ALIGN_TOKEN(pToken); ???
			pToken++; // ???
			if ((((LPBYTE)pToken)+sizeof(StringTableA)) <= (LPBYTE)pEnd1) {
				StringTableA *pST = (StringTableA*)pToken;
				char* pEnd2 = (char*)(((LPBYTE)pToken)+*((WORD*)pToken));
				if (pEnd2 > pEnd1)
					pEnd2 = pEnd1;
				// Specifies an 8-digit hexadecimal number stored as a Unicode string. 
				// The four most significant digits represent the language identifier. 
				// The four least significant digits represent the code page for which 
				// the data is formatted. Each Microsoft Standard Language identifier contains 
				// two parts: the low-order 10 bits specify the major language, 
				// and the high-order 6 bits specify the sublanguage.
				wcscat(psz, L"        BLOCK \"");
				psz += wcslen(psz);
				//wmemmove(psz, pST->szKey, 8); ???
				psz[MultiByteToWideChar(CP_ACP,0,pST->szKey,8,psz,64)] = 0;
				psz += 8; *psz = 0; wcscat(psz, L"\"\n");
				wcscat(psz, L"        BEGIN\n");
				pToken = (char*)(pST->szKey+8);
				//while (*pToken == 0 && pToken < pEnd2) pToken++;
				//ALIGN_TOKEN(pToken); ???
				pToken += 4; //???
				while ((((LPBYTE)pToken)+sizeof(StringA)) <= (LPBYTE)pEnd2) {
					StringA *pS = (StringA*)pToken;
					if (pS->wLength == 0) break; // Invalid?
					char* pNext = (char*)(((LPBYTE)pToken)+pS->wLength);
					if (pNext > pEnd2)
						pNext = pEnd2;
					wcscat(psz, L"            VALUE \""); psz += wcslen(psz);
					//wcscat(psz, pS->szKey); ???
					psz[MultiByteToWideChar(CP_ACP,0,pS->szKey,-1,psz,32)] = 0;
					wcscat(psz, L"\", "); psz += wcslen(psz);
					// ¬ыровн€ть текст в результирующем .rc
					for (int k = lstrlenA(pS->szKey); k < 17; k++) *(psz++) = L' ';
					*(psz++) = L'"'; *psz = 0;
					pToken = (char*)(pS->szKey+strlen(pS->szKey)+1);
					while (*pToken == 0 && pToken < pNext) pToken++;
					//ALIGN_TOKEN(pToken); ???
					int nLenLeft = min(pS->wValueLength,MAX_PATH*2);
					if (nLenLeft > (pNext-pToken))
						nLenLeft = (int)(pNext-pToken);
					szTemp[MultiByteToWideChar(CP_ACP,0,pToken,nLenLeft,szTemp,nLenLeft)] = 0;
					pwsz = szTemp;
					while (nLenLeft>0) {
						switch (*pwsz) {
							case 0:
								*(psz++) = L'\\'; *(psz++) = L'0'; break;
							case L'\r':
								*(psz++) = L'\\'; *(psz++) = L'r'; break;
							case L'\n':
								*(psz++) = L'\\'; *(psz++) = L'n'; break;
							case L'\t':
								*(psz++) = L'\\'; *(psz++) = L't'; break;
							default:
								*(psz++) = *pwsz;
						}
						pwsz++; nLenLeft--;
					}
					*psz = 0;
					//if (pToken < pEnd2 && pS->wValueLength) {
					//	// ¬ообще-то тут бы провести замены \r\n\t"
					//	wcscat(psz, (LPCWSTR)pToken);
					//}
					wcscat(psz, L"\"\n"); psz += wcslen(psz);

					// Next value
					pToken = pNext;
					if (pToken < pEnd2 && *pToken == 0)
					{
						wcscat(psz, L"            // Zero-length item found\n"); psz += wcslen(psz);
						while (pToken < pEnd2 && *pToken == 0) pToken ++;
					}
				}
				wcscat(psz, L"        END\n");
			}
			//
			wcscat(psz, L"    END\n");
		}
	}
}

void ParseVersionInfoVariableVar(char* fileNameBuffer, MPanelItem *&pChild, LPVOID ptrRes, DWORD &resSize, LPBYTE &ptrBuf, BOOL &bIsCopy, LPWORD pToken, wchar_t* &psz)
{
	VarFileInfo *pSFI = (VarFileInfo*)pToken;
	LPWORD pEnd = (LPWORD)(((LPBYTE)ptrRes)+resSize);
	if (pToken < pEnd && *pToken > sizeof(VarFileInfo)) {
		LPWORD pEnd1 = (LPWORD)(((LPBYTE)pToken)+*pToken);
		if (pEnd < pEnd1)
			pEnd1 = pEnd;
		wcscat(psz, L"    BLOCK \"");
		wcscat(psz, pSFI->szKey);
		wcscat(psz, L"\"\n");
		if (pSFI->wType != 1) {
			wcscat(psz, L"    // Warning! Binary data in VarFileInfo\n");
		}
		{
			wcscat(psz, L"    BEGIN\n");
			psz += wcslen(psz);
			// Padding - Contains as many zero words as necessary to align the Children member on a 32-bit boundary.
			pToken = (LPWORD)(pSFI->szKey+wcslen(pSFI->szKey)+1);
			//while (*pToken == 0 && pToken < pEnd1) pToken++;
			ALIGN_TOKEN(pToken);
			if ((((LPBYTE)pToken)+sizeof(Var)) <= (LPBYTE)pEnd1) {
				pToken = (LPWORD)(pSFI->szKey+wcslen(pSFI->szKey)+1);
				//while (*pToken == 0 && pToken < pEnd1) pToken++;
				ALIGN_TOKEN(pToken);
				while ((((LPBYTE)pToken)+sizeof(Var)) <= (LPBYTE)pEnd1) {
					Var *pS = (Var*)pToken;
					if (pS->wLength == 0) break; // Invalid?
					LPWORD pNext = (LPWORD)(((LPBYTE)pToken)+pS->wLength);
					wcscat(psz, L"        VALUE \""); psz += wcslen(psz);
					wcscat(psz, pS->szKey);
					wcscat(psz, L"\""); psz += wcslen(psz);
					pToken = (LPWORD)(pS->szKey+wcslen(pS->szKey)+1);
					// Align to 32bit boundary
					ALIGN_TOKEN(pToken);
					//pToken++;
					//while (*pToken == 0 && pToken < pEnd1) pToken++;

					// The low-order word of each DWORD must contain a Microsoft language identifier, 
					// and the high-order word must contain the IBM code page number. 
					// Either high-order or low-order word can be zero, indicating that the file 
					// is language or code page independent. 
					while ((pToken+2) <= pEnd1) {
						psz += wcslen(psz);
						//DWORD nLangCP = *((LPDWORD)pToken);
						wsprintfW(psz, L", 0x%X, %u", (DWORD)(pToken[0]), (DWORD)(pToken[1]));
						pToken += 2;
					}
					//	// ¬ообще-то тут бы провести замены \r\n\t"
					//	wcscat(psz, (LPCWSTR)pToken);
					//}
					wcscat(psz, L"\n"); psz += wcslen(psz);

					// Next value
					pToken = pNext;
				}
			}
			wcscat(psz, L"    END\n");
		}
	}
}

void ParseVersionInfoVariable(char* fileNameBuffer, MPanelItem *&pChild, LPVOID ptrRes, DWORD &resSize, LPBYTE &ptrBuf, BOOL &bIsCopy, LPWORD pToken, wchar_t* &psz)
{
	StringFileInfo *pSFI = (StringFileInfo*)pToken;

	if (_wcsicmp(pSFI->szKey, L"StringFileInfo") == 0) {
		LPWORD pEnd = (LPWORD)(((LPBYTE)ptrRes)+resSize);
		if (pToken < pEnd && *pToken > sizeof(StringFileInfo)) {
			ParseVersionInfoVariableString(fileNameBuffer, pChild, ptrRes, resSize, ptrBuf, bIsCopy, pToken, psz);
		}
	} else if (_wcsicmp(pSFI->szKey, L"VarFileInfo") == 0) {
		LPWORD pEnd = (LPWORD)(((LPBYTE)ptrRes)+resSize);
		if (pToken < pEnd && *pToken > sizeof(VarFileInfo)) {
			ParseVersionInfoVariableVar(fileNameBuffer, pChild, ptrRes, resSize, ptrBuf, bIsCopy, pToken, psz);
		}
	}
}
void ParseVersionInfoVariableA(char* fileNameBuffer, MPanelItem *&pChild, LPVOID ptrRes, DWORD &resSize, LPBYTE &ptrBuf, BOOL &bIsCopy, char* pToken, wchar_t* &psz)
{
	StringFileInfoA *pSFI = (StringFileInfoA*)pToken;

	if (_stricmp(pSFI->szKey, "StringFileInfo") == 0) {
		char* pEnd = (char*)(((LPBYTE)ptrRes)+resSize);
		if (pToken < pEnd && *pToken > sizeof(StringFileInfo)) {
			ParseVersionInfoVariableStringA(fileNameBuffer, pChild, ptrRes, resSize, ptrBuf, bIsCopy, pToken, psz);
		}
	}
}

void ParseVersionInfo(char* fileNameBuffer, MPanelItem *&pChild, LPVOID &ptrRes, DWORD &resSize, LPBYTE &ptrBuf, BOOL &bIsCopy)
{
	WORD nTestSize = ((WORD*)ptrRes)[0];
	WORD nTestShift = ((WORD*)ptrRes)[1];
	if (nTestSize == resSize && nTestShift < resSize) {
		VS_FIXEDFILEINFO* pVer = (VS_FIXEDFILEINFO*)(((LPBYTE)ptrRes)+0x28);
		// ѕо идее, должно быть здесь, но если нет - ищем сигнатуру
		if (pVer->dwSignature != 0xfeef04bd) {
			DWORD nMax = resSize - sizeof(VS_FIXEDFILEINFO);
			for (UINT i = 4; i < nMax; i++) {
				if (((VS_FIXEDFILEINFO*)(((LPBYTE)ptrRes)+i))->dwSignature == 0xfeef04bd) {
					pVer = (VS_FIXEDFILEINFO*)(((LPBYTE)ptrRes)+i);
					break;
				}
			}
		}
		DWORD nNewSize = resSize*2 + 2048;
		if (pVer->dwSignature == 0xfeef04bd && nNewSize > resSize) {
			ptrBuf = (LPBYTE)malloc(nNewSize);
			if (!ptrBuf) {
				//pRoot->printf(_T("\n!!! Can't allocate %i bytes !!!\n"), nNewSize);
			} else {
				wchar_t* psz = (LPWSTR)ptrBuf;
				WORD nBOM = 0xFEFF; // CP-1200
				*psz++ = nBOM;
				
				// VS_FIXEDFILEINFO
				ParseVersionInfoFixed(fileNameBuffer, pChild, ptrRes, resSize, ptrBuf, bIsCopy, pVer, psz);

				StringFileInfo *pSFI = (StringFileInfo*)(pVer+1);
				LPWORD pToken = (LPWORD)pSFI;
				LPWORD pEnd = (LPWORD)(((LPBYTE)ptrRes)+resSize);
				while (pToken < pEnd && *pToken > sizeof(StringFileInfo)) {
					pSFI = (StringFileInfo*)pToken;
					if (pSFI->wLength == 0)
						break; // Invalid
					ParseVersionInfoVariable(fileNameBuffer, pChild, ptrRes, resSize, ptrBuf, bIsCopy, pToken, psz);
					//
					pToken = (LPWORD)(((LPBYTE)pSFI)+pSFI->wLength);
					while (pToken < pEnd && *pToken == 0)
						pToken ++;
					psz += wcslen(psz);
				}

				// Done
				wcscat(psz, L"END\n");
				{
					// сначала добавим "бинарные данные" ресурса (NOT PARSED)
					//pChild = pRoot->AddFile(fileNameBuffer, resSize);
					//pChild->SetData((LPBYTE)ptrRes, resSize);
				}
				strcat(fileNameBuffer, ".rc");
				ptrRes = ptrBuf;
				// » вернем преобразованные в "*.rc" (PARSED)
				resSize = lstrlenW((LPWSTR)ptrBuf)*2; // первый WORD - BOM
				bIsCopy = TRUE;
			}
		}
	}
}

// ANSI for 16bit PE's
void ParseVersionInfoA(char* fileNameBuffer, MPanelItem *&pChild, LPVOID &ptrRes, DWORD &resSize, LPBYTE &ptrBuf, BOOL &bIsCopy)
{
	WORD nTestSize = ((WORD*)ptrRes)[0];
	//WORD nTestShift = ((WORD*)ptrRes)[1]; // ???
	if (nTestSize >= sizeof(VS_FIXEDFILEINFO) && nTestSize <= resSize /*&& nTestShift < resSize*/) {
		VS_FIXEDFILEINFO* pVer = (VS_FIXEDFILEINFO*)(((LPBYTE)ptrRes)+0x28);
		// ѕо идее, должно быть здесь, но если нет - ищем сигнатуру
		if (pVer->dwSignature != 0xfeef04bd) {
			DWORD nMax = resSize - sizeof(VS_FIXEDFILEINFO);
			for (UINT i = 4; i < nMax; i++) {
				//BUGBUG: ѕроверить, на x64 жить такое будет, или свалитс€ на Alignment?
				if (((VS_FIXEDFILEINFO*)(((LPBYTE)ptrRes)+i))->dwSignature == 0xfeef04bd) {
					pVer = (VS_FIXEDFILEINFO*)(((LPBYTE)ptrRes)+i);
					break;
				}
			}
		}
		DWORD nNewSize = resSize*2 + 2048;
		if (pVer->dwSignature == 0xfeef04bd // —игнатура найдена
			&& nNewSize > resSize)          // –азмер ресурса (resSize) не превышает DWORD :)
		{
			ptrBuf = (LPBYTE)malloc(nNewSize);
			if (!ptrBuf) {
				//pRoot->printf(_T("\n!!! Can't allocate %i bytes !!!\n"), nNewSize);
			} else {
				wchar_t* psz = (LPWSTR)ptrBuf;
				WORD nBOM = 0xFEFF; // CP-1200
				*psz++ = nBOM;

				// VS_FIXEDFILEINFO
				ParseVersionInfoFixed(fileNameBuffer, pChild, ptrRes, resSize, ptrBuf, bIsCopy, pVer, psz);

				StringFileInfoA *pSFI = (StringFileInfoA*)(pVer+1);
				char* pToken = (char*)pSFI;
				char* pEnd = (char*)(((LPBYTE)ptrRes)+resSize);
				while (pToken < pEnd && *pToken > sizeof(StringFileInfoA)) {
					pSFI = (StringFileInfoA*)pToken;
					if (pSFI->wLength == 0)
						break; // Invalid
					ParseVersionInfoVariableA(fileNameBuffer, pChild, ptrRes, resSize, ptrBuf, bIsCopy, pToken, psz);
					//
					pToken = (char*)(((LPBYTE)pSFI)+pSFI->wLength);
					while (pToken < pEnd && *pToken == 0)
						pToken ++;
					psz += wcslen(psz);
				}


				// Done
				wcscat(psz, L"END\n");
				{
					// сначала добавим "бинарные данные" ресурса (NOT PARSED)
					//pChild = pRoot->AddFile(fileNameBuffer, resSize);
					//pChild->SetData((LPBYTE)ptrRes, resSize);
				}
				strcat(fileNameBuffer, ".rc");
				ptrRes = ptrBuf;
				// » вернем преобразованные в "*.rc" (PARSED)
				resSize = lstrlenW((LPWSTR)ptrBuf)*2;
				bIsCopy = TRUE;
			}
		}
	}
}

MPanelItem* CreateResource(DWORD rootType, LPVOID ptrRes, DWORD resSize,
						   LPCSTR asID, LPCSTR langID, DWORD stringIdBase, DWORD anLangId)
{
	char fileNameBuffer[MAX_PATH];
	MPanelItem *pChild = NULL;
	LPBYTE ptrBuf = NULL;
	BOOL bIsCopy = FALSE;
	TCHAR szType[32] = {0}, szInfo[64] = {0};
	TCHAR szTextInfo[1024];
	BOOL bDontSetData = FALSE;

	if (asID && *asID) {
		strcpy(fileNameBuffer, asID);
	} else {
		strcpy(fileNameBuffer, "????");
	}
	if (langID && *langID) {
		strcat(fileNameBuffer, "."); strcat(fileNameBuffer, langID);
	}

	switch (rootType) {
	case (WORD)RT_GROUP_ICON: case 0x8000+(WORD)RT_GROUP_ICON:
	case (WORD)RT_GROUP_CURSOR: case 0x8000+(WORD)RT_GROUP_CURSOR:
		{
			strcat(fileNameBuffer, ".txt");
			//pChild = pRoot->AddFile(fileNameBuffer, 0);
			if (!ValidateMemory(ptrRes, max(resSize,sizeof(GRPICONDIR)))) {
				//pChild->SetErrorPtr(ptrRes, resSize);
			} else if (resSize < sizeof(GRPICONDIR)) {
				//pChild->SetColumns(
				//	((rootType & 0x7FFF)==(WORD)RT_GROUP_ICON) ? _T("GROUP_ICON") : _T("GROUP_CURSOR"),
				//	_T("!!! Invalid size of resource"));
				//pChild->printf(_T("\n!!! Invalid size of %s resource, expected %i bytes, but only %i bytes exists !!!"),
				//	((rootType & 0x7FFF)==(WORD)RT_GROUP_ICON) ? _T("GROUP_ICON") : _T("GROUP_CURSOR"),
				//	sizeof(GRPICONDIR), resSize);
			} else {
				GRPICONDIR* pIcon = (GRPICONDIR*)ptrRes;
				int nSizeLeft = resSize - 6;
				int nResCount = pIcon->idCount;

				wsprintf(szInfo, _T("Count: %i"), nResCount);
				pChild->SetColumns(
					((rootType & 0x7FFF)==(WORD)RT_GROUP_ICON) ? _T("GROUP_ICON") : _T("GROUP_CURSOR"),
					szInfo);

				wsprintf(szTextInfo, _T("%s, Count: %u\n%s\n"),
					((rootType & 0x7FFF)==(WORD)RT_GROUP_ICON) ? _T("GROUP_ICON") : _T("GROUP_CURSOR"),
					nResCount,
					((rootType & 0x7FFF)==(WORD)RT_GROUP_ICON) ? _T("====================") : _T("======================"));
				pChild->AddText(szTextInfo, -1, TRUE); // Ќе добавл€ть в DUMP.TXT		

				LPGRPICONDIRENTRY pEntry = pIcon->idEntries;
				while (nSizeLeft >= sizeof(GRPICONDIRENTRY)) {
					if (!ValidateMemory(pEntry, sizeof(GRPICONDIRENTRY))) {
						pChild->SetErrorPtr(pEntry, sizeof(GRPICONDIRENTRY));
						break;
					}
					wsprintf(szTextInfo, 
						_T("ID: %u,  BytesInRes: %i\n")
						_T("  Width: %i, Height: %i\n")
						_T("  ColorCount: %i, Planes: %i, BitCount: %i\n\n"),
						(UINT)pEntry->nID, (UINT)pEntry->dwBytesInRes,
						(UINT)pEntry->bWidth, (UINT)pEntry->bHeight,
						(UINT)pEntry->bColorCount, (UINT)pEntry->wPlanes, (UINT)pEntry->wBitCount);
					//
					pChild->AddText(szTextInfo, -1, TRUE); // Ќе добавл€ть в DUMP.TXT		
					nSizeLeft -= sizeof(GRPICONDIRENTRY);
					nResCount --;
					pEntry++;
				}
			}
			return pChild;
		} break;
	case (WORD)RT_ICON: case 0x8000+(WORD)RT_ICON:
		{
			_tcscpy(szType, _T("ICON"));

			if (!ValidateMemory(ptrRes, resSize)) {
				//pRoot->SetErrorPtr(ptrRes, resSize);
			} else if (resSize>4 && *((DWORD*)ptrRes) == 0x474e5089/* %PNG */) {
				strcat(fileNameBuffer, ".png");
				_tcscpy(szInfo, _T("PNG format"));
			} else
			if (resSize > sizeof(BITMAPINFOHEADER) 
				&& ((BITMAPINFOHEADER*)ptrRes)->biSize == sizeof(BITMAPINFOHEADER)
				&& (((BITMAPINFOHEADER*)ptrRes)->biWidth && ((BITMAPINFOHEADER*)ptrRes)->biWidth < 256)
				&& (((BITMAPINFOHEADER*)ptrRes)->biHeight == (((BITMAPINFOHEADER*)ptrRes)->biWidth * 2)))
			{
				wsprintf(szInfo, _T("%ix%i (%ibpp)"), 
					((BITMAPINFOHEADER*)ptrRes)->biWidth, ((BITMAPINFOHEADER*)ptrRes)->biWidth,
					((BITMAPINFOHEADER*)ptrRes)->biBitCount*((BITMAPINFOHEADER*)ptrRes)->biPlanes);

				// ƒелаем копию буфера, но предвар€ем его заголовком иконки
				DWORD nNewSize = resSize + sizeof(ICONDIR);
				ptrBuf = (LPBYTE)malloc(nNewSize);
				if (!ptrBuf) {
					//pRoot->printf(_T("\n!!! Can't allocate %i bytes !!!\n"), nNewSize);
				} else {
					ICONDIR* pIcon = (ICONDIR*)ptrBuf;
					pIcon->idReserved = 0;
					pIcon->idType = 1;
					pIcon->idCount = 1;
					pIcon->idEntries[0].bWidth = (BYTE)((BITMAPINFOHEADER*)ptrRes)->biWidth;
					pIcon->idEntries[0].bHeight = (BYTE)((BITMAPINFOHEADER*)ptrRes)->biWidth;
					pIcon->idEntries[0].bColorCount = (((BITMAPINFOHEADER*)ptrRes)->biBitCount >= 8)
						? 0 : (1 << ((BITMAPINFOHEADER*)ptrRes)->biBitCount);
					pIcon->idEntries[0].bReserved = 0;
					pIcon->idEntries[0].wPlanes = ((BITMAPINFOHEADER*)ptrRes)->biPlanes;
					pIcon->idEntries[0].wBitCount = ((BITMAPINFOHEADER*)ptrRes)->biBitCount;
					pIcon->idEntries[0].dwBytesInRes = resSize;
					pIcon->idEntries[0].dwImageOffset = sizeof(ICONDIR);
					memmove(&(pIcon->idEntries[1]), ptrRes, resSize);

					ptrRes = ptrBuf;
					resSize = nNewSize;
					bIsCopy = TRUE;
					strcat(fileNameBuffer, ".ico");
				}
			}
		} break;
	case (WORD)RT_CURSOR: case 0x8000+(WORD)RT_CURSOR:
		{
			_tcscpy(szType, _T("CURSOR"));

			if (!ValidateMemory(ptrRes, resSize)) {
				//pRoot->SetErrorPtr(ptrRes, resSize);
			} else if (resSize>4 && *((DWORD*)ptrRes) == 'GNP%') {
				strcat(fileNameBuffer, ".png");
			} else if (resSize > (sizeof(BITMAPINFOHEADER)+4)) {
				BITMAPINFOHEADER* pBmpTest = (BITMAPINFOHEADER*)(((LPBYTE)ptrRes)+4);
				if (pBmpTest->biSize == sizeof(BITMAPINFOHEADER)
					&& pBmpTest->biWidth && pBmpTest->biWidth < 256
					&& pBmpTest->biHeight == pBmpTest->biWidth * 2)
				{
					wsprintf(szInfo, _T("%ix%i (%ibpp)"), 
						pBmpTest->biWidth, pBmpTest->biWidth,
						pBmpTest->biBitCount*pBmpTest->biPlanes);

					// ƒелаем копию буфера, но предвар€ем его заголовком иконки
					DWORD nNewSize = resSize + sizeof(ICONDIR) - 4;
					ptrBuf = (LPBYTE)malloc(nNewSize);
					if (!ptrBuf) {
						//pRoot->printf(_T("\n!!! Can't allocate %i bytes !!!\n"), nNewSize);
					} else {
						ICONDIR* pIcon = (ICONDIR*)ptrBuf;
						pIcon->idReserved = 0;
						pIcon->idType = 2;
						pIcon->idCount = 1;
						pIcon->idEntries[0].bWidth = (BYTE)pBmpTest->biWidth;
						pIcon->idEntries[0].bHeight = (BYTE)pBmpTest->biWidth;
						pIcon->idEntries[0].bColorCount = 0;
							//(pBmpTest->biBitCount >= 8) ? 0 : (1 << (pBmpTest->biBitCount));
						pIcon->idEntries[0].bReserved = 0;
						pIcon->idEntries[0].wPlanes = ((WORD*)ptrRes)[0];
						pIcon->idEntries[0].wBitCount = ((WORD*)ptrRes)[1];
						pIcon->idEntries[0].dwBytesInRes = resSize-4;
						pIcon->idEntries[0].dwImageOffset = sizeof(ICONDIR);
						memmove(&(pIcon->idEntries[1]), ((LPBYTE)ptrRes)+4, resSize-4);

						ptrRes = ptrBuf;
						resSize = nNewSize;
						bIsCopy = TRUE;
						strcat(fileNameBuffer, ".cur");
					}
				}
			}
		} break;
	case (WORD)RT_BITMAP: case 0x8000+(WORD)RT_BITMAP:
		{
			_tcscpy(szType, _T("BITMAP"));
			DWORD nNewSize = sizeof(BITMAPFILEHEADER)+resSize;
			if (!ValidateMemory(ptrRes, max(sizeof(BITMAPINFOHEADER),resSize))) {
				//pRoot->SetErrorPtr(ptrRes, resSize);
			} else if (nNewSize<resSize) {
				//pRoot->AddText(_T("\n!!! Too large resource, can't insert header !!!\n"));
			} else {
				LPBITMAPINFOHEADER pInfo = (LPBITMAPINFOHEADER)ptrRes;
				if (pInfo->biSize < sizeof(BITMAPINFOHEADER)
					|| pInfo->biPlanes != 1 || pInfo->biBitCount > 32
					|| (pInfo->biWidth & 0xFF000000) || (pInfo->biHeight & 0xFF000000))
				{
					//pRoot->AddText(_T("\n!!! Invalid BITMAPINFOHEADER !!!\n"));
					_tcscpy(szInfo, _T("Invalid BITMAPINFOHEADER"));
				} else {
					ptrBuf = (LPBYTE)malloc(nNewSize);
					if (!ptrBuf) {
						//pRoot->printf(_T("\n!!! Can't allocate %i bytes !!!\n"), nNewSize);
					} else {
						LPBITMAPFILEHEADER pBmp = (LPBITMAPFILEHEADER)ptrBuf;

						wsprintf(szInfo, _T("%ix%i (%ibpp)"), 
							pInfo->biWidth, pInfo->biWidth,
							pInfo->biBitCount*pInfo->biPlanes);

						pBmp->bfType = 0x4d42; //'BM';
						pBmp->bfSize = nNewSize;
						pBmp->bfReserved1 = pBmp->bfReserved2 = 0;
						//TODO: наверное еще и размер таблицы цветов нужно добавить?
						pBmp->bfOffBits = sizeof(BITMAPFILEHEADER)+pInfo->biSize;
						if (pInfo->biBitCount == 0 || pInfo->biCompression == BI_JPEG || pInfo->biCompression == BI_PNG) {
							// ƒополнительные сдвиги на палитру не требуютс€
						} else {
							if (pInfo->biCompression == BI_BITFIELDS) {
								if (pInfo->biBitCount == 16 || pInfo->biBitCount == 32) {
									// color table consists of three DWORD color masks that specify the 
									// red, green, and blue components, respectively, of each pixel
									pBmp->bfOffBits += 3 * sizeof(DWORD);
								}
							} else if (pInfo->biCompression == BI_RGB || pInfo->biCompression == BI_RLE4 || pInfo->biCompression == BI_RLE8) {
								if (pInfo->biBitCount == 1 || pInfo->biBitCount == 4 || pInfo->biBitCount == 8) {
									if (pInfo->biClrUsed == 0) {
										pBmp->bfOffBits += ((DWORD)1 << pInfo->biBitCount) * (DWORD)sizeof(RGBQUAD);
									} else if (pInfo->biClrUsed <= 256) {
										// BUGBUG: What is real palette size in such files?
										pBmp->bfOffBits += pInfo->biClrUsed * (DWORD)sizeof(RGBQUAD);
									}
								}
							}
						}
						
						// Copying contents of bitmap resource
						memmove(pBmp+1, ptrRes, resSize);

						ptrRes = ptrBuf;
						resSize = nNewSize;
						bIsCopy = TRUE;
						strcat(fileNameBuffer, ".bmp");
					}
				}
			}
		} break;
	case (WORD)RT_MANIFEST:
		{
			strcat(fileNameBuffer, ".xml");
		} break;
	case (WORD)RT_STRING:
		{
			//MPanelItem* pBin = pRoot->AddFolder(_T("Binary Data"), FALSE);

			//if (!pRoot->IsColumnTitles())
			//	pBin->AddText( _T("<String Table>\n==============\n"), -1, TRUE );

			_ASSERTE(stringIdBase<=0x1000); // base of StringID
			//__try {
				DumpStringTable( pBin, stringIdBase, anLangId, (LPCWSTR)ptrRes, resSize );
			//}__except(EXCEPTION_EXECUTE_HANDLER){
			//	pRoot->SetError(SZ_DUMP_EXCEPTION);
			//}

			//pBin->AddText( _T("\n"), -1, TRUE );

			//pRoot = pBin;

		} break;
	case (WORD)RT_VERSION:
		{
			if (resSize > sizeof(VS_FIXEDFILEINFO)) {
				ParseVersionInfo(fileNameBuffer, pChild, ptrRes, resSize, ptrBuf, bIsCopy);
			}
		} break;
	case 0x8000+(WORD)RT_VERSION:
		{
			if (resSize > sizeof(VS_FIXEDFILEINFO)) {
				ParseVersionInfoA(fileNameBuffer, pChild, ptrRes, resSize, ptrBuf, bIsCopy);
			}
		} break;
	}

	//pChild = pRoot->AddFile(fileNameBuffer, resSize);
	//if (szType[0]) {
	//	if (!pRoot->IsColumnTitles())
	//		pRoot->SetColumnsTitles(cszType, 12, cszInfo, 0, -18);
	//
	//	pChild->SetColumns(szType, szInfo[0] ? szInfo : NULL);
	//}

	//if (!bDontSetData)
	//	pChild->SetData((LPBYTE)ptrRes, resSize, bIsCopy);

	if (ptrBuf)
		free(ptrBuf);
	return pChild;
}


//
// Dump the information about one resource directory entry.  If the
// entry is for a subdirectory, call the directory dumping routine
// instead of printing information in this routine.
//
void DumpResourceEntry(
    LPCSTR asID,
	PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry,
    PBYTE pResourceBase,
    DWORD level, DWORD rootType, DWORD parentType )
{
    UINT i;
    char nameBuffer[128];
    PIMAGE_RESOURCE_DATA_ENTRY pResDataEntry;
    
    if ( pResDirEntry->OffsetToData & IMAGE_RESOURCE_DATA_IS_DIRECTORY )
    {
        DumpResourceDirectory( (PIMAGE_RESOURCE_DIRECTORY)
            ((pResDirEntry->OffsetToData & 0x7FFFFFFF) + pResourceBase),
            pResourceBase, level, pResDirEntry->Name, rootType);
        return;
    }

	pResDataEntry = MakePtr(PIMAGE_RESOURCE_DATA_ENTRY, pResourceBase, pResDirEntry->OffsetToData);

	LPVOID ptrRes = NULL;
	if (g_bIs64Bit)
		ptrRes = GetPtrFromRVA(pResDataEntry->OffsetToData, gpNTHeader64, g_pMappedFileBase);
	else
		ptrRes = GetPtrFromRVA(pResDataEntry->OffsetToData, gpNTHeader32, g_pMappedFileBase);

    if ( pResDirEntry->Name & IMAGE_RESOURCE_NAME_IS_STRING )
    {
        GetResourceNameFromId(pResDirEntry->Name, pResourceBase, nameBuffer,
                              sizeof(nameBuffer));
    }
    else
    {
		sprintf(nameBuffer, "0x%04X", pResDirEntry->Name);
    }

	CreateResource(rootType, ptrRes, pResDataEntry->Size, asID, nameBuffer, parentType, pResDirEntry->Name);

	//// Spit out the spacing for the level indentation
	//for ( i=0; i < level; i++ ) pRoot->printf("    ");
	//pRoot->printf("Name: %s  DataEntryOffs: %08X\n",
	//	nameBuffer, pResDirEntry->OffsetToData);
	//
	//
    //// the resDirEntry->OffsetToData is a pointer to an
    //// IMAGE_RESOURCE_DATA_ENTRY.  Go dump out that information.  First,
    //// spit out the proper indentation
    //for ( i=0; i < level; i++ ) pRoot->printf("    ");
    //
    //pRoot->printf("DataRVA: %05X  DataSize: %05X  CodePage: %X\n",
    //        pResDataEntry->OffsetToData, pResDataEntry->Size,
    //        pResDataEntry->CodePage);
}


//
// Dump the information about one resource directory.
//
void DumpResourceDirectory( PIMAGE_RESOURCE_DIRECTORY pResDir,
							PBYTE pResourceBase,
							DWORD level,
							DWORD resourceType, DWORD rootType /*= 0*/, DWORD parentType /*= 0*/ )
{
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resDirEntry;
    char szType[64];
    UINT i;

    // Level 1 resources are the resource types
    if ( level == 1 )
    {
		rootType = resourceType;

		if ( resourceType & IMAGE_RESOURCE_NAME_IS_STRING )
		{
			GetResourceNameFromId( resourceType, pResourceBase,
									szType, sizeof(szType) );
		}
		else
		{
	        GetResourceTypeName( resourceType, szType, sizeof(szType) );
		}
	}
    else    // All other levels, just print out the regular id or name
    {
        GetResourceNameFromId( resourceType, pResourceBase, szType,
                               sizeof(szType) );
    }
	
	//if (level == 1) {
	//	// заложимс€ на то, что в 16бит PE в типах ресурсов установлен бит 0x8000
	//	if ((resourceType & 0x7FFF) == (WORD)RT_GROUP_CURSOR)
	//		pChild = pRoot->AddFolder(_T("CURSOR"), FALSE);
	//	else if ((resourceType & 0x7FFF) == (WORD)RT_GROUP_ICON)
	//		pChild = pRoot->AddFolder(_T("ICON"), FALSE);
	//	else
	//		pChild = pRoot->AddFolder(szType, FALSE);
	//}


	//
	// The "directory entries" immediately follow the directory in memory
	//
    resDirEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResDir+1);
	    
    for ( i=0; i < pResDir->NumberOfNamedEntries; i++, resDirEntry++ )
        DumpResourceEntry(pChild, szType, resDirEntry, pResourceBase, level+1, rootType, resourceType);

    for ( i=0; i < pResDir->NumberOfIdEntries; i++, resDirEntry++ )
        DumpResourceEntry(pChild, szType, resDirEntry, pResourceBase, level+1, rootType, resourceType);
}


//
// Top level routine called to dump out the entire resource hierarchy
//
template <class T> void DumpResourceSection( PBYTE pImageBase, T * pNTHeader)	// 'T' = PIMAGE_NT_HEADERS 32/64
{
	DWORD resourcesRVA;
    PBYTE pResDir;

	bool bIs64Bit = ( pNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC );

	resourcesRVA = GetImgDirEntryRVA(pNTHeader, IMAGE_DIRECTORY_ENTRY_RESOURCE);
	if ( !resourcesRVA )
		return;

    pResDir = (PBYTE)GetPtrFromRVA( resourcesRVA, pNTHeader, pImageBase );

	if ( !pResDir )
		return;
		
    DumpResourceDirectory((PIMAGE_RESOURCE_DIRECTORY)pResDir, pResDir, 0, 0);

	if ( !fShowResources )
		return;
}

void DumpResources( PBYTE pImageBase, PIMAGE_NT_HEADERS32 pNTHeader )
{
	if ( pNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC )
		DumpResourceSection( pImageBase, (PIMAGE_NT_HEADERS64)pNTHeader );
	else
		DumpResourceSection( pImageBase, (PIMAGE_NT_HEADERS32)pNTHeader );
}


//
// top level routine called from PEDUMP.CPP to dump the components of a PE file
//
bool DumpExeFile( PIMAGE_DOS_HEADER dosHeader )
{
    PIMAGE_NT_HEADERS32 pNTHeader;
    PBYTE pImageBase = (PBYTE)dosHeader;
    
	// Make pointers to 32 and 64 bit versions of the header.
    pNTHeader = MakePtr( PIMAGE_NT_HEADERS32, dosHeader,
                                dosHeader->e_lfanew );

	DWORD nSignature = 0;
    // First, verify that the e_lfanew field gave us a reasonable
    // pointer, then verify the PE signature.
	if ( !IsBadReadPtr( pNTHeader, sizeof(pNTHeader->Signature) ) )
	{
		nSignature = pNTHeader->Signature;
		if ( nSignature == IMAGE_NT_SIGNATURE )
		{
			return DumpExeFilePE( dosHeader, pNTHeader );
		}
		else if ( (nSignature & 0xFFFF) == IMAGE_OS2_SIGNATURE )
		{
			return DumpExeFileNE( dosHeader, (IMAGE_OS2_HEADER*)pNTHeader );
		}
		else if ( (nSignature & 0xFFFF) == IMAGE_OS2_SIGNATURE_LE )
		{
			return DumpExeFileNE( dosHeader, (IMAGE_OS2_HEADER*)pNTHeader );
		}
		else if ( (nSignature & 0xFFFF) == IMAGE_VXD_SIGNATURE )
		{
			return DumpExeFileVX( dosHeader, (IMAGE_VXD_HEADER*)pNTHeader );
		}
	}

    return false;
}

bool ValidateMemory(LPCVOID ptr, size_t nSize)
{
	if (!ptr || (LPBYTE)ptr < (LPBYTE)g_pMappedFileBase)
		return false;
	ULONGLONG nPos = ((LPBYTE)ptr - (LPBYTE)g_pMappedFileBase);
	if ((nPos+nSize) > g_FileSize.QuadPart)
		return false;
	return true;
}

bool DumpExeFileVX( PIMAGE_DOS_HEADER dosHeader, PIMAGE_VXD_HEADER pVXDHeader )
{
	//PBYTE pImageBase = (PBYTE)dosHeader;
	//
	//MPanelItem* pChild = pRoot->AddFolder(_T("VxD Header"));
	//pChild->AddText(_T("<VxD Header>\n"));
	//
	//pChild->AddText(_T("  Signature:         IMAGE_VXD_SIGNATURE\n"));
	//
	//MPanelItem* pDos = pRoot->AddFile(_T("DOS_Header"), sizeof(*dosHeader));
	//pDos->SetData((const BYTE*)dosHeader, sizeof(*dosHeader));
	//
	//MPanelItem* pVXD = pRoot->AddFile(_T("VxD_Header"), sizeof(*pVXDHeader));
	//pVXD->SetData((const BYTE*)pVXDHeader, sizeof(*pVXDHeader));

	return false;
}

bool DumpExeFilePE( PIMAGE_DOS_HEADER dosHeader, PIMAGE_NT_HEADERS32 pNTHeader )
{
	PBYTE pImageBase = (PBYTE)dosHeader;
	PIMAGE_NT_HEADERS64 pNTHeader64;

	pNTHeader64 = (PIMAGE_NT_HEADERS64)pNTHeader;

	bool bIs64Bit = ( pNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC );
	g_bIs64Bit = bIs64Bit;

	if ( bIs64Bit ) {
		gpNTHeader64 = pNTHeader64;
		//DumpOptionalHeader(pRoot, &pNTHeader64->OptionalHeader);
		//
		//MPanelItem* pPE = pRoot->AddFile(_T("PE64_Header"), sizeof(*gpNTHeader64));
		//pPE->SetData((const BYTE*)gpNTHeader64, sizeof(*gpNTHeader64));
	} else {
		gpNTHeader32 = (PIMAGE_NT_HEADERS32)pNTHeader;
		//DumpOptionalHeader(pRoot, &pNTHeader->OptionalHeader);
		//
		//MPanelItem* pPE = pRoot->AddFile(_T("PE32_Header"), sizeof(*gpNTHeader32));
		//pPE->SetData((const BYTE*)gpNTHeader32, sizeof(*gpNTHeader32));
	}

    //pRoot->printf("\n");
    //
	//// IsExe = TRUE, means "NOT *.obj file"
    //DumpSectionTable( pRoot, IMAGE_FIRST_SECTION(pNTHeader), 
    //                    pNTHeader->FileHeader.NumberOfSections, TRUE);
    //pRoot->printf("\n");
    //
	//if ( bIs64Bit )
	//	DumpExeDebugDirectory( pRoot, pImageBase, pNTHeader64 );
	//else
	//	DumpExeDebugDirectory( pRoot, pImageBase, pNTHeader );
	//
    //if ( pNTHeader->FileHeader.PointerToSymbolTable == 0 )
    //    g_pCOFFHeader = 0; // Doesn't really exist!
    //pRoot->printf("\n");

	DumpResources(pImageBase, pNTHeader);

	//bIs64Bit
	//	? DumpTLSDirectory( pRoot, pImageBase, pNTHeader64, (PIMAGE_TLS_DIRECTORY64)0 )	// Passing NULL ptr is a clever hack
	//	: DumpTLSDirectory( pRoot, pImageBase, pNTHeader, (PIMAGE_TLS_DIRECTORY32)0 );		// See if you can figure it out! :-)
	//
	//bIs64Bit
	//    ? DumpImportsSection(pRoot, pImageBase, pNTHeader64 )
	//	: DumpImportsSection(pRoot, pImageBase, pNTHeader);
	//
	//bIs64Bit
	//	? DumpDelayedImportsSection(pRoot, pImageBase, pNTHeader64, bIs64Bit )
	//	: DumpDelayedImportsSection(pRoot, pImageBase, pNTHeader, bIs64Bit );
	//
	//bIs64Bit 
	//	? DumpBoundImportDescriptors( pRoot, pImageBase, pNTHeader64 )
	//	: DumpBoundImportDescriptors( pRoot, pImageBase, pNTHeader );
	//
	//bIs64Bit
	//    ? DumpExportsSection( pRoot, pImageBase, pNTHeader64 )
	//	: DumpExportsSection( pRoot, pImageBase, pNTHeader );
	//
	//bIs64Bit
	//	? DumpCOR20Header( pRoot, pImageBase, pNTHeader64 )
	//	: DumpCOR20Header( pRoot, pImageBase, pNTHeader );
	//
	//bIs64Bit
	//	? DumpLoadConfigDirectory( pRoot, pImageBase, pNTHeader64, (PIMAGE_LOAD_CONFIG_DIRECTORY64)0 )	// Passing NULL ptr is a clever hack
	//	: DumpLoadConfigDirectory( pRoot, pImageBase, pNTHeader, (PIMAGE_LOAD_CONFIG_DIRECTORY32)0 );	// See if you can figure it out! :-)
	//
	//bIs64Bit
	//	? DumpCertificates( pRoot, pImageBase, pNTHeader64 )
	//	: DumpCertificates( pRoot, pImageBase, pNTHeader );
	//
	////=========================================================================
	////
	//// If we have COFF symbols, create a symbol table now
	////
	////=========================================================================
	//
	//if ( g_pCOFFHeader )	// Did we see a COFF symbols header while looking
	//{						// through the debug directory?
	//	g_pCOFFSymbolTable = new COFFSymbolTable(
	//			pImageBase+ pNTHeader->FileHeader.PointerToSymbolTable,
	//			pNTHeader->FileHeader.NumberOfSymbols );
	//}
	//
	//if ( fShowPDATA )
	//{
	//	bIs64Bit
	//		? DumpRuntimeFunctions( pRoot, pImageBase, pNTHeader64 )
	//		: DumpRuntimeFunctions( pRoot, pImageBase, pNTHeader ); 
	//			
	//	pRoot->printf( "\n" );
	//}
	//
    //if ( fShowRelocations )
    //{
    //    bIs64Bit
	//		? DumpBaseRelocationsSection( pRoot, pImageBase, pNTHeader64 )
	//		: DumpBaseRelocationsSection( pRoot, pImageBase, pNTHeader );
    //    pRoot->printf("\n");
    //} 
    //
	//if ( fShowSymbolTable && g_pMiscDebugInfo )
	//{
	//	DumpMiscDebugInfo( pRoot, g_pMiscDebugInfo );
	//	pRoot->printf( "\n" );
	//}
	//
	//if ( fShowSymbolTable && g_pCVHeader )
	//{
	//	DumpCVDebugInfoRecord( pRoot, g_pCVHeader );
	//	pRoot->printf( "\n" );
	//}
	//
    //if ( fShowSymbolTable && g_pCOFFHeader )
    //{
    //    DumpCOFFHeader( pRoot, g_pCOFFHeader );
    //    pRoot->printf("\n");
    //}
    //
    //if ( fShowLineNumbers && g_pCOFFHeader )
    //{
    //    DumpLineNumbers( pRoot, MakePtr(PIMAGE_LINENUMBER, g_pCOFFHeader,
    //                        g_pCOFFHeader->LvaToFirstLinenumber),
    //                        g_pCOFFHeader->NumberOfLinenumbers);
    //    pRoot->printf("\n");
    //}
    //
    //if ( fShowSymbolTable )
    //{
    //    if ( pNTHeader->FileHeader.NumberOfSymbols 
    //        && pNTHeader->FileHeader.PointerToSymbolTable
	//		&& g_pCOFFSymbolTable )
    //    {
    //        DumpCOFFSymbolTable( pRoot, g_pCOFFSymbolTable );
    //        pRoot->printf("\n");
    //    }
    //}
    //
	//// 04.03.2010 Maks - ¬ Exe не инетересно видеть HexDump, да это еще и долго
	////if ( fShowRawSectionData )
	////{
	////	PIMAGE_SECTION_HEADER pSectionHdr;
	////
	////	pSectionHdr = bIs64Bit ? (PIMAGE_SECTION_HEADER)(pNTHeader64+1) : (PIMAGE_SECTION_HEADER)(pNTHeader+1);
	////
	////    DumpRawSectionData( pRoot, pSectionHdr, dosHeader, pNTHeader->FileHeader.NumberOfSections);
	////}
	//
	//if ( g_pCOFFSymbolTable )
	//	delete g_pCOFFSymbolTable;

	return true;
}


bool DumpFile(LPBYTE pFileData, __int64 nFileSize)
{
	bool lbSucceeded = false;
    HANDLE hFile;
    HANDLE hFileMapping;
    PIMAGE_DOS_HEADER dosHeader;
    
	gpNTHeader32 = NULL;
	gpNTHeader64 = NULL;
	g_bIs64Bit = false;

	g_pCVHeader = 0;
	g_pCOFFHeader = 0;
	g_pCOFFSymbolTable = 0;


	g_FileSize.QuadPart = nFileSize;
    g_pMappedFileBase = pFileData;
    
    dosHeader = (PIMAGE_DOS_HEADER)g_pMappedFileBase;
	PIMAGE_FILE_HEADER pImgFileHdr = (PIMAGE_FILE_HEADER)g_pMappedFileBase;

	if ( dosHeader->e_magic == IMAGE_DOS_SIGNATURE )
	{
		lbSucceeded = DumpExeFile( dosHeader );
	}

    return lbSucceeded;
}





BOOL WINAPI CET_Init(struct CET_Init* pInit)
{
	_ASSERTE(pInit->cbSize >= sizeof(struct CET_Init));
	if (pInit->cbSize < sizeof(struct CET_Init)) {
		pInit->nErrNumber = PGE_OLD_PLUGIN;
		return FALSE;
	}

	ghModule = pInit->hModule;

	pInit->pContext == (LPVOID)1;
	return TRUE;
}

VOID WINAPI CET_Done(struct CET_Init* pInit)
{
}


#define SETERROR(n) if (pLoadPreview) pLoadPreview->nErrNumber = n;


BOOL WINAPI CET_Load(struct CET_LoadInfo* pLoadPreview)
{
	if (!pLoadPreview || *((LPDWORD)pLoadPreview) != sizeof(struct CET_LoadInfo)) {
		_ASSERTE(*((LPDWORD)pLoadPreview) == sizeof(struct CET_LoadInfo));
		SETERROR(PGE_INVALID_VERSION);
		return FALSE;
	}
	
	
	if (!pLoadPreview->pContext != (LPVOID)1) {
		SETERROR(PGE_INVALID_CONTEXT);
		return FALSE;
	}
	

	if (!pLoadPreview->pFileData || !pLoadPreview->nFileSize < 512) {
		SETERROR(PGE_FILE_NOT_FOUND);
		return FALSE;
	}
	
	const BYTE  *pb  = (const BYTE*)pLoadPreview->pFileData;
	if (pb[0] != 'M' || pb[1] != 'Z') {
		SETERROR(PGE_UNSUPPORTEDFORMAT);
		return FALSE;
	}

	
	PEData *pData = (PEData*)CALLOC(sizeof(PEData));
	if (!pImage) {
		SETERROR(PGE_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	pData->nMagic = eGdiStr_Data;
	pLoadPreview->pFileContext = (void*)pData;
	
	TODO("Load version info, and ICON?");
	gpCurData = pData;
	
	BOOL lbRc = FALSE;
	
	if (DumpFile((LPBYTE)pLoadPreview->pFileData, pLoadPreview->nFileSize)) {
		lbRc = TRUE;
	} else {
		pLoadPreview->pFileContext = NULL;
		pData->Close();
	}

	// Done
	gpCurData = NULL;
	return lbRc;
}

VOID WINAPI CET_Free(struct CET_LoadInfo* pLoadPreview)
{
	if (!pLoadPreview || *((LPDWORD)pLoadPreview) != sizeof(struct CET_LoadInfo)) {
		_ASSERTE(*((LPDWORD)pLoadPreview) == sizeof(struct CET_LoadInfo));
		SETERROR(PGE_INVALID_VERSION);
		return;
	}
	if (!pLoadPreview->pFileContext) {
		SETERROR(PGE_INVALID_CONTEXT);
		return;
	}

	if ((*(LPDWORD)pLoadPreview->pFileContext) == eGdiStr_Data) {
		PEData *pData = (PEData*)pLoadPreview->pFileContext;
		pData->Close();
	}
}

VOID WINAPI CET_Cancel(LPVOID pContext)
{
}
