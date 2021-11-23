#include "parse_object.hpp"
#include "matcher.hpp"
#include "meta.hpp"
#include <concepts>
#include <cmath>
#include <iostream>
#include <variant>
#include <optional>
#include <tuple>

template<bool b, typename TRUE, typename FALSE>
struct t_if_else {};

template<typename TRUE, typename FALSE>
struct t_if_else<true,TRUE,FALSE> {
	using type = TRUE;
};

template<typename TRUE, typename FALSE>
struct t_if_else<false,TRUE,FALSE> {
	using type = FALSE;
};
class VOID {};
template<typename ... ARGS>
struct  print_types;

template<typename T, T...vals>
struct [[deprecated]] print_vals;

template<typename T, typename L>
struct push_list {
	using type = T;
	using rest = L;
};

template<int amount, typename L>
struct pop_n  {
	using type = typename pop_n<amount-1,typename L::rest>::type;
};

template<typename L>
struct pop_n<0,L> {
	using type = L;
};

template<bool b, template<typename...> class C, typename...ARGS>
struct instantiate_if {
	using type = VOID;
};
template< template<typename...> class C, typename...ARGS>
struct instantiate_if<true,C,ARGS...> : C<ARGS...> {
	using type = C<ARGS...>;
};

template<bool b, template<typename...> class C, typename...ARGS>
struct instantiate_if_inner {
	using type = VOID;
};
template< template<typename...> class C, typename...ARGS>
struct instantiate_if_inner<true,C,ARGS...> : C<ARGS...> {
	using type = typename C<ARGS...>::type;
};

template<template<typename...> class T, typename L, typename...ARGS>
struct apply_list { 
	using type = typename apply_list<T,typename L::rest,ARGS...,typename L::type>::type;
	/* using type = t_if_else< */
	/* 	std::is_same_v<TLIST<EOL>,L>, */
	/* 	typename instantiate_if<std::is_same_v<TLIST<EOL>,L>,T,ARGS...>::type, */
	/* 	typename instantiate_if_inner<!std::is_same_v<TLIST<EOL>,L>,apply_list,T,typename L::rest,ARGS...,typename L::type>::type */
	/* >::type; */
};
template<template<typename...> class T, typename ... ARGS>
struct apply_list<T,TLIST<EOL>,ARGS...> {
	using type = T<ARGS...>;
};


template<typename ... ARGS>
struct print_types;


template<typename F, typename ... ARGS>
struct invoke_list_result {
	static_assert(std::is_invocable_v<F,ARGS...>,"Function is Not Invocable");
	using args = TLIST<ARGS...>;
	using ret = std::invoke_result_t<F,ARGS...>;
};


template<typename L, typename...ARGS>
struct append_list_hlp {
	using type = typename append_list_hlp<typename L::rest,ARGS...,typename L::type>::type;
};
template<typename ... ARGS>
struct append_list_hlp<TLIST<EOL>,ARGS...> {
	using type = TLIST<ARGS...>;
};
template<typename L1, typename L2, typename ... ARGS>
struct append_list {
	using type = typename append_list<typename L1::rest, L2, ARGS..., typename L1::type>::type;
};
template<typename L2, typename ...ARGS>
struct append_list<TLIST<EOL>,L2,ARGS...>{
	using type = typename append_list_hlp<L2,ARGS...>::type; 
};

template<typename L>
struct list_size {
	static constexpr int value = 1+list_size<typename L::rest>::value;
};
template<>
struct list_size<TLIST<EOL>> {
	static constexpr int value = 0;
};
template<typename L1, typename L2>
struct select_max {
	using type = typename t_if_else<
		(list_size<L1>::value > list_size<L2>::value),
		L1,
		L2>::type;
};

template<typename L>
struct select_max<L,VOID> {
	using type = L;
};
template<typename L>
struct select_max<VOID,L> {
	using type = L;
};
template<>
struct select_max<VOID,VOID> {
	using type = VOID;
};
static_assert(std::is_same_v<typename select_max<TLIST<int>,TLIST<int,float>>::type,TLIST<int,float>>);
template<template<typename...> class C, typename L, typename ... ARGS>
struct instantiate_list {
	using type = typename instantiate_list<C,typename L::rest, ARGS..., typename L::type>::type;
};

template<template<typename...> class C,typename ... ARGS>
struct instantiate_list<C,TLIST<EOL>,ARGS...> {
	using type = C<ARGS...>;
};

