#pragma once
#include "ezpz/parse_object.hpp"
#include "ezpz/meta.hpp"
#include <variant>
#include <optional>
#include <tuple>

namespace ezpz{



template<typename consumer, parser P>
struct consume_p {
	using invoke_info = invoke_list<consumer,typename P::ezpz_output>;
	using invoke_args = typename invoke_info::args;

	static constexpr bool callable = !std::is_same_v<invoke_args,VOID>;
	static constexpr bool ref_callable = std::is_same_v<
		invoke_list<consumer,typename get_ref_list<
				typename P::ezpz_output
			>::type
		>,
		invoke_info
	>;
	static_assert(callable || !ref_callable,"consumer is callable with l-value references but not with r-value references");
	static_assert(callable,"consumer is not callable by any of the available values");

	using remaining_types = 
		typename pop_n<invoke_args::size,typename P::ezpz_output>::type;
	static constexpr bool returning_consumer = !std::is_same_v<typename invoke_info::ret,void>;
	using added_types = t_if_else<returning_consumer,
		  TLIST<typename invoke_info::ret>,
		  TLIST<>
	>::type;
	using ezpz_output = typename added_types::append<remaining_types>;
	using ezpz_prop = TLIST<dbg_inline>::append<typename get_prop_tag<P>::type>;

	[[no_unique_address]] P parent;
	[[no_unique_address]] consumer f;

