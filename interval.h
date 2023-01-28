#if defined __cplusplus && __cplusplus >= 202002L
#include <compare>
#include <concepts>

template<class T>
	requires std::totally_ordered<T> && std::move_constructible<T>
class interval {
public:
	enum class relation { disjoint, overlap, contained };

	constexpr interval()
		noexcept(noexcept(T{}))
		requires std::default_initializable<T>
		: min{}
		, max{}
	{}

	constexpr interval(T a, T b)
		noexcept(noexcept(a = static_cast<T&&>(b)))
		: min{static_cast<T&&>(a)}
		, max{static_cast<T&&>(b)}
	{
		if (min > max) {
			T temp = static_cast<T&&>(min);
			min = static_cast<T&&>(max);
			max = static_cast<T&&>(temp);
		}
	}

	constexpr std::partial_ordering operator<=>(const interval<T>& rhs)
		const noexcept(noexcept(min <=> rhs.min))
	{
		return max < rhs.min ? std::partial_ordering::less :
		       rhs.max < min ? std::partial_ordering::greater :
		       std::partial_ordering::unordered;
	}

	constexpr bool contains(const T& x)
		const noexcept(noexcept(min <= x))
	{
		return min <= x && x <= max;
	}

	constexpr bool contains(const interval<T>& x)
		const noexcept(noexcept(min <= x.min))
	{
		return min <= x.min && x.max <= max;
	}

	static constexpr relation compare(const interval& a, const interval& b)
		noexcept(noexcept(a.max < b.min) && noexcept(a.min <= b.max))
	{
		return a.max < b.min || b.max < a.min ? relation::disjoint :
		       a.contains(b) || b.contains(a) ? relation::contained :
		       relation::overlap;
	}

	constexpr const T& lower_bound() const noexcept { return min; }
	constexpr const T& upper_bound() const noexcept { return max; }

private:
	T min;
	T max;
};
#else
#error This header is for C++20 or later
#endif
