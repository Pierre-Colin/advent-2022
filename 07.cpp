#include <algorithm>
#include <cstdint>
#include <execution>
#include <istream>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "common.h"

using namespace std::literals;

struct file {
	std::string name;
	std::uintmax_t size;
};

namespace {

class session_parser;

class directory : public file {
	friend session_parser;
public:
	class iterator {
	public:
		using value_type = std::uintmax_t;
		using difference_type = void;
		using reference = const value_type&;
		using pointer = const value_type*;
		using iterator_category = std::input_iterator_tag;

		constexpr iterator() noexcept = default;
		iterator(directory& d);

		constexpr value_type operator*() const noexcept {
			return stack.back().size;
		}

		iterator& operator++();

		iterator operator++(int) {
			iterator old = *this;
			++*this;
			return old;
		}

		bool operator==(const iterator& rhs) const {
			return stack.size() == rhs.stack.size()
			       && std::equal(std::execution::unseq,
			                     stack.cbegin(), stack.cend(),
			                     rhs.stack.cbegin());
		}

		bool operator!=(const iterator& rhs) const {
			return !(*this == rhs);
		}

		pointer operator->() const noexcept {
			return &stack.back().size;
		}

	private:
		struct state {
			bool operator==(const state& rhs) const noexcept {
				return dir == rhs.dir && it == rhs.it;
			}

			directory* dir;
			std::vector<std::unique_ptr<directory>>::iterator it;
			std::uintmax_t size;
		};

		std::vector<state> stack{};
	};

	directory(std::istream& in);
	directory(const directory&) = delete;
	directory(directory&& rhs);
	directory& operator=(const directory&) = delete;
	iterator begin() { return iterator(*this); }
	constexpr iterator end() const noexcept { return iterator(); }

private:
	directory(directory* p, std::string name);
	directory* find_subdirectory(std::string_view name) const;
	void emplace_directory(std::string name);
	void emplace_file(file f);
	bool named(std::string_view n) const noexcept { return name == n; }

	directory* parent;
	std::vector<file> files;
	std::vector<std::unique_ptr<directory>> subdirectories;
};

class session_parser {
public:
	constexpr session_parser(directory& d) noexcept : dir{&d} {}
	std::istream& read(std::istream& in);

private:
	directory* dir;
};

directory::iterator::iterator(directory& d) : stack{}
{
	directory* ptr = &d;
	do {
		std::uintmax_t file_size = 0;
		for (const auto& file : ptr->files)
			file_size += file.size;
		stack.emplace_back(ptr, ptr->subdirectories.begin(),
		                   file_size);
		if (stack.back().it != stack.back().dir->subdirectories.end())
			ptr = stack.back().it->get();
	} while (stack.back().it != stack.back().dir->subdirectories.end());
}

directory::iterator& directory::iterator::operator++()
{
	const std::uintmax_t child_size = stack.back().size;
	stack.pop_back();
	if (stack.empty())
		return *this;
	stack.back().size += child_size;
	if ((++stack.back().it) == stack.back().dir->subdirectories.end())
		return *this;
	directory* ptr = stack.back().it->get();
	do {
		std::uintmax_t file_size = 0;
		for (const auto& file : ptr->files)
			file_size += file.size;
		stack.emplace_back(ptr, ptr->subdirectories.begin(),
		                   file_size);
		if (stack.back().it != stack.back().dir->subdirectories.end())
			ptr = stack.back().it->get();
	} while (stack.back().it != stack.back().dir->subdirectories.end());
	return *this;
}

std::istream& session_parser::read(std::istream& in)
{
	std::string line;
	if (!std::getline(in, line))
		return in;
	if (line.starts_with("$ "sv)) {
		constexpr char cd[] = "cd ";
		constexpr char ls[] = "ls";
		if (line.size() == 4
		    && std::equal(ls, ls + 2, line.cbegin() + 2))
			return in;
		if (line.size() <= 5
		    || !std::equal(cd, cd + 3, line.cbegin() + 2))
			throw std::runtime_error("Invalid command");
		if (line.size() == 6 && line[5] == '/') {
			while (dir->parent)
				dir = dir->parent;
		} else if (line.size() == 7 && line.ends_with(".."sv)) {
			if (dir->parent == nullptr) [[unlikely]]
				throw std::runtime_error("No parent dir");
			dir = dir->parent;
		} else {
			dir = dir->find_subdirectory(line.substr(5));
		}
	} else if (line.starts_with("dir "sv)) {
		dir->emplace_directory(line.substr(4));
	} else {
		std::istringstream is{line};
		std::uintmax_t size;
		std::string name;
		if (!(is >> size >> name)) [[unlikely]]
			throw std::runtime_error("Bad ls format");
		dir->emplace_file(file{std::move(name), size});
	}
	return in;
}

directory::directory(std::istream& in)
	: file{"/"s, 0}
	, parent{nullptr}
	, files{}
	, subdirectories{}
{
	constexpr const char boilerplate[] = "$ cd /\n";
	char buf[7];
	if (!in.read(buf, 7) && !in.eof()) [[unlikely]]
		throw std::runtime_error("Error reading puzzle input");
	if (!std::equal(boilerplate, boilerplate + 7, std::cbegin(buf)))
		throw std::runtime_error("Puzzle input doesn't begin right");
	session_parser p{*this};
	while (p.read(in));
	if (!in.eof()) [[unlikely]]
		throw std::runtime_error("Error while reading puzzle input");
}

directory::directory(directory&& rhs)
	: file{std::move(rhs.name), 0}
	, parent{std::exchange(rhs.parent, nullptr)}
	, files{std::move(rhs.files)}
	, subdirectories{std::move(rhs.subdirectories)}
{}

directory::directory(directory* p, std::string fname)
	: file{std::move(fname), 0}
	, parent{p}
	, files{}
	, subdirectories{}
{}

directory* directory::find_subdirectory(std::string_view fname) const
{
	const auto pred = [=](const auto& p) { return p->named(fname); };
	const auto it = std::find_if(std::execution::unseq,
	                             subdirectories.cbegin(),
	                             subdirectories.cend(), pred);
	if (it == subdirectories.cend())
		throw std::runtime_error("Directory not found");
	return it->get();
}

void directory::emplace_directory(std::string fname)
{
	subdirectories.emplace_back(std::make_unique<directory>(
		directory{this, std::move(fname)}
	));
}

void directory::emplace_file(file f)
{
	files.emplace_back(std::move(f));
}

}

static std::vector<std::uintmax_t> directory_sizes(directory& dir)
{
	std::vector<std::uintmax_t> result(dir.begin(), dir.end());
	if (result.empty()) [[unlikely]]
		throw std::invalid_argument("Tree is empty");
	std::sort(result.begin(), result.end());
	return result;
}

template<> output_pair day<7>(std::istream& in)
{
	directory dir{in};
	const auto sizes = directory_sizes(dir);
	const auto a = std::upper_bound(sizes.cbegin(), sizes.cend(), 100000);
	const std::uintmax_t free_space = 70000000 - sizes.back();
	const std::uintmax_t to_free = 30000000 - free_space;
	const auto b = std::lower_bound(sizes.cbegin(), sizes.cend(), to_free);
	if (b == sizes.cend()) [[unlikely]]
		throw std::runtime_error("Cannot free enough space");
	return {std::accumulate(sizes.cbegin(), a, 0), *b};
}
