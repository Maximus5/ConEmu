
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
#include "../common/CEStr.h"
#include "test_mock_file.h"
#include "gtest.h"

using test_mocks::FileSystemMock;

thread_local FileSystemMock* gpFileSystemMock = nullptr;

bool FileExistsMock(const wchar_t* asFilePath, uint64_t* pnSize /*= nullptr*/, bool& result)
{
	if (!gpFileSystemMock)
		return false;

	result = false;

	if (!asFilePath || !*asFilePath)
	{
		EXPECT_TRUE(asFilePath != nullptr && *asFilePath != L'\0');
		return true;
	}

	if (const auto* fileInfo = gpFileSystemMock->HasFilePath(asFilePath))
	{
		if (pnSize)
			*pnSize = fileInfo->size;
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

bool SearchPathMock(LPCWSTR path, LPCWSTR fileName, LPCWSTR extension, CEStr& resultPath, int& rc)
{
	if (!gpFileSystemMock)
		return false;

	rc = 0;

	if (!fileName || !*fileName)
	{
		EXPECT_TRUE(fileName != nullptr && *fileName != L'\0');
		return true;
	}

	if (path == nullptr || *path == L'\0')
	{
		const auto found = gpFileSystemMock->FindInPath(fileName, extension);
		if (!found.empty())
		{
			resultPath = found.c_str();
			rc = static_cast<int>(resultPath.GetLen());
		}
	}
	else
	{
		std::wstring filePath(path);
		if (filePath.back() != L'\\' && filePath.back() != L'/')
			filePath += L'\\';
		filePath.append(fileName);
		if (gpFileSystemMock->HasFilePath(filePath))
		{
			resultPath = filePath.c_str();
			rc = static_cast<int>(resultPath.GetLen());			
		}
		else if (extension && *extension == L'.')
		{
			EXPECT_TRUE(extension[0] == L'.' && extension[1] != '\0');
			filePath.append(extension);
			if (gpFileSystemMock->HasFilePath(filePath))
			{
				resultPath = filePath.c_str();
				rc = static_cast<int>(resultPath.GetLen());
			}
		}
	}

	return true;
}

bool FindImageSubsystemMock(const wchar_t *module, DWORD& imageSubsystem, DWORD& imageBits, LPDWORD fileAttrs, bool& result)
{
	if (const auto* fileInfo = gpFileSystemMock->HasFilePath(module))
	{
		imageSubsystem = fileInfo->subsystem;
		imageBits = fileInfo->bitness;
		if (fileAttrs)
			*fileAttrs = FILE_ATTRIBUTE_NORMAL;
		result = true;
		return true;
	}

	return false;
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

void FileSystemMock::MockFile(const std::wstring& filePath, const uint64_t size, const uint32_t subsystem, const uint32_t bitness)
{
	files_.insert({ MakeCanonic(filePath), FileInfo{size, subsystem, bitness} });
}

void FileSystemMock::MockDirectory(const std::wstring& directoryPath)
{
	directories_.insert(MakeCanonic(directoryPath));
}

void FileSystemMock::MockPathFile(const std::wstring& fileName, const std::wstring& filePath, const uint64_t size, const uint32_t subsystem, const uint32_t bitness)
{
	auto canonicPath = MakeCanonic(filePath);
	fileToPath_.insert({ MakeCanonic(fileName), canonicPath });
	files_.insert({ std::move(canonicPath), FileInfo{size, subsystem, bitness} });
}

const FileSystemMock::FileInfo* FileSystemMock::HasFilePath(const std::wstring& filePath) const
{
	const auto found = files_.find(MakeCanonic(filePath));
	if (found == files_.end())
		return nullptr;
	return &found->second;
}

bool FileSystemMock::HasDirectoryPath(const std::wstring& directoryPath) const
{
	return directories_.count(MakeCanonic(directoryPath)) > 0;
}

std::wstring FileSystemMock::FindInPath(const std::wstring& fileName, const wchar_t* fileExtension) const
{
	std::wstring canonic(MakeCanonic(fileName));
	auto found = fileToPath_.find(canonic);
	if (found == fileToPath_.end() && fileExtension && *fileExtension)
	{
		EXPECT_TRUE(fileExtension[0] == L'.' && fileExtension[1] != L'\0');
		canonic += MakeCanonic(fileExtension);
		found = fileToPath_.find(canonic);
	}
	return (found == fileToPath_.end()) ? std::wstring() : found->second;
}

std::wstring FileSystemMock::MakeCanonic(const std::wstring& filePath)
{
	std::wstring canonic(filePath);
	for (auto& c : canonic)
	{
		if (c == L'/')
			c = L'\\';
		else
			c = tolower(c);
	}
	return canonic;
}
