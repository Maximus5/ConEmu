
// Just a cl wrapper for Win32/Win64 files

#if defined(_WIN64)
#include "../modules/minhook/src/HDE/hde64.c"
#else
#include "../modules/minhook/src/HDE/hde32.c"
#endif
