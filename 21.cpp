#include <algorithm>
#include <cstdint>
#include <ios>
#include <istream>
#include <limits>
#include <map>
#include <span>
#include <stdexcept>
#include <utility>
#include <variant>

#include "common.h"

namespace {

using monkey_type = std::uint_fast32_t;
static_assert(std::numeric_limits<monkey_type>::min() >= 0,
              "The type specified for monkeys is signed");
static_assert(std::numeric_limits<monkey_type>::max() >= 26 * 26 * 26 * 26,
              "The type specified for monkeys is too small");

enum class operation { plus, minus, times, divided };

struct expression {
	monkey_type left;
	operation op;
	monkey_type right;
};

[[nodiscard]] constexpr monkey_type parse_monkey(std::span<const char, 4> name)
{
	using std::invalid_argument;
	constexpr const char alpha[] = "abcdefghijklmnopqrstuvwxyz";
	monkey_type acc = 0;
	for (const char c : name) {
		const char *p;
		if constexpr (std::is_sorted(alpha, alpha + 26)) {
			p = std::lower_bound(alpha, alpha + 26, c);
			if (*p != c) [[unlikely]]
				throw invalid_argument("Unexpected character");
		} else {
			p = std::find(alpha, alpha + 26, c);
			if (p == alpha + 26) [[unlikely]]
				throw invalid_argument("Unexpected character");
		}
		monkey_type d = p - alpha;
		acc = 26 * acc + std::move(d);
	}
	return acc;
}

static std::istream& consume_whitespace(std::istream& in)
{
	std::istream::int_type c;
	while ((c = in.peek()) == std::istream::traits_type::to_int_type(' ')
	       || c == std::istream::traits_type::to_int_type('\n'))
		in.ignore();
	return in;
}

static std::istream&
parse_job(std::map<monkey_type, std::variant<std::intmax_t, expression>>& dst,
          std::istream& in)
{
	using traits = std::istream::traits_type;
	if (!consume_whitespace(in))
		return in;
	char buf[4];
	if (!in.read(buf, 4) || in.get() != traits::to_int_type(':')
	    || !consume_whitespace(in))
		return in;
	monkey_type m = parse_monkey(buf);
	if (in.peek() >= '0' && in.peek() <= '9') {
		std::intmax_t n;
		if (!(in >> n))
			return in;
		dst.insert({std::move(m), std::move(n)});
	} else {
		if (!in.read(buf, 4) || !consume_whitespace(in))
			return in;
		monkey_type left = parse_monkey(buf);
		operation op;
		switch (in.get()) {
		default:
			in.setstate(std::ios_base::failbit);
			return in;
		case traits::to_int_type('+'):
			op = operation::plus;
			break;
		case traits::to_int_type('-'):
			op = operation::minus;
			break;
		case traits::to_int_type('*'):
			op = operation::times;
			break;
		case traits::to_int_type('/'):
			op = operation::divided;
		}
		if (!consume_whitespace(in) || !in.read(buf, 4))
			return in;
		expression ex{std::move(left), op, parse_monkey(buf)};
		dst.insert({std::move(m), std::move(ex)});
	}
	return in;
}

[[nodiscard]] constexpr monkey_type root_monkey() noexcept
{
	constexpr const char text[] = {'r', 'o', 'o', 't'};
	return parse_monkey(text);
}

[[nodiscard]] constexpr monkey_type humn_id() noexcept
{
	constexpr const char text[] = {'h', 'u', 'm', 'n'};
	return parse_monkey(text);
}

[[nodiscard]]
static std::map<monkey_type, std::variant<std::intmax_t, expression>>
parse(std::istream& in)
{
	std::map<monkey_type, std::variant<std::intmax_t, expression>> result;
	while (parse_job(result, in));
	if (!in.eof())
		throw std::runtime_error("Error while parsing puzzle input");
	if (!result.contains(root_monkey()))
		throw std::runtime_error("root monkey not found in input");
	return result;
}

[[nodiscard]] constexpr std::intmax_t
calculate(operation op, std::intmax_t left, std::intmax_t right)
{
	constexpr auto min = std::numeric_limits<std::intmax_t>::min();
	constexpr auto max = std::numeric_limits<std::intmax_t>::max();
	switch (op) {
	case operation::plus:
		if ((left > 0 && right > 0 && left > max - right)
		    || (left > 0 && right < 0 && left < min - right)
		    || (left < 0 && right > 0 && right < min - left)
		    || (left < 0 && right < 0 && left < min - right))
			throw std::domain_error("Integer overflow detected");
		return left + right;
	case operation::minus:
		if ((left > 0 && right > 0 && left < min + right)
		    || (left > 0 && right < 0 && left > max + right)
		    || (left < 0 && right > 0 && left < min + right)
		    || (left < 0 && right < 0 && left < min + right))
			throw std::domain_error("Integer overflow detected");
		return left - right;
	case operation::times:
		if ((left > 0 && right > 0 && left > max / right)
		    || (left == min && (right < 0 || right > 1))
		    || (right == min && (left < 0 || left > 1))
		    || (left < 0 && right < 0 && -left > max / -right)
		    || (left < 0 && right > 0 && left < min / right)
		    || (left > 0 && right < 0 && right < min / left))
			throw std::domain_error("Integer overflow detected");
		return left * right;
	case operation::divided:
		if (right == 0)
			throw std::domain_error("Division by zero detected");
		return left / right;
	}
	throw std::logic_error("Reached unreachable code");
}

[[nodiscard]] static std::intmax_t
calc_reduce(std::map<monkey_type, std::variant<std::intmax_t, expression>>& j,
            const monkey_type x)
{
	const auto it = j.find(x);
	if (it == j.end())
		throw std::invalid_argument("Monkey not found");
	if (const expression* ex = std::get_if<expression>(&it->second)) {
		const std::intmax_t left = calc_reduce(j, ex->left);
		const std::intmax_t right = calc_reduce(j, ex->right);
		const std::intmax_t result = calculate(ex->op, left, right);
		j.insert_or_assign(it, x, result);
		return result;
	} else {
		return std::get<std::intmax_t>(it->second);
	}
}

[[nodiscard]] static std::uintmax_t
predict_root(std::map<monkey_type, std::variant<std::intmax_t, expression>> j)
{
	const std::intmax_t result = calc_reduce(j, root_monkey());
	if (result < 0)
		throw std::range_error("Result was negative");
	return result;
}

[[nodiscard]] static int
locate_humn(const std::map<monkey_type,
                           std::variant<std::intmax_t, expression>>& m,
            std::map<monkey_type,
                     std::variant<std::intmax_t, expression>>::const_iterator it)
{
	if (it->first == humn_id())
		return 0b11;
	const auto& v = it->second;
	if (const expression* ex = std::get_if<expression>(&v)) {
		const auto l = m.find(ex->left);
		if (l == m.end())
			throw std::invalid_argument("Ill-formed tree");
		const auto r = m.find(ex->right);
		if (r == m.end())
			throw std::invalid_argument("Ill-formed tree");
		const bool found_left = locate_humn(m, l);
		const bool found_right = locate_humn(m, r);
		return (found_left * 0b10) | (found_right * 0b01);
	}
	return 0b00;
}

[[nodiscard]] static std::intmax_t
update_target_left(std::map<monkey_type, std::variant<std::intmax_t, expression>>& j,
                   operation op, monkey_type right, std::intmax_t target)
{
	switch (op) {
	default:
		throw std::logic_error("Reached unreachable code");
	case operation::plus:
		return target - calc_reduce(j, right);
	case operation::minus:
		return target + calc_reduce(j, right);
	case operation::times:
		return target / calc_reduce(j, right);
	case operation::divided:
		return target * calc_reduce(j, right);
	}
}

[[nodiscard]] static std::intmax_t
update_target_right(std::map<monkey_type, std::variant<std::intmax_t, expression>>& j,
                    monkey_type left, operation op, std::intmax_t target)
{
	switch (op) {
	default:
		throw std::logic_error("Reached unreachable code");
	case operation::plus:
		return target - calc_reduce(j, left);
	case operation::minus:
		return calc_reduce(j, left) - target;
	case operation::times:
		return target / calc_reduce(j, left);
	case operation::divided:
		return calc_reduce(j, left) / target;
	}
}

[[nodiscard]] static std::uintmax_t
solve_input(std::map<monkey_type, std::variant<std::intmax_t, expression>>&& j,
            monkey_type to_solve, std::intmax_t target)
{
	if (to_solve == humn_id())
		return target;
	const auto it = j.find(to_solve);
	if (it == std::end(j))
		throw std::invalid_argument("Monkey not found");
	switch (locate_humn(j, it)) {
		const expression* ex;
	default:
		throw std::logic_error("Reached unreachable code");
	case 0b11:
		throw std::invalid_argument("Found humn on both sides");
	case 0b10:
		ex = &std::get<expression>(it->second);
		target = update_target_left(j, ex->op, ex->right, target);
		return solve_input(std::move(j), ex->left, target);
	case 0b01:
		ex = &std::get<expression>(it->second);
		target = update_target_right(j, ex->left, ex->op, target);
		return solve_input(std::move(j), ex->right, target);
	}
}

[[nodiscard]] static std::uintmax_t
solve_input(std::map<monkey_type, std::variant<std::intmax_t, expression>>&& j)
{
	const auto it = j.find(root_monkey());
	if (it == j.cend())
		throw std::invalid_argument("No root node found");
	switch (locate_humn(j, it)) {
		const expression* ex;
		std::intmax_t t;
	default:
		throw std::logic_error("Reached unreachable code");
	case 0b00:
		throw std::invalid_argument("Found no humn");
	case 0b11:
		throw std::invalid_argument("Found humn on both sides");
	case 0b10:
		ex = &std::get<expression>(it->second);
		t = calc_reduce(j, ex->right);
		return solve_input(std::move(j), ex->left, t);
	case 0b01:
		ex = &std::get<expression>(it->second);
		t = calc_reduce(j, ex->left);
		return solve_input(std::move(j), ex->right, t);
	}
}

}

template<> output_pair day<21>(std::istream& in)
{
	auto jobs = parse(in);
	const std::uintmax_t root_prediction = predict_root(jobs);
	return {root_prediction, solve_input(std::move(jobs))};
}
