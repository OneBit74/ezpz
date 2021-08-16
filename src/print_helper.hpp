#pragma once
#include "parse_object.hpp"

struct print_t {
	parse_object_ref operator()(std::string_view text);
} print;
parse_object_ref fail = f_parser([](auto){return false;});
parse_object_ref eoi = f_parser([](context& ctx){return ctx.done();});
