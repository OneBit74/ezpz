#pragma once
#include "parse_object.hpp"

inline struct print_t {
	static void print_text(std::string_view sv);
	parser auto  operator()(std::string_view text){
		auto ret =  f_parser([=](auto){
					print_text(text);
					return true;
				});
		return ret;
	};
} print;
inline parser auto fail = f_parser([](auto){return false;});
inline parser auto eoi = f_parser([](context& ctx){return ctx.done();});

auto copy(auto&& val){
	auto cp = val;
	return cp;
}

template<typename parser>
class parse_object_ref : public parse_object {
public:
	using active = typename parser::active;
	using UNPARSED_LIST = typename parser::UNPARSED_LIST;

	parser& p;
	parse_object_ref(parser& op) : p(op) {}

	bool _match(context& ctx) {
		return p._match(ctx);
	}
	void _undo(context& ctx) {
		p._undo(ctx);
	}
	bool _parse(context& ctx, auto&...args) requires rparser<parser>{
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
struct capture_t : public parse_object {
	using UNPARSED_LIST = TLIST<std::string_view>;
	using active_t = active_f;

	P parent;
	capture_t(P&& op) : parent(std::move(op)) {}
	capture_t(const P& op) : parent(op) {}
	bool _parse(context& ctx, std::string_view& sv){
		auto start = ctx.pos;
		auto ret = match(ctx,parent);
		sv = std::string_view{ctx.input.c_str()+start,ctx.pos-start};
		return ret;
	}
	bool _match(context& ctx){
		return match(ctx,parent);
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
