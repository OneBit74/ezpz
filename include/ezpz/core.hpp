#pragma once
#include "ezpz/parse_object.hpp"
#include "ezpz/meta.hpp"
#include <variant>
#include <optional>
#include <tuple>

namespace ezpz{

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
struct consume_p {
	using UNPARSED_LIST = TLIST<UNPARSED...>;
	using active = active_f;

	[[no_unique_address]] REM parent;
	[[no_unique_address]] unp f;

	consume_p(auto&& f, auto&& parent) :
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
		if constexpr (!std::is_same_v<typename unp::RET,void>){
			using hold_args = typename get_decay_list<
					typename unp::ARGS>::type;
			using hold_type = typename instantiate_list<hold_normal,hold_args>::type;
			hold_type hold;

			bool success = hold.apply([&](auto&, auto&...upper){
				return [&](auto&...args){
					return parse(ctx,parent,args...,upper...);
				};
			}(up_args...));
			if(!success)return false;

			assign_first(hold.move_into(f),up_args...);
			return true;
		}else{
			using hold_args = typename get_decay_list<typename unp::ARGS>::type;
			using hold_type = typename instantiate_list<hold_normal,hold_args>::type;
			hold_type hold;
			bool success = hold.apply([&](auto&...args){
				return parse(ctx,parent,args...);
			},up_args...);
			if(!success)return false;

			hold.move_into(f);

			return true;
		}
	}
};

template<typename...ARGS>
struct takefront_and_call{
	static auto call(auto&& target, ARGS&&...args, auto&&...){
		return target(args...);
	}
};

template<typename...ARGS>
struct takeback_and_call {
	static auto call(auto&& target, ARGS&&..., auto&&...args){
		return target(args...);
	}
};


template<parser T>
constexpr bool should_forget = std::is_same_v<typename T::active,active_f>;

template<parser LHS, parser RHS>
struct and_p {
	using L_ARGS = typename t_if_else<
		should_forget<LHS>,
		TLIST<>,
		typename LHS::UNPARSED_LIST
	>::type;
	using R_ARGS = typename t_if_else<
		should_forget<RHS>,
		TLIST<>,
		typename RHS::UNPARSED_LIST
	>::type;
	using UNPARSED_LIST = typename L_ARGS::append<R_ARGS>;
	using active = active_t;
	using ezpz_prop = typename t_if_else<
		contains<typename get_prop_tag<LHS>::type, always_true>::value
		&& contains<typename get_prop_tag<RHS>::type, always_true>::value,
		TLIST<always_true>,
		typename t_if_else<
			contains<typename get_prop_tag<LHS>::type, always_false>::value
			|| contains<typename get_prop_tag<RHS>::type, always_false>::value,
			TLIST<always_false>,
			TLIST<>
		>::type
	>::type;

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
		bool ret;
		if constexpr (should_forget<LHS>){
			ret = parse(ctx,lhs.get());
		}else{
			using front_t = typename instantiate_list<takefront_and_call, typename get_ref_list<L_ARGS>::type>::type;
			ret = front_t::call(cb_lhs,args...);
		}
		if(!ret)return false;
		if constexpr (should_forget<RHS>){
			ret = parse(ctx,rhs.get());
		} else {
			using back_t = typename instantiate_list<takeback_and_call, typename get_ref_list<L_ARGS>::type>::type;
			ret = back_t::call(cb_rhs,args...);
		}
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
struct activated {
	using UNPARSED_LIST = typename parser::UNPARSED_LIST;
	using active = active_t;

	parser p;
	activated(auto&& p) : p(std::forward<parser>(p)) {};

	bool dbg_inline(){
		return true;
	}
	bool _parse(auto& ctx, auto&...args){
		return parse(ctx,p,args...);
	}
};

template<parser T> 
auto operator!(T&& nr) {
	using P = std::decay_t<T>;
	return activated<P>(std::forward<P>(nr));
}
template<parser parser_t>
struct forget {
	using UNPARSED_LIST = TLIST<>;
	using active = active_f;

