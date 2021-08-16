#pragma once
#include <string>
#include <unordered_map>
#include <regex>

class context {
public:
	std::string input;
	bool debug = false;
	size_t pos = 0;
	size_t depth = 0;
	std::unordered_map<std::string,std::regex> regex_cache;

	context() = default;
	context(std::string str);

	char get();
	bool done();
	void indent();
};
