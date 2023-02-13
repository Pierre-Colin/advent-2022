#include <algorithm>
#include <array>
#include <cstdint>
#include <deque>
#include <istream>
#include <stdexcept>
#include <vector>

#include "common.h"

enum class direction {left, right};

static constexpr std::array<std::uint_least16_t, 5> pieces{
	0x000f, 0x04e4, 0x022e, 0x8888, 0x00cc
};

namespace {

class boulder_cave {
public:
	explicit boulder_cave(std::istream& in);
	boulder_cave(const boulder_cave&) = delete;
	boulder_cave& operator=(const boulder_cave&) = delete;

	void drop_boulders(std::uint_fast64_t n) {
		while (n > 0) {
			if (cycle_len > 0 && n >= cycle_len && can_short()) {
				const auto q = n / cycle_len;
				const auto r = n % cycle_len;
				omitted_height += q * cycle_height;
				n = r;
				num_dropped += q * cycle_len;
			} else {
				drop_boulder();
				--n;
				++num_dropped;
				mark_cycle_if_possible();
			}
		}
	}

	[[nodiscard]] constexpr std::uintmax_t height() const noexcept {
		return omitted_height + partial_height();
	}

private:
	using wind_container_type = std::deque<direction>;

	struct memory {
		wind_container_type::const_iterator wind_it;
		std::uint_fast64_t num_dropped;
		std::uintmax_t partial_height;
	};

	void drop_boulder();

	constexpr void mark_cycle_if_possible() {
		if (has_cycle_marked() || piece_type != pieces.cbegin()
		    || !stabilizes_bar())
			return;
		remember_state();
		try_mark_cycle();
	}

	[[nodiscard]] constexpr bool has_cycle_marked() const noexcept {
		return cycle_len != 0;
	}

	[[nodiscard]] constexpr bool can_short() const {
		return has_cycle_marked() && piece_type == pieces.cbegin()
		       && wind_it == remembered_states.back().wind_it
		       && stabilizes_bar();
	}

	void remember_state() {
		remembered_states.push_back({wind_it, num_dropped,
		                             partial_height()});
	}

	[[nodiscard]] constexpr bool stabilizes_bar() const {
		if (obstacles.empty())
			return true;
		const auto beg = obstacles.crbegin();
		for (auto row = beg; row != beg + 28; row += 7) {
			const auto it = std::find(row, row + 4, true);
			if (it == row + 3)
				return true;
			const bool r = it != row + 4;
			bool l = std::find(row + 4, row + 7, true) != row + 7;
			if (l || r)
				return l && r;
		}
		throw std::logic_error("Too much space above pile of rocks");
	}

	constexpr std::uintmax_t partial_height() const noexcept {
		if (obstacles.empty())
			return 0;
		const auto beg = obstacles.crbegin();
		auto it = std::find(beg, beg + 28, true);
		if (it == beg + 28)
			return 0;
		return obstacles.size() / 7 - (it - beg) / 7;
	}

	[[nodiscard]] constexpr bool
	would_collide(std::uint_least16_t type, int x, std::uintmax_t y)
		const noexcept
	{
		if (x < 0 || y < 3)
			return true;
		for (std::vector<bool>::size_type i = 0; i < 4; ++i) {
			bool is_high = 7 * (y - 3 + i) >= obstacles.size();
			for (std::vector<bool>::size_type j = 0; j < 4; ++j) {
				if (!(type & (1u << (4 * i + j))))
					continue;
				if (x + 3 - j >= 7)
					return true;
				if (!is_high
				    && obstacles[7 * (y - 3 + i) + x + 3 - j])
					return true;
			}
		}
		return false;
	}

	constexpr void try_mark_cycle() noexcept {
		if (remembered_states.empty())
			return;
		const memory& last_state = remembered_states.back();
		const auto p = [&last_state](const memory& m) {
			return m.wind_it == last_state.wind_it;
		};
		const auto it = std::find_if(remembered_states.crbegin() + 1,
		                             remembered_states.crend(), p);
		if (it == remembered_states.crend())
			return;
		cycle_len = last_state.num_dropped - it->num_dropped;
		cycle_height = last_state.partial_height - it->partial_height;
	}

	// TODO: use a std::variant to join cycle data and remembered_states
	wind_container_type wind{};
	wind_container_type::const_iterator wind_it = wind.cbegin();
	decltype(pieces)::const_iterator piece_type = pieces.cbegin();
	std::vector<bool> obstacles{};
	std::uint_fast64_t num_dropped = 0;
	std::uint_fast64_t cycle_len = 0;
	std::uintmax_t cycle_height{};
	std::uintmax_t omitted_height = 0;
	std::vector<memory> remembered_states{};
};

boulder_cave::boulder_cave(std::istream& in)
{
	using traits_type = std::istream::traits_type;
	std::istream::int_type x;
	while ((x = in.get()) != traits_type::eof()) {
		const std::istream::char_type c = traits_type::to_char_type(x);
		switch (c) {
		case '<':
			wind.push_back(direction::left);
			break;
		case '>':
			wind.push_back(direction::right);
			[[fallthrough]];
		case '\n':
			break;
		[[unlikely]] default:
			throw std::runtime_error("Unexpected character");
		}
	}
	if (!in.eof()) [[unlikely]]
		throw std::runtime_error("Internal error while reading input");
	wind_it = wind.cbegin();
}

template<class C> constexpr void
advance_cyclical(typename C::const_iterator& it, const C& c) noexcept
{
	if (++it == c.cend())
		it = c.cbegin();
}

void boulder_cave::drop_boulder()
{
	const std::uint_least16_t type = *piece_type;
	int x = 2;
	std::uintmax_t y = partial_height() + 6;
	for (;;) {
		switch (*wind_it) {
		case direction::left:
			if (!would_collide(type, x - 1, y))
				--x;
			break;
		case direction::right:
			if (!would_collide(type, x + 1, y))
				++x;
		}
		advance_cyclical(wind_it, wind);
		if (would_collide(type, x, y - 1))
			break;
		--y;
	}
	if (obstacles.size() <= 7 * (y + 1))
		obstacles.resize(7 * (y + 1), false);
	for (std::vector<bool>::size_type i = 0; i < 4; ++i) {
		for (std::vector<bool>::size_type j = 0; j < 4; ++j) {
			if (x + 3 - j >= 7)
				continue;
			if (type & std::uint_least16_t(1) << (4 * i + j))
				obstacles[7 * (y - 3 + i) + x + 3 - j] = true;
		}
	}
	advance_cyclical(piece_type, pieces);
}

}

template<> output_pair day<17>(std::istream& in)
{
	boulder_cave cave{in};
	cave.drop_boulders(2022);
	const std::uintmax_t part1 = cave.height();
	cave.drop_boulders(1000000000000 - 2022);
	return {part1, cave.height()};
}
