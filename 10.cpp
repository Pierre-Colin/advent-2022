#include <algorithm>
#include <cstdint>
#include <execution>
#include <istream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

#include "common.h"

namespace {

class cpu {
	friend std::istream& operator>>(std::istream&, cpu&);
public:
	constexpr cpu() noexcept { screen.reserve(246); }
	std::intmax_t strengths() const noexcept { return signal_strengths; }
	std::string&& display() noexcept { return std::move(screen); }

private:
	void inspect_tick();

	std::intmax_t x = 1;
	std::intmax_t signal_strengths = 0;
	std::intmax_t tick = 1;
	std::string screen{};
};

bool instr_eq(std::span<const char, 4> a, std::span<const char, 4> b) noexcept
{
	return std::equal(std::execution::unseq, a.begin(), a.end(),
	                  b.begin());
}

std::istream& operator>>(std::istream& in, cpu& p)
{
	using limits = std::numeric_limits<std::intmax_t>;
	constexpr const char addx[4] = {'a', 'd', 'd', 'x'};
	constexpr const char noop[4] = {'n', 'o', 'o', 'p'};
	while (in.peek() == in.widen('\n'))
		in.get();
	char buf[4];
	if (!in.read(buf, 4))
		return in;
	if (instr_eq(buf, addx)) {
		std::intmax_t a;
		if (!(in >> a))
			return in;
		if ((a > 0 && limits::max() - a < p.x)
		    || (a < 0 && limits::min() - a > p.x)) [[unlikely]]
			throw std::runtime_error("Integer overflow detected");
		p.inspect_tick();
		++p.tick;
		p.inspect_tick();
		++p.tick;
		p.x += a;
	} else if (instr_eq(buf, noop)) {
		p.inspect_tick();
		++p.tick;
	} else [[unlikely]] {
		throw std::runtime_error("Bad instruction");
	}
	if (p.tick >= 250) [[unlikely]]
		throw std::runtime_error("The program is too long");
	return in;
}

void cpu::inspect_tick()
{
	using limits = std::numeric_limits<std::intmax_t>;
	if (tick >= 20 && (tick - 20) % 40 == 0) {
		if ((x > 0 && x >= limits::max() / tick)
		    || (x < 0 && x <= limits::min() / tick)) [[unlikely]]
			throw std::runtime_error("Integer overflow detected");
		const std::intmax_t add = x * tick;
		if ((add > 0 && limits::max() - add < signal_strengths)
		    || (add < 0 && limits::min() - add > signal_strengths))
			throw std::runtime_error("Integer overflow detected");
		signal_strengths += add;
	}
	const std::intmax_t pos = (tick - 1) % 40;
	screen += pos >= x - 1 && pos <= x + 1 ? '#' : '.';
	if (tick % 40 == 0 && tick != 240)
		screen += '\n';
}

}

template<> output_pair day<10>(std::istream& in)
{
	cpu c;
	while (in >> c);
	if (!in.eof())
		throw std::runtime_error("Error while reading puzzle input");
	const std::intmax_t s = c.strengths();
	if (s < 0)
		throw std::runtime_error("Negative signal strength sum");
	return {static_cast<std::uintmax_t>(s), c.display()};
}
