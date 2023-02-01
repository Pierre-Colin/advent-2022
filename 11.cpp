#include <cstddef>
#include <cstdint>
#include <execution>
#include <ios>
#include <istream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "common.h"

enum class operation { add, multiply };

namespace {

struct monkey {
	std::vector<std::uintmax_t> items{};
	operation op = operation::add;
	std::uintmax_t operand = 0;
	std::uintmax_t test = 0;
	std::size_t throw_to[2];
	std::uintmax_t items_inspected = 0;
};

class monkey_circle {
public:
	explicit monkey_circle(std::istream& in);
	void run_round(bool calm_down);

	[[nodiscard]] std::uintmax_t monkey_business() const noexcept {
		std::uintmax_t max[2] = {0, 0};
		for (const monkey& m : monkeys) {
			if (m.items_inspected > max[0]) {
				max[1] = max[0];
				max[0] = m.items_inspected;
			} else if (m.items_inspected > max[1]) {
				max[1] = m.items_inspected;
			}
		}
		return max[0] * max[1];
	}

private:
	std::vector<monkey> monkeys{};
	std::uintmax_t product_tests = 1;
};

static std::istream& read_expect(std::istream& in, std::string_view s)
{
	char buf[29];
	if (sizeof buf < std::size(s)) [[unlikely]]
		throw std::logic_error("read_expect buffer is too small");
	if (!in.read(buf, std::size(s)))
		return in;
	if (!std::equal(std::execution::unseq, s.cbegin(), s.cend(), buf))
		in.setstate(std::ios_base::failbit);
	return in;
}

static std::istream& operator>>(std::istream& in, monkey& m)
{
	using namespace std::literals;
	using limits = std::numeric_limits<std::streamsize>;
	while (in.peek() == in.widen('\n'))
		in.ignore();
	if (!read_expect(in, "Monkey"sv)
	    || !in.ignore(limits::max(), in.widen('\n'))
	    || !read_expect(in, "  Starting items:"sv)) [[unlikely]]
		return in;
	m.items.clear();
	std::uintmax_t item;
	if (in.peek() != in.widen('\n')) {
		if (!(in >> item)) [[unlikely]]
			return in;
		m.items.push_back(item);
		while (in.peek() == in.widen(',')) {
			in.ignore();
			if (!(in >> item))
				return in;
			m.items.push_back(item);
		}
		if (in.get() != in.widen('\n')) {
			in.setstate(std::ios_base::failbit);
			m.items.clear();
			return in;
		}
	}
	m.items.shrink_to_fit();
	if (!read_expect(in, "  Operation: new = old "sv))
		return in;
	const int c = in.get();
	if (c == in.widen('+')) {
		m.op = operation::add;
	} else if (c == in.widen('*')) {
		m.op = operation::multiply;
	} else [[unlikely]] {
		in.setstate(std::ios_base::failbit);
		return in;
	}
	while (in.peek() == in.widen(' '))
		in.ignore();
	if (!(in >> m.operand)) {
		in.clear();
		if (!read_expect(in, "old"sv))
			return in;
		m.operand = std::numeric_limits<std::uintmax_t>::max();
	}
	if (!in.ignore(limits::max(), in.widen('\n'))
	    || !read_expect(in, "  Test: divisible by"sv) >> m.test
	    || !(in >> m.test).ignore(limits::max(), in.widen('\n'))
	    || !read_expect(in, "    If true: throw to monkey"sv)
	    || !(in >> m.throw_to[1]).ignore(limits::max(), in.widen('\n'))
	    || !read_expect(in, "    If false: throw to monkey"sv))
		return in;
	return in >> m.throw_to[0];
}

monkey_circle::monkey_circle(std::istream& in)
{
	monkey m;
	while (in >> m)
		monkeys.emplace_back(std::move(m));
	if (!in.eof()) [[unlikely]]
		throw std::runtime_error("Error while reading puzzle input");
	for (std::size_t i = 0; i < monkeys.size(); ++i) {
		const auto p = [this, i](std::size_t to) {
			return to == i || to >= std::size(monkeys);
		};
		if (std::any_of(std::execution::unseq,
		                std::cbegin(monkeys[i].throw_to),
		                std::cend(monkeys[i].throw_to), p))
			throw std::runtime_error("Ill-defined  monkey throw");
		for (std::size_t j = i + 1; j < monkeys.size(); ++j) {
			if (std::gcd(monkeys[i].test, monkeys[j].test) > 1)
				throw std::runtime_error("Tests not coprime");
		}
		product_tests *= monkeys[i].test;
	}
}

void monkey_circle::run_round(const bool calm_down)
{
	constexpr auto old = std::numeric_limits<std::uintmax_t>::max();
	for (monkey& m : monkeys) {
		for (std::uintmax_t item : m.items) {
			std::uintmax_t worry = item;
			auto operand = m.operand == old ? worry : m.operand;
			if (m.op == operation::add)
				worry += operand;
			else
				worry *= operand;
			if (calm_down)
				worry /= 3;
			worry %= product_tests;
			const std::size_t to = m.throw_to[worry % m.test == 0];
			monkeys[to].items.push_back(worry);
		}
		m.items_inspected += m.items.size();
		m.items.clear();
	}
}

}

template<> output_pair day<11>(std::istream& in)
{
	monkey_circle circle_1{in};
	monkey_circle circle_2 = circle_1;
	for (int r = 0; r < 20; ++r)
		circle_1.run_round(true);
	for (int r = 0; r < 10000; ++r)
		circle_2.run_round(false);
	return {circle_1.monkey_business(), circle_2.monkey_business()};
}
