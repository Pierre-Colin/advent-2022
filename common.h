#if defined __cplusplus && __cplusplus >= 202002L
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>

class puzzle_output {
	friend std::ostream& operator<<(std::ostream&, const puzzle_output&);
public:
	puzzle_output() = delete;
	puzzle_output(const puzzle_output&) = delete;

	constexpr ~puzzle_output() noexcept {
		using namespace std;
		if (is_string)
			u.s.~string();
	}

	constexpr puzzle_output(puzzle_output&& rhs) noexcept
		: is_string{rhs.is_string}
		, u{}
	{
		using namespace std;
		if (is_string)
			new (&u.s) string(static_cast<string&&>(rhs.u.s));
		else
			u.i = rhs.u.i; // starts the lifetime of u.i correctly
	}

	constexpr puzzle_output(const std::uintmax_t n) noexcept
		: is_string{false}
		, u{n}
	{}

	constexpr puzzle_output(std::string&& s) noexcept
		: is_string{true}
		, u{static_cast<std::string&&>(s)}
	{}

	puzzle_output& operator=(const puzzle_output&) = delete;

private:
	union union_type {
		constexpr union_type() noexcept : i{} {}
		constexpr union_type(const std::uintmax_t n) noexcept : i{n} {}

		constexpr union_type(std::string&& x) noexcept
			: s{static_cast<std::string&&>(x)}
		{}
	
		constexpr ~union_type() noexcept {}

		uintmax_t i;
		std::string s;
	};

	bool is_string;
	union_type u;
};

struct output_pair {
	puzzle_output first;
	puzzle_output second;
};

template<int D> output_pair day(std::istream& in);
#else
#error This header is for C++20 or later
#endif
