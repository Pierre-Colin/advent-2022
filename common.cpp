#include <ostream>

#include "common.h"

std::ostream& operator<<(std::ostream& out, const puzzle_output& x)
{
	return x.is_string ? out << x.u.s : out << x.u.i;
}
