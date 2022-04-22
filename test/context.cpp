#include "ezpz/ezpz.hpp"
#include "ezpz/macros.hpp"
#include <gtest/gtest.h>

using namespace ezpz;


TEST(context,range_context){
	std::vector<int> range = {1,2,3,3,2,1};
	forward_range_context ctx(range);
	auto v1 = EZPZ_SINGLE_TOKEN((token == 1));
	auto v2 = EZPZ_SINGLE_TOKEN((token == 2));
	auto v3 = EZPZ_SINGLE_TOKEN((token == 3));
	EXPECT_TRUE(parse(ctx,v1+v2+v3+v3+v2+v1));
}
TEST(context,done){
	basic_context ctx;
	EXPECT_TRUE(ctx.done());
}

