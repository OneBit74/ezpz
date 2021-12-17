#pragma once
#include "ezpz/parse_object.hpp"
#include "ezpz/meta.hpp"
#include <variant>
#include <optional>
#include <tuple>


template<typename F, typename _RET, typename _ARGS>
struct f_wrapper : public F {
	using RET = _RET;
	using ARGS = _ARGS;
	using self_t = F;
	f_wrapper(F&& f) : F(std::forward<F>(f)) {};
	f_wrapper(F& f) : F(std::forward<F>(f)) {};
};

template<typename T1, typename...REST>
auto assign_first(T1&& src, T1& dst, REST&&...){
	dst = std::forward<T1>(src);
}
template<typename T1, typename...REST>
auto assign_last(T1&& src, REST&&..., T1& dst){
	dst = std::forward<T1>(src);
}

template<typename unp = VOID, typename REM = VOID, typename ... UNPARSED>
struct nr_parser {
	using UNPARSED_LIST = TLIST<UNPARSED...>;
	using active = active_f;

	[[no_unique_address]] REM parent;
	[[no_unique_address]] unp f;

	nr_parser(auto&& f, auto&& parent) :
		parent(std::forward<REM>(parent)),
		f(std::forward<unp>(f))
	{};
	void _undo(auto& ctx){
		if constexpr ( !std::is_same_v<REM,VOID> ) {
			undo(ctx,parent);
		}
	}
	bool dbg_inline() {
		return true;
	}
	bool _parse(auto& ctx, UNPARSED&...up_args){
		if constexpr(std::is_same_v<REM,VOID>){
			return true;
		}else{
			if constexpr( std::is_same_v<unp,VOID> ) {
				return parse(ctx,parent,up_args...);
			/* }else if constexpr(list_size<typename unp::ARGS>::value == 0 && std::is_same_v<typename unp::RET,void>){ */
			/* 	auto ret = parse(ctx,parent,up_args...); */
			/* 	if(ret)f(); */
			/* 	return ret; */
			}else if constexpr (!std::is_same_v<typename unp::RET,void>){
				using hold_args = typename get_decay_list<
					typename reverse_list<
						typename unp::ARGS>::type>::type;
				using hold_type = typename instantiate_list<hold,hold_args>::type;
				hold_type hold;

				bool success = hold.apply_not_first([&](auto&...args){
					return parse(ctx,parent,args...);
				},up_args...);
				if(!success)return false;

				assign_first(hold.apply(f),up_args...);
				return true;
			}else{
				using hold_args = typename get_decay_list<typename reverse_list<typename unp::ARGS>::type>::type;
				using hold_type = typename instantiate_list<hold,hold_args>::type;
				hold_type hold;
				bool success = hold.apply([&](auto&...args){
					return parse(ctx,parent,args...);
				},up_args...);
				if(!success)return false;

				hold.apply(f);

				return true;
			}
		}
	}
};

template<typename...ARGS>
struct takefront_and_call{
	static auto call(auto&& target, ARGS&&...args, auto&&...){
		return target(args...);
	}
};

template<int how_many>
auto takeback_and_call(auto&& target, auto&&...args){
	static_assert(sizeof...(args) >= how_many, "not enough arguments to parse");
	if constexpr(how_many == 0){
		return target();
	}else{
		return takeback_and_call_hlp<how_many>(target,args...);
	}
}
template<int how_many>
auto takeback_and_call_hlp(auto&& target, auto&& first, auto&&...args){
	if constexpr(1+sizeof...(args) != how_many){
		return takeback_and_call_hlp<how_many>(target,args...);
	}else{
		return target(first,args...);
	}
}

template<parser LHS, parser RHS>
struct and_p {
	using UNPARSED_LIST = typename append_list<typename LHS::UNPARSED_LIST, typename RHS::UNPARSED_LIST>::type;
	using active = active_t;
	[[no_unique_address]] dont_store_empty<LHS> lhs;
	[[no_unique_address]] dont_store_empty<RHS> rhs;
	and_p(auto&& p1, auto&& p2) :
		lhs(std::forward<LHS>(p1)),
		rhs(std::forward<RHS>(p2))
	{}
	
	template<typename...ARGS>
	bool _parse(auto& ctx, ARGS&...args){
		using L = TLIST<ARGS...>;
		auto cb_lhs = [&](auto&...few_args){
				return parse(ctx,lhs.get(),few_args...);
		};
		auto cb_rhs = [&](auto&...few_args){
				return parse(ctx,rhs.get(),few_args...);
		};
		using front_t = typename instantiate_list<takefront_and_call, typename get_ref_list<typename LHS::UNPARSED_LIST>::type>::type;
		auto ret = front_t::call(cb_lhs,args...);
		/* auto ret = takefront_and_call<list_size<typename LHS::UNPARSED_LIST>::value>(cb_lhs,args...); */
		if(!ret)return false;
		ret = takeback_and_call<list_size<typename RHS::UNPARSED_LIST>::value>(cb_rhs,args...);
		return ret;
	}
	void _undo(auto& ctx){
		undo(ctx,rhs.get());
		undo(ctx,lhs.get());
	}

