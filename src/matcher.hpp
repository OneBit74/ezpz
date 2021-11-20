#pragma once
#include "parse_object.hpp"
/* #include "r_parser.hpp" */
#include "context.hpp"
#include "quantifiers.hpp"
#include <fmt/core.h>

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
		},false,fmt::format("text \"{}\"",sv)
	);
}
inline parser auto text(const std::string_view& sv) {
	return f_parser([&](context& ctx){
		for(size_t i = 0; i < sv.size(); ++i){
			if(sv[i] != ctx.get())return false;
			++ctx.pos;
		}
		return true;
	},false,"dynamic text");
}

inline parser auto operator>>(any_t,std::string_view rhs){
	return any >> text_parser(rhs);
}
inline parser auto operator>>(not_t, std::string_view rhs){
	return not_v >> text_parser(rhs);
}
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

/* template<typename integer, int base> */
/* auto number = r_parser<integer>([](context& ctx, auto&& output){ */
/* 	static_assert(base <= 10); */
/* 	static_assert(base >= 2); */
/* 	if(ctx.done())return false; */
/* 	bool negative = false; */
/* 	if(ctx.get() == '-'){ */
/* 		negative = true; */
/* 		++ctx.pos; */
/* 	}else if(ctx.get() == '+'){ */
/* 		++ctx.pos; */
/* 	} */
/* 	if(ctx.done())return false; */
/* 	integer ret = 0; */
/* 	bool invalid = true; */
/* 	while(!ctx.done() && std::isdigit(ctx.get())){ */
/* 		invalid = false; */
/* 		ret *= base; */
/* 		ret += ctx.get()-'0'; */
/* 		++ctx.pos; */
/* 	} */
/* 	if(invalid)return false; */
/* 	if(negative)ret = -ret; */
/* 	output << ret; */
/* 	return true; */
/* }); */
/* template<typename integer> */
/* auto& decimal = number<integer,10>; */

