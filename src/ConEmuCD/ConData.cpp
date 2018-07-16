
/*
Copyright (c) 2018-present Maximus5
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

#include "../common/Common.h"
#include "../common/CEStr.h"

#include "ConData.h"

#undef TABLE_DEBUGDUMP
#ifdef _DEBUG
#define TABLE_DEBUGDUMP
#endif

namespace condata
{


console_error::console_error(const char* message)
	: logic_error(message)
{
}



#ifdef _DEBUG
struct RowUnitTest
{
	struct Cell { wchar_t ch; Attribute attr; };

	void Validate(std::vector<Cell>& data, Row& row)
	{
		const auto len = row.GetLength();
		row.Validate();
		_ASSERTE(data.size() == len);
		const ssize_t max_len = std::min<ssize_t>(data.size(), row.GetLength());
		Attribute attr; unsigned hint = 0;
		for (ssize_t i = 0; i < max_len; ++i)
		{
			const auto& cell = data[i];
			hint = row.GetCellAttr(unsigned(i), attr, hint);
			const auto wc = row.GetCellChar(unsigned(i));
			_ASSERTE((attr == cell.attr));
			_ASSERTE((wc == cell.ch));
		}
	}

	void RequestSize(std::vector<Cell>& data, unsigned new_size)
	{
		if (data.size() < new_size)
		{
			Cell cell { L' ' };
			if (!data.empty())
				cell.attr = data.back().attr;
			data.insert(data.end(), new_size - data.size(), cell);
		}
	}

	void Write(std::vector<Cell>& data, Row& row, unsigned cell, const Attribute& attr, const wchar_t* text, unsigned count)
	{
		RequestSize(data, cell + count);
		for (unsigned i = 0; i < count; ++i)
		{
			data[cell + i].ch = text[i];
			data[cell + i].attr = attr;
		}
		row.Write(cell, attr, text, count);
		Validate(data, row);
	}

	void Insert(std::vector<Cell>& data, Row& row, unsigned cell, const Attribute& attr, const wchar_t chr, unsigned count)
	{
		RequestSize(data, cell);
		data.insert(data.begin() + cell, count, Cell { chr, attr });
		row.Insert(cell, attr, chr, count);
		Validate(data, row);
	}

	void Delete(std::vector<Cell>& data, Row& row, unsigned cell, unsigned count)
	{
		if (cell < data.size())
			data.erase(data.begin()+cell, data.begin()+std::min<size_t>(cell+count, data.size()));
		row.Delete(cell, count);
		Validate(data, row);
	}

	void SetAttr(std::vector<Cell>& data, Row& row, unsigned cell, const Attribute& attr, unsigned count)
	{
		RequestSize(data, cell + count);
		for (size_t i = 0; i < count; ++i)
		{
			data[cell + i].attr = attr;
		}
		row.SetCellAttr(cell, attr, count);
		Validate(data, row);
	}

	RowUnitTest()
	{
		// single row - test 1
		{
			Row row;
			std::vector<Cell> data;
			// " abc"
			Write(data, row, 1, Attribute{1}, L"abc", 3);
			// " abc def"
			Write(data, row, 5, Attribute{2}, L"def", 3);
			// " abc def 123"
			Write(data, row, 9, Attribute{3}, L"123", 3);
			// "abc def 123"
			Delete(data, row, 0, 1);
			// "abc ef 123"
			Delete(data, row, 4, 1);
			// "abc e f 123"
			Insert(data, row, 5, Attribute{4}, L' ', 1);
			// "abc e f  123"
			Insert(data, row, 8, Attribute{5}, L' ', 1);
			// "ab23"
			Delete(data, row, 2, 8);
			// "ab  23"
			Insert(data, row, 2, Attribute{6}, L' ', 2);
			// "ab  23"
			SetAttr(data, row, 1, Attribute{7}, 4);
			// ""
			Delete(data, row, 0, row.GetLength());
		}

		// single row - test 2
		{
			Row row{Attribute{1}};
			std::vector<Cell> data;
			// "a"
			Write(data, row, 0, Attribute{1}, L"a", 1);
			// "ab"
			Write(data, row, 1, Attribute{1}, L"b", 1);
			// "abc"
			Write(data, row, 2, Attribute{1}, L"c", 1);
			// "bc"
			Delete(data, row, 0, 1);
			// "c"
			Delete(data, row, 0, 1);
			// ""
			Delete(data, row, 0, 1);
			// "a"
			Write(data, row, 0, Attribute{1}, L"a", 1);
			// "ab"
			Write(data, row, 1, Attribute{1}, L"b", 1);
			// "abc"
			Write(data, row, 2, Attribute{2}, L"c", 1);
			// "abcd"
			Write(data, row, 3, Attribute{2}, L"d", 1);
			// "abcde"
			Write(data, row, 4, Attribute{3}, L"e", 1);
			// "abcdef"
			Write(data, row, 5, Attribute{3}, L"f", 1);
			// "12345f"
			Write(data, row, 0, Attribute{1}, L"12345", 5);
			// "a2345f"
			Write(data, row, 0, Attribute{2}, L"a", 1);
			// "a25f"
			Delete(data, row, 2, 2);
		}

		// working table - test 1
		{
			Table table({7}, {10, 4}, 10);
			const wchar_t txt1[] = L"1234567890abcdefg";
			table.SetAttr(Attribute{1});
			table.Write(txt1, (unsigned)wcslen(txt1));
			table.DebugDump();
			//
			const wchar_t txt2[] = L"***\n###\t###\n\t===\r---\n";
			table.SetAttr(Attribute{2});
			table.Write(txt2, (unsigned)wcslen(txt2));
			table.DebugDump();
			//
			CharInfo data[10][12];
			table.GetData((CharInfo*)data, Coord{12, 10}, RECT{0, -4, 9, 3});
			//
			wchar_t txt3[10];
			for (int i = 0; i < 14; ++i)
			{
				for (int j = 0; j < countof(txt3); ++j) txt3[j] = '0'+i;
				table.SetAttr(Attribute{(unsigned short)(3+i)});
				table.Write(txt3, 10);
				table.DebugDump();
			}
		}
	};
};
RowUnitTest rowUnitTest;
#endif



/**
	Color and font outline attributes
**/

