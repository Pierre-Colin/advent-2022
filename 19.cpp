#include <cstdint>
#include <functional>
#include <ios>
#include <istream>
#include <iterator>
#include <memory>
#include <numeric>
#include <string_view>
#include <vector>

#include "common.h"
#include "read.h"

namespace {

struct blueprint {
	unsigned int ore_cost;
	unsigned int clay_cost;
	unsigned int obsidian_cost_ore;
	unsigned int obsidian_cost_clay;
	unsigned int geode_cost_ore;
	unsigned int geode_cost_obsidian;
	unsigned int max_ore;
};

[[nodiscard]]
constexpr unsigned int max_geodes(const blueprint& bp, int t) noexcept;

static bool consume_whitespace_characters(std::istream& in);

class factory {
public:
	explicit factory(std::istream& in) {
		using namespace std::literals;
		using traits = std::istream::traits_type;
		constexpr std::string_view bp1 = "Blueprint"sv;
		constexpr std::string_view bp2 = "Each ore robot costs"sv;
		constexpr std::string_view bp3 = " ore."sv;
		constexpr std::string_view bp4 = "Each clay robot costs"sv;
		constexpr std::string_view bp5 = "Each obsidian robot costs"sv;
		constexpr std::string_view bp6 = " ore and"sv;
		constexpr std::string_view bp7 = " clay."sv;
		constexpr std::string_view bp8 = "Each geode robot costs"sv;
		constexpr std::string_view bp9 = " obsidian."sv;
		while (consume_whitespace_characters(in)) {
			decltype(bp)::size_type id;
			if (!read_expect(in, bp1) || !(in >> id))
				throw std::runtime_error("Puzzle input error");
			if (id != std::size(bp) + 1)
				throw std::runtime_error("Wrong blueprint ID");
			if (in.get() != traits::to_int_type(':')
			    || !consume_whitespace_characters(in)
			    || !read_expect(in, bp2))
				throw std::runtime_error("Puzzle input error");
			blueprint b;
			if (!(in >> b.ore_cost) || !read_expect(in, bp3)
			    || !consume_whitespace_characters(in)
			    || !read_expect(in, bp4) || !(in >> b.clay_cost)
			    || !read_expect(in, bp3)
			    || !consume_whitespace_characters(in)
			    || !read_expect(in, bp5)
			    || !(in >> b.obsidian_cost_ore)
			    || !read_expect(in, bp6)
			    || !(in >> b.obsidian_cost_clay)
			    || !read_expect(in, bp7)
			    || !consume_whitespace_characters(in)
			    || !read_expect(in, bp8)
			    || !(in >> b.geode_cost_ore)
			    || !read_expect(in, bp6)
			    || !(in >> b.geode_cost_obsidian)
			    || !read_expect(in, bp9))
				throw std::runtime_error("Puzzle input error");
			b.max_ore = b.ore_cost;
			if (b.clay_cost > b.max_ore)
				b.max_ore = b.clay_cost;
			if (b.obsidian_cost_ore > b.max_ore)
				b.max_ore = b.obsidian_cost_ore;
			if (b.geode_cost_ore > b.max_ore)
				b.max_ore = b.geode_cost_ore;
			bp.push_back(b);
		}
		if (!in.eof())
			throw std::runtime_error("Puzzle input error");
	}

	[[nodiscard]]
	constexpr std::uintmax_t sum_quality_levels() const noexcept {
		std::uintmax_t acc = 0;
		std::uintmax_t d = 0;
		for (const auto& b : bp)
			acc += ++d * max_geodes(b, 24);
		return acc;
	}

