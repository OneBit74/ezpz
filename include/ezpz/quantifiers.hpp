#pragma once
#include "ezpz/core.hpp"
#include "ezpz/parse_object.hpp"
#include "ezpz/helper.hpp"

namespace ezpz{

template<parser T>
parser auto plus(T&& rhs) {
	using TT = std::decay_t<T>;
	return make_rpo([r=std::forward<TT>(rhs)](auto& ctx) mutable {
		if(!parse(ctx,r))return false;
		while(parse_or_undo(ctx,r)){}
		return true;
	});
}

template<parser P, std::invocable VAL_F, typename AGG_F>
struct agg_any_p {
	using RET = std::invoke_result_t<VAL_F>;
	using UNPARSED_LIST = TLIST<RET>;
	
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
struct empty_aggregator {
	inline void operator()(){
	};
};
template<parser T>
struct any_p {
	using UNPARSED_LIST = TLIST<>;
	using INNER_RET = typename T::UNPARSED_LIST;
	using ezpz_prop = TLIST<always_true>;

	static_assert(!get_prop_tag<T>::type::template contains<always_true>,
			"[ezpz][any_p] inner parser never fails."
			" This is equivalent to a while true statement");

	[[no_unique_address]] T p;
	any_p(auto&& p) : p(std::forward<T>(p)) {}

