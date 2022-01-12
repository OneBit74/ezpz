#pragma once
#include "ezpz/core.hpp"
#include "ezpz/parse_object.hpp"
#include "ezpz/helper.hpp"

template<parser T>
parser auto plus(T&& rhs) {
	using TT = std::decay_t<T>;
	return make_rpo([r=std::forward<TT>(rhs)](auto& ctx) mutable {
		if(!parse(ctx,r))return false;
		while(parse_or_undo(ctx,r) && !ctx.done()){}
		return true;
	});
}

template<parser P, typename RET, typename F>
struct agg_any_p {
	using UNPARSED_LIST = TLIST<RET>;
	using active = active_t;
	
	P p;
	RET v;
	F f;

	agg_any_p(auto&& p, auto&& v, auto&& f) :
		p(std::forward<P>(p)),
		v(std::forward<RET>(v)),
		f(std::forward<F>(f))
	{}

	bool _parse(auto& ctx, RET& ret){
		ret = v;
		using hold_t = typename apply_list<hold_normal,typename P::UNPARSED_LIST>::type;
		while(!ctx.done()){
			hold_t hold;
			bool success = hold.apply([&](auto&...args){
				return parse_or_undo(ctx,p,args...);
			});
			if(!success)break;

			hold.apply([&](auto&...result){
				using constraint = typename apply_list<
					std::is_invocable_r,
					typename append_list<
						TLIST<RET,F,RET>,
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

	T p;
	any_p(auto&& p) : p(std::forward<T>(p)) {}
	bool _parse(auto& ctx){
		while(parse_or_undo(ctx,p) && !ctx.done()){}
		return true;
	}

	template<typename V, typename F> 
	auto reduce(V base, F comb){
		return agg_any_p<T,V,F>{p,base,comb};
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