template<typename F, typename ARGS>
struct invoke_list_get_ret {
	using type = typename instantiate_list<
		std::invoke_result,typename append_list<TLIST<F>,ARGS>::type
		>::type::type;
};
template<typename F, typename L, typename ... ARGS>
struct invoke_list {
	/* using cur = */ 
	/* 	typename instantiate_if< */
	/* 		std::is_invocable_v<F,ARGS&...>,TLIST,ARGS...>::type; */
	/* using inner = */ 
	/* 	typename invoke_list<F,typename L::rest,  ARGS...,typename L::type>::args; */
	using args = typename select_max<
		typename instantiate_if<
			std::is_invocable_v<F,ARGS...>,TLIST,ARGS...>::type,
		typename invoke_list<F,typename L::rest,  ARGS...,typename L::type>::args
	>::type;
	/* print_types<cur,inner,args> fds; */
	using ret = typename instantiate_if_inner<
		!std::is_same_v<args,VOID>,
		invoke_list_get_ret,F,args
		>::type;
};
template<typename L, typename ... ARGS>
struct get_decay_list {
	using type = get_decay_list<typename L::rest, ARGS...,typename std::decay_t<typename L::type>>::type;
};
template<typename ... ARGS>
struct get_decay_list<TLIST<EOL>,ARGS...> {
	using type = TLIST<ARGS...>;
};
template<typename L, typename ... ARGS>
struct get_ref_list {
	using type = get_ref_list<typename L::rest, ARGS...,typename L::type>::type;
};
template<typename ... ARGS>
struct get_ref_list<TLIST<EOL>,ARGS...> {
	using type = TLIST<ARGS&...>;
};

template<typename F, typename ... ARGS>
struct invoke_list<F,EOL,ARGS...> {
	using args = VOID;
	using ret = VOID;
};

template<typename L, typename ... ARGS>
struct reverse_list {
	using type = typename reverse_list<
		typename L::rest,
		typename L::type,
		ARGS...>::type;
};
template<typename ... ARGS>
struct reverse_list<TLIST<EOL>,ARGS...> {
	using type = TLIST<ARGS...>;
};
template<typename FIRST=void, typename ... ARGS>
struct hold_normal {
	FIRST first;
	hold_normal<ARGS...> rest;

	template<typename F, typename ... OARGS>
	auto apply(F&& f, OARGS&&...oargs){
		return rest.apply(f,oargs...,first);
	}
};
template<>
struct hold_normal<void>{
	template<typename F, typename ...OARGS>
	auto apply(F&&f, OARGS&&...oargs){
		/* print_types<F,OARGS...> fds; */
		return f(oargs...);
	}
};
template<typename FIRST=void, typename ... ARGS>
struct hold {
	FIRST first;
	hold<ARGS...> rest;

	template<typename F, typename ... OARGS>
	auto apply(F&& f, OARGS&&...oargs){
		return rest.apply(f,first,oargs...);
	}
	template<typename F, typename X, typename ...OARGS>
	auto apply_not_first(F&&f, X&&, OARGS&&...oargs){
		return this->apply(f,oargs...);
	}
};
template<>
struct hold<void>{
	template<typename F, typename ...OARGS>
	auto apply(F&&f, OARGS&&...oargs){
		/* print_types<F,OARGS...> fds; */
		return f(oargs...);
	}
	template<typename F, typename X, typename ...OARGS>
	auto apply_not_first(F&&f, X&&, OARGS&&...oargs){
		return this->apply(f,oargs...);
	}
};

template<typename F, typename _RET, typename _ARGS>
struct f_wrapper : public F {
	using RET = _RET;
	using ARGS = _ARGS;
	using self_t = F;
	f_wrapper(F&& f) : F(std::forward<F>(f)) {};
	f_wrapper(F& f) : F(std::forward<F>(f)) {};
};

/* template<typename L,typename...ARGS> */
/* struct create_r_parser_with_unparsed_args { */
/* 	using type = */ 
/* 		typename create_r_parser_with_unparsed_args< */
/* 		typename L::rest, */
/* 		typename L::type,ARGS...>::type; */
/* }; */
/* template<typename...ARGS> */
/* struct create_r_parser_with_unparsed_args<TLIST<EOL>,ARGS...> { */
/* 	bool parse(context& ctx, ARGS...args); */
/* }; */
template<typename T1, typename...REST>
auto assign_first(T1&& src, T1& dst, REST&&...){
	dst = std::forward<T1>(src);
}

