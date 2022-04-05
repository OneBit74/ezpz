#include "ezpz/ezpz.hpp"
#include <variant>
#include <string>
#include <type_traits>

enum TOKEN_TYPE {
	LBRACE,RBRACE,EQ,
	INTEGER,FLOAT,STRING,IDENT
};

struct TOKEN {
	TOKEN_TYPE type;
	std::variant<int,float,std::string> data;

	explicit TOKEN(TOKEN_TYPE type) : type(type) {}
	explicit TOKEN(int data) : type(INTEGER), data(data) {}
	explicit TOKEN(float data) : type(FLOAT), data(data) {}
	explicit TOKEN(std::string data) : type(STRING), data(data) {}

	TOKEN(TOKEN_TYPE type, auto data) : type(type), data(data) {}

	bool operator==(const TOKEN_TYPE otype) const {
		return otype == type;
	}
};

int main(){
	std::vector<TOKEN> tokens;
	auto token_parser 
		= plus(" "_p)
		| "="_p * [&](){
			tokens.push_back(TOKEN{EQ});
		}
		| "{"_p * [&](){
			tokens.push_back(TOKEN{LBRACE});
		}
		| "}"_p * [&](){
			tokens.push_back(TOKEN{RBRACE});
		}
		| decimal<float> * [&](float f){
			tokens.push_back(TOKEN{f});
		}
		| decimal<int>  * [&](int i){
			tokens.push_back(TOKEN{i});
		}
		| capture("\""+must(any(notf("\"")+single) + "\"")) * [&](std::string_view sv){
			tokens.push_back(TOKEN{std::string{sv}});
		}
		| capture(plus(notf(" ")+single)) * [&](std::string_view sv){
			tokens.push_back(TOKEN{IDENT,std::string{sv}});
		};

	
	parse(" abc = 420 ",any(token_parser)+eoi | print("error"));
	std::cout << tokens.size() << std::endl;

	using ctx_t = forward_range_context<std::vector<TOKEN>>;

	auto int_p = accept_if_equal(INTEGER);
	auto float_p = accept_if_equal(FLOAT);
	auto num = int_p | float_p;
	auto string = accept_if_equal(STRING);
	auto ident = accept_if_equal(IDENT);
	auto eq = accept_if_equal(EQ);

	auto assignment = ident + eq + num;
	auto programm = assignment;

	ctx_t ctx(tokens);
	parse(ctx,programm+eoi | print("error"));

}
