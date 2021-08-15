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

add_executable(ezp
	src/parser.cpp)
