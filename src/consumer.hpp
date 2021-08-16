#pragma once
#include <utility>

template<typename vector>
auto emplace(vector& vec){
	return [&vec=vec](typename vector::value_type&& value){
		vec.emplace_back(std::forward<typename vector::value_type>(value));
	};
};

template<typename vector>
auto push(vector& vec){
	return [&vec=vec](typename vector::value_type&& value){
		vec.push_back(std::forward<typename vector::value_type>(value));
	};
};

template<typename T>
auto assign(T& target){
	return [&](T&& t){
		target = std::forward<T>(t);
	};
}
