#include "tests.h"
#include <gtest/gtest.h>
#include "../common/wcwidth.h"
#include "../ConEmu/helper.h"

HWND ghWnd = NULL;
HINSTANCE g_hInstance = NULL;
bool gbMessagingStarted = false;
namespace FastConfig
{
HWND ghFastCfg = NULL;
}

// TODO: remove when finished
void initMainThread() {};

int MsgBox(LPCTSTR lpText, UINT uType, LPCTSTR lpCaption /*= NULL*/, HWND ahParent /*= (HWND)-1*/, bool abModal /*= true*/)
{
	return IDCANCEL;
}

void AssertBox(LPCTSTR szText, LPCTSTR szFile, UINT nLine, LPEXCEPTION_POINTERS ExceptionInfo /*= NULL*/)
{
}

bool isCharAltFont(ucs32 inChar)
{
	return false;
}

TEST(ConEmuTest, Main)
{
	const int val = 1;
	EXPECT_EQ(val, 1);
}

int main(int argc, char** argv)
{
	HeapInitialize();
	initMainThread();
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
