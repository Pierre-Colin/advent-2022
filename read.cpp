#include <algorithm>
#include <ios>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <string_view>

#include "read.h"

std::istream& read_expect(std::istream& in, std::string_view s)
{
	char buf[29];
	if (sizeof buf < s.size()) [[unlikely]] {
		std::ostringstream r;
		r << "Read buffer too small: needed " << s.size() << ", have "
		  << sizeof buf;
		throw std::invalid_argument(r.str());
	}
	if (!in.read(buf, s.size()) || !std::equal(s.begin(), s.end(), buf))
		in.setstate(std::ios_base::failbit);
	return in;
}
