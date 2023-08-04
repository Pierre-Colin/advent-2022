#include <limits>

template<class T>
constexpr bool checked_add(T& dest, const T x, const T y) noexcept
{
	using lim = std::numeric_limits<T>;
	if ((y > 0 && x > lim::max() - y) || (y < 0 && x < lim::min() - y))
		return true;
	dest = x + y;
	return false;
}

template<class T>
constexpr bool checked_sub(T& dest, const T x, const T y) noexcept
{
	using lim = std::numeric_limits<T>;
	if ((y < 0 && x > lim::max() + y) || (y > 0 && x > lim::min() + y))
		return true;
	dest = x + y;
	return false;
}

template<class T>
constexpr bool checked_mul(T& dest, const T x, const T y) noexcept
{
	using lim = std::numeric_limits<T>;
	if constexpr (lim::min() < 0) {
		if ((x == -1 && y == lim::min())
		    || (x == lim::min() && y == -1)
		    || (x > 0 && (y < lim::min() / x || y > lim::max() / x))
		    || (x < 0 && (y > lim::min() / x || y < lim::max() / x)))
			return true;
	} else {
		if (y > 0 && x > lim::max() / y)
			return true;
	}
	dest = x * y;
	return false;
}

#ifdef __GNUC__
template<>
constexpr bool checked_add(int& dest, const int x, const int y) noexcept
{
	return __builtin_sadd_overflow(x, y, &dest);
}

template<>
constexpr bool
checked_add(unsigned& dest, const unsigned x, const unsigned y) noexcept
{
	return __builtin_uadd_overflow(x, y, &dest);
}

template<>
constexpr bool checked_add(long& dest, const long x, const long y) noexcept
{
	return __builtin_saddl_overflow(x, y, &dest);
}

template<>
constexpr bool
checked_add(unsigned long& dest, const unsigned long x,
            const unsigned long y)
	noexcept
{
	return __builtin_uaddl_overflow(x, y, &dest);
}

template<>
constexpr bool
checked_add(long long& dest, const long long x, const long long y) noexcept
{
	return __builtin_saddll_overflow(x, y, &dest);
}

template<>
constexpr bool
checked_add(unsigned long long& dest, const unsigned long long x,
            const unsigned long long y)
	noexcept
{
	return __builtin_uaddll_overflow(x, y, &dest);
}

template<>
constexpr bool checked_sub(int& dest, const int x, const int y) noexcept
{
	return __builtin_ssub_overflow(x, y, &dest);
}

template<>
constexpr bool
checked_sub(unsigned& dest, const unsigned x, const unsigned y) noexcept
{
	return __builtin_usub_overflow(x, y, &dest);
}

template<>
constexpr bool checked_sub(long& dest, const long x, const long y) noexcept
{
	return __builtin_ssubl_overflow(x, y, &dest);
}

template<>
constexpr bool
checked_sub(unsigned long& dest, const unsigned long x,
            const unsigned long y)
	noexcept
{
	return __builtin_usubl_overflow(x, y, &dest);
}

template<>
constexpr bool
checked_sub(long long& dest, const long long x, const long long y) noexcept
{
	return __builtin_ssubll_overflow(x, y, &dest);
}

template<>
constexpr bool
checked_sub(unsigned long long& dest, const unsigned long long x,
            const unsigned long long y)
	noexcept
{
	return __builtin_usubll_overflow(x, y, &dest);
}

template<>
constexpr bool checked_mul(int& dest, const int x, const int y) noexcept
{
	return __builtin_smul_overflow(x, y, &dest);
}

template<>
constexpr bool
checked_mul(unsigned& dest, const unsigned x, const unsigned y) noexcept
{
	return __builtin_umul_overflow(x, y, &dest);
}

template<>
constexpr bool checked_mul(long& dest, const long x, const long y) noexcept
{
	return __builtin_smull_overflow(x, y, &dest);
}

template<>
constexpr bool
checked_mul(unsigned long& dest, const unsigned long x,
            const unsigned long y)
	noexcept
{
	return __builtin_umull_overflow(x, y, &dest);
}

template<>
constexpr bool
checked_mul(long long& dest, const long long x, const long long y) noexcept
{
	return __builtin_smulll_overflow(x, y, &dest);
}

template<>
constexpr bool
checked_mul(unsigned long long& dest, const unsigned long long x,
            const unsigned long long y)
	noexcept
{
	return __builtin_umulll_overflow(x, y, &dest);
}
#endif
