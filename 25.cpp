#include <algorithm>
#include <cstdint>
#include <execution>
#include <istream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "common.h"

constexpr std::uintmax_t parse_snafu(std::string_view s)
{
	std::intmax_t acc = 0;
	for (char c : s) {
		switch (c) {
		[[unlikely]] default:
			throw std::invalid_argument("Character is not SNAFU");
		case '0':
		case '1':
		case '2':
			acc = 5 * acc + c - '0';
			break;
		case '-':
			acc = 5 * acc - 1;
			break;
		case '=':
			acc = 5 * acc - 2;
		}
	}
	if (acc < 0) [[unlikely]]
		throw std::invalid_argument("Number is negative");
	return static_cast<std::uintmax_t>(acc);
}

static std::uintmax_t sum_snafu(std::istream& in)
{
	std::uintmax_t acc = 0;
	std::string line;
	while (std::getline(in, line)) {
		if (line.empty())
			continue;
		acc += parse_snafu(line);
	}
	return acc;
}

static std::string to_snafu(std::uintmax_t n)
{
	std::string result;
	while (n > 0) {
		constexpr const char c[5]{'0', '1', '2', '=', '-'};
		const std::uintmax_t m = n % 5;
		result += c[m];
		n = n / 5 + (m >= 3);
	}
	std::reverse(std::execution::unseq, result.begin(), result.end());
	return result;
}

template<> output_pair day<25>(std::istream& in)
{
	using namespace std::literals;
	const std::uintmax_t s = sum_snafu(in);
	return {to_snafu(s), "FREE"s};
}