bool Attribute::isBold() const
{
	return (flags & fBold);
}
bool Attribute::isItalic() const
{
	return (flags & fItalic);
}
bool Attribute::isUnderscore() const
{
	return (attr.attr & COMMON_LVB_UNDERSCORE);
}
bool Attribute::isReverse() const
{
	return (attr.attr & COMMON_LVB_REVERSE_VIDEO);
}
bool Attribute::isFore4b() const
{
	return !(flags & (fFore8b | fFore24b));
}
bool Attribute::isFore8b() const
{
	return (flags & fFore8b);
}
bool Attribute::isFore24b() const
{
	return (flags & fFore24b);
}
bool Attribute::isBack4b() const
{
	return !(flags & (fBack8b | fBack24b));
}
bool Attribute::isBack8b() const
{
	return (flags & fBack8b);
}
bool Attribute::isBack24b() const
{
	return (flags & fBack24b);
}
bool Attribute::operator==(const Attribute& a) const
{
	return (attr.attr == a.attr.attr && flags == a.flags
		&& crForeColor == a.crForeColor && crBackColor == a.crBackColor);
}
bool Attribute::operator!=(const Attribute& a) const
{
	return !(*this == a);
}



/**
	Class represents one console row (line)
**/

Row::Row(const Attribute& attr /*= {}*/)
{
	m_Colors.push_back(RowColor{0, attr});
}

Row::~Row()
{
}

Row::Row(const Row& src)
{
	src.Validate();
	m_RowID = src.m_RowID;

	if (unsigned length = src.GetLength())
	{
		if (src.m_Text.size() > 0)
		{
			_ASSERTE(src.m_Text.size()>1 && src.m_Text[src.m_Text.size()-1]==0); // should be '\0'-terminated string
			m_Text.resize(length + 1); // prefer our strings to be '\0'-terminated
			memmove_s(&m_Text[0], sizeof(m_Text[0]) * m_Text.size(), &src.m_Text[0], sizeof(m_Text[0]) * length);
		}

		if (src.m_Colors.size() > 0)
		{
			m_Colors.resize(src.m_Colors.size());
			memmove_s(&m_Colors[0], sizeof(m_Colors[0]) * m_Colors.size(), &src.m_Colors[0], sizeof(m_Colors[0]) * src.m_Colors.size());
		}
	}
	else
	{
		if (!src.m_Colors.empty())
		{
			_ASSERTE(m_Colors.empty() && src.m_Colors[0].cell == 0);
			m_Colors.push_back(src.m_Colors[0]);
		}
		else
		{
			_ASSERTE(!src.m_Colors.empty());
			m_Colors.push_back(RowColor{0});
		}
	}

	Validate();
}

