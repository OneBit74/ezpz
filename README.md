# EZPZ
**ezpz** is a C++20 parser combinator library that tries to make writing parsers as easy as possible. You can combine parsers, propagate output, write custom contexts, get decent error messages and most importantly opt-out, if you need to.

## Tutorial
Read [./examples/tutorial.cpp](./examples/tutorial.cpp) as an example based overview of ezpz.

## Dependencies
- fmt
- C++20 (e.g. gcc 11.1)

### Testing
- gtest/gmock

## Building
**ezpz** is a header-only library. To build the tests and examples you need a compiler supporting c++20 language features (e.g. gcc 11.1). To build you need to have cmake and make installed. Go into the project root directory. Then, run the following commands:
```bash
mkdir build
cd build
cmake ..
make
```
