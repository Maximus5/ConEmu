// test_scroll.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define NO_MINMAX
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <Windows.h>

struct Handle
{
	HANDLE h_ = nullptr;
	// ReSharper disable once CppNonExplicitConversionOperator
	operator HANDLE() const { return h_; }
	// ReSharper disable once CppNonExplicitConvertingConstructor
	// ReSharper disable once CppParameterMayBeConst
	Handle(HANDLE h) noexcept : h_(h) {}
	bool IsValid() const { return h_ != nullptr && h_ != INVALID_HANDLE_VALUE; }
};

std::wstring RgbBack(const unsigned r, const unsigned g, const unsigned b)
{
	return L"\x1B[48;2;" + std::to_wstring(r) + L";" + std::to_wstring(g) + L";" + std::to_wstring(b) + L"m";
}

std::wstring RgbReset()
{
	return L"\x1B[m";
}

class ExitException final : public std::exception
{
public:
	using std::exception::exception;
};

std::string ToLower(std::string str)
{
	for (auto& c : str)
	{
		c = std::tolower(c);
	}
	return str;
}

class Framework
{
	const Handle hConIn = GetStdHandle(STD_INPUT_HANDLE);
	const Handle hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	unsigned lastRow = 0;
	unsigned lastScroll = 0;
	enum class Clipping { None, View, Smaller } clipping = Clipping::None;
	enum class ConApi { WinApi, Ansi } conApi = ConApi::Ansi;
	bool altBuffer = false;

	struct Action
	{
		WORD vk;
		std::string key;
		std::string description;
		std::function<void()> action;
	};
	std::vector<Action> actions;
	std::unordered_map<std::string, size_t> keyMap;
	std::unordered_map<WORD, size_t> vkMap;

	void AddAction(const WORD vk, std::string key, std::string description, std::function<void()> action)
	{
		const size_t idx = actions.size();
		actions.emplace_back(Action{vk, std::move(key), std::move(description), std::move(action)});
		const auto keyLower = ToLower(actions[idx].key);
		keyMap[keyLower] = idx;
		vkMap[vk] = idx;
	}

	void InitActions()
	{
		AddAction(VK_F1, "F1", "print this help",
			[this]()
			{
				PrintHelp();
			});
		AddAction(VK_F2, "F2", "fill current view",
			[this]()
			{
				FillCurrentView();
			});
		AddAction(VK_F3, "F3", "switch use of WinAPI/ANSI",
			[this]()
			{
				conApi = (conApi == ConApi::WinApi) ? ConApi::Ansi : ConApi::WinApi;
				UpdateTitle();
			});
		AddAction(VK_F4, "F4", "switch clipping region None/View/Smaller",
			[this]()
			{
				clipping = (clipping < Clipping::Smaller) ? Clipping(static_cast<unsigned>(clipping) + 1) : Clipping::None;
				UpdateTitle();
			});
		AddAction(VK_F5, "F5", "switch alternative buffer",
			[this]()
			{
				SwitchAltBuffer();
			});
		AddAction(VK_F8, "F8", "erase the whole buffer",
			[this]()
			{
				Erase();
			});
		AddAction(VK_F9, "F9", "run MS example https://docs.microsoft.com/en-us/windows/console/scrolling-a-screen-buffer-s-contents",
			[this]()
			{
				MsExample();
			});
		AddAction(VK_UP, "Up", "scrolls the buffer upwards",
			[this]()
			{
				DoScroll(-1);
			});
		AddAction(VK_DOWN, "Down", "scrolls the buffer downwards",
			[this]()
			{
				DoScroll(+1);
			});
		AddAction(VK_PRIOR, "PgUp", "set viewport -1 Y coordinate (scroll window)",
			[this]()
			{
				GoPrevLine();
			});
		AddAction(VK_NEXT, "PgDn", "set viewport +1 Y coordinate (scroll window)",
			[this]()
			{
				GoNextLine();
			});
		AddAction(VK_HOME, "Home", "set viewport to start of the scroll buffer",
			[this]()
			{
				GoHome();
			});
		AddAction(VK_END, "End", "set viewport to the end of the scroll buffer",
			[this]()
			{
				GoEnd();
			});
		AddAction(VK_F10, "F10", "Exit",
			[this]()
			{
				Erase();
				throw ExitException();
			});
	}

public:
	void Run(const int argc, char** argv)
	{
		DWORD mode{}; GetConsoleMode(hConOut, &mode);
		SetConsoleMode(hConOut, 7);
		try {
			INPUT_RECORD input{};
			DWORD read;

			UpdateTitle();
			InitActions();
			PrintHelp();

			for (int i = 0; i < argc; ++i)
			{
				if (strcmp(argv[i], "--keys") != 0)
					continue;
				while (++i < argc)
				{
					ProcessKey(argv[i]);
				}
				break;
			}

			while (ReadConsoleInputW(hConIn, &input, 1, &read))
			{
				if (!read)
					continue;
				if (input.EventType == KEY_EVENT)
				{
					if (!ProcessKey(input.Event.KeyEvent))
						break;
				}
			}
		}
		catch (const ExitException&)
		{
			std::cout << std::endl << "F10 pressed, exiting" << std::endl;
		}
		SetConsoleMode(hConOut, mode);
	}

