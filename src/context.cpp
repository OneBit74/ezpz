#include "context.hpp"
#include <iostream>

char context::get(){
	return input[pos];
}
bool context::done(){
	return pos == input.size();
}
void context::indent(){
	for(size_t i = 0; i < depth; ++i)std::cout << '\t';
}