	bool dbg_inline() const {
		return true;
	}
};


template<typename parser>
struct activated : public parser {
	using parser::parser;
	using active = active_t;
	activated(parser&& self) : parser(self) {};
};

template<parser T> 
auto operator!(T&& nr) {
	using P = std::decay_t<T>;
	return activated<P>(std::forward<P>(nr));
}
template<parser parser_t>
struct forget {
	using UNPARSED_LIST = TLIST<EOL>;
	using active = active_f;

	parser_t p;

	forget(auto&& op) : p(std::forward<parser_t>(op)) {};
	
	bool _parse(auto& ctx){
		return parse(ctx,p);
	}
	bool dbg_inline(){
		return true;
	}
};

template<typename L>
struct inline_tuple {
	using type = 
		typename t_if_else<
			list_size<L>::value == 1,
			typename L::type,
			typename apply_list<std::tuple,L>::type>::type;
};
template<typename T>
struct inline_variant {
	using type = TLIST<T>;
};
template<typename ... ARGS>
struct inline_variant<TLIST<std::variant<ARGS...> > > {
	using type = TLIST<ARGS...>;
};
template<typename L1, typename L2>
struct or_helper {
	using NL1 = typename inline_variant<typename inline_tuple<L1>::type>::type;
	using NL2 = typename inline_variant<typename inline_tuple<L2>::type>::type;
	using merge = typename append_list<NL1,NL2>::type;
	using non_empty = typename remove_t<merge,std::tuple<>>::type;
	using non_dup = typename dedup<non_empty>::type;
	using type = 
		typename t_if_else<
			std::is_same_v<NL1,NL2> && list_size<NL1>::value == 1,
			typename NL1::type,//TODO can be list
			typename t_if_else<
				list_size<non_dup>::value == 1,
				std::optional<typename non_dup::type>,
				typename apply_list<std::variant,non_dup>::type
			>::type
		>::type;
};


template<parser P1, parser P2>
struct or_p {
	using ret_type = typename or_helper<
		typename P1::UNPARSED_LIST,
		typename P2::UNPARSED_LIST>::type;
	using UNPARSED_LIST = TLIST<ret_type>;
	using active = active_t;

	[[no_unique_address]] dont_store_empty<P1> p1;
	[[no_unique_address]] dont_store_empty<P2> p2;

	or_p(auto&& op1, auto&& op2) : p1(std::forward<P1>(op1)), p2(std::forward<P2>(op2)) {}

	bool _parse(auto& ctx, ret_type& ret){
		auto attempt = [&]<parser T>(T&& t) -> bool{
			using parser_t = std::decay_t<T>;
			if constexpr( rparser<parser_t> ){
				using h_t = typename apply_list<hold_normal,typename parser_t::UNPARSED_LIST>::type;
		
				h_t h;
				auto success = h.apply([&](auto& ctx, auto&...args){
					return parse_or_undo(ctx,t,args...);
				},ctx);
				if(success){
					ret = h.apply([](auto&...args) -> ret_type{
						return ret_type{args...};
					});
				}
				return success;
			}else{
				return parse_or_undo(ctx,t);
			}
		};
		return attempt(p1.get()) || attempt(p2.get());
	}
	bool dbg_inline() const{
		return true;
	}
};

