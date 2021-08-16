#pragma once
#include "parse_object.hpp"
#include "r_parser.hpp"

inline struct optional_t {} optional;
inline struct any_t {} any;
inline struct plus_t {} plus;
inline struct not_t {} not_v;

auto operator>>(any_t,parse_object_ref rhs) -> ret_parse_object<std::string_view>;
auto operator>>(any_t,std::string_view rhs) -> parse_object_ref;

auto operator>>(optional_t,parse_object_ref rhs) -> parse_object_ref;
auto operator>>(optional_t,std::string_view text) -> parse_object_ref;

auto operator>>(plus_t, parse_object_ref rhs) -> parse_object_ref;
auto operator>>(plus_t,std::string_view text) -> parse_object_ref;

auto operator>>(not_t, parse_object_ref rhs) -> parse_object_ref;
auto operator>>(not_t, std::string_view rhs) -> parse_object_ref;

struct max_t {
	int val;
	auto operator>>(std::string_view sv) -> parse_object_ref;
	auto operator>>(parse_object_ref inner) -> parse_object_ref;
};
struct min_t {
	int val;
	auto operator>>(std::string_view sv) -> parse_object_ref;
	auto operator>>(parse_object_ref inner) -> parse_object_ref;
};
struct minmax_t {
	int val1,val2;
	auto operator>>(std::string_view sv) -> parse_object_ref;
	auto operator>>(parse_object_ref inner) -> parse_object_ref;
};

auto min(int val) -> min_t;
auto max(int val) -> max_t;
auto minmax(int val1, int val2) -> minmax_t;
auto exact(int val) -> minmax_t;
