#include "parse_object.hpp"
#include "matcher.hpp"
#include <iostream>


void dbg_log_enter(context& ctx, std::string_view comment){
	ctx.indent();
	std::cout << "starting ";
	constexpr int lo = 7;
	constexpr size_t ro = 7;
	auto start_pos = std::max(0,int(ctx.pos)-lo);
	auto end_pos = std::min(ctx.input.size(),ctx.pos+ro);
	std::cout << '\"';
	std::cout << std::string_view{ctx.input.begin()+start_pos, ctx.input.begin()+end_pos};
	std::cout << '\"';
	std::cout << " " << comment;
	std::cout << std::endl;
	ctx.indent();
	std::cout << "          ";
	for(size_t i = start_pos; i <= end_pos; ++i){
		std::cout << (i == ctx.pos ? '^' : ' ');
	}
	std::cout << std::endl;
	ctx.depth++;
}
void dbg_log_leave(context& ctx,bool success, size_t prev_pos){
	ctx.depth--;
	ctx.indent();
	if(!success){
		std::cout << "failed ";
	}else{
		std::cout << "accepted ";
	}
	std::cout 
		<< '\"' 
		<< std::string_view{ctx.input.begin()+prev_pos,ctx.input.begin()+ctx.pos} 
		<< '\"'
		<< std::endl;
}
