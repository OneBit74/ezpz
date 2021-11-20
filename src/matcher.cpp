#include "matcher.hpp"
#include <fmt/core.h>

/* ret_parse_object<std::string_view> match(std::string_view pattern){ */
/* 	return r_parser<std::string_view>([=](auto& ctx,auto&& output){ */
/* 		const std::string str{pattern}; */
/* 		auto [regex_iter,b] = ctx.regex_cache.try_emplace(str,str); */
/* 		std::smatch match; */
/* 		const auto& c_input = ctx.input; */
/* 		std::regex_search(c_input.begin()+ctx.pos,c_input.end(),match,regex_iter->second); */
/* 		if(match.empty())return false; */
/* 		if(match.position() != 0)return false; */
/* 		auto ret = std::string_view{ */
/* 			c_input.begin() + ctx.pos, */
/* 			c_input.begin() + ctx.pos + match.length() */
/* 		}; */
/* 		output << ret; */
/* 		ctx.pos += match.length(); */
/* 		return true; */
/* 	},false,fmt::format("matching \"{}\"",pattern)); */
/* } */
/* ret_parse_object<std::string_view> until(std::string_view pattern){ */
/* 	return r_parser<std::string_view>([=](auto& ctx,auto&& output){ */
/* 		const std::string str{pattern}; */
/* 		auto [regex_iter,b] = ctx.regex_cache.try_emplace(str,str); */
/* 		std::smatch match; */
/* 		const auto& c_input = ctx.input; */
/* 		std::regex_search(c_input.begin()+ctx.pos,c_input.end(),match,regex_iter->second); */
/* 		if(match.empty())return false; */
/* 		ctx.pos += match.position(); */
/* 		auto ret = std::string_view{ */
/* 			c_input.begin() + ctx.pos, */
/* 			c_input.begin() + ctx.pos + match.length() */
/* 		}; */
/* 		/1* ctx.pos += match.length(); *1/ */
/* 		output << ret; */
/* 		return true; */
/* 	},false,fmt::format("until \"{}\"",pattern)); */
/* } */
