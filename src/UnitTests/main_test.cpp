//#include "tests.h"
#include <iostream>
#include <gtest/gtest.h>

namespace {
TEST(ConEmuTest, Main) {
	const int val = 1;
	EXPECT_EQ(val, 1);
}
}

int main(int argc, char** argv) {
	std::cout << "Entering main" << std::endl;
	//HeapInitialize();
	std::cout << "Heap Initialized" << std::endl;
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
