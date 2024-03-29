#pragma once
#include "ezpz/meta.hpp"
#include "ezpz/context.hpp"
#include <string>

namespace ezpz {
template<typename T>
concept tlist_c = requires(){
	typename T::type;
	typename T::rest;
};
template<typename T>
concept parser_d =
	requires(T t){
		typename T::ezpz_output;
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
	typename std::enable_if_t<!std::same_as<typename P::ezpz_output, TLIST<>>>;
	requires tlist_c<typename P::ezpz_output>;
	/* requires apply_list<rparser_invocability,append_list<TLIST<P>,typename P::ezpz_output>::type>::type; */
	/* typename std::decay_t<P>::ezpz_output; */
	/* typename std::enable_if_t<!std::same_as<typename std::decay_t<P>::ezpz_output, TLIST<EOL>>>; */
};
template<typename P, typename ... RET>
concept RPO_c = parser<P> && std::same_as<typename P::ezpz_output,TLIST<RET...>>;

bool parse(std::string_view s, parser auto&& p, auto&...args){
	basic_context ctx(std::move(s));
	return parse(ctx,std::forward<std::decay_t<decltype(p)>>(p),args...);
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
		return false;
	}
}

template<typename ctx_t, typename p_t, typename ... ARGS>
concept parseable = requires(ctx_t& ctx, p_t& p, ARGS...args){
	{p._parse(ctx,args...)} -> std::same_as<bool>;
};
template<context_c context_t, typename P, typename...ARGS> 
requires parser<std::decay_t<P>>
bool parse(context_t& ctx, P&& p, ARGS&...args) {
	using P_t = std::decay_t<P>;
	static_assert(sizeof...(ARGS) == 0 || sizeof...(ARGS) == P_t::ezpz_output::size,
			"invalid amount of arguments to ezpz::parse");
	static_assert(sizeof...(ARGS) == 0 || std::same_as<typename get_decay_list<TLIST<ARGS...>>::type,typename P_t::ezpz_output>,
			"wrong argument types to ezpz::parse");
	if constexpr(sizeof...(ARGS) == 0 && P_t::ezpz_output::size != 0){
		using hold_t = typename instantiate_list<hold_normal, typename P_t::ezpz_output>::type;
		hold_t hold;
		return hold.apply([&](auto&...args){
			return parse(ctx,p,args...);
		});
	} else if constexpr(!parseable<context_t,P_t,ARGS...> && contains<typename get_prop_tag<context_t>::type,has_inner_ctx>::value){
		return parse(ctx.get_inner_ctx(),p,args...);
	}else{
		/* print_types<context_t> asd; */
		/* print_types<P_t> asd2; */
		/* print_types<ARGS...> asd3; */
		/* print_vals<bool, parseable<context_t,P_t,ARGS...>> asd4; */
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
	
}