void Row::swap(Row& row)
{
	row.Validate();
	std::swap(m_RowID, row.m_RowID);
	m_Colors.swap(row.m_Colors);
	m_Text.swap(row.m_Text);
}

unsigned Row::SetReqSize(unsigned cell)
{
	if (m_Colors.empty())
	{
		_ASSERTE(m_Colors.size() > 0);
		// Should be at least one color (even black-on-black) per row
		m_Colors.push_back({0, Attribute{}});
	}

	const unsigned cur_len = GetLength();
	if (cell <= cur_len)
		return cur_len;

	// if cell is greater than previous output (e.g. cursor was moved after "eol")
	_ASSERTE(cell + 1 > m_Text.size());
	// additional '\0'-termination
	m_Text.resize(cell + 1);
	for (unsigned i = cur_len; i < cell; ++i)
	{
		m_Text[i] = L' ';
	}

	return cell;
}

void Row::Reserve(unsigned len)
{
	m_Text.reserve(len);
}

void Row::Write(unsigned cell, const Attribute& attr, const wchar_t* text, unsigned count)
{
	if (!text || !count)
		return;

	Reserve(((cell + count + 80) / 80) * 80);

	SetReqSize(cell + count);
	for (unsigned i = 0; i < count; ++i)
	{
		m_Text[cell + i] = text[i];
	}

	SetCellAttr(cell, attr, count);
}

void Row::Insert(unsigned cell, const Attribute& attr, const wchar_t chr, unsigned count)
{
	if (!count)
		return;

	Reserve(cell + count + 1);
	SetReqSize(cell);
	m_Text.insert(cell, chr, count);

	Validate();
	for (ssize_t i = 0; i < m_Colors.size(); ++i)
	{
		if (m_Colors[i].cell >= cell)
			m_Colors[i].cell += count;
	}

	SetCellAttr(cell, attr, count);
}

void Row::Delete(unsigned cell, unsigned count)
{
	const unsigned cur_len = GetLength();
	if (!count || cell >= cur_len)
		return;
	if (cell + count > cur_len)
		count = cur_len - cell;

	Attribute firstAttr, lastAttr, newAttr;
	unsigned first = GetCellAttr(cell, firstAttr);
	unsigned last = GetCellAttr(cell + count, lastAttr, first);

	Validate();
	if (first < last)
	{
		unsigned del_first = (m_Colors[first].cell == cell) ? first : first + 1;
		if (del_first < last)
		{
			m_Colors.erase(del_first, last - del_first);
			// Validate(); -- invalid till next decrement cycle
		}
	}
	for (ssize_t i = first; i < m_Colors.size(); ++i)
	{
		auto& ic = m_Colors[i];
		if (ic.cell >= cell + count)
			ic.cell -= count;
		else if (ic.cell > cell)
			ic.cell = cell;
	}
	Validate();

	if (cell < GetLength())
	{
		GetCellAttr(cell, newAttr, first);
		if (!(newAttr == lastAttr))
			m_Colors.insert(first, RowColor{cell, lastAttr});
		Validate();
	}

	// Do it at the end because GetCellAttr take into account m_Text length
	m_Text.erase(cell, count);
	Validate();
}

unsigned Row::GetLength() const
{
	if (m_Text.size() > 0)
	{
		if (m_Text[m_Text.size() - 1] != 0)
		{
			_ASSERTE(m_Text.size()>0 && m_Text[m_Text.size()-1]==0); // Missed '\0'-terminator
			return unsigned(m_Text.size());
		}
		else
		{
			return unsigned(m_Text.size() - 1); // // valid '\0'-terminated string
		}
	}
	// Empty line
	return 0;
}

bool Row::HasLineFeed() const
{
	// #condata Implement fCRLF flags
	return false;
}

