#pragma once
#include "parse_object.hpp"
#include "meta.hpp"
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
struct nr_parser : public parse_object {
	using UNPARSED_LIST = TLIST<UNPARSED...>;
	using active = active_f;

	[[no_unique_address]] REM parent;
	[[no_unique_address]] unp f;

	nr_parser(unp&& f, REM& parent) :
		parent(std::forward<REM>(parent)),
		f(std::forward<unp>(f))
	{};
	nr_parser(unp&& f, REM&& parent) :
		parent(std::forward<REM>(parent)),
		f(std::forward<unp>(f))
	{};
	void _undo(context& ctx){
		if constexpr ( !std::is_same_v<REM,VOID> ) {
			parent._undo(ctx);
		}
	}
	bool _parse(context& ctx, UNPARSED&...up_args){
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
	bool _match(context& ctx) {
		using hold_args = typename reverse_list<TLIST<UNPARSED...>>::type;
		using hold_type = typename instantiate_list<hold,hold_args>::type;
		hold_type h;
		return h.apply([&](UNPARSED&...u_args){return this->_parse(ctx,u_args...);});
	}
};
template<typename...A1,typename...A2>
auto create_join_parser(auto&& p1, auto&& p2, TLIST<A1...>,TLIST<A2...>){
	using P1 = std::decay_t<decltype(p1)>;
	using P2 = std::decay_t<decltype(p2)>;
	if constexpr (std::is_same_v<TLIST<A2...>,TLIST<EOL>>){
		struct join_p : public parse_object {
			using active = active_t;
			using UNPARSED_LIST = TLIST<A1...>;
			[[no_unique_address]] P1 p1;
			[[no_unique_address]] P2 p2;
			join_p(P1&& p1, P2&& p2) :
				p1(std::forward<P1>(p1)),
				p2(std::forward<P2>(p2))
			{}
			void _undo(context& ctx){
				p2._undo(ctx);
				p1._undo(ctx);
			}
			bool _parse(context& ctx,A1&...a1) {
				return parse(ctx,p1,a1...) && match(ctx,p2);
			}
			bool _match(context& ctx) {
				hold_normal<A1...> h;
				return h.apply([self=this](context& ctx,A1&...a1) mutable {
						return self->_parse(ctx,a1...);
					},ctx);
			}
			bool dbg_inline() const {
				return true;
			}
		};
		auto ret = join_p{std::forward<P1>(p1),std::forward<P2>(p2)};
		return ret;
	}else if constexpr( std::is_same_v<TLIST<A1...>,TLIST<EOL>>){
		struct join_p : public parse_object {
			using active = active_t;
			using UNPARSED_LIST = TLIST<A2...>;
			[[no_unique_address]] P1 p1;
			[[no_unique_address]] P2 p2;
			join_p(P1&& p1, P2&& p2) :
				p1(std::forward<P1>(p1)),
				p2(std::forward<P2>(p2))
			{}
			void _undo(context& ctx){
				p2._undo(ctx);
				p1._undo(ctx);
			}
			bool _parse(context& ctx,A2&...a2) {
				return match(ctx,p1) && parse(ctx,p2,a2...);
			}
			bool _match(context& ctx) {
				hold_normal<A2...> h;
				return h.apply([self=this](context& ctx,A2&...a2) mutable {
						return self->_parse(ctx,a2...);
				},ctx);
			}
			bool dbg_inline() const {
				return true;
			}
		};
		auto ret = join_p{std::forward<P1>(p1),std::forward<P2>(p2)};
		return ret;
	}else{
		struct join_p : public parse_object {
			using active = active_t;
			using UNPARSED_LIST = TLIST<A1...,A2...>;
			[[no_unique_address]] P1 p1;
			[[no_unique_address]] P2 p2;
			join_p(P1&& p1, P2&& p2) :
				p1(std::forward<P1>(p1)),
				p2(std::forward<P2>(p2))
			{}
			void _undo(context& ctx){
				p2._undo(ctx);
				p1._undo(ctx);
			}
			bool _parse(context& ctx,A1&...a1,A2&...a2) {
				return parse(ctx,p1,a1...) && parse(ctx,p2,a2...);
			}
			bool _match(context& ctx) {
				hold_normal<A1...,A2...> h;
				return h.apply([self=this](context& ctx,A1&...a1,A2&...a2) mutable {
						return self->_parse(ctx,a1...,a2...);
				},ctx);
		
			}
			bool dbg_inline() const {
				return true;
			}
		};
		auto ret = join_p{std::forward<P1>(p1),std::forward<P2>(p2)};
		return ret;
	}

}


template<typename parser>
struct activated : public parser {
	using active = active_t;
	activated(parser&& self) : parser(self) {};
};

template<parser T> 
auto operator!(T&& nr) {
	using P = std::decay_t<T>;
	return activated<P>(std::forward<P>(nr));
}
template<typename parser>
struct forget : public parse_object {
	using UNPARSED_LIST = TLIST<EOL>;
	using active = active_f;

