#include <type_traits>
#pragma once

template<typename ... ARGS>
struct  print_types;

template<typename T, T...vals>
struct [[deprecated]] print_vals;

struct active_t;
struct active_f;

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

template<template<typename...> class T, typename L, typename...ARGS>
struct apply_list { 
	using type = typename apply_list<T,typename L::rest,ARGS...,typename L::type>::type;
};
template<template<typename...> class T, typename ... ARGS>
struct apply_list<T,TLIST<EOL>,ARGS...> {
	using type = T<ARGS...>;
};

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
	using args = typename select_max<
		typename instantiate_if<
			std::is_invocable_v<F,ARGS...>,TLIST,ARGS...>::type,
		typename invoke_list<F,typename L::rest,  ARGS...,typename L::type>::args
	>::type;
	using ret = typename instantiate_if_inner<
		!std::is_same_v<args,VOID>,
		invoke_list_get_ret,F,args
		>::type;
};
template<typename L, typename ... ARGS>
struct get_decay_list {
	using type = typename get_decay_list<typename L::rest, ARGS...,typename std::decay_t<typename L::type>>::type;
};
template<typename ... ARGS>
struct get_decay_list<TLIST<EOL>,ARGS...> {
	using type = TLIST<ARGS...>;
};
template<typename L, typename ... ARGS>
struct get_ref_list {
	using type = typename get_ref_list<typename L::rest, ARGS...,typename L::type>::type;
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
template<typename L, typename E>
struct contains {
	static constexpr bool value = std::is_same_v<typename L::type,E> 
				|| contains<typename L::rest,E>::value;
};
template<typename E>
struct contains<TLIST<EOL>,E> {
	static constexpr bool value = false;
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
