
#pragma once

#include <windows.h>

#ifndef NORM_STOP_ON_NULL
#define NORM_STOP_ON_NULL 0x10000000
#endif

#include <wchar.h>

#define countof(a) (sizeof(a)/sizeof(a[0]))

#define NullToEmpty(s) (s?s:L"")

#if (defined(__GNUC__)) || (defined(_MSC_VER) && _MSC_VER<1600)
#define nullptr NULL
#endif


#include "..\common\pluginW1007.hpp" // Отличается от 995 наличием SynchoApi

#include "UnicodeString.hpp"
#include "FileFilterParams.hpp"
#include "farwinapi.hpp"
