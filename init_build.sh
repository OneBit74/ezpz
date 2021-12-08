#!/bin/sh
mkdir -p build
mkdir -p build/release
mkdir -p build/debug
cd build/release
cmake -GNinja -DCMAKE_BUILD_TYPE=Release ../..
	# -DCMAKE_CXX_COMPILER:FILEPATH=clang++
cd ../debug
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug ../..
	# -DCMAKE_CXX_COMPILER:FILEPATH=clang++
