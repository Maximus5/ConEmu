/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
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

#define HIDE_USE_EXCEPTION_INFO
#include "../common/Common.h"
#include "../common/CmdLine.h"
#include "Execute.h"
#include "WObjects.h"
#include <TlHelp32.h>





// Выдранный кусок из будущего GetFileInfo, получаем достоверную информацию о ГУЯХ PE-модуля

// Возвращаем константы IMAGE_SUBSYSTEM_* дабы консоль отличать
// При выходе из процедуры IMAGE_SUBSYTEM_UNKNOWN означает
// "файл не является исполняемым".
// Для DOS-приложений определим еще одно значение флага.

// 17.12.2010 Maks
// Если GetImageSubsystem вернет true - то имеет смысл проверять следующие значения
// IMAGE_SUBSYSTEM_WINDOWS_CUI    -- Win Console (32/64)
// IMAGE_SUBSYSTEM_DOS_EXECUTABLE -- DOS Executable (ImageBits == 16)

//#define IMAGE_SUBSYSTEM_DOS_EXECUTABLE  255

//struct IMAGE_HEADERS
//{
//	DWORD Signature;
//	IMAGE_FILE_HEADER FileHeader;
//	union
//	{
//		IMAGE_OPTIONAL_HEADER32 OptionalHeader32;
//		IMAGE_OPTIONAL_HEADER64 OptionalHeader64;
//	};
//};

bool IsFileBatch(LPCWSTR pszExt)
{
	if (!pszExt || !*pszExt)
		return false;
	LPCWSTR Known[] = {
		// Batches
		L".cmd", L".bat", L".btm"/*take command*/,
		// Other scripts ?
		// L".ps1", L".sh", L".py",
		// End of predefined
		NULL};
	for (INT_PTR i = 0; Known[i]; i++)
	{
		if (lstrcmpi(Known[i], pszExt) == 0)
			return true;
	}
	return false;
}

