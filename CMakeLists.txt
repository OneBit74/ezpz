project(ezp)
cmake_minimum_required(VERSION 3.21)

include(FetchContent)
FetchContent_Declare(rapidcheck
	GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
	GIT_PROGRESS true
	GIT_SHALLOW true
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/rapidcheck
)
FetchContent_MakeAvailable(rapidcheck)
include_directories(${rapidcheck_SOURCE_DIR}/include)
include_directories(${rapidcheck_SOURCE_DIR}/extras/gtest/include)


add_compile_options(
	"-Wall" "-Wpedantic" "-Wextra" "-fexceptions" "-fopenmp" "-fconcepts-diagnostics-depth=20"
	"$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb;-ftrapv;-fbounds-check;-fsanitize=undefined>"
	"$<$<CONFIG:RELEASE>:-O3>"
	)
set(CMAKE_CXX_STANDARD 20)

link_libraries(ubsan)


find_package(fmt REQUIRED)
include_directories(fmt)

add_library(ezpz
	src/context.cpp
	src/quantifiers.cpp
	src/parse_object.cpp
	src/matcher.cpp
	src/helper.cpp)
target_link_libraries(ezpz fmt)

add_executable(redone src/redone.cpp)
target_link_libraries(redone ezpz)

add_executable(main
	src/parser.cpp)
target_link_libraries(main fmt ezpz)

find_package(GTest REQUIRED)
add_executable(stest
	src/test.cpp)
target_link_libraries(stest fmt ${GTEST_BOTH_LIBRARIES} ezpz)
add_dependencies(stest rapidcheck)
target_link_libraries(stest ${rapidcheck_BINARY_DIR}/librapidcheck.a)
