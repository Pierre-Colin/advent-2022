#include <cstddef>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "common.h"

static std::size_t parse(std::string arg)
{
	std::size_t r;
	std::istringstream s{std::move(arg)};
	if (!(s >> r))
		throw std::invalid_argument("Day must be a number");
	if (r < 1 || r > std::size(days))
		throw std::invalid_argument("Day not in range");
	return r;
}

int main(int argc, char *argv[])
{
	const std::size_t d = argc > 1 ? parse(argv[1]) : std::size(days);
	const auto [p1, p2] = days[d - 1](std::cin);
	std::cout << p1 << '\n' << p2 << std::endl;
}
