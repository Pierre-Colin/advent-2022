#include <algorithm>
#include <cstdint>
#include <execution>
#include <ios>
#include <istream>
#include <limits>
#include <map>
// #include <set>
#include <stdexcept>
#include <vector>

#include "common.h"
#include "interval.h"
#include "interval_union.h"

namespace {

struct point {
	constexpr std::strong_ordering operator<=>(const point&) const noexcept
		= default;

	std::intmax_t x;
	std::intmax_t y;
};

class sand_simulation {
public:
	explicit sand_simulation(std::istream& in);
	point drop_sand();

	constexpr bool has_floor(const std::intmax_t y) const noexcept {
		return y == floor_y;
	}

private:
	using hint_type =
		std::map<std::intmax_t, interval_union>::const_iterator;

	[[nodiscard]] bool collide(const point p) const noexcept;
	[[nodiscard]] bool
	collide(const point p, hint_type hint) const noexcept;

	std::intmax_t floor_y{};
	std::map<std::intmax_t, interval_union> obstacles{};
};

bool expect_arrow(std::istream& in)
{
	static constexpr char arr[3]{' ', '-', '>'};
	char buf[3];
	if (!in.read(buf, 3)
	    || !std::equal(std::execution::unseq, std::cbegin(buf),
	                   std::cend(buf), std::cbegin(arr))) [[unlikely]]
		in.setstate(std::ios_base::failbit);
	return (bool) in;
}

sand_simulation::sand_simulation(std::istream& in)
{
	using limits = std::numeric_limits<std::intmax_t>;
	std::intmax_t max_x = 0;
	std::intmax_t max_y = 0;
	const auto update_max = [&max_x, &max_y](const point p) {
		if (p.x > max_x)
			max_x = p.x;
		if (p.y > max_y)
			max_y = p.y;
	};
	for (;;) {
		while (in.peek() == '\n')
			in.ignore();
		if (!in)
			break;
		point p;
		if (!(in >> p.x) || in.get() != ',' || !(in >> p.y)) {
			in.setstate(std::ios_base::failbit);
			break;
		}
		update_max(p);
		if (in.peek() == '\n') {
			obstacles[p.y].insert(interval{p.x, p.x});
			continue;
		}
		while (in.peek() != '\n' && expect_arrow(in)) {
			point n;
			if (!(in >> n.x) || in.get() != ',' || !(in >> n.y)) {
				in.setstate(std::ios_base::failbit);
				break;
			}
			update_max(n);
			if (n.x == p.x) {
				const auto [ya, yb] = std::minmax(n.y, p.y);
				for (std::intmax_t y = ya; y <= yb; ++y)
					obstacles[y].insert(n.x);
			} else if (n.y == p.y) {
				obstacles[n.y].insert(interval{p.x, n.x});
			} else [[unlikely]] {
				throw std::runtime_error("Unaligned line");
			}
			p = std::move(n);
		}
		if (!in)
			break;
	}
	if (!in.eof())
		throw std::runtime_error("Puzzle input error");
	floor_y = max_y + 2;
	obstacles[floor_y].insert(interval{limits::min(), limits::max()});
}

point sand_simulation::drop_sand()
{
	point s{500, 0};
	if (collide(s)) [[unlikely]]
		return s;
fall:
	++s.y;
	const auto sand_it = obstacles.find(s.y);
	if (!collide(s, sand_it))
		goto fall;
	--s.x;
	if (!collide(s, sand_it))
		goto fall;
	s.x += 2;
	if (!collide(s, sand_it))
		goto fall;
	--s.y;
	--s.x;
	obstacles[s.y].insert(s.x);
	return s;
}

bool
sand_simulation::collide(const point p, hint_type hint) const noexcept
{
	return hint != obstacles.cend() && hint->second.contains(p.x);
}

bool sand_simulation::collide(const point p) const noexcept
{
	return collide(p, obstacles.find(p.y));
}

}

template<> output_pair day<14>(std::istream& in)
{
	sand_simulation sim{in};
	std::uintmax_t count = 0;
	std::uintmax_t fell = 0;
	for (;;) {
		const point s = sim.drop_sand();
		if (sim.has_floor(s.y + 1) && fell == 0)
			fell = count;
		if (s.x == 500 && s.y == 0)
			break;
		++count;
	}
	return {fell, count + 1};
}
