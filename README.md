# EZPZ
**ezpz** is a C++20 parser combinator library that tries to make writing parsers as easy as possible. You can combine parsers, propagate output, write custom contexts, get decent error messages and most importantly opt-out, if you need to.

## Mini-Example
```c++
parse("Hello World!", "Hello "+any(alpha)+"!"+eoi);
```

## Documentation
- [Tutorial](./examples/tutorial.cpp): Read this as an example based overview of ezpz.
- [Reference](./reference.md): Read this for more indepth information of concepts and techniques in ezpz.

## Dependencies
- fmt
- C++20 (e.g. gcc 11.1)

### Test Dependencies
- gtest/gmock
- rapidcheck (downloaded automatically by cmake)

## Building
**ezpz** is a header-only library, which means you do not have to build anything to start using this library. If you want to build the tests and examples anyway, you need to have cmake and make installed. Go into the project root directory. Then, run the following commands:
```bash
mkdir build
cd build
cmake ..
make
```
