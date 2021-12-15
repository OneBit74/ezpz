#pragma once
#include <string>
#include <unordered_map>
#include <regex>
#include <iostream>
#include <cxxabi.h>
#include <cassert>
 
template<typename T>
std::string type_name()
{
	std::string tname = typeid(T).name();
	#if defined(__clang__) || defined(__GNUG__)
	int status;
	char *demangled_name = abi::__cxa_demangle(tname.c_str(), NULL, NULL, &status);
	if(status == 0)
	{
		tname = demangled_name;
		std::free(demangled_name);
	}
	#endif
	return tname;
}
 
template<typename T>
concept context_c = requires(T t){
	{
		t.done()
	} -> std::same_as<bool>;
	t.token();
	t.advance();
	t.setPosition(t.getPosition());
};
template<typename T>
concept basic_context_c = context_c<T> && requires(T t){
	{
		t.token()
	} -> std::same_as<char>;
};

class min_context {
public:
	std::string input;
	size_t pos = 0;

	min_context() = default;
	inline min_context(std::string str) : input(std::move(str)) {}

	inline char token() const {assert(!done());return input[pos];}
	inline bool done() const {return pos == input.size();}
	inline void advance() {
		++pos;
	};
	inline void setPosition(int pos) {
		this->pos = pos;
	}
	inline int getPosition() const {
		return pos;
	}
	inline void notify_enter(auto&) {}
	inline void notify_leave(auto&, bool) {}
};

class basic_context : public min_context {
public:
	using min_context::min_context;
	bool debug = false;
	size_t depth = 0;
	std::unordered_map<std::string,std::regex> regex_cache;


	bool is_dbg_inline(auto&p){
		if constexpr( requires(decltype(p) p){p.dbg_inline();} ){
			return p.dbg_inline();
		}
		return false;
	}
	inline void notify_leave(auto& parser, bool success, int prev_pos) {
		if(!debug || is_dbg_inline(parser)){
			return;
		}

		depth--;
		indent();
		if(!success){
			std::cout << "failed ";
		}else{
			std::cout << "accepted ";
		}
		std::cout 
			<< '\"' 
			<< std::string_view{input.begin()+prev_pos,input.begin()+pos} 
			<< '\"'
			<< std::endl;
	}
	int notify_enter(auto& parser) {
		if(!debug || is_dbg_inline(parser)){
			return 0;
		}

		indent();
		std::cout << "starting ";
		constexpr int lo = 7;
		constexpr size_t ro = 7;
		auto start_pos = std::max(0,int(pos)-lo);
		auto end_pos = std::min(input.size(),pos+ro);
		std::cout << '\"';
		std::cout << std::string_view{input.begin()+start_pos, input.begin()+end_pos};
		std::cout << '\"';
		std::cout << " " << type_name<decltype(parser)>();
		std::cout << std::endl;
		indent();
		std::cout << "          ";
		for(size_t i = start_pos; i <= end_pos; ++i){
			std::cout << (i == pos ? '^' : ' ');
		}
		std::cout << std::endl;
		depth++;

		return pos;
	}
	inline void indent() const {
		for(size_t i = 0; i < depth; ++i)std::cout << '\t';
	}
};