bool GetImageSubsystem(const wchar_t *FileName,DWORD& ImageSubsystem,DWORD& ImageBits/*16/32/64*/,DWORD& FileAttrs)
{
	bool Result = false;
	ImageSubsystem = IMAGE_SUBSYSTEM_UNKNOWN;
	ImageBits = 0;
	FileAttrs = (DWORD)-1;
	wchar_t* pszExpand = NULL;
	// Пытаться в UNC? Хотя сам CreateProcess UNC не поддерживает, так что смысла пока нет
	HANDLE hModuleFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);

	// Переменные окружения
	if ((hModuleFile == INVALID_HANDLE_VALUE) && FileName && wcschr(FileName, L'%'))
	{
		pszExpand = ExpandEnvStr(FileName);
		if (pszExpand)
		{
			hModuleFile = CreateFile(pszExpand,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
			if (hModuleFile != INVALID_HANDLE_VALUE)
			{
				FileName = pszExpand;
			}
		}
	}

#if 0
	// Если не указан путь к файлу - попробовать найти его в %PATH%
	if (hModuleFile != INVALID_HANDLE_VALUE && FileName && wcschr(FileName, L'\\') == NULL && wcschr(FileName, L'.') != NULL)
	{
		DWORD nErrCode = GetLastError();
		if (nErrCode)
		{
			wchar_t szFind[MAX_PATH], *psz;
			DWORD nLen = SearchPath(NULL, FileName, NULL, countof(szFind), szFind, &psz);
			if (nLen && nLen < countof(szFind))
				hModuleFile = CreateFile(szFind,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
		}
	}
#endif

	if (hModuleFile != INVALID_HANDLE_VALUE)
	{
		BY_HANDLE_FILE_INFORMATION bfi = {0};
		IMAGE_DOS_HEADER DOSHeader;
		DWORD ReadSize;

		if (GetFileInformationByHandle(hModuleFile, &bfi))
			FileAttrs = bfi.dwFileAttributes;

		// Это батник - сразу вернуть IMAGE_SUBSYSTEM_BATCH_FILE
		LPCWSTR pszExt = PointToExt(FileName);
		if (pszExt && IsFileBatch(pszExt))
		{
			CloseHandle(hModuleFile);
			ImageSubsystem = IMAGE_SUBSYSTEM_BATCH_FILE;
			ImageBits = IsWindows64() ? 64 : 32; //-V112
			Result = true;
			goto wrap;
		}

		if (ReadFile(hModuleFile,&DOSHeader,sizeof(DOSHeader),&ReadSize,NULL))
		{
			_ASSERTE(IMAGE_DOS_SIGNATURE==0x5A4D);
			if (DOSHeader.e_magic != IMAGE_DOS_SIGNATURE)
			{
				//const wchar_t *pszExt = wcsrchr(FileName, L'.');

				if (pszExt && lstrcmpiW(pszExt, L".com") == 0)
				{
					ImageSubsystem = IMAGE_SUBSYSTEM_DOS_EXECUTABLE;
					ImageBits = 16;
					Result = true;
				}
			}
			else
			{
				ImageSubsystem = IMAGE_SUBSYSTEM_DOS_EXECUTABLE;
				ImageBits = 16;
				Result = true;

				if (SetFilePointer(hModuleFile,DOSHeader.e_lfanew,NULL,FILE_BEGIN))
				{
					IMAGE_HEADERS PEHeader;

					if (ReadFile(hModuleFile,&PEHeader,sizeof(PEHeader),&ReadSize,NULL))
					{
						_ASSERTE(IMAGE_NT_SIGNATURE==0x00004550);
						if (PEHeader.Signature == IMAGE_NT_SIGNATURE)
						{
							switch(PEHeader.OptionalHeader32.Magic)
							{
								case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
								{
									ImageSubsystem = PEHeader.OptionalHeader32.Subsystem;
									_ASSERTE((ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) || (ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI));
									ImageBits = 32; //-V112
								}
								break;
								case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
								{
									ImageSubsystem = PEHeader.OptionalHeader64.Subsystem;
									_ASSERTE((ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) || (ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI));
									ImageBits = 64;
								}
								break;
								/*default:
									{
										// unknown magic
									}*/
							}
						}
						else if ((WORD)PEHeader.Signature == IMAGE_OS2_SIGNATURE)
						{
							ImageBits = 32; //-V112
							/*
							NE,  хмм...  а как определить что оно ГУЕВОЕ?

							Andrzej Novosiolov <andrzej@se.kiev.ua>
							AN> ориентироваться по флагу "Target operating system" NE-заголовка
							AN> (1 байт по смещению 0x36). Если там Windows (значения 2, 4) - подразумеваем
							AN> GUI, если OS/2 и прочая экзотика (остальные значения) - подразумеваем консоль.
							*/
							BYTE ne_exetyp = reinterpret_cast<PIMAGE_OS2_HEADER>(&PEHeader)->ne_exetyp;

							if (ne_exetyp==2||ne_exetyp==4) //-V112
							{
								ImageSubsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
							}
							else
							{
								ImageSubsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
							}
						}

						/*else
						{
							// unknown signature
						}*/
					}

					/*else
					{
						// обломс вышел с чтением следующего заголовка ;-(
					}*/
				}

				/*else
				{
					// видимо улетели куда нить в трубу, т.к. dos_head.e_lfanew указал
					// слишком в неправдоподное место (например это чистой воды DOS-файл)
				}*/
			}

			/*else
			{
				// это не исполняемый файл - у него нету заголовка MZ, например, NLM-модуль
				// TODO: здесь можно разбирать POSIX нотацию, например "/usr/bin/sh"
			}*/
		}

		/*else
		{
			// ошибка чтения
		}*/
		CloseHandle(hModuleFile);
	}

	/*else
	{
		// ошибка открытия
	}*/
wrap:
	SafeFree(pszExpand);
	return Result;
}

bool GetImageSubsystem(DWORD& ImageSubsystem,DWORD& ImageBits/*16/32/64*/)
{
	HMODULE hModule = GetModuleHandle(NULL);

	ImageSubsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
	#ifdef _WIN64
	ImageBits = 64;
	#else
	ImageBits = 32; //-V112
	#endif

	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)hModule;
	IMAGE_HEADERS* nt_header = NULL;

	_ASSERTE(IMAGE_DOS_SIGNATURE==0x5A4D);
	if (dos_header && dos_header->e_magic == IMAGE_DOS_SIGNATURE /*'ZM'*/)
	{
		nt_header = (IMAGE_HEADERS*)((char*)hModule + dos_header->e_lfanew);

		_ASSERTE(IMAGE_NT_SIGNATURE==0x00004550);
		if (nt_header->Signature != IMAGE_NT_SIGNATURE)
			return false;

		switch(nt_header->OptionalHeader32.Magic)
		{
			case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
			{
				ImageSubsystem = nt_header->OptionalHeader32.Subsystem;
				_ASSERTE((ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) || (ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI));
				_ASSERTE(ImageBits == 32); //-V112
			}
			break;
			case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
			{
				ImageSubsystem = nt_header->OptionalHeader64.Subsystem;
				_ASSERTE((ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) || (ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI));
				_ASSERTE(ImageBits == 64);
			}
			break;
			/*default:
				{
					// unknown magic
				}*/
		}
		return true;
	}

	return false;
}

