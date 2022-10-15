project(ezpz)
cmake_minimum_required(VERSION 3.16.3)
enable_testing()

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
	"-Wall" "-Wpedantic" "-Wno-unused-local-typedefs" "-Wextra" "-fexceptions" "-ftemplate-backtrace-limit=0" #"-fconcepts-diagnostics-depth=3000"
	"$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb;-ftrapv;-fbounds-check;-fsanitize=undefined>"
	"$<$<CONFIG:RELEASE>:-O3>"
	)
set(CMAKE_CXX_STANDARD 20)

link_libraries(ubsan)
link_libraries(fmt)


include_directories(include)

add_executable(expr1 examples/expr_v1.cpp)
add_executable(expr2 examples/arith_expr_repl.cpp)

add_executable(xml examples/xml.cpp)
add_executable(lex examples/lexical.cpp)
add_executable(json examples/json.cpp)

find_package(GTest REQUIRED)
add_executable(stest
	test/extra/cin_context.cpp
	test/props.cpp
	test/quantifiers.cpp
	test/helper.cpp
	test/matcher.cpp
	test/consumer.cpp
	test/core.cpp
	test/context.cpp
)
target_link_libraries(stest ${GTEST_BOTH_LIBRARIES} GTest::gmock)
add_dependencies(stest rapidcheck)
target_link_libraries(stest ${rapidcheck_BINARY_DIR}/librapidcheck.a pthread)
add_test(ezpz_tests stest)
