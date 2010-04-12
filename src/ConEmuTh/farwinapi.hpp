#pragma once

/*
farwinapi.hpp

Враперы вокруг некоторых WinAPI функций
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

#include "plugin.hpp"
#include "UnicodeString.hpp"

#define NT_MAX_PATH 32768

struct FAR_FIND_DATA_EX
{
	DWORD    dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	unsigned __int64 nFileSize;

	unsigned __int64 nPackSize;
	struct
	{
		DWORD dwReserved0;
		DWORD dwReserved1;
	};

	string   strFileName;
	string   strAlternateFileName;

	void Clear()
	{
		dwFileAttributes=0;
		memset(&ftCreationTime,0,sizeof(ftCreationTime));
		memset(&ftLastAccessTime,0,sizeof(ftLastAccessTime));
		memset(&ftLastWriteTime,0,sizeof(ftLastWriteTime));
		nFileSize=0;
		nPackSize=0;
		dwReserved0=0;
		dwReserved1=0;
		strFileName.Clear();
		strAlternateFileName.Clear();
	}

	FAR_FIND_DATA_EX& operator=(const FAR_FIND_DATA_EX &ffdexCopy)
	{
		if (this != &ffdexCopy)
		{
			dwFileAttributes=ffdexCopy.dwFileAttributes;
			ftCreationTime=ffdexCopy.ftCreationTime;
			ftLastAccessTime=ffdexCopy.ftLastAccessTime;
			ftLastWriteTime=ffdexCopy.ftLastWriteTime;
			nFileSize=ffdexCopy.nFileSize;
			nPackSize=ffdexCopy.nPackSize;
			dwReserved0=ffdexCopy.dwReserved0;
			dwReserved1=ffdexCopy.dwReserved1;
			strFileName=ffdexCopy.strFileName;
			strAlternateFileName=ffdexCopy.strAlternateFileName;
		}

		return *this;
	}
};

class FindFile
{
public:
	FindFile(LPCWSTR Object, bool ScanSymLink = true);
	~FindFile();
	bool Get(FAR_FIND_DATA_EX& FindData);

private:
	HANDLE Handle;
	bool empty;
	bool admin;
	WIN32_FIND_DATA W32FindData;
};

class File
{
public:
	File();
	~File();
	bool Open(LPCWSTR Object, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDistribution, DWORD dwFlagsAndAttributes=0, HANDLE hTemplateFile=nullptr);
	bool Read(LPVOID Buffer, DWORD NumberOfBytesToRead, LPDWORD NumberOfBytesRead, LPOVERLAPPED Overlapped = nullptr) const;
	bool Write(LPCVOID Buffer, DWORD NumberOfBytesToWrite, LPDWORD NumberOfBytesWritten, LPOVERLAPPED Overlapped = nullptr) const;
	bool SetPointer(INT64 DistanceToMove, PINT64 NewFilePointer, DWORD MoveMethod);
	bool SetEnd();
	bool GetTime(LPFILETIME CreationTime, LPFILETIME LastAccessTime, LPFILETIME LastWriteTime);
	bool SetTime(const FILETIME* CreationTime, const FILETIME* LastAccessTime, const FILETIME* LastWriteTime);
	bool GetSize(UINT64& Size);
	bool IoControl(DWORD IoControlCode, LPVOID InBuffer, DWORD InBufferSize, LPVOID OutBuffer, DWORD OutBufferSize, LPDWORD BytesReturned, LPOVERLAPPED Overlapped = nullptr);
	bool Close();

private:
	HANDLE Handle;
	bool admin;
};

NTSTATUS GetLastNtStatus();

DWORD apiGetEnvironmentVariable(
    const wchar_t *lpwszName,
    string &strBuffer
);

DWORD apiGetCurrentDirectory(
    string &strCurDir
);

DWORD apiGetTempPath(
    string &strBuffer
);

DWORD apiGetModuleFileName(
    HMODULE hModule,
    string &strFileName
);

DWORD apiExpandEnvironmentStrings(
    const wchar_t *src,
    string &strDest
);

DWORD apiGetConsoleTitle(
    string &strConsoleTitle
);

DWORD apiWNetGetConnection(
    const wchar_t *lpwszLocalName,
    string &strRemoteName
);

BOOL apiGetVolumeInformation(
    const wchar_t *lpwszRootPathName,
    string *pVolumeName,
    LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength,
    LPDWORD lpFileSystemFlags,
    string *pFileSystemName
);

BOOL apiFindStreamClose(
    HANDLE hFindFile
);

void apiFindDataToDataEx(
    const FAR_FIND_DATA *pSrc,
    FAR_FIND_DATA_EX *pDest);

void apiFindDataExToData(
    const FAR_FIND_DATA_EX *pSrc,
    FAR_FIND_DATA *pDest
);

void apiFreeFindData(
    FAR_FIND_DATA *pData
);

BOOL apiGetFindDataEx(
    const wchar_t *lpwszFileName,
    FAR_FIND_DATA_EX& FindData,
    bool ScanSymLink=true);

bool apiGetFileSizeEx(
    HANDLE hFile,
    UINT64 &Size);

//junk

BOOL apiDeleteFile(
    const wchar_t *lpwszFileName
);

BOOL apiRemoveDirectory(
    const wchar_t *DirName
);

HANDLE apiCreateFile(
    const wchar_t *lpwszFileName,     // pointer to name of the file
    DWORD dwDesiredAccess,  // access (read-write) mode
    DWORD dwShareMode,      // share mode
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, // pointer to security attributes
    DWORD dwCreationDistribution, // how to create
    DWORD dwFlagsAndAttributes,   // file attributes
    HANDLE hTemplateFile=nullptr          // handle to file with attributes to copy
);

BOOL apiCopyFileEx(
    const wchar_t *lpExistingFileName,
    const wchar_t *lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine,
    LPVOID lpData,
    LPBOOL pbCancel,
    DWORD dwCopyFlags
);

BOOL apiMoveFile(
    const wchar_t *lpwszExistingFileName, // address of name of the existing file
    const wchar_t *lpwszNewFileName   // address of new name for the file
);

BOOL apiMoveFileEx(
    const wchar_t *lpwszExistingFileName, // address of name of the existing file
    const wchar_t *lpwszNewFileName,   // address of new name for the file
    DWORD dwFlags   // flag to determine how to move file
);

int apiRegEnumKeyEx(
    HKEY hKey,
    DWORD dwIndex,
    string &strName,
    PFILETIME lpftLastWriteTime=nullptr
);

BOOL apiMoveFileThroughTemp(
    const wchar_t *Src,
    const wchar_t *Dest
);

BOOL apiIsDiskInDrive(
    const wchar_t *Root
);

int apiGetFileTypeByName(
    const wchar_t *Name
);

BOOL apiGetDiskSize(
    const wchar_t *Path,
    unsigned __int64 *TotalSize,
    unsigned __int64 *TotalFree,
    unsigned __int64 *UserFree
);

BOOL apiGetConsoleKeyboardLayoutName(
    string &strDest
);

HANDLE apiFindFirstFileName(
    LPCWSTR lpFileName,
    DWORD dwFlags,
    string& strLinkName
);

BOOL apiFindNextFileName(
    HANDLE hFindStream,
    string& strLinkName
);

BOOL apiCreateDirectory(
    LPCWSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
);

DWORD apiGetFileAttributes(
    LPCWSTR lpFileName
);

BOOL apiSetFileAttributes(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
);

void InitCurrentDirectory();

BOOL apiSetCurrentDirectory(
    LPCWSTR lpPathName,
    bool Validate = true
);

// for AdminMode only, dont' use outside.
BOOL CreateSymbolicLinkInternal(LPCWSTR Object,LPCWSTR Target, DWORD dwFlags);

BOOL apiCreateSymbolicLink(
    LPCWSTR lpSymlinkFileName,
    LPCWSTR lpTargetFileName,
    DWORD dwFlags
);

bool apiGetCompressedFileSize(
    LPCWSTR lpFileName,
    UINT64& Size
);

BOOL apiCreateHardLink(
    LPCWSTR lpFileName,
    LPCWSTR lpExistingFileName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
);

BOOL apiSetFilePointerEx(
    HANDLE hFile,
    INT64 DistanceToMove,
    PINT64 NewFilePointer,
    DWORD dwMoveMethod
);

HANDLE apiFindFirstStream(
    LPCWSTR lpFileName,
    STREAM_INFO_LEVELS InfoLevel,
    LPVOID lpFindStreamData,
    DWORD dwFlags=0
);

BOOL apiFindNextStream(
    HANDLE hFindStream,
    LPVOID lpFindStreamData
);

bool apiGetLogicalDriveStrings(
    string& DriveStrings
);

bool apiGetFinalPathNameByHandle(
    HANDLE hFile,
    string& FinalFilePath
);

bool apiSearchPath(
	const wchar_t *Path,
	const wchar_t *FileName,
	const wchar_t *Extension,
	string &strDest
);

bool apiQueryDosDevice(
	const wchar_t *DeviceName,
	string& Path
);

bool apiGetVolumeNameForVolumeMountPoint(
	LPCWSTR VolumeMountPoint,
	string& strVolumeName
);
