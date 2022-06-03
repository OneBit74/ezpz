#pragma once
#include <string>
#include <unordered_map>
#include <regex>
#include <iostream>
#include <cxxabi.h>
#include <cassert>
#include <ranges>
#include <string_view>
namespace ezpz{
	struct text_p;
	struct text_ci_p;
	 
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
	concept error_context_c = requires(T t){
		{
			t.describePosition(t.getPosition())
		} -> std::same_as<std::string>;
	};
	template<typename T>
	concept basic_context_c = context_c<T> && requires(T t){
		{
			t.token()
		} -> std::same_as<char>;
	};

	class min_context {
	public:
		std::string_view input;
		size_t pos = 0;

		min_context() = default;
		inline min_context(std::string_view str) : input(std::move(str)) {}

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
	inline void print_special_chars(std::string_view sv){
		for(char c : sv){
			switch(c){
				case '"':
					std::cout << "\\\"";
					break;
				case '\t':
					std::cout << "\\t";
					break;
				case '\r':
					std::cout << "\\r";
					break;
				case '\n':
					std::cout << "\\n";
					break;
				default:
					std::cout << c;
			}
		}
	}

	bool is_dbg_inline(auto&p){
		if constexpr( requires(decltype(p) p){p.dbg_inline();} ){
			return p.dbg_inline();
		}
		return false;
	}
	class basic_context : public min_context {
	public:
		using min_context::min_context;
		bool debug = false;
		size_t depth = 0;
		std::unordered_map<std::string,std::regex> regex_cache;
		std::vector<decltype(pos)> nl_pos;

		void advance(){
			min_context::advance();

			if( (nl_pos.empty() || pos > nl_pos.back()) && !done()){
				if(token() == '\n')nl_pos.push_back(pos);
			}
		}
		std::string describePosition(int pos){
			int line, col;
			if(nl_pos.empty()){
				line = 1;
				col = pos + 1;
			} else{
				auto iter = std::upper_bound(std::begin(nl_pos), std::end(nl_pos), pos);
				if(iter == std::begin(nl_pos)){
					line = 1;
					col = pos + 1;
				}else{
					iter = std::prev(iter);
					line = (iter - std::begin(nl_pos)) + 2;
					col = pos - *iter;
				}
			}

			std::ostringstream oss;
			oss << line << ":" << col;
			oss << '\n';
			size_t cur = pos-col+1;
			while(cur < input.size() && input[cur] != '\n'){
				oss << input[cur];
				++cur;
			}
			oss << '\n';
			for(int i = 0; i < col; ++i) oss << ' ';
			oss << '^';
			return oss.str();
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
				<< '\"';
			print_special_chars(std::string_view{input.begin()+prev_pos,input.begin()+pos});
			std::cout 
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
			print_special_chars(std::string_view{input.begin()+start_pos, input.begin()+end_pos});
			std::cout << '\"';
			std::cout << " " << type_name<decltype(parser)>();
			if constexpr(TLIST<ezpz::text_p,ezpz::text_ci_p>::template contains<std::decay_t<decltype(parser)>>){
				std::cout << "(\"";
				print_special_chars(parser.sv);
				std::cout << "\")";
			}
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

	template<std::ranges::forward_range R>
	struct forward_range_context {
		using iterator = std::ranges::iterator_t<R>;
		using end_iterator = decltype(std::end(std::declval<R>()));
		R range;

		iterator start;
		end_iterator end;
		iterator cur;

		forward_range_context(auto&& range_p) : range(std::forward<R>(range_p)), start(std::begin(range)), end(std::end(range)), cur(std::begin(range)) {}

		auto token(){
			return *cur;
		}
		iterator getPosition(){
			return cur;
		}
		void setPosition(iterator pos){
			cur = pos;
		}
		std::string describePosition(iterator pos){
			std::ostringstream oss;

			iterator cur = start;
			oss << '\n';
			while(cur != pos)oss << *(cur++) << ' ';
			int whitespace_size = oss.tellp();
			oss << *pos;
			int squigles_size = int(oss.tellp())-whitespace_size;
			++cur;
			while(cur != end)oss << ' ' << *(cur++);

			oss << '\n';
			for(int i = 0; i < whitespace_size-1; ++i)oss << ' ';
			for(int i = 0; i < squigles_size; ++i)oss << '~';

			return oss.str();
		}

		
		void notify_enter(auto&) {}
		void notify_leave(auto&, bool) {}

		bool done() const {return cur == end;}
		void advance() {
			++cur;
		};
	};

}
