#pragma once
#include "ezpz/core.hpp"
#include "ezpz/parse_object.hpp"
#include "ezpz/helper.hpp"

template<parser T>
parser auto plus(T&& rhs) {
	return f_parser([r=std::forward<T>(rhs)](auto& ctx) mutable {
		if(!match(ctx,r))return false;
		while(match_or_undo(ctx,r) && !ctx.done()){}
		return true;
	});
}
template<parser T>
parser auto any(T&& rhs) {
	return f_parser([r=std::forward<T>(rhs)](auto& ctx) mutable {
		while(match_or_undo(ctx,r) && !ctx.done()){}
		return true;
	});
}
template<typename T>
parser auto notf(T&& rhs) requires parser<std::decay_t<T>>{
	return f_parser([r=std::forward<T>(rhs)](auto& ctx) mutable {
		return !match(ctx,r);
	});
}
template<parser T>
parser auto optional(T&& rhs) {
	auto ret = std::forward<T>(rhs) | notf(fail);
	return ret;
}

auto times(int amount,auto&& parser){
	return f_parser([=](auto& ctx) mutable {
			for(int i = 0; i < amount; ++i){
				if(!match(ctx,parser))return false;
			}
			return true;
		}
	);
}
auto max(int amount,auto&& parser){
	return f_parser([=](auto& ctx) mutable {
		int counter = 0;
		while(true){
			auto start = ctx.getPosition();
			if(!match(ctx,parser)){
				return true;
			}else if (counter >= amount) {
				ctx.setPosition(start);
				return false;
			}
			counter++;
		}
	});
}
auto min(int amount,auto&& parser){
	return f_parser([=](auto& ctx) mutable {
		int counter = 0;
		while(true){
			auto start = ctx.getPosition();
			if(!match(ctx,parser)){
				if(counter < amount){
					ctx.setPosition(start);
				}
				return counter >= amount;
			}
			counter++;
		}
	});
}


/* struct max_t { */
/* 	int val; */
/* 	auto operator>>(std::string_view sv) -> parse_object_ref; */
/* 	auto operator>>(parse_object_ref inner) -> parse_object_ref; */
/* }; */
/* struct min_t { */
/* 	int val; */
/* 	auto operator>>(std::string_view sv) -> parse_object_ref; */
/* 	auto operator>>(parse_object_ref inner) -> parse_object_ref; */
/* }; */
/* struct minmax_t { */
/* 	int val1,val2; */
/* 	auto operator>>(std::string_view sv) -> parse_object_ref; */
/* 	auto operator>>(parse_object_ref inner) -> parse_object_ref; */
/* }; */

/* auto min(int val) -> min_t; */
/* auto max(int val) -> max_t; */
/* auto minmax(int val1, int val2) -> minmax_t; */
/* auto times(int val) -> minmax_t; */
