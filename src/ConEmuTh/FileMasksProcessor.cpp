/*
FileMasksProcessor.cpp

Класс для работы с простыми масками файлов (не учитывается наличие масок
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

#include "FileMasksProcessor.hpp"
#include "processname.hpp"

FileMasksProcessor::FileMasksProcessor():
	BaseFileMask(),
	re(nullptr),
	m(nullptr),
	n(0),
	bRE(false)
{
}

void FileMasksProcessor::Free()
{
	Masks.Free();

	if (re)
		delete re;

	re = nullptr;

	if (m)
		xf_free(m);

	m = nullptr;
	n = 0;
	bRE = false;
}

/*
 Инициализирует список масок. Принимает список, разделенных запятой.
 Возвращает FALSE при неудаче (например, одна из
 длина одной из масок равна 0)
*/

bool FileMasksProcessor::Set(const wchar_t *masks, DWORD Flags)
{
	Free();
	// разделителем масок является не только запятая, но и точка с запятой!
	DWORD flags=ULF_PACKASTERISKS|ULF_PROCESSBRACKETS|ULF_SORT|ULF_UNIQUE;

	if (Flags&FMPF_ADDASTERISK)
		flags|=ULF_ADDASTERISK;

	bRE = (masks && *masks == L'/');

	if (bRE)
	{
		re = new RegExp;

		if (re && re->Compile(masks, OP_PERLSTYLE|OP_OPTIMIZE))
		{
			n = re->GetBracketsCount();
			m = (SMatch *)xf_malloc(n*sizeof(SMatch));

			if (m == nullptr)
			{
				n = 0;
				return false;
			}

			return true;
		}

		return false;
	}

	Masks.SetParameters(L',',L';',flags);
	return Masks.Set(masks);
}

bool FileMasksProcessor::IsEmpty()
{
	if (bRE)
	{
		return !n;
	}

	Masks.Reset();
	return Masks.IsEmpty();
}

/* сравнить имя файла со списком масок
   Возвращает TRUE в случае успеха.
   Путь к файлу в FileName НЕ игнорируется */
bool FileMasksProcessor::Compare(const wchar_t *FileName)
{
	if (bRE)
	{
		int i = n;
		int len = StrLength(FileName);
		bool ret = re->Search(FileName,FileName+len,m,i) ? TRUE : FALSE;

		//Освободим память если большая строка, чтоб не накапливалось.
		if (len > 1024)
			re->CleanStack();

		return ret;
	}

	Masks.Reset();

	while (nullptr!=(MaskPtr=Masks.GetNext()))
	{
		// SkipPath=FALSE, т.к. в CFileMask вызывается PointToName
		if (CmpName(MaskPtr,FileName, false))
			return true;
	}

	return false;
}
