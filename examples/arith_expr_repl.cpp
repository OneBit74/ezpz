#include "ezpz/ezpz.hpp"
#include "ezpz/macros.hpp"
#include <cmath>
#include <iostream>

int main(){

	using num_t = float;
	using ctx_t = basic_context;

	auto op_maker_la = [](std::string_view op, auto upper, auto&& operation){
		return make_rpo<num_t>([op,operation,upper](ctx_t& ctx, num_t& ret) mutable {
			auto aggregate = [&](num_t next){
				std::cout << ret << " " << op << " " << next << " = " << operation(ret,next) << std::endl;
				ret = operation(ret,next);
			};
			return parse(ctx,!upper+ws+(any(op+ws+ref(upper)*aggregate)),ret);
		});
	};
	polymorphic_rpo<ctx_t,num_t> base;
	auto p0 = op_maker_la("^",ref(base),[](auto a, auto b){
		return std::pow(a,b);
	});
	auto p1 = op_maker_la("/",p0,std::divides<num_t>());
	auto p2 = op_maker_la("*",p1,std::multiplies<num_t>());
	auto p3 = op_maker_la("-",p2,std::minus<num_t>());
	auto p4 = op_maker_la("+",p3,std::plus<num_t>());
	auto p5 = op_maker_la("&&",p4,std::logical_and());
	auto p6 = op_maker_la("||",p5,std::logical_or());
	auto p7 = op_maker_la("<",p6,std::less<num_t>());
	auto p8 = op_maker_la(">",p7,std::greater<num_t>());
	auto p9 = op_maker_la("<=",p8,std::less_equal<num_t>());
	auto p10 = op_maker_la(">=",p9,std::greater_equal<num_t>());
	auto p11 = op_maker_la("==",p10,std::equal_to<num_t>());
	auto p12 = op_maker_la("!=",p11,std::not_equal_to<num_t>());

	auto& last = p12;
	auto expr = ws+!ref(last)+ws;

	auto ident = capture("%" | plus(alpha));
	std::unordered_map<std::string,num_t> store;
	auto function = 
		  ( EZPZ_STRING("sin(")+!(ref(expr)*[](auto& val){return std::sin(val);})+EZPZ_STRING(")") )
		| ( EZPZ_STRING("cos(")+!(ref(expr)*[](auto& val){return std::cos(val);})+EZPZ_STRING(")") )
		| ( EZPZ_STRING("abs(")+!(ref(expr)*[](auto& val){return std::abs(val);})+EZPZ_STRING(")") )
		| ( EZPZ_STRING("sqrt(")+!(ref(expr)*[](auto& val){return (num_t)(std::sqrt(val));})+EZPZ_STRING(")") )
		;
	auto base_impl = make_poly<ctx_t,num_t>(
		  ("-"+ws+!(ref(base)*std::negate<num_t>()))
		| ("("+ws+!ref(expr)+ws+")")
		| !decimal<num_t>
		| !("pi"_p*ret(std::numbers::pi_v<num_t>))
		| !("e"_p*ret(std::numbers::e_v<num_t>))
		| function
		| !("true"_p * ret(num_t(1)))
		| !("false"_p * ret(num_t(0)))
		| !(ident*[&](auto id){
			auto ret =  store[std::string{id}];
			std::cout << "loading " << id << " with " << ret << std::endl;
			return ret;
		}))
		;
	base = base_impl;

	auto assignment = (!ident + ws + ":=" + ws + !expr + eoi)*[&](auto id, auto val){
		std::cout << "storing " << id << " with " << val << std::endl;
		store[std::string{id}] = val;
	};

	while(true){
		std::string line;
		std::getline(std::cin,line);
		if(std::cin.eof())break;
		ctx_t ctx(line);

		parse(ctx,(assignment | ((!expr+eoi)*[&](auto val){store["%"] = val; std::cout << val << std::endl;}) | print("error")));
	}

}
