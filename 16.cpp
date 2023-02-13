#include <algorithm>
#include <cstdint>
#include <ios>
#include <istream>
#include <limits>
#include <map>
#include <utility>
#include <vector>

#include "common.h"
#include "read.h"

namespace {

struct vertex_data {
	std::uintmax_t rate{};
	std::vector<unsigned> neighbors{};
};

struct vertex_input_data {
	unsigned name{};
	vertex_data data{};
};

constexpr int letter_id(const char c)
{
	constexpr char runes[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	const auto it = std::find(std::begin(runes), std::end(runes) - 1, c);
	if (it == std::end(runes) - 1) [[unlikely]]
		throw std::invalid_argument("Expected a capital letter");
	return static_cast<int>(it - std::begin(runes));
}

std::istream& operator>>(std::istream& in, vertex_input_data &x)
{
	using namespace std::literals;
	using traits = std::istream::traits_type;
	while (in.peek() == in.widen('\n'))
		in.ignore();
	if (!read_expect(in, "Valve "sv)) [[unlikely]]
		return in;
	int c = in.get();
	if (c == traits::eof()) [[unlikely]]
		return in;
	x.name = static_cast<int>(letter_id(traits::to_char_type(c)));
	if ((c = in.get()) == traits::eof()) [[unlikely]]
		return in;
	x.name = 26 * x.name
	         + static_cast<int>(letter_id(traits::to_char_type(c)));
	if (!read_expect(in, " has flow rate="sv) || !(in >> x.data.rate)
	    || !read_expect(in, "; tunnel"sv)) [[unlikely]]
		return in;
	if (in.peek() == in.widen('s'))
		in.ignore();
	if (!read_expect(in, " lead"sv)) [[unlikely]]
		return in;
	if (in.peek() == in.widen('s'))
		in.ignore();
	if (!read_expect(in, " to valve"sv)) [[unlikely]]
		return in;
	if (in.peek() == in.widen('s'))
		in.ignore();
	x.data.neighbors.clear();
	while (in.peek() == in.widen(' ')) {
		in.ignore();
		c = in.get();
		if (c == traits::eof()) [[unlikely]]
			return in;
		unsigned n = static_cast<int>(
			letter_id(traits::to_char_type(c))
		);
		c = in.get();
		if (c == traits::eof()) [[unlikely]]
			return in;
		n = 26 * n
		    + static_cast<int>(letter_id(traits::to_char_type(c)));
		x.data.neighbors.push_back(n);
		if (in.peek() == in.widen(','))
			in.ignore();
	}
	c = in.peek();
	if (c != traits::eof() && c != in.widen('\n'))
		in.setstate(std::ios_base::failbit);
	return in;
}

std::istream& operator>>(std::istream& in, std::vector<vertex_data>& v)
{
	std::vector<vertex_input_data> vidv;
	std::vector<unsigned> names;
	vertex_input_data vid;
	while (in >> vid) {
		names.push_back(vid.name);
		vidv.emplace_back(std::move(vid));
	}
	if (names.empty()) [[unlikely]] {
		in.setstate(std::ios_base::failbit);
		return in;
	}
	std::sort(names.begin(), names.end());
	if (names.front() != 0) [[unlikely]] {
		in.setstate(std::ios_base::failbit);
		return in;
	}
	v.clear();
	v.resize(vidv.size());
	const auto id = [&names](unsigned n) noexcept {
		return std::lower_bound(names.cbegin(), names.cend(), n)
		       - names.cbegin();
	};
	for (vertex_input_data& x : vidv) {
		vertex_data& y = v.at(id(x.name));
		y.rate = x.data.rate;
		for (unsigned nj : x.data.neighbors)
			y.neighbors.emplace_back(id(nj));
	}
	return in;
}

class solver_state {
public:
	explicit solver_state(const std::vector<vertex_data>& v);

	[[nodiscard]] std::uintmax_t solve(unsigned t, unsigned a) {
		if (t > 30) [[unlikely]]
			throw std::invalid_argument("Time limit too high");
		if (a == 0 || a > 2) [[unlikely]]
			throw std::invalid_argument("Wrong number of agents");
		return solve(0, 0, t, a);
	}

private:
	[[nodiscard]] std::uintmax_t
	solve(unsigned valve, std::uintmax_t opened, unsigned t, unsigned a);

	std::vector<std::uintmax_t> rates{};
	std::vector<unsigned> distances{};
	std::vector<std::uintmax_t> memoizer{};
};

solver_state::solver_state(const std::vector<vertex_data>& v)
{
	using limits = std::numeric_limits<unsigned>;
	std::vector<unsigned> temp_dist(v.size() * v.size(), limits::max());
	for (unsigned i = 0; i < v.size(); ++i) {
		for (unsigned j : v[i].neighbors)
			temp_dist[i * v.size() + j] = 1;
		temp_dist[i * v.size() + i] = 0;
	}
	// Floyd-Warshall algorithm
	for (unsigned k = 0; k < v.size(); ++k) {
		for (unsigned i = 0; i < v.size(); ++i) {
			for (unsigned j = 0; j < v.size(); ++j) {
				const unsigned a = temp_dist[i * v.size() + k];
				const unsigned b = temp_dist[k * v.size() + j];
				if (a == limits::max() || b == limits::max())
					continue;
				const unsigned t = a + b;
				unsigned& r = temp_dist[i * v.size() + j];
				if (t < r)
					r = t;
			}
		}
	}
	rates.push_back(v.front().rate);
	for (unsigned i = 1; i < v.size(); ++i) {
		if (v[i].rate > 0)
			rates.push_back(v[i].rate);
	}
	distances.reserve(rates.size() * rates.size());
	for (unsigned i = 0; i < v.size(); ++i) {
		if (i != 0 && v[i].rate == 0)
			continue;
		for (unsigned j = 0; j < v.size(); ++j) {
			if (j != 0 && v[j].rate == 0)
				continue;
			distances.push_back(temp_dist[i * v.size() + j]);
		}
	}
	temp_dist.clear();
	if (rates.size() >= limits::digits) [[unlikely]]
		throw std::invalid_argument("Too many nonzero valves");
	memoizer.resize((1u << rates.size()) * rates.size() * 31 * 2, 0);
}

std::uintmax_t
solver_state::solve(unsigned valve, std::uintmax_t opened, unsigned t,
                    unsigned a)
{
	std::uintmax_t& memo = memoizer.at(
		((opened * rates.size() + valve) * 31 + t) * 2 + a);
	if (memo != 0)
		return memo;
	if (t == 0)
		return a == 2 ? solve(0, opened, 26, 1) : 0;
	std::uintmax_t acc = 0;
	const std::uintmax_t mask = std::uintmax_t{1} << valve;
	if (valve != 0 && (opened & mask) == 0) {
		acc = (t - 1) * rates[valve]
		      + solve(valve, opened | mask, t - 1, a);
	}
	for (unsigned n = 0; n < rates.size(); ++n) {
		if (n == valve)
			continue;
		const unsigned d = distances[valve * rates.size() + n];
		if (d > t || n == valve)
			continue;
		const std::uintmax_t rec = solve(n, opened, t - d, a);
		if (rec > acc)
			acc = rec;
	}
	memo = acc;
	return acc;
}

}

template<> output_pair day<16>(std::istream& in)
{
	std::vector<vertex_data> v;
	in >> v;
	if (!in.eof())
		throw std::runtime_error("Error while reading puzzle input");
	solver_state solver{std::move(v)};
	return {solver.solve(30, 1), solver.solve(26, 2)};
}
