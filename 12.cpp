#include <algorithm>
#include <cstdint>
#include <execution>
#include <istream>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "common.h"

namespace {

class hill_map {
public:
	explicit hill_map(std::istream& in);
	std::pair<std::uintmax_t, std::uintmax_t> get_distances() const;

private:
	using size_type = std::vector<unsigned char>::size_type;

	std::vector<unsigned char> map{};
	size_type width = 0;
	size_type start{};
	size_type goal{};
};

hill_map::hill_map(std::istream& in)
{
	constexpr char a[] = "abcdefghijklmnopqrstuvwxyz";
	std::istream::int_type c;
	size_type x = 0;
	bool set_start = false;
	bool set_goal = false;
	const auto check_width = [this, &x] {
		if (width != 0 && x > width)
			throw std::runtime_error("Inconsistent width");
	};
	while ((c = in.get()) != std::istream::traits_type::eof()) {
		if (c == '\n') {
			if (x == 0)
				continue;
			if (width == 0)
				width = x;
			else if (x != width)
				throw std::runtime_error("Inconsistent width");
			x = 0;
		} else if (c == 'S') {
			check_width();
			set_start = true;
			start = map.size();
			map.push_back(0);
			++x;
		} else if (c == 'E') {
			check_width();
			set_goal = true;
			goal = map.size();
			map.push_back(25);
			++x;
		} else {
			check_width();
			const unsigned char b = static_cast<unsigned char>(c);
			const auto it = std::find(std::execution::unseq, a,
			                          a + 26, b);
			if (it == a + 26)
				throw std::runtime_error("Bad character");
			map.push_back(static_cast<unsigned char>(it - a));
			++x;
		}
	}
	if (!in.eof())
		throw std::runtime_error("Puzzle input error");
	if (!set_start)
		throw std::runtime_error("No starting point specified");
	if (!set_goal)
		throw std::runtime_error("No goal specified");
	if (width == 0)
		width = map.size();
	else if (map.size() % width != 0)
		throw std::runtime_error("Inconstistent width");
}

std::pair<std::uintmax_t, std::uintmax_t> hill_map::get_distances() const
{
	using frontier_type = std::vector<size_type>;
	std::vector<bool> explored(map.size(), false);
	explored[goal] = true;
	frontier_type frontier;
	frontier_type next_frontier;
	const auto add_neighbors =
		[this, &explored](frontier_type& to, size_type i) {
			const unsigned char t = map[i] - 1;
			size_type n = i - width;
			const auto add_n = [&n, &explored, &to] {
				explored[n] = true;
				to.push_back(n);
			};
			if (i > width && !explored[n] && map[n] >= t)
				add_n();
			n = i + width;
			if (map.size() - width > i && !explored[n]
			    && map[n] >= t)
				add_n();
			n = i - 1;
			if (i % width > 0 && !explored[n] && map[n] >= t)
				add_n();
			n = i + 1;
			if (i % width < width - 1 && !explored[n]
			    && map[n] >= t)
				add_n();
		};
	add_neighbors(frontier, goal);
	std::optional<std::uintmax_t> opt_start;
	std::optional<std::uintmax_t> opt_any;
	std::uintmax_t distance = 0;
	while (!frontier.empty()) [[likely]] {
		++distance;
		for (size_type i : frontier) {
			if (i == start)
				opt_start = distance;
			if (!opt_any && map[i] == 0)
				opt_any = distance;
			if (opt_start && opt_any)
				return {*opt_start, *opt_any};
			add_neighbors(next_frontier, i);
		}
		frontier = std::move(next_frontier);
	}
	throw std::logic_error("No path was found");
}

}

template<> output_pair day<12>(std::istream& in)
{
	const hill_map m{in};
	auto [start, any] = m.get_distances();
	return {std::move(start), std::move(any)};
}
