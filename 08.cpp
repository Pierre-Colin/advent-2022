#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <execution>
#include <istream>
#include <memory>
#include <string>
#include <vector>

#include "common.h"

static bool is_digits(const std::string& s) noexcept
{
	return std::all_of(std::execution::unseq, s.cbegin(), s.cend(),
	                   [](const char c){ return '0' <= c && c <= '9'; });
}

class forest {
public:
	explicit forest(std::istream& in) {
		using limits = std::numeric_limits<std::ptrdiff_t>;
		std::string line;
		if (!std::getline(in, line) || !is_digits(line)) [[unlikely]]
			throw std::runtime_error("Puzzle input error");
		std::vector<char> dat(line.cbegin(), line.cend());
		width = line.size();
		height = 1;
		while (std::getline(in, line) && is_digits(line)) {
			if (line.empty())
				continue;
			if (line.size() != width)
				throw std::runtime_error("Inconsistent width");
			++height;
			dat.insert(dat.cend(), line.cbegin(), line.cend());
		}
		line.clear();
		if (!in.eof()) [[unlikely]]
			throw std::runtime_error("Puzzle input error");
		if (width > static_cast<std::size_t>(limits::max()) / height)
			throw std::runtime_error("Forest is too big");
		data = std::make_unique<char[]>(width * height);
		std::copy(std::execution::unseq, dat.cbegin(), dat.cend(),
		          data.get());
	}

	[[nodiscard]] std::size_t count_visible_trees() const {
		const std::size_t area = width * height;
		std::unique_ptr<bool[]> visible =
			std::make_unique<bool[]>(area);
		std::fill(std::execution::unseq, visible.get(),
		          visible.get() + width, true);
		for (std::size_t r = 1; r + 1 < height; ++r) {
			visible[width * r] = true;
			std::fill(std::execution::unseq,
			          visible.get() + width * r + 1,
			          visible.get() + width * (r + 1) - 1, false);
			visible[width * r + width - 1] = true;
		}
		std::fill(std::execution::unseq,
		          visible.get() + (height - 1) * width,
		          visible.get() + area, true);
		for (std::size_t r = 1; r + 1 < height; ++r) {
			char m = data[r * width];
			for (std::size_t c = 1; c < width; ++c) {
				if (data[r * width + c] > m) {
					visible[r * width + c] = true;
					m = data[r * width + c];
				}
			}
			m = data[r * width + width - 1];
			for (std::size_t c = width - 1; c > 0; --c) {
				if (data[r * width + c - 1] > m) {
					visible[r * width + c - 1] = true;
					m = data[r * width + c - 1];
				}
			}
		}
		for (std::size_t c = 1; c + 1 < width; ++c) {
			char m = data[c];
			for (std::size_t r = 1; r < height; ++r) {
				if (data[r * width + c] > m) {
					visible[r * width + c] = true;
					m = data[r * width + c];
				}
			}
			m = data[(height - 1) * width + c];
			for (std::size_t r = height - 1; r > 0; --r) {
				if (data[(r - 1) * width + c] > m) {
					visible[(r - 1) * width + c] = true;
					m = data[(r - 1) * width + c];
				}
			}
		}
		return std::count(std::execution::unseq, visible.get(),
				  visible.get() + width * height, true);
	}

	[[nodiscard]] std::uintmax_t max_scenic_score() const noexcept {
		std::uintmax_t acc = 0;
		for (std::size_t r = 1; r + 1 < height; ++r) {
			for (std::size_t c = 1; c + 1 < width; ++c) {
				const std::uintmax_t s = scenic_score(r, c);
				if (s > acc)
					acc = s;
			}
		}
		return acc;
	}

private:
	[[nodiscard]] std::uintmax_t
	scenic_score(const std::size_t r, const std::size_t c) const noexcept {
		const char t = data[r * width + c];
		std::uintmax_t d_left = 1;
		for (std::size_t x = c - 1;
		     x > 0 && data[r * width + x] < t; --x)
			++d_left;
		std::uintmax_t d_right = 1;
		for (std::size_t x = c + 1;
		     x + 1 < width && data[r * width + x] < t; ++x)
			++d_right;
		std::uintmax_t d_up = 1;
		for (std::size_t y = r - 1; y > 0 && data[y * width + c] < t;
		     --y)
			++d_up;
		std::uintmax_t d_down = 1;
		for (std::size_t y = r + 1;
		     y + 1 < height && data[y * width + c] < t; ++y)
			++d_down;
		return d_up * d_down * d_left * d_right;
	}

	std::unique_ptr<char[]> data = nullptr;
	std::size_t width = 0;
	std::size_t height = 0;
};

template<> output_pair day<8>(std::istream& in)
{
	const forest f{in};
	return {f.count_visible_trees(), f.max_scenic_score()};
}
