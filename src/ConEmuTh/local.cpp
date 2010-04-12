/*
local.cpp

Сравнение без учета регистра, преобразование регистра
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

#include "local.hpp"

const wchar_t DOS_EOL_fmt[]  = L"\r\n";
const wchar_t UNIX_EOL_fmt[] = L"\n";
const wchar_t MAC_EOL_fmt[]  = L"\r";
const wchar_t WIN_EOL_fmt[]  = L"\r\r\n";

const wchar_t * __cdecl StrStrI(const wchar_t *str1, const wchar_t *str2)
{
	const wchar_t *cp = str1;
	const wchar_t *s1, *s2;

	if (!*str2)
		return str1;

	while (*cp)
	{
		s1 = cp;
		s2 = str2;

		while (*s1 && *s2 && !(Lower(*s1)-Lower(*s2)))
		{
			s1++;
			s2++;
		}

		if (!*s2)
			return cp;

		cp++;
	}

	return nullptr;
}

const wchar_t * __cdecl StrStr(const wchar_t *str1, const wchar_t *str2)
{
	const wchar_t *cp = str1;
	const wchar_t *s1, *s2;

	if (!*str2)
		return str1;

	while (*cp)
	{
		s1 = cp;
		s2 = str2;

		while (*s1 && *s2 && !(*s1 - *s2))
		{
			s1++;
			s2++;
		}

		if (!*s2)
			return cp;

		cp++;
	}

	return nullptr;
}

const wchar_t * __cdecl RevStrStrI(const wchar_t *str1, const wchar_t *str2)
{
	int len1 = StrLength(str1);
	int len2 = StrLength(str2);

	if (len2 > len1)
		return nullptr;

	if (!*str2)
		return &str1[len1];

	const wchar_t *cp = &str1[len1-len2];
	const wchar_t *s1, *s2;

	while (cp >= str1)
	{
		s1 = cp;
		s2 = str2;

		while (*s1 && *s2 && !(Lower(*s1)-Lower(*s2)))
		{
			s1++;
			s2++;
		}

		if (!*s2)
			return cp;

		cp--;
	}

	return nullptr;
}

const wchar_t * __cdecl RevStrStr(const wchar_t *str1, const wchar_t *str2)
{
	int len1 = StrLength(str1);
	int len2 = StrLength(str2);

	if (len2 > len1)
		return nullptr;

	if (!*str2)
		return &str1[len1];

	const wchar_t *cp = &str1[len1-len2];
	const wchar_t *s1, *s2;

	while (cp >= str1)
	{
		s1 = cp;
		s2 = str2;

		while (*s1 && *s2 && !(*s1 - *s2))
		{
			s1++;
			s2++;
		}

		if (!*s2)
			return cp;

		cp--;
	}

	return nullptr;
}

int NumStrCmp(const wchar_t *s1, size_t n1, const wchar_t *s2, size_t n2, bool IgnoreCase)
{
	size_t l1 = 0;
	size_t l2 = 0;
	while (l1 < n1 && l2 < n2 && *s1 && *s2)
	{
		if (iswdigit(*s1) && iswdigit(*s2))
		{
			// skip leading zeroes
			while (l1 < n1 && *s1 == L'0')
			{
				s1++;
				l1++;
			}
			while (l2 < n2 && *s2 == L'0')
			{
				s2++;
				l2++;
			}

			// if end of string reached
			if (l1 == n1 || *s1 == 0 || l2 == n2 || *s2 == 0)
				break;

			// compare numbers
			int res = 0;
			while (l1 < n1 && l2 < n2 && iswdigit(*s1) && iswdigit(*s2))
			{
				if (res == 0 && *s1 != *s2)
					res = *s1 < *s2 ? -1 : 1;

				s1++; s2++;
				l1++; l2++;
			}
			if ((l1 == n1 || !iswdigit(*s1)) && (l2 == n2 || !iswdigit(*s2)))
			{
				if (res)
					return res;
			}
			else if (l1 == n1 || !iswdigit(*s1))
				return -1;
			else if (l2 == n2 || !iswdigit(*s2))
				return 1;
		}
		else
		{
			int res = IgnoreCase ? StrCmpNI(s1, s2, 1) : StrCmpN(s1, s2, 1);
			if (res)
				return res;

			s1++; s2++;
			l1++; l2++;
		}
	}

	if ((l1 == n1 || *s1 == 0) && (l2 == n2 || *s2 == 0))
	{
		if (l1 < l2)
			return -1;
		else if (l1 == l2)
			return 0;
		else
			return 1;
	}
	else if (l1 == n1 || *s1 == 0)
		return -1;
	else if (l2 == n2 || *s2 == 0)
		return 1;

	assert(false);
	return 0;
}

SELF_TEST(
	assert(NumStrCmp(L"", -1, L"", -1, false) == 0);
	assert(NumStrCmp(L"", -1, L"a", -1, false) < 0);
	assert(NumStrCmp(L"a", -1, L"a", -1, false) == 0);

	assert(NumStrCmp(L"0", -1, L"1", -1, false) < 0);
	assert(NumStrCmp(L"0", -1, L"00", -1, false) < 0);
	assert(NumStrCmp(L"1", -1, L"00", -1, false) > 0);
	assert(NumStrCmp(L"10", -1, L"1", -1, false) > 0);
	assert(NumStrCmp(L"10", -1, L"2", -1, false) > 0);
	assert(NumStrCmp(L"10", -1, L"0100", -1, false) < 0);
	assert(NumStrCmp(L"1", -1, L"001", -1, false) < 0);

	assert(NumStrCmp(L"10a", -1, L"2b", -1, false) > 0);
	assert(NumStrCmp(L"10a", -1, L"0100b", -1, false) < 0);
	assert(NumStrCmp(L"a1a", -1, L"a001a", -1, false) < 0);
	assert(NumStrCmp(L"a1b2c", -1, L"a1b2c", -1, false) == 0);
	assert(NumStrCmp(L"a01b2c", -1, L"a1b002c", -1, false) < 0);
	assert(NumStrCmp(L"a01b3c", -1, L"a1b002", -1, false) > 0);

	assert(NumStrCmp(L"10", 2, L"0100", 2, false) > 0);
	assert(NumStrCmp(L"01", 2, L"0100", 2, false) == 0);

	assert(NumStrCmp(L"A1", -1, L"a2", -1, false) > 0);
	assert(NumStrCmp(L"A1", -1, L"a2", -1, true) < 0);
)
