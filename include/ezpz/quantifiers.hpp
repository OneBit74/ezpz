#pragma once
#include "ezpz/core.hpp"
#include "ezpz/parse_object.hpp"
#include "ezpz/helper.hpp"

template<parser T>
parser auto plus(T&& rhs) {
	using TT = std::decay_t<T>;
	return make_rpo([r=std::forward<TT>(rhs)](auto& ctx) mutable {
		if(!parse(ctx,r))return false;
		while(parse(ctx,r)){}
		return true;
	});
}

template<parser P, std::invocable VAL_F, typename AGG_F>
struct agg_any_p {
	using RET = std::invoke_result_t<VAL_F>;
	using UNPARSED_LIST = TLIST<RET>;
	using active = active_t;
	
	[[no_unique_address]] P p;
	[[no_unique_address]] VAL_F v;
	[[no_unique_address]] AGG_F f;

	agg_any_p(auto&& p, auto&& v, auto&& f) :
		p(std::forward<P>(p)),
		v(std::forward<VAL_F>(v)),
		f(std::forward<AGG_F>(f))
	{}

	bool _parse(auto& ctx, RET& ret){
		ret = v();
		using hold_t = typename apply_list<hold_normal,typename P::UNPARSED_LIST>::type;
		while(!ctx.done()){
			hold_t hold;
			bool success = hold.apply([&](auto&...args){
				return parse(ctx,p,args...);
			});
			if(!success)break;

			hold.apply([&](auto&...result){
				using constraint = typename apply_list<
					std::is_invocable_r,
					typename append_list<
						TLIST<RET,AGG_F,RET>,
						typename P::UNPARSED_LIST>::type
				>::type;
				if constexpr (constraint::value){
					ret = f(ret,result...);
				}else{
					f(ret,result...);
				}
			});

		}
		
		return true;
	}
};
template<parser T>
struct any_p {
	using UNPARSED_LIST = TLIST<EOL>;
	using active = active_f;

	[[no_unique_address]] T p;
	any_p(auto&& p) : p(std::forward<T>(p)) {}
	bool _parse(auto& ctx){
		while(parse(ctx,p)){}
		return true;
	}

	template<typename VAL_F, typename AGG_F> 
	auto reduce(VAL_F base, AGG_F comb) {
		using VAL_F_t = std::decay_t<VAL_F>;
		using AGG_F_t = std::decay_t<AGG_F>;
		if constexpr(std::invocable<VAL_F_t>){
			return agg_any_p<T,VAL_F_t,AGG_F_t>{
				std::forward<T>(p),
				std::forward<VAL_F_t>(base),
				std::forward<AGG_F_t>(comb)};
		}else{
			auto hold_f = [base=std::forward<VAL_F_t>(base)](){
				return base;
			};
			using hold_f_t = decltype(hold_f);
			return agg_any_p<T,hold_f_t,AGG_F_t>{
				std::forward<T>(p),
				std::move(hold_f),
				std::forward<AGG_F_t>(comb)};
		}
	}
};
template<parser T>
parser auto any(T&& rhs) {
	using TT = std::decay_t<T>;
	return any_p<TT>{std::forward<TT>(rhs)};
}
template<typename T>
parser auto notf(T&& rhs) requires parser<std::decay_t<T>>{
	using TT = std::decay_t<T>;
	return make_rpo([r=std::forward<TT>(rhs)](auto& ctx) mutable {
		return !parse_or_undo(ctx,r);
	});
}
template<parser T>
parser auto optional(T&& rhs) {
	using TT = std::decay_t<T>;
	auto ret = std::forward<TT>(rhs) | notf(fail);
	return ret;
}

auto times(int amount,auto&& parser){
	return make_rpo([=](auto& ctx) mutable {
			for(int i = 0; i < amount; ++i){
				if(!parse(ctx,parser))return false;
			}
			return true;
		}
	);
}
auto max(int amount,auto&& parser){
	return make_rpo([=](auto& ctx) mutable {
		int counter = 0;
		while(true){
			auto start = ctx.getPosition();
			if(!parse(ctx,parser)){
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
	return make_rpo([=](auto& ctx) mutable {
		int counter = 0;
		while(true){
			auto start = ctx.getPosition();
			if(!parse(ctx,parser)){
				if(counter < amount){
					ctx.setPosition(start);
				}
				return counter >= amount;
			}
			counter++;
		}
	});
}


