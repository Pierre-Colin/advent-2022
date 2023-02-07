#include <algorithm>
#include <cstdint>
#include <functional>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

#include "interval.h"
#include "interval_union.h"

void interval_union::insert(interval<std::intmax_t>&& i)
{
	/*
	static constexpr auto cmp =
		[](const interval<std::intmax_t>& a,
		   const interval<std::intmax_t>& b) {
			return a.lower_bound() < b.lower_bound();
		};
	if (parts.empty()) {
		parts.emplace_back(std::move(i));
		return;
	}
	auto it = std::lower_bound(parts.begin(), parts.end(), i, cmp);
	const std::intmax_t i_min = i.lower_bound();
	const std::intmax_t i_max = i.upper_bound();
	if (it == parts.end()) {
		interval<std::intmax_t>& back = parts.back();
		if (back.upper_bound() + 1 < i_min)
			parts.emplace_back(i);
		else if (back.upper_bound() < i_max)
			back = interval{back.lower_bound(), i_max};
		return;
	}
	if (it->lower_bound() == i_min) {
		if (it->upper_bound() >= i_max)
			return;
		*it = i;
	} else if (i_max + 1 >= it->lower_bound()) {
		if (it->upper_bound() < i_max)
			*it = i;
		else
			*it = interval{i_min, it->upper_bound()};
	} else {
		parts.emplace(it, i);
	}
	for (it = parts.begin(); it + 1 != parts.end(); (void) 0) {
		if (it->upper_bound() + 1 >= (it + 1)->lower_bound()) {
			if (it->upper_bound() < (it + 1)->upper_bound())
				*it = interval{it->lower_bound(), (it + 1)->upper_bound()};
			parts.erase(it + 1);
		} else {
			++it;
		}
	}
	*/
	static constexpr auto cmp_low =
		[](const interval<std::intmax_t>& l,
		   const interval<std::intmax_t>& r) noexcept {
			return l.lower_bound() < r.lower_bound();
		};
	const std::intmax_t lim = i.upper_bound();
	auto it = std::upper_bound(parts.begin(), parts.end(), i, cmp_low);
	it = parts.emplace(it, std::move(i));
	if (it != parts.begin())
		--it;
	while (it + 1 != parts.end() && it->lower_bound() <= lim) {
		if (it->upper_bound() + 1 >= (it + 1)->lower_bound()) {
			const std::intmax_t r_max = (it + 1)->upper_bound();
			if (it->upper_bound() < r_max)
				*it = interval{it->lower_bound(), r_max};
			it = parts.erase(it + 1) - 1;
		} else {
			++it;
		}
	}
}

std::uintmax_t interval_union::cardinal() const noexcept
{
	static constexpr auto len = [](const interval<std::intmax_t>& i) {
		const std::intmax_t d = i.upper_bound() - i.lower_bound();
		return static_cast<std::uintmax_t>(d);
	};
	return std::transform_reduce(parts.cbegin(), parts.cend(),
	                             parts.size(), std::plus{}, len);
}

bool interval_union::contains(const std::intmax_t x) const noexcept
{
	const interval<std::intmax_t> i{x};
	const auto it = std::lower_bound(parts.cbegin(), parts.cend(), i);
	return it != parts.cend() && it->contains(x);
}

std::optional<std::intmax_t>
interval_union::dead_spot(const interval<std::intmax_t>& s) const noexcept
{
	static constexpr auto comp_sup =
		[](const interval<std::intmax_t>& l,
		   const interval<std::intmax_t>& r) {
			return l.upper_bound() < r.upper_bound();
		};
	const std::intmax_t s_min = s.lower_bound();
	const auto left = std::lower_bound(parts.cbegin(), parts.cend(),
	                                   interval<std::intmax_t>{0, s_min},
	                                   comp_sup);
	if (left == parts.cend())
		return 0;
	const std::intmax_t s_max = s.upper_bound();
	const auto right = std::lower_bound(left, parts.cend(),
	                                    interval<std::intmax_t>{0, s_max},
	                                    comp_sup);
	if (right == parts.cend())
		return parts.back().upper_bound() + 1;
	return right != left ? std::optional{left->upper_bound() + 1}
	                     : std::nullopt;
}
