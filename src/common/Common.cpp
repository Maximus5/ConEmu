
#include <windows.h>
#include "common.hpp"

int NextArg(const wchar_t** asCmdLine, wchar_t* rsArg/*[MAX_PATH+1]*/)
{
    LPCWSTR psCmdLine = *asCmdLine, pch = NULL;
    wchar_t ch = *psCmdLine;
    int nArgLen = 0;
    
    while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);
    if (ch == 0) return CERR_CMDLINEEMPTY;

    // аргумент начинается с "
    if (ch == L'"') {
        psCmdLine++;
        pch = wcschr(psCmdLine, L'"');
        if (!pch) return CERR_CMDLINE;
        while (pch[1] == L'"') {
            pch += 2;
            pch = wcschr(pch, L'"');
            if (!pch) return CERR_CMDLINE;
        }
        // Теперь в pch ссылка на последнюю "
    } else {
        // До конца строки или до первого пробела
        //pch = wcschr(psCmdLine, L' ');
        // 09.06.2009 Maks - обломался на: cmd /c" echo Y "
        pch = psCmdLine;
        while (*pch && *pch!=L' ' && *pch!=L'"') pch++;
        //if (!pch) pch = psCmdLine + wcslen(psCmdLine); // до конца строки
    }
    
    nArgLen = pch - psCmdLine;
    if (nArgLen > MAX_PATH) return CERR_CMDLINE;

    // Вернуть аргумент
    memcpy(rsArg, psCmdLine, nArgLen*sizeof(wchar_t));
    rsArg[nArgLen] = 0;

    psCmdLine = pch;
    
    // Finalize
    ch = *psCmdLine; // может указывать на закрывающую кавычку
    if (ch == L'"') ch = *(++psCmdLine);
    while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);
    *asCmdLine = psCmdLine;
    
    return 0;
}
