#if __cplusplus >= 201703L
#include <istream>
#include <string_view>

std::istream& read_expect(std::istream& in, std::string_view s);
#else
#error This header is for C++17 or above.
#endif
