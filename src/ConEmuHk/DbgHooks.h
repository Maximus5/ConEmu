
#pragma once

/* Hook GetLastError/SetLastError procedures */
//#define HOOK_ERROR_PROC
#undef HOOK_ERROR_PROC


/* Define macros HLOGx and DLOGx */
/* SKIPHOOKLOG is not defined or undefined here, by it    */
/* may be used to disable logging in the exact *.cpp file */
#ifdef _DEBUG
	//#define USEHOOKLOG
	#undef USEHOOKLOG
#else
	#undef USEHOOKLOG
	#undef USEHOOKLOGANALYZE
#endif
/* USEHOOKLOGANALYZE used to determine voracious steps on exit */
#ifdef USEHOOKLOG
	#define USEHOOKLOGANALYZE
#else
	#undef USEHOOKLOGANALYZE
#endif


/* Print to console on ExitProcess/TerminateProcess */
//#define PRINT_ON_EXITPROCESS_CALLS
#undef PRINT_ON_EXITPROCESS_CALLS


/* gh#272: Crash if NVIDIA CoProcManager dlls are loaded */
//#define USE_GH_272_WORKAROUND
#undef USE_GH_272_WORKAROUND
#ifdef _DEBUG
//#define FORCE_GH_272_WORKAROUND
#endif

/* Print to console, if GetMainThreadId is forced to CreateToolhelp32Snapshot */
#ifdef _DEBUG
	#define FORCE_GETMAINTHREAD_PRINTF
#else
	#undef FORCE_GETMAINTHREAD_PRINTF
#endif
