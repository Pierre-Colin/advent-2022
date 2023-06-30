#include <algorithm>
#include <cstdint>
#include <deque>
#include <istream>
#include <iterator>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "checked.h"
#include "common.h"

namespace {

using container_type = std::vector<std::ptrdiff_t>;

struct mixing_data {
	container_type::value_type val;
	container_type::size_type ord;
};

static_assert(std::is_trivially_copyable_v<mixing_data>,
              "mixing_data is not trivially copyable");

using mixer_type = std::vector<mixing_data>;

constexpr container_type::value_type key = 811589153;
constexpr container_type::value_type min_val =
	std::numeric_limits<container_type::value_type>::min() / 811589153;
constexpr container_type::value_type max_val =
	std::numeric_limits<container_type::value_type>::max() / 811589153;

static container_type parse(std::istream& in)
{
	container_type c;
	container_type::value_type n;
	container_type::size_type zeros = 0;
	while (in >> n) {
		if (n == 0)
			++zeros;
		if (n < min_val || n > max_val)
			throw std::runtime_error("Value out of range");
		c.emplace_back(std::move(n));
	}
	if (!in.eof())
		throw std::runtime_error("Error while reading puzzle input");
	if (zeros != 1)
		throw std::runtime_error("Wrong number of zeros in input");
	if (std::size(c) == std::numeric_limits<decltype(c)::size_type>::max())
		throw std::runtime_error("The puzzle input was too big");
	return c;
}

static mixer_type make_mixer(const container_type& c)
{
	mixer_type mixer;
	mixer.reserve(std::size(c) + 1);
	mixer_type::size_type order = 0;
	for (const auto n : c)
		mixer.emplace_back(n, order++);
	return mixer;
}

template<class Iterator>
	requires std::is_base_of_v<
		std::bidirectional_iterator_tag,
		typename std::iterator_traits<Iterator>::iterator_category
	>
constexpr bool
cycle(const Iterator beg, const Iterator end, Iterator& it,
      std::iter_difference_t<Iterator> n, std::iter_difference_t<Iterator> sz)
{
	using traits = std::iterator_traits<Iterator>;
	constexpr bool rai = std::is_base_of_v<std::random_access_iterator_tag,
		                               typename traits::iterator_category>;
	if (n > sz)
		n %= sz;
	else if (n < -sz)
		n = -(-(n + sz) % sz);
	const std::strong_ordering cmp = n <=> 0;
	bool cycled = false;
	if (std::is_lt(cmp)) {
		if constexpr (rai) {
			const auto dl = std::distance(beg, it);
			if (n < -dl) {
				it = std::prev(end);
				n += 1 + dl;
				cycled = true;
			}
			std::advance(it, n);
		} else {
			while (n < 1 && it != beg) {
				--it;
				++n;
			}
			if (n < 0 && it == beg) {
				it = std::prev(end);
				++n;
				cycled = true;
			}
			while (n < 0) {
				--it;
				++n;
			}
		}
	} else if (std::is_gt(cmp)) {
		if constexpr (rai) {
			const auto dr = std::distance(it, end);
			if (n >= dr) {
				it = beg;
				n -= dr;
				cycled = true;
			}
			std::advance(it, n);
		} else {
			while (n > 0 && it != end) {
				++it;
				--n;
			}
			if (it == end) {
				it = beg;
				cycled = true;
			}
			while (n > 0) {
				++it;
				--n;
			}
		}
	}
	return cycled;
}

static void move_mixed_data(mixer_type& mixer, mixer_type::size_type order)
{
	using limits = std::numeric_limits<std::ptrdiff_t>;
	const auto p = [&order](const mixing_data& m) {
		return m.ord == order;
	};
	auto it = std::find_if(std::begin(mixer), std::end(mixer), p);
	if (it == std::end(mixer)) [[unlikely]]
		throw std::logic_error("Mixing element not found");
	const auto old_p = std::distance(std::begin(mixer), it);
	if (it->val > 0 && old_p > limits::max() - it->val) [[unlikely]]
		throw std::runtime_error("Integer overflow detected");
	mixing_data old = *it;
	it = mixer.erase(it);
	if (it == std::end(mixer))
		it = std::begin(mixer);
	cycle(std::begin(mixer), std::end(mixer), it, old.val,
	      static_cast<mixer_type::difference_type>(std::size(mixer)));
	mixer.emplace(it, std::move(old));
}

static container_type translate_mixer(const mixer_type& mixer)
{
	container_type c;
	c.reserve(std::size(mixer));
	for (const auto& m : mixer)
		c.push_back(m.val);
	return c;
}

[[nodiscard]] static container_type mix(const container_type& c, int rounds)
{
	mixer_type mixer = make_mixer(c);
	while (rounds-- > 0) {
		for (mixer_type::size_type o = 0; o < std::size(mixer); ++o)
			move_mixed_data(mixer, o);
	}
	return translate_mixer(mixer);
}

}

[[nodiscard]]
static std::intmax_t find_grove_coordinates(const container_type& c)
{
	auto it = std::find(std::cbegin(c), std::cend(c), 0);
	cycle(std::cbegin(c), std::cend(c), it, 1000, std::size(c));
	std::intmax_t acc = *it;
	cycle(std::cbegin(c), std::cend(c), it, 1000, std::size(c));
	if (checked_add(acc, acc, *it)) [[unlikely]]
		throw std::runtime_error("Integer overflow detected");
	cycle(std::cbegin(c), std::cend(c), it, 1000, std::size(c));
	if (checked_add(acc, acc, *it)) [[unlikely]]
		throw std::runtime_error("Integer overflow detected");
	return acc;
}

[[nodiscard]] static std::intmax_t first_try(container_type c)
{
	return find_grove_coordinates(mix(c, 1));
}

template<> output_pair day<20>(std::istream& in)
{
	container_type c = parse(in);
	const std::intmax_t part1 = first_try(c);
	if (part1 < 0)
		throw std::runtime_error("Grove coordinate is negative");
	for (auto& x : c)
		x *= key;
	const std::intmax_t part2 = find_grove_coordinates(mix(c, 10));
	if (part2 < 0)
		throw std::runtime_error("Grove coordinate is negative");
	return {static_cast<std::uintmax_t>(part1),
	        static_cast<std::uintmax_t>(part2)};
}
