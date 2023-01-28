#include <cstdint>
#include <ios>
#include <istream>

#include "common.h"
#include "interval.h"

using interval_type = interval<std::uintmax_t>;

static std::istream& operator>>(std::istream& i, interval_type& x)
{
	std::uintmax_t a, b;
	if (i >> a && i.get() == '-' && i >> b)
		x = interval{a, b};
	else
		i.setstate(std::ios_base::failbit);
	return i;
}

template<> output_pair day<4>(std::istream& in)
{
	interval_type a;
	interval_type b;
	std::uintmax_t count_contains = 0;
	std::uintmax_t count_overlap = 0;
	while (in >> a && in.get() == ',' && in >> b) {
		switch (interval_type::compare(a, b)) {
		default: break;
		case interval_type::relation::contained:
			++count_contains;
			[[fallthrough]];
		case interval_type::relation::overlap:
			++count_overlap;
		}
	}
	if (!in.eof())
		throw std::runtime_error("Error while reading puzzle input");
	return {count_contains, count_overlap};
}
