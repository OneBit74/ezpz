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

template<parser T>
struct any_p {
	using ezpz_output = typename T::ezpz_output;
	using ezpz_prop = TLIST<always_true>;

	static_assert(!get_prop_tag<T>::type::template contains<always_true>,
			"[ezpz][any_p] inner parser never fails. "
			"This is equivalent to a while true statement");

	[[no_unique_address]] T p;
	any_p(auto&& p) : p(std::forward<T>(p)) {}

	bool _parse(auto& ctx, auto&...ARGS){
		while(parse_or_undo(ctx,p,ARGS...)){}
		return true;
	}
};
template<parser T>
parser auto any(T&& rhs) {
	using TT = std::decay_t<T>;
	return any_p<TT>{std::forward<TT>(rhs)};
}
template<typename P>
struct not_p {
	using ezpz_output = TLIST<>;
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
template<typename P>
struct optional_p {
	using ezpz_output = typename t_if_else<
		P::ezpz_output::size == 0,
		TLIST<>,
		typename t_if_else<
			P::ezpz_output::size == 1,
			TLIST<typename instantiate_if<P::ezpz_output::size == 1,std::optional,typename P::ezpz_output::type>::type>,
			TLIST<std::optional<typename apply_list<std::tuple, typename P::ezpz_output>::type>>
		>::type
	>::type;
	using ezpz_prop = TLIST<always_true>;

	[[no_unique_address]] P p;

	optional_p(auto&& p) : p(std::forward<P>(p)) {}

	bool _parse(auto& ctx, auto&...ARGS){
		if constexpr(sizeof...(ARGS) == 0){
			parse(ctx, p);
		} else {
			using hold_type = typename instantiate_list<hold_normal,typename P::ezpz_output>::type;
			hold_type hold;
			hold.apply([&](auto&...args){
				parse(ctx, p, args...);
				if constexpr( sizeof...(args) == 1 ){
					assign_first(std::optional{args...},ARGS...);
				} else {
					assign_first(std::optional{std::tuple{args...}},ARGS...);
				}
			});
		}
		return true;
	}
};
template<parser T>
parser auto optional(T&& p) {
	using TT = std::decay_t<T>;
	return optional_p<TT>{std::forward<TT>(p)};
}
template<parser T>
struct peek_p {
	using ezpz_output = typename T::ezpz_output; 
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

template<size_t amount, parser P>
struct timesc_p {
	using ezpz_output = typename P::ezpz_output;
	using ezpz_prop = typename get_prop_tag<P>::type;

	[[no_unique_address]] P p;

	bool _parse(auto& ctx, auto&...args){
		for(size_t i = 0; i < amount; ++i){
			if(!parse(ctx,p,args...))return false;
		}
		return true;
	}
};
template<parser P>
struct timesd_p {
	using ezpz_output = typename P::ezpz_output;
	using ezpz_prop = typename get_prop_tag<P>::type;

	size_t count = 0;
	[[no_unique_address]] P p;

	bool _parse(auto& ctx, auto&...args){
		for(size_t i = 0; i < count; ++i){
			if(!parse(ctx,p,args...))return false;
		}
		return true;
	}
};
template<size_t amount, typename P>
auto times(P&& parser){
	using P_t = std::decay_t<P>;
	return timesc_p<amount,P_t>(std::forward<P_t>(parser));
}
template<typename P>
auto times(int amount,P&& parser){
	using P_t = std::decay_t<P>;
	return timesd_p<P_t>(amount,std::forward<P_t>(parser));
}
template<parser P, typename count_f_t>
struct max_p_impl {
	using ezpz_output = TLIST<>;
	using INNER_RET = typename P::ezpz_output;

	[[no_unique_address]] P p;
	[[no_unique_address]] count_f_t count_f;

	max_p_impl(auto&& p, auto&& count_f) : p(std::forward<P>(p)), count_f(std::forward<count_f_t>(count_f))
	{}
	bool _parse(auto& ctx){
		int counter = 0;
		while(true){
			auto start = ctx.getPosition();
			if(!parse(ctx,p)){
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
	using ezpz_output = TLIST<>;
	using INNER_RET = typename P::ezpz_output;

	[[no_unique_address]] P p;
	[[no_unique_address]] count_f_t count_f;

	min_p_impl(auto&& p, auto&& count_f) : p(std::forward<P>(p)), count_f(std::forward<count_f_t>(count_f))
	{}
	
	bool _parse(auto& ctx){
		int counter = 0;
		while(true){
			auto start = ctx.getPosition();
			if(!parse(ctx,p)){
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


}
