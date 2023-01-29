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
template<> output_pair day<1>(std::istream& in);
template<> output_pair day<2>(std::istream& in);
template<> output_pair day<3>(std::istream& in);
template<> output_pair day<4>(std::istream& in);
template<> output_pair day<5>(std::istream& in);
template<> output_pair day<6>(std::istream& in);
template<> output_pair day<7>(std::istream& in);
template<> output_pair day<8>(std::istream& in);
template<> output_pair day<9>(std::istream& in);
template<> output_pair day<10>(std::istream& in);
/*
template<> output_pair day<11>(std::istream& in);
template<> output_pair day<12>(std::istream& in);
template<> output_pair day<13>(std::istream& in);
template<> output_pair day<14>(std::istream& in);
template<> output_pair day<15>(std::istream& in);
template<> output_pair day<16>(std::istream& in);
template<> output_pair day<17>(std::istream& in);
template<> output_pair day<18>(std::istream& in);
template<> output_pair day<19>(std::istream& in);
template<> output_pair day<20>(std::istream& in);
template<> output_pair day<21>(std::istream& in);
template<> output_pair day<22>(std::istream& in);
template<> output_pair day<23>(std::istream& in);
template<> output_pair day<24>(std::istream& in);
template<> output_pair day<25>(std::istream& in);
*/

constexpr output_pair (*days[])(std::istream&) = {
	day<1>, day<2>, day<3>, day<4>, day<5>, day<6>, day<7>, day<8>, day<9>,
	day<10>/*, day<11>, day<12>, day<13>, day<14>, day<15>, day<16>, day<17>,
	day<18>, day<19>, day<20>, day<21>, day<22>, day<23>, day<24>, day<25>*/
};
#else
#error This header is for C++20 or later
#endif
