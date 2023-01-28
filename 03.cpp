#include <algorithm>
#include <cstdint>
#include <execution>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "common.h"

namespace {

class parser;

static std::istream& operator>>(std::istream& in, parser& p);

class parser {
	friend std::istream& operator>>(std::istream&, parser&);
public:
	constexpr parser() noexcept = default;

	[[nodiscard]] constexpr std::uintmax_t priority() const noexcept {
		return sum_priority;
	}

	[[nodiscard]] constexpr std::uintmax_t badges() const noexcept {
		return sum_badges;
	}

	[[nodiscard]] constexpr bool operator!() const noexcept {
		return group != 0;
	}

private:
	void visit_item(char t);
	[[nodiscard]] constexpr std::uintmax_t group_badge() const;

	std::uintmax_t sum_priority = 0;
	std::uintmax_t sum_badges = 0;
	std::uint_fast64_t present[3]{0, 0, 0};
	std::size_t group = 0;
};

constexpr const char item_types[53] =
	"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

constexpr char common_item(std::string_view a, std::string_view b) noexcept
{
	char t = 0;
	auto it = std::cbegin(b);
	for (const char x : a) {
		it = std::lower_bound(it, std::cend(b), x);
		if (it == std::cend(b))
			break;
		if (*it == x) {
			if (t != 0 && *it != t)
				return 0;
			t = x;
			++it;
		}
	}
	return t;
}

static int item_priority(const char t) noexcept
{
	constexpr auto beg = std::cbegin(item_types);
	constexpr auto end = std::next(beg, 52);
	if constexpr (std::is_sorted(beg, end)) {
		const auto it = std::lower_bound(beg, end, t);
		return *it == t ? static_cast<int>(std::distance(beg, it) + 1)
		                : -1;
	} else {
		const auto it = std::find(std::execution::unseq, beg, end, t);
		return it != end ? static_cast<int>(std::distance(beg, it) + 1)
		                 : -1;
	}
}

static std::uintmax_t common_priority(std::string rucksack)
{
	const auto mid = std::size(rucksack) / 2;
	std::sort(std::begin(rucksack), std::begin(rucksack) + mid);
	std::sort(std::begin(rucksack) + mid, std::end(rucksack));
	const auto beg = std::cbegin(rucksack);
	const char common = common_item({beg, beg + mid},
	                                {beg + mid, std::cend(rucksack)});
	if (common == 0) [[unlikely]]
		throw std::invalid_argument("Several common items were found");
	return item_priority(common);
}

void parser::visit_item(const char t)
{
	const int p = item_priority(t);
	if (p < 0) {
		std::ostringstream s;
		s << "Wrong item type '" << t << '\'';
		throw std::invalid_argument(s.str());
	}
	present[group] |= std::uint_fast64_t{1} << (p - 1);
}

constexpr std::uintmax_t parser::group_badge() const
{
	std::size_t result = 127;
	for (std::size_t i = 0; i < 52; ++i) {
		const auto p = [i](const std::uint_fast64_t x) {
			return (x & std::uint_fast64_t{1} << i) != 0;
		};
		if (std::all_of(std::cbegin(present), std::cend(present), p)) {
			if (result < 127)
				throw std::runtime_error("Bad group");
			result = i;
		}
	}
	if (result >= 52)
		throw std::runtime_error("No group badge found");
	return result + 1;
}

static std::istream& operator>>(std::istream& in, parser& p)
{
	std::string rucksack;
	if (!(in >> rucksack))
		return in;
	if (std::size(rucksack) % 2 != 0) [[unlikely]] {
		in.setstate(std::ios_base::failbit);
		return in;
	}
	for (const char t : rucksack)
		p.visit_item(t);
	p.sum_priority += common_priority(std::move(rucksack));
	++p.group;
	if (p.group == 3) {
		p.sum_badges += p.group_badge();
		std::fill(std::execution::unseq, std::begin(p.present),
		          std::end(p.present), std::uint_fast64_t{0});
		p.group = 0;
	}
	return in;
}

}

template<> output_pair day<3>(std::istream& in)
{
	parser p;
	std::uintmax_t n = 1;
	while (in >> p)
		++n;
	if (!in.eof()) [[unlikely]] {
		std::ostringstream s;
		s << "Error while parsing rucksack " << n;
		throw std::runtime_error(s.str());
	} else if (!p) [[unlikely]] {
		throw std::runtime_error("Rucksack number is not divisible by 3");
	}
	return {p.priority(), p.badges()};
}
