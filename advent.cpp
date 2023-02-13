// FIXME: performance issues when multithreading (possible false sharing)
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <future>
#include <iostream>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "common.h"

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

static constexpr output_pair (*days[])(std::istream&) = {
	day<1>, day<2>, day<3>, day<4>, day<5>, day<6>, day<7>, day<8>, day<9>,
	day<10>, day<11>, day<12>, day<13>, day<14>, day<15>, day<16>, day<17>,
	day<18>, day<19>, day<20>, day<21>, day<22>, day<23>, day<24>, day<25>
};

static constexpr unsigned job_time[] = {
	3, 4, 5, 3, 3, 4, 5, 3, 12, 3, 24, 9, 26, 820, 5387, 5463,
	37, 28, 608, 215, 37, 14, 17639, 747, 3
};

static_assert(std::size(days) == std::size(job_time),
              "days and time have different sizes");

static std::size_t parse(std::string arg)
{
	std::size_t r;
	std::istringstream s{std::move(arg)};
	if (!(s >> r))
		throw std::invalid_argument("Day must be a number");
	if (r < 1 || r > std::size(days))
		throw std::invalid_argument("Day not in range");
	return r;
}

constexpr std::vector<std::uint_fast32_t> make_jobs(const unsigned num_threads)
{
	if (num_threads != 2)
		throw std::invalid_argument("Wrong number of threads");
	return {0b1101111111111111111111111, 0b10000000000000000000000};
}

static std::pair<output_pair, std::chrono::milliseconds>
make_exception_output(const std::exception& e) noexcept
{
	using namespace std::literals;
	return {output_pair{std::string(e.what()), ""s}, 0ms};
}

static void
work(std::uint_fast32_t jobs,
     std::span<std::promise<std::pair<output_pair, std::chrono::milliseconds>>, std::size(days)> out)
{
	using namespace std::literals;
	for (unsigned d = 0; d < std::size(days); ++d) {
		if ((jobs & (std::uint_fast32_t{1} << d)) == 0)
			continue;
		std::ostringstream s;
		s << "input-" << (d + 1);
		try {
			std::ifstream f(s.str());
			const auto start = std::chrono::steady_clock::now();
			output_pair p = days[d](f);
			const auto end = std::chrono::steady_clock::now();
			const auto t = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			out[d].set_value(std::pair{std::move(p), t});
		} catch (const std::exception& e) {
			out[d].set_value(make_exception_output(e));
		}
	}
}

static int run_all_tests(const unsigned num_threads) noexcept
{
	const std::vector<std::uint_fast32_t> jobs = make_jobs(num_threads);
	std::vector<std::thread> workers;
	workers.reserve(num_threads);
	std::promise<std::pair<output_pair, std::chrono::milliseconds>> out_promise[std::size(days)];
	std::future<std::pair<output_pair, std::chrono::milliseconds>> out[std::size(days)];
	std::chrono::milliseconds durations[std::size(days)];
	for (unsigned d = 0; d < std::size(days); ++d)
		out[d] = out_promise[d].get_future();
	for (unsigned t = 0; t < num_threads; ++t)
		workers.emplace_back(work, jobs[t], std::span(out_promise));
	for (unsigned d = 0; d < std::size(days); ++d) {
		auto [p, dur] = out[d].get();
		durations[d] = std::move(dur);
		std::cout << "Day " << (d + 1) << '\n' << p.first << '\n'
		          << p.second << '\n' << std::endl;
	}
	for (auto& worker : workers)
		worker.join();
	std::cout << "Summary:\n";
	for (unsigned d = 0; d < std::size(days); ++d)
		std::cout << (d + 1) << '\t' << durations[d] << '\n';
	std::cout << std::endl;
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	if (argc == 2 && argv[1][0] == '-' && argv[1][1] == 'a' && argv[1][2] == 0) {
		return run_all_tests(2);
	} else if (argc <= 2) {
		const std::size_t d = argc > 1 ? parse(argv[1]) : std::size(days);
		const auto [p1, p2] = days[d - 1](std::cin);
		std::cout << p1 << '\n' << p2 << std::endl;
	} else {
		std::cerr << "usage: " << (argc >= 1 ? argv[0] : "advent")
		          << " [day | -a]" << std::endl;
		return EXIT_FAILURE;
	}
}