const wchar_t* Row::GetText() const
{
	unsigned len = GetLength();
	if (!len)
		return L"";
	if (len == m_Text.size())
		throw console_error("Row has no 0-terminator");
	return (const wchar_t*)&m_Text[0];
}

unsigned Row::GetCellAttr(unsigned cell, Attribute& attr, unsigned hint /*= 0*/) const
{
	if (m_Colors.empty())
	{
		_ASSERTE(FALSE && "m_Colors should be already filled");
		attr = Attribute{};
		return 0;
	}

	// We may use binary search for optimization, but
	// expected size of the m_Colors is just a few elements
	for (ssize_t i = hint; i < m_Colors.size(); ++i)
	{
		const auto& ic = m_Colors[i].cell;
		if (ic < cell)
			continue;
		if ((ic > cell) && (i > 0))
		{
			_ASSERTE(m_Colors[i - 1].cell < cell);
			attr = m_Colors[i - 1].attr;
			return unsigned(i - 1);
		}
		else
		{
			_ASSERTE(ic == cell);
			attr = m_Colors[i].attr;
			return unsigned(i);
		}
	}

	attr = m_Colors[m_Colors.size()-1].attr;
	return unsigned(m_Colors.size()-1);
}

void Row::SetCellAttr(unsigned cell, const Attribute& attr, unsigned count)
{
	_ASSERTE(cell < GetLength() && count);

	Validate();

	SetReqSize(cell + count);

	Attribute firstAttr, nextAttr, newAttr;
	unsigned first = GetCellAttr(cell, firstAttr);
	unsigned next = GetCellAttr(cell + count, nextAttr, first);
	#ifdef _DEBUG
	const decltype(m_Colors)::vector_type color_copy(m_Colors.cbegin(), m_Colors.cend());
	#endif

	// [...] ... [first:A] ... [next:B] ... [...]

	if (firstAttr == attr)
	{
		_ASSERTE(m_Colors.size() > 0); // must be validated in SetReqSize
		if (first < next)
		{
			if (m_Colors[next].cell < cell + count)
				m_Colors[next].cell = cell + count;
			if (first + 1 < next)
			{
				const unsigned del_first = first + 1;
				m_Colors.erase(del_first, next - del_first);
			}
		}
	}
	else
	{
		if (m_Colors[first].cell == cell)
		{
			m_Colors[first].attr = attr;
			// [...] ... [first:attr] ... [next:B] ... [...]
		}
		else
		{
			_ASSERTE(m_Colors[first].cell<cell && (first+1 >= m_Colors.size() || m_Colors[first+1].cell > cell));
			m_Colors.insert(++first, RowColor{cell, attr});
			// [...] ... [first-1:A] [first:attr] ... [next:B] ... [...]
		}
		next = first + 1;
		// Validate(); -- may be invalid at the moment

		if (next < m_Colors.size())
		{
			if (m_Colors[next].cell < cell + count)
			{
				while (next + 1 < m_Colors.size() && m_Colors[next + 1].cell <= cell + count)
				{
					++next;
				}
				if (next < m_Colors.size() && m_Colors[next].cell < cell + count)
				{
					_ASSERTE(next > 0);
					m_Colors[next].cell = cell + count;
				}
			}
		}

		// Drop overwritten regions
		if (first + 1 < next)
		{
			const unsigned del_first = first + 1;
			m_Colors.erase(del_first, next - del_first);
		}

		// Validate next, may be changed after insert/erase
		if (cell + count < GetLength())
		{
			unsigned new_next = GetCellAttr(cell + count, newAttr, first);
			if (!(newAttr == nextAttr))
			{
				m_Colors.insert(new_next + 1, RowColor{cell + count, nextAttr});
				Validate();
			}
		}
	}

	Validate();
}

wchar_t Row::GetCellChar(unsigned cell) const
{
	wchar_t ch = (cell < m_Text.size()) ? m_Text[cell] : L' ';
	return ch ? ch : L' ';
}

void Row::Validate() const
{
#ifdef _DEBUG
	const unsigned len = GetLength();
	_ASSERTE(len==0 || (m_Colors.size()>=1 && m_Colors[0].cell==0));
	ssize_t prev = 0;
	for (ssize_t i = 1; i < m_Colors.size(); ++i)
	{
		_ASSERTE(prev < ssize_t(m_Colors[i].cell));
		_ASSERTE(m_Colors[i].cell < len);
		prev = ssize_t(m_Colors[i].cell);
	}
#endif
}



