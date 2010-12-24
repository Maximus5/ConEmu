/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
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

#include <windows.h>

// Выдранный кусок из будущего GetFileInfo, получаем достоверную информацию о ГУЯХ PE-модуля

// Возвращаем константы IMAGE_SUBSYSTEM_* дабы консоль отличать
// При выходе из процедуры IMAGE_SUBSYTEM_UNKNOWN означает
// "файл не является исполняемым".
// Для DOS-приложений определим еще одно значение флага.

// 17.12.2010 Maks
// Если GetImageSubsystem вернет true - то имеет смысл проверять следующие значения
// IMAGE_SUBSYSTEM_WINDOWS_CUI    -- Win Console (32/64)
// IMAGE_SUBSYSTEM_DOS_EXECUTABLE -- DOS Executable (ImageBits == 16)

#define IMAGE_SUBSYSTEM_DOS_EXECUTABLE  255

struct IMAGE_HEADERS
{
	DWORD Signature;
	IMAGE_FILE_HEADER FileHeader;
	union
	{
		IMAGE_OPTIONAL_HEADER32 OptionalHeader32;
		IMAGE_OPTIONAL_HEADER64 OptionalHeader64;
	};
};

bool GetImageSubsystem(const wchar_t *FileName,DWORD& ImageSubsystem,DWORD& ImageBits/*16/32/64*/)
{
	bool Result = false;
	ImageSubsystem = IMAGE_SUBSYSTEM_UNKNOWN;
	ImageBits = 32;
	HANDLE hModuleFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
	if (hModuleFile != INVALID_HANDLE_VALUE)
	{
		IMAGE_DOS_HEADER DOSHeader;
		DWORD ReadSize;
		if (ReadFile(hModuleFile,&DOSHeader,sizeof(DOSHeader),&ReadSize,NULL))
		{
			if (DOSHeader.e_magic!=IMAGE_DOS_SIGNATURE)
			{
				const wchar_t *pszExt = wcsrchr(FileName, L'.');
				if (lstrcmpiW(pszExt, L".com") == 0)
				{
					ImageSubsystem = IMAGE_SUBSYSTEM_DOS_EXECUTABLE;
					ImageBits = 16;
					Result=true;
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
						if (PEHeader.Signature == IMAGE_NT_SIGNATURE)
						{
							switch (PEHeader.OptionalHeader32.Magic)
							{
							case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
								{
									ImageSubsystem = PEHeader.OptionalHeader32.Subsystem;
									ImageBits = 32;
								}
								break;
							case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
								{
									ImageSubsystem = PEHeader.OptionalHeader64.Subsystem;
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
							ImageBits = 32;
							/*
							NE,  хмм...  а как определить что оно ГУЕВОЕ?

							Andrzej Novosiolov <andrzej@se.kiev.ua>
							AN> ориентироваться по флагу "Target operating system" NE-заголовка
							AN> (1 байт по смещению 0x36). Если там Windows (значения 2, 4) - подразумеваем
							AN> GUI, если OS/2 и прочая экзотика (остальные значения) - подразумеваем консоль.
							*/
							BYTE ne_exetyp = reinterpret_cast<PIMAGE_OS2_HEADER>(&PEHeader)->ne_exetyp;
							if (ne_exetyp==2||ne_exetyp==4)
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
	return Result;
}
