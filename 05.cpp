#include <algorithm>
#include <execution>
#include <ios>
#include <istream>
#include <limits>
#include <locale>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "common.h"

using namespace std::literals;

struct instruction {
	std::vector<char>::size_type num;
	std::vector<std::vector<char>>::size_type from;
	std::vector<std::vector<char>>::size_type to;
};

namespace {

static std::istream& read_expect(std::istream& in, std::string_view s)
{
	char buf[5];
	if (std::size(s) > sizeof buf) [[unlikely]]
		throw std::logic_error("Input buffer is too small");
	if (!in.read(buf, std::size(s)))
		return in;
	if (!std::equal(std::execution::unseq, std::cbegin(s), std::cend(s),
	                std::cbegin(buf)))
		in.setstate(std::ios_base::failbit);
	return in;
}

static void check_crate(std::span<const char, 5> buf, const std::locale& loc)
{
	const bool b =
		buf[0] == '[' && std::isalpha(buf[1], loc) && buf[2] == ']';
	if (!b) [[unlikely]] {
		std::ostringstream s;
		s << "Wrong crate format: " << buf[0] << buf[1] << buf[2];
		throw std::invalid_argument(s.str());
	}
}

static std::istream& ignore_line(std::istream& in)
{
	return in.ignore(std::numeric_limits<std::streamsize>::max(),
	                 in.widen('\n'));
}

[[nodiscard]]
static std::vector<std::vector<char>> parse_crates(std::istream& in)
{
	constexpr const char empty[] = "   ";
	char buf[5];
	std::vector<std::vector<char>>::size_type column = 0;
	std::locale loc;
	std::vector<std::vector<char>> result;
	while (in.get(buf, 5) && buf[1] != '1') {
		if (!std::equal(std::execution::unseq, buf, buf + 3, empty)) {
			check_crate(buf, loc);
			if (column + 1 > result.size())
				result.resize(column + 1);
			result[column].push_back(buf[1]);
		}
		if (buf[3] == ' ') {
			++column;
		} else if (buf[3] == 0) {
			column = 0;
			in.ignore();
		} else [[unlikely]] {
			throw std::runtime_error("Bad crate separator");
		}
	}
	if (!ignore_line(ignore_line(in))) [[unlikely]]
		throw std::runtime_error("Premature end of puzzle input");
	for (std::vector<char>& s : result)
		std::reverse(std::execution::unseq, s.begin(), s.end());
	return result;
}

[[nodiscard]]
static std::vector<instruction> parse_instructions(std::istream& in)
{
	std::vector<instruction> result;
	instruction instr;
	while (read_expect(in, "move"sv) && in >> instr.num
	       && read_expect(in, " from"sv) && in >> instr.from
	       && read_expect(in, " to"sv) && in >> instr.to) {
		if (instr.from-- == 0) [[unlikely]]
			throw std::runtime_error("Tried to move from stack 0");
		if (instr.to-- == 0) [[unlikely]]
			throw std::runtime_error("Tried to move to stack 0");
		if (instr.from == instr.to) [[unlikely]]
			throw std::runtime_error("Ill-formed move");
		result.push_back(instr);
		in.ignore();
	}
	if (!in.eof()) [[unlikely]]
		throw std::runtime_error("Error while reading puzzle input");
	return result;
}

static std::string read_top(const std::vector<std::vector<char>>& stacks)
{
	std::string result;
	result.reserve(stacks.size());
	for (const std::vector<char>& stack : stacks) {
		if (!stack.empty())
			result += stack.back();
	}
	return result;
}

template<class F>
std::vector<std::vector<char>>
move_crates(std::vector<std::vector<char>> stacks,
            const std::vector<instruction>& instructions, F&& m)
{
	for (auto [num, from, to] : instructions) {
		if (from >= stacks.size()) [[unlikely]] {
			std::ostringstream s;
			s << "Tried to move from empty crate " << (from + 1);
			throw std::invalid_argument(s.str());
		}
		if (to >= stacks.size()) [[unlikely]]
			stacks.resize(to + 1);
		if (num > stacks[from].size()) [[unlikely]]
			num = stacks[from].size();
		m(num, stacks[from], stacks[to]);
	}
	return stacks;
}

static std::vector<std::vector<char>>
move_crates_rev(std::vector<std::vector<char>> stacks,
                 const std::vector<instruction>& instructions)
{
	constexpr auto m = [](std::vector<char>::size_type num,
	                  std::vector<char>& from, std::vector<char>& to) {
		to.insert(to.end(), from.crbegin(), from.crbegin() + num);
		from.erase(from.end() - num, from.end());
	};
	return move_crates(std::move(stacks), instructions, m);
}

static std::vector<std::vector<char>>
move_crates_id(std::vector<std::vector<char>> stacks,
                 const std::vector<instruction>& instructions)
{
	constexpr auto m = [](std::vector<char>::size_type num,
	                  std::vector<char>& from, std::vector<char>& to) {
		to.insert(to.end(), from.end() - num, from.end());
		from.erase(from.end() - num, from.end());
	};
	return move_crates(std::move(stacks), instructions, m);
}

}

template<> output_pair day<5>(std::istream& in)
{
	std::vector<std::vector<char>> state9000 = parse_crates(in);
	std::vector<std::vector<char>> state9001 = state9000;
	std::vector<instruction> instructions = parse_instructions(in);
	return {read_top(move_crates_rev(std::move(state9000), instructions)),
	        read_top(move_crates_id(std::move(state9001), instructions))};
}
