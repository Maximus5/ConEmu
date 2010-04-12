/*
UnicodeString.hpp

Unicode строки
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

UnicodeStringData *eus()
{
	//дл€ оптимизации создани€ пустых UnicodeString
	static UnicodeStringData *EmptyUnicodeStringData = new UnicodeStringData(1,1);
	return EmptyUnicodeStringData;
}

void UnicodeString::SetEUS()
{
	m_pData = eus();
	m_pData->AddRef();
}

void UnicodeString::Inflate(size_t nSize)
{
	if (m_pData->GetRef() == 1)
	{
		m_pData->Inflate(nSize);
	}
	else
	{
		UnicodeStringData *pNewData = new UnicodeStringData(nSize);
		size_t nNewLength = Min(m_pData->GetLength(),nSize-1);
		wmemcpy(pNewData->GetData(),m_pData->GetData(),nNewLength);
		pNewData->SetLength(nNewLength);
		m_pData->DecRef();
		m_pData = pNewData;
	}
}

size_t UnicodeString::GetCharString(char *lpszStr, size_t nSize, UINT CodePage) const
{
	if (!lpszStr)
		return 0;

	size_t nCopyLength = (nSize <= m_pData->GetLength()+1 ? nSize-1 : m_pData->GetLength());
	WideCharToMultiByte(CodePage,0,m_pData->GetData(),(int)nCopyLength,lpszStr,(int)nCopyLength+1,nullptr,nullptr);
	lpszStr[nCopyLength] = 0;
	return nCopyLength+1;
}

UnicodeString& UnicodeString::Replace(size_t Pos, size_t Len, const wchar_t* Data, size_t DataLen)
{
	// Pos & Len must be valid
	assert(Pos <= m_pData->GetLength());
	assert(Len <= m_pData->GetLength());
	assert(Pos + Len <= m_pData->GetLength());
	// Data and *this must not intersect (but Data can be located entirely within *this)
	assert(!(Data < m_pData->GetData() && Data + DataLen > m_pData->GetData()));
	assert(!(Data < m_pData->GetData() + m_pData->GetLength() && Data + DataLen > m_pData->GetData() + m_pData->GetLength()));

	if ((Len == 0) && (DataLen == 0))
		return *this;

	size_t NewLength = m_pData->GetLength() + DataLen - Len;

	if (m_pData->GetRef() == 1 && NewLength + 1 <= m_pData->GetSize())
	{
		if (NewLength)
		{
			if (Data >= m_pData->GetData() && Data + DataLen <= m_pData->GetData() + m_pData->GetLength())
			{
				// copy data from self
				UnicodeString TmpStr(Data, DataLen);
				wmemmove(m_pData->GetData() + Pos + DataLen, m_pData->GetData() + Pos + Len, m_pData->GetLength() - Pos - Len);
				wmemcpy(m_pData->GetData() + Pos, TmpStr.CPtr(), TmpStr.GetLength());
			}
			else
			{
				wmemmove(m_pData->GetData() + Pos + DataLen, m_pData->GetData() + Pos + Len, m_pData->GetLength() - Pos - Len);
				wmemcpy(m_pData->GetData() + Pos, Data, DataLen);
			}
		}

		m_pData->SetLength(NewLength);
	}
	else
	{
		if (!NewLength)
		{
			m_pData->DecRef();
			SetEUS();
			return *this;
		}

		UnicodeStringData *NewData = new UnicodeStringData(NewLength + 1);
		wmemcpy(NewData->GetData(), m_pData->GetData(), Pos);
		wmemcpy(NewData->GetData() + Pos, Data, DataLen);
		wmemcpy(NewData->GetData() + Pos + DataLen, m_pData->GetData() + Pos + Len, m_pData->GetLength() - Pos - Len);
		NewData->SetLength(NewLength);
		m_pData->DecRef();
		m_pData = NewData;
	}

	return *this;
}

UnicodeString& UnicodeString::Append(const char *lpszAdd, UINT CodePage)
{
	if (lpszAdd && *lpszAdd)
	{
		size_t nAddSize = MultiByteToWideChar(CodePage,0,lpszAdd,-1,nullptr,0);
		size_t nNewLength = m_pData->GetLength() + nAddSize - 1;
		Inflate(nNewLength + 1);
		MultiByteToWideChar(CodePage,0,lpszAdd,(int)nAddSize,m_pData->GetData() + m_pData->GetLength(),(int)m_pData->GetSize());
		m_pData->SetLength(nNewLength);
	}

	return *this;
}

UnicodeString& UnicodeString::Copy(const UnicodeString &Str)
{
	if (Str.m_pData != m_pData)
	{
		m_pData->DecRef();
		m_pData = Str.m_pData;
		m_pData->AddRef();
	}

	return *this;
}

UnicodeString& UnicodeString::Copy(const char *lpszData, UINT CodePage)
{
	m_pData->DecRef();

	if (!lpszData || !*lpszData)
	{
		SetEUS();
	}
	else
	{
		size_t nSize = MultiByteToWideChar(CodePage,0,lpszData,-1,nullptr,0);
		m_pData = new UnicodeStringData(nSize);
		MultiByteToWideChar(CodePage,0,lpszData,(int)nSize,m_pData->GetData(),(int)m_pData->GetSize());
		m_pData->SetLength(nSize - 1);
	}

	return *this;
}

UnicodeString UnicodeString::SubStr(size_t Pos, size_t Len) {
	if (Pos >= GetLength())
		return UnicodeString();
	if (Len == static_cast<size_t>(-1) || Len > GetLength() || Pos + Len > GetLength())
		Len = GetLength() - Pos;
	return UnicodeString(m_pData->GetData() + Pos, Len);
}

bool UnicodeString::Equal(size_t Pos, size_t Len, const wchar_t* Data, size_t DataLen) const
{
	if (Pos >= m_pData->GetLength())
		Len = 0;
	else if (Len >= m_pData->GetLength() || Pos + Len >= m_pData->GetLength())
		Len = m_pData->GetLength() - Pos;

	if (Len != DataLen)
		return false;

	return wmemcmp(m_pData->GetData() + Pos, Data, Len) == 0;
}

const UnicodeString operator+(const UnicodeString &strSrc1, const UnicodeString &strSrc2)
{
	return UnicodeString(strSrc1).Append(strSrc2);
}

const UnicodeString operator+(const UnicodeString &strSrc1, const char *lpszSrc2)
{
	return UnicodeString(strSrc1).Append(lpszSrc2);
}

const UnicodeString operator+(const UnicodeString &strSrc1, const wchar_t *lpwszSrc2)
{
	return UnicodeString(strSrc1).Append(lpwszSrc2);
}

wchar_t *UnicodeString::GetBuffer(size_t nSize)
{
	Inflate(nSize == (size_t)-1?m_pData->GetSize():nSize);
	return m_pData->GetData();
}

void UnicodeString::ReleaseBuffer(size_t nLength)
{
	if (nLength == (size_t)-1)
		nLength = StrLength(m_pData->GetData());

	if (nLength >= m_pData->GetSize())
		nLength = m_pData->GetSize() - 1;

	m_pData->SetLength(nLength);
}

size_t UnicodeString::SetLength(size_t nLength)
{
	if (nLength < m_pData->GetLength())
	{
		if (!nLength && m_pData->GetRef() > 1)
		{
			m_pData->DecRef();
			SetEUS();
		}
		else
		{
			Inflate(nLength+1);
			return m_pData->SetLength(nLength);
		}
	}

	return m_pData->GetLength();
}

UnicodeString& UnicodeString::Clear()
{
	if (m_pData->GetRef() > 1)
	{
		m_pData->DecRef();
		SetEUS();
	}
	else
	{
		m_pData->SetLength(0);
	}

	return *this;
}

int __cdecl UnicodeString::Format(const wchar_t * format, ...)
{
	wchar_t *buffer = nullptr;
	size_t Size = MAX_PATH;
	int retValue = -1;
	va_list argptr;
	va_start(argptr, format);

	do
	{
		Size <<= 1;
		wchar_t *tmpbuffer = (wchar_t*)xf_realloc_nomove(buffer, Size*sizeof(wchar_t));

		if (!tmpbuffer)
		{
			va_end(argptr);
			xf_free(buffer);
			return retValue;
		}

		buffer = tmpbuffer;
		//_vsnwprintf не всегда ставит '\0' вконце.
		//ѕоэтому надо обнулить и передать в _vsnwprintf размер-1.
		buffer[Size-1] = 0;
		retValue = _vsnwprintf(buffer, Size-1, format, argptr);
	}
	while (retValue == -1);

	va_end(argptr);
	Copy(buffer);
	xf_free(buffer);
	return retValue;
}

UnicodeString& UnicodeString::Lower(size_t nStartPos, size_t nLength)
{
	Inflate(m_pData->GetSize());
	CharLowerBuffW(m_pData->GetData()+nStartPos, nLength==(size_t)-1?(DWORD)(m_pData->GetLength()-nStartPos):(DWORD)nLength);
	return *this;
}

UnicodeString&  UnicodeString::Upper(size_t nStartPos, size_t nLength)
{
	Inflate(m_pData->GetSize());
	CharUpperBuffW(m_pData->GetData()+nStartPos, nLength==(size_t)-1?(DWORD)(m_pData->GetLength()-nStartPos):(DWORD)nLength);
	return *this;
}

bool UnicodeString::Pos(size_t &nPos, wchar_t Ch, size_t nStartPos) const
{
	const wchar_t *lpwszStr = wcschr(m_pData->GetData()+nStartPos,Ch);

	if (lpwszStr)
	{
		nPos = lpwszStr - m_pData->GetData();
		return true;
	}

	return false;
}

bool UnicodeString::Pos(size_t &nPos, const wchar_t *lpwszFind, size_t nStartPos) const
{
	const wchar_t *lpwszStr = wcsstr(m_pData->GetData()+nStartPos,lpwszFind);

	if (lpwszStr)
	{
		nPos = lpwszStr - m_pData->GetData();
		return true;
	}

	return false;
}

bool UnicodeString::PosI(size_t &nPos, const wchar_t *lpwszFind, size_t nStartPos) const
{
	const wchar_t *lpwszStr = StrStrI(m_pData->GetData()+nStartPos,lpwszFind);

	if (lpwszStr)
	{
		nPos = lpwszStr - m_pData->GetData();
		return true;
	}

	return false;
}

bool UnicodeString::RPos(size_t &nPos, wchar_t Ch, size_t nStartPos) const
{
	const wchar_t *lpwszStrStart = m_pData->GetData()+nStartPos;
	const wchar_t *lpwszStrEnd = m_pData->GetData()+m_pData->GetLength();

	do
	{
		if (*lpwszStrEnd == Ch)
		{
			nPos = lpwszStrEnd - m_pData->GetData();
			return true;
		}

		lpwszStrEnd--;
	}
	while (lpwszStrEnd >= lpwszStrStart);

	return false;
}
