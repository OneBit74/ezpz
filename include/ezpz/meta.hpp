#include <type_traits>
#include <tuple>
#include <variant>
#include <optional>
#include <string>
#include <cxxabi.h>
#pragma once

namespace ezpz{

template<typename T>
std::string type_name()
{
	std::string tname = typeid(T).name();
	#if defined(__clang__) || defined(__GNUG__)
	int status;
	char *demangled_name = abi::__cxa_demangle(tname.c_str(), NULL, NULL, &status);
	if(status == 0)
	{
		tname = demangled_name;
		std::free(demangled_name);
	}
	#endif
	return tname;
}

template<typename ... ARGS>
struct  print_types;

template<typename T, T...vals>
struct [[deprecated]] print_vals;

class VOID {};
struct always_true;
struct always_false;

// context properties
struct is_recover_ctx;
struct has_inner_ctx;

template<bool b, template<typename...> class C, typename...ARGS>
struct instantiate_if {
	using type = VOID;
};
template< template<typename...> class C, typename...ARGS>
struct instantiate_if<true,C,ARGS...> : C<ARGS...> {
	using type = C<ARGS...>;
};

struct EOL {};

template<typename FIRST, typename ... REST>
struct first{
	using type = FIRST;
};
template<typename ... TS>
struct TLIST;
template<typename FIRST, typename ... REST>
struct rest{
	using type = TLIST<REST...>;
};
template<typename L1, typename L2>
struct append_list;
template<typename...A1, typename...A2>
struct append_list<TLIST<A1...>,TLIST<A2...>> {
	using type = TLIST<A1...,A2...>;
};

template<typename L, typename ... ARGS>
struct reverse_list {
	using type = typename reverse_list<
		typename L::rest,
		typename L::type,
		ARGS...>::type;
};
template<typename ... ARGS>
struct reverse_list<TLIST<>,ARGS...> {
	using type = TLIST<ARGS...>;
};

template<int amount, typename L>
struct pop_n  {
	using type = typename pop_n<amount-1,typename L::rest>::type;
};

template<typename L>
struct pop_n<0,L> {
	using type = L;
};

template<typename ... TS>
struct TLIST {
	using type = typename first<TS...>::type;
	using rest = typename rest<TS...>::type;
	static constexpr auto size = sizeof...(TS);

	template<typename L>
	using append = typename append_list<TLIST<TS...>,L>::type;
	using reverse = reverse_list<TLIST<TS...>>;
	template<int amount>
	using pop = pop_n<amount,TLIST<TS...>>;
	template<typename T>
	static constexpr bool contains = std::is_same_v<type,T> || rest::template contains<T>;
};
template<>
struct TLIST<> {
	using type = EOL;
	using rest = EOL;
	static constexpr auto size = 0;
	template<typename L>
	using append = L;
	using reverse = TLIST<>;
	template<typename>
	static constexpr bool contains = false;
};

template<class T>
concept has_type = requires(){
	typename T::type;
};
template<class T>
concept has_prop_tag = requires(){
	typename T::ezpz_prop;
};

template<template<typename...> class T, typename L, typename...ARGS>
struct apply_list { 
	using type = typename apply_list<T,typename L::rest,ARGS...,typename L::type>::type;
};
template<template<typename...> class T, typename ... ARGS>
struct apply_list<T,TLIST<>,ARGS...> {
	using type = T<ARGS...>;
};

template<bool b, typename TRUE, typename FALSE>
struct t_if_else;

template<typename TRUE, typename FALSE>
struct t_if_else<true,TRUE,FALSE> {
	using type = TRUE;
};

template<typename TRUE, typename FALSE>
struct t_if_else<false,TRUE,FALSE> {
	using type = FALSE;
};

template<typename T, typename L>
struct push_list {
	using type = T;
	using rest = L;
};



template<bool b, template<typename...> class C, typename...ARGS>
struct instantiate_if_inner {
	using type = VOID;
};
template< template<typename...> class C, typename...ARGS>
struct instantiate_if_inner<true,C,ARGS...> : C<ARGS...> {
	using type = typename C<ARGS...>::type;
};

template<typename L1, typename L2>
struct select_max {
	using type = typename t_if_else<
		(L1::size > L2::size),
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
struct instantiate_list<C,TLIST<>,ARGS...> {
	using type = C<ARGS...>;
};

template<typename F, typename ARGS>
struct invoke_list_get_ret {
	using type = typename instantiate_list<
		std::invoke_result,typename TLIST<F>::template append<ARGS>
		>::type::type;
};
template<typename F, typename L, typename ... ARGS>
struct invoke_list {
	using args = typename select_max<
		typename instantiate_if<
			std::is_invocable_v<F,ARGS...>,TLIST,ARGS...>::type,
		typename invoke_list<F,typename L::rest,  ARGS...,typename L::type>::args
	>::type;
	/* print_types<F,L,ARGS...,args> asd; */
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
struct get_decay_list<TLIST<>,ARGS...> {
	using type = TLIST<ARGS...>;
};
template<typename L, typename ... ARGS>
struct get_ref_list {
	using type = typename get_ref_list<typename L::rest, ARGS...,typename L::type>::type;
};
template<typename ... ARGS>
struct get_ref_list<TLIST<>,ARGS...> {
	using type = TLIST<ARGS&...>;
};

template<typename F, typename ... ARGS>
struct invoke_list<F,EOL,ARGS...> {
	using args = VOID;
	using ret = VOID;
};

template<typename ... ARGS>
struct hold_normal {
	std::tuple<ARGS...> data;