/**
	Main class to manipulate console surface
**/

Table::Table(const Attribute& _attr /*= {7}*/, const Coord& _size /*= {80, 25}*/,
			 const int _backscrollMax /*= 1<<20*/, BellCallback _bellCallback /*= nullptr*/)
	: m_Attr(_attr)
	, m_Size(_size)
	, m_BackscrollMax(_backscrollMax)
	, m_BellCallback(_bellCallback)
{
}

Table::~Table()
{
}

bool Table::IsEmpty() const
{
	//if (m_Workspace.empty())
	//	return true;
	if (m_Size.x < 1 || m_Size.y < 1)
		return true;
	return false;
}

Coord Table::GetSize() const
{
	_ASSERTE(m_Size.x >= 1 && m_Size.y >= 1);
	return Coord{m_Size.x, std::max<int>(0, m_Size.y)};
}

Coord Table::GetCursor() const
{
	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		return Coord{};
	}
	_ASSERTE(m_Cursor.y >= 0);
	const Region rgn = GetScrollRegion();
	return Coord{std::min(m_Cursor.x, m_Size.x), std::max<int>(rgn.Top, std::min<int>(m_Cursor.y, rgn.Bottom))};
}

unsigned Table::GetBackscroll() const
{
	return (unsigned) m_Backscroll.size();
}

condata::Attribute Table::GetAttr() const
{
	return m_Attr;
}

void Table::Resize(const Coord newSize)
{
	if (newSize.x < 1 || newSize.y < 1)
	{
		_ASSERTE(newSize.x >= 1 && newSize.y >= 1);
		return;
	}

	// If newSize.y becomes larger than m_Size.y, return lines from backscroll to working area
	while (newSize.y > m_Size.y)
	{
		if (m_Backscroll.empty())
			break;
		m_Workspace.insert(m_Workspace.begin(), Row(m_Backscroll.front()));
		m_Backscroll.erase(m_Backscroll.begin());
		++m_Size.y;
		ShiftRowIndexes(+1);
	}

	// If newSize.y becomes lesser than m_Size.y, push upper lines to backscroll and adjust cursor position
	while (newSize.y < m_Size.y)
	{
		if (m_Workspace.empty())
			break;
		if (m_BackscrollMax < 0 || (ssize_t)m_Backscroll.size() < m_BackscrollMax)
			m_Backscroll.push_front(Row(m_Workspace.front()));
		m_Workspace.erase(m_Workspace.begin());
		--m_Size.y;
		ShiftRowIndexes(-1);
	}

	// The height must be already adjusted
	_ASSERTE(m_Size.y == newSize.y);
	m_Size = newSize;

	// Ensure the cursor is in allowed place
	SetCursor(m_Cursor);
}

void Table::ShiftRowIndexes(int direction)
{
	if (direction > 0)
	{
		_ASSERTE(direction == 1);
		SetCursor({m_Cursor.x, m_Cursor.y + direction});
		if (m_Flags.Region && m_Region.Bottom + 1 < m_Size.y)
		{
			m_Region.Top += direction;
			m_Region.Bottom += direction;
		}
	}
	else if (direction < 0)
	{
		_ASSERTE(direction == -1);
		SetCursor({m_Cursor.x, m_Cursor.y - direction});
		if (m_Flags.Region && m_Region.Top > 0)
		{
			m_Region.Top += direction;
			m_Region.Bottom += direction;
		}
	}
}

void Table::SetAttr(const Attribute& attr)
{
	// #condata Show warning when set "black-on-black"?
	m_Attr = attr;
}

void Table::SetCursor(const Coord cursor)
{
	_ASSERTE(m_Size.x >= 1 && m_Size.y >= 1);
	m_Cursor = {
		// If AutoWrap == false - cursor stays at *m_Size.x-1* indefinitely
		std::min<unsigned>(cursor.x, m_Flags.AutoWrap ? m_Size.x  : (m_Size.x - 1)),
		std::max<int>(0, std::min<int>(cursor.y, m_Size.y - 1))
	};
}

