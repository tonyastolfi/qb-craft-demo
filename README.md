# Quick Base C++ Craft demo: Solution Notes
_Tony Astolfi_

## Summary

## Stack Choices

This solution was developed on MacOSX, v10.14.6, using Conan 1.6.1 for
package management, CMake 3.9.0 for build system configuration, GNU
Make 3.81 as the build driver, and Clang/LLVM (Apply) 9.1.0 with
C++17.  Google Mock/Test are used for the unit testing and
benchmarking, and the project also pulls in the Boost Libraries for
various augmentations to the C++17 standard library.

All code formatting is being done by ClangFormat, with default
settings similar to the Google C++ Style Guide conventions.

## How to Build

```
$ cd PROJECT_ROOT
$ mkdir -p build && cd build
$ conan install ..
$ cmake -DCMAKE_BUILD_TYPE=Release|Debug .. 
$ make test
```

(Also `make help` to show other build targets.)

## Implementation Approach and Tradeoffs

My design changes `QBRecordCollection` from a type alias (vector of
records) into a class to encapsulate its choice of data structures and
algorithms.  The high-level goals of the specific design choice are to
minimize record search time at the expense of some extra time spent at
record insertion, and some extra memory used for indexing integer
fields (which require an exact match) and string fields (which require
substring matching and therefore are indexed using a string suffix
trie data structure).

The other key element of my design is changing the QBRecord type to a
std::tuple and adding compile-time metadata in the form of
QBRecordTraits to drive the record collection implementation in a way
that is general with respect to the number and types of columns.  This
approach reduces the amount of code and enhances the
reusability/extensibility of the design at the cost of some more
complex/sophisticated programming techniques (some template
meta-programming).

## Test Plan

See:

 - `src/qb_record_collection_test.cpp`
 - `src/string_trie_test.cpp`

## Alternative Designs

- Instead of trie-based string indexing maybe use bi-gram and/or
  tri-gram bucketing of strings as a first-pass filter, then a brute
  force approach like the original baseline.

## TODOs

- The last few test points are not yet implemented
- It would be good to do stress/performance testing comparison between
  the new impl and the baseline.  The StringTrie test gives some
  notion of the speedup, but we should also measure memory usage and
  indexing time more explicitly.
