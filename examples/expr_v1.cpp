#include "ezpz/ezpz.hpp"

int main(){
	using num_t = int;
	basic_rpo<num_t> base;

	auto expr = (!ref(base) + any("+"+!ref(base)).reduce(0,std::plus{}))*std::plus{};
	base = (!decimal<num_t>) | ("("+!expr+")");

	basic_context ctx("1+2+3");
	ctx.debug = true;
	parse(ctx,(!expr+ eoi)*print_all | print("error"));
}
