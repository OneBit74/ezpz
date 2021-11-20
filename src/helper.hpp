#pragma once
#include "parse_object.hpp"

inline struct print_t {
	static void print_text(std::string_view sv);
	parser auto  operator()(std::string_view text){
		auto ret =  f_parser([=](auto){
					print_text(text);
					return true;
				},true);
		return ret;
	};
} print;
inline parser auto fail = f_parser([](auto){return false;});
inline parser auto eoi = f_parser([](context& ctx){return ctx.done();});
