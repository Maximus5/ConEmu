#pragma once

#include <gtest/gtest.h>

#include <sstream>
#include <memory>

// ReSharper disable once CppInconsistentNaming
namespace testing
{
	// ReSharper disable once CppInconsistentNaming
	namespace internal
	{
		// ReSharper disable once IdentifierTypo
		void ColoredPrintf(GTestColor color, const char* fmt, ...);
	}
}

struct wcdbg : public std::wostringstream
{
	wcdbg(const char* label = nullptr)
	{
		m_label = label ? label : "DEBUG";
		m_label.resize(8, ' ');
	}

	wcdbg(const wcdbg&) = delete;
	wcdbg(wcdbg&&) = delete;
	wcdbg& operator=(const wcdbg&) = delete;
	wcdbg& operator=(wcdbg&&) = delete;

	virtual ~wcdbg()
	{
		const auto data = this->str();
		std::string utf_data;
		const auto console_cp = GetConsoleOutputCP();
		const auto cp = console_cp ? console_cp : CP_UTF8;
		const int len = WideCharToMultiByte(cp, 0, data.c_str(), int(data.length()), nullptr, 0, nullptr, nullptr);
		utf_data.resize(len);
		WideCharToMultiByte(cp, 0, data.c_str(), int(data.length()), &(utf_data[0]), len, nullptr, nullptr);
		testing::internal::ColoredPrintf(testing::internal::COLOR_YELLOW, "[ %s ] %s", m_label.c_str(), utf_data.c_str());
	}

private:
	std::string m_label;
};

struct cdbg : public std::ostringstream
{
	cdbg(const char* label = nullptr)
	{
		m_label = label ? label : "DEBUG";
		m_label.resize(8, ' ');
	}
	cdbg(const cdbg&) = delete;
	cdbg(cdbg&&) = delete;
	cdbg& operator=(const cdbg&) = delete;
	cdbg& operator=(cdbg&&) = delete;

	virtual ~cdbg()
	{
		const auto data = this->str();
		testing::internal::ColoredPrintf(testing::internal::COLOR_YELLOW, "[ %s ] %s", m_label.c_str(), data.c_str());
		static const bool in_color_mode = _isatty(_fileno(stdout)) != 0;
		if (!in_color_mode)
			fflush(stdout);
	}

private:
	std::string m_label;
};
