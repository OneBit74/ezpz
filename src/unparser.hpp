#pragma once
#include <iostream>

auto assign(auto& dst){
	using dst_type = std::decay_t<decltype(dst)>;
	return [&](dst_type& val){
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
