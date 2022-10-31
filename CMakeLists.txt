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

find_package(GTest REQUIRED)
link_libraries(ubsan fmt pthread ${GTEST_BOTH_LIBRARIES} gmock)


include_directories(include)

set(test_src 
	test/extra/cin_context.cpp
	test/props.cpp
	test/quantifiers.cpp
	test/helper.cpp
	test/matcher.cpp
	test/consumer.cpp
	test/core.cpp
	test/context.cpp
)
set(examples 
	tutorial
	expr_v1
	arith_expr_repl
	xml
	lexical
	json
	cli
)


add_executable(stest ${test_src})
add_test(unittests stest)
add_dependencies(stest rapidcheck)
target_link_libraries(stest ${rapidcheck_BINARY_DIR}/librapidcheck.a)

foreach(example ${examples})
	add_executable(${example} examples/${example}.cpp)
	if(NOT ${example} STREQUAL arith_expr_repl)
		add_test(example/${example} ${example})
	endif()
endforeach()
