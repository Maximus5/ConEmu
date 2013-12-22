
#pragma once

typedef enum _MINIDUMP_TYPE {
  MiniDumpNormal                           = 0x00000000,
  MiniDumpWithDataSegs                     = 0x00000001,
  MiniDumpWithFullMemory                   = 0x00000002,
  MiniDumpWithHandleData                   = 0x00000004,
  MiniDumpFilterMemory                     = 0x00000008,
  MiniDumpScanMemory                       = 0x00000010,
  MiniDumpWithUnloadedModules              = 0x00000020,
  MiniDumpWithIndirectlyReferencedMemory   = 0x00000040,
  MiniDumpFilterModulePaths                = 0x00000080,
  MiniDumpWithProcessThreadData            = 0x00000100,
  MiniDumpWithPrivateReadWriteMemory       = 0x00000200,
  MiniDumpWithoutOptionalData              = 0x00000400,
  MiniDumpWithFullMemoryInfo               = 0x00000800,
  MiniDumpWithThreadInfo                   = 0x00001000,
  MiniDumpWithCodeSegs                     = 0x00002000,
  MiniDumpWithoutAuxiliaryState            = 0x00004000,
  MiniDumpWithFullAuxiliaryState           = 0x00008000 
} MINIDUMP_TYPE;

typedef struct _MINIDUMP_EXCEPTION_INFORMATION {
  DWORD               ThreadId;
  PVOID/*PEXCEPTION_POINTERS*/ ExceptionPointers;
  BOOL                ClientPointers;
}MINIDUMP_EXCEPTION_INFORMATION, *PMINIDUMP_EXCEPTION_INFORMATION;

typedef struct _MINIDUMP_USER_STREAM_INFORMATION {
  ULONG                 UserStreamCount;
  PVOID/*PMINIDUMP_USER_STREAM*/ UserStreamArray;
}MINIDUMP_USER_STREAM_INFORMATION, *PMINIDUMP_USER_STREAM_INFORMATION;

typedef struct _MINIDUMP_CALLBACK_INFORMATION {
  PVOID/*MINIDUMP_CALLBACK_ROUTINE*/ CallbackRoutine;
  PVOID                     CallbackParam;
}MINIDUMP_CALLBACK_INFORMATION, *PMINIDUMP_CALLBACK_INFORMATION;

//typedef BOOL (CALLBACK* MINIDUMP_CALLBACK_ROUTINE)(
//  PVOID CallbackParam,
//  const PMINIDUMP_CALLBACK_INPUT CallbackInput,
//  PMINIDUMP_CALLBACK_OUTPUT CallbackOutput
//);

typedef struct _MINIDUMP_USER_STREAM {
    ULONG32 Type;
    ULONG BufferSize;
    PVOID Buffer;

} MINIDUMP_USER_STREAM, *PMINIDUMP_USER_STREAM;
