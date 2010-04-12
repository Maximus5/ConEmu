#pragma once

/*
UnicodeString.hpp

Unicode строка
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

#include "local.hpp"

#define __US_DELTA 20

class UnicodeStringData
{
	private:
		size_t m_nLength;
		size_t m_nSize;
		size_t m_nDelta;
		int m_nRefCount;
		wchar_t *m_pData;

		wchar_t *AllocData(size_t nSize, size_t *nNewSize)
		{
			if (nSize <= m_nDelta)
				*nNewSize = m_nDelta;
			else if (nSize%m_nDelta > 0)
				*nNewSize = (nSize/m_nDelta + 1) * m_nDelta;
			else
				*nNewSize = nSize;

			return new wchar_t[*nNewSize];
		}

		void FreeData(wchar_t *pData) { delete [] pData; }

	public:
		UnicodeStringData(size_t nSize=0, size_t nDelta=0)
		{
			m_nDelta = (nDelta == 0 ? __US_DELTA : nDelta);
			m_nLength = 0;
			m_nRefCount = 1;
			m_pData = AllocData(nSize,&m_nSize);
			//Так как ни где выше в коде мы не готовы на случай что памяти не хватит
			//то уж лучше и здесь не проверять а сразу падать
			*m_pData = 0;
		}

		size_t SetLength(size_t nLength)
		{
			//if (nLength<m_nSize) //Эту проверку делает верхний класс, так что скажем что это оптимизация
			{
				m_nLength = nLength;
				m_pData[m_nLength] = 0;
			}
			return m_nLength;
		}

		void Inflate(size_t nSize)
		{
			if (nSize <= m_nSize)
				return;

			if (nSize >= m_nDelta << 3)
				nSize = nSize << 1;
			else
				nSize = (nSize/m_nDelta + 1) * m_nDelta;

			wchar_t *pOldData = m_pData;
			m_pData = AllocData(nSize,&m_nSize);
			//Так как ни где выше в коде мы не готовы на случай что памяти не хватит
			//то уж лучше и здесь не проверять а сразу падать
			wmemcpy(m_pData,pOldData,m_nLength);
			m_pData[m_nLength] = 0;
			FreeData(pOldData);
		}

		wchar_t *GetData() { return m_pData; }
		size_t GetLength() const { return m_nLength; }
		size_t GetSize() const { return m_nSize; }

		int GetRef() const { return m_nRefCount; }
		void AddRef() { m_nRefCount++; }
		void DecRef()
		{
			m_nRefCount--;

			if (!m_nRefCount)
				delete this;
		}

		~UnicodeStringData() { FreeData(m_pData); }
};

typedef class UnicodeString
{
	private:
		UnicodeStringData *m_pData;

		void SetEUS();

	public:

		UnicodeString() { SetEUS(); }
		UnicodeString(const UnicodeString &strCopy) { SetEUS(); Copy(strCopy); }
		UnicodeString(const wchar_t *lpwszData) { SetEUS(); Copy(lpwszData); }
		UnicodeString(const wchar_t *lpwszData, size_t nLength) { SetEUS(); Copy(lpwszData, nLength); }
		UnicodeString(const char *lpszData, UINT CodePage=CP_OEMCP) { SetEUS(); Copy(lpszData, CodePage); }
		explicit UnicodeString(size_t nSize, size_t nDelta=0) { m_pData = new UnicodeStringData(nSize, nDelta); }

		~UnicodeString() { /*if (m_pData) он не должен быть nullptr*/ m_pData->DecRef(); }

		void Inflate(size_t nSize);
		wchar_t *GetBuffer(size_t nSize = (size_t)-1);
		void ReleaseBuffer(size_t nLength = (size_t)-1);

		size_t GetLength() const { return m_pData->GetLength(); }
		size_t SetLength(size_t nLength);

		wchar_t At(size_t nIndex) const { return m_pData->GetData()[nIndex]; }

		bool IsEmpty() const { return !(m_pData->GetLength() && *m_pData->GetData()); }

		size_t GetCharString(char *lpszStr, size_t nSize, UINT CodePage=CP_OEMCP) const;

		int __cdecl Format(const wchar_t * format, ...);

		UnicodeString& Replace(size_t Pos, size_t Len, const wchar_t* Data, size_t DataLen);
		UnicodeString& Replace(size_t Pos, size_t Len, const UnicodeString& Str) { return Replace(Pos, Len, Str.CPtr(), Str.GetLength()); }
		UnicodeString& Replace(size_t Pos, size_t Len, const wchar_t* Str) { return Replace(Pos, Len, Str, StrLength(NullToEmpty(Str))); }
		UnicodeString& Replace(size_t Pos, size_t Len, wchar_t Ch) { return Replace(Pos, Len, &Ch, 1); }

		UnicodeString& Append(const wchar_t* Str, size_t StrLen) { return Replace(GetLength(), 0, Str, StrLen); }
		UnicodeString& Append(const UnicodeString& Str) { return Append(Str.CPtr(), Str.GetLength()); }
		UnicodeString& Append(const wchar_t* Str) { return Append(Str, StrLength(NullToEmpty(Str))); }
		UnicodeString& Append(wchar_t Ch) { return Append(&Ch, 1); }
		UnicodeString& Append(const char *lpszAdd, UINT CodePage=CP_OEMCP);

		UnicodeString& Insert(size_t Pos, const wchar_t* Str, size_t StrLen) { return Replace(Pos, 0, Str, StrLen); }
		UnicodeString& Insert(size_t Pos, const UnicodeString& Str) { return Insert(Pos, Str.CPtr(), Str.GetLength()); }
		UnicodeString& Insert(size_t Pos, const wchar_t* Str) { return Insert(Pos, Str, StrLength(NullToEmpty(Str))); }
		UnicodeString& Insert(size_t Pos, wchar_t Ch) { return Insert(Pos, &Ch, 1); }

		UnicodeString& Copy(const wchar_t *Str, size_t StrLen) { return Replace(0, GetLength(), Str, StrLen); }
		UnicodeString& Copy(const wchar_t *Str) { return Copy(Str, StrLength(NullToEmpty(Str))); }
		UnicodeString& Copy(wchar_t Ch) { return Copy(&Ch, 1); }
		UnicodeString& Copy(const UnicodeString &Str);
		UnicodeString& Copy(const char *lpszData, UINT CodePage=CP_OEMCP);

		UnicodeString& Remove(size_t Pos, size_t Len = 1) { return Replace(Pos, Len, nullptr, 0); }
		UnicodeString& LShift(size_t nShiftCount, size_t nStartPos=0) { return Remove(nStartPos, nShiftCount); }

		UnicodeString& Clear();

		const wchar_t *CPtr() const { return m_pData->GetData(); }
		operator const wchar_t *() const { return m_pData->GetData(); }

		UnicodeString SubStr(size_t Pos, size_t Len = -1);

		const UnicodeString& operator=(const UnicodeString &strCopy) { return Copy(strCopy); }
		const UnicodeString& operator=(const char *lpszData) { return Copy(lpszData); }
		const UnicodeString& operator=(const wchar_t *lpwszData) { return Copy(lpwszData); }
		const UnicodeString& operator=(wchar_t chData) { return Copy(chData); }

		const UnicodeString& operator+=(const UnicodeString &strAdd) { return Append(strAdd); }
		const UnicodeString& operator+=(const char *lpszAdd) { return Append(lpszAdd); }
		const UnicodeString& operator+=(const wchar_t *lpwszAdd) { return Append(lpwszAdd); }
		const UnicodeString& operator+=(wchar_t chAdd) { return Append(chAdd); }

		friend const UnicodeString operator+(const UnicodeString &strSrc1, const UnicodeString &strSrc2);
		friend const UnicodeString operator+(const UnicodeString &strSrc1, const char *lpszSrc2);
		friend const UnicodeString operator+(const UnicodeString &strSrc1, const wchar_t *lpwszSrc2);

		bool Equal(size_t Pos, size_t Len, const wchar_t* Data, size_t DataLen) const;
		bool Equal(size_t Pos, const wchar_t* Str, size_t StrLen) const { return Equal(Pos, StrLen, Str, StrLen); }
		bool Equal(size_t Pos, const wchar_t* Str) const { return Equal(Pos, StrLength(Str), Str, StrLength(Str)); }
		bool Equal(size_t Pos, const UnicodeString& Str) const { return Equal(Pos, Str.GetLength(), Str.CPtr(), Str.GetLength()); }
		bool Equal(size_t Pos, wchar_t Ch) const { return Equal(Pos, 1, &Ch, 1); }
		bool operator==(const UnicodeString& Str) const { return Equal(0, GetLength(), Str.CPtr(), Str.GetLength()); }
		bool operator==(const wchar_t* Str) const { return Equal(0, GetLength(), Str, StrLength(Str)); }
		bool operator==(wchar_t Ch) const { return Equal(0, GetLength(), &Ch, 1); }

		UnicodeString& Lower(size_t nStartPos=0, size_t nLength=(size_t)-1);
		UnicodeString& Upper(size_t nStartPos=0, size_t nLength=(size_t)-1);

		bool Pos(size_t &nPos, wchar_t Ch, size_t nStartPos=0) const;
		bool Pos(size_t &nPos, const wchar_t *lpwszFind, size_t nStartPos=0) const;
		bool PosI(size_t &nPos, const wchar_t *lpwszFind, size_t nStartPos=0) const;
		bool RPos(size_t &nPos, wchar_t Ch, size_t nStartPos=0) const;

		bool Contains(wchar_t Ch, size_t nStartPos=0) const { return wcschr(m_pData->GetData()+nStartPos,Ch)==nullptr ? false : true; }
		bool Contains(const wchar_t *lpwszFind, size_t nStartPos=0) const { return wcsstr(m_pData->GetData()+nStartPos,lpwszFind)==nullptr ? false : true; }
} string;
