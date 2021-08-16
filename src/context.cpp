#include "context.hpp"
#include <iostream>

context::context(std::string str) : input(std::move(str)) {}
char context::get(){
	return input[pos];
}
bool context::done(){
	return pos == input.size();
}
void context::indent(){
	for(size_t i = 0; i < depth; ++i)std::cout << '\t';
}
