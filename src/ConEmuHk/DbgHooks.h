
#pragma once

#ifdef _DEBUG
// Выполнять или нет InitHooksLibrary()
//#define HOOKS_SKIP_LIBRARY
#undef  HOOKS_SKIP_LIBRARY

//#define HOOKS_SKIP_GETPROCADDRESS
#undef HOOKS_SKIP_GETPROCADDRESS

// Выполнять или нет InitHooksCommon()
//#define HOOKS_SKIP_COMMON
#undef  HOOKS_SKIP_COMMON

//#define HOOKS_COMMON_PROCESS_ONLY
#undef HOOKS_COMMON_PROCESS_ONLY

#ifdef _DEBUG
//#define HOOKS_VIRTUAL_ALLOC
#undef HOOKS_VIRTUAL_ALLOC
#endif

// !! если включить - не ставятся хуки?
// Попробовать выполнять инициализацию хуков в отдельной нити
//#define HOOK_USE_DLLTHREAD
#undef HOOK_USE_DLLTHREAD


//#define SKIP_GETIMAGESUBSYSTEM_ONLOAD
#undef SKIP_GETIMAGESUBSYSTEM_ONLOAD

#endif
