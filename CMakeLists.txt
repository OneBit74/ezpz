project(ezp)
cmake_minimum_required(VERSION 3.21)

include(FetchContent)

add_compile_options(
	"-Wall" "-Wpedantic" "-Wextra" "-fexceptions" "-fopenmp" "-fconcepts-diagnostics-depth=20"
	"$<$<CONFIG:RELEASE>:-O3>"
	"$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb;-ftrapv;-fbounds-check;-fsanitize=undefined;-fsanitize-undefined-trap-on-error>"
	)
set(CMAKE_CXX_STANDARD 20)



find_package(fmt REQUIRED)
include_directories(fmt)

add_library(ezpz
	src/context.cpp
	src/quantifiers.cpp
	src/parse_object.cpp
	src/matcher.cpp
	src/helper.cpp)
target_link_libraries(ezpz fmt)

add_executable(main
	src/parser.cpp)
target_link_libraries(main fmt ezpz)

find_package(GTest REQUIRED)
add_executable(stest
	src/test.cpp)
target_link_libraries(stest fmt ${GTEST_BOTH_LIBRARIES} ezpz)
