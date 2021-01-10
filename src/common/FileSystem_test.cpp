
/*
Copyright (c) 2009-present Maximus5
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

#include "defines.h"
#include "../UnitTests/gtest.h"
#include "Common.h"
#include "EnvVar.h"
#include "WObjects.h"


TEST(FileExistsSearch, ByEnvVar)
{
	CEStr found;
	EXPECT_TRUE(FileExistsSearch(L"%windir%\\system32\\cmd.exe", found));
	const CEStr cmdPath(ExpandEnvStr(L"%windir%\\system32\\cmd.exe"));
	EXPECT_STREQ(found.c_str(), cmdPath.c_str());
}

TEST(FileExistsSearch, ByFullPath)
{
	CEStr found;
	const CEStr cmdPath(ExpandEnvStr(L"%windir%\\system32\\cmd.exe"));
	EXPECT_TRUE(FileExistsSearch(cmdPath, found));
	EXPECT_STREQ(found.c_str(), cmdPath.c_str());
	EXPECT_TRUE(FileExistsSearch(found.c_str(), found));
	EXPECT_STREQ(found.c_str(), cmdPath.c_str());
}

TEST(FileExistsSearch, SearchInDir)
{
	CEStr found;
	const CEStr cmdPath(ExpandEnvStr(L"%windir%\\system32\\cmd.exe"));
	const CEStr cmdNoExt(ExpandEnvStr(L"%windir%\\system32\\cmd"));
	EXPECT_FALSE(FileSearchInDir(L"%windir%\\system32\\cmd", found));
	EXPECT_TRUE(FileSearchInDir(cmdNoExt, found));
	EXPECT_STREQ(found.c_str(), cmdPath.c_str());
}
