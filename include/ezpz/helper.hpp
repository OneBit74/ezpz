#pragma once
#include "ezpz/parse_object.hpp"
#include <iostream>

inline struct print_t {
	static void print_text(std::string_view sv);
	parser auto  operator()(std::string_view text){
		auto ret =  make_rpo([=](auto){
					std::cout << text << std::endl;
					return true;
				});
		return ret;
	};
} print;
inline parser auto fail = make_rpo([](auto&){return false;});
inline struct eoi_t {
	using active = active_f;
	using UNPARSED_LIST = TLIST<EOL>;

	bool _parse(auto& ctx){
		return ctx.done();
	}
} eoi;

auto copy(auto&& val){
	auto cp = val;
	return cp;
}

template<parser parser>
class parse_object_ref {
public:
	using active = typename parser::active;
	using UNPARSED_LIST = typename parser::UNPARSED_LIST;

	parser& p;
	parse_object_ref(parser& op) : p(op) {}

	bool _parse(auto& ctx) {
		return p._parse(ctx);
	}
	void _undo(auto& ctx) {
		undo(ctx,p);
	}
	bool _parse(auto& ctx, auto&...args) requires rparser<parser>{
		return parse(ctx,p,args...);
	}
	bool dbg_inline(){
		return true;
	}
};

parser auto ref(auto& p){
	using inner = std::decay_t<decltype(p)>;
	return parse_object_ref<inner>{p};
}

template<parser P>
struct capture_t {
	using active = active_f;
	using UNPARSED_LIST = TLIST<std::string_view>;

	P parent;
	capture_t(P&& op) : parent(std::move(op)) {}
	capture_t(const P& op) : parent(op) {}
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
	return capture_t<P>(std::forward<P>(p));
}