bool GetImageSubsystem(PROCESS_INFORMATION pi,DWORD& ImageSubsystem,DWORD& ImageBits/*16/32/64*/)
{
	DWORD nErrCode = 0;
	DWORD nFlags = TH32CS_SNAPMODULE;

	ImageBits = 32; //-V112
	ImageSubsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;

	#ifdef _WIN64
		HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
		if (hKernel)
		{
			typedef BOOL (WINAPI* IsWow64Process_t)(HANDLE, PBOOL);
			IsWow64Process_t IsWow64Process_f = (IsWow64Process_t)GetProcAddress(hKernel, "IsWow64Process");

			if (IsWow64Process_f)
			{
				BOOL bWow64 = FALSE;
				if (IsWow64Process_f(pi.hProcess, &bWow64) && !bWow64)
				{
					ImageBits = 64;
				}
				else
				{
					ImageBits = 32;
					nFlags = TH32CS_SNAPMODULE32;
				}
			}
		}
	#endif

	HANDLE hSnap = CreateToolhelp32Snapshot(nFlags, pi.dwProcessId);
	if (hSnap == INVALID_HANDLE_VALUE)
	{
		nErrCode = GetLastError();
		return false;
	}
	IMAGE_DOS_HEADER dos;
	IMAGE_HEADERS hdr;
	SIZE_T hdrReadSize;
	MODULEENTRY32 mi = {sizeof(MODULEENTRY32)};
	BOOL lbModule = Module32First(hSnap, &mi);
	CloseHandle(hSnap);
	if (!lbModule)
		return false;

	// Теперь можно считать данные процесса
	if (!ReadProcessMemory(pi.hProcess, mi.modBaseAddr, &dos, sizeof(dos), &hdrReadSize))
		nErrCode = -3;
	else if (dos.e_magic != IMAGE_DOS_SIGNATURE)
		nErrCode = -4; // некорректная сигнатура - должно быть 'MZ'
	else if (!ReadProcessMemory(pi.hProcess, mi.modBaseAddr+dos.e_lfanew, &hdr, sizeof(hdr), &hdrReadSize))
		nErrCode = -5;
	else if (hdr.Signature != IMAGE_NT_SIGNATURE)
		nErrCode = -6;
	else if (hdr.OptionalHeader32.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC
	        &&  hdr.OptionalHeader64.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
		nErrCode = -7;
	else
	{
		nErrCode = 0;

		switch (hdr.OptionalHeader32.Magic)
		{
			case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
			{
				_ASSERTE(ImageBits == 32); //-V112
				ImageBits = 32; //-V112
				ImageSubsystem = hdr.OptionalHeader32.Subsystem;
				_ASSERTE((ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) || (ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI));
			}
			break;
			case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
			{
				_ASSERTE(ImageBits == 64);
				ImageBits = 64;
				ImageSubsystem = hdr.OptionalHeader64.Subsystem;
				_ASSERTE((ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI) || (ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI));
			}
			break;
			default:
			{
				nErrCode = -8;
			}
		}
	}

	return (nErrCode == 0);
}


/* ******************************************* */
/* PE File Info (kernel32.dll -> LoadLibraryW) */
/* ******************************************* */
struct IMAGE_MAPPING
{
	union
	{
		LPBYTE ptrBegin; //-V117
		PIMAGE_DOS_HEADER pDos; //-V117
	};
	LPBYTE ptrEnd;
	IMAGE_HEADERS* pHdr;
};
#ifdef _DEBUG
static bool ValidateMemory(LPVOID ptr, DWORD_PTR nSize, IMAGE_MAPPING* pImg)
{
	if ((ptr == NULL) || (((LPBYTE)ptr) < pImg->ptrBegin))
		return false;

	if ((((LPBYTE)ptr) + nSize) >= pImg->ptrEnd)
		return false;

	return true;
}
#endif

//================================================================================
//
// Given an RVA, look up the section header that encloses it and return a
// pointer to its IMAGE_SECTION_HEADER
//
PIMAGE_SECTION_HEADER GetEnclosingSectionHeader(DWORD rva, IMAGE_MAPPING* pImg)
{
	// IMAGE_FIRST_SECTION doesn't need 32/64 versions since the file header is the same either way.
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pImg->pHdr); //-V220
	unsigned i;

	for(i = 0; i < pImg->pHdr->FileHeader.NumberOfSections; i++, section++)
	{
		// This 3 line idiocy is because Watcom's linker actually sets the
		// Misc.VirtualSize field to 0.  (!!! - Retards....!!!)
		DWORD size = section->Misc.VirtualSize;

		if (0 == size)
			size = section->SizeOfRawData;

		// Is the RVA within this section?
		if ((rva >= section->VirtualAddress) &&
		        (rva < (section->VirtualAddress + size)))
			return section;
	}

	return NULL;
}

LPVOID GetPtrFromRVA(DWORD rva, IMAGE_MAPPING* pImg)
{
	if (!pImg || !pImg->ptrBegin || !pImg->pHdr)
	{
		_ASSERTE(pImg!=NULL && pImg->ptrBegin!=NULL && pImg->pHdr!=NULL);
		return NULL;
	}

	PIMAGE_SECTION_HEADER pSectionHdr;
	INT delta;
	pSectionHdr = GetEnclosingSectionHeader(rva, pImg);

	if (!pSectionHdr)
		return NULL;

	delta = (INT)(pSectionHdr->VirtualAddress - pSectionHdr->PointerToRawData);
	return (LPVOID)(pImg->ptrBegin + rva - delta);
}

