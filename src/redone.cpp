#include "matcher.hpp"
#include "ezpz.hpp"
#include "unparser.hpp"
#include <cmath>
#include <iostream>
#include <fstream>

int main(){
	std::ifstream t("temp");
	std::stringstream buffer;
	buffer << t.rdbuf();

	/* context ctx("asd(asd<fd[asd,asd]>)"); */
	context ctx(buffer.str());
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
		any >> 
			( (capture(text_parser("operator>>")|regex("f_wrapper\\<[^\\>]*\\>"))*print_one*inc_indent)
			| (capture(text_parser("(")|"<"|"[")*print_one*nl*inc_indent*indent)
			| (capture(text_parser(")")|">"|"]")*dec_indent*nl*indent*print_one)
			| (capture(text_parser(", "))*print_one*nl*indent)
			| capture(single)*print_one
			);
	std::cout << sizeof(p) << std::endl;
	std::cout << match(ctx,p+eoi) << std::endl;

	return 0;

	using num_t = float;

	auto op_maker_la = [](std::string_view op, auto& upper, auto&& operation){
		return fr_parser<num_t>([op,operation,&upper](context& ctx, num_t& num){
			return match(ctx,ref(upper)*assign(num)+ws+(any >> (op+ws+ref(upper)*
					[&](num_t val){
						std::cout << num << " " << op << " " << val << " = " << operation(num,val) << std::endl;
						num = operation(num,val);
					}
			)));
		});
	};
	rpo<num_t> base;
	auto p0 = op_maker_la("^",base,[](auto a, auto b){
		return std::pow(a,b);
	});
	auto p1 = op_maker_la("/",p0,[](auto a, auto b){
		return a/b;
	});
	auto p2 = op_maker_la("*",p1,[](auto a, auto b){
		return a*b;
	});
	auto p3 = op_maker_la("-",p2,[](auto a, auto b){
		return a-b;
	});
	auto p4 = op_maker_la("+",p3,[](auto a, auto b){
		return a+b;
	});
	base = "("+ws+!ref(p4)+ws+")" | !decimal<num_t>;
	auto expr = ws+!ref(p4)+ws+eoi;

	while(true){
		std::string line;
		std::getline(std::cin,line);
		context ctx(line);
		match(ctx,((expr*print_all) | print("error")));
	}

}
