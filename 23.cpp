#include <algorithm>
#include <cstdint>
#include <istream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "checked.h"
#include "common.h"

namespace {

enum class direction {north, south, west, east};

struct point {
	constexpr std::strong_ordering operator<=>(const point&) const noexcept
		= default;

	std::size_t x;
	std::size_t y;
};

class elf_herd {
public:
	explicit elf_herd(std::istream& in);

	[[nodiscard]] std::uintmax_t empty_space() const {
		using limits = std::numeric_limits<std::size_t>;
		if (coords.empty())
			return 0;
		std::size_t min_x = limits::max();
		std::size_t max_x = limits::min();
		std::size_t min_y = limits::max();
		std::size_t max_y = limits::min();
		for (const auto& [x, y] : coords) {
			if (x < min_x)
				min_x = x;
			if (x > max_x)
				max_x = x;
			if (y < min_y)
				min_y = y;
			if (y > max_y)
				max_y = y;
		}
		const std::size_t w = max_x - min_x + 1;
		const std::size_t h = max_y - min_y + 1;
		std::size_t area;
		if (checked_mul(area, w, h))
			throw std::invalid_argument("Area too big");
		return area - coords.size();
	}

	[[nodiscard]] constexpr std::uintmax_t get_round() const noexcept {
		return round;
	}

	constexpr operator bool() const noexcept { return stabilized; }

	void resume() {
		if (stabilized)
			return;
		++round;
		insert_margins();
		move_elves(compute_propositions());
		rotate_directions();
	}

private:
	void insert_margins();

	std::unique_ptr<std::pair<std::size_t, std::size_t>[]>
	compute_propositions() const;

	void
	move_elves(std::unique_ptr<std::pair<std::size_t, std::size_t>[]>);

	void rotate_directions() noexcept {
		direction d = *std::begin(dirs);
		std::shift_left(std::begin(dirs), std::end(dirs), 1);
		*std::next(std::begin(dirs), 3) = std::move(d);
	}

	[[nodiscard]] std::optional<point>
	propose_dest(std::size_t x, std::size_t y) const {
		std::uint_fast8_t mask = 0;
		if (!occupied[y * width + x + 1])
			mask |= 1u;
		if (!occupied[(y + 1) * width + x + 1])
			mask |= 1u << 1;
		if (!occupied[(y + 1) * width + x])
			mask |= 1u << 2;
		if (!occupied[(y + 1) * width + x - 1])
			mask |= 1u << 3;
		if (!occupied[y * width + x - 1])
			mask |= 1u << 4;
		if (!occupied[(y - 1) * width + x - 1])
			mask |= 1u << 5;
		if (!occupied[(y - 1) * width + x])
			mask |= 1u << 6;
		if (!occupied[(y - 1) * width + x + 1])
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
	try_propose(std::size_t x, std::size_t y, direction dir,
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

	std::size_t width = 0;
	std::vector<bool> occupied{};
	std::vector<std::pair<std::size_t, std::size_t>> coords{};
	direction dirs[4] = {direction::north, direction::south,
	                     direction::west, direction::east};
	std::uintmax_t round = 0;
	bool stabilized = false;
};

elf_herd::elf_herd(std::istream& in)
{
	using traits = std::istream::traits_type;
	using limits = std::numeric_limits<std::size_t>;
	std::size_t x = 0;
	std::size_t y = 0;
	std::istream::int_type i;
	bool first_line = true;
	while ((i = in.get()) != traits::eof()) {
		if (i == in.widen('#')) {
			occupied.push_back(true);
			coords.push_back(std::pair{x, y});
			++x;
		} else if (i == in.widen('\n')) {
			if (y == limits::max() - 1)
				throw std::runtime_error("Too big");
			++y;
			if (first_line) {
				first_line = false;
				width = x;
			} else if (x != width) {
				std::ostringstream s;
				s << "First line had length " << width
				  << ", line " << y << " has length " << x;
				throw std::runtime_error(s.str());
			}
			x = 0;
		} else {
			++x;
			if (x == limits::max())
				throw std::runtime_error("Too big");
			occupied.push_back(false);
		}
	}
	if (!in.eof()) {
		std::ostringstream s;
		s << "Error occurred on line " << y;
		throw std::runtime_error(s.str());
	}
	if (x > 0 && x != width) {
		std::ostringstream s;
		s << "First line had length " << width
		  << ", last has length " << x;
		throw std::runtime_error(s.str());
	}
}

void elf_herd::insert_margins()
{
	std::size_t up = 0, down = 0, left = 0, right = 0;
	const auto height = occupied.size() / width;
	for (const auto& [x, y] : coords) {
		if (x == 0)
			left = 1;
		if (x + 1 == width)
			right = 1;
		if (y == 0)
			up = 1;
		if (y + 1 == height)
			down = 1;
	}
	if (!(up | down | left | right))
		return;
	const auto nw = width + left + right;
	const auto nh = height + up + down;
	occupied.resize(nw * nh, false);
	for (std::size_t y = nh - down; y-- != up; (void) 0) {
		if (right)
			occupied[(y + 1) * nw - 1] = false;
		std::copy_backward(occupied.cbegin() + (y - up) * width,
		                   occupied.cbegin() + (y - up + 1) * width,
		                   occupied.begin() + y * nw + left + width);
		if (left)
			occupied[y * nw] = false;
	}
	if (up)
		std::fill(occupied.begin(), occupied.begin() + nw, false);
	width = nw;
	if (!(up | left))
		return;
	for (auto& [x, y] : coords) {
		x += left;
		y += up;
	}
}

std::unique_ptr<std::pair<std::size_t, std::size_t>[]>
elf_herd::compute_propositions() const
{
	const std::size_t num_elves = coords.size();
	std::unique_ptr<std::pair<std::size_t, std::size_t>[]> prop =
		std::make_unique<std::pair<std::size_t, std::size_t>[]>(
			num_elves
		);
	for (std::size_t e = 0; e < num_elves; ++e) {
		const auto& [x, y] = coords[e];
		if (auto opt = propose_dest(x, y)) {
			const auto& [dx, dy] = *opt;
			prop[e] = std::pair{dx, dy};
		} else {
			prop[e] = coords[e];
		}
	}
	return prop;
}

void elf_herd::move_elves(
	std::unique_ptr<std::pair<std::size_t, std::size_t>[]> prop
)
{
	bool has_moved = false;
	std::vector<bool> blocked(coords.size(), false);
	for (std::size_t e = 0; e < coords.size(); ++e) {
		if (blocked[e])
			continue;
		for (std::size_t f = e + 1; f < coords.size(); ++f) {
			if (prop[f] == prop[e])
				blocked[e] = blocked[f] = true;
		}
		if (blocked[e])
			continue;
		if (prop[e] == coords[e])
			continue;
		const auto& [xfrom, yfrom] = coords[e];
		const auto& [xto, yto] = prop[e];
		occupied[yfrom * width + xfrom] = false;
		occupied[yto * width + xto] = true;
		coords[e] = prop[e];
		has_moved = true;
	}
	if (!has_moved)
		stabilized = true;
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