//int ParseExportsSection(IMAGE_MAPPING* pImg)
//{
//	PIMAGE_EXPORT_DIRECTORY pExportDir;
//	//PIMAGE_SECTION_HEADER header;
//	//INT delta;
//	//PSTR pszFilename;
//	DWORD i;
//	PDWORD pdwFunctions;
//	PWORD pwOrdinals;
//	DWORD *pszFuncNames;
//	DWORD exportsStartRVA; //, exportsEndRVA;
//	LPCSTR pszFuncName;
//
//	//exportsStartRVA = GetImgDirEntryRVA(pNTHeader,IMAGE_DIRECTORY_ENTRY_EXPORT);
//	//exportsEndRVA = exportsStartRVA +
//	//	GetImgDirEntrySize(pNTHeader, IMAGE_DIRECTORY_ENTRY_EXPORT);
//	if (pImg->pHdr->OptionalHeader64.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
//	{
//		exportsStartRVA = pImg->pHdr->OptionalHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
//		//exportDirSize = hdr.OptionalHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
//		//pExportDirAddr = GetPtrFromRVA(exportsStartRVA, (PIMAGE_NT_HEADERS64)&hdr, mi.modBaseAddr);
//	}
//	else
//	{
//		exportsStartRVA = pImg->pHdr->OptionalHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
//		//exportDirSize = hdr.OptionalHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
//		//pExportDirAddr = GetPtrFromRVA(exportsStartRVA, (PIMAGE_NT_HEADERS32)&hdr, mi.modBaseAddr);
//	}
//
//	// Get the IMAGE_SECTION_HEADER that contains the exports.  This is
//	// usually the .edata section, but doesn't have to be.
//	//header = GetEnclosingSectionHeader( exportsStartRVA, pNTHeader );
//	//if ( !header )
//	//	return -201;
//	//delta = (INT)(header->VirtualAddress - header->PointerToRawData);
//	pExportDir = (PIMAGE_EXPORT_DIRECTORY)GetPtrFromRVA(exportsStartRVA, pImg);
//
//	if (!pExportDir || !ValidateMemory(pExportDir, sizeof(IMAGE_EXPORT_DIRECTORY), pImg))
//		return -201;
//
//	//pszFilename = (PSTR)GetPtrFromRVA( pExportDir->Name, pNTHeader, pImageBase );
//	pdwFunctions =	(PDWORD)GetPtrFromRVA(pExportDir->AddressOfFunctions, pImg);
//	pwOrdinals =	(PWORD) GetPtrFromRVA(pExportDir->AddressOfNameOrdinals, pImg);
//	pszFuncNames =	(DWORD*)GetPtrFromRVA(pExportDir->AddressOfNames, pImg);
//
//	if (!pdwFunctions || !pwOrdinals || !pszFuncNames)
//		return -202;
//
//	for(i=0; i < pExportDir->NumberOfFunctions; i++, pdwFunctions++)
//	{
//		DWORD entryPointRVA = *pdwFunctions;
//
//		if (entryPointRVA == 0)      // Skip over gaps in exported function
//			continue;               // ordinals (the entrypoint is 0 for
//
//		// these functions).
//
//		// See if this function has an associated name exported for it.
//		for(unsigned j=0; j < pExportDir->NumberOfNames; j++)
//		{
//			if (pwOrdinals[j] == i)
//			{
//				pszFuncName = (LPCSTR)GetPtrFromRVA(pszFuncNames[j], pImg);
//
//				if (pszFuncName)
//				{
//					if (strcmp(pszFuncName, "LoadLibraryW"))
//					{
//						// Нашли
//						return entryPointRVA;
//					}
//				}
//			}
//		}
//	}
//
//	return -203;
//}

//static int FindLoadLibrary(LPCWSTR asKernel32)
//{
//	int nLoadLibraryOffset = 0;
//	MWow64Disable wow; wow.Disable(); // Требуется в Win64 системах. Если не нужно - ничего не делает.
//	HANDLE hMapping = NULL, hKernel = NULL;
//	LPBYTE ptrMapping = NULL;
//	LARGE_INTEGER nFileSize;
//	hKernel = CreateFile(asKernel32, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
//
//	if (!hKernel || (hKernel == INVALID_HANDLE_VALUE))
//		nLoadLibraryOffset = -101;
//	else if (!GetFileSizeEx(hKernel, &nFileSize) || nFileSize.HighPart)
//		nLoadLibraryOffset = -102;
//	else if (nFileSize.LowPart < (sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_HEADERS)))
//		nLoadLibraryOffset = -103;
//	else if (!(hMapping = CreateFileMapping(hKernel, NULL, PAGE_READONLY, 0,0, NULL)) || (hMapping == INVALID_HANDLE_VALUE))
//		nLoadLibraryOffset = -104;
//	else if (!(ptrMapping = (LPBYTE)MapViewOfFile(hMapping, FILE_MAP_READ, 0,0,0)))
//		nLoadLibraryOffset = -105;
//	else // Поехали
//	{
//		IMAGE_MAPPING img;
//		img.pDos = (PIMAGE_DOS_HEADER)ptrMapping;
//		img.pHdr = (IMAGE_HEADERS*)(ptrMapping + img.pDos->e_lfanew);
//		img.ptrEnd = (ptrMapping + nFileSize.LowPart);
//
//		if (img.pDos->e_magic != IMAGE_DOS_SIGNATURE)
//			nLoadLibraryOffset = -110; // некорректная сигнатура - должно быть 'MZ'
//		else if (!ValidateMemory(img.pHdr, sizeof(*img.pHdr), &img))
//			nLoadLibraryOffset = -111;
//		else if (img.pHdr->Signature != IMAGE_NT_SIGNATURE)
//			nLoadLibraryOffset = -112;
//		else if (img.pHdr->OptionalHeader32.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC
//		        &&  img.pHdr->OptionalHeader64.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
//			nLoadLibraryOffset = -113;
//		else // OK, файл первичную валидацию прошел
//		{
//			nLoadLibraryOffset = ParseExportsSection(&img);
//		}
//	}
//
//	// Закрываем дескрипторы
//	if (ptrMapping)
//		UnmapViewOfFile(ptrMapping);
//
//	if (hMapping && (hMapping != INVALID_HANDLE_VALUE))
//		CloseHandle(hMapping);
//
//	if (hKernel && (hKernel != INVALID_HANDLE_VALUE))
//		CloseHandle(hKernel);
//
//	// Found result
//	return nLoadLibraryOffset;
//}