void Table::SetScrollRegion(bool setRegion /*= false*/, unsigned top /*= 0*/, unsigned bottom /*= 0*/)
{
	if (setRegion && ((int)top >= 0 && (int)top <= (int)bottom && (int)bottom < m_Size.y))
	{
		m_Region.Top = top;
		m_Region.Bottom = bottom;
		m_Flags.Region = true;
		// e.g. mintty resets cursor explicitly in position {0,0}, but that looks like a bug
		SetCursor({0, int(top)});
	}
	else
	{
		_ASSERTE(!setRegion);
		m_Flags.Region = false;
		m_Region.Top = m_Region.Bottom = 0;
	}
}

void Table::SetAutoCRLF(bool autoCRLF)
{
	m_Flags.AutoCRLF = autoCRLF;
}

void Table::SetAutoWrap(bool autoWrap, int wrapAt /*= -1*/)
{
	m_Flags.AutoWrap = autoWrap;
	m_WrapAt = wrapAt;
}

void Table::SetBeyondEOL(bool beyondEOL)
{
	m_Flags.BeyondEOL = beyondEOL;
}

Table::vector_type::iterator Table::CurrentRow()
{
	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		throw console_error("Workspace is empty");
	}
	// Extend workspace to requested row
	const size_t idx = (size_t)std::max<int>(0, (int) std::min<ssize_t>(m_Size.y - 1, m_Cursor.y));
	_ASSERTE(idx < 1000); // sane console dimensions?
	if (m_Workspace.size() <= idx)
		m_Workspace.resize(idx+1);
	return (m_Workspace.begin() + idx);
}

void Table::DoAutoScroll()
{
	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		throw console_error("Workspace is empty");
	}

	const auto cur = GetCursor();
	const Region rgn = GetScrollRegion();
	if (cur.y < (int)rgn.Top)
	{
		_ASSERTE(FALSE && "Cursor or Region is broken");
		throw console_error("Cursor or Region is broken");
	}

	if (m_Workspace.size() <= (size_t)cur.y)
	{
		m_Workspace.resize((size_t)cur.y + 1);
	}

	if ((rgn.Top == 0) && (m_BackscrollMax < 0 || (ssize_t)m_Backscroll.size() < m_BackscrollMax))
	{
		m_Backscroll.push_front(Row(m_Workspace.front()));
	}

	m_Workspace.erase(m_Workspace.begin() + rgn.Top);
	m_Workspace.insert(m_Workspace.begin() + std::min<unsigned>(cur.y, rgn.Bottom), Row{m_Attr});
}

void Table::DoLineFeed(bool cr)
{
	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		throw console_error("Workspace is empty");
	}

	const Region rgn = GetScrollRegion();
	Coord cur = GetCursor();

	if (cr)
	{
		cur.x = 0;
	}

	if (cur.y >= (int)rgn.Bottom)
	{
		_ASSERTE(cur.y == (int)rgn.Bottom);
		// scroll scrollable contents upward, insert empty line at the bottom
		// cursor y-position is not changed
		DoAutoScroll();
	}
	else
	{
		_ASSERTE(cur.y >= 0);
		++cur.y;
	}

	SetCursor(cur);
}

Table::Region Table::GetScrollRegion() const
{
	const auto sz = GetSize();
	Region rgn = {};
	rgn.Top = (m_Flags.Region && m_Region.Top <= m_Region.Bottom)
		? std::max<int>(0, std::min<int>(m_Region.Top, sz.y - 1))
		: 0;
	rgn.Bottom = (m_Flags.Region && m_Region.Top <= m_Region.Bottom)
		? std::max<int>(rgn.Top, std::min<int>(m_Region.Bottom, sz.y - 1))
		: sz.y - 1;
	_ASSERTE(rgn.Top >= 0 && rgn.Bottom >= rgn.Top);
	return rgn;
}

