
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
#include "InQueue.h"
#include "Common.h"
#include <vector>

namespace {
std::vector<INPUT_RECORD> MakeRecords(const wchar_t* chars)
{
	std::vector<INPUT_RECORD> result;
	if (chars)
	{
		const auto len = wcslen(chars);
		result.reserve(len);
		for (const auto* pch = chars; *pch; ++pch)
		{
			INPUT_RECORD ir{};
			ir.EventType = KEY_EVENT;
			ir.Event.KeyEvent.bKeyDown = true;
			ir.Event.KeyEvent.uChar.UnicodeChar = *pch;
			result.push_back(ir);
		}
	}
	return result;
}
}

inline bool operator==(const INPUT_RECORD& r1, const INPUT_RECORD& r2)
{
	return memcmp(&r1, &r2, sizeof(r1)) == 0;
}

TEST(InQueue, ReadWrite)
{
	const int queueMax = 10;
	InQueue queue{};
	const auto writeRecords = MakeRecords(L"abcdefgh");
	std::vector<INPUT_RECORD> readRecords; readRecords.resize(queueMax);
	DWORD readCount{};
	
	queue.Initialize(queueMax, nullptr);

	EXPECT_TRUE(queue.IsInputQueueEmpty());
	EXPECT_FALSE(queue.ReadInputQueue(&readRecords[0], &(readCount = 1), false));
	
	EXPECT_TRUE(queue.WriteInputQueue(&writeRecords[0], false, 1));
	EXPECT_FALSE(queue.IsInputQueueEmpty());
	
	EXPECT_TRUE(queue.ReadInputQueue(&readRecords[0], &(readCount = 1), true));
	EXPECT_EQ(readCount, 1);
	EXPECT_EQ(readRecords[0], writeRecords[0]);
	EXPECT_FALSE(queue.IsInputQueueEmpty());
	EXPECT_TRUE(queue.ReadInputQueue(&readRecords[0], &(readCount = 1), false));
	EXPECT_EQ(readCount, 1);
	EXPECT_EQ(readRecords[0], writeRecords[0]);
	EXPECT_TRUE(queue.IsInputQueueEmpty());
	EXPECT_FALSE(queue.ReadInputQueue(&readRecords[0], &(readCount = 1), false));

	EXPECT_TRUE(queue.WriteInputQueue(&writeRecords[0], false, static_cast<DWORD>(writeRecords.size())));
	EXPECT_FALSE(queue.IsInputQueueEmpty());
	EXPECT_TRUE(queue.ReadInputQueue(&readRecords[0], &(readCount = 4), false));
	EXPECT_EQ(readCount, 4);
	EXPECT_FALSE(queue.IsInputQueueEmpty());
	EXPECT_TRUE(queue.ReadInputQueue(&readRecords[4], &(readCount = 6), false));
	EXPECT_EQ(readCount, 4);
	EXPECT_TRUE(queue.IsInputQueueEmpty());
	readRecords.resize(8);
	EXPECT_EQ(readRecords, writeRecords);

	// At the moment buffer has 1 record before end pointer, and it's actually empty
	EXPECT_TRUE(queue.IsInputQueueEmpty());
	EXPECT_TRUE(queue.WriteInputQueue(&writeRecords[0], false, static_cast<DWORD>(writeRecords.size())));
	EXPECT_FALSE(queue.IsInputQueueEmpty());
	readRecords.resize(10);
	EXPECT_TRUE(queue.ReadInputQueue(&readRecords[0], &(readCount = 8), false));
	EXPECT_EQ(readCount, 1); // this should be fixed by finish reading from the buffer start
	EXPECT_FALSE(queue.IsInputQueueEmpty());
	EXPECT_TRUE(queue.ReadInputQueue(&readRecords[1], &(readCount = 8), false));
	EXPECT_EQ(readCount, 7);
	EXPECT_TRUE(queue.IsInputQueueEmpty());
	readRecords.resize(8);
	EXPECT_EQ(readRecords, writeRecords);

	queue.Release();
}
