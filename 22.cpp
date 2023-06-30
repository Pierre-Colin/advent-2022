#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <istream>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "common.h"

namespace {

enum class direction { right, down, left, up };

enum class cell { outside, empty, obstacle };

struct transition {
	int new_sqr;
	direction new_dir;
};

static constexpr direction turn_right(const direction dir) noexcept
{
	return static_cast<direction>((static_cast<int>(dir) + 1) % 4);
}

static constexpr direction turn_left(const direction dir) noexcept
{
	return static_cast<direction>((static_cast<int>(dir) + 3) % 4);
}

class line_parser {
public:
	[[nodiscard]] bool read_line(std::istream& in) {
		constexpr auto invalid_char = [](const char c) {
			return c != ' ' && c != '.' && c != '#';
		};
		if (parsing_line > 250) [[unlikely]]
			throw std::runtime_error("Puzzle input is too high");
		char line[251];
		if (!in.get(line, 251)) [[unlikely]] {
			if (in.eof()) {
				std::stringstream s;
				s << "Could not input line " << parsing_line;
				throw std::runtime_error(s.str());
			}
			in.clear();
		}
		if (line[0] == 0)
			return true;
		if (std::size(lines) >= 250) [[unlikely]]
			throw std::runtime_error("Board is too high");
		const auto it = std::find_if(std::cbegin(line),
		                             std::prev(std::cend(line)),
		                             invalid_char);
		if (*it != 0) [[unlikely]] {
			std::stringstream s;
			s << "Unexpected char '" << *it << "' on line "
			  << parsing_line;
			throw std::runtime_error(s.str());
		} else if (in.get() != in.widen('\n')) [[unlikely]] {
			std::stringstream s;
			s << "Line " << parsing_line << " is too long";
			throw std::runtime_error(s.str());
		}
		lines.emplace_back(line, std::distance(std::cbegin(line), it));
		++parsing_line;
		return false;
	}

	[[nodiscard]]
	std::tuple<int, int, std::unique_ptr<cell[]>> build() const;

private:
	std::vector<std::string> lines{};
	int parsing_line = 1;
};

std::tuple<int, int, std::unique_ptr<cell[]>> line_parser::build() const
{
	int width = 0;
	for (const auto& line : lines) {
		const std::string::size_type t = std::size(line);
		if (t > static_cast<std::string::size_type>(width))
			width = static_cast<int>(t);
	}
	std::unique_ptr<cell[]> data =
		std::make_unique<cell[]>(width * std::size(lines));
	std::fill(data.get(), data.get() + width * std::size(lines),
	          cell::outside);
	for (int y = 0; y < static_cast<int>(std::size(lines)); ++y) {
		const int string_width = static_cast<int>(std::size(lines[y]));
		for (int x = 0; x < string_width; ++x) {
			if (lines[y][x] == '.')
				data[y * width + x] = cell::empty;
			else if (lines[y][x] == '#')
				data[y * width + x] = cell::obstacle;
		}
	}
	return {width, std::size(lines), std::move(data)};
}

class grid {
public:
	grid() = delete;

	grid(std::istream& in) {
		line_parser p;
		while (!p.read_line(in));
		std::tie(width, height, data) = p.build();
		while (start_x < width && at(start_x, 0) == cell::outside)
			++start_x;
		if (start_x >= width) [[unlikely]]
			throw std::invalid_argument("First line is empty");
		if (auto opt = find_squares()) {
			sqr_size = opt->first;
			for (int i = 0; i < 6; ++i) {
				const auto& a = opt->second[i];
				squares[i].first = a.first / sqr_size;
				squares[i].second = a.second / sqr_size;
			}
			fill_transitions();
		} else {
			throw std::runtime_error("Board is not 6 squares");
		}
		for (;;) {
			std::istream::int_type c = in.peek();
			int instruction;
			if (c == in.widen('L')) {
				instruction = -1;
				in.ignore();
			} else if (c == in.widen('R')) {
				instruction = -2;
				in.ignore();
			} else if (!(in >> instruction) || instruction < 0) {
				break;
			}
			instructions.push_back(instruction);
		}
		if (!in.eof())
			throw std::runtime_error("Invalid instruction list");
	}

	[[nodiscard]] cell at(int x, int y) const noexcept {
		return data[y * width + x];
	}