	[[no_unique_address]] parser_t p;

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
			L::size == 1,
			L,
			TLIST< typename apply_list<std::tuple,L>::type>
	>::type;
};
template<typename T>
struct inline_variant {
	using type = T;
};
template<typename ... ARGS>
struct inline_variant<TLIST<std::variant<ARGS...> > > {
	using type = TLIST<std::variant<ARGS...>>;
};
template<typename T>
struct inline_variant<TLIST<std::optional<T> > > {
	using type = TLIST<std::variant<T>>;
};
template<typename T>
struct get_variant_args {
	using type = TLIST<T>;
};

template<typename ... ARGS>
struct get_variant_args<std::variant<ARGS...>> {
	using type = TLIST<ARGS...>;
};
template<typename L1, typename L2>
struct or_helper {
	using NL1 = typename inline_variant<typename inline_tuple<L1>::type>::type;
	using NL2 = typename inline_variant<typename inline_tuple<L2>::type>::type;
	static constexpr bool uncertainty = is_variant<typename NL1::type> || is_variant<typename NL2::type> || is_optional<typename NL1::type> || is_optional<typename NL2::type>;
	using merge = typename append_list<typename get_variant_args<typename NL1::type>::type, typename get_variant_args<typename NL2::type>::type >::type;
	using non_empty = typename remove_t<merge,std::tuple<>>::type;
	using non_dup = typename dedup<non_empty>::type;
	using type = 
		typename t_if_else<
			std::is_same_v<NL1,NL2> && NL1::size == 1,
			TLIST<typename NL1::type>,
			typename t_if_else<
				non_dup::size == 1,
				TLIST<
					typename t_if_else<uncertainty,
						std::optional<typename non_dup::type>,
						typename non_dup::type
					>::type
				>,
				TLIST<typename apply_list<std::variant,non_dup>::type>
			>::type
		>::type;
	/* print_types<NL1,NL2,merge,non_empty,non_dup,L1,L2,type> asd; */
};

template<typename...A, typename...B> 
struct or_helper<TLIST<std::variant<A...>>,TLIST<std::variant<B...>>> {
	using type = typename instantiate_list<std::variant,typename dedup<TLIST<A...,B...>>::type>::type;
	print_types<type,A...,B...> asd;
};
template<typename T> requires (!std::same_as<T,TLIST<>>)
struct or_helper<T,TLIST<>> {
	using inner = typename t_if_else<
			T::size == 1,
			typename T::type,
			typename apply_list<std::tuple,T>::type
		>::type;
	using type = TLIST<std::optional<inner>>;
};
template<typename T> requires (!std::same_as<T,TLIST<>>)
struct or_helper<TLIST<>,T> {
	using type = typename or_helper<T,TLIST<>>::type;
};
template<typename T>
struct or_helper<T,T> {
	using type = T;
};

/* template<typename A..., typename B> */
/* struct variant_merge<std::variant<A...>,std::variant<B...>> { */
	
/* }; */
/* template<typename...A, typename...B> */ 
/* struct or_helper<TLIST<A...>,TLIST<B...>> { */
/* 	using TL = typename t_if_else<sizeof...(A) == 1, */
/* 			typename TLIST<A...>::type, */
/* 			std::tuple<A...> */
/* 		>::type; */
/* 	using TR = typename t_if_else<sizeof...(B) == 1, */
/* 			typename TLIST<B...>::type, */
/* 			std::tuple<B...> */
/* 		>::type; */
/* 	using VM = typename variant_merge<TL,TR>::type; */
/* 	using type = typename t_if_else<std::is_same_v<TL,TR>, */
/* 		  TLIST<TL>, */
/* 		  typename t_if_else<!std::is_same */

/* }; */


/* static_assert(std::same_as<or_helper<TLIST<EOL>,TLIST<EOL>>::type,TLIST<EOL>>); */
/* static_assert(std::same_as<or_helper<TLIST<int>,TLIST<EOL>>::type,TLIST<std::optional<int>>>); */
/* static_assert(std::same_as<or_helper<TLIST<EOL>,TLIST<int>>::type,TLIST<std::optional<int>>>); */
/* static_assert(std::same_as<or_helper<TLIST<int>,TLIST<int>>::type,TLIST<int>>); */
/* static_assert(std::same_as<or_helper<TLIST<int>,TLIST<float>>::type,TLIST<std::variant<int,float>>>); */
/* static_assert(std::same_as<or_helper<TLIST<std::variant<int,float>>,TLIST<float>>::type,TLIST<std::variant<int,float>>>); */

auto& get_first(auto& first, auto&...){
	return first;
}

template<parser P1, parser P2>
struct or_p {
	using UNPARSED_LIST = typename or_helper<
		typename P1::UNPARSED_LIST,
		typename P2::UNPARSED_LIST>::type;
	using active = active_t;
	using ezpz_prop = typename t_if_else< 
		contains<typename get_prop_tag<P2>::type,always_true>::value,
		TLIST<always_true>,
		TLIST<>
	>::type;

