
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
Copyright (c) 2014-present Maximus5
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
#include "Common.h"
#include "CmdLine.h"
#include "EnvVar.h"
#include "execute.h"
#include "WObjects.h"
#include "MModule.h"
#include "MToolHelp.h"


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

namespace {
struct IMAGE_HEADERS
{
	DWORD Signature;
	// ReSharper disable once CppDeclaratorNeverUsed
	IMAGE_FILE_HEADER FileHeader;
	union
	{
		IMAGE_OPTIONAL_HEADER32 OptionalHeader32;
		IMAGE_OPTIONAL_HEADER64 OptionalHeader64;
	};
};
}

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
		nullptr};
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
	FileAttrs = static_cast<DWORD>(-1);
	CEStr pszExpand;
	// Пытаться в UNC? Хотя сам CreateProcess UNC не поддерживает, так что смысла пока нет
	HANDLE hModuleFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);

	// Переменные окружения
	if ((hModuleFile == INVALID_HANDLE_VALUE) && FileName && wcschr(FileName, L'%'))
	{
		pszExpand = ExpandEnvStr(FileName);
		if (pszExpand)
		{
			hModuleFile = CreateFile(pszExpand,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
			if (hModuleFile != INVALID_HANDLE_VALUE)
			{
				FileName = pszExpand;
			}
		}
	}

#if 0
	// Если не указан путь к файлу - попробовать найти его в %PATH%
	if (hModuleFile != INVALID_HANDLE_VALUE && FileName && wcschr(FileName, L'\\') == nullptr && wcschr(FileName, L'.') != nullptr)
	{
		DWORD nErrCode = GetLastError();
		if (nErrCode)
		{
			wchar_t szFind[MAX_PATH], *psz;
			DWORD nLen = SearchPath(nullptr, FileName, nullptr, countof(szFind), szFind, &psz);
			if (nLen && nLen < countof(szFind))
				hModuleFile = CreateFile(szFind,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
		}
	}
#endif

	if (hModuleFile != INVALID_HANDLE_VALUE)
	{
		BY_HANDLE_FILE_INFORMATION bfi = {};
		IMAGE_DOS_HEADER DOSHeader{};
		DWORD ReadSize = 0;

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

		if (ReadFile(hModuleFile,&DOSHeader,sizeof(DOSHeader),&ReadSize,nullptr))
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

				if (SetFilePointer(hModuleFile,DOSHeader.e_lfanew,nullptr,FILE_BEGIN))
				{
					IMAGE_HEADERS PEHeader{};

					if (ReadFile(hModuleFile,&PEHeader,sizeof(PEHeader),&ReadSize,nullptr))
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
						else if (static_cast<WORD>(PEHeader.Signature) == IMAGE_OS2_SIGNATURE)
						{
							ImageBits = 32; //-V112
							/*
							NE,  хмм...  а как определить что оно ГУЕВОЕ?

							Andrzej Novosiolov <andrzej@se.kiev.ua>
							AN> ориентироваться по флагу "Target operating system" NE-заголовка
							AN> (1 байт по смещению 0x36). Если там Windows (значения 2, 4) - подразумеваем
							AN> GUI, если OS/2 и прочая экзотика (остальные значения) - подразумеваем консоль.
							*/
							const BYTE exeType = reinterpret_cast<PIMAGE_OS2_HEADER>(&PEHeader)->ne_exetyp;

							if (exeType == 2 || exeType == 4) //-V112
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
	return Result;
}

bool GetImageSubsystem(DWORD& ImageSubsystem, DWORD& ImageBits/*16/32/64*/)
{
	auto* hModule = GetModuleHandle(nullptr);

	ImageSubsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
	#ifdef _WIN64
	ImageBits = 64;
	#else
	ImageBits = 32; //-V112
	#endif

	IMAGE_DOS_HEADER* dos_header = reinterpret_cast<IMAGE_DOS_HEADER*>(hModule);
	IMAGE_HEADERS* nt_header = nullptr;

	_ASSERTE(IMAGE_DOS_SIGNATURE==0x5A4D);
	if (dos_header && dos_header->e_magic == IMAGE_DOS_SIGNATURE /*'ZM'*/)
	{
		nt_header = reinterpret_cast<IMAGE_HEADERS*>(reinterpret_cast<char*>(hModule) + dos_header->e_lfanew);

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

bool GetImageSubsystem(PROCESS_INFORMATION pi, DWORD& ImageSubsystem, DWORD& ImageBits/*16/32/64*/)
{
	DWORD nErrCode = 0;
	bool snapModule32 = false;

	ImageBits = 32; //-V112
	ImageSubsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;

	#ifdef _WIN64
	MModule hKernel(GetModuleHandle(L"kernel32.dll"));
	if (hKernel.IsValid())
	{
		BOOL(WINAPI * IsWow64Process_f)(HANDLE, PBOOL) = nullptr;
		if (hKernel.GetProcAddress("IsWow64Process", IsWow64Process_f))
		{
			BOOL bWow64 = FALSE;
			if (IsWow64Process_f(pi.hProcess, &bWow64) && !bWow64)
			{
				ImageBits = 64;
			}
			else
			{
				ImageBits = 32;
				snapModule32 = true;
			}
		}
	}
	#endif

	MToolHelpModule hSnap(pi.dwProcessId, snapModule32);
	if (!hSnap.Open())
	{
		nErrCode = GetLastError();
		return false;
	}
	IMAGE_DOS_HEADER dos{};
	IMAGE_HEADERS hdr{};
	SIZE_T hdrReadSize = 0;
	MODULEENTRY32W mi = {};
	if (!hSnap.Next(mi) || !mi.modBaseAddr)
		return false;
	hSnap.Close();

	// Теперь можно считать данные процесса
	if (!ReadProcessMemory(pi.hProcess, mi.modBaseAddr, &dos, sizeof(dos), &hdrReadSize))
	{
		nErrCode = -3;
	}
	else if (dos.e_magic != IMAGE_DOS_SIGNATURE)
	{
		nErrCode = -4; // wrong signature - should be 'MZ'
	}
	else if (!ReadProcessMemory(pi.hProcess, mi.modBaseAddr + dos.e_lfanew, &hdr, sizeof(hdr), &hdrReadSize))
	{
		nErrCode = -5;
	}
	else if (hdr.Signature != IMAGE_NT_SIGNATURE)
	{
		nErrCode = -6;
	}
	else if (hdr.OptionalHeader32.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC
		&& hdr.OptionalHeader64.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		nErrCode = -7;
	}
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


bool FindImageSubsystem(const wchar_t *Module, DWORD& ImageSubsystem, DWORD& ImageBits, LPDWORD pFileAttrs /*= nullptr*/)
{
	#if CE_UNIT_TEST==1
	{
		bool mockResult = false;
		extern bool FindImageSubsystemMock(const wchar_t *module, DWORD& imageSubsystem, DWORD& imageBits, LPDWORD fileAttrs, bool& result);
		if (FindImageSubsystemMock(Module, ImageSubsystem, ImageBits, pFileAttrs, mockResult))
		{
			return mockResult;
		}
	}
	#endif

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
	const wchar_t* ModuleExt = PointToExt(Module);
	//wchar_t *strPathExt/*[MAX_WIDE_PATH_LENGTH]*/ = nullptr; //(L".COM;.EXE;.BAT;.CMD;.VBS;.JS;.WSH");
	CEStr strPathEnv/*[MAX_WIDE_PATH_LENGTH]*/;
	CEStr strTmpName/*[MAX_WIDE_PATH_LENGTH]*/;
	const int cchStrTmpName = MAX_WIDE_PATH_LENGTH;
	wchar_t *pszFilePart = nullptr;
	//DWORD nPathExtLen = 0;
	LPCWSTR pszPathExtEnd = nullptr;
	const wchar_t* Ext = nullptr;
	const wchar_t* pszExtCur = nullptr;
	DWORD FileAttrs = 0;

	typedef LONG (WINAPI *RegOpenKeyExW_t)(HKEY hKey, LPCTSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
	RegOpenKeyExW_t _RegOpenKeyEx = nullptr;
	typedef LONG (WINAPI *RegQueryValueExW_t)(HKEY hKey, LPCTSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
	RegQueryValueExW_t _RegQueryValueEx = nullptr;
	typedef LONG (WINAPI *RegCloseKey_t)(HKEY hKey);
	RegCloseKey_t _RegCloseKey = nullptr;
	MModule hAdvApi;

	// Actually, we need only to check, if the Module is executable (PE) or not
	// Plus, if it is IMAGE_SUBSYSTEM_BATCH_FILE
	// All other options are not interesting to us
	wchar_t strPathExt[] = L".COM;.EXE;.BAT;.CMD;.BTM;"; // Must be Zero-zero-terminated
	pszPathExtEnd = strPathExt+lstrlen(strPathExt);
	// tokenize
	for (wchar_t* colon = wcschr(strPathExt, L';'); colon; colon = wcschr(colon + 1, L';'))
	{
		*colon = L'\0';
	}

	TODO("Проверить на превышение длин строк");

	// первый проход - в текущем каталоге
	pszExtCur = strPathExt;
	// Указано расширение?
	if (ModuleExt)
	{
		// А оно вообще указано в списке допустимых?
		const bool bBatchExt = IsFileBatch(ModuleExt);
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

	if (!strTmpName.GetBuffer(cchStrTmpName))
	{
		goto wrap;
	}
	strTmpName.SetAt(0, L'\0');

	while (!ModuleExt && (pszExtCur < pszPathExtEnd))
	{
		Ext = pszExtCur;
		pszExtCur = pszExtCur + lstrlen(pszExtCur)+1;

		_wcscpyn_c(strTmpName.data(), cchStrTmpName, Module, cchStrTmpName); //-V501

		if (!ModuleExt)
		{
			if (!*Ext)
				continue;
			_wcscatn_c(strTmpName.data(), cchStrTmpName, Ext, cchStrTmpName);
		}

		if (GetImageSubsystem(strTmpName, ImageSubsystem, ImageBits/*16/32/64*/, FileAttrs))
		{
			Result = true;
			goto wrap;
		}

		_ASSERTE(!ModuleExt)
	}

	// seconds path - use SearchPath rules

	// use PATH variable
	strPathEnv = GetEnvVar(L"PATH");
	if (strPathEnv)
	{
		wchar_t* pszPathEnvEnd = strPathEnv.data() + strPathEnv.GetLen();

		wchar_t* pszPathCur = strPathEnv.data();
		while (pszPathCur && (pszPathCur < pszPathEnvEnd))
		{
			wchar_t* Path = pszPathCur;
			wchar_t* pszPathNext = wcschr(pszPathCur, L';');
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

				if (SearchPath(Path, Module, Ext, cchStrTmpName, strTmpName.data(), &pszFilePart))
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

		if (SearchPath(nullptr, Module, Ext, cchStrTmpName, strTmpName.data(), &pszFilePart))
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
		if (!hAdvApi.Load(L"AdvApi32.dll"))
			goto wrap;
		if (!hAdvApi.GetProcAddress("RegOpenKeyExW", _RegOpenKeyEx)
			|| !hAdvApi.GetProcAddress("RegQueryValueExW", _RegQueryValueEx)
			|| !hAdvApi.GetProcAddress("RegCloseKey", _RegCloseKey))
			goto wrap;

		const wchar_t RegPath[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\";
		// В строке Module заменить исполняемый модуль на полный путь, который
		// берется из SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths
		// Сначала смотрим в HKCU, затем - в HKLM
		HKEY RootFindKey[] = {HKEY_CURRENT_USER,HKEY_LOCAL_MACHINE,HKEY_LOCAL_MACHINE};

		BOOL lbAddExt = FALSE;
		pszExtCur = strPathExt;
		while (pszExtCur < pszPathExtEnd)
		{
			if (!lbAddExt)
			{
				Ext = nullptr;
				lbAddExt = TRUE;
			}
			else
			{
				Ext = pszExtCur;
				pszExtCur = pszExtCur + lstrlen(pszExtCur)+1;
				if (!*Ext)
					continue;
			}

			_wcscpy_c(strTmpName.data(), cchStrTmpName, RegPath);
			_wcscatn_c(strTmpName.data(), cchStrTmpName, Module, cchStrTmpName);
			if (Ext)
				_wcscatn_c(strTmpName.data(), cchStrTmpName, Ext, cchStrTmpName);

			DWORD samDesired = KEY_QUERY_VALUE;
			DWORD RedirectionFlag = 0;
			// App Paths key is shared in Windows 7 and above
			if (!IsWin7())
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
					const int RegResult = _RegQueryValueEx(hKey, L"", nullptr, &nType, reinterpret_cast<LPBYTE>(strTmpName.data()), &nSize);
					_RegCloseKey(hKey);

					if ((RegResult == ERROR_SUCCESS) && (nType == REG_SZ || nType == REG_EXPAND_SZ || nType == REG_MULTI_SZ))
					{
						strTmpName.SetAt((nSize >> 1) + 1, L'\0');
						auto strExpand = ExpandEnvStr(strTmpName);
						if (GetImageSubsystem(Unquote(strExpand.data()), ImageSubsystem, ImageBits, FileAttrs))
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
	if (pFileAttrs)
		*pFileAttrs = FileAttrs;
	return Result;
}
