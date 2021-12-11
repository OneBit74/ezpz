#pragma once
#include <string>
#include <unordered_map>
#include <regex>
#include <iostream>

class context {
public:
	std::string input;
	bool debug = false;
	size_t pos = 0;
	size_t depth = 0;
	std::unordered_map<std::string,std::regex> regex_cache;

	context() = default;
	inline context(std::string str) : input(std::move(str)) {}

	inline char get() const {return input[pos];}
	inline bool done() const {return pos == input.size();}
	inline void indent() const {
		for(size_t i = 0; i < depth; ++i)std::cout << '\t';
	}
};
