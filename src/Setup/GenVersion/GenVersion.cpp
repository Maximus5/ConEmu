// KeyEvents.cpp : Defines the entry point for the console application.
//

#include <Windows.h>
#include <stdio.h>
#include <tchar.h>

#ifdef __GNUC__
int main(int argc, char* argv[])
#else
int _tmain(int argc, _TCHAR* argv[])
#endif
{
	SYSTEMTIME st; GetSystemTime(&st);
	int iMinor = 0;
	char szVersion[1024], szString[32], szNumeric[32], szMinor[2];
	szString[0] = szNumeric[0] = szMinor[0] = 0;

	if (argc > 1)
	{
		char* psz;
		DWORD ulDate = strtoul(argv[1], &psz, 10);
		if (ulDate)
		{
			st.wDay = ulDate%100; ulDate = (ulDate - (ulDate%100)) / 100;
			st.wMonth = ulDate%100; ulDate = (ulDate - (ulDate%100)) / 100;
			st.wYear = ulDate%100;
			if (psz && (*psz >= 'a' && *psz <= 'z'))
			{
				szMinor[0] = *psz; szMinor[1] = 0;
				iMinor = (*psz - 'a' + 1);
				if (iMinor < 0) iMinor = 0; else if (iMinor > 9) iMinor = 9;
				wsprintfA(szString,  "%02u%02u%02u%s", st.wYear % 100, st.wMonth, st.wDay, szMinor);
				wsprintfA(szNumeric, "%02u%u.%u%02u%u", st.wYear % 100, (st.wMonth - (st.wMonth % 10)) / 10, st.wMonth % 10, st.wDay, iMinor);
			}
		}
	}
	if (!*szString)
		wsprintfA(szString,  "%02u%02u%02u", st.wYear % 100, st.wMonth, st.wDay);
	if (!*szNumeric)
		wsprintfA(szNumeric, "%02u%u.%u%02u0", st.wYear % 100, (st.wMonth - (st.wMonth % 10)) / 10, st.wMonth % 10, st.wDay);

	wsprintfA(szVersion,
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>" "\n"
		"<Include>" "\n"
		"" "\n"
		"  <?define Version = '$(var.MajorVersion).%s' ?>" "\n"
		"  <?define ConEmuVerS = '%s.$(var.Platform)' ?>" "\n"
		"" "\n"
		"</Include>",
		szNumeric, szString);

    // generate params
    FILE* f = fopen("Version.wxi", "w");
    fwrite(szVersion, lstrlenA(szVersion), 1, f);
    fclose(f);
    
    
	wsprintfA(szVersion,
		"#define CONEMUVERN %u,%u,%u,%u\n"
		"#define CONEMUVERS \"%s\"\n"
		"#define CONEMUVERL L\"%s\"\n"
		"#define MSI86 \"../ConEmu.%s.x86.msi\"\n"
		"#define MSI64 \"../ConEmu.%s.x64.msi\"\n",
		st.wYear % 100, st.wMonth, st.wDay, iMinor, szString, szString, szString, szString);
    
    // setupper
    f = fopen("Setupper\\VersionI.h", "w");
    fwrite(szVersion, lstrlenA(szVersion), 1, f);
    fclose(f);

	return 0;
}
