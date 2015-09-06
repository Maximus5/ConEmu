
#pragma once

// !! если включить - не ставятся хуки?
// Попробовать выполнять инициализацию хуков в отдельной нити
//#define HOOK_USE_DLLTHREAD
#undef HOOK_USE_DLLTHREAD

// Hook GetLastError/SetLastError procedures
//#define HOOK_ERROR_PROC
#undef HOOK_ERROR_PROC
