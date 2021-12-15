#pragma once
#include "parse_object.hpp"
#include "context.hpp"
#include "quantifiers.hpp"
#include <regex>


struct text_parser {
	using active = active_f;
	using UNPARSED_LIST = TLIST<EOL>;

	std::string_view sv;
	inline text_parser(std::string_view sv) : sv(sv) {}
	
	inline bool _parse(basic_context_c auto& ctx) {
		for(char c : sv){
			if(ctx.done())return false;
			char i = ctx.token();
			if(i != c)return false;
			ctx.advance();
		}
		return true;
	}
};
inline parser auto text(const std::string_view& sv) {
	return make_rpo([&](auto& ctx){
		for(size_t i = 0; i < sv.size(); ++i){
			if(sv[i] != ctx.token())return false;
			ctx.advance();
		}
		return true;
	});
}

inline parser auto any(std::string_view rhs){
	return any(text_parser(rhs));
}
inline parser auto notf(std::string_view rhs){
	return notf(text_parser(rhs));
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
inline auto operator "" _p(const char* s, size_t len){
	return text_parser(std::string_view{s,len});
}

inline struct ws_t {
	using active = active_f;
	using UNPARSED_LIST = TLIST<EOL>;

	inline bool _parse(basic_context_c auto& ctx){
		while(!ctx.done()){
			switch(ctx.token()){
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
	}
} ws;
inline parser auto digit = make_rpo([](basic_context_c auto& ctx){
	if(ctx.done())return false;
	if(std::isdigit(ctx.token())){
		++ctx.pos;
		return true;
	}
	return false;
});
inline parser auto graph_letter = make_rpo([](auto& ctx){
	if(ctx.done())return false;
	if(std::isgraph(ctx.token())){
		++ctx.pos;
		return true;
	}
	return false;
});
inline parser auto string = "\"" + any(notf("\"")) + "\"";

template<typename num_t, int base> 
struct number_parser {
	using active = active_f;
	using UNPARSED_LIST = TLIST<num_t>;

	static_assert(base <= 10);
	static_assert(base >= 2);
	
	bool _parse(basic_context_c auto& ctx, num_t& ret) {
		if(ctx.done())return false;
		bool negative = false;
		if(ctx.token() == '-'){
			negative = true;
			ctx.advance();
		}else if(ctx.token() == '+'){
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
				++ctx.pos;
				invalid = true;
				num_t alpha = 1;
				while(!ctx.done() && std::isdigit(ctx.token())){
					invalid = false;
					alpha /= base;
					ret += (ctx.token()-'0')*alpha;
					++ctx.pos;
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
auto number = number_parser<num_t,base>{};

template<typename integer>
auto& decimal = number<integer,10>;

inline struct alpha_p {
	using active = active_f;
	using UNPARSED_LIST = TLIST<char>;

	bool _parse(basic_context_c auto& ctx, char& c){
		if(ctx.done())return false;
		c = ctx.token();
		auto ret = isalpha(ctx.token());
		ctx.advance();
		return ret;
	}
} alpha;
inline struct single_p {
	using active = active_f;
	using UNPARSED_LIST = TLIST<char>;

	bool _parse(auto& ctx, char& c){
		if(ctx.done())return false;
		c = ctx.token();
		ctx.advance();
		return true;
	}
} single;

auto token(auto&& val){
	return make_rpo([=](auto& ctx){
		auto ret = ctx.token() == val;
		ctx.advance();
		return ret;
	});
}

inline auto regex(std::string_view pattern){
	return make_rpo<std::string_view>([=](basic_context& ctx, std::string_view& output){
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