void Table::Write(const wchar_t* text, unsigned count)
{
	if (!text || !count)
		return;

	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		return;
	}

	if (m_Size.x < 1 || m_Size.y < 1)
	{
		_ASSERTE(m_Size.x >= 1 && m_Size.y >= 1);
		return;
	}


	#ifdef _DEBUG
	const Coord prev_cur = m_Cursor;
	const Coord prev_size = m_Size;
	const Region rgn = GetScrollRegion();
	#endif


	// Iterate characters
	for (unsigned pos = 0; pos < count; ++pos)
	{
		switch (text[pos])
		{
		case L'\t':
			// #ConData Store in the row attribute that '\t' was "written" at this place?
			SetCursor({std::max<unsigned>(m_Size.x - 1, ((m_Cursor.x + 8) >> 3) << 3), m_Cursor.y});
			break;
		case L'\r':
			// just put cursor at the line start
			SetCursor({0, m_Cursor.y});
			break;
		case L'\n':
			// #ConData Store in the row attribute that line was '\n'-terminated
			DoLineFeed(m_Flags.AutoCRLF);
			// New line == prompt reset
			PromptPosReset();
			break;
		case 7:
			//Beep (no spacing)
			if (m_BellCallback)
				m_BellCallback();
			// cursor position is not changed, we just skip the bell position in text
			break;
		case 8: // L'\b'
			//BS
			SetCursor({m_Cursor.x > 0 ? (m_Cursor.x - 1) : m_Cursor.x, m_Cursor.y});
			break;
		default:
			// Just a letter
			// RealConsole can't have non-spacing glyphs, exceptions are processed above

			// First, ensure the cursor is on allowed place
			SetCursor(m_Cursor);
			// If AutoWrap allowed and we are at EOL (xterm mode)
			if (m_Flags.AutoWrap && m_Cursor.x >= m_Size.x)
				DoLineFeed(true);

			// They should not go to the console data (processed above)
			_ASSERTE(text[pos]!=L'\n' && text[pos]!=L'\r' && text[pos]!=L'\t' && text[pos]!=L'\b' && text[pos]!=7/*bell*/);

			// Write character one-by-one
			CurrentRow()->Write(m_Cursor.x, m_Attr, text + pos, 1);

			// advance cursor position
			SetCursor({m_Cursor.x + 1, m_Cursor.y});

			// And in WinApi mode do line feed
			if (m_Flags.AutoWrap && !m_Flags.BeyondEOL && m_Cursor.x >= m_Size.x)
				DoLineFeed(true);
		}
	}
}

void Table::InsertCell(unsigned count /*= 1*/, const wchar_t chr /*= L' '*/)
{
	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		return;
	}

	auto row = CurrentRow();
	const auto cur = GetCursor();
	row->Insert(cur.x, m_Attr, chr, count);
}

void Table::DeleteCell(unsigned count /*= 1*/)
{
	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		return;
	}

	auto row = CurrentRow();
	const auto cur = GetCursor();
	// Row::Delete properly does maximum allowed deletion size
	row->Delete(cur.x, count);
}

void Table::InsertRow(unsigned count /*= 1*/)
{
	if (!count)
	{
		return;
	}
	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		return;
	}

	const auto sz = GetSize();
	auto row = CurrentRow();
	for (unsigned i = 0; i < count; ++i)
	{
		row = m_Workspace.insert(row, Row{m_Attr});
	}
	if (m_Workspace.size() >= (unsigned)sz.y)
	{
		// These lines go to trash
		m_Workspace.erase(m_Workspace.begin() + sz.y, m_Workspace.end());
	}
}

void Table::DeleteRow(unsigned count /*= 1*/)
{
	if (!count)
	{
		return;
	}
	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		return;
	}

	auto row = CurrentRow();
	ptrdiff_t tail = m_Workspace.end() - row;
	if (tail <= 0)
	{
		_ASSERTE(tail > 0);
		return;
	}
	m_Workspace.erase(row, row + std::min<size_t>(tail, count));
}

