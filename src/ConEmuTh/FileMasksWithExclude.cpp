/*
FileMasksWithExclude.cpp

Класс для работы со сложными масками файлов (учитывается наличие масок
исключения).
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"
#pragma hdrstop

#include "FileMasksWithExclude.hpp"

const wchar_t EXCLUDEMASKSEPARATOR=L'|';

FileMasksWithExclude::FileMasksWithExclude():BaseFileMask()
{
}

void FileMasksWithExclude::Free()
{
	Include.Free();
	Exclude.Free();
}

bool FileMasksWithExclude::IsExcludeMask(const wchar_t *masks)
{
	return FindExcludeChar(masks)!=nullptr;
}

const wchar_t *FileMasksWithExclude::FindExcludeChar(const wchar_t *masks)
{
	const wchar_t *pExclude = masks;

	if (*pExclude == L'/')
	{
		pExclude++;

		while (*pExclude && (*pExclude != L'/' || *(pExclude-1) == L'\\'))
			pExclude++;

		while (*pExclude && *pExclude != EXCLUDEMASKSEPARATOR)
			pExclude++;

		if (*pExclude != EXCLUDEMASKSEPARATOR)
			pExclude = nullptr;
	}
	else
	{
		pExclude = wcschr(masks,EXCLUDEMASKSEPARATOR);
	}

	return pExclude;
}

/*
 Инициализирует список масок. Принимает список, разделенных запятой.
 Возвращает FALSE при неудаче (например, одна из
 длина одной из масок равна 0)
*/

bool FileMasksWithExclude::Set(const wchar_t *masks, DWORD Flags)
{
	Free();

	if (nullptr==masks || !*masks) return FALSE;

	size_t len=StrLength(masks)+1;
	bool rc=false;
	wchar_t *MasksStr=(wchar_t *) xf_malloc(len*sizeof(wchar_t));

	if (MasksStr)
	{
		rc=true;
		wcscpy(MasksStr, masks);
		wchar_t *pExclude = (wchar_t *) FindExcludeChar(MasksStr);

		if (pExclude)
		{
			*pExclude=0;
			++pExclude;

			if (*pExclude!=L'/' && wcschr(pExclude, EXCLUDEMASKSEPARATOR))
				rc=FALSE;
		}

		if (rc)
		{
			rc = Include.Set(*MasksStr?MasksStr:L"*",(Flags&FMPF_ADDASTERISK)?FMPF_ADDASTERISK:0);

			if (rc)
				rc=Exclude.Set(pExclude, 0);
		}
	}

	if (!rc)
		Free();

	if (MasksStr)
		xf_free(MasksStr);

	return rc;
}

/* сравнить имя файла со списком масок
   Возвращает TRUE в случае успеха.
   Путь к файлу в FileName НЕ игнорируется */
bool FileMasksWithExclude::Compare(const wchar_t *FileName)
{
	return (Include.Compare(FileName) && !Exclude.Compare(FileName));
}

bool FileMasksWithExclude::IsEmpty()
{
	return (Include.IsEmpty() && Exclude.IsEmpty());
}
