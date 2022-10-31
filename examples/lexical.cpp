#include "ezpz/ezpz.hpp"
#include <variant>
#include <string>
#include <type_traits>

using namespace ezpz;

enum TOKEN_TYPE {
	LBRACE,RBRACE,EQ,
	INTEGER,FLOAT,STRING,IDENT
};

struct TOKEN {
	TOKEN_TYPE type;
	std::variant<int,float,std::string_view> data;

	TOKEN() = default;
	TOKEN(TOKEN_TYPE type) : type(type) {}
	TOKEN(int data) : type(INTEGER), data(data) {}
	TOKEN(float data) : type(FLOAT), data(data) {}
	TOKEN(std::string_view data) : type(STRING), data(data) {}

	TOKEN(TOKEN_TYPE type, auto data) : type(type), data(data) {}

	bool operator==(const TOKEN_TYPE otype) const {
		return otype == type;
	}
};

int main(){
	std::vector<TOKEN> tokens;
	auto add_simple_token = [&](TOKEN_TYPE type){
		return [&tokens,type=type](){
			tokens.push_back(TOKEN{type});
		};
	};
	auto push_token = [&](TOKEN&& token){
		tokens.push_back(token);
	};
	auto token_parser 
		= plus(" "_p)
		| "="_p * add_simple_token(EQ)
		| "{"_p * add_simple_token(LBRACE)
		| "}"_p * add_simple_token(RBRACE)
		| decimal<float> * into<TOKEN> * push_token
		| decimal<int>  * into<TOKEN> * push_token
		| capture("\""+recover(must(any(notf("\"")+single) + "\""))) * into<TOKEN> * push_token
		| capture(plus(notf(" ")+single)) * [&](std::string_view sv){
			tokens.push_back(TOKEN{IDENT,std::string{sv}});
		};

	parse(" abc = 420 ",(any(notf(eoi) + recover(token_parser))+eoi));
	std::cout << tokens.size() << std::endl;

	using ctx_t = forward_range_context<std::vector<TOKEN>>;

	auto int_p = accept_if_equal(INTEGER);
	auto float_p = accept_if_equal(FLOAT);
	auto num = int_p | float_p;
	/* auto string = accept_if_equal(STRING); */
	auto ident = accept_if_equal(IDENT);
	auto eq = accept_if_equal(EQ);

	auto assignment = ident + recover(eq + num);

	ctx_t ctx(tokens);
	parse(ctx,recover(assignment));
	/* auto programm = any(assignment); */
	/* parse(ctx,(programm+eoi) | print("error")); */
}
