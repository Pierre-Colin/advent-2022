Advent of Code 2022 solutions
=============================

These are my C++20 solutions to the [Advent of Code 2022](https://adventofcode.com/2022). Everything is written using plain C++ and is intended to be as portable as possible. The code is in the process of being cleaned, and new days will be added as they are proofread and cleaned up.

Building
--------

Just run `make`. The `Makefile` is POSIX compliant, and if you are using a compiler other than G++ it should be easy to adjust. If available, don’t hesitate to use the `-j` option to parallelize the building process.

Organization
------------

The plan was to use idiomatic C++20 as much as possible. In practice, it wasn’t that much:

* modules were not used because they are not stable in GCC;

* coroutines turned out not to have been useful;

* there was an attempt at using concepts, but they were more trouble than they were worth;

* ranges had some mild uses.

An option to run all days in parallel and time them is to be implemented and is likely to bring some more C++20 into this codebase.

Each day has its own translation unit with one externally-linked function defined in it. It takes the puzzle input as a `std::istream` and returns an `output_pair` object, which is a pair of value-semantic polymorphic objects declared in `"common.h"`. These are implemented using plain unions for two reasons:

* I wanted to learn how to use those;

* it prevents `"common.h"` from implicitly including the very complex `<variant>` header file.

A plain array containing all the function pointers is defined in the same file. Its purpose is to derive the right function to call from a runtime-given parameter (this system is barely more elegant than a dirty `switch` statement; ideas to make things cleaner are welcome).

The file `"interval.h"` defines an integer interval type, which was useful for days 4, 14 and 15.

The script `input-dl.sh` takes your session ID cookie as a parameter and downloads all your puzzle inputs. I don’t know AoC’s policy on doing that, so you’re encouraged to change the bounds in its main loop to not download them all at once.

**Fun fact:** In C++, the only things guaranteed about the order of characters is that the null character is zero and all decimal digits are mapped sequentially. This is why days where letters must be mapped to their position in the alphabet use those weird `constexpr` arrays. Even POSIX doesn’t define ASCII to be the standard execution character set.

My opinion on AoC 2022
----------------------

Definitely easier than 2021, and by far the year I looked for hints from other users the least (only once, for what heuristics to use on day 19). Still, not as satisfying as [solving everything in C](https://github.com/Pierre-Colin/advent-2020). Before even starting day 1, I correctly guessed that:

* the Chinese remainder theorem would be useful (on day 11, though its application was trivial);

* one day (17) would involve detecting cycles in a system to skip almost all computations.

Unless GCC’s module support is complete by December 2023, I will not be using C++ next time.