template<typename unp = VOID, typename REM = VOID, typename ... UNPARSED>
struct nr_parser : public parse_object {
	/* using self_t = nr_parser<unp,REM,UNPARSED...>; */
	using UNPARSED_LIST = TLIST<UNPARSED...>;
	using active = active_f;

	REM parent;
	unp f;

	nr_parser(unp&& f, REM& parent) :
		parent(std::forward<REM>(parent)),
		f(std::forward<unp>(f))
	{};
	nr_parser(unp&& f, REM&& parent) :
		parent(std::forward<REM>(parent)),
		f(std::forward<unp>(f))
	{};
	bool parse(context& ctx, UNPARSED&...up_args){
		if constexpr(std::is_same_v<REM,VOID>){
			return true;
		}else{
			if constexpr( std::is_same_v<unp,VOID> ) {
				if(ctx.debug && !dbg_inline){
					auto prev = parse_object::dbg_log_enter(ctx);
					auto ret = parent.parse(ctx,up_args...);
					parse_object::dbg_log_leave(ctx);
					std::cout << "accepted ";
					dbg_log_comment(ctx,prev);
					return ret;
				}else{
					return parent.parse(ctx,up_args...);
				}
			}else if constexpr (!std::is_same_v<typename unp::RET,void>){
				using hold_args = typename get_decay_list<
					typename reverse_list<
						typename unp::ARGS>::type>::type;
				using hold_type = typename instantiate_list<hold,hold_args>::type;
				hold_type hold;
				/* print_types<UNPARSED...> fds; */
				bool success = hold.apply_not_first([&](auto&...args){
					return parent.parse(ctx,args...);
				},up_args...);
				if(!success)return false;

				if(ctx.debug && !dbg_inline){
					auto prev = parse_object::dbg_log_enter(ctx);
					assign_first(hold.apply(f),up_args...);
					parse_object::dbg_log_leave(ctx);
					std::cout << "accepted ";
					dbg_log_comment(ctx,prev);
				}else{
					assign_first(hold.apply(f),up_args...);
				}

				return true;
			}else{
				using hold_args = typename get_decay_list<typename reverse_list<typename unp::ARGS>::type>::type;
				using hold_type = typename instantiate_list<hold,hold_args>::type;
				hold_type hold;
				bool success = hold.apply([&](auto&...args){
					/* print_types<decltype(args)...> fds; */
					return parent.parse(ctx,args...);
				},up_args...);
				if(!success)return false;

				if(ctx.debug && !dbg_inline){
					auto prev = parse_object::dbg_log_enter(ctx);
					hold.apply(f);
					parse_object::dbg_log_leave(ctx);
					std::cout << "accepted ";
					dbg_log_comment(ctx,prev);
				}else{
					hold.apply(f);
				}

				return true;
			}
		}
	}
	bool _match(context& ctx) override{
		using hold_args = typename reverse_list<TLIST<UNPARSED...>>::type;
		using hold_type = typename instantiate_list<hold,hold_args>::type;
		hold_type h;
		return h.apply([&](UNPARSED&...u_args){return parse(ctx,u_args...);});
	}
};
template<typename...A1,typename...A2>
auto create_join_parser(auto&& p1, auto&& p2, TLIST<A1...>,TLIST<A2...>){
	using P1 = std::decay_t<decltype(p1)>;
	using P2 = std::decay_t<decltype(p2)>;
	if constexpr (std::is_same_v<TLIST<A2...>,TLIST<EOL>>){
		using parent_t = nr_parser<VOID,VOID,A1...>;
		struct join_p : public parent_t {
			using active = active_t;
			using UNPARSED_LIST = typename parent_t::UNPARSED_LIST;
			P1 p1;
			P2 p2;
			join_p(P1&& p1, P2&& p2) :
				parent_t({},{}),
				p1(std::forward<P1>(p1)),
				p2(std::forward<P2>(p2))
			{}
			bool parse(context& ctx,A1&...a1) {
				return p1.parse(ctx,a1...) && p2._match(ctx);
			}
			bool _match(context& ctx) override {
				hold_normal<A1...> h;
				return h.apply([self=this](context& ctx,A1&...a1) mutable {
						return self->parse(ctx,a1...);
					},ctx);
			}
		};
		auto ret = join_p{std::forward<P1>(p1),std::forward<P2>(p2)};
		ret.dbg_inline = true;
		return ret;
	}else if constexpr( std::is_same_v<TLIST<A1...>,TLIST<EOL>>){
		using parent_t = nr_parser<VOID,VOID,A2...>;
		struct join_p : public parent_t {
			using active = active_t;
			using UNPARSED_LIST = typename parent_t::UNPARSED_LIST;
			P1 p1;
			P2 p2;
			join_p(P1&& p1, P2&& p2) :
				parent_t({},{}),
				p1(std::forward<P1>(p1)),
				p2(std::forward<P2>(p2))
			{}
			bool parse(context& ctx,A2&...a2) {
				return p1._match(ctx) && p2.parse(ctx,a2...);
			}
			bool _match(context& ctx) override {
				hold_normal<A2...> h;
				return h.apply([self=this](context& ctx,A2&...a2) mutable {
						return self->parse(ctx,a2...);
					},ctx);
			}
		};
		auto ret = join_p{std::forward<P1>(p1),std::forward<P2>(p2)};
		ret.dbg_inline = true;
		return ret;
	}else{
		using parent_t = nr_parser<VOID,VOID,A1...,A2...>;
		struct join_p : public parent_t {
			using active = active_t;
			using UNPARSED_LIST = typename parent_t::UNPARSED_LIST;
			P1 p1;
			P2 p2;
			join_p(P1&& p1, P2&& p2) :
				parent_t({},{}),
				p1(std::forward<P1>(p1)),
				p2(std::forward<P2>(p2))
			{}
			bool parse(context& ctx,A1&...a1,A2&...a2) {
				return p1.parse(ctx,a1...) && p2.parse(ctx,a2...);
			}
			bool _match(context& ctx) override {
				hold_normal<A1...,A2...> h;
				return h.apply([self=this](context& ctx,A1&...a1,A2&...a2) mutable {
						return self->parse(ctx,a1...,a2...);
					},ctx);
		
			}
		};
		auto ret = join_p{std::forward<P1>(p1),std::forward<P2>(p2)};
		ret.dbg_inline = true;
		return ret;
	}

}


