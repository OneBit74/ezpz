#pragma once
#include "ezpz/parse_object.hpp"
#include <iostream>

struct print_p {
	using active = active_f;
	using UNPARSED_LIST = TLIST<EOL>;

	std::string_view text;
	bool _parse(auto&){
		std::cout << text << std::endl;
		return true;
	}
};
parser auto print(std::string_view text){
	return print_p{text};
};

inline parser auto fail = make_rpo([](auto&){return false;});
inline struct eoi_p {
	using active = active_f;
	using UNPARSED_LIST = TLIST<EOL>;

	bool _parse(auto& ctx){
		return ctx.done();
	}
} eoi;


template<parser P>
struct capture_p {
	using active = active_f;
	using UNPARSED_LIST = TLIST<std::string_view>;

	P parent;
	capture_p(P&& op) : parent(std::move(op)) {}
	capture_p(const P& op) : parent(op) {}
	bool _parse(auto& ctx, std::string_view& sv){
		auto start = ctx.pos;
		auto ret = parse(ctx,parent);
		sv = std::string_view{ctx.input.c_str()+start,ctx.pos-start};
		return ret;
	}
	bool dbg_inline(){
		return true;
	}
};
template<parser T>
rparser auto capture(T&& p){
	using P = std::decay_t<T>;
	return capture_p<P>(std::forward<P>(p));
}
