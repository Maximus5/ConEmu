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
	char szVersion[512];

	if (argc > 1)
	{
		char* psz;
		DWORD ulDate = strtoul(argv[1], &psz, 10);
		if (ulDate)
		{
			st.wDay = ulDate%100; ulDate = (ulDate - (ulDate%100)) / 100;
			st.wMonth = ulDate%100; ulDate = (ulDate - (ulDate%100)) / 100;
			st.wYear = ulDate%100;
		}
	}

	wsprintfA(szVersion,
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>" "\r\n"
		"<Include>" "\r\n"
		"" "\r\n"
		"  <?define Version.Year  = '%02u' ?>" "\r\n"
		"  <?define Version.Month = '%02u' ?>" "\r\n"
		"  <?define Version.Day   = '%02u' ?>" "\r\n"
		"" "\r\n"
		"  <?define Version = $(var.MajorVersion).$(var.Version.Year).$(var.Version.Month)$(var.Version.Day) ?>" "\r\n"
		"  <?define ConEmuVerS = $(var.Version.Year)$(var.Version.Month)$(var.Version.Day) ?>" "\r\n"
		"" "\r\n"
		"</Include>",
		st.wYear % 100, st.wMonth, st.wDay);

    // generate params
    FILE* f = fopen("Version.wxi", "w");
    fwrite(szVersion, lstrlenA(szVersion), 1, f);
    fclose(f);

	return 0;
}