	bool _parse(auto& ctx){
		while(parse_or_undo(ctx,p)){}
		return true;
	}
	bool _aggregate(auto& ctx, auto&& agg){
		while(parse_or_undo(ctx,p*agg)){}
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
template<typename P>
struct not_p {
	using UNPARSED_LIST = TLIST<>;
	using ezpz_prop = typename t_if_else<
		contains<typename get_prop_tag<P>::type, always_true>::value,
		TLIST<always_false>,
		typename t_if_else<
			contains<typename get_prop_tag<P>::type, always_false>::value,
			TLIST<always_true>,
			TLIST<>
		>::type
	>::type;
				
	[[no_unique_address]] P p;

	not_p(auto&& p) : p(std::forward<P>(p)) {}

	bool _parse(auto& ctx){
		return !parse_or_undo(ctx,p);
	}
};
template<typename T>
parser auto notf(T&& rhs) requires parser<std::decay_t<T>>{
	using TT = std::decay_t<T>;
	return not_p<TT>(std::forward<TT>(rhs));
}
/* template<typename P> */
/* struct optional_p { */
/* 	using UNPARSED_LIST = typename t_if_else< */
/* 		  std::same_as<typename P::UNPARSED_LIST,TLIST<>>, */
/* 		  TLIST<>, */
/* 		  apply_list<std::optional,P::UNPARSED_LIST>::type */
/* 	>::type; */
/* 	using ezpz_prop = TLIST<always_true>; */

/* 	[[no_unique_address]] P p; */

/* 	optional_p(auto&& p) : p(std::forward<P>(p)) {} */

/* 	bool _parse(auto& ctx) */
/* }; */
template<parser T>
parser auto optional(T&& rhs) {
	using TT = std::decay_t<T>;
	auto ret = std::forward<TT>(rhs) | notf(fail);
	return ret;
}
template<parser T>
struct peek_p {
	using UNPARSED_LIST = typename T::UNPARSED_LIST; 
	using ezpz_prop = typename get_prop_tag<T>::type;

	T p;
	peek_p(auto&& p) : p(std::forward<T>(p)) {}

	bool _parse(auto& ctx, auto&...ARGS){
		auto prev_pos = ctx.getPosition();
		auto ret = parse(ctx,p,ARGS...);
		ctx.setPosition(prev_pos);
		return ret;
	}
};
template<parser T>
parser auto peek(T&& rhs) {
	using TT = std::decay_t<T>;
	return peek_p<TT>(std::forward<TT>(rhs));
}

struct dynamic_int_holder {
	int val;
	int operator()(){
		return val;
	}
};
template<int val>
struct constant_int_holder {
	int operator()(){
		return val;
	}
};

auto times(int amount,auto&& parser){
	return make_rpo([=](auto& ctx) mutable {
			for(int i = 0; i < amount; ++i){
				if(!parse(ctx,parser))return false;
			}
			return true;
		}
	);
}
template<parser P, typename count_f_t>
struct max_p_impl {
	using UNPARSED_LIST = TLIST<>;
	using INNER_RET = typename P::UNPARSED_LIST;

	[[no_unique_address]] P p;
	[[no_unique_address]] count_f_t count_f;

	max_p_impl(auto&& p, auto&& count_f) : p(std::forward<P>(p)), count_f(std::forward<count_f_t>(count_f))
	{}
	bool _parse(auto& ctx){
		return _aggregate(ctx,[](auto&&...){});
	}
	bool _aggregate(auto& ctx, auto&& agg){
		int counter = 0;
		while(true){
			auto start = ctx.getPosition();
			if(!parse(ctx,p*agg)){
				return true;
			}else if (counter >= count_f()) {
				ctx.setPosition(start);
				return false;
			}
			counter++;
		}
	}
};
template<parser P, int count>
using max_p_base_helper = typename t_if_else<
		count==-1,
		max_p_impl<P,dynamic_int_holder>,
		max_p_impl<P,constant_int_holder<count>>
	>::type;
 
template<parser P, int count = -1>
struct max_p : public max_p_base_helper<P,count>{
	using base = max_p_base_helper<P,count>;

	static_assert(!get_prop_tag<P>::type::template contains<always_true>,
			"[ezpz][any_p] inner parser never fails."
			" This is equivalent to a while true statement");

	max_p(auto&& p) : base(std::forward<P>(p),constant_int_holder<count>{}) {
		static_assert(count != -1);
	}
	max_p(auto&& p, int c) : base(std::forward<P>(p),dynamic_int_holder{c}) {
		static_assert(count == -1);
	}
};

template<int amount>
auto max(auto&& parser){
	using P_t = std::decay_t<decltype(parser)>;
	return max_p<P_t,amount>(std::forward<P_t>(parser));
}
auto max(int amount,auto&& parser){
	using P_t = std::decay_t<decltype(parser)>;
	return max_p<P_t,-1>(std::forward<P_t>(parser),amount);
}
template<parser P, typename count_f_t>
struct min_p_impl {
	using UNPARSED_LIST = TLIST<>;
	using INNER_RET = typename P::UNPARSED_LIST;

	[[no_unique_address]] P p;
	[[no_unique_address]] count_f_t count_f;

	min_p_impl(auto&& p, auto&& count_f) : p(std::forward<P>(p)), count_f(std::forward<count_f_t>(count_f))
	{}
	
	bool _parse(auto& ctx){
		return _aggregate(ctx,[](auto&&...){});
	}
	bool _aggregate(auto& ctx, auto&& agg){
		int counter = 0;
		while(true){
			auto start = ctx.getPosition();
			if(!parse(ctx,p*agg)){
				if(counter < count_f()){
					ctx.setPosition(start);
				}
				return counter >= count_f();
			}
			counter++;
		}
	}
};
template<parser P, int count>
using min_p_base_helper = typename t_if_else<
		count==-1,
		min_p_impl<P,dynamic_int_holder>,
		min_p_impl<P,constant_int_holder<count>>
	>::type;
 
template<parser P, int count = -1>
struct min_p : public min_p_base_helper<P,count>{
	using base = min_p_base_helper<P,count>;
	min_p(auto&& p) : base(std::forward<P>(p),constant_int_holder<count>{}) {
		static_assert(count != -1);
	}
	min_p(auto&& p, int c) : base(std::forward<P>(p),dynamic_int_holder{c}) {
		static_assert(count == -1);
	}
};
template<int count>
parser auto min(parser auto&& parser){
	using P_t = std::decay_t<decltype(parser)>;
	return min_p<P_t,count>(std::forward<P_t>(parser));
}
parser auto min(int count, parser auto&& parser){
	using P_t = std::decay_t<decltype(parser)>;
	return min_p<P_t,-1>(std::forward<P_t>(parser),count);
}


template<typename P, typename V, typename A>
struct reduce_p {
	using RET = std::invoke_result_t<V>;
	using UNPARSED_LIST = TLIST<RET>;
	using ezpz_prop = typename get_prop_tag<P>::type;

	static_assert(!std::same_as<TLIST<>,typename P::INNER_RET>, "[ezpz][reduce_p] no return values available to aggregate over");

	[[no_unique_address]] P p;
	[[no_unique_address]] V v;
	[[no_unique_address]] A a;

	reduce_p(auto&& p, auto&& v, auto&& a) :
		p(std::forward<P>(p)),
		v(std::forward<V>(v)),
		a(std::forward<A>(a))
	{}

	bool _parse(auto& ctx, RET& ret){
		ret = v();
		return p._aggregate(ctx,
				[&]<typename...ARGS> 
				requires std::same_as<typename get_decay_list<TLIST<ARGS...>>::type,typename P::INNER_RET>
				(ARGS&&...args){
			using constraint = typename apply_list<
				std::is_invocable_r,
				TLIST<RET,A,RET,ARGS&&...>
			>::type;
			if constexpr (constraint::value){
				ret = a(ret,std::move(args)...);
			}else{
				//TODO check invocable
				a(ret,std::move(args)...);
			}
		});
	}
};
auto reduce(parser auto&& aggregatable_parser, std::invocable auto&& val_f, auto&& comb_f){
	using P_t = std::decay_t<decltype(aggregatable_parser)>;
	using V_t = std::decay_t<decltype(val_f)>;
	using A_t = std::decay_t<decltype(comb_f)>;

	return reduce_p<P_t,V_t,A_t>{
		std::forward<P_t>(aggregatable_parser),
		std::forward<V_t>(val_f),
		std::forward<A_t>(comb_f)
	};
}

}
