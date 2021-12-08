#pragma once
#include "meta.hpp"
#include "context.hpp"
#include <concepts>
#include <string>
#include <string_view>
#include <memory>

template<typename T>
concept tlist_c = requires(){
	typename T::type;
	typename T::rest;
};
template<typename T>
concept parser = requires(T t, context& ctx){
	{ 
		t.dbg_inline()
	} -> std::same_as<bool>;
	{ 
		t.name()
	} -> std::same_as<std::string>;
	{ 
		t._undo(ctx)
	} -> std::same_as<void>;
	{ 
		t._match(ctx)
	} -> std::same_as<bool>;
};
template<typename P, typename...ARGS>
concept rparser_invocability = requires(P p,context& ctx, ARGS...args){
	{
		p._parse(ctx,args...)
	} -> std::same_as<bool>;
};
template<typename P>
concept rparser = parser<P> && requires(){
	typename P::UNPARSED_LIST;
	typename P::active;
	typename std::enable_if_t<!std::same_as<typename P::UNPARSED_LIST, TLIST<EOL>>>;
	requires tlist_c<typename P::UNPARSED_LIST>;
	/* requires apply_list<rparser_invocability,append_list<TLIST<P>,typename P::UNPARSED_LIST>::type>::type; */
	/* typename std::decay_t<P>::UNPARSED_LIST; */
	/* typename std::decay_t<P>::active; */
	/* typename std::enable_if_t<!std::same_as<typename std::decay_t<P>::UNPARSED_LIST, TLIST<EOL>>>; */
};

void dbg_log_enter(context&, std::string_view comment);
void dbg_log_leave(context& ctx,bool success, size_t prev_pos);

bool match_or_undo(context& ctx, parser auto&& p){
	size_t start_match = ctx.pos;
	if(match(ctx,p)){
		return true;
	}else{
		ctx.pos = start_match;
		p._undo(ctx);
		return false;
	}
}
bool match(context& ctx, parser auto&& p){
	if(ctx.debug && !p.dbg_inline()){
		dbg_log_enter(ctx,p.name());
		auto prev_pos = ctx.pos;
		auto ret = p._match(ctx);
		dbg_log_leave(ctx,ret,prev_pos);
		return ret;
	}else{
		return p._match(ctx);
	}
}
bool match(std::string s, parser auto&& p){
	context ctx(std::move(s));
	return match(ctx,std::forward<std::decay_t<decltype(p)>>(p));
}
template<typename P> requires parser<std::decay_t<P>>
bool parse_or_undo(context& ctx, P&& p, auto&...args){
	using P_t = std::decay_t<P>;
	auto start_pos = ctx.pos;
	if(parse(ctx,std::forward<P_t>(p), args...)){
		return true;
	}else{
		ctx.pos = start_pos;
		p._undo(ctx);
		return false;
	}
}
template<typename P> requires parser<std::decay_t<P>>
bool parse(context& ctx, P&& p, auto&...args){
	if(ctx.debug && !p.dbg_inline()){
		dbg_log_enter(ctx,p.name());
		auto prev_pos = ctx.pos;
		auto ret = p._parse(ctx,args...);
		dbg_log_leave(ctx,ret,prev_pos);
		return ret;
	}else{
		return p._parse(ctx,args...);
	}
}

class parse_object {
public:
	using active = active_f;
	using UNPARSED_LIST = TLIST<EOL>;

	constexpr bool _match(context&){return true;};
	constexpr void _undo(context&) {};
	inline std::string name() const {return "";}
	constexpr bool dbg_inline() const {return false;}
};
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
template<typename F_TYPE> requires std::invocable<F_TYPE,context&>
class f_parser_t : public parse_object {
public:
	F_TYPE f;

	f_parser_t(F_TYPE& of) : f(of) {}
	f_parser_t(F_TYPE&& of) : f(std::move(of)) {}
	bool _match(context& ctx) {
		return f(ctx);
	}
	bool dbg_inline() const {
		return true;
	}
};
template<typename F_TYPE> requires std::invocable<F_TYPE,context&>
parser auto f_parser(F_TYPE&& f){
	auto ret = f_parser_t<F_TYPE>{std::forward<F_TYPE>(f)};
	return ret;
}
template<parser T1, parser T2>
struct bi_comb : public parse_object {
	T1 t1;
	T2 t2;
	bi_comb(auto&& p1, auto&& p2) : t1(std::forward<T1>(p1)), t2(std::forward<T2>(p2)) {}
};
template<parser T1, parser T2>
struct simple_or_parser : public bi_comb<T1,T2> {
	using parent_t = bi_comb<T1,T2>;
	bool _match(context& ctx) {
		return match_or_undo(ctx,parent_t::t1) || match(ctx,parent_t::t2);
	};
	bool dbg_inline(){
		return true;
	}
};
template<parser T1, parser T2>
struct simple_and_parser : public bi_comb<T1,T2> {
	using parent_t = bi_comb<T1,T2>;
	bool _match(context& ctx) {
		return match(ctx,parent_t::t1) && match(ctx,parent_t::t2);
	};
	bool dbg_inline(){
		return true;
	}
};
template<parser T1, parser T2>
parser auto operator+(T1&& lhs, T2&& rhs){
	using p_type = simple_and_parser<T1,T2>;
	return p_type{bi_comb<T1,T2>{std::forward<T1>(lhs), std::forward<T2>(rhs)}};
}
template<parser T1, parser T2>
parser auto operator|(T1&& lhs, T2&& rhs){
	using p_type = simple_or_parser<T1,T2>;
	return p_type{bi_comb<T1,T2>{std::forward<T1>(lhs), std::forward<T2>(rhs)}};
}
