#pragma once

template<typename ... ARGS>
struct  print_types;

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

