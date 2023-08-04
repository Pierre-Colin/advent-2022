#include <algorithm>
#include <cstdint>
#include <execution>
#include <istream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "checked.h"
#include "common.h"

namespace {

enum class direction {north, south, west, east};

struct point {
	constexpr std::strong_ordering operator<=>(const point&) const noexcept
		= default;

	std::intmax_t x;
	std::intmax_t y;
};

[[nodiscard]]
static std::set<point> second_half(std::map<point, point>&&);

class elf_herd {
public:
	explicit elf_herd(std::istream& in) {
		using traits = std::istream::traits_type;
		using limits = std::numeric_limits<std::intmax_t>;
		std::intmax_t x = 0;
		std::intmax_t y = 0;
		std::istream::int_type i;
		while ((i = in.get()) != traits::eof()) {
			if (i == in.widen('#'))
				elves.insert({x, y});
			if (i == in.widen('\n')) {
				x = 0;
				if (y == limits::max() - 1)
					throw std::runtime_error("Too big");
				++y;
			} else {
				if (x == limits::max() - 1)
					throw std::runtime_error("Too big");
				++x;
			}
		}
		if (!in.eof()) {
			std::ostringstream s;
			s << "Error occurred on line " << y;
			throw std::runtime_error(s.str());
		}
	}

	[[nodiscard]] std::uintmax_t empty_space() const {
		using limits = std::numeric_limits<std::intmax_t>;
		if (elves.empty())
			return 0;
		std::intmax_t min_x = limits::max();
		std::intmax_t max_x = limits::min();
		std::intmax_t min_y = limits::max();
		std::intmax_t max_y = limits::min();
		for (const auto [x, y] : elves) {
			if (x < min_x)
				min_x = x;
			if (x > max_x)
				max_x = x;
			if (y < min_y)
				min_y = y;
			if (y > max_y)
				max_y = y;
		}
		std::intmax_t width, height, area;
		if (checked_sub(width, max_x, min_x)
		    || checked_add(width, width, std::intmax_t{1})
		    || checked_sub(height, max_y, min_y)
		    || checked_add(height, height, std::intmax_t{1})
		    || checked_mul(area, width, height))
			throw std::invalid_argument("Area too big");
		return static_cast<std::uintmax_t>(area) - elves.size();
	}

	[[nodiscard]] constexpr std::uintmax_t get_round() const noexcept {
		return round;
	}

	constexpr operator bool() const noexcept { return stabilized; }

	void resume() {
		if (stabilized)
			return;
		auto [stable, proposed] = first_half();
		++round;
		if (proposed.empty()) {
			stabilized = true;
			return;
		}
		const auto moved = second_half(std::move(proposed));
		stable.insert(moved.cbegin(), moved.cend());
		elves = std::move(stable);
		direction d = *std::begin(dirs);
		std::shift_left(std::begin(dirs), std::end(dirs), 1);
		*std::next(std::begin(dirs), 3) = std::move(d);
	}

private:
	[[nodiscard]]
	std::pair<std::set<point>, std::map<point, point>>
	first_half() const {
		std::set<point> stable;
		std::map<point, point> proposed;
		for (const auto [x, y] : elves) {
			if (auto opt = propose_dest(x, y)) {
				proposed.insert({point{x, y},
				                 *std::move(opt)});
			} else {
				stable.emplace(x, y);
			}
		}
		return {std::move(stable), std::move(proposed)};
	}

	[[nodiscard]] std::optional<point>
	propose_dest(std::intmax_t x, std::intmax_t y) const {
		std::uint_fast8_t mask = 0;
		if (!elves.contains(point{x + 1, y}))
			mask |= 1u;
		if (!elves.contains(point{x + 1, y + 1}))
			mask |= 1u << 1;
		if (!elves.contains(point{x, y + 1}))
			mask |= 1u << 2;
		if (!elves.contains(point{x - 1, y + 1}))
			mask |= 1u << 3;
		if (!elves.contains(point{x - 1, y}))
			mask |= 1u << 4;
		if (!elves.contains(point{x - 1, y - 1}))
			mask |= 1u << 5;
		if (!elves.contains(point{x, y - 1}))
			mask |= 1u << 6;
		if (!elves.contains(point{x + 1, y - 1}))
			mask |= 1u << 7;
		if (mask == 0b11111111)
			return std::nullopt;
		for (const direction d : dirs) {
			if (auto opt = try_propose(x, y, d, mask))
				return *std::move(opt);
		}
		return point{x, y};
	}

	[[nodiscard]] std::optional<point>
	try_propose(std::intmax_t x, std::intmax_t y, direction dir,
	            std::uint_fast8_t mask)
		const
	{
		switch (dir) {
		case direction::north:
			if ((mask & 0b11100000) == 0b11100000)
				return point{x, y - 1};
			return std::nullopt;
		case direction::south:
			if ((mask & 0b00001110) == 0b00001110)
				return point{x, y + 1};
			return std::nullopt;
		case direction::west:
			if ((mask & 0b00111000) == 0b00111000)
				return point{x - 1, y};
			return std::nullopt;
		case direction::east:
			if ((mask & 0b10000011) == 0b10000011)
				return point{x + 1, y};
			return std::nullopt;
		[[unlikely]] default:
			throw std::logic_error("Reached unreachable code");
		}
	}

	std::set<point> elves{};
	direction dirs[4] = {direction::north, direction::south,
	                     direction::west, direction::east};
	std::uintmax_t round = 0;
	bool stabilized = false;
};

static std::set<point> second_half(std::map<point, point>&& proposed)
{
	std::set<point> result;
	while (!proposed.empty()) {
		const auto f = proposed.extract(proposed.begin());
		const auto p = [&f](const std::unordered_map<point, point>::value_type& n) {
			return n.second == f.mapped();
		};
		auto it = std::find_if(proposed.begin(), proposed.end(), p);
		if (it == proposed.end()) {
			result.insert(f.mapped());
			continue;
		}
		result.insert(f.key());
		do {
			result.insert(it->first);
			it = std::find_if(proposed.erase(it), proposed.end(),
			                  p);
		} while (it != proposed.end());
	}
	return result;
}

}

template<> output_pair day<23>(std::istream& in)
{
	elf_herd herd{in};
	for (int i = 0; i < 10 && !herd; ++i)
		herd.resume();
	const std::uintmax_t part1 = herd.empty_space();
	while (!herd)
		herd.resume();
	return {part1, herd.get_round()};
}