//// Определить адрес процедуры LoadLibraryW для запущенного процесса
//int FindKernelAddress(HANDLE ahProcess, DWORD anPID, DWORD* pLoadLibrary)
//{
//	int iRc = -100;
//	*pLoadLibrary = NULL;
//	int nBits = 0;
//	SIZE_T hdrReadSize;
//	IMAGE_DOS_HEADER dos;
//	IMAGE_HEADERS hdr;
//	MODULEENTRY32 mi = {sizeof(MODULEENTRY32)};
//	// Must be TH32CS_SNAPMODULE32 for spy 32bit from 64bit process
//	// Сначала пробуем как Native, если процесс окажется другой битности - переоткроем snapshot
//	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, anPID);
//
//	if (!hSnap || hSnap == INVALID_HANDLE_VALUE)
//		iRc = -1;
//	else
//	{
//		WARNING("По всей видимости, 32-битный процесс не может получить информацию о 64-битном!");
//
//		// Теперь нужно определить битность процесса
//		if (!Module32First(hSnap, &mi))
//			iRc = -2;
//		else if (!ReadProcessMemory(ahProcess, mi.modBaseAddr, &dos, sizeof(dos), &hdrReadSize))
//			iRc = -3;
//		else if (dos.e_magic != IMAGE_DOS_SIGNATURE)
//			iRc = -4; // некорректная сигнатура - должно быть 'MZ'
//		else if (!ReadProcessMemory(ahProcess, mi.modBaseAddr+dos.e_lfanew, &hdr, sizeof(hdr), &hdrReadSize))
//			iRc = -5;
//		else if (hdr.Signature != IMAGE_NT_SIGNATURE)
//			iRc = -6;
//		else if (hdr.OptionalHeader32.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC
//		        &&  hdr.OptionalHeader64.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
//			iRc = -7;
//		else
//		{
//			TODO("Потенциально, еще можно попробовать обработать IMAGE_OS2_SIGNATURE?");
//			nBits = (hdr.OptionalHeader32.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) ? 32 : 64;
//#ifdef WIN64
//
//			// Если ahProcess - 64 бита, но нужно переоткрыть snapshot с флагом TH32CS_SNAPMODULE32
//			// В принципе, мы сюда попадать не должны, т.к. ConEmuC.exe - 32битный.
//			if (nBits == 32)
//			{
//				CloseHandle(hSnap);
//				hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE32, anPID);
//
//				if (!hSnap || hSnap == INVALID_HANDLE_VALUE)
//				{
//					iRc = -8;
//					hSnap = NULL;
//				}
//				else if (!Module32First(hSnap, &mi))
//				{
//					iRc = -9;
//					CloseHandle(hSnap);
//					hSnap = NULL;
//				}
//			}
//
//#endif
//
//			if (hSnap != NULL)
//			{
//				iRc = (hdr.OptionalHeader32.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) ? -20 : -21;
//
//				do
//				{
//					if (lstrcmpi(mi.szModule, L"kernel32.dll") == 0)
//					{
//						//if (!ReadProcessMemory(ahProcess, mi.modBaseAddr, &dos, sizeof(dos), &hdrReadSize))
//						//	iRc = -23;
//						//else if (dos.e_magic != IMAGE_DOS_SIGNATURE)
//						//	iRc = -24; // некорректная сигнатура - должно быть 'MZ'
//						//else if (!ReadProcessMemory(ahProcess, mi.modBaseAddr+dos.e_lfanew, &hdr, sizeof(hdr), &hdrReadSize))
//						//	iRc = -25;
//						//else if (hdr.Signature != IMAGE_NT_SIGNATURE)
//						//	iRc = -26;
//						//else if (hdr.OptionalHeader32.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC
//						//	&&  hdr.OptionalHeader64.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
//						//	iRc = -27;
//						//else
//						iRc = 0;
//						break;
//					}
//				}
//				while(Module32Next(hSnap, &mi));
//			}
//		}
//
//		if (hSnap)
//			CloseHandle(hSnap);
//	}
//
//	// Если kernel32.dll нашли в обрабатываемом процессе
//	if (iRc == 0 && nBits)
//	{
//		BOOL lbNeedLoad = FALSE;
//		static DWORD nLoadLibraryW32 = 0;
//		static DWORD nLoadLibraryW64 = 0;
//		DWORD_PTR ptr = 0;
//		lbNeedLoad = (nBits == 64) ? (nLoadLibraryW32 == 0) : (nLoadLibraryW64 == 0);
//
//		if (lbNeedLoad)
//		{
//			iRc = FindLoadLibrary(mi.szExePath);
//
//			if (iRc > 0)
//			{
//				if (nBits == 64)
//					nLoadLibraryW64 = iRc;
//				else
//					nLoadLibraryW32 = iRc;
//
//				lbNeedLoad = FALSE; // OK
//			}
//
//			//LPVOID pExportDirAddr;
//			//DWORD exportsStartRVA;
//			//DWORD exportDirSize;
//			//if (nBits == 64)
//			//{
//			//	exportsStartRVA = hdr.OptionalHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
//			//	exportDirSize = hdr.OptionalHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
//			//	pExportDirAddr = GetPtrFromRVA(exportsStartRVA, (PIMAGE_NT_HEADERS64)&hdr, mi.modBaseAddr);
//			//}
//			//else
//			//{
//			//	exportsStartRVA = hdr.OptionalHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
//			//	exportDirSize = hdr.OptionalHeader32.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
//			//	pExportDirAddr = GetPtrFromRVA(exportsStartRVA, (PIMAGE_NT_HEADERS32)&hdr, mi.modBaseAddr);
//			//}
//			//if (!pExportDirAddr)
//			//	iRc = -30;
//			//else if (!ReadProcessMemory(ahProcess, pExportDirAddr,
//		}
//
//		if (lbNeedLoad)
//		{
//			// Не удалось
//			if (iRc == 0)
//				iRc = -40;
//		}
//		else
//		{
//			ptr = ((DWORD_PTR)mi.modBaseAddr) + ((nBits == 64) ? nLoadLibraryW64 : nLoadLibraryW32);
//
//			if (ptr != (DWORD)ptr)
//			{
//				// BaseAddress даже для 64-битного Kernel32 мал, и доступен в 32-битных процессах,
//				// но тем не менее проверяем, и если он "ушел" - то возвращаем ошибку.
//				iRc = -41;
//			}
//			else
//				*pLoadLibrary = (DWORD)ptr;
//		}
//	}
//
//	return iRc;
//}

