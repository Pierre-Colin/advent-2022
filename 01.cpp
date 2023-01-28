#include <algorithm>
#include <cstdint>
#include <functional>
#include <ios>
#include <iterator>
#include <numeric>
#include <stdexcept>

#include "common.h"

namespace {

class energy_report;

static std::istream& operator>>(std::istream&, energy_report&);

class energy_report {
	friend std::istream& operator>>(std::istream&, energy_report&);
public:
	constexpr energy_report() noexcept = default;

	[[nodiscard]] constexpr std::uintmax_t max() const noexcept {
		return *std::cbegin(top);
	}
	
	[[nodiscard]] constexpr std::uintmax_t top3() const noexcept {
		return std::accumulate(std::cbegin(top), std::cend(top), 0);
	}

private:
	std::uintmax_t top[3]{0, 0, 0};
};

static std::istream& operator>>(std::istream& in, energy_report& report)
{
	std::uintmax_t elf_energy = 0;
	std::uintmax_t item_energy;
	while (in >> item_energy) {
		elf_energy += item_energy;
		if (in.get() != in.widen('\n')) [[unlikely]] {
			in.setstate(std::ios_base::failbit);
			break;
		}
		if (in.peek() == in.widen('\n'))
			break;
	}
	const auto end = std::end(report.top);
	const auto it = std::lower_bound(std::begin(report.top), end,
	                                 elf_energy, std::greater{});
	if (it != end) {
		std::shift_right(it, end, 1);
		*it = elf_energy;
	}
	return in;
}

}

template<> output_pair day<1>(std::istream& in)
{
	energy_report report;
	while (in >> report);
	if (!in.eof()) [[unlikely]]
		throw std::runtime_error("Error while reading puzzle input");
	return {report.max(), report.top3()};
}
