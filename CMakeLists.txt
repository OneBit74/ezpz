project(ezpz)
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


# set(CMAKE_CXX_CLANG_TIDY
#   clang-tidy;
#   -header-filter=.;
#   -checks=clang-analyzer-core.*;)

#"-fconcepts-diagnostics-depth=20" "-fopenmp"
add_compile_options(
	"-Wall" "-Wpedantic" "-Wno-unused-local-typedefs" "-Wextra" "-fexceptions"
	"$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb;-ftrapv;-fbounds-check;-fsanitize=undefined>"
	"$<$<CONFIG:RELEASE>:-O3>"
	)
set(CMAKE_CXX_STANDARD 20)

link_libraries(ubsan)


include_directories(include)

add_executable(graph examples/graph.cpp)

add_executable(expr examples/arith_expr_repl.cpp)

add_executable(xml examples/xml.cpp)

find_package(GTest REQUIRED)
add_executable(stest
	test/test.cpp)
target_link_libraries(stest ${GTEST_BOTH_LIBRARIES})
add_dependencies(stest rapidcheck)
target_link_libraries(stest ${rapidcheck_BINARY_DIR}/librapidcheck.a)
