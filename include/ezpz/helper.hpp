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

inline struct fail_p {
	using UNPARSED_LIST = TLIST<>;
	using active = active_f;
	using ezpz_prop = TLIST<always_false>;

	bool _parse(auto&){
		return false;
	}
	
} fail;
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
		sv = std::string_view{ctx.input.data()+start,ctx.pos-start};
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

#include <stdexcept>
struct parse_error : public std::runtime_error {
	using std::runtime_error::runtime_error;
	
	parse_error() noexcept : std::runtime_error("parse error") {}
	parse_error(const parse_error&) noexcept = default;
	parse_error(parse_error&&) noexcept = default;
};

template<parser P1, parser P2>
struct recover_p {
	using UNPARSED_LIST = typename t_if_else<
		std::same_as<typename P1::UNPARSED_LIST, typename P2::UNPARSED_LIST>,
		typename P1::UNPARSED_LIST,
		TLIST<>
	>::type;
	using active = active_f;
	using ezpz_prop = TLIST<always_true>;

	[[no_unique_address]] P1 p1;
	[[no_unique_address]] P2 p2;

	recover_p(auto&& p1, auto&& p2) : p1(std::forward<P1>(p1)), p2(std::forward<P2>(p2)) {}

	bool _parse(auto& ctx,auto&...args) {
		auto start = ctx.getPosition();
		try {
			if(parse(ctx,p1,args...)){
				return true;
			}
		}catch(parse_error& pe){
			//notify context 
		}
		ctx.setPosition(start);
		return parse(ctx,p2,args...);
	}
};
parser auto recover(auto&& p1, auto&& p2){
	using P1_t = std::decay_t<decltype(p1)>;
	using P2_t = std::decay_t<decltype(p2)>;
	return recover_p<P1_t,P2_t>(std::forward<P1_t>(p1),std::forward<P2_t>(p2));
}

template<parser P, typename err_msg>
struct must_p {
	using UNPARSED_LIST = typename P::UNPARSED_LIST;
	using active = typename P::active;
	using ezpz_prop = TLIST<always_true>;

	[[no_unique_address]] P p;
	[[no_unique_address]] err_msg err;
	
	must_p(auto&& p, err_msg&& err) : p(std::forward<P>(p)), err(std::forward<err_msg>(err)) {}

	bool _parse(auto& ctx, auto&&...args){
		if(!parse(ctx,p,args...)){
			err_msg(ctx);
			throw parse_error();
		}
		return true;
	}
};

parser auto must(auto&& p1){
	using P1_t = std::decay_t<decltype(p1)>;
	auto empty_msg = [](auto&){};
	return must_p<P1_t, decltype(empty_msg)>(std::forward<P1_t>(p1),{});
}
parser auto must(auto&& p1, auto&& err_msg){
	using P1_t = std::decay_t<decltype(p1)>;
	using msg_t = std::decay_t<decltype(err_msg)>;
	return must_p<P1_t, msg_t>(std::forward<P1_t>(p1),std::forward<msg_t>(err_msg));
}


