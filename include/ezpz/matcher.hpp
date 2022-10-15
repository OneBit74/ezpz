#pragma once
#include "parse_object.hpp"
#include "context.hpp"
#include "quantifiers.hpp"
#include <regex>
#include <cctype>

namespace ezpz{

	struct ref_text_p {
		using active = active_f;
		using UNPARSED_LIST = TLIST<>;

		const std::string_view& sv;
		inline ref_text_p(const std::string_view& sv) : sv(sv) {}
		
		inline bool _parse(basic_context_c auto& ctx) {
			for(char c : sv){
				if(ctx.done())return false;
				char i = ctx.token();
				if(i != c)return false;
				ctx.advance();
			}
			return true;
		}
		inline auto _description(){
			return sv;
		}
	};
	template<typename F> requires std::is_invocable_r_v<std::string_view,F>
	struct fast_text_p {
		using active = active_f;
		using UNPARSED_LIST = TLIST<>;

		[[no_unique_address]] F f;
		inline fast_text_p(auto&& f) : f(std::forward<F>(f)) {}
		
		inline bool _parse(basic_context_c auto& ctx) {
			std::string_view sv = f();
			for(char c : sv){
				if(ctx.done())return false;
				char i = ctx.token();
				if(i != c)return false;
				ctx.advance();
			}
			return true;
		}
		inline auto _description(){
			return f();
		}
	};

	struct text_ci_p {
		using active = active_f;
		using UNPARSED_LIST = TLIST<>;

		std::string_view sv;
		constexpr text_ci_p(std::string_view sv) : sv(sv) {}
		
		inline bool _parse(basic_context_c auto& ctx) {
			for(char c : sv){
				if(ctx.done())return false;
				char i = std::tolower(ctx.token());
				if(i != c)return false;
				ctx.advance();
			}
			return true;
		}

		inline auto _description() {
			return sv;
		}
	};

	struct text_p {
		using active = active_f;
		using UNPARSED_LIST = TLIST<>;

		std::string_view sv;
		constexpr text_p(std::string_view sv) : sv(sv) {}
		
		inline bool _parse(basic_context_c auto& ctx) {
			for(char c : sv){
				if(ctx.done())return false;
				char i = ctx.token();
				if(i != c)return false;
				ctx.advance();
			}
			return true;
		}
		auto _description(){
			return sv;
		}
	};
	auto fast_text(auto&& f){
		using F_TYPE = std::decay_t<decltype(f)>;
		return fast_text_p<F_TYPE>{std::forward<F_TYPE>(f)};
	}
	inline parser auto text(const std::string_view& sv) {
		return ref_text_p{sv};
	}

	inline parser auto any(std::string_view rhs){
		return any(text_p(rhs));
	}
	inline parser auto notf(std::string_view rhs){
		return notf(text_p(rhs));
	}
	template<parser T>
	parser auto operator|(T&& rhs, std::string_view sv){
		return std::forward<T>(rhs) | text_p(sv);
	}
	template<parser T>
	parser auto operator|(std::string_view sv, T&& rhs){
		return text_p(sv) | std::forward<T>(rhs);
	}
	template<parser T>
	parser auto operator+(T&& rhs, std::string_view sv){
		return std::forward<T>(rhs) + text_p(sv);
	}
	template<parser T>
	parser auto operator+(std::string_view sv, T&& rhs){
		return text_p(sv) + std::forward<T>(rhs);
	}
	template<const char* data, size_t size>
	struct text_pc {
		using active = active_f;
		using UNPARSED_LIST = TLIST<>;
		static constexpr const char* end = data+size;
		inline bool _parse(basic_context_c auto& ctx) {
			auto cur = data;
			while(cur != end){
				auto c = *cur;
				if(ctx.done())return false;
				char i = ctx.token();
				if(i != c)return false;
				ctx.advance();
				++cur;
			}
			return true;
		}
	};
	inline constexpr parser auto operator "" _cip(const char* data, size_t size){
		return text_ci_p(std::string_view{data,size});
	}
	inline constexpr parser auto operator "" _p(const char* data, size_t size){
		return text_p(std::string_view{data,size});
	}

	inline struct ws_p {
		using active = active_f;
		using UNPARSED_LIST = TLIST<>;

		static constexpr auto _description = "whitespace";
		inline bool _parse(basic_context_c auto& ctx){
			while(!ctx.done()){
				switch(ctx.token()){
				case ' ':
				case '\t':
				case '\n':
				case '\r':
					ctx.advance();
					break;
				default:
					return true;
				}
			}
			return true;
		}
	} ws;
	inline parser auto digit = make_rpo<int>([](basic_context_c auto& ctx, int& ret){
		if(ctx.done())return false;
		if(std::isdigit(ctx.token())){
			ret = ctx.token();
			ctx.advance();
			return true;
		}
		return false;
	});
	inline parser auto graph_letter = make_rpo([](auto& ctx){
		if(ctx.done())return false;
		if(std::isgraph(ctx.token())){
			ctx.advance();
			return true;
		}
		return false;
	});
	inline parser auto string = "\"" + any(notf("\"")) + "\"";

