#include "parse_object.hpp"
#include "matcher.hpp"
#include <iostream>

bool parse_object::_match(context&) {return false;}
void parse_object::_undo(context&) {};
bool parse_object::operator()(std::string_view sv){
	context ctx;
	ctx.input = sv;
	return (*this)(ctx);
}
bool parse_object::operator()(context& ctx){
	return match_or_undo(ctx);
}
bool parse_object::match_or_undo(context& ctx){
	size_t start_match = ctx.pos;
	if(match(ctx)){
		return true;
	}else{
		ctx.pos = start_match;
		_undo(ctx);
		return false;
	}
}
bool parse_object::match(context& ctx){
	if(ctx.debug && !dbg_inline){
		auto prev_pos = dbg_log_enter(ctx);
		auto ret = _match(ctx);
		dbg_log_leave(ctx);
		if(!ret){
			std::cout << "failed ";
		}else{
			std::cout << "accepted ";
		}
		dbg_log_comment(ctx,prev_pos);
		return ret;
	}else{
		return _match(ctx);
	}
}
void parse_object::dbg_log_comment(context& ctx, size_t prev_pos){
	std::cout << '\"' << std::string_view{ctx.input.begin()+prev_pos,ctx.input.begin()+ctx.pos} <<'\"' << std::endl;
}
size_t parse_object::dbg_log_enter(context& ctx){
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
	return ctx.pos;
}
void parse_object::dbg_log_leave(context& ctx){
	ctx.depth--;
	ctx.indent();
}
void parse_object::undo(context& ctx){
	_undo(ctx);
}
