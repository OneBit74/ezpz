#pragma once
#include <utility>

template<typename T>
concept std_map_c = requires(T t, T::key_type k, T::mapped_type m){
	{
		t.insert_or_assign(k,m)
	};
};
template<typename T>
concept std_seq_c = requires(T t, T::value_type val){
	t.push_back(val);
};
template<typename T>
concept std_adaptor_c = requires(T t, T::value_type val){
	t.push(val);
};
template<typename T>
concept std_set_c = requires(T t, T::value_type val){
	t.insert(val);
};

template<std_map_c into_t>
auto insert(into_t& data){
	return [&data](typename into_t::key_type& key, typename into_t::mapped_type& val){
		data.insert_or_assign(std::move(key),std::move(val));
	};
}
template<std_adaptor_c into_t>
auto insert(into_t& data){
	return [&data](typename into_t::value_type& value){
		data.push(std::move(value));
	};
};
template<std_set_c into_t>
auto insert(into_t& data){
	return [&data](typename into_t::value_type& value){
		data.insert(std::move(value));
	};
};
template<std_seq_c into_t>
auto insert(into_t& data){
	return [&data](typename into_t::value_type& value){
		data.push_back(std::move(value));
	};
};

template<typename T>
auto assign(T& target){
	return [&](T& t){
		target = std::forward<T>(t);
	};
}
template<typename T>
auto assign(T& target, T value){
	return [&,value](){
		target = value;
	};
}

inline auto print_all = [](auto&...args){
	((std::cout << args << ' '),...); 
	std::cout << '\n';
};

auto ret(auto val){
	return [=](){return val;};
}
