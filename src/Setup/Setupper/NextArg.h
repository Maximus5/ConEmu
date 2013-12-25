// NO BOM! Compiled with old gcc!

#pragma once

// Возвращает 0, если успешно, иначе - ошибка
int NextArg(const wchar_t** asCmdLine, wchar_t (&rsArg)[MAX_PATH+1], const wchar_t** rsArgStart=NULL)
{
	LPCWSTR psCmdLine = *asCmdLine, pch = NULL;
	wchar_t ch = *psCmdLine;
	size_t nArgLen = 0;
	bool lbQMode = false;

	while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);

	if (ch == 0) return -1;

	// аргумент начинается с "
	if (ch == L'"')
	{
		lbQMode = true;
		psCmdLine++;
		pch = wcschr(psCmdLine, L'"');

		if (!pch) return -2;

		while (pch[1] == L'"')
		{
			pch += 2;
			pch = wcschr(pch, L'"');

			if (!pch) return -2;
		}

		// Теперь в pch ссылка на последнюю "
	}
	else
	{
		// До конца строки или до первого пробела
		//pch = wcschr(psCmdLine, L' ');
		// 09.06.2009 Maks - обломался на: cmd /c" echo Y "
		pch = psCmdLine;

		// Ищем обычным образом (до пробела/кавычки)
		while (*pch && *pch!=L' ' && *pch!=L'"') pch++;

		//if (!pch) pch = psCmdLine + lstrlenW(psCmdLine); // до конца строки
	}

	nArgLen = pch - psCmdLine;

	if (nArgLen > MAX_PATH) return -2;

	// Вернуть аргумент
	memcpy(rsArg, psCmdLine, nArgLen*sizeof(*rsArg));

	if (rsArgStart) *rsArgStart = psCmdLine;

	rsArg[nArgLen] = 0;
	psCmdLine = pch;
	// Finalize
	ch = *psCmdLine; // может указывать на закрывающую кавычку

	if (lbQMode && ch == L'"') ch = *(++psCmdLine);

	while (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n') ch = *(++psCmdLine);

	*asCmdLine = psCmdLine;
	return 0;
}
