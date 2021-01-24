
#pragma once

#define IMAGE_SUBSYSTEM_BATCH_FILE  254
#define IMAGE_SUBSYSTEM_DOS_EXECUTABLE  255
#define IMAGE_SUBSYSTEM_FAR_PLUGIN 256 // 0x100
#define IMAGE_SUBSYSTEM_INTERNAL_DEBUGGER 257 // 0x101

// Если GetImageSubsystem вернет true - то имеет смысл проверять следующие значения
// IMAGE_SUBSYSTEM_WINDOWS_CUI    -- Win Console (32/64)
// IMAGE_SUBSYSTEM_DOS_EXECUTABLE -- DOS Executable (ImageBits == 16)
bool GetImageSubsystem(const wchar_t* FileName, DWORD& ImageSubsystem, DWORD& ImageBits/*16/32/64*/, DWORD& FileAttrs);
bool GetImageSubsystem(DWORD& ImageSubsystem, DWORD& ImageBits/*16/32/64*/);
bool GetImageSubsystem(PROCESS_INFORMATION pi, DWORD& ImageSubsystem, DWORD& ImageBits/*16/32/64*/);
bool FindImageSubsystem(const wchar_t* Module, /*wchar_t* pstrDest,*/ DWORD& ImageSubsystem, DWORD& ImageBits, LPDWORD pFileAttrs = nullptr);
