#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <ios>
#include <istream>
#include <limits>
#include <memory>

#include "common.h"

namespace {

enum class cube { empty, lava, water };

class droplet {
public:
	explicit droplet(std::istream& in) {
		using limit = std::numeric_limits<std::size_t>;
		using traits = std::istream::traits_type;
		std::deque<point> points;
		point p;
		while (read_point(in, p)) {
			if (p.x > width)
				width = p.x;
			if (p.y > height)
				height = p.y;
			if (p.z > depth)
				depth = p.z;
			points.push_back(p);
			const traits::int_type c = in.get();
			if (c != traits::to_int_type('\n')) {
				in.setstate(std::ios_base::failbit);
				break;
			}
		}
		if (!in.eof())
			throw std::runtime_error("Puzzle input parsing error");
		constexpr std::size_t max_coord =
			(std::size_t{1} << (limit::digits / 3)) - 2;
		if (width > max_coord || height > max_coord
		    || depth > max_coord)
			throw std::runtime_error("Droplet is too big");
		++width;
		++height;
		++depth;
		mask = std::make_unique<cube[]>(width * height * depth);
		std::fill(begin(), end(), cube::empty);
		for (point m : points)
			coord(m.x, m.y, m.z) = cube::lava;
		submerge();
	}

	[[nodiscard]] constexpr std::uintmax_t surface() const noexcept {
		constexpr auto p = [](cube c) { return c != cube::lava; };
		std::uintmax_t count = 0;
		for (std::size_t x = 0; x < width; ++x) {
			for (std::size_t y = 0; y < height; ++y) {
				for (std::size_t z = 0; z < depth; ++z) {
					count += surface_cube(x, y, z, p);
				}
			}
		}
		return count;
	}

	[[nodiscard]] constexpr std::uintmax_t surface_water() const noexcept {
		constexpr auto p = [](cube c) { return c == cube::water; };
		std::uintmax_t count = 0;
		for (std::size_t x = 0; x < width; ++x) {
			for (std::size_t y = 0; y < height; ++y) {
				for (std::size_t z = 0; z < depth; ++z) {
					count += surface_cube(x, y, z, p);
				}
			}
		}
		return count;
	}

private:
	struct point {
		std::size_t x;
		std::size_t y;
		std::size_t z;
	};

	[[nodiscard]] cube* begin() noexcept { return mask.get(); }

	[[nodiscard]] cube* end() noexcept {
		return mask.get() + width * height * depth;
	}

	static std::istream& read_point(std::istream& in, point& p) {
		const auto comma = [&in] {
			using traits = std::istream::traits_type;
			return in.get() == traits::to_int_type(',');
		};
		if (!(in >> p.x) || !comma() || !(in >> p.y) || !comma())
			in.setstate(std::ios_base::failbit);
		return in >> p.z;
	}

	[[nodiscard]] cube&
	coord(std::size_t x, std::size_t y, std::size_t z) noexcept
	{
		return mask.get()[(z * height + y) * width + x];
	}

	[[nodiscard]] const cube&
	coord(std::size_t x, std::size_t y, std::size_t z) const noexcept
	{
		return mask.get()[(z * height + y) * width + x];
	}

	void submerge() {
		submerge_z_plan(0);
		for (std::size_t z = 1; z + 1 < depth; ++z) {
			submerge_y_line(0, z);
			for (std::size_t y = 1; y + 1 < height; ++y) {
				submerge_cube(0, y, z);
				submerge_cube(width - 1, y, z);
			}
			submerge_y_line(height - 1, z);
		}
		submerge_z_plan(depth - 1);
	}

	void submerge_z_plan(const std::size_t z) {
		for (std::size_t y = 0; y < height; ++y)
			submerge_y_line(y, z);
	}

	void submerge_y_line(const std::size_t y, const std::size_t z) {
		for (std::size_t x = 0; x < width; ++x)
			submerge_cube(x, y, z);
	}

	void submerge_cube(std::size_t x, std::size_t y, std::size_t z) {
		cube& p = coord(x, y, z);
		if (p != cube::empty)
			return;
		p = cube::water;
		if (x > 0)
			submerge_cube(x - 1, y, z);
		if (x + 1 < width)
			submerge_cube(x + 1, y, z);
		if (y > 0)
			submerge_cube(x, y - 1, z);
		if (y + 1 < height)
			submerge_cube(x, y + 1, z);
		if (z > 0)
			submerge_cube(x, y, z - 1);
		if (z + 1 < depth)
			submerge_cube(x, y, z + 1);
	}

	template<class P>
	constexpr std::uintmax_t
	surface_cube(std::size_t x, std::size_t y, std::size_t z, const P& p)
		const noexcept
	{
		if (coord(x, y, z) != cube::lava)
			return 0;
		std::uintmax_t count = 0;
		if (x == 0 || std::invoke(p, coord(x - 1, y, z)))
			++count;
		if (x + 1 == width || std::invoke(p, coord(x + 1, y, z)))
			++count;
		if (y == 0 || std::invoke(p, coord(x, y - 1, z)))
			++count;
		if (y + 1 == height || std::invoke(p, coord(x, y + 1, z)))
			++count;
		if (z == 0 || std::invoke(p, coord(x, y, z - 1)))
			++count;
		if (z + 1 == depth || std::invoke(p, coord(x, y, z + 1)))
			++count;
		return count;
	}

	std::size_t width = 0;
	std::size_t height = 0;
	std::size_t depth = 0;
	std::unique_ptr<cube[]> mask = nullptr;
};

}

template<> output_pair day<18>(std::istream& in)
{
	const droplet d{in};
	return {d.surface(), d.surface_water()};
}