template<typename parser>
struct activated: public parser {
	using active = active_t;
	activated(parser&& self) : parser(self) {};
};

template<parser T> 
auto operator!(T&& nr) {
	using P = std::decay_t<T>;
	return activated<P>(std::forward<P>(nr));
}
template<typename parser>
struct forget : public parser {
	using UNPARSED_LIST = TLIST<EOL>;
	using active = active_f;

	forget(parser& p) : parser(p) {};
	forget(parser&& p) : parser(std::move(p)) {};
	
	bool parse(context& ctx, EOL&){
		return parser::_match(ctx);
	}
};

template<typename L, typename E>
struct remove_t {
	using rest = typename remove_t<typename L::rest,E>::type;
	using type = typename 
		t_if_else<
			std::is_same_v<typename L::type, E>,
			rest,
			typename append_list<TLIST<typename L::type>,rest>::type
		>::type;
};

template<typename E>
struct remove_t<TLIST<EOL>,E> {
	using type = TLIST<EOL>;
};

template<typename L, typename E>
struct contains {
	static constexpr bool value = std::is_same_v<typename L::type,E> 
				|| contains<typename L::rest,E>::value;
};
template<typename E>
struct contains<TLIST<EOL>,E> {
	static constexpr bool value = false;
};
template<typename L>
struct dedup {
	using inner = typename dedup<typename L::rest>::type;
	using type = typename t_if_else<
		contains<inner,typename L::type>::value,
		inner,
		typename append_list<TLIST<typename L::type>,inner>::type
	>::type;
};
template<>
struct dedup<TLIST<EOL>> {
	using type = TLIST<EOL>;
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
struct or_parser : public parse_object {
	using ret_type = typename or_helper<
		typename P1::UNPARSED_LIST,
		typename P2::UNPARSED_LIST>::type;
	using UNPARSED_LIST = TLIST<ret_type>;
	using active = active_t;

	P1 p1;
	P2 p2;
	or_parser(P1&& op1, P2&& op2) : p1(std::move(op1)), p2(std::move(op2)) {}
	or_parser(P1&& op1, const P2& op2) : p1(std::move(op1)), p2(op2) {}
	or_parser(const P1& op1, P2&& op2) : p1(op1), p2(std::move(op2)) {}
	or_parser(const P1& op1, const P2& op2) : p1(op1), p2(op2) {}

