#include "ezpz/ezpz.hpp"
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>
using namespace ezpz;

RC_GTEST_PROP(ezpz,simple_list_parser,(std::vector<int> vec)){
	std::ostringstream oss;
	for(auto x : vec){
		oss << x << ", ";
	}
	std::vector<int> unparsed;
	std::string data = oss.str();
	basic_context ctx(data);
	parse(ctx,any(decimal<int> * insert(unparsed) + ", "));

	RC_ASSERT(vec == unparsed);
	RC_ASSERT(ctx.done());
}
RC_GTEST_PROP(ezpz,integer_decimal_parse_identity,(long long val)){
	basic_context ctx;
	auto data = std::to_string(val); 
	ctx.input = data;
	long long parsed = -1;
	RC_ASSERT(parse(ctx,decimal<long long> * assign(parsed)));
	RC_ASSERT(val == parsed);
	RC_ASSERT(ctx.done());
}
RC_GTEST_PROP(ezpz,floating_decimal_parse_identity,(double val)){
	std::ostringstream out;
	out.precision(32);
    out << std::fixed << val;
	basic_context ctx;
	auto data = out.str();
    ctx.input = data;
	double parsed = -1;
	RC_ASSERT(parse(ctx,decimal<double> * assign(parsed)));
	if(val != 0){
		RC_ASSERT(std::abs((val-parsed)/val) <= 0.000001);
	}else{
		RC_ASSERT(val == parsed);
	}
	RC_ASSERT(ctx.done());
}
RC_GTEST_PROP(ezpz,text_parse_identity,(std::string s)){
	basic_context ctx;
	ctx.input = s;
	RC_ASSERT(parse(ctx,text(s)));
	RC_ASSERT(ctx.done());
}
