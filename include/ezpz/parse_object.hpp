#pragma once
#include "ezpz/meta.hpp"
#include "ezpz/context.hpp"
#include <concepts>
#include <string>
#include <string_view>
#include <memory>
#include <iostream>

template<typename T>
concept tlist_c = requires(){
	typename T::type;
	typename T::rest;
};
template<typename T>
concept parser_d =
	requires(T t){
		typename T::UNPARSED_LIST;
		typename T::active;
		{ 
			t.dbg_inline()
		} -> std::same_as<bool>;
		{ 
			t.name()
		} -> std::same_as<std::string>;
	};
template<typename T>
concept parser = parser_d<std::decay_t<T>>;
template<typename T, typename context_t>
concept parser_for_context = context_c<context_t> && parser<T> &&
	requires(context_t ctx, T t){
		{ 
			t._undo(ctx)
		} -> std::same_as<void>;
		{ 
			t._match(ctx)
		} -> std::same_as<bool>;
	};
template<typename context_t, typename P, typename...ARGS>
concept rparser_invocability = context_c<context_t> &&
	requires(P p,context_t& ctx, ARGS...args){
		{
			p._parse(ctx,args...)
		} -> std::same_as<bool>;
	};
template<typename P>
concept rparser = parser<P> && requires(){
	typename std::enable_if_t<!std::same_as<typename P::UNPARSED_LIST, TLIST<EOL>>>;
	requires tlist_c<typename P::UNPARSED_LIST>;
	/* requires apply_list<rparser_invocability,append_list<TLIST<P>,typename P::UNPARSED_LIST>::type>::type; */
	/* typename std::decay_t<P>::UNPARSED_LIST; */
	/* typename std::decay_t<P>::active; */
	/* typename std::enable_if_t<!std::same_as<typename std::decay_t<P>::UNPARSED_LIST, TLIST<EOL>>>; */
};


template<context_c context_t>
bool match_or_undo(context_t& ctx, parser_for_context<context_t> auto&& p){
	auto start_pos = ctx.getPosition();
	if(match(ctx,p)){
		return true;
	}else{
		ctx.setPosition(start_pos);
		p._undo(ctx);
		return false;
	}
}
template<context_c context_t>
bool match(context_t& ctx, parser_for_context<context_t> auto&& p){
	if constexpr(!std::is_same_v<void,decltype(ctx.notify_enter(p))>){
		auto msg = ctx.notify_enter(p);
		auto ret = p._match(ctx);
		ctx.notify_leave(p,ret,msg);
		return ret;
	}else{
		ctx.notify_enter(p);
		auto ret = p._match(ctx);
		ctx.notify_leave(p,ret);
		return ret;
	}
}
bool match(std::string s, parser_for_context<basic_context> auto&& p){
	basic_context ctx(std::move(s));
	return match(ctx,std::forward<std::decay_t<decltype(p)>>(p));
}
template<typename P, typename context, typename ... ARGS>
concept parser_callable = requires(P p, context ctx, ARGS...args){
	{
		p._parse(ctx,args...)
	} -> std::same_as<bool>;
};
template<context_c context_t, typename P, typename...ARGS> 
requires parser_for_context<std::decay_t<P>, context_t> && parser_callable<P,context_t,ARGS...>
bool parse_or_undo(context_t& ctx, P&& p, ARGS&...args) {
	using P_t = std::decay_t<P>;
	auto start_pos = ctx.getPosition();
	if(parse(ctx,std::forward<P_t>(p), args...)){
		return true;
	}else{
		ctx.setPosition(start_pos);
		p._undo(ctx);
		return false;
	}
}
template<context_c context_t, typename P, typename...ARGS> 
requires parser_for_context<std::decay_t<P>, context_t> && parser_callable<P,context_t,ARGS...>
bool parse(context_t& ctx, P&& p, ARGS&...args) {
	using P_t = std::decay_t<P>;
	if constexpr(!rparser<P_t>){
		return match(ctx,std::forward<P_t>(p));
	}else{
		if constexpr(!std::is_same_v<void,decltype(ctx.notify_enter(p))>){
			auto msg = ctx.notify_enter(p);
			auto ret = p._parse(ctx,args...);
			ctx.notify_leave(p,ret,msg);
			return ret;
		}else{
			ctx.notify_enter(p);
			auto ret = p._parse(ctx,args...);
			ctx.notify_leave(p,ret);
			return ret;
		}
	}
}
class parse_object {
public:
	using active = active_f;
	using UNPARSED_LIST = TLIST<EOL>;

	constexpr bool _match(auto&){return true;};
	constexpr bool _parse(auto&){return true;};
	constexpr void _undo(auto&) {};
	inline std::string name() const {return "";}
	constexpr bool dbg_inline() const {return false;}
};

template<typename F_TYPE> 
class f_parser_t : public parse_object {
public:
	F_TYPE f;

	f_parser_t(F_TYPE& of) : f(of) {}
	f_parser_t(F_TYPE&& of) : f(std::move(of)) {}
	bool _match(auto& ctx) {
		return f(ctx);
	}
	bool dbg_inline() const {
		return false;
	}
};
template<typename F_TYPE>
auto f_parser(F_TYPE&& f){
	auto ret = f_parser_t<F_TYPE>{std::forward<F_TYPE>(f)};
	return ret;
}
template<parser T>
struct dont_store_empty {
	T parser;
	dont_store_empty(auto&& parser) : parser(std::forward<T>(parser)) {}

	T& get(){
		return parser;
	}
};

template<parser T> requires std::is_empty_v<T>  
struct dont_store_empty<T>{
	dont_store_empty(auto&&) {}
	T& get(){
		return *(T*)(1);
	}
};

template<parser T1, parser T2>
struct bi_comb : public parse_object {
	[[no_unique_address]] dont_store_empty<T1> t1;
	[[no_unique_address]] dont_store_empty<T2> t2;
	bi_comb(auto&& p1, auto&& p2) : t1(std::forward<T1>(p1)), t2(std::forward<T2>(p2)) {}
};
template<parser T1, parser T2>
struct simple_or_parser : public bi_comb<T1,T2> {
	using parent_t = bi_comb<T1,T2>;
	bool _match(auto& ctx) {
		return match_or_undo(ctx,parent_t::t1.get()) || match(ctx,parent_t::t2.get());
	};
	bool dbg_inline(){
		return true;
	}
};
template<parser T1, parser T2>
struct simple_and_parser : public bi_comb<T1,T2> {
	using parent_t = bi_comb<T1,T2>;
	bool _match(auto& ctx) {
		return match(ctx,parent_t::t1.get()) && match(ctx,parent_t::t2.get());
	};
	bool dbg_inline(){
		return true;
	}
};
template<parser T1, parser T2>
parser auto operator+(T1&& lhs, T2&& rhs){
	using T1_t = std::decay_t<T1>;
	using T2_t = std::decay_t<T2>;
	using p_type = simple_and_parser<T1_t,T2_t>;
	return p_type{bi_comb<T1_t,T2_t>{std::forward<T1_t>(lhs), std::forward<T2_t>(rhs)}};
}
template<parser T1, parser T2>
parser auto operator|(T1&& lhs, T2&& rhs){
	using T1_t = std::decay_t<T1>;
	using T2_t = std::decay_t<T2>;
	using p_type = simple_or_parser<T1_t,T2_t>;
	return p_type{bi_comb<T1_t,T2_t>{std::forward<T1_t>(lhs), std::forward<T2_t>(rhs)}};
}