	consume_p(auto&& f, auto&& parent) :
		parent(std::forward<P>(parent)),
		f(std::forward<consumer>(f))
	{};
	bool _parse(auto& ctx, auto&...up_args){
		using hold_args = typename get_decay_list<invoke_args>::type;
		using hold_type = typename instantiate_list<hold_normal,hold_args>::type;
		hold_type hold;
		if constexpr (returning_consumer){
			bool success = hold.apply([&](auto&, auto&...upper){
				return [&](auto&...args){
					return parse(ctx,parent,args...,upper...);
				};
			}(up_args...));
			if(!success)return false;

			assign_first(hold.move_into(f),up_args...);
			return true;
		}else{
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



template<parser LHS, parser RHS>
struct and_p {
	using L_ARGS = typename LHS::ezpz_output;
	using R_ARGS = typename RHS::ezpz_output;
	using ezpz_output = typename L_ARGS::template append<R_ARGS>;

	static_assert(!contains<typename get_prop_tag<LHS>::type,always_false>::value, "[ezpz][and_p] LHS will always fail so RHS is never entered");

	using ezpz_prop_success = typename t_if_else<
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
	using ezpz_prop = TLIST<dbg_inline>::append<ezpz_prop_success>;

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

		using front_t = typename instantiate_list<takefront_and_call, typename get_ref_list<L_ARGS>::type>::type;
		bool ret = front_t::call(cb_lhs,args...);
		if(!ret)return false;

		using back_t = typename instantiate_list<takeback_and_call, typename get_ref_list<L_ARGS>::type>::type;
		ret = back_t::call(cb_rhs,args...);
		return ret;
	}

};


template<parser parser_t>
struct forget {
	using ezpz_output = TLIST<>;
	using ezpz_prop = typename get_prop_tag<parser_t>::type::append<TLIST<dbg_inline>>;

	[[no_unique_address]] parser_t p;

	forget(auto&& op) : p(std::forward<parser_t>(op)) {};
	
	bool _parse(auto& ctx){
		return parse(ctx,p);
	}
};
template<parser T> 
auto operator!(T&& nr) {
	using P = std::decay_t<T>;
	return forget<P>(std::forward<P>(nr));
}

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
	using type = TLIST<typename instantiate_list<std::variant,typename dedup<TLIST<A...,B...>>::type>::type>;
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
	using ezpz_output = typename or_helper<
		typename P1::ezpz_output,
		typename P2::ezpz_output>::type;

	static_assert(!contains<typename get_prop_tag<P1>::type, always_true>::value, "[ezpz][or_p|] left alternative always accepts, right side is never entered");

	using ezpz_prop = typename t_if_else< 
		contains<typename get_prop_tag<P2>::type,always_true>::value,
		TLIST<always_true>,
		TLIST<>
	>::type::append<TLIST<dbg_inline>>;

	[[no_unique_address]] dont_store_empty<P1> p1;
	[[no_unique_address]] dont_store_empty<P2> p2;

	or_p(auto&& op1, auto&& op2) : p1(std::forward<P1>(op1)), p2(std::forward<P2>(op2)) {}

	template<typename T,bool undo>
	auto attempt(T& t, auto& ctx, auto&...ret) -> bool {
		using parser_t = std::decay_t<T>;
		if constexpr( rparser<parser_t> ){
			if constexpr (ezpz_output::size == 1){
				using h_t = typename apply_list<hold_normal,typename parser_t::ezpz_output>::type;
				using ret_type = typename ezpz_output::type;

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
						if constexpr (sizeof...(args) == 1 && is_variant<typename parser_t::ezpz_output::type>) {
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
auto operator*(P&& p, F&& consumer)
requires (!std::is_function_v<std::remove_pointer_t<std::decay_t<F>>>)
{
	using P_t = std::decay_t<P>;
	using F_t = std::decay_t<F>;
	return consume_p<F_t,P_t>{std::forward<F_t>(consumer), std::forward<P_t>(p)};
}
inline struct no_parser_p {
	using ezpz_output = TLIST<>;

	constexpr bool _parse(const auto&){
		return true;
	}
} no_parser;
template<parser P, typename F> requires (not parser<F>)
auto operator*(F&& consumer, P&& p) {
	return no_parser*consumer+p;
}
static_assert(std::is_same_v<invoke_list<std::function<void(int&,int&)>,TLIST<int&,int&>>::args,TLIST<int&,int&>>);
/* static_assert(std::is_same_v<invoke_list<std::function<void(int&&)>,TLIST<int>>::args,TLIST<int>>); */

template<typename F_TYPE, typename ... ARGS>
struct fr_parser_t {
	using ezpz_output = TLIST<ARGS...>;

	F_TYPE fds;
	fr_parser_t(F_TYPE&& fds) : fds(std::forward<F_TYPE>(fds)) {};
	bool _parse(auto& ctx, ARGS&...args) requires 
		requires(decltype(ctx) c, decltype(fds) f, ARGS&...as)
			{
				{f(c,as...)} -> std::same_as<bool>;
			} ||
		requires(decltype(ctx) c, decltype(*this) self,  decltype(fds) f, ARGS&...as)
			{
				{f(c,self,as...)} -> std::same_as<bool>;
			}
	{
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
template<context_c context_t, typename...OUTPUT>
struct rpo {
	using f_type = std::function<bool(context_t&,OUTPUT&...)>;
	using ezpz_output = TLIST<OUTPUT...>;
	using ezpz_prop = TLIST<dbg_inline>;

	f_type f;

	rpo() = default;
	rpo(const rpo&) = delete;
	rpo(rpo&&) = delete;

	template<typename F> requires std::invocable<F,context_t&,OUTPUT&...>
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
		static_assert(std::same_as<typename P_t::ezpz_output, ezpz_output>, "unexpected return-types of right-hand-side parser to ezpz::rpo");
		f = [p = std::forward<P_t>(p)](context_t& ctx, auto&...up_args) mutable -> bool {
			return parse(ctx,p,up_args...);
		};
	}

	bool _parse(context_t& ctx, OUTPUT&...up_args){
		return f(ctx,up_args...);
	}
};
template<typename...ARGS>
using basic_rpo = rpo<basic_context, ARGS...>;
auto erase(parser auto&& parser){
	using prev_type = std::decay_t<decltype(parser)>;
	using ret_type = 
		typename apply_list<rpo,typename prev_type::ezpz_output>::type;
	ret_type ret = parser;
	return ret;
}

template<typename ctx, typename...RET>
struct polymorphic_rpo_base {
	using ezpz_output = TLIST<RET...>;

	virtual bool _parse(ctx& ,RET&...) = 0;
};
template<parser parser>
class ref_p {
public:
	using ezpz_output = typename parser::ezpz_output;
	using ezpz_prop = typename get_prop_tag<parser>::type::append<TLIST<dbg_inline>>;

	parser* p;
	ref_p() = default;
	ref_p(ref_p&&) = default;
	ref_p(const ref_p&) = default;
	ref_p& operator=(ref_p&&) = default;
	ref_p& operator=(const ref_p&) = default;
	ref_p(parser& op) : p(&op) {}

	bool _parse(auto& ctx, auto&...args) {
		return parse(ctx,*p,args...);
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