	[[no_unique_address]] dont_store_empty<P1> p1;
	[[no_unique_address]] dont_store_empty<P2> p2;

	or_p(auto&& op1, auto&& op2) : p1(std::forward<P1>(op1)), p2(std::forward<P2>(op2)) {}

	template<typename T,bool undo>
	auto attempt(T& t, auto& ctx, auto&...ret) -> bool {
		using parser_t = std::decay_t<T>;
		if constexpr( rparser<parser_t> ){
			if constexpr (UNPARSED_LIST::size == 1){
				using h_t = typename apply_list<hold_normal,typename parser_t::UNPARSED_LIST>::type;
				using ret_type = typename UNPARSED_LIST::type;

				h_t h;
				auto success = h.apply([&](auto&...args){
					if constexpr(undo){
						return parse_or_undo(ctx,t,args...);
					}else{
						return parse(ctx,t,args...);
					}
				});
				if(success){
					get_first(ret...) = h.apply([](auto&...args) -> ret_type{
						if constexpr (sizeof...(args) == 1 && is_variant<typename parser_t::UNPARSED_LIST::type>) {
							return std::visit([](auto&& inner){
									return ret_type{std::forward<std::decay_t<decltype(inner)>>(inner)};
							}, std::move(args)...);
						}else {
							return ret_type{std::move(args)...};
						}
					});
				}
				return success;
			}else{
				if constexpr(undo){
					return parse_or_undo(ctx,t,ret...);
				}else{
					return parse(ctx,t,ret...);
				}
			}
		}else{
			if constexpr(undo){
				return parse_or_undo(ctx,t);
			}else{
				return parse(ctx,t);
			}
		}
	}
	bool _parse(auto& ctx, auto&... ret){
		return attempt<P1,true>(p1.get(),ctx,ret...) || attempt<P2,false>(p2.get(),ctx,ret...);
	}
	bool dbg_inline() const{
		return true;
	}
};

template<parser P1, parser P2> 
parser auto operator|(P1&& p1, P2&& p2){
	using P1_t = std::decay_t<P1>;
	using P2_t = std::decay_t<P2>;

	static_assert(!contains<typename get_prop_tag<P1_t>::type, always_true>::value, "[ezpz] [operator|] left alternative always accepts, right side is never entered");

	return or_p<P1_t,P2_t>{std::forward<P1_t>(p1), std::forward<P2_t>(p2)};
}
template<parser P1, parser P2> 
parser auto operator+(P1&& p1, P2&& p2){
	using P1_t = std::decay_t<P1>;
	using P2_t = std::decay_t<P2>;
	return and_p<P1_t,P2_t>{std::forward<P1_t>(p1),std::forward<P2_t>(p2)};
}
template<parser P>
auto operator*(P&& p, auto func)
requires std::is_function_v<std::remove_pointer_t<decltype(func)>> 
{
	using P_t = std::decay_t<P>;
	using F_t = decltype(func);
	return std::forward<P_t>(p)*
		[=](auto&&...args)
		requires std::is_invocable_v<F_t,decltype(args)...>
		{
			if constexpr ( std::is_same_v<void,decltype(func(args...))> ) {
				func(args...);
			}else{
				return func(args...);
			}
		};
}
template<parser P, typename F>
auto operator*(P&& p, F&& unparser)
requires (!std::is_function_v<std::remove_pointer_t<std::decay_t<F>>>)
{
	using P_t = std::decay_t<P>;
	using invoke_info = invoke_list<F,typename P_t::UNPARSED_LIST>;
	using invoke_args = typename invoke_info::args;
	/* print_types<typename P::UNPARSED_LIST,invoke_args> asd; */
	static constexpr bool callable = !std::is_same_v<invoke_args,VOID>;
	if constexpr(!callable){
		static constexpr bool not_ref_callable = std::is_same_v<
			invoke_list<F,typename get_ref_list<
					typename P_t::UNPARSED_LIST
				>::type
			>,
			invoke_info
		>;
		static_assert(not_ref_callable,"unparser callback callable with l-value references but not with r-value references");
	}
	static_assert(callable,"unparser callback is not callable by any of the available values");
	using remaining_types = 
		typename pop_n<invoke_args::size,typename P_t::UNPARSED_LIST>::type;
	using F_t = std::decay_t<F>;
	using F_TYPE = f_wrapper<F_t,typename invoke_info::ret,typename invoke_info::args>;
	if constexpr (!std::is_same_v<typename invoke_info::ret,void> && !std::is_same_v<typename invoke_info::ret,VOID>){
		using ret_args = 
			typename append_list<TLIST<F_TYPE,P_t,typename invoke_info::ret>,remaining_types>::type;//TODO reorder return type to back
		using ret_type = 
			typename apply_list<consume_p,ret_args>::type;
		return ret_type(F_TYPE{std::forward<F_t>(unparser)},std::forward<P>(p));
	}else{
		using ret_args = 
			typename append_list<TLIST<F_TYPE,P_t>,remaining_types>::type;
		using ret_type = 
			typename apply_list<consume_p,ret_args>::type;
		return ret_type(F_TYPE{std::forward<F_t>(unparser)},std::forward<P>(p));
	}
}
inline struct no_parser_p {
	using UNPARSED_LIST = TLIST<>;
	using active = active_f;
	constexpr bool _parse(const auto&){
		return true;
	}
} no_parser;
template<parser P, typename F> requires (not parser<F>)
auto operator*(F&& unparser, P&& p) {
	return no_parser*unparser+p;
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
		if constexpr (requires(decltype(ctx) ctx, decltype(*this) self, ARGS...args){fds(ctx,self,args...);}){
			return fds(ctx,*this,args...);
		}else{
			return fds(ctx,args...);
		}
	}
};