	[[nodiscard]]
	constexpr std::uintmax_t product_geodes() const noexcept {
		const auto beg = std::cbegin(bp);
		const auto end = std::size(bp) >= 3 ? std::next(beg, 3)
		                                    : std::cend(bp);
		return std::transform_reduce(
			beg, end, std::uintmax_t{1}, std::multiplies{},
			[](const blueprint& b) { return max_geodes(b, 32); }
		);
	}

private:
	std::vector<blueprint> bp{};
};

struct state {
	unsigned int ore;
	unsigned int ore_bot;
	bool skipped_ore;
	unsigned int clay;
	unsigned int clay_bot;
	bool skipped_clay;
	unsigned int obsidian;
	unsigned int obsidian_bot;
	bool skipped_obsidian;
	unsigned int geode;
	unsigned int geode_bot;
};

static bool consume_whitespace_characters(std::istream& in)
{
	using traits = std::istream::traits_type;
	std::istream::int_type c;
	while ((c = in.peek()) == traits::to_int_type(' ')
	       || c == traits::to_int_type('\n'))
		in.ignore();
	return in.good();
}

constexpr unsigned int backtrack(const blueprint& bp, const state& s, int t)
	noexcept
{
	if (t == 0)
		return s.geode;
	if (s.ore >= bp.geode_cost_ore
	    && s.obsidian >= bp.geode_cost_obsidian) {
		state n = s;
		n.ore -= bp.geode_cost_ore;
		n.ore += n.ore_bot;
		n.clay += n.clay_bot;
		n.obsidian -= bp.geode_cost_obsidian;
		n.obsidian += n.obsidian_bot;
		n.geode += n.geode_bot;
		++n.geode_bot;
		n.skipped_ore = n.skipped_clay = n.skipped_obsidian = false;
		return backtrack(bp, n, t - 1);
	}
	unsigned int acc = 0;
	const bool can_ore = s.ore >= bp.ore_cost;
	if (!s.skipped_ore && can_ore && s.ore_bot < bp.max_ore) {
		state n = s;
		n.ore -= bp.ore_cost;
		n.ore += n.ore_bot;
		n.clay += n.clay_bot;
		n.obsidian += n.obsidian_bot;
		n.geode += n.geode_bot;
		++n.ore_bot;
		n.skipped_ore = n.skipped_clay = n.skipped_obsidian = false;
		const unsigned int r = backtrack(bp, n, t - 1);
		if (r > acc)
			acc = r;
	}
	const bool can_clay = s.ore >= bp.clay_cost;
	if (!s.skipped_clay && can_clay
	    && s.clay_bot < bp.obsidian_cost_clay) {
		state n = s;
		n.ore -= bp.clay_cost;
		n.ore += n.ore_bot;
		n.clay += n.clay_bot;
		n.obsidian += n.obsidian_bot;
		n.geode += n.geode_bot;
		++n.clay_bot;
		n.skipped_ore = n.skipped_clay = n.skipped_obsidian = false;
		const unsigned int r = backtrack(bp, n, t - 1);
		if (r > acc)
			acc = r;
	}
	const bool can_obsidian = s.ore >= bp.obsidian_cost_ore
	                          && s.clay >= bp.obsidian_cost_clay;
	if (!s.skipped_obsidian && can_obsidian
	    && s.clay_bot < bp.geode_cost_obsidian) {
		state n = s;
		n.ore -= bp.obsidian_cost_ore;
		n.ore += n.ore_bot;
		n.clay -= bp.obsidian_cost_clay;
		n.clay += n.clay_bot;
		n.obsidian += n.obsidian_bot;
		n.geode += n.geode_bot;
		++n.obsidian_bot;
		n.skipped_ore = n.skipped_clay = n.skipped_obsidian = false;
		const unsigned int r = backtrack(bp, n, t - 1);
		if (r > acc)
			acc = r;
	}
	state n = s;
	n.ore += n.ore_bot;
	n.clay += n.clay_bot;
	n.obsidian += n.obsidian_bot;
	n.geode += n.geode_bot;
	n.skipped_ore |= can_ore;
	n.skipped_clay |= can_clay;
	n.skipped_obsidian |= can_obsidian;
	const unsigned int r = backtrack(bp, n, t - 1);
	if (r > acc)
		acc = r;
	return acc;
}

constexpr unsigned int max_geodes(const blueprint& bp, int t) noexcept
{
	const state initial{0, 1, false, 0, 0, false, 0, 0, false, 0, 0};
	return backtrack(bp, initial, t);
}

}

template<> output_pair day<19>(std::istream& in)
{
	const factory f{in};
	return {f.sum_quality_levels(), f.product_geodes()};
}
