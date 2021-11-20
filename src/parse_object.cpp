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
f_parser_t::f_parser_t(decltype(f) f) : f(f) {};
bool f_parser_t::_match(context& ctx) {
	return f(ctx);
}
parse_object_ref f_parser(std::function<bool(context& ctx)> f, bool dbg_inline, std::string comment){
	auto ret = parse_object_ref{std::make_shared<f_parser_t>(f)};
	ret.impl->dbg_inline = dbg_inline;
	ret.impl->comment = comment;
	return ret;
}
#include <cassert>
parse_object_ref::parse_object_ref(std::shared_ptr<parse_object> ptr) : 
	impl(ptr)
{
	assert(impl.get());
}
bool parse_object_ref::match_or_undo(context& ctx){
	return impl->match_or_undo(ctx);
}
bool parse_object_ref::operator()(std::string_view sv){
	return (*impl)(sv);
}
bool parse_object_ref::operator()(context& ctx){
	return (*impl)(ctx);
}
bool parse_object_ref::match(context& ctx){
	return impl->match(ctx);
}
bool parse_object_ref::_match(context& ctx){
	return impl->_match(ctx);
}
void parse_object_ref::_undo(context& ctx){
	return impl->_undo(ctx);
}
parse_object_ref operator+(parse_object_ref lhs, parse_object_ref rhs){
	return f_parser([=] 
				(context& ctx) mutable {
					return lhs.match(ctx) && rhs.match(ctx);
				},true);
}
parse_object_ref operator+(std::string_view sv, parse_object_ref rhs){
	return text_parser(sv) + rhs;
		
}
parse_object_ref operator+(parse_object_ref lhs, std::string_view sv){
	return lhs + text_parser(sv);
}
parse_object_ref operator|(parse_object_ref lhs, parse_object_ref rhs){
	return f_parser([=] 
				(context& ctx) mutable {
					return lhs.match_or_undo(ctx) || rhs.match(ctx);
				},true);
}
parse_object_ref operator|(parse_object_ref lhs, std::string_view rhs){
	return lhs | text_parser(rhs);
}
parse_object_ref operator|(std::string_view lhs, parse_object_ref rhs){
	return text_parser(lhs) | rhs;
}
