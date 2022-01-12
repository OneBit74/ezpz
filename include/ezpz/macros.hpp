#include "matcher.hpp"
#define EZPZ_STRING(lit) fast_text([](){return lit;})
#define EZPZ_SINGLE_TOKEN(b_expr)\
	make_rpo([](auto& ctx){\
		if(ctx.done())return false;\
		auto token = ctx.token();\
		if( b_expr ){\
			ctx.advance();\
			return true;\
		}else{return false;}\
	})