bool FindImageSubsystem(const wchar_t *Module, /*wchar_t* pstrDest,*/ DWORD& ImageSubsystem, DWORD& ImageBits, LPDWORD pFileAttrs /*= NULL*/)
{
	if (!Module || !*Module)
		return false;

	bool Result = false;
	//ImageSubsystem = IMAGE_SUBSYSTEM_UNKNOWN;

	// Исключения нас не интересуют - команда уже сформирована и отдана в CreateProcess!
	//// нулевой проход - смотрим исключения
	//// Берем "исключения" из реестра, которые должны исполняться директом,
	//// например, некоторые внутренние команды ком. процессора.
	//string strExcludeCmds;
	//GetRegKey(strSystemExecutor,L"ExcludeCmds",strExcludeCmds,L"");
	//UserDefinedList ExcludeCmdsList;
	//ExcludeCmdsList.Set(strExcludeCmds);
	//while (!ExcludeCmdsList.IsEmpty())
	//{
	//	if (!StrCmpI(Module,ExcludeCmdsList.GetNext()))
	//	{
	//		ImageSubsystem=IMAGE_SUBSYSTEM_WINDOWS_CUI;
	//		Result=true;
	//		break;
	//	}
	//}

	//string strFullName=Module;
	LPCWSTR ModuleExt = PointToExt(Module);
	//wchar_t *strPathExt/*[32767]*/ = NULL; //(L".COM;.EXE;.BAT;.CMD;.VBS;.JS;.WSH");
	wchar_t *strPathEnv/*[32767]*/ = NULL;
	wchar_t *strExpand/*[32767]*/ = NULL;  int cchstrExpand = 32767;
	wchar_t *strTmpName/*[32767]*/ = NULL; int cchstrTmpName = 32767;
	wchar_t *pszFilePart = NULL;
	//DWORD nPathExtLen = 0;
	LPCWSTR pszPathExtEnd = NULL;
	LPWSTR Ext = NULL;
	LPWSTR pszExtCur;
	DWORD FileAttrs = 0;

	typedef LONG (WINAPI *RegOpenKeyExW_t)(HKEY hKey, LPCTSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
	RegOpenKeyExW_t _RegOpenKeyEx = NULL;
	typedef LONG (WINAPI *RegQueryValueExW_t)(HKEY hKey, LPCTSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
	RegQueryValueExW_t _RegQueryValueEx = NULL;
	typedef LONG (WINAPI *RegCloseKey_t)(HKEY hKey);
	RegCloseKey_t _RegCloseKey = NULL;
	HMODULE hAdvApi = NULL;

	// Actually, we need only to check, if the Module is executable (PE) or not
	// Plus, if it is IMAGE_SUBSYSTEM_BATCH_FILE
	// All other options are not interesting to us
	wchar_t strPathExt[] = L".COM;.EXE;.BAT;.CMD;.BTM;"; // Must be Zero-zero-terminated
	//strPathExt = GetEnvVar(L"PATHEXT");
	//if (!strPathExt || !*strPathExt)
	//{
	//	SafeFree(strPathExt);
	//	strPathExt = lstrdup(L".COM;.EXE;.BAT;.CMD;.VBS;.JS;.WSH;"); // Must becode Zero-zero-terminated
	//	if (!strPathExt)
	//		goto wrap;
	//}
	//#ifdef _DEBUG
	//else
	//{
	//	int iLen = lstrlen(strPathExt);
	//	_ASSERTE(strPathExt[iLen] == 0 && strPathExt[iLen+1] == 0); // Must be Zero-zero-terminated
	//}
	//#endif
	pszPathExtEnd = strPathExt+lstrlen(strPathExt);
	// Разбить на токены
	Ext = wcschr(strPathExt, L';');
	while (Ext)
	{
		*Ext = 0;
		Ext = wcschr(Ext+1, L';');
	}

	TODO("Проверить на превышение длин строк");

	// первый проход - в текущем каталоге
	pszExtCur = strPathExt;
	// Указано расширение?
	if (ModuleExt)
	{
		// А оно вообще указано в списке допустимых?
		bool bBatchExt = IsFileBatch(ModuleExt);
		bool bExecutable = bBatchExt;
		if (!bExecutable)
		{
			while (pszExtCur < pszPathExtEnd)
			{
				if (lstrcmpi(pszExtCur, ModuleExt) == 0)
				{
					bExecutable = true;
					break;
				}
				pszExtCur = pszExtCur + lstrlen(pszExtCur)+1;
			}
		}

		if (bBatchExt && !pFileAttrs)
		{
			ImageSubsystem = IMAGE_SUBSYSTEM_BATCH_FILE;
			ImageBits = IsWindows64() ? 64 : 32; //-V112
			Result = true;
			goto wrap;
		}

		if (!bExecutable)
		{
			// Если расширение не указано в списке "исполняемых", то и проверять нечего
			goto wrap;
		}

		// Check for executable?
		if (GetImageSubsystem(Module, ImageSubsystem, ImageBits/*16/32/64*/, FileAttrs))
		{
			Result = true;
			goto wrap;
		}
	}

	strTmpName = (wchar_t*)malloc(cchstrTmpName*sizeof(wchar_t)); *strTmpName = 0;

	while (!ModuleExt && (pszExtCur < pszPathExtEnd))
	{
		Ext = pszExtCur;
		pszExtCur = pszExtCur + lstrlen(pszExtCur)+1;

		_wcscpyn_c(strTmpName, cchstrTmpName, Module, cchstrTmpName); //-V501

		if (!ModuleExt)
		{
			if (!*Ext)
				continue;
			_wcscatn_c(strTmpName, cchstrTmpName, Ext, cchstrTmpName);
		}

		if (GetImageSubsystem(strTmpName, ImageSubsystem, ImageBits/*16/32/64*/, FileAttrs))
		{
			Result = true;
			goto wrap;
		}

		_ASSERTE(!ModuleExt)
	}

	// второй проход - по правилам SearchPath

	// поиск по переменной PATH
	if ((strPathEnv = GetEnvVar(L"PATH")))
	{
		LPWSTR pszPathEnvEnd = strPathEnv + lstrlen(strPathEnv);

		LPWSTR pszPathCur = strPathEnv;
		while (pszPathCur && (pszPathCur < pszPathEnvEnd))
		{
			LPWSTR Path = pszPathCur;
			LPWSTR pszPathNext = wcschr(pszPathCur, L';');
			if (pszPathNext)
			{
				*pszPathNext = 0;
				pszPathCur = pszPathNext+1;
			}
			else
			{
				pszPathCur = pszPathEnvEnd;
			}
			if (!*Path)
				continue;

			pszExtCur = strPathExt;
			while (pszExtCur < pszPathExtEnd)
			{
				Ext = pszExtCur;
				pszExtCur = pszExtCur + lstrlen(pszExtCur)+1;
				if (!*Ext)
					continue;

				if (SearchPath(Path, Module, Ext, cchstrTmpName, strTmpName, &pszFilePart))
				{
					if (GetImageSubsystem(strTmpName, ImageSubsystem, ImageBits, FileAttrs))
					{
						Result = true;
						goto wrap;
					}
				}
			}
		}
	}

	pszExtCur = strPathExt;
	while (pszExtCur < pszPathExtEnd)
	{
		Ext = pszExtCur;
		pszExtCur = pszExtCur + lstrlen(pszExtCur)+1;
		if (!*Ext)
			continue;

		if (SearchPath(NULL, Module, Ext, cchstrTmpName, strTmpName, &pszFilePart))
		{
			if (GetImageSubsystem(strTmpName, ImageSubsystem, ImageBits, FileAttrs))
			{
				Result = true;
				goto wrap;
			}
		}
	}

	// третий проход - лезем в реестр в "App Paths"
	if (!wcschr(Module, L'\\'))
	{
		hAdvApi = LoadLibrary(L"AdvApi32.dll");
		if (!hAdvApi)
			goto wrap;
		_RegOpenKeyEx = (RegOpenKeyExW_t)GetProcAddress(hAdvApi, "RegOpenKeyExW");
		_RegQueryValueEx = (RegQueryValueExW_t)GetProcAddress(hAdvApi, "RegQueryValueExW");
		_RegCloseKey = (RegCloseKey_t)GetProcAddress(hAdvApi, "RegCloseKey");
		if (!_RegOpenKeyEx || !_RegQueryValueEx || !_RegCloseKey)
			goto wrap;

		LPCWSTR RegPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\";
		// В строке Module заменить исполняемый модуль на полный путь, который
		// берется из SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths
		// Сначала смотрим в HKCU, затем - в HKLM
		HKEY RootFindKey[] = {HKEY_CURRENT_USER,HKEY_LOCAL_MACHINE,HKEY_LOCAL_MACHINE};

		strExpand = (wchar_t*)malloc(cchstrExpand*sizeof(wchar_t)); *strExpand = 0;

		BOOL lbAddExt = FALSE;
		pszExtCur = strPathExt;
		while (pszExtCur < pszPathExtEnd)
		{
			if (!lbAddExt)
			{
				Ext = NULL;
				lbAddExt = TRUE;
			}
			else
			{
				Ext = pszExtCur;
				pszExtCur = pszExtCur + lstrlen(pszExtCur)+1;
				if (!*Ext)
					continue;
			}

			_wcscpy_c(strTmpName, cchstrTmpName, RegPath);
			_wcscatn_c(strTmpName, cchstrTmpName, Module, cchstrTmpName);
			if (Ext)
				_wcscatn_c(strTmpName, cchstrTmpName, Ext, cchstrTmpName);

			DWORD samDesired = KEY_QUERY_VALUE;
			DWORD RedirectionFlag = 0;
			// App Paths key is shared in Windows 7 and above
			static int sIsWindows7 = 0;
			if (sIsWindows7 == 0)
			{
				_ASSERTE(_WIN32_WINNT_WIN7==0x601);
				OSVERSIONINFOEXW osvi = {sizeof(osvi), HIBYTE(_WIN32_WINNT_WIN7), LOBYTE(_WIN32_WINNT_WIN7)};
				DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);
				sIsWindows7 = VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask) ? 1 : -1;
			}
			if (sIsWindows7 != 1)
			{
				#ifdef _WIN64
				RedirectionFlag = KEY_WOW64_32KEY;
				#else
				RedirectionFlag = IsWindows64() ? KEY_WOW64_64KEY : 0;
				#endif
			}
			for (size_t i = 0; i < countof(RootFindKey); i++)
			{
				if (i == (countof(RootFindKey)-1))
				{
					if (RedirectionFlag)
						samDesired |= RedirectionFlag;
					else
						break;
				}
				HKEY hKey;
				if (_RegOpenKeyEx(RootFindKey[i], strTmpName, 0, samDesired, &hKey) == ERROR_SUCCESS)
				{
					DWORD nType = 0, nSize = sizeof(strTmpName)-2;
					int RegResult = _RegQueryValueEx(hKey, L"", NULL, &nType, (LPBYTE)strTmpName, &nSize);
					_RegCloseKey(hKey);

					if ((RegResult == ERROR_SUCCESS) && (nType == REG_SZ || nType == REG_EXPAND_SZ || nType == REG_MULTI_SZ))
					{
						strTmpName[(nSize >> 1)+1] = 0;
						if (!ExpandEnvironmentStrings(strTmpName, strExpand, cchstrExpand))
							_wcscpy_c(strExpand, cchstrExpand, strTmpName);
						if (GetImageSubsystem(Unquote(strExpand), ImageSubsystem, ImageBits, FileAttrs))
						{
							Result = true;
							goto wrap;
						}
					}
				}
			}
		}
	}

wrap:
	SafeFree(strPathEnv);
	SafeFree(strExpand);
	SafeFree(strTmpName);
	if (hAdvApi)
		FreeLibrary(hAdvApi);
	if (pFileAttrs)
		*pFileAttrs = FileAttrs;
	return Result;
}