	template<typename F>
	auto move_into(F&& f){
		return std::apply(f,std::move(data));
	}
	template<typename F>
	auto apply(F&& f){
		return std::apply(f,data);
	}
	template<typename F, size_t...Is>
	auto apply_idx(F&& f){
		return f(data.template get<Is>()...);
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
struct contains<TLIST<>,E> {
	static constexpr bool value = false;
};

template<typename L, typename E>
struct remove_t {
	using rest = typename remove_t<typename L::rest,E>::type;
	using type = typename 
		t_if_else<
			std::is_same_v<typename L::type, E>,
			rest,
			typename TLIST<typename L::type>::template append<rest>
		>::type;
};

template<typename E>
struct remove_t<TLIST<>,E> {
	using type = TLIST<>;
};

/* template<typename L> */
/* struct dedup { */
/* 	using inner = typename dedup<typename L::rest>::type; */
/* 	using type = typename t_if_else< */
/* 		contains<inner,typename L::type>::value, */
/* 		inner, */
/* 		typename TLIST<typename L::type>::template append<inner> */
/* 	>::type; */
/* }; */
/* template<> */
/* struct dedup<TLIST<>> { */
/* 	using type = TLIST<>; */
/* }; */
template<typename L>
struct dedup {
	using type = typename append_list<TLIST<typename L::type>,
		typename dedup<typename remove_t<L,typename L::type>::type>::type
	>::type;
};
template<>
struct dedup<TLIST<>> {
	using type = TLIST<>;
};
template<typename T>
constexpr bool is_variant = false;
template<typename ...T>
constexpr bool is_variant<std::variant<T...>> = true;
template<typename T>
constexpr bool is_optional = false;
template<typename ...T>
constexpr bool is_optional<std::optional<T...>> = true;

struct dbg_inline; // parser property
				   //
template<typename T>
struct get_prop_tag {
	using type = TLIST<>;
	using raw = TLIST<>;
};
template<typename T> requires has_prop_tag<T>
struct get_prop_tag<T> {
	using type = typename remove_t<typename T::ezpz_prop,dbg_inline>::type;
	using raw = typename T::ezpz_prop;
};

template<typename L, typename T>
struct index_in_list;

template<typename L, typename T> requires std::same_as<typename L::type,T>
struct index_in_list<L,T> {
	static constexpr bool found = true;
	static constexpr size_t index = 0;
};
template<typename L, typename T> requires (!std::same_as<typename L::type,T>)
struct index_in_list<L,T> {
	using lower = index_in_list<typename L::rest, T>;
	static constexpr bool found = lower::found;
	static constexpr size_t index = 1+lower::index;
};
template<typename T>
struct index_in_list<TLIST<>,T> {
	static constexpr bool found = false;
};

template<size_t...Is>
struct idx_seq {};

template<size_t i, size_t ... Is>
idx_seq<i,Is...> append(idx_seq<Is...>){return {};}

template<typename L1, typename L2>
struct index_L_in_R {
	static constexpr size_t index = index_in_list<L2,typename L1::type>::index;
	using rest = index_L_in_R<typename L1::rest,L2>;
	using ix = decltype(append<index>(typename rest::ix{}));
};
template<typename T, typename L2>
struct index_L_in_R<TLIST<T>,L2> {
	static constexpr size_t index = index_in_list<L2,T>::index;
	using ix = idx_seq<index>;
};

template<size_t index>
decltype(auto) get(auto&& arg, auto&&...ARGS){
	if constexpr(index == 0){
		return arg;
	}else {
		return get<index-1>(ARGS...);
	}
}
template<typename T1, typename...REST>
auto assign_first(T1&& src, T1& dst, REST&&...){
	dst = std::forward<T1>(src);
}
template<typename T1, typename...REST>
auto assign_last(T1&& src, REST&&..., T1& dst){
	dst = std::forward<T1>(src);
}

}
