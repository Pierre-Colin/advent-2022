#include <cstdint>
#include <optional>
#include <vector>

#include "interval.h"

class interval_union {
public:
	void insert(interval<std::intmax_t>&& i);
	void insert(const std::intmax_t x) { insert(interval{x, x}); }
	[[nodiscard]] std::uintmax_t cardinal() const noexcept;
	[[nodiscard]] bool contains(std::intmax_t x) const noexcept;

	[[nodiscard]] std::optional<std::intmax_t>
	dead_spot(const interval<std::intmax_t>& l) const noexcept;

private:
	std::vector<interval<std::intmax_t>> parts{};
};
