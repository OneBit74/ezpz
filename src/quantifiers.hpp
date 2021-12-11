#pragma once
#include "parse_object.hpp"
#include "ezpz.hpp"
#include "helper.hpp"

template<parser T>
parser auto plus(T&& rhs) {
	return f_parser([r=std::forward<T>(rhs)](context& ctx) mutable {
		if(!match(ctx,r))return false;
		while(match_or_undo(ctx,r) && !ctx.done()){}
		return true;
	});
}
template<parser T>
parser auto any(T&& rhs) {
	return f_parser([r=std::forward<T>(rhs)](context& ctx) mutable {
		while(match_or_undo(ctx,r) && !ctx.done()){}
		return true;
	});
}
template<parser T>
parser auto notf(T&& rhs){
	return f_parser([r=std::forward<T>(rhs)](auto& ctx) mutable {
		return !match(ctx,r);
	});
}
template<parser T>
parser auto optional(T&& rhs) {
	auto ret = std::forward<T>(rhs) | notf(fail);
	return ret;
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
