#pragma once
#include "parse_object.hpp"
#include "r_parser.hpp"
#include "context.hpp"
#include "quantifiers.hpp"

ret_parse_object<std::string_view> match(std::string_view pattern);
ret_parse_object<std::string_view> until(std::string_view pattern);

auto text_parser(std::string_view sv) -> parse_object_ref;
auto text(const std::string_view& sv) -> parse_object_ref;

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
inline parse_object_ref digit = f_parser([](context& ctx){
	if(ctx.done())return false;
	if(std::isdigit(ctx.get())){
		++ctx.pos;
		return true;
	}
	return false;
});
inline auto graph_letter = r_parser([](context& ctx){
	if(ctx.done())return false;
	if(std::isgraph(ctx.get())){
		++ctx.pos;
		return true;
	}
	return false;
});
inline auto string = "\"" + (any >> (not_v >> "\"")) + "\"";

template<typename integer, int base>
auto number = r_parser<integer>([](context& ctx, auto&& output){
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
	integer ret = 0;
	bool invalid = true;
	while(!ctx.done() && std::isdigit(ctx.get())){
		invalid = false;
		ret *= base;
		ret += ctx.get()-'0';
		++ctx.pos;
	}
	if(invalid)return false;
	if(negative)ret = -ret;
	output << ret;
	return true;
});
template<typename integer>
auto& decimal = number<integer,10>;