	[[nodiscard]] std::uintmax_t solve_flat() const {
		return solve(&grid::step_flat);
	}

	[[nodiscard]] std::uintmax_t solve_cube() const {
		return solve(&grid::step_cube);
	}

private:
	[[nodiscard]]
	std::optional<std::pair<int, std::array<std::pair<int, int>, 6>>>
	find_squares() const {
		const auto cont = [this](const int s) {
			return s <= 50 && 2 * s <= height && s <= start_x;
		};
		for (int s = 1; cont(s); ++s) {
			if (auto opt = find_squares(s))
				return std::pair(s, *opt);
		}
		return std::nullopt;
	}

	[[nodiscard]] std::optional<std::array<std::pair<int, int>, 6>>
	find_squares(int s) const {
		if (start_x % s != 0 || width % s != 0 || height % s != 0)
			return std::nullopt;
		std::array<std::pair<int, int>, 6> sqr{std::pair(start_x, 0)};
		auto it = std::next(std::begin(sqr));
		for (int y = 0; y < height; y += s) {
			const int b = y == 0 ? start_x + s : 0;
			for (int x = b; x < width; x += s) {
				if (at(x, y) == cell::outside)
					continue;
				*it++ = {x, y};
				if (it != std::end(sqr))
					continue;
				return check_squares(sqr, s) ? std::make_optional(std::move(sqr))
				                             : std::nullopt;
			}
		}
		return std::nullopt;
	}

