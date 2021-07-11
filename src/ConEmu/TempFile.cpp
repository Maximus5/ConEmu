
/*
Copyright (c) 2021-present Maximus5
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
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


#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"
#include "../common/EnvVar.h"
#include "../common/MStrSafe.h"
#include "../common/WFiles.h"
#include "TempFile.h"

namespace {
void ValidatePath(const CEStr& path, LPCWSTR asDir)
{
	if (path.IsEmpty())
	{
		throw ErrorInfo(L"CreateTempFile.asDir(%s) failed, path is null", asDir, ERROR_NOT_ENOUGH_MEMORY);
	}

	const wchar_t* pszColon1;
	if ((pszColon1 = wcschr(path, L':')) != nullptr)
	{
		// ReSharper disable once CppEntityAssignedButNoRead
		const wchar_t* pszColon2;
		if ((pszColon2 = wcschr(pszColon1 + 1, L':')) != nullptr)
		{
			throw ErrorInfo(L"Invalid download path (%%TEMP%% variable?)\n%s", path.c_str(), 0);
		}
	}
}
}

TempFile::TempFile()
{
}

TempFile::TempFile(LPCWSTR asDir, LPCWSTR asFileNameTempl)
{
	CreateTempFile(asDir, asFileNameTempl);
}

TempFile::~TempFile()
{
	Release(true);
}

ErrorInfo TempFile::CreateTempFile(LPCWSTR asDir, LPCWSTR asFileNameTempl)
{
	try {
		Release();

		CEStr szDir;
		//CEStr szFile;
		//wchar_t szName[128];

		if (!asDir || !*asDir)
			asDir = L"%TEMP%\\ConEmu";
		if (!asFileNameTempl || !*asFileNameTempl)
			asFileNameTempl = L"ConEmu.tmp";

		if (wcschr(asDir, L'%'))
		{
			szDir = ExpandEnvStr(asDir);
			if (szDir.IsEmpty())
			{
				throw ErrorInfo(L"CreateTempFile.ExpandEnvironmentStrings(%s) failed, code=%u", asDir, GetLastError());
			}
		}
		else
		{
			szDir.Set(asDir);
		}

		// Checking %TEMP% for valid path
		ValidatePath(szDir, asDir);

		LPCWSTR pszName = PointToName(asFileNameTempl);
		_ASSERTE(pszName == asFileNameTempl);
		if (!pszName || !*pszName || (*pszName == L'.'))
		{
			_ASSERTE(pszName && *pszName && (*pszName != L'.'));
			pszName = L"ConEmu";
		}

		LPCWSTR pszExt = PointToExt(pszName);
		if (pszExt == nullptr)
		{
			_ASSERTE(pszExt != nullptr);
			pszExt = L".tmp";
		}

		const ssize_t cchNameMax = 128;
		CEStr szName;
		szName.GetBuffer(cchNameMax);
		szName.Set(pszName, cchNameMax - 16);
		wchar_t* psz = wcsrchr(szName.data(), L'.');
		if (psz)
			*psz = 0;

		DWORD dwErr = 0;
		bool fileCreated = false;

		for (UINT i = 0; i <= 9999 && !fileCreated; i++)
		{
			wchar_t fileIndex[16] = L"";
			if (i)
				swprintf_c(fileIndex, countof(fileIndex), L"(%u)", i);
			const CEStr szFilePart(szName, fileIndex, pszExt);

			path_ = JoinPath(szDir, szFilePart);
			if (!path_)
			{
				throw ErrorInfo(L"Can't allocate memory (%i bytes)", static_cast<unsigned>(szDir.GetLen() + szFilePart.GetLen()));
			}

			SECURITY_ATTRIBUTES sec = { sizeof(sec), nullptr, TRUE };
			fileHandle_.SetHandle(
				CreateFileW(path_, GENERIC_WRITE, FILE_SHARE_READ, &sec, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY, nullptr),
				MHandle::CloseHandle);
			if (!fileHandle_)
			{
				dwErr = GetLastError();
				if ((dwErr == ERROR_PATH_NOT_FOUND) && (i == 0))
				{
					if (!MyCreateDirectory(szDir.data()))
					{
						throw ErrorInfo(L"CreateTempFile.asDir(%s) failed", asDir, 0);
					}

					fileHandle_.SetHandle(
						CreateFileW(path_, GENERIC_WRITE, FILE_SHARE_READ, &sec, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY, nullptr),
						MHandle::CloseHandle);
				}
			}

			if (fileHandle_.HasHandle())
			{
				fileCreated = true;
				break;
			}
		}

		if (!fileCreated)
		{
			throw ErrorInfo(L"Can't create temp file(%s), code=%u", path_, dwErr);
		}

		return ErrorInfo();
	}
	catch (const ErrorInfo& ex)
	{
		Release();
		return ex;
	}
}

bool TempFile::IsValid() const
{
	return !path_.IsEmpty();
}

void TempFile::Release(const bool deleteFile)
{
	CloseFile();

	if (deleteFile)
	{
		const CEStr fileToDelete(std::move(path_));
		if (!fileToDelete.IsEmpty())
		{
			::DeleteFileW(fileToDelete.c_str());
		}
	}
	path_.Release();
}

void TempFile::CloseFile()
{
	if (fileHandle_)
	{
		fileHandle_.Close();
	}
}

HANDLE TempFile::GetHandle() const
{
	return fileHandle_.GetHandle();
}

const wchar_t* TempFile::GetFilePath() const
{
	return path_.c_str(L"");
}
