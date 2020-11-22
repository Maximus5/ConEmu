
/*
Copyright (c) 2020-present Maximus5
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

#include "../common/Common.h"
#include "test_mock_file.h"

using test_mocks::FileSystemMock;

thread_local FileSystemMock* gpFileSystemMock = nullptr;

bool FileExistsMock(const wchar_t* asFilePath, uint64_t* pnSize /*= nullptr*/, bool& result)
{
	if (!gpFileSystemMock)
		return false;

	if (gpFileSystemMock->HasFilePath(asFilePath))
	{
		if (pnSize)
			*pnSize = 512;
		result = true;
	}
	else if (gpFileSystemMock->HasDirectoryPath(asFilePath))
	{
		if (pnSize)
			*pnSize = 0;
		result = true;
	}

	return true;
}


FileSystemMock::FileSystemMock()
{
	gpFileSystemMock = this;
}

FileSystemMock::~FileSystemMock()
{
	gpFileSystemMock = nullptr;
}

void FileSystemMock::Reset()
{
	files_.clear();
	directories_.clear();
}

void FileSystemMock::MockFile(const wchar_t* filePath)
{
	files_.insert(MakeCanonic(filePath));
}

void FileSystemMock::MockDirectory(const wchar_t* directoryPath)
{
	directories_.insert(MakeCanonic(directoryPath));
}

bool FileSystemMock::HasFilePath(const wchar_t* filePath) const
{
	return files_.count(MakeCanonic(filePath)) > 0;
}

bool FileSystemMock::HasDirectoryPath(const wchar_t* directoryPath) const
{
	return directories_.count(MakeCanonic(directoryPath)) > 0;
}

std::wstring FileSystemMock::MakeCanonic(const wchar_t* filePath)
{
	std::wstring canonic(filePath);
	for (auto slash = canonic.find(L'/'); slash != std::wstring::npos; slash = canonic.find(L'/', slash))
	{
		canonic[slash] = L'\\';
	}
	return canonic;
}