void Table::GetData(CharInfo *pData, const Coord& bufSize, const RECT& rgn) const
{
	if (!pData || bufSize.x < 1 || bufSize.y < 1)
		throw console_error("Empty buffer passed to Table::GetData");
	if (IsRectEmpty(&rgn))
		return; // nothing to read

	CharInfo* ptr = pData;
	Attribute attr = m_Attr;
	const Row* row = nullptr;
	const unsigned max_x = std::min<unsigned>(bufSize.x, rgn.right - rgn.left + 1);
	const unsigned from_x = (unsigned)std::max<int>(0, rgn.left);
	_ASSERTE(from_x == rgn.left);

	int buf_y = 0;
	for (int y = rgn.top; y <= rgn.bottom && buf_y < bufSize.y; ++y, ++buf_y)
	{
		// Select either backscroll or workspace
		if (y < 0)
		{
			size_t back_idx = size_t(-(y+1));
			if (back_idx < m_Backscroll.size())
				row = &(m_Backscroll[back_idx]);
			else
				row = nullptr;
		}
		else
		{
			size_t work_idx = size_t(y);
			if (work_idx < m_Workspace.size())
				row = &(m_Workspace[work_idx]);
			else
				row = nullptr;
		}

		// Retrieve default attribute from topmost row
		if (buf_y == 0 && !row)
		{
			if (y < 0 && !m_Backscroll.empty())
			{
				m_Backscroll.back().GetCellAttr(0, attr);
			}
			else if (!m_Workspace.empty())
			{
				m_Workspace[0].GetCellAttr(0, attr);
			}
		}

		unsigned x = 0;
		if (row)
		{
			unsigned hint = 0;
			for (; x < max_x; ++x)
			{
				auto& dst = ptr[x];
				dst.chr = row->GetCellChar(x + from_x);
				hint = row->GetCellAttr(x + from_x, attr, hint);
				dst.attr = attr;
			}
		}
		for (; x < bufSize.x; ++x)
		{
			auto& dst = ptr[x];
			dst.chr = L' ';
			dst.attr = attr;
		}
		ptr += bufSize.x;
	}
	// Clean the tail
	for (; buf_y < bufSize.y; ++buf_y)
	{
		for (unsigned x = 0; x < bufSize.x; ++x)
		{
			auto& dst = ptr[x];
			dst.chr = L' ';
			dst.attr = attr;
		}
		ptr += bufSize.x;
	}
}

void Table::SetBellCallback(BellCallback bellCallback)
{
	m_BellCallback = bellCallback;
}

void Table::PromptPosStore()
{
	// #condata PromptPosStore
}

void Table::PromptPosReset()
{
	// #condata PromptPosReset
}

std::unique_lock<std::mutex> Table::Lock()
{
	return std::unique_lock<std::mutex>(m_Mutex);
}

void Table::DebugDump() const
{
#ifdef TABLE_DEBUGDUMP
	wchar_t prefix[20];
	MArray<CEStr> dbgout;
	dbgout.push_back(L"\n<<< Table::DebugDump <<< backscroll begin\n");
	for (size_t i = m_Backscroll.size(); i > 0; --i)
	{
		swprintf_s(prefix, L"%04i: ", -int(i));
		const auto& row = m_Backscroll[i-1];
		dbgout.push_back(CEStr(prefix, row.GetText(), row.HasLineFeed() ? L"\\n" : L"\\0", L"\n"));
	}
	dbgout.push_back(L"<<< Table::DebugDump <<< workspace begin\n");
	for (size_t i = 0; i < m_Workspace.size(); ++i)
	{
		swprintf_s(prefix, L"%04i: ", int(i));
		const auto& row = m_Workspace[i];
		dbgout.push_back(CEStr(prefix, row.GetText(), row.HasLineFeed() ? L"\\n" : L"\\0", L"\n"));
	}
	dbgout.push_back(L"<<< Table::DebugDump <<< end\n\n");
	ssize_t total = 0;
	MArray<ssize_t> lens;
	for (const auto& s : dbgout)
	{
		const ssize_t l = s.GetLen();
		total += l;
		lens.push_back(l);
	}
	CEStr buffer;
	wchar_t* ptr = buffer.GetBuffer(total);
	for (ssize_t i = 0; i < dbgout.size(); ++i)
	{
		const ssize_t l = lens[i];
		wcscpy_s(ptr, l+1, dbgout[i]);
		ptr += l;
	}

	OutputDebugStringW(buffer);
#endif
}




/**
	Main class to manipulate console surface
**/

TablePtr::TablePtr(Table& _table)
	: m_table(&_table)
	, m_lock(_table.Lock())
{
}

TablePtr::~TablePtr()
{
}

condata::Table* TablePtr::operator->()
{
	if (!m_table)
	{
		_ASSERTE(m_table!=nullptr);
		throw console_error("m_table is nullptr");
	}
	return m_table;
}

};