	[[nodiscard]] bool
	check_squares(std::span<const std::pair<int, int>, 6> sqr, int sz)
		const noexcept
	{
		const auto is_in_square = [=, this](int x, int y) {
			const auto b = [=](const std::pair<int, int>& p) {
				return p.first <= x && x < p.first + sz
				       && p.second <= y && y < p.second + sz;
			};
			return std::find_if(sqr.begin(), sqr.end(), b)
				!= sqr.end();
		};
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				const bool b = is_in_square(x, y);
				const bool c = at(x, y) != cell::outside;
				if (b != c)
					return false;
			}
		}
		return true;
	}

	void fill_transitions() {
		std::ranges::fill(transitions,
		                  transition{-1, direction::right});
		for (int i = 0; i < 5; ++i) {
			for (int j = i + 1; j < 6; ++j)
				fill_trivial_transition(i, j);
		}
		for (int i = 0; i < 6; ++i)
			validate_neighbors(i);
		while (fold());
		if (!is_folded())
			throw std::runtime_error("Board is not a cube net");
	}

	bool fold() noexcept {
		bool changed = false;
		for (int i = 0; i < 6; ++i) {
			for (int d = 0; d < 4; ++d) {
				if (transitions[4 * i + d].new_sqr >= 0)
					continue;
				if (try_fold(i, d))
					changed = true;
			}
		}
		return changed;
	}

	constexpr bool try_fold(const int i, int d) noexcept {
		transition& t = transitions[4 * i + d];
		d = (d + 3) % 4;
		if (transitions[4 * i + d].new_sqr >= 0) {
			const transition& u = transitions[4 * i + d];
			const int j = u.new_sqr;
			const int e = (static_cast<int>(u.new_dir) + 1) % 4;
			if (transitions[4 * j + e].new_sqr >= 0) {
				const transition& v = transitions[4 * j + e];
				t.new_sqr = v.new_sqr;
				t.new_dir = turn_left(v.new_dir);
				return true;
			}
		} else {
			d = (d + 2) % 4;
			if (transitions[4 * i + d].new_sqr >= 0) {
				const transition& u = transitions[4 * i + d];
				const int j = u.new_sqr;
				const int e =
					(static_cast<int>(u.new_dir) + 3) % 4;
				if (transitions[4 * j + e].new_sqr >= 0) {
					const auto& v = transitions[4 * j + e];
					t.new_sqr = v.new_sqr;
					t.new_dir = turn_right(v.new_dir);
					return true;
				}
			}
		}
		return false;
	}

	[[nodiscard]] constexpr bool is_folded() const noexcept {
		constexpr auto p = [](const transition& t) {
			return t.new_sqr >= 0;
		};
		return std::ranges::all_of(transitions, p);
	}

	void fill_trivial_transition(const int i, const int j) {
		const auto& [xi, yi] = squares[i];
		const auto& [xj, yj] = squares[j];
		if (xi == xj) {
			if (yi == yj + 1) {
				transitions[4 * i + 3] = {j, direction::up};
				transitions[4 * j + 1] = {i, direction::down};
			} else if (yi == yj - 1) {
				transitions[4 * i + 1] = {j, direction::down};
				transitions[4 * j + 3] = {i, direction::up};
			}
		} else if (yi == yj) {
			if (xi == xj + 1) {
				transitions[4 * i + 2] = {j, direction::left};
				transitions[4 * j] = {i, direction::right};
			} else if (xi == xj - 1) {
				transitions[4 * i] = {j, direction::right};
				transitions[4 * j + 2] = {i, direction::left};
			}
		}
	}

	void validate_neighbors(const int i) const {
		constexpr auto p = [](const transition& t) {
			return t.new_sqr >= 0;
		};
		const auto it = std::next(std::cbegin(transitions), 4 * i);
		if (std::none_of(it, std::next(it, 4), p)) {
			std::ostringstream s;
			s << "Square " << (i + 1) << " has no neighbor";
			throw std::runtime_error(s.str());
		}
	}

	[[nodiscard]] std::uintmax_t
	solve(std::tuple<int, int, direction> (grid::*step)(int, int, direction) const noexcept)
		const
	{
		int x = start_x;
		int y = 0;
		direction dir = direction::right;
		for (int instr : instructions) {
			if (instr < -2) [[unlikely]]
				throw std::logic_error("Bad instruction");
			switch (instr) {
			case -2:
				dir = turn_right(dir);
				break;
			case -1:
				dir = turn_left(dir);
				break;
			default:
				std::tie(x, y, dir) =
					walk(x, y, dir, instr, step);
			}
		}
		return 1000 * static_cast<std::uintmax_t>(y + 1)
		       + 4 * static_cast<std::uintmax_t>(x + 1)
		       + static_cast<int>(dir);
	}

	[[nodiscard]] std::tuple<int, int, direction>
	walk(int x, int y, direction dir, int dist, std::tuple<int, int, direction> (grid::*step)(int, int, direction) const noexcept)
		const noexcept
	{
		while (dist-- > 0) {
			auto t = std::invoke(step, *this, x, y, dir);
			if (at(std::get<0>(t), std::get<1>(t)) == cell::obstacle)
				break;
			std::tie(x, y, dir) = std::move(t);
		}
		return {x, y, dir};
	}

	[[nodiscard]] std::tuple<int, int, direction>
	step_flat(int x, int y, direction dir) const noexcept {
		int to;
		switch (dir) {
		case direction::right:
			if (x + 1 < width && at(x + 1, y) != cell::outside) {
				to = x + 1;
			} else {
				to = x;
				while (to > 0
				       && at(to - 1, y) != cell::outside)
					--to;
			}
			if (at(to, y) == cell::empty)
				x = to;
			break;
		case direction::down:
			if (y + 1 < height && at(x, y + 1) != cell::outside) {
				to = y + 1;
			} else {
				to = y;
				while (to > 0
				       && at(x, to - 1) != cell::outside)
					--to;
			}
			if (at(x, to) == cell::empty)
				y = to;
			break;
		case direction::left:
			if (x > 0 && at(x - 1, y) != cell::outside) {
				to = x - 1;
			} else {
				to = x;
				while (to + 1 < width
				       && at(to + 1, y) != cell::outside)
					++to;
			}
			if (at(to, y) == cell::empty)
				x = to;
			break;
		case direction::up:
			if (y > 0 && at(x, y - 1) != cell::outside) {
				to = y - 1;
			} else {
				to = y;
				while (to + 1 < width
				       && at(x, to + 1) != cell::outside)
					++to;
			}
			if (at(x, to) == cell::empty)
				y = to;
		}
		return {x, y, dir};
	}

	[[nodiscard]] int get_square(const int x, const int y) const noexcept {
		const auto p = [=, this](const std::pair<int, int>& m) {
			const auto [sx, sy] = m;
			return sx * sqr_size <= x && x < (sx + 1) * sqr_size
			       && sy * sqr_size <= y
			       && y < (sy + 1) * sqr_size;
		};
		const auto beg = std::cbegin(squares);
		const auto it = std::find_if(beg, std::cend(squares), p);
		return static_cast<int>(std::distance(beg, it));
	}

	[[nodiscard]] std::tuple<int, int, direction>
	step_cube(int x, int y, direction dir) const noexcept {
		if (dir == direction::right && (x + 1) % sqr_size == 0) {
			const int sqr = get_square(x, y);
			const int delta = y % sqr_size;
			const transition& t = transitions[4 * sqr + static_cast<int>(dir)];
			const auto& [sx, sy] = squares[t.new_sqr];
			switch (t.new_dir) {
			case direction::right:
				x = sqr_size * sx;
				y = sqr_size * sy + delta;
				break;
			case direction::down:
				x = sqr_size * (sx + 1) - 1 - delta;
				y = sqr_size * sy;
				break;
			case direction::left:
				x = sqr_size * (sx + 1) - 1;
				y = sqr_size * (sy + 1) - 1 - delta;
				break;
			case direction::up:
				x = sqr_size * sx + delta;
				y = sqr_size * (sy + 1) - 1;
				break;
			}
			dir = t.new_dir;
		} else if (dir == direction::down && (y + 1) % sqr_size == 0) {
			const int sqr = get_square(x, y);
			const int delta = x % sqr_size;
			const transition& t = transitions[4 * sqr + static_cast<int>(dir)];
			const auto& [sx, sy] = squares[t.new_sqr];
			switch (t.new_dir) {
			case direction::right:
				x = sqr_size * sx;
				y = sqr_size * (sy + 1) - 1 - delta;
				break;
			case direction::down:
				x = sqr_size * sx + delta;
				y = sqr_size * sy;
				break;
			case direction::left:
				x = sqr_size * (sx + 1) - 1;
				y = sqr_size * sy + delta;
				break;
			case direction::up:
				x = sqr_size * (sx + 1) - 1 - delta;
				y = sqr_size * (sy + 1) - 1;
				break;
			}
			dir = t.new_dir;
		} else if (dir == direction::left && x % sqr_size == 0) {
			const int sqr = get_square(x, y);
			const int delta = y % sqr_size;
			const transition& t = transitions[4 * sqr + static_cast<int>(dir)];
			const auto& [sx, sy] = squares[t.new_sqr];
			switch (t.new_dir) {
			case direction::right:
				x = sqr_size * sx;
				y = sqr_size * (sy + 1) - 1 - delta;
				break;
			case direction::down:
				x = sqr_size * sx + delta;
				y = sqr_size * sy;
				break;
			case direction::left:
				x = sqr_size * (sx + 1) - 1;
				y = sqr_size * sy + delta;
				break;
			case direction::up:
				x = sqr_size * (sx + 1) - 1 - delta;
				y = sqr_size * (sy + 1) - 1;
				break;
			}
			dir = t.new_dir;
		} else if (dir == direction::up && y % sqr_size == 0) {
			const int sqr = get_square(x, y);
			const int delta = x % sqr_size;
			const transition& t = transitions[4 * sqr + static_cast<int>(dir)];
			const auto& [sx, sy] = squares[t.new_sqr];
			switch (t.new_dir) {
			case direction::right:
				x = sqr_size * sx;
				y = sqr_size * sy + delta;
				break;
			case direction::down:
				x = sqr_size * (sx + 1) - 1 - delta;
				y = sqr_size * sy;
				break;
			case direction::left:
				x = sqr_size * (sx + 1) - 1;
				y = sqr_size * (sy + 1) - 1 - delta;
				break;
			case direction::up:
				x = sqr_size * sx + delta;
				y = sqr_size * (sy + 1) - 1;
				break;
			}
			dir = t.new_dir;
		} else {
			return step_flat(x, y, dir);
		}
		return {x, y, dir};
	}

	int width{};
	int height{};
	std::unique_ptr<cell[]> data = nullptr;
	int start_x = 0;
	std::vector<int> instructions{};
	std::array<std::pair<int, int>, 6> squares{};
	int sqr_size{};
	transition transitions[24]{};
};

}

template<> output_pair day<22>(std::istream& in)
{
	const grid g{in};
	return {g.solve_flat(), g.solve_cube()};
}
