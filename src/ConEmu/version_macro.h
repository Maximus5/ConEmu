#pragma once

#define STRING2(x) #x
#define STRING(x) STRING2(x)
#define WSTRING2(x) L ## x
#define WSTRING(x) WSTRING2(x)
#define VERSION_NUMBERS2(a,b) a##b
#define VERSION_NUMBERS(a,b) VERSION_NUMBERS2(a,b)

#if (MVV_2<10)
  #define MVV_2s "0" STRING(MVV_2)
#else
  #define MVV_2s STRING(MVV_2)
#endif
#if (MVV_3<10)
  #define MVV_3s "0" STRING(MVV_3)
#else
  #define MVV_3s STRING(MVV_3)
#endif
//#if (MVV_4>0)
//  #define MVV_3n VERSION_NUMBERS(MVV_4,MVV_3)
//#else
  #define MVV_3n MVV_3
//#endif

#ifndef _DEBUG
  #ifdef MVV_git
    #define CONEMUVERS STRING(MVV_1) MVV_2s MVV_3s "-" MVV_4a
  #else
    #define CONEMUVERS STRING(MVV_1) MVV_2s MVV_3s MVV_4a
  #endif
#else
  #ifdef MVV_git
    #define CONEMUVERS STRING(MVV_1) MVV_2s MVV_3s "-" MVV_4a "-D"
  #else
    #define CONEMUVERS STRING(MVV_1) MVV_2s MVV_3s MVV_4a "-D"
  #endif
#endif

#ifdef __GNUC__
#define CONEMUVERN MVV_1,MVV_2,MVV_3,MVV_4
#else
//#define CONEMUVERN 20##MVV_1,MVV_2,MVV_3n,MVV_4
#define CONEMUVERN MVV_1,MVV_2,MVV_3n,MVV_4
#endif

#if defined(_WIN64) || defined(WIN64)
#define CONEMUPLTFRM " (x64)"
#else
#define CONEMUPLTFRM " (x86)"
#endif
