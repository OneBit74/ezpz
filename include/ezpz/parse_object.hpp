#pragma once
#include "ezpz/meta.hpp"
#include "ezpz/context.hpp"
#include <string>

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
	};
template<typename T>
concept parser = parser_d<std::decay_t<T>>;
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

void undo(context_c auto& ctx, parser auto& p){
	if constexpr( requires(decltype(p) p, decltype(ctx) ctx){p._undo(ctx);} ) {
		p._undo(ctx);
	}
}

bool parse(std::string s, parser auto&& p){
	basic_context ctx(std::move(s));
	return parse(ctx,std::forward<std::decay_t<decltype(p)>>(p));
}
template<context_c context_t, typename P, typename...ARGS> 
requires parser<std::decay_t<P>> 
bool parse_or_undo(context_t& ctx, P&& p, ARGS&...args) {
	using P_t = std::decay_t<P>;
	auto start_pos = ctx.getPosition();
	if(parse(ctx,std::forward<P_t>(p), args...)){
		return true;
	}else{
		ctx.setPosition(start_pos);
		undo(ctx,p);
		return false;
	}
}
template<context_c context_t, typename P, typename...ARGS> 
requires parser<std::decay_t<P>>
bool parse(context_t& ctx, P&& p, ARGS&...args) {
	using P_t = std::decay_t<P>;
	static_assert(sizeof...(ARGS) == 0 || sizeof...(ARGS) == list_size<typename P_t::UNPARSED_LIST>::value,
			"invalid amount of arguments to ezpz::parse");
	static_assert(sizeof...(ARGS) == 0 || std::same_as<typename get_decay_list<TLIST<ARGS...>>::type,typename P_t::UNPARSED_LIST>,
			"wrong argument types to ezpz::parse");
	if constexpr(sizeof...(ARGS) == 0 && list_size<typename P_t::UNPARSED_LIST>::value != 0){
		using hold_t = typename instantiate_list<hold_normal, typename P_t::UNPARSED_LIST>::type;
		hold_t hold;
		return hold.apply([&](auto&...args){
			return parse(ctx,p,args...);
		});
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

template<typename F_TYPE> 
struct f_parser_t {
	using active = active_f;
	using UNPARSED_LIST = TLIST<EOL>;

	F_TYPE f;

	f_parser_t(F_TYPE& of) : f(of) {}
	f_parser_t(F_TYPE&& of) : f(std::move(of)) {}
	bool _parse(auto& ctx) {
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
struct bi_comb {
	using active = active_f;
	using UNPARSED_LIST = TLIST<EOL>;
	[[no_unique_address]] dont_store_empty<T1> t1;
	[[no_unique_address]] dont_store_empty<T2> t2;
	bi_comb(auto&& p1, auto&& p2) : t1(std::forward<T1>(p1)), t2(std::forward<T2>(p2)) {}
};
template<parser T1, parser T2>
struct simple_or_parser : public bi_comb<T1,T2> {
	using active = active_f;
	using UNPARSED_LIST = TLIST<EOL>;
	using parent_t = bi_comb<T1,T2>;
	bool _parse(auto& ctx) {
		return parse_or_undo(ctx,parent_t::t1.get()) || parse(ctx,parent_t::t2.get());
	};
	bool dbg_inline(){
		return true;
	}
};
template<parser T1, parser T2>
struct simple_and_parser : public bi_comb<T1,T2> {
	using active = active_f;
	using UNPARSED_LIST = TLIST<EOL>;
	using parent_t = bi_comb<T1,T2>;
	bool _parse(auto& ctx) {
		return parse(ctx,parent_t::t1.get()) && parse(ctx,parent_t::t2.get());
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