template<typename ... ARGS>
auto make_rpo(auto&& f){
	using F_TYPE = std::decay_t<decltype(f)>;
	using parser = fr_parser_t<F_TYPE,ARGS...>;
	return parser{std::forward<F_TYPE>(f)};
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
	explicit rpo(F&& f) {
		this->f = std::forward<F>(f);
	}
	/* template<parser T> */ 
	/* explicit rpo(T&& f) { */
	/* 	operator=(std::forward<std::decay_t<T>>(f)); */
	/* } */

	template<parser T>
	void operator=(T&& p){
		using P_t = std::decay_t<T>;
		static_assert(std::same_as<typename P_t::UNPARSED_LIST, UNPARSED_LIST>, "unexpected return-types of right-hand-side parser to ezpz::rpo");
		f = [p = std::forward<P_t>(p)](context_t& ctx, auto&...up_args) mutable -> bool {
			return parse(ctx,p,up_args...);
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

template<typename ctx, typename...RET>
struct polymorphic_rpo_base {
	using UNPARSED_LIST = TLIST<RET...>;
	using active = active_f;

	virtual bool _parse(ctx& ,RET&...) = 0;
};
template<parser parser>
class ref_p {
public:
	using active = typename parser::active;
	using UNPARSED_LIST = typename parser::UNPARSED_LIST;

	parser* p;
	ref_p() = default;
	ref_p(ref_p&&) = default;
	ref_p(const ref_p&) = default;
	ref_p& operator=(ref_p&&) = default;
	ref_p& operator=(const ref_p&) = default;
	ref_p(parser& op) : p(&op) {}

	void _undo(auto& ctx) {
		undo(ctx,*p);
	}
	bool _parse(auto& ctx, auto&...args) {
		return parse(ctx,*p,args...);
	}
	bool dbg_inline(){
		return true;
	}
};

parser auto ref(auto* p){
	using inner = std::decay_t<decltype(p)>;
	return ref_p<std::remove_pointer_t<inner>>{*p};
}
parser auto ref(auto& p){
	using inner = std::decay_t<decltype(p)>;
	return ref_p<inner>{p};
}
template<typename ctx, typename...RET>
using polymorphic_rpo = ref_p<polymorphic_rpo_base<ctx,RET...>>;

template<typename P, typename ctx, typename...RET>
struct polymorphic_rpo_derived : public polymorphic_rpo_base<ctx,RET...> {
	[[no_unique_address]] P p;
	polymorphic_rpo_derived(auto&& p) : p(std::forward<P>(p)) {}
	bool _parse(ctx& c, RET&...args) override {
		return parse(c,p,args...);
	}
};
template<typename ctx, typename...RET>
parser auto make_poly(auto&& parser){
	using P_t = std::decay_t<decltype(parser)>;
	return polymorphic_rpo_derived<P_t,ctx,RET...>{std::forward<P_t>(parser)};
}

}