	bool parse(context& ctx, ret_type& ret){
		auto attempt = [&]<parser T>(T& t) -> bool{
			auto prev = ctx.pos;
			if constexpr( rparser<T> ){
				using h_t = apply_list<hold_normal,typename T::UNPARSED_LIST>::type;
		
				h_t h;
				auto success = h.apply([&](context& ctx, auto&...args){
					return t.parse(ctx,args...);
				},ctx);
				if(success){
					ret = h.apply([](auto&...args) -> ret_type{
						return ret_type{args...};
					});
				}
				if(!success)ctx.pos = prev;
				return success;
			}else{
				return t.match(ctx);
			}
		};
		return attempt(p1) || attempt(p2);
		//hold p1 args
		//apply p1
	}
};

template<parser P1, parser P2> requires rparser<P1> || rparser<P2>
parser auto operator|(P1&& p1, P2&& p2){
	using P1_t = std::decay_t<P1>;
	using P2_t = std::decay_t<P2>;

	return or_parser<P1_t,P2_t>{std::forward<P1_t>(p1), std::forward<P2_t>(p2)};



	//compose return type
	//optional
	
	//l,r
	//inline tuple
	//merge variant
	//inline variant => optional
	
	//init ret type
	//
}
template<parser P1, parser P2> requires rparser<P1> || rparser<P2>
rparser auto operator+(P1&& p1, P2&& p2){
	using P1_t = std::decay_t<P1>;
	using P2_t = std::decay_t<P2>;
	/* print_types<typename P1_t::UNPARSED_LIST, typename P2_t::UNPARSED_LIST, P1_t,P2_t> asd; */
	/* print_types<P1,P2_t> asd; */
	/* print_vals<int, std::is_same_v<P2_t,const char*>> asd2; */
	if constexpr( std::is_same_v<P2_t,const char*> ){
		return p1 + text_parser(p2);
	} else if constexpr(std::is_same_v<active_t,typename P1_t::active> && std::is_same_v<active_t,typename P2_t::active>){
		return create_join_parser(p1,p2,typename P1_t::UNPARSED_LIST{},typename P2_t::UNPARSED_LIST{});
	} else if constexpr(std::is_same_v<active_t,typename P1_t::active> && !std::is_same_v<active_t,typename P2_t::active>){
		return create_join_parser(p1,
				forget{std::forward<P2>(p2)},
				typename P1_t::UNPARSED_LIST{},
				TLIST<EOL>{});
	} else if constexpr(!std::is_same_v<active_t,typename P1_t::active> && std::is_same_v<active_t,typename P2_t::active>){
		return create_join_parser(
				forget{std::forward<P1>(p1)},
				p2,
				TLIST<EOL>{},
				typename P2_t::UNPARSED_LIST{});
	} else {
		return f_parser([first = std::forward<P1>(p1),second=std::forward<P2>(p2)] 
		(auto& ctx) mutable {
			return first.match(ctx) && second.match(ctx);
		});
	}
}
template<parser P, typename F>
auto operator*(P&& p, F&& unparser) {
	using P_t = std::decay_t<P>;
	using invoke_info = invoke_list<F,typename get_ref_list<typename P_t::UNPARSED_LIST>::type>;
	using invoke_args = typename invoke_info::args;
	/* using invoke_ret = typename invoke_info::ret; */
	/* print_types<invoke_args,F,typename P::UNPARSED_LIST> asd; */
	using remaining_types = 
		typename pop_n<list_size<invoke_args>::value,typename P_t::UNPARSED_LIST>::type;
	using F_TYPE = f_wrapper<typename std::decay_t<F>,typename invoke_info::ret,typename invoke_info::args>;
	if constexpr (!std::is_same_v<typename invoke_info::ret,void> && !std::is_same_v<typename invoke_info::ret,VOID>){
		using ret_args = 
			typename append_list<TLIST<F_TYPE,P,typename invoke_info::ret>,remaining_types>::type;
		/* print_types<ret_args,typename invoke_info::ret> sfds; */
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
			/* nr_parser<decltype(unparser),self_t,RET>, */
			/* args_size, */
			/* UNPARSED...>::type; */

			/* (unparser,std::move(*this)); */
}
/* print_types<invoke_list<std::function<void(int,int)>,TLIST<int,int>>> dfhskf; */
static_assert(std::is_same_v<invoke_list<std::function<void(int&,int&)>,TLIST<int&,int&>>::args,TLIST<int&,int&>>);
template<typename F_TYPE>
struct fr_parser_t {
	F_TYPE fds;
	fr_parser_t(F_TYPE&& fds) : fds(std::forward<F_TYPE>(fds)) {};
	bool parse(context& ctx, auto&...args){
		return fds(ctx,args...);
	}
};
template<typename ... ARGS>
auto fr_parser(auto&& f){
	using F_TYPE = std::decay_t<decltype(f)>;
	return nr_parser<VOID,fr_parser_t<F_TYPE>,ARGS...>({},fr_parser_t<F_TYPE>{std::forward<F_TYPE>(f)});
}
template<typename integer, int base>
auto number = fr_parser<integer>([](context& ctx, integer& ret){
	static_assert(base <= 10);
	static_assert(base >= 2);
	if(ctx.done())return false;
	bool negative = false;
	if(ctx.get() == '-'){
		negative = true;
		++ctx.pos;
	}else if(ctx.get() == '+'){
		++ctx.pos;
	}
	if(ctx.done())return false;
	ret = 0;
	bool invalid = true;
	while(!ctx.done() && std::isdigit(ctx.get())){
		invalid = false;
		ret *= base;
		ret += ctx.get()-'0';
		++ctx.pos;
	}
	if(invalid)return false;
	if constexpr(std::floating_point<integer>){
		if(ctx.get() == '.'){
			++ctx.pos;
			invalid = true;
			integer alpha = 1;
			while(!ctx.done() && std::isdigit(ctx.get())){
				invalid = false;
				alpha /= base;
				ret += (ctx.get()-'0')*alpha;
				++ctx.pos;
			}
			if(invalid)return false;
		}
	}
	if(negative)ret = -ret;
	return true;
});
template<typename integer>
auto& decimal = number<integer,10>;

template<typename...UNPARSED>
struct rpo : public parse_object{
	using f_type = std::function<bool(context&,UNPARSED&...)>;
	using UNPARSED_LIST = TLIST<UNPARSED...>;

	f_type f;

	rpo() = default;

	template<typename F> requires std::invocable<F,context&,UNPARSED&...>
	rpo(F&& f) {
		this->f = std::forward<F>(f);
	}

	template<parser T>
	void operator=(T&& p){
		f = [p = std::forward<T>(p)](context& ctx, auto&...up_args) mutable -> bool {
				return p.parse(ctx,up_args...);
		};
	}

	bool parse(context& ctx, UNPARSED&...up_args){
		return f(ctx,up_args...);
	}
	bool _match(context& ctx) override {
		hold_normal<UNPARSED...> h;
		return h.apply(f,ctx);
	}
};
auto erase(parser auto&& parser){
	using prev_type = std::decay_t<decltype(parser)>;
	using ret_type = 
		typename apply_list<rpo,typename prev_type::UNPARSED_LIST>::type;
	ret_type ret(
			[parser= std::forward<prev_type>(parser)](context& ctx, auto&...up_args) mutable -> bool {
			return parser.parse(ctx,up_args...);
		});
	return ret;
}
auto copy(auto&& val){
	auto cp = val;
	return cp;
}
template<typename UNP, typename REM, typename ... UNPARSED>
auto erase(const nr_parser<UNP,REM,UNPARSED...>& parser){
	auto cp = copy(parser);
	return erase(std::move(cp));
}
parser auto ref(auto& p){
	using inner = std::decay_t<decltype(p)>;
	return parse_object_ref<inner>{p};
}
template<rparser T>
parser auto operator>>(optional_t,T&& rhs) {
	auto ret = std::forward<T>(rhs) | (not_v >> fail);
	return ret;
}
auto assign(auto& dst){
	return [&](decltype(dst)& val){
		dst = val;
	};
}

int main(){
	/* nr_parser<VOID,VOID,int,std::string_view> p({},{}); */
	/* auto next = std::move(p) * [](int i){ */
	/* 	std::cout << i << std::endl; */
	/* } * [](std::string_view sv){ */
	/* 	std::cout << sv.size() << " " << sv << std::endl; */
	/* }; */
	/* auto next = std::move(p) * [](int,std::string_view){}; */
	/* context ctx; */
	/* ctx.input = "hallo"; */
	/* next.parse(ctx); */
	/* auto test = fr_parser<std::string_view,int>([](context&,std::string_view& i,int& s){ */ 
	/* 		i="Hello, World!"; */
	/* 		s = 24; */
	/* 		return true;}) * */
	/* 	[](auto&...args){ */
	/* 		((std::cout << args << ' '),...); */
	/* 		std::cout << std::endl; */
	/* 	}; */

	/* auto test = ( */
	/* 		!fr_parser<int>([](context&, int& i){ */
	/* 			i = 2; */
	/* 			return true; */
	/* 		}) */ 
	/* 		+ */
	/* 		!fr_parser<int>([](context&, int& i){ */
	/* 			i = 53; */
	/* 			return true; */
	/* 		}) */ 
	/* 		+ */
	/* 		!fr_parser<std::string_view>([](context&, auto& i){ */
	/* 			i = "Hello, World"; */
	/* 			return true; */
	/* 		}) */ 
	/* 		) */
	/* 	* [](auto a, auto b){return a+b;} */
	/* 	* */
	/* 	[](auto&...args){ */
	/* 		((std::cout << args << ' '),...); */
	/* 		std::cout << std::endl; */
	/* 	}; */
	/* print_types<decltype(test)> asd; */
	/* test(ctx); */
	auto print_all = [](auto&...args){
		((std::cout << args << ' '),...); 
		std::cout << '\n';
	};

	using num_t = float;
	/* rpo<num_t> m_expr, s_expr; */
	/* auto rm_expr = ref(m_expr); */
	/* auto rs_expr = ref(s_expr); */
	/* m_expr = erase( */
	/* 	 (!decimal<num_t> + ws + */ 
	/* 	 (optional >> ("*" +  ws + !rm_expr)) )*[](auto a, auto b) -> num_t */
	/* 	 {return b?a**b:a;} */
	/* ); */
	/* s_expr = erase( */
	/* 	 (!rm_expr + ws + */ 
	/* 	 (optional >> ("+" +  ws + !rs_expr) ))*[](auto a, auto b) -> num_t */
	/* 	 {return b?a+*b:a;} */
	/* ); */
	/* context ctx("7*0.8"); */
	/* ctx.debug = true; */
	/* (rs_expr*print_all)(ctx); */

	auto op_maker_la = [](std::string_view op, auto& upper, auto&& operation){
		auto parser = std::make_unique<rpo<num_t>>();
		*parser = fr_parser<num_t>([op,operation,&upper](context& ctx, num_t& num){
			return (ref(upper)*assign(num)+ws+(any >> (op+ws+ref(upper)*
					[&](num_t val){
						std::cout << num << " - " << val << " = " << num-val << std::endl;
						num -= val;
					}
			)))(ctx);
		});
		return parser;
	};
	auto op_maker = [](std::string_view op, auto& upper, auto&& operation){
		auto parser = std::make_unique<rpo<num_t>>();
		*parser = 
			 (!ref(upper) + ws + (optional >> (op + ws + !ref(*parser))))
			 *[=,operation](auto a, auto b){
				 if(b){
					auto ret = operation(a,*b);
					std::cout << a << " " << op << " " << *b << " = " << ret << std::endl;
					return ret;
				 }else{
					return a;
				 }
				 /* return b ? operation(a,*b) : a; */
			 };
		return parser;
	};

	rpo<num_t> base;
	auto p0 = op_maker("^",base,[](auto a, auto b){
		return std::pow(a,b);
	});
	auto p1 = op_maker("/",*p0,[](auto a, auto b){
		return a/b;
	});
	auto p2 = op_maker("*",*p1,[](auto a, auto b){
		return a*b;
	});
	auto p3 = op_maker_la("-",*p2,[](auto a, auto b){
		return a-b;
	});
	auto p4 = op_maker("+",*p3,[](auto a, auto b){
		return a+b;
	});
	base = "("+ws+!ref(*p4)+ws+")" | !decimal<num_t>;
	auto expr = ws+!ref(*p4)+ws+eoi;

	/* context ctx("1*2/3-4+5^5"); */
	while(true){
		std::string line;
		std::getline(std::cin,line);
		context ctx(line);
		/* ctx.debug = true; */
		((expr*print_all) | print("error"))(ctx);
	}

	/* std::cout << ctx.pos << std::endl; */
	/* test(ctx); */
	/* std::cout << ctx.pos << std::endl; */
}
