#pragma once
#include <iostream>

auto assign(auto& dst){
	return [&](decltype(dst)& val){
		dst = val;
	};
}

inline auto print_all = [](auto&...args){
	((std::cout << args << ' '),...); 
	std::cout << '\n';
};

auto ret(auto val){
	return [=](){return val;};
}
