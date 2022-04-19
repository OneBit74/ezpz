#!/bin/sh
mkdir -p build
mkdir -p build/release
mkdir -p build/debug
cd build/release
cmake ../.. -DCMAKE_BUILD_TYPE=Release -GNinja
	# -DCMAKE_CXX_COMPILER:FILEPATH=clang++
cd ../debug
cmake ../.. -DCMAKE_BUILD_TYPE=Debug -GNinja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	# -DCMAKE_CXX_COMPILER:FILEPATH=clang++
