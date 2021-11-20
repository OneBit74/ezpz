#include "parse_object.hpp"
#include "matcher.hpp"
#include "meta.hpp"
#include <concepts>
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
	using self_t = nr_parser<unp,REM,UNPARSED...>;
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
			P1 p1;
			P2 p2;
			join_p(P1&& p1, P2&& p2) :
				parent_t({},{}),
				p1(std::forward<P1>(p1)),
				p2(std::forward<P2>(p2))
			{}
			bool parse(context& ctx,A1&...a1) {
				return p1.parse(ctx,a1...) && p2.match(ctx);
			}
			bool _match(context& ctx) override {
				hold_normal<A1...> h;
				return h.apply([self=this](context& ctx,A1&...a1) mutable {
						return self->parse(ctx,a1...);
					},ctx);
			}
		};
		return join_p{std::forward<P1>(p1),std::forward<P2>(p2)};
	}else if constexpr( std::is_same_v<TLIST<A1...>,TLIST<EOL>>){
		using parent_t = nr_parser<VOID,VOID,A2...>;
		struct join_p : public parent_t {
			using active = active_t;
			P1 p1;
			P2 p2;
			join_p(P1&& p1, P2&& p2) :
				parent_t({},{}),
				p1(std::forward<P1>(p1)),
				p2(std::forward<P2>(p2))
			{}
			bool parse(context& ctx,A2&...a2) {
				return p1.match(ctx) && p2.parse(ctx,a2...);
			}
			bool _match(context& ctx) override {
				hold_normal<A2...> h;
				return h.apply([self=this](context& ctx,A2&...a2) mutable {
						return self->parse(ctx,a2...);
					},ctx);
			}
		};
		return join_p{std::forward<P1>(p1),std::forward<P2>(p2)};
	}else{
		using parent_t = nr_parser<VOID,VOID,A1...,A2...>;
		struct join_p : public parent_t {
			using active = active_t;
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
		return join_p{std::forward<P1>(p1),std::forward<P2>(p2)};
	}

}


template<typename parser>
struct activated_parser : public parser {
	using active = active_t;
	activated_parser(parser&& self) : parser(self) {};
};

template<typename T>
concept parser = requires(T t, context& ctx){
	{ 
		t.undo(ctx)
	} -> std::same_as<void>;
	{ 
		t.match(ctx)
	} -> std::same_as<bool>;
};
template<parser T> 
auto operator!(T&& nr) {
	using P = std::decay_t<T>;
	return activated_parser<P>(std::forward<P>(nr));
}
template<typename parser>
struct forget : public parser {
	using UNPARSED_LIST = TLIST<EOL>;

	forget(parser& p) : parser(p) {};
	forget(parser&& p) : parser(std::move(p)) {};
	
	bool parse(context& ctx, EOL&){
		return parser::_match(ctx);
	}
};

template<typename P1, typename P2>
auto operator+(P1&& p1, P2&& p2){
	using P1_t = std::decay_t<P1>;
	using P2_t = std::decay_t<P2>;
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
template<typename P, typename F>
auto operator*(P&& p, F&& unparser) {
	using P_t = std::decay_t<P>;
	using invoke_info = invoke_list<F,typename get_ref_list<typename P_t::UNPARSED_LIST>::type>;
	using invoke_args = typename invoke_info::args;
	/* using invoke_ret = typename invoke_info::ret; */
	/* print_types<invoke_args> asd; */
	using remaining_types = 
		typename pop_n<list_size<invoke_args>::value,typename P_t::UNPARSED_LIST>::type;
	using F_TYPE = f_wrapper<typename std::decay_t<F>,typename invoke_info::ret,typename invoke_info::args>;
	if constexpr (!std::is_same_v<typename invoke_info::ret,void> && !std::is_same_v<typename invoke_info::ret,VOID>){
		using ret_args = 
			typename append_list<TLIST<F_TYPE,P,typename invoke_info::ret>,remaining_types>::type;
		/* print_types<ret_args,typename invoke_info::ret> sfds; */
		using ret_type = 
			typename apply_list<nr_parser,ret_args>::type;
		return ret_type(F_TYPE{std::forward<F>(unparser)},std::move(p));
	}else{
		using ret_args = 
			typename append_list<TLIST<F_TYPE,P_t>,remaining_types>::type;
		using ret_type = 
			typename apply_list<nr_parser,ret_args>::type;
		return ret_type(F_TYPE{std::forward<F>(unparser)},std::move(p));
	}
			/* nr_parser<decltype(unparser),self_t,RET>, */
			/* args_size, */
			/* UNPARSED...>::type; */

			/* (unparser,std::move(*this)); */
}
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
	if(negative)ret = -ret;
	return true;
});
template<typename integer>
auto& decimal = number<integer,10>;

template<typename...UNPARSED>
struct rpo : public parse_object{
	using f_type = std::function<bool(context&,UNPARSED&...)>;
	using UNPARSED_LIST = TLIST<UNPARSED...>;

	std::shared_ptr<f_type> data;

	rpo() {
		data = std::make_shared<f_type>();
	}
	rpo(const rpo<UNPARSED...>& other) = default;
	rpo(rpo<UNPARSED...>&& other) = default;

	rpo<UNPARSED...>& operator=(const rpo<UNPARSED...>&) = default;
	rpo<UNPARSED...>& operator=(rpo<UNPARSED...>&&) = default;
	
	template<typename F> requires std::invocable<F,context&,UNPARSED&...>
	void operator=(F&& f) {
		(*data.get()) = std::forward<F>(f);
	}


	bool parse(context& ctx, UNPARSED&...up_args){
		return (*data.get())(ctx,up_args...);
	}
	bool _match(context& ctx) override {
		hold_normal<UNPARSED...> h;
		return h.apply(*data.get(),ctx);
	}
};
auto erase(parser auto&& parser){
	using prev_type = std::decay_t<decltype(parser)>;
	using ret_type = 
		typename apply_list<rpo,typename prev_type::UNPARSED_LIST>::type;
	ret_type ret;
	ret = 
		[parser= std::forward<prev_type>(parser)](context& ctx, auto&...up_args) mutable -> bool {
			return parser.parse(ctx,up_args...);
		};
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

	rpo<int> m_expr, s_expr;
	auto inner = !decimal<int>;
	s_expr = erase(inner);
	m_expr = erase(
			( !s_expr 
			 + ws +  "*" +  ws +
			 !m_expr)*[](int a, int b) -> int{return a*b;});

	* [](int& x){
		std::cout << x << std::endl;
	};
	auto simple = ws+"a"+ws;
	auto print_all = [](auto&...args){
		(std::cout << ... << args) << '\n';
	};
	context ctx("4 * 3");
	/* context ctx(" a "); */
	ctx.debug = true;
	/* simple(ctx); */
	(m_expr*print_all)(ctx);

	/* std::cout << ctx.pos << std::endl; */
	/* test(ctx); */
	/* std::cout << ctx.pos << std::endl; */
}
