#include <algorithm>
#include <cstdint>
#include <execution>
#include <ios>
#include <istream>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <variant>
#include <vector>

#include "common.h"

namespace {

class value {
public:
	using list_type = std::vector<std::unique_ptr<value>>;

	explicit value(std::istream& in);
	value(const value& rhs);
	constexpr value(std::uintmax_t n) noexcept : data{n} {}
	value(list_type&& l) noexcept(nothrow_move) : data{std::move(l)} {}
	std::strong_ordering operator<=>(const value& rhs) const noexcept;
	value& operator=(const value& rhs) = delete;
	value& operator=(value&& rhs) noexcept = default;

	bool operator==(const value& rhs) const noexcept {
		return std::is_eq(*this <=> rhs);
	}

	static value separation_packet(std::uintmax_t n);

private:	
	static constexpr bool nothrow_move =
		std::is_nothrow_move_constructible_v<list_type>;

	std::variant<std::uintmax_t, list_type> data;
};

value::value(std::istream& in) : data{}
{
	std::uintmax_t n;
	if (in >> n) {
		data = n;
	} else {
		in.clear();
		if (in.get() != '[') [[unlikely]] {
			in.setstate(std::ios_base::failbit);
			throw std::runtime_error("Unexpected character");
		}
		data = list_type{};
		if (in.peek() == ']') {
			in.ignore();
			return;
		}
		while (in) [[likely]] {
			std::unique_ptr<value> p = std::make_unique<value>(in);
			std::get<list_type>(data).emplace_back(std::move(p));
			const std::istream::int_type c = in.get();
			if (c == ']') {
				return;
			} else if (c != ',') {
				in.setstate(std::ios_base::failbit);
				throw std::runtime_error("Expected comma");
			}
		}
		throw std::runtime_error("Puzzle input error");
	}
}

value::value(const value& rhs) : data{}
{
	if (std::holds_alternative<std::uintmax_t>(rhs.data)) {
		data = std::get<std::uintmax_t>(rhs.data);
		return;
	}
	data = list_type{};
	for (const auto& p : std::get<list_type>(rhs.data)) {
		std::get<list_type>(data).emplace_back(
			std::make_unique<value>(*p)
		);
	}
}

std::strong_ordering value::operator<=>(const value& rhs) const noexcept
{
	using namespace std;
	struct visitor {
		strong_ordering operator()(uintmax_t a, uintmax_t b) noexcept {
			return a <=> b;
		}

		strong_ordering
		operator()(const list_type& a, const list_type& b)
			noexcept
		{
			auto ba = begin(a);
			const auto ea = end(a);
			auto bb = begin(b);
			const auto eb = end(b);
			while (ba != ea && bb != eb) {
				strong_ordering cmp = **ba <=> **bb;
				if (is_neq(cmp))
					return cmp;
				++ba;
				++bb;
			}
			return ba == ea && bb == eb ? strong_ordering::equal :
			       ba == ea ? strong_ordering::less :
			       strong_ordering::greater;
		}

		strong_ordering operator()(uintmax_t a, const list_type& b)
			noexcept
		{
			if (b.empty())
				return strong_ordering::greater;
			const strong_ordering cmp = value(a) <=> *b.front();
			return is_neq(cmp) ? cmp :
			       b.size() > 1 ? strong_ordering::less :
			       strong_ordering::equal;
		}

		strong_ordering operator()(const list_type& a, uintmax_t b)
			noexcept
		{
			if (a.empty())
				return strong_ordering::less;
			const strong_ordering cmp = *a.front() <=> value(b);
			return is_neq(cmp) ? cmp :
			       a.size() > 1 ? strong_ordering::greater :
			       strong_ordering::equal;
		}
	};

	return std::visit(visitor{}, data, rhs.data);
}

value value::separation_packet(const std::uintmax_t n)
{
	list_type inner;
	inner.emplace_back(std::make_unique<value>(n));
	list_type outer;
	outer.emplace_back(std::make_unique<value>(std::move(inner)));
	return value{std::move(outer)};
}

}

template<> output_pair day<13>(std::istream& in)
{
	std::uintmax_t index = 1;
	std::uintmax_t sum = 0;
	std::vector<value> packets;
	while (in) {
		packets.emplace_back(in);
		packets.emplace_back(in);
		if (packets[packets.size() - 2] <= packets.back())
			sum += index;
		while (in.peek() == '\n')
			in.ignore();
		if (in.peek() == std::istream::traits_type::eof())
			break;
		++index;
	}
	const value a = value::separation_packet(2);
	const value b = value::separation_packet(6);
	packets.emplace_back(a);
	packets.emplace_back(b);
	std::sort(std::execution::unseq, packets.begin(), packets.end());
	const auto i = std::lower_bound(packets.cbegin(), packets.cend(), a);
	const auto j = std::lower_bound(packets.cbegin(), packets.cend(), b);
	return {sum, (i - packets.cbegin() + 1) * (j - packets.cbegin() + 1)};
}
