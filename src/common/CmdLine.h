
/*
Copyright (c) 2013 Maximus5
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

#pragma once

int NextArg(const wchar_t** asCmdLine, wchar_t (&rsArg)[MAX_PATH+1], const wchar_t** rsArgStart=NULL);
//int NextArg(const char** asCmdLine, char (&rsArg)[MAX_PATH+1], const char** rsArgStart=NULL);
const wchar_t* SkipNonPrintable(const wchar_t* asParams);
bool CompareFileMask(const wchar_t* asFileName, const wchar_t* asMask);
LPCWSTR GetDrive(LPCWSTR pszPath, wchar_t* szDrive, int/*countof(szDrive)*/ cchDriveMax);

bool IsExecutable(LPCWSTR aszFilePathName, wchar_t** rsExpandedVars = NULL);
bool IsFarExe(LPCWSTR asModuleName);
BOOL IsNeedCmd(BOOL bRootCmd, LPCWSTR asCmdLine, LPCWSTR* rsArguments, BOOL *rbNeedCutStartEndQuot,
			   wchar_t (&szExe)[MAX_PATH+1],
			   BOOL& rbRootIsCmdExe, BOOL& rbAlwaysConfirmExit, BOOL& rbAutoDisableConfirmExit);
