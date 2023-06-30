#include <limits>

template<class T>
constexpr bool checked_add(T& dest, const T& x, const T& y) noexcept
{
	using limits = std::numeric_limits<T>;
	if ((x > 0 && y > 0 && x + y > limits::max())
	    || (x < 0 && y < 0 && x < limits::min() - y))
		return true;
	dest = x + y;
	return false;
}

#ifdef __GNUC__
template<>
constexpr bool checked_add(int& dest, const int& x, const int& y) noexcept
{
	return __builtin_sadd_overflow(x, y, &dest);
}

template<>
constexpr bool
checked_add(unsigned& dest, const unsigned& x, const unsigned& y) noexcept
{
	return __builtin_uadd_overflow(x, y, &dest);
}

template<>
constexpr bool checked_add(long& dest, const long& x, const long& y) noexcept
{
	return __builtin_saddl_overflow(x, y, &dest);
}

template<>
constexpr bool
checked_add(unsigned long& dest, const unsigned long& x,
            const unsigned long& y)
	noexcept
{
	return __builtin_uaddl_overflow(x, y, &dest);
}

template<>
constexpr bool
checked_add(long long& dest, const long long& x, const long long& y) noexcept
{
	return __builtin_saddll_overflow(x, y, &dest);
}

template<>
constexpr bool
checked_add(unsigned long long& dest, const unsigned long long& x,
            const unsigned long long& y)
	noexcept
{
	return __builtin_uaddll_overflow(x, y, &dest);
}
#endif
