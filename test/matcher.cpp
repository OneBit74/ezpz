#include "ezpz/ezpz.hpp"
#include <gtest/gtest.h>
using namespace ezpz;

TEST(matcher,accept_if){
	std::vector<int> range = {1,2,3,4,5,6};
	forward_range_context<std::vector<int>> ctx(std::move(range));

	auto even = accept_if([](int val){return (val+1)%2;});
	auto odd = accept_if([](int val){return val%2;});

	EXPECT_TRUE(parse(ctx,odd+even+odd+even+odd+even+eoi));
}
TEST(matcher,string){
	EXPECT_TRUE(parse("\"this is a string\"",string+eoi));
	EXPECT_FALSE(parse("this is not a string",string+eoi));
}
TEST(matcher,ws){
	basic_context ctx;
	ctx.input = "   \t \t \n\t\n  ";
	EXPECT_TRUE(parse(ctx,ws));
	EXPECT_TRUE(ctx.done());
}
TEST(matcher,token){
	EXPECT_TRUE(parse("aaa",token('a')));
	EXPECT_FALSE(parse("bbb",token('a')));
	EXPECT_TRUE(parse("aaa",token('a')+token('a')+token('a')+eoi));
}
TEST(matcher,text_parser){
	bool success;
	auto parser = "hello" + make_rpo([&](auto&){
		success = true;
		return true;
	});

	success = false;
	EXPECT_TRUE(parse("hello",parser));
	EXPECT_TRUE(success);
	success = false;

	EXPECT_FALSE(parse("hell",parser));
	EXPECT_FALSE(success);
	success = false;

	EXPECT_TRUE(parse("helloo",parser));
	EXPECT_TRUE(success);
	success = false;

	EXPECT_FALSE(parse("",parser));
	EXPECT_FALSE(success);
	success = false;
}
