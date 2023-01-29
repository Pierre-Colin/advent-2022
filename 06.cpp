#include <algorithm>
#include <cstdint>
#include <execution>
#include <istream>
#include <span>
#include <string>

#include "common.h"

namespace {

template<class Iter> constexpr bool has_repeat(Iter begin, Iter end) noexcept
{
	for (Iter it = begin; it + 1 != end; ++it) {
		if (std::find(std::execution::unseq, it + 1, end, *it) != end)
			return true;
	}
	return false;
}

}

[[noreturn]] static void fail_on_input(const std::istream& in)
{
	if (in.eof())
		throw std::runtime_error("The puzzle input is too short");
	throw std::runtime_error("Error while reading the puzzle input");
}

static void extract_character(std::istream& in, std::span<char, 14> buf)
{
	std::shift_left(buf.begin(), buf.end(), 1);
	const std::istream::int_type next = in.get();
	if (next == std::istream::traits_type::eof()) [[unlikely]]
		fail_on_input(in);
	buf.back() = std::istream::traits_type::to_char_type(next);
}

template<> output_pair day<6>(std::istream& in)
{
	char buf[14];
	std::uintmax_t packet = 4;
	if (!in.read(buf, 14))
		fail_on_input(in);
	for (auto it = std::cbegin(buf);
	     it + 3 != std::cend(buf) && has_repeat(it, it + 4);
	     ++it, ++packet);
	if (packet == 15) {
		--packet;
		do {
			extract_character(in, buf);
			++packet;
		} while (has_repeat(std::cend(buf) - 4, std::cend(buf)));
	}
	std::uintmax_t message = packet;
	do {
		extract_character(in, buf);
		++message;
	} while (has_repeat(std::cbegin(buf), std::cend(buf)));
	return {packet, message};
}
