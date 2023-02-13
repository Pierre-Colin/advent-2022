#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <istream>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "common.h"
#include "interval.h"
#include "interval_union.h"

// TODO: try to optimize the interval_union away

namespace {

[[nodiscard]] constexpr std::uintmax_t abs(std::intmax_t x) noexcept;

struct point {
	constexpr point operator-(const point& rhs) const noexcept {
		return {x - rhs.x, y - rhs.y};
	}

	[[nodiscard]] constexpr std::uintmax_t norm() const noexcept {
		return abs(x) + abs(y);
	}

	std::intmax_t x;
	std::intmax_t y;
};

class sensor_reading {
public:
	explicit constexpr sensor_reading(const point p, const point c)
		noexcept
		: position{p}
		, closest{c}
		, distance{(p - c).norm()}
	{}

	[[nodiscard]] constexpr std::optional<interval<std::intmax_t>>
	beaconless_abscissas(std::intmax_t row) const noexcept;

	[[nodiscard]] constexpr std::optional<interval<std::intmax_t>>
	vision_slice(std::intmax_t row) const noexcept;

private:
	point position;
	point closest;
	std::uintmax_t distance;
};

class sensor_report {
	friend std::istream& operator>>(std::istream&, sensor_report&);
public:
	constexpr sensor_report() noexcept {}
	[[nodiscard]] std::uintmax_t beaconless_positions() const;
	[[nodiscard]] constexpr std::uintmax_t beacon_tuning_frequency() const;

private:
	std::vector<sensor_reading> readings{};
};

[[nodiscard]] constexpr std::uintmax_t abs(std::intmax_t x) noexcept
{
	return x >= 0 ? x : -x;
}

static bool read_expect(std::istream& in, std::span<char, 25> buf,
                        std::span<const char> exp)
{
	return in.read(buf.data(), exp.size())
	       && std::equal(exp.begin(), exp.end(), buf.begin());
}

static bool read_point(std::istream& in, std::span<char, 25> buf, point& p)
{
	static constexpr char coord_sep[] = {',', ' ', 'y', '='};
	return (in >> p.x) && read_expect(in, buf, coord_sep) && (in >> p.y);
}

std::istream& operator>>(std::istream& in, sensor_report& r)
{
	static constexpr char sensor_at[] = "Sensor at x=";
	static constexpr char closest[] = ": closest beacon is at x=";
	char buf[25];
	while (in.peek() == '\n')
		in.ignore();
	if (!in.read(buf, 12))
		return in;
	if (!std::equal(sensor_at, sensor_at + 12, std::cbegin(buf))) {
		in.setstate(std::ios_base::failbit);
		return in;
	}
	point s;
	if (!read_point(in, buf, s) || !read_expect(in, buf, {closest, 25})) {
		in.setstate(std::ios_base::failbit);
		return in;
	}
	point b;
	if (read_point(in, buf, b) && in.get() == '\n')
		r.readings.emplace_back(s, b);
	else
		in.setstate(std::ios_base::failbit);
	return in;
}

std::uintmax_t sensor_report::beaconless_positions()
	const
{
	const std::intmax_t row = readings.size() == 14 ? 10 : 2000000;
	interval_union u;
	for (const sensor_reading& reading : readings) {
		auto i = reading.beaconless_abscissas(row);
		if (i)
			u.insert(std::move(i).value());
	}
	return u.cardinal();
}

constexpr std::uintmax_t sensor_report::beacon_tuning_frequency()
	const
{
	const std::intmax_t limit = readings.size() == 14 ? 20 : 4000000;
	for (std::intmax_t y = 0; y <= limit; ++y) {
		interval_union u;
		for (const sensor_reading& reading: readings) {
			auto i = reading.vision_slice(y);
			if (i)
				u.insert(std::move(i).value());
		}
		auto o = u.dead_spot(interval<std::intmax_t>{0, limit});
		if (o)
			return 4000000 * o.value() + y;
	}
	throw std::runtime_error("No solution found");
}

constexpr std::optional<interval<std::intmax_t>>
sensor_reading::beaconless_abscissas(std::intmax_t row) const noexcept
{
	const std::uintmax_t dy = abs(position.y - row);
	if (dy > distance)
		return std::nullopt;
	if (dy == distance) {
		if (closest.y != row)
			return interval(position.x, position.x);
		return std::nullopt;
	}
	const std::intmax_t dx = distance - dy;
	if (closest.y != row)
		return interval(position.x - dx, position.x + dx);
	if (closest.x == position.x - dx)
		return interval(position.x - dx + 1, position.x + dx);
	return interval(position.x - dx, position.x + dx - 1);
}

constexpr std::optional<interval<std::intmax_t>>
sensor_reading::vision_slice(std::intmax_t row) const noexcept
{
	const std::uintmax_t dy = abs(position.y - row);
	if (dy > distance)
		return std::nullopt;
	const std::intmax_t dx = distance - dy;
	return interval(position.x - dx, position.x + dx);
}

}

template<> output_pair day<15>(std::istream& in)
{
	sensor_report r;
	while (in >> r);
	if (!in.eof())
		throw std::runtime_error("Error while reading puzzle input");
	return {r.beaconless_positions(), r.beacon_tuning_frequency()};
}
