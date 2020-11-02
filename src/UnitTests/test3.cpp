
#include <windows.h>
#include <cstdio>

struct Handle
{
	HANDLE h_ = nullptr;
	operator HANDLE() const { return h_; }
	Handle(HANDLE h) noexcept : h_(h) {}
	bool IsValid() const { return h_ != nullptr && h_ != INVALID_HANDLE_VALUE; }
};

int Fail(const char* label, const int result)
{
	char msg[256] = "";
	sprintf_s(msg, "%s, code=%u", label, GetLastError());
	MessageBoxA(nullptr, msg, "ScreenBufferTest", MB_OK | MB_SYSTEMMODAL | MB_ICONASTERISK);
	return result;
}

void WaitKey(const WORD vk)
{
	const Handle h(GetStdHandle(STD_INPUT_HANDLE));
	for (;;)
	{
		INPUT_RECORD ir{}; DWORD read = 0;
		if (!ReadConsoleInput(h, &ir, 1, &read) || !read)
			break;
		if (ir.EventType != KEY_EVENT)
			continue;
		if (ir.Event.KeyEvent.wVirtualKeyCode != vk)
			continue;
		if (!ir.Event.KeyEvent.bKeyDown)
			continue;
		break;
	}
}

bool WriteText(const Handle& h, const char* text)
{
	DWORD written = 0;
	if (!WriteConsoleA(h, text, DWORD(strlen(text)), &written, nullptr))
		return false;
	return true;
}

int main()
{
	const Handle origStd(GetStdHandle(STD_OUTPUT_HANDLE));
	if (!WriteText(origStd, "Press <Enter> to create and change screen buffer\n"))
		return Fail("Write failed", 1);
	WaitKey(VK_RETURN);
	CONSOLE_SCREEN_BUFFER_INFO origSbi{};
	if (!GetConsoleScreenBufferInfo(origStd, &origSbi))
		return Fail("GetConsoleScreenBufferInfo(origStd) failed", 1);
	
	const Handle newBuffer(CreateConsoleScreenBuffer(
		GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr, CONSOLE_TEXTMODE_BUFFER, nullptr));
	if (!newBuffer.IsValid())
		return Fail("CreateConsoleScreenBuffer failed, code=%u", 1);
	const COORD newSize = { SHORT(origSbi.srWindow.Right - origSbi.srWindow.Left + 1), SHORT(origSbi.srWindow.Bottom - origSbi.srWindow.Top + 1) };
	if (!SetConsoleScreenBufferSize(newBuffer, newSize))
		return Fail("Set new buffer size failed", 1);
	if (!SetConsoleActiveScreenBuffer(newBuffer))
		return Fail("SetConsoleActiveScreenBuffer(newBuffer) failed, code=%u", 1);
	for (int i = 1; i <= newSize.Y * 2; ++i)
	{
		char line[80];
		sprintf_s(line, "%i: The quick brown fox jumps over the lazy dog\n", i);
		if (!WriteText(newBuffer, line))
			return Fail("Write failed", 1);
	}

	if (!WriteText(origStd, "Press <Enter> to go back to original screen\n"))
		return Fail("Write failed", 1);
	WaitKey(VK_RETURN);

	if (!SetConsoleActiveScreenBuffer(origStd))
		return Fail("Set new buffer size failed", 1);

	CloseHandle(newBuffer);

	return 0;
}
