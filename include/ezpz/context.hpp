#pragma once
#include <string>
#include <unordered_map>
#include <regex>
#include <iostream>
#include <cassert>
#include <ranges>
#include <string_view>
#include "fmt/core.h"
namespace ezpz{
	struct text_p;
	struct text_ci_p;

	 
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
		bool error_mode = false;

		void advance(){
			min_context::advance();

			if( (nl_pos.empty() || pos > nl_pos.back()) && !done()){
				if(token() == '\n')nl_pos.push_back(pos);
			}
		}
		std::pair<int,int> getLineCol(int pos){
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
			return {line,col};
		}
		std::string describePosition(int pos, int end_pos){
			assert((size_t)pos <= input.size());
			assert((size_t)end_pos <= input.size());

			auto [line,col] = getLineCol(pos);
			std::ostringstream oss;
			size_t cur = pos-col+1;
			while(cur < input.size() && input[cur] != '\n'){
				oss << input[cur];
				++cur;
			}
			oss << '\n';
			for(int i = 0; i < col-1; ++i) oss << ' ';
			oss << '^';
			for(int i = 1; i < end_pos - pos; ++i){
				if(input[i+pos] == '\n')oss << '\n';
				oss << '~';
			}
			return oss.str();
		}

		static constexpr int lo = 14;
		static constexpr size_t ro = 7;
		inline void notify_leave(auto& parser, bool success, int prev_pos) {
			if(!debug || is_dbg_inline(parser)){
				return;
			}

			depth--;
			for(int i = 0; i < indent(); ++i){
				std::cout << ' ';
			}
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

			int start_pos = std::max(0,int(pos)-lo);
			int end_pos = std::min(input.size(),pos+ro);
			std::cout << '\"';
			print_special_chars(std::string_view{input.begin()+start_pos, input.begin()+end_pos});
			std::cout << '\"';

			for(int i = 0; i < indent()-(2+end_pos-start_pos); ++i){
				std::cout << ' ';
			}
			std::cout << "starting ";
			std::cout << " " << type_name<decltype(parser)>();
			if constexpr(TLIST<ezpz::text_p,ezpz::text_ci_p>::template contains<std::decay_t<decltype(parser)>>){
				std::cout << "(\"";
				print_special_chars(parser.sv);
				std::cout << "\")";
			}
			std::cout << std::endl;
			/* std::cout << "          "; */
			std::cout << ' ';
			for(int i = start_pos; i <= end_pos; ++i){
				std::cout << (i == (int)pos ? '^' : ' ');
			}
			std::cout << std::endl;
			depth++;

			return pos;
		}
		inline int indent() const {
			return depth*5 + lo + ro + 4;
		}

		std::optional<std::string> resource_name;
		void error(auto& candidates){
			error_mode = true;
			if(!candidates.empty()){
				pos = candidates[0].start_pos;
			}
			auto [line,col] = getLineCol(pos);
			if(resource_name)fmt::print("{}:",*resource_name);

			fmt::print("{: >3}:{: >2} ", line, col);

			if(candidates.size() == 1)fmt::print("expected ");
			else if(candidates.size() > 1)fmt::print("did you mean ");

			for(size_t i = 0; i < candidates.size(); ++i){
				if(i > 0)fmt::print(", ");
				/* fmt::print("{}", candidates[i].parser_description); */
				if(candidates[i].parser_description.size() == 1){
					fmt::print("\'");
				}else{
					fmt::print("\"");
				}
				print_special_chars(candidates[i].parser_description);
				if(candidates[i].parser_description.size() == 1){
					fmt::print("\'");
				}else{
					fmt::print("\"");
				}
			}
			int start_pos = getPosition();
			int end_pos = getPosition();
			if(!candidates.empty()){
				start_pos = candidates[0].start_pos;
				end_pos = candidates[0].end_pos;
			}

			fmt::print("\n{}\n", describePosition(start_pos, end_pos));
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

		bool error_mode = false;

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
		void error(auto&&) {
			error_mode = true;
		}

		
		void notify_enter(auto&) {}
		void notify_leave(auto&, bool) {}

		bool done() const {return cur == end;}
		void advance() {
			++cur;
		};
	};

	template<typename T>
	struct basic_debug_context {

	};

	inline void print_range(auto& ctx, auto from, auto to){
		auto prev = ctx.getPosition();
		ctx.setPosition(from);
		while(ctx.getPosition() != to){
			std::cout << ctx.token();
		}
		ctx.setPosition(prev);
	}
}
