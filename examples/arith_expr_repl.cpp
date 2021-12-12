#include "ezpz/ezpz.hpp"
#include <cmath>
#include <iostream>
#include <fstream>

void m1(){
	std::ifstream t("temp");
	std::stringstream buffer;
	buffer << t.rdbuf();

	/* context ctx("asd(asd<fd[asd,asd]>)"); */
	basic_context ctx(buffer.str());
	/* ctx.debug = true; */

	auto level = 0;
	auto indent = [&](){
		for(int i = 0; i < level; ++i){
			std::cout << ' ';
		}
	};
	auto dec_indent = [&](){
		level--;
		level = std::max(0,level);
	};
	auto inc_indent = [&](){
		level++;
	};
	auto print_one = [](auto& x){
		std::cout << x << std::flush;
	};
	auto nl = [](){
		std::cout << '\n';
	};
	auto p = 
		any
			( (capture("operator>>"_p|regex("f_wrapper\\<[^\\>]*\\>"))*print_one*inc_indent)
			| (capture("("_p|"<"|"[")*print_one*nl*inc_indent*indent)
			| (capture(")"_p|">"|"]")*dec_indent*nl*indent*print_one)
			| (capture(text_parser(", "))*print_one*nl*indent)
			| capture(single)*print_one
			);
	std::cout << sizeof(p) << std::endl;
	std::cout << match(ctx,p+eoi) << std::endl;
}
int main(){

	using num_t = float;

	auto op_maker_la = [](std::string_view op, auto upper, auto&& operation){
		return fr_parser<num_t>([op,operation,upper](basic_context& ctx, num_t& num) mutable {
			return match(ctx,upper*assign(num)+ws+(any(op+ws+ref(upper)*
					[&](num_t val){
						std::cout << num << " " << op << " " << val << " = " << operation(num,val) << std::endl;
						num = operation(num,val);
					}
			)));
		});
	};
	basic_rpo<num_t> base;
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
	auto ident = capture("%" | plus(alpha));
	std::cout << sizeof(p12) << std::endl;
	std::unordered_map<std::string,num_t> store;
	base = 
		("-"+ws+!(ref(base)*std::negate<num_t>())) |
		("("+ws+!ref(last)+ws+")") |
		!decimal<num_t> |
		!(ident*[&](auto id){
			auto ret =  store[std::string{id}];
			std::cout << "loading " << id << " with " << ret << std::endl;
			return ret;
		});
	auto expr = ws+!ref(last)+ws+eoi;

	auto assignment = (!ident + ws + ":=" + ws + !expr)*[&](auto id, auto val){
		std::cout << "storing " << id << " with " << val << std::endl;
		store[std::string{id}] = val;
	};
	while(true){
		std::string line;
		std::getline(std::cin,line);
		if(std::cin.eof())break;
		basic_context ctx(line);
		match(ctx,(assignment | (expr*[&](auto val){store["%"] = val; std::cout << val << std::endl;}) | print("error")));
	}

}
