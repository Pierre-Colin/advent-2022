#include <algorithm>
#include <cstdint>
#include <execution>
#include <ios>
#include <istream>
#include <iterator>
#include <locale>
#include <sstream>
#include <stdexcept>

#include "common.h"

enum class decision { rock, paper, scissors };

struct score {
	std::uintmax_t naive;
	std::uintmax_t strategic;
};

namespace {

static constexpr std::uintmax_t
compute_score(const decision a, const decision b) noexcept
{
	const int score_decision = static_cast<int>(b) + 1;
	const int d = a == decision::rock && b == decision::scissors ? -1 :
		a == decision::scissors && b == decision::rock ? 1 :
		static_cast<int>(b) - static_cast<int>(a);
	const int score_matchup = (d + 1) * 3;
	return static_cast<std::uintmax_t>(score_matchup + score_decision);
}

template<char P, char Q, char R>
constexpr int parse(std::istream::char_type c) noexcept
{
	constexpr const std::istream::char_type lookup[3]{P, Q, R};
	const auto it = std::find(std::execution::unseq, std::cbegin(lookup),
	                          std::cend(lookup), c);
	if (it == std::cend(lookup)) [[unlikely]]
		return -1;
	return static_cast<int>(std::distance(std::cbegin(lookup), it));
}

static std::istream& operator>>(std::istream& in, score& s)
{
	std::istream::char_type buf[3];
	if (!in.read(buf, std::size(buf)))
		return in;
	const int a_data = parse<'A', 'B', 'C'>(buf[0]);
	if (a_data < 0) [[unlikely]] {
		in.setstate(std::ios_base::failbit);
		return in;
	}
	const decision a = static_cast<decision>(a_data);
	if (buf[1] != ' ') [[unlikely]] {
		in.setstate(std::ios_base::failbit);
		return in;
	}
	const int b_data = parse<'X', 'Y', 'Z'>(buf[2]);
	if (b_data < 0) [[unlikely]] {
		in.setstate(std::ios_base::failbit);
		return in;
	}
	const decision b_naive = static_cast<decision>(b_data);
	const decision b_strategy =
		static_cast<decision>((a_data + b_data + 2) % 3);
	s.naive += compute_score(a, b_naive);
	s.strategic += compute_score(a, b_strategy);
	return in;
}

}

template<> output_pair day<2>(std::istream& in)
{
	using traits = std::istream::traits_type;
	std::locale loc = in.getloc();
	score s{0, 0};
	std::uintmax_t line = 1;
	while (in >> s) {
		std::istream::int_type c;
		while ((c = in.peek()) != traits::eof()
		       && std::isspace(traits::to_char_type(c), loc)) {
			if (c == in.widen('\n'))
				++line;
			in.ignore();
		}
	}
	if (!in.eof()) {
		std::ostringstream os;
		os << "Puzzle input error occurred on line " << line;
		throw std::runtime_error(os.str());
	}
	return {s.naive, s.strategic};
}