	template<typename num_t, int base> 
	struct number_p{
		using active = active_f;
		using UNPARSED_LIST = TLIST<num_t>;

		static_assert(base <= 10);
		static_assert(base >= 2);
		
		static constexpr auto _description = "number";

		bool _parse(basic_context_c auto& ctx, num_t& ret) {
			if(ctx.done())return false;
			bool negative = false;
			if(ctx.token() == '-'){
				negative = true;
				ctx.advance();
			}
			if(ctx.done())return false;
			ret = 0;
			bool invalid = true;
			while(!ctx.done() && std::isdigit(ctx.token())){
				invalid = false;
				ret *= base;
				ret += ctx.token()-'0';
				ctx.advance();
			}
			if(invalid)return false;
			if constexpr(std::floating_point<num_t>){
				if(!ctx.done() && ctx.token() == '.'){
					ctx.advance();
					invalid = true;
					num_t alpha = 1;
					while(!ctx.done() && std::isdigit(ctx.token())){
						invalid = false;
						alpha /= base;
						ret += (ctx.token()-'0')*alpha;
						ctx.advance();
					}
					if(invalid)return false;
				}
			}
			if(negative)ret = -ret;
			return true;
		}
		bool _parse(auto& ctx){
			num_t ret;
			return _parse(ctx,ret);
		}
	};
	template<typename num_t, int base>
	auto number = number_p<num_t,base>{};

	template<typename integer>
	auto& decimal = number<integer,10>;

	inline struct alpha_p {
		using active = active_f;
		using UNPARSED_LIST = TLIST<char>;

		static constexpr auto _description = "alphabetic character";
		bool _parse(basic_context_c auto& ctx, char& c){
			if(ctx.done())return false;
			c = ctx.token();
			auto ret = isalpha(ctx.token());
			ctx.advance();
			return ret;
		}
	} alpha;

	template<typename F>
	struct accept_if_p {
		using active = active_f;
		using UNPARSED_LIST = TLIST<>;

		[[no_unique_address]] F f;

		accept_if_p(auto&& f) : f(std::forward<F>(f)) {}

		bool _parse(auto& ctx){
			if(ctx.done())return false;
			bool succ = f(ctx.token());
			if(succ){
				ctx.advance();
				return true;
			}else{
				return false;
			}
		}
	};

	template<typename F>
	auto accept_if(F&& f){
		using F_t = std::decay_t<F>;
		return accept_if_p<F_t>(std::forward<F_t>(f));
	}
	inline auto single = accept_if([](const auto&){return true;});

	template<typename F>
	auto accept_if_equal(F&& f){
		return accept_if([f=std::forward<std::decay_t<F>>(f)](const auto& token){
			return token == f;
		});
	}
	inline auto accept_if_equal(const char* s){
		return accept_if_equal(std::string_view{s});
	}
	template<typename T>
	struct token_eq_p {
		using active = active_f;
		using UNPARSED_LIST = TLIST<>;
		T token;

		bool _parse(auto& ctx){
			if(ctx.done())return false;
			if(ctx.token() == token){
				ctx.advance();
				return true;
			}
			return false;
		}
	};

	auto token(auto&& val){
		using val_t = typename std::decay_t<decltype(val)>;
		return token_eq_p<val_t>(std::forward<val_t>(val));
	}
	inline auto token(const char* val){
		return token(std::string_view{val});
	}

	inline auto regex(std::string_view pattern){
		return make_rpo<std::string_view>([=](basic_context& ctx, std::string_view& output){
			const std::string str{pattern};
			auto [regex_iter,b] = ctx.regex_cache.try_emplace(str,str);
			std::match_results<std::string_view::const_iterator> match;
			auto& c_input = ctx.input;
			std::regex_search(c_input.data()+ctx.pos,c_input.data()+ctx.input.size(),match,regex_iter->second,std::regex_constants::match_continuous);
			if(match.empty())return false;
			if(match.position() != 0)return false;
			output = std::string_view{
				c_input.begin() + ctx.pos,
				c_input.begin() + ctx.pos + match.length()
			};
			ctx.pos += match.length();
			return true;
		});
	}

	template<template<typename...> class, typename...>
	constexpr bool is_instantiation = false;

	template<template<typename...> class U, typename... T>
	constexpr bool is_instantiation<U, U<T...>> = true;

	static_assert(is_instantiation<consume_p, decltype(""_p*[](){})>);

	template<parser P>
	inline auto description(P& p){
		if constexpr(requires(){P::_description;}) {
			return p._description;
		}else if constexpr(requires(P p){p._description();}) {
			return p._description();
		}else {
			return type_name<P>();
		}
	}
}
