#include <algorithm>
#include <cinttypes>
#include <execution>
#include <istream>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

#include "common.h"

namespace {

class rope {
	friend std::istream& operator>>(std::istream&, rope&);
public:
	std::uintmax_t count_tail_positions_short() const noexcept {
		return visited_1.size();
	}

	std::uintmax_t count_tail_positions_long() const noexcept {
		return visited_9.size();
	}

private:
	struct coord {
		std::strong_ordering operator<=>(const coord&) const noexcept
			= default;

		std::intmax_t x;
		std::intmax_t y;
	};

	bool drag(unsigned int n);

	coord segments[10]{{0, 0}};
	std::set<coord> visited_1{{0, 0}};
	std::set<coord> visited_9{{0, 0}};
};

std::istream& operator>>(std::istream& in, rope& r)
{
	using limits = std::numeric_limits<std::intmax_t>;
	constexpr const char directions[] = {'U', 'D', 'L', 'R'};
	std::istream::int_type c;
	while ((c = in.get()) == in.widen('\n'));
	if (c == std::istream::traits_type::eof())
		return in;
	const auto dir = std::find(std::execution::unseq,
	                           std::begin(directions),
	                           std::end(directions),
	                           std::istream::traits_type::to_char_type(c));
	if (dir == std::end(directions)) [[unlikely]]
		throw std::runtime_error("Invalid direction");
	auto div_result = std::imaxdiv(dir - std::begin(directions), 2);
	std::intmax_t amount;
	if (!(in >> amount)) [[unlikely]] {
		in.setstate(std::ios_base::failbit);
		return in;
	}
	if (amount < 0) [[unlikely]]
		throw std::runtime_error("Moving by a negative amount");
	std::intmax_t& z = div_result.quot ? r.segments[0].x : r.segments[0].y;
	if (div_result.rem == 1
	    && limits::max() - 1 - amount < z) [[unlikely]] {
		throw std::runtime_error("Integer overflow detected");
	} else if (div_result.rem == 0
	           && limits::min() + 1 + amount > z) [[unlikely]] {
		throw std::runtime_error("Integer overflow detected");
	}
	for (std::intmax_t a = 0; a < amount; ++a) {
		z += div_result.rem ? 1 : -1;
		for (unsigned int n = 1; n < 10; ++n) {
			if (r.drag(n)) {
				if (n == 1)
					r.visited_1.insert(r.segments[n]);
				else if (n == 9) {
					r.visited_9.insert(r.segments[n]);
				}
			}
		}
	}
	return in;
}

bool rope::drag(unsigned int n)
{
	auto& [hx, hy] = segments[n - 1];
	auto& [tx, ty] = segments[n];
	if ((tx + 1 < hx && ty < hy) || (tx < hx && ty + 1 < hy)) {
		++tx;
		++ty;
		return true;
	} else if ((tx + 1 < hx && ty > hy) || (tx < hx && ty - 1 > hy)) {
		++tx;
		--ty;
		return true;
	} else if ((tx - 1 > hx && ty < hy) || (tx > hx && ty + 1 < hy)) {
		--tx;
		++ty;
		return true;
	} else if ((tx - 1 > hx && ty > hy) || (tx > hx && ty - 1 > hy)) {
		--tx;
		--ty;
		return true;
	} else if (tx + 1 < hx) {
		++tx;
		return true;
	} else if (tx - 1 > hx) {
		--tx;
		return true;
	} else if (ty + 1 < hy) {
		++ty;
		return true;
	} else if (ty - 1 > hy) {
		--ty;
		return true;
	}
	return false;
}

}

template<> output_pair day<9>(std::istream& in)
{
	rope r;
	while (in >> r);
	if (!in.eof()) [[unlikely]]
		throw std::runtime_error("Error while reading puzzle input");
	return {r.count_tail_positions_short(), r.count_tail_positions_long()};
}
