
; Windows 2000 compatible fix for VC 14
.model flat

EXTRN _FixEncodePointer@4:PROC
EXTRN _FixDecodePointer@4:PROC
EXTRN _FixGetModuleHandleExW@12:PROC
EXTRN _FixInitializeSListHead@4:PROC
EXTRN _FixInterlockedFlushSList@4:PROC
EXTRN _FixInterlockedPushEntrySList@8:PROC

EXTERNDEF __imp__EncodePointer@4:DWORD
EXTERNDEF __imp__DecodePointer@4:DWORD
EXTERNDEF __imp__GetModuleHandleExW@12:DWORD
EXTERNDEF __imp__InitializeSListHead@4:DWORD
EXTERNDEF __imp__InterlockedFlushSList@4:DWORD
EXTERNDEF __imp__InterlockedPushEntrySList@8:DWORD

.data
__imp__EncodePointer@4 dd _FixEncodePointer@4
__imp__DecodePointer@4 dd _FixDecodePointer@4
__imp__GetModuleHandleExW@12 dd _FixGetModuleHandleExW@12
__imp__InitializeSListHead@4 dd _FixInitializeSListHead@4
__imp__InterlockedFlushSList@4 dd _FixInterlockedFlushSList@4
__imp__InterlockedPushEntrySList@8 dd _FixInterlockedPushEntrySList@8

end
