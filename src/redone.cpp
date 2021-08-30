#include "parse_object.hpp"
#include <iostream>

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

struct EOL {};
template<typename FIRST=EOL, typename ... REST>
struct TLIST : TLIST<REST...>{
	using type = FIRST;
	using rest = TLIST<REST...>;
};
template<>
struct TLIST<EOL> : EOL {
	using type = EOL;
	using rest = EOL;
};

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
struct hold {
	FIRST first;
	hold<ARGS...> rest;

	template<typename F, typename ... OARGS>
	auto apply(F&& f, OARGS&&...oargs){
		return rest.apply(f,first,oargs...);
	}
};
template<>
struct hold<void>{
	template<typename F, typename ...OARGS>
	auto apply(F&&f, OARGS&&...oargs){
		/* print_types<F,OARGS...> fds; */
		return f(oargs...);
	}
};

template<typename F, typename _RET, typename _ARGS>
struct f_wrapper : public F {
	using RET = _RET;
	using ARGS = _ARGS;
	using self_t = F;
	f_wrapper(F&& f) : F(std::forward<F>(f)) {};
};

template<typename unp = VOID, typename REM = VOID, typename ... UNPARSED>
struct nr_parser : public parse_object {
	using self_t = nr_parser<unp,REM,UNPARSED...>;

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
				return parent.parse(ctx,up_args...);
			}else{
				using hold_args = typename get_decay_list<typename reverse_list<typename unp::ARGS>::type>::type;
				using hold_type = typename instantiate_list<hold,hold_args>::type;
				hold_type hold;
				bool success = hold.apply([&](auto&...args){
					/* print_types<decltype(args)...> fds; */
					return parent.parse(ctx,args...);
				},up_args...);
				if(!success)return false;
				hold.apply(f);
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
	template<typename F>
	auto operator*(F&& unparser) && {
		/* static_assert(std::is_invocable_v<F,int>,"yes"); */
		using invoke_info = invoke_list<F,TLIST<UNPARSED&...>>;
		using invoke_args = typename invoke_info::args;
		/* using invoke_ret = typename invoke_info::ret; */
		/* print_types<invoke_args> asd; */
		using remaining_types = 
			typename pop_n<list_size<invoke_args>::value,TLIST<UNPARSED...>>::type;
		/* print_types<remaining_types> asd1; */
		using F_TYPE = f_wrapper<F,typename invoke_info::ret,typename invoke_info::args>;
		using ret_args = 
			typename append_list<TLIST<F_TYPE,self_t>,remaining_types>::type;
		/* print_types<ret_args> asd2; */
		using ret_type = 
			typename apply_list<nr_parser,ret_args>::type;
		return ret_type(std::forward<F>(unparser),std::move(*this));
				/* nr_parser<decltype(unparser),self_t,RET>, */
				/* args_size, */
				/* UNPARSED...>::type; */

				/* (unparser,std::move(*this)); */
	}
};
/* print_types<invoke_list<std::function<void(int,int)>,TLIST<int,int>>> dfhskf; */
static_assert(std::is_same_v<invoke_list<std::function<void(int&,int&)>,TLIST<int&,int&>>::args,TLIST<int&,int&>>);
template<typename ... ARGS>
auto fr_parser(auto&& f){
	using F_TYPE = std::decay_t<decltype(f)>;
	struct fr_parser_t {
		F_TYPE fds;
		fr_parser_t(F_TYPE&& fds) : fds(std::forward<F_TYPE>(fds)) {};
		bool parse(context& ctx, ARGS&...args){
			return fds(ctx,args...);
		}
	};
	return nr_parser<VOID,fr_parser_t,ARGS...>({},fr_parser_t{std::forward<F_TYPE>(f)});
}
int main(){
	/* nr_parser<VOID,VOID,int,std::string_view> p({},{}); */
	/* auto next = std::move(p) * [](int i){ */
	/* 	std::cout << i << std::endl; */
	/* } * [](std::string_view sv){ */
	/* 	std::cout << sv.size() << " " << sv << std::endl; */
	/* }; */
	/* auto next = std::move(p) * [](int,std::string_view){}; */
	context ctx;
	/* next.parse(ctx); */
	auto test = fr_parser<std::string_view,int>([](context&,std::string_view& i,int& s){ 
			i="Hello, World!";
			s = 24;
			return true;}) *
		/* [](std::string_view& sv){ */
		/* 	std::cout << sv << std::endl; */
		/* }; */
		/* [](std::string_view&){}* */
		[](auto&...args){
			((std::cout << args << ' '),...);
			std::cout << std::endl;
		};

	test(ctx);
}