	void UpdateTitle() const
	{
		std::wstring title = L"Console test tool: ";
		switch (conApi)
		{
		case ConApi::WinApi:
			title += L"WinAPI"; break;
		case ConApi::Ansi:
			title += L"ANSI"; break;
		}
		title += L", ";
		switch (clipping)
		{
		case Clipping::None:
			title += L"NoClip"; break;
		case Clipping::View:
			title += L"ClipView"; break;
		case Clipping::Smaller:
			title += L"ClipSmaller"; break;
		}
		SetConsoleTitleW(title.c_str());
	}

	void PrintHelp() const
	{
		std::cout << std::endl
			<< "Scroll test tool. Use hotkeys:" << std::endl;
		for (const auto& action : actions)
		{
			std::cout << "  " << action.key << " " << action.description << std::endl;
		}
	}

	void FillCurrentView()
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi{};
		if (!GetConsoleScreenBufferInfo(hConOut, &csbi))
			throw std::system_error(GetLastError(), std::system_category(), "GetConsoleScreenBufferInfo failed");
		const size_t width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		const std::wstring fillChars =
			// ReSharper disable StringLiteralTypo
			L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~*ΐΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟ"
			L"ΣΤΥΦΧΨΩΪΫάέήίΰαβγδεζηθικλμνξοπρςστυφχψωϊϋόύώϏ"
			L"АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдежзийклмнопрстуфхцчшщъыьэюя";
			// ReSharper restore StringLiteralTypo
		size_t fillIdx = 0;
		std::vector<CHAR_INFO> line; line.reserve(width);
		CHAR_INFO ch{}; ch.Attributes = 7;
		SetConsoleTextAttribute(hConOut, ch.Attributes);
		const COORD bufferSize{ static_cast<SHORT>(width), 1 };
		const unsigned r = 42, g = 161, b = 152;
		for (auto y = csbi.srWindow.Top; y <= csbi.srWindow.Bottom; ++y)
		{
			line.clear();
			const auto lineNo = std::to_wstring((++lastRow) % 1000);
			for (auto c : lineNo)
			{
				ch.Char.UnicodeChar = c;
				line.push_back(ch);
			}
			ch.Char.UnicodeChar = L'0';
			while (line.size() < 3) line.insert(line.begin(), ch);
			while (line.size() < width)
			{
				ch.Char.UnicodeChar = L'·';
				line.push_back(ch);
				if (line.size() >= width) break;
				ch.Char.UnicodeChar = fillChars[(fillIdx++ % fillChars.size())];
				line.push_back(ch);
			}

			//if (y > csbi.srWindow.Top && y < csbi.srWindow.Bottom)
			if (conApi == ConApi::Ansi)
			{
				std::wstring lineStr; lineStr.reserve(line.size() + 64);
				const float mul = 0.4f + 0.6f * static_cast<float>(y - csbi.srWindow.Top) / static_cast<float>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
				lineStr += RgbBack(static_cast<unsigned>(r * mul), static_cast<unsigned>(g * mul), static_cast<unsigned>(b * mul));
				for (const auto& c : line)
				{
					lineStr += c.Char.UnicodeChar;
				}
				lineStr += RgbReset();
				SetConsoleCursorPosition(hConOut, COORD{0, y});
				DWORD written = 0;
				WriteConsoleW(hConOut, lineStr.c_str(), lineStr.length(), &written, nullptr);
			}
			else
			{
				SMALL_RECT rcOut{};
				rcOut.Top = rcOut.Bottom = y;
				rcOut.Right = csbi.srWindow.Right;
				WriteConsoleOutputW(hConOut, line.data(), bufferSize, COORD{}, &rcOut);
			}
		}
		SetConsoleCursorPosition(hConOut, csbi.dwCursorPosition);
	}

	void Erase() const
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi{};
		if (!GetConsoleScreenBufferInfo(hConOut, &csbi))
			throw std::system_error(GetLastError(), std::system_category(), "GetConsoleScreenBufferInfo failed");
		SMALL_RECT rcFull = { 0, 0, SHORT(csbi.dwSize.X - 1), SHORT(csbi.dwSize.Y - 1) };
		CHAR_INFO space{};
		space.Attributes = 7;
		space.Char.UnicodeChar = L' ';
		if (!ScrollConsoleScreenBufferW(hConOut, &rcFull, nullptr, COORD{ 0, -32767 }, &space))
			throw std::system_error(GetLastError(), std::system_category(), "ScrollConsoleScreenBufferW failed");
		if (!SetConsoleCursorPosition(hConOut, COORD{}))
			throw std::system_error(GetLastError(), std::system_category(), "SetConsoleCursorPosition failed");
	}

	void GoHome() const
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi{};
		if (!GetConsoleScreenBufferInfo(hConOut, &csbi))
			throw std::system_error(GetLastError(), std::system_category(), "GetConsoleScreenBufferInfo failed");
		const SHORT height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
		const SHORT width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		const SMALL_RECT newRect = { 0, 0, SHORT(width - 1), SHORT(height - 1) };
		if (!SetConsoleWindowInfo(hConOut, true, &newRect))
			throw std::system_error(GetLastError(), std::system_category(), "SetConsoleWindowInfo failed");
		if (!SetConsoleCursorPosition(hConOut, COORD{newRect.Left, newRect.Top}))
			throw std::system_error(GetLastError(), std::system_category(), "SetConsoleCursorPosition failed");
	}

	void GoEnd() const
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi{};
		if (!GetConsoleScreenBufferInfo(hConOut, &csbi))
			throw std::system_error(GetLastError(), std::system_category(), "GetConsoleScreenBufferInfo failed");
		const SHORT height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
		const SHORT width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		const SMALL_RECT newRect = { 0, SHORT(csbi.dwSize.Y - height), SHORT(width - 1), SHORT(csbi.dwSize.Y - 1) };
		if (!SetConsoleWindowInfo(hConOut, true, &newRect))
			throw std::system_error(GetLastError(), std::system_category(), "SetConsoleWindowInfo failed");
		if (!SetConsoleCursorPosition(hConOut, COORD{newRect.Left, newRect.Top}))
			throw std::system_error(GetLastError(), std::system_category(), "SetConsoleCursorPosition failed");
	}

	void GoPrevLine() const
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi{};
		if (!GetConsoleScreenBufferInfo(hConOut, &csbi))
			throw std::system_error(GetLastError(), std::system_category(), "GetConsoleScreenBufferInfo failed");
		SMALL_RECT newRect = csbi.srWindow;
		if (newRect.Top > 0)
		{
			--newRect.Bottom; --newRect.Top;
		}
		if (!SetConsoleWindowInfo(hConOut, true, &newRect))
			throw std::system_error(GetLastError(), std::system_category(), "SetConsoleWindowInfo failed");
	}

	void GoNextLine() const
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi{};
		if (!GetConsoleScreenBufferInfo(hConOut, &csbi))
			throw std::system_error(GetLastError(), std::system_category(), "GetConsoleScreenBufferInfo failed");
		SMALL_RECT newRect = csbi.srWindow;
		if (newRect.Bottom + 1 < csbi.dwSize.Y)
		{
			++newRect.Bottom; ++newRect.Top;
		}
		if (!SetConsoleWindowInfo(hConOut, true, &newRect))
			throw std::system_error(GetLastError(), std::system_category(), "SetConsoleWindowInfo failed");
	}

	void DoScroll(int direction)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi{};
		if (!GetConsoleScreenBufferInfo(hConOut, &csbi))
			throw std::system_error(GetLastError(), std::system_category(), "GetConsoleScreenBufferInfo failed");
		SMALL_RECT rcView = csbi.srWindow;

		if (conApi == ConApi::WinApi)
		{
			SMALL_RECT rcClip = csbi.srWindow;
			if (clipping == Clipping::Smaller)
			{
				rcClip.Left += 4;
				rcClip.Top += 4;
				rcClip.Right -= 4;
				rcClip.Bottom -= 4;
			}
			CHAR_INFO fill{};
			fill.Attributes = 8;
			fill.Char.UnicodeChar = (direction < 0) ? L'↑' : L'↓';
			if (!ScrollConsoleScreenBufferW(hConOut, &rcView, (clipping == Clipping::None) ? nullptr : &rcClip, COORD{ 0, SHORT(rcView.Top + direction) }, &fill))
				throw std::system_error(GetLastError(), std::system_category(), "ScrollConsoleScreenBufferW failed");
		}
		else
		{
			std::wstring scrollCommand;
			const unsigned xtermFrom = 232 + 4, xtermTo = 255 - 3;
			//scrollCommand += L"\x1B[100m";
			scrollCommand += L"\x1B[48;5;" + std::to_wstring(xtermFrom + ((++lastScroll) % (xtermTo - xtermFrom))) + L"m";
			switch (clipping)
			{
			case Clipping::None:
				scrollCommand += L"\x1B[r"; break;
			case Clipping::View:
				scrollCommand += L"\x1B[1;" + std::to_wstring(rcView.Bottom - rcView.Top + 1) + L"r"; break;
			case Clipping::Smaller:
				scrollCommand += L"\x1B[2;" + std::to_wstring(rcView.Bottom - rcView.Top) + L"r"; break;
			}
			scrollCommand += L"\x1B[" + std::to_wstring(std::abs(direction)) + ((direction < 0) ? L"S" : L"T");
			scrollCommand += L"\x1B[r"; // reset region
			scrollCommand += L"\x1B[m"; // reset text color

			DWORD written = 0;
			WriteConsoleW(hConOut, scrollCommand.c_str(), scrollCommand.length(), &written, nullptr);
		}
	}

	void SwitchAltBuffer()
	{
		altBuffer = !altBuffer;
		const std::wstring action = altBuffer ? L"\x1B[?1049h" : L"\x1B[?1049l";

		DWORD written = 0;
		WriteConsoleW(hConOut, action.c_str(), action.length(), &written, nullptr);
	}

	static void MsExample()
	{
		// ReSharper disable once CppJoinDeclarationAndAssignment
		HANDLE hStdout;
		CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
		// ReSharper disable twice IdentifierTypo
		SMALL_RECT srctScrollRect, srctClipRect;
		CHAR_INFO chiFill;
		COORD coordDest;
		// ReSharper disable once CppJoinDeclarationAndAssignment
		int i;

		printf("\nPrinting 20 lines for reference. ");
		printf("Notice that line 6 is discarded during scrolling.\n");
		// ReSharper disable once CppJoinDeclarationAndAssignment
		for (i = 0; i <= 20; i++)
			printf("%d\n", i);

		// ReSharper disable once CppJoinDeclarationAndAssignment
		hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

		if (hStdout == INVALID_HANDLE_VALUE)
			throw std::system_error(GetLastError(), std::system_category(), "GetStdHandle failed");

		// Get the screen buffer size.

		if (!GetConsoleScreenBufferInfo(hStdout, &csbiInfo))
			throw std::system_error(GetLastError(), std::system_category(), "GetConsoleScreenBufferInfo failed");

		// The scrolling rectangle is the bottom 15 rows of the
		// screen buffer.

		srctScrollRect.Top = csbiInfo.dwSize.Y - 16;
		srctScrollRect.Bottom = csbiInfo.dwSize.Y - 1;
		srctScrollRect.Left = 0;
		srctScrollRect.Right = csbiInfo.dwSize.X - 1;

		// The destination for the scroll rectangle is one row up.

		coordDest.X = 0;
		coordDest.Y = csbiInfo.dwSize.Y - 17;

		// The clipping rectangle is the same as the scrolling rectangle.
		// The destination row is left unchanged.

		srctClipRect = srctScrollRect;

		// Fill the bottom row with green blanks.

		chiFill.Attributes = BACKGROUND_GREEN | FOREGROUND_RED;
		chiFill.Char.AsciiChar = ' ';

		// Scroll up one line.

		if (!ScrollConsoleScreenBufferA(
			hStdout,         // screen buffer handle
			&srctScrollRect, // scrolling rectangle
			&srctClipRect,   // clipping rectangle
			coordDest,       // top left destination cell
			&chiFill))       // fill character and color
			throw std::system_error(GetLastError(), std::system_category(), "ScrollConsoleScreenBuffer failed");
	}

	bool ProcessKey(const KEY_EVENT_RECORD& key)
	{
		if (!key.bKeyDown)
			return true;
		try {
			const auto action = vkMap.find(key.wVirtualKeyCode);
			if (action == vkMap.end())
				return true;
			actions[action->second].action();
		}
		catch (const ExitException&)
		{
			throw;
		}
		catch (const std::system_error& e)
		{
			SetConsoleTextAttribute(hConOut, 12);
			std::cerr << std::endl << "Exception: " << e.what() << ", code=" << e.code() << std::endl;
			SetConsoleTextAttribute(hConOut, 7);
		}
		catch (const std::exception& e)
		{
			std::cerr << std::endl << "Exception: " << e.what() << std::endl;
		}
		return true;
	}

	bool ProcessKey(const std::string& key)
	{
		try {
			const auto keyLower = ToLower(key);
			const auto action = keyMap.find(keyLower);
			if (action == keyMap.end())
				return true;
			actions[action->second].action();
		}
		catch (const ExitException&)
		{
			throw;
		}
		catch (const std::system_error& e)
		{
			SetConsoleTextAttribute(hConOut, 12);
			std::cerr << std::endl << "Exception: " << e.what() << ", code=" << e.code() << std::endl;
			SetConsoleTextAttribute(hConOut, 7);
		}
		catch (const std::exception& e)
		{
			std::cerr << std::endl << "Exception: " << e.what() << std::endl;
		}
		return true;
	}
};

int main(const int argc, char** argv)
{
	try {
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);
		setlocale(LC_ALL, ".UTF-8");
		Framework{}.Run(argc, argv);
		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << std::endl << "Exception: " << e.what() << std::endl;
		return 100;
	}
}
