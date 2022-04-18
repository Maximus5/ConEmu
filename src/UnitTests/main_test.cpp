
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

#include <gtest/gtest.h>
#include <vector>
#include <string>

namespace conemu {
namespace tests {
void PrepareGoogleTests();
int RunLineFeedTest();
int RunFishPromptTest();
int RunLineWrapWin11Test();
int RunLineFeedTestXTerm();
int RunLineFeedTestParent();
int RunLineFeedTestChild();
int RunXTermTestChild();
int RunXTermTestParent();
std::vector<std::string> gTestArgs;
}  // namespace tests
}  // namespace conemu

// *******************
TEST(ConEmuTest, Main)
{
	const int val = 1;
	EXPECT_EQ(val, 1);
}

int main(int argc, char** argv)
{
	conemu::tests::PrepareGoogleTests();

	for (int i = 0; i < argc; ++i)
	{
		if (argv[i] && strcmp(argv[i], "RunLineFeedTest") == 0)
			return conemu::tests::RunLineFeedTest();
		if (argv[i] && strcmp(argv[i], "RunFishPromptTest") == 0)
			return conemu::tests::RunFishPromptTest();
		if (argv[i] && strcmp(argv[i], "RunLineWrapWin11Test") == 0)
			return conemu::tests::RunLineWrapWin11Test();
		if (argv[i] && strcmp(argv[i], "RunLineFeedTestXTerm") == 0)
			return conemu::tests::RunLineFeedTestXTerm();
		if (argv[i] && strcmp(argv[i], "RunLineFeedTestParent") == 0)
			return conemu::tests::RunLineFeedTestParent();
		if (argv[i] && strcmp(argv[i], "RunLineFeedTestChild") == 0)
			return conemu::tests::RunLineFeedTestChild();
		if (argv[i] && strcmp(argv[i], "RunXTermTestChild") == 0)
			return conemu::tests::RunXTermTestChild();
		if (argv[i] && strcmp(argv[i], "RunXTermTestParent") == 0)
			return conemu::tests::RunXTermTestParent();
	}

	::testing::InitGoogleTest(&argc, argv);
	conemu::tests::gTestArgs.reserve(argc);
	for (int i = 0; i < argc; ++i)
	{
		if (argv[i])
			conemu::tests::gTestArgs.emplace_back(argv[i]);
	}
	return RUN_ALL_TESTS();
}