template<parser P1, parser P2> 
parser auto operator|(P1&& p1, P2&& p2){
	using P1_t = std::decay_t<P1>;
	using P2_t = std::decay_t<P2>;

	return or_p<P1_t,P2_t>{std::forward<P1_t>(p1), std::forward<P2_t>(p2)};
}
template<parser P1, parser P2> 
parser auto operator+(P1&& p1, P2&& p2){
	using P1_t = std::decay_t<P1>;
	using P2_t = std::decay_t<P2>;
	if constexpr(std::is_same_v<active_t,typename P1_t::active> && std::is_same_v<active_t,typename P2_t::active>){
		return and_p<P1_t,P2_t>{std::forward<P1_t>(p1),std::forward<P2_t>(p2)};
	} else if constexpr(std::is_same_v<active_t,typename P1_t::active> && !std::is_same_v<active_t,typename P2_t::active>){
		using forget_t = forget<P2_t>;
		return and_p<P1_t, forget_t>(std::forward<P1_t>(p1),forget_t{std::forward<P2_t>(p2)});
	} else if constexpr(!std::is_same_v<active_t,typename P1_t::active> && std::is_same_v<active_t,typename P2_t::active>){
		using forget_t = forget<P1_t>;
		return and_p<forget_t, P2_t>(forget_t{std::forward<P1_t>(p1)},std::forward<P2_t>(p2));
	} else {
		using forget_t1 = forget<P1_t>;
		using forget_t2 = forget<P2_t>;
		return and_p<forget_t1, forget_t2>(forget_t1{std::forward<P1_t>(p1)},forget_t2{std::forward<P2_t>(p2)});
	}
}
template<parser P, typename F>
auto operator*(P&& p, F&& unparser) {
	using P_t = std::decay_t<P>;
	using invoke_info = invoke_list<F,typename get_ref_list<typename P_t::UNPARSED_LIST>::type>;
	using invoke_args = typename invoke_info::args;
	static_assert(!std::is_same_v<invoke_args,VOID>,"unparser callback is not callable by any of the available values");
	using remaining_types = 
		typename pop_n<list_size<invoke_args>::value,typename P_t::UNPARSED_LIST>::type;
	using F_TYPE = f_wrapper<typename std::decay_t<F>,typename invoke_info::ret,typename invoke_info::args>;
	if constexpr (!std::is_same_v<typename invoke_info::ret,void> && !std::is_same_v<typename invoke_info::ret,VOID>){
		using ret_args = 
			typename append_list<TLIST<F_TYPE,P_t,typename invoke_info::ret>,remaining_types>::type;//TODO reorder return type to back
		using ret_type = 
			typename apply_list<nr_parser,ret_args>::type;
		return ret_type(F_TYPE{std::forward<F>(unparser)},std::forward<P>(p));
	}else{
		using ret_args = 
			typename append_list<TLIST<F_TYPE,P_t>,remaining_types>::type;
		using ret_type = 
			typename apply_list<nr_parser,ret_args>::type;
		return ret_type(F_TYPE{std::forward<F>(unparser)},std::forward<P>(p));
	}
}
static_assert(std::is_same_v<invoke_list<std::function<void(int&,int&)>,TLIST<int&,int&>>::args,TLIST<int&,int&>>);
/* static_assert(std::is_same_v<invoke_list<std::function<void(int&&)>,TLIST<int>>::args,TLIST<int>>); */

template<typename F_TYPE, typename ... ARGS>
struct fr_parser_t {
	using active = active_f;
	using UNPARSED_LIST = TLIST<ARGS...>;

	F_TYPE fds;
	fr_parser_t(F_TYPE&& fds) : fds(std::forward<F_TYPE>(fds)) {};
	bool _parse(auto& ctx, ARGS&...args){
		return fds(ctx,args...);
	}
};

template<typename ... ARGS>
auto make_rpo(auto&& f){
	using F_TYPE = std::decay_t<decltype(f)>;
	using parser = fr_parser_t<F_TYPE,ARGS...>;
	return nr_parser<VOID,parser,ARGS...>(VOID{},parser{std::forward<F_TYPE>(f)});
}
template<context_c context_t, typename...UNPARSED>
struct rpo {
	using f_type = std::function<bool(context_t&,UNPARSED&...)>;
	using UNPARSED_LIST = TLIST<UNPARSED...>;
	using active = active_f;

	f_type f;

	rpo() = default;
	rpo(const rpo&) = delete;
	rpo(rpo&&) = delete;

	template<typename F> requires std::invocable<F,context_t&,UNPARSED&...>
	rpo(F&& f) {
		this->f = std::forward<F>(f);
	}

	template<parser T>
	void operator=(T&& p){
		using P_t = std::decay_t<T>;
		static_assert(std::same_as<typename P_t::UNPARSED_LIST, UNPARSED_LIST>, "unexpected return-types of right-hand-side parser to ezpz::rpo");
		f = [p = std::forward<P_t>(p)](context_t& ctx, auto&...up_args) mutable -> bool {
			if constexpr (rparser<T>){
				return parse(ctx,p,up_args...);
			}else{
				return parse(ctx,p);
			}
		};
	}

	bool _parse(context_t& ctx, UNPARSED&...up_args){
		return f(ctx,up_args...);
	}
	bool dbg_inline() const {
		return true;
	}
};
template<typename...ARGS>
using basic_rpo = rpo<basic_context, ARGS...>;
auto erase(parser auto&& parser){
	using prev_type = std::decay_t<decltype(parser)>;
	using ret_type = 
		typename apply_list<rpo,typename prev_type::UNPARSED_LIST>::type;
	ret_type ret = parser;
	return ret;
}