	parser p;

	forget(parser& op) : p(op) {};
	forget(parser&& op) : p(std::move(op)) {};
	
	bool _match(context& ctx){
		return match(ctx,p);
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
struct or_parser : public parse_object {
	using ret_type = typename or_helper<
		typename P1::UNPARSED_LIST,
		typename P2::UNPARSED_LIST>::type;
	using UNPARSED_LIST = TLIST<ret_type>;
	using active = active_t;

	[[no_unique_address]] P1 p1;
	[[no_unique_address]] P2 p2;
	or_parser(P1&& op1, P2&& op2) : p1(std::move(op1)), p2(std::move(op2)) {}
	or_parser(P1&& op1, const P2& op2) : p1(std::move(op1)), p2(op2) {}
	or_parser(const P1& op1, P2&& op2) : p1(op1), p2(std::move(op2)) {}
	or_parser(const P1& op1, const P2& op2) : p1(op1), p2(op2) {}

	bool _parse(context& ctx, ret_type& ret){
		auto attempt = [&]<parser T>(T& t) -> bool{
			if constexpr( rparser<T> ){
				using h_t = typename apply_list<hold_normal,typename T::UNPARSED_LIST>::type;
		
				h_t h;
				auto success = h.apply([&](context& ctx, auto&...args){
					return parse_or_undo(ctx,t,args...);
				},ctx);
				if(success){
					ret = h.apply([](auto&...args) -> ret_type{
						return ret_type{args...};
					});
				}
				return success;
			}else{
				return match_or_undo(ctx,t);
			}
		};
		return attempt(p1) || attempt(p2);
	}
	bool _match(context& ctx){
		using hold_type = typename instantiate_list<hold,UNPARSED_LIST>::type;
		hold_type h;
		return h.apply([&](auto&...u_args){return this->_parse(ctx,u_args...);});
	}
	bool dbg_inline() const{
		return true;
	}
};

template<parser P1, parser P2> requires rparser<P1> || rparser<P2>
parser auto operator|(P1&& p1, P2&& p2){
	using P1_t = std::decay_t<P1>;
	using P2_t = std::decay_t<P2>;

	return or_parser<P1_t,P2_t>{std::forward<P1_t>(p1), std::forward<P2_t>(p2)};
}
template<parser P1, parser P2> requires rparser<P1> || rparser<P2>
parser auto operator+(P1&& p1, P2&& p2){
	using P1_t = std::decay_t<P1>;
	using P2_t = std::decay_t<P2>;
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
			return match(ctx,first) && match(ctx,second);
		});
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
struct fr_parser_t : public parse_object {
	using UNPARSED_LIST = TLIST<ARGS...>;
	F_TYPE fds;
	fr_parser_t(F_TYPE&& fds) : fds(std::forward<F_TYPE>(fds)) {};
	bool _parse(context& ctx, ARGS&...args){
		return fds(ctx,args...);
	}
};

template<typename ... ARGS>
auto fr_parser(auto&& f){
	using F_TYPE = std::decay_t<decltype(f)>;
	using parser = fr_parser_t<F_TYPE,ARGS...>;
	return nr_parser<VOID,parser,ARGS...>({},parser{std::forward<F_TYPE>(f)});
}
template<typename...UNPARSED>
struct rpo : public parse_object{
	using f_type = std::function<bool(context&,UNPARSED&...)>;
	using UNPARSED_LIST = TLIST<UNPARSED...>;

	f_type f;

	rpo() = default;
	rpo(const rpo&) = delete;
	rpo(rpo&&) = delete;

	template<typename F> requires std::invocable<F,context&,UNPARSED&...>
	rpo(F&& f) {
		this->f = std::forward<F>(f);
	}

	template<parser T>
	void operator=(T&& p){
		f = [p = std::forward<T>(p)](context& ctx, auto&...up_args) mutable -> bool {
			if constexpr (rparser<T>){
				return parse(ctx,p,up_args...);
			}else{
				return match(ctx,p);
			}
		};
	}

	bool _parse(context& ctx, UNPARSED&...up_args){
		return f(ctx,up_args...);
	}
	bool _match(context& ctx) {
		hold_normal<UNPARSED...> h;
		return h.apply(f,ctx);
	}
	bool dbg_inline() const {
		return true;
	}
};
auto erase(parser auto&& parser){
	using prev_type = std::decay_t<decltype(parser)>;
	using ret_type = 
		typename apply_list<rpo,typename prev_type::UNPARSED_LIST>::type;
	ret_type ret = parser;
	return ret;
}
