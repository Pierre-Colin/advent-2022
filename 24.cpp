#include <algorithm>
#include <cstddef>
#include <execution>
#include <istream>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

#include "common.h"

class valley {
public:
	explicit valley(std::istream& in) {
		std::string line;
		while (std::getline(in, line)) {
			if (!line.empty())
				lines.emplace_back(std::move(line));
		}
		if (lines.empty())
			throw std::runtime_error("Empty");
		for (auto it = lines.cbegin() + 1; it != lines.cend(); ++it) {
			if (it->size() != lines.front().size())
				throw std::runtime_error("Inconsistent width");
		}
	}
	
private:
	std::vector<std::string> lines{};
};

class state {
public:
	state(const std::vector<std::string>& lines)
		: blizzards(lines.size())
		, xo{1}
		, yo{0}
	{
		blizzards.front().resize(lines.front().size(), 0);
		for (std::size_t i = 1; i + 1 < lines.size(); ++i) {
			blizzards[i].resize(lines.front().size(), 0);
			for (std::size_t j = 1; j + 1 < lines.front().size(); ++j) {
				switch (lines[i][j]) {
				case '^':
					blizzards[i][j] = 1;
					break;
				case 'v':
					blizzards[i][j] = 2;
					break;
				case '<':
					blizzards[i][j] = 4;
					break;
				case '>':
					blizzards[i][j] = 8;
					break;
				}
			}
		}
		blizzards.back().resize(lines.front().size(), 0);
	}

	[[nodiscard]] std::uintmax_t distance_exit() {
		std::set<std::pair<std::size_t, std::size_t>> frontier{
			std::pair{xo, yo}
		};
		std::uintmax_t step = 1;
		while (!frontier.empty()) {
			advance_blizzards();
			std::set<std::pair<std::size_t, std::size_t>> n;
			for (auto [x, y] : frontier) {
				if (x + 2 == blizzards.front().size() && y + 2 == blizzards.size())
					return step;
				if (blizzards[y][x] == 0)
					n.emplace(x, y);
				if (x > 1 && blizzards[y][x - 1] == 0)
					n.emplace(x - 1, y);
				if (y > 0 && x + 2 < blizzards.front().size() && blizzards[y][x + 1] == 0)
					n.emplace(x + 1, y);
				if ((y == 1 && x == 1) || (y > 1 && blizzards[y - 1][x] == 0))
					n.emplace(x, y - 1);
				if (y + 2 < blizzards.size() && blizzards[y + 1][x] == 0)
					n.emplace(x, y + 1);
			}
			frontier = std::move(n);
			++step;
		}
		throw std::runtime_error("No solution found");
	}

	[[nodiscard]] std::uintmax_t distance_three() {
		std::set<std::tuple<std::size_t, std::size_t, int>> frontier{
			std::tuple(xo, yo, 0)
		};
		std::uintmax_t step = 1;
		while (!frontier.empty()) {
			advance_blizzards();
			std::set<std::tuple<std::size_t, std::size_t, int>> n;
			for (auto [x, y, s] : frontier) {
				if (s == 2 && x + 2 == blizzards.front().size()
				    && y + 2 == blizzards.size())
					return step;
				if (blizzards[y][x] == 0)
					n.emplace(x, y, s);
				if (y + 1 != blizzards.size() && x > 1 && blizzards[y][x - 1] == 0)
					n.emplace(x - 1, y, s);
				if (y > 0 && x + 2 < blizzards.front().size() && blizzards[y][x + 1] == 0)
					n.emplace(x + 1, y, s);
				if (x == 1 && y == 1) {
					if (s == 1) {
						n.clear();
						n.emplace(x, y - 1, 2);
						break;
					}
					n.emplace(x, y - 1, s);
				} else if (y > 1 && blizzards[y - 1][x] == 0) {
					n.emplace(x, y - 1, s);
				}
				if (x + 2 == blizzards.front().size() && y + 2 == blizzards.size()) {
					if (s != 1) {
						n.clear();
						n.emplace(x, y + 1, s + 1);
						break;
					}
					n.emplace(x, y + 1, s);
				} else if (y + 2 < blizzards.size() && blizzards[y + 1][x] == 0) {
					n.emplace(x, y + 1, s);
				}
			}
			frontier = std::move(n);
			++step;
		}
		throw std::runtime_error("No solution found");
	}

private:
	void advance_blizzards() {
		std::vector<std::vector<unsigned char>> n(blizzards.size());
		n.front().resize(blizzards.front().size(), 0);
		for (std::size_t i = 1; i + 1 < n.size(); ++i) {
			const std::size_t up = i == 1 ? n.size() - 2 : i - 1;
			const std::size_t down = i + 2 == n.size() ? 1 : i + 1;
			n[i].resize(blizzards.front().size(), 0);
			for (std::size_t j = 1; j + 1 < n[i].size(); ++j) {
				const std::size_t left = j == 1 ? n.front().size() - 2 : j - 1;
				const std::size_t right = j + 2 == n.front().size() ? 1 : j + 1;
				if (blizzards[down][j] & 1)
					n[i][j] |= 1;
				if (blizzards[up][j] & 2)
					n[i][j] |= 2;
				if (blizzards[i][right] & 4)
					n[i][j] |= 4;
				if (blizzards[i][left] & 8)
					n[i][j] |= 8;
			}
		}
		n.back().resize(blizzards.front().size(), 0);
		blizzards = std::move(n);
	}

	std::vector<std::vector<unsigned char>> blizzards;
	std::size_t xo;
	std::size_t yo;
};

static std::vector<std::string> get_lines(std::istream& in)
{
	std::vector<std::string> lines;
	std::string line;
	while (std::getline(in, line)) {
		if (!line.empty())
			lines.emplace_back(std::move(line));
	}
	return lines;
}

template<> output_pair day<24>(std::istream& in)
{
	const std::vector<std::string> lines = get_lines(in);
	return {state(lines).distance_exit(), state(lines).distance_three()};
}
