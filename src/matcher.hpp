#pragma once
#include "parse_object.hpp"
/* #include "r_parser.hpp" */
#include "context.hpp"
#include "quantifiers.hpp"
#include <fmt/core.h>
#include <regex>

/* ret_parse_object<std::string_view> match(std::string_view pattern); */
/* ret_parse_object<std::string_view> until(std::string_view pattern); */

inline parser auto text_parser(std::string_view sv) {
	return f_parser([=]
		(context& ctx){
			for(char c : sv){
				if(ctx.done())return false;
				char i = ctx.get();
				if(i != c)return false;
				++ctx.pos;
			}
			return true;
		}
	);
}
inline parser auto text(const std::string_view& sv) {
	return f_parser([&](context& ctx){
		for(size_t i = 0; i < sv.size(); ++i){
			if(sv[i] != ctx.get())return false;
			++ctx.pos;
		}
		return true;
	});
}

inline parser auto operator>>(any_t,std::string_view rhs){
	return any >> text_parser(rhs);
}
inline parser auto operator>>(not_t, std::string_view rhs){
	return not_v >> text_parser(rhs);
}
/* inline parser auto operator>>(optional_t,std::string_view text) { */
/* 	return optional >> text_parser(text); */
/* } */
template<parser T>
parser auto operator|(T&& rhs, std::string_view sv){
	return std::forward<T>(rhs) | text_parser(sv);
}
template<parser T>
parser auto operator|(std::string_view sv, T&& rhs){
	return text_parser(sv) | std::forward<T>(rhs);
}
template<parser T>
parser auto operator+(T&& rhs, std::string_view sv){
	return std::forward<T>(rhs) + text_parser(sv);
}
template<parser T>
parser auto operator+(std::string_view sv, T&& rhs){
	return text_parser(sv) + std::forward<T>(rhs);
}
inline auto operator "" _p(const char* s, size_t len){
	return text_parser(std::string_view{s,len});
}

inline auto ws = f_parser([](context& ctx){
	while(!ctx.done()){
		switch(ctx.get()){
		case ' ':
		case '\t':
		case '\n':
			++ctx.pos;
			break;
		default:
			return true;
		}
	}
	return true;
});
inline parser auto digit = f_parser([](context& ctx){
	if(ctx.done())return false;
	if(std::isdigit(ctx.get())){
		++ctx.pos;
		return true;
	}
	return false;
});
inline parser auto graph_letter = f_parser([](context& ctx){
	if(ctx.done())return false;
	if(std::isgraph(ctx.get())){
		++ctx.pos;
		return true;
	}
	return false;
});
inline parser auto string = "\"" + (any >> (not_v >> "\"")) + "\"";

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

inline struct alpha_p : public parse_object {
	bool _match(context& ctx){
		if(ctx.done())return false;
		auto ret = isalpha(ctx.get());
		ctx.pos++;
		return ret;
	}
} alpha;
inline struct single_p : public parse_object {
	bool _match(context& ctx){
		if(ctx.done())return false;
		ctx.pos++;
		return true;
	}
} single;

inline auto regex(std::string_view pattern){
	return fr_parser<std::string_view>([=](context& ctx, std::string_view& output){
		const std::string str{pattern};
		auto [regex_iter,b] = ctx.regex_cache.try_emplace(str,str);
		std::smatch match;
		const auto& c_input = ctx.input;
		std::regex_search(c_input.begin()+ctx.pos,c_input.end(),match,regex_iter->second,std::regex_constants::match_continuous);
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

/* auto operator|(std::string_view sv1, std::string_view sv2){ */
/* 	return text_parser(sv1) | text_parser(sv2); */
/* } */
/* template<parser P> */
/* auto capture(parser P&& p){ */
/* 	return fr_parser<std::string_view>( */
/* 			[p=std::forward<P>(p)](context& ctx, std::string_view& sv){ */
/* 				auto start = ctx.pos; */
/* 				auto ret = match(ctx,p); */
/* 				if(ret){ */
/* 					auto delta = ctx.pos-start; */
/* 					sv = std::string_view{ctx.input.c_str()+start,delta}; */
/* 				} */
/* 				return ret; */
/* 			}); */
/* } */
