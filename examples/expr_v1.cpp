#include "ezpz/ezpz.hpp"
#include "ezpz/macros.hpp"

using namespace ezpz;

int main(){
	using num_t = int;
	auto expr = make_rpo<num_t>([](auto& ctx, auto& self, num_t& ret){
		auto base = (decimal<num_t>) | (EZPZ_STRING("(")+self+EZPZ_STRING(")"));
		auto add = [](auto& agg, auto val){
			agg += val;
		};
		auto expr = merge(base + any(EZPZ_STRING("+")+agg(base,add)));
		return parse(ctx,expr,ret);
	});
	parse("1+2+3",(expr+ eoi)*print_all | print("error"));
}
