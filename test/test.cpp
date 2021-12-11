#include "ezpz/ezpz.hpp"

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

TEST(ezpz,done){
	context ctx;
	EXPECT_TRUE(ctx.done());
}
TEST(ezpz,ws){
	context ctx;
	ctx.input = "   \t \t \n\t\n  ";
	EXPECT_TRUE(match(ctx,ws));
	EXPECT_TRUE(ctx.done());
}
TEST(ezpz,assign){
	int res;
	ASSERT_TRUE(match("123",decimal<int> * assign(res)));
	EXPECT_EQ(res,123);
}
TEST(ezpz,push){
	std::vector<int> vec;
	ASSERT_TRUE(match("123",decimal<int> * insert(vec)));
	EXPECT_EQ(vec.size(),1);
	EXPECT_EQ(vec[0],123);
}
TEST(ezpz,text_parser){
	bool success;
	auto parser = "hello" + f_parser([&](auto&){
		success = true;
		return true;
	});

	success = false;
	EXPECT_TRUE(match("hello",parser));
	EXPECT_TRUE(success);
	success = false;

	EXPECT_FALSE(match("hell",parser));
	EXPECT_FALSE(success);
	success = false;

	EXPECT_TRUE(match("helloo",parser));
	EXPECT_TRUE(success);
	success = false;

	EXPECT_FALSE(match("",parser));
	EXPECT_FALSE(success);
	success = false;
}
RC_GTEST_PROP(ezpz,simple_list_parser,(std::vector<int> vec)){
	std::ostringstream oss;
	for(auto x : vec){
		oss << x << ", ";
	}
	std::vector<int> unparsed;
	context ctx(oss.str());
	match(ctx,any(decimal<int> * insert(unparsed) + ", "));

	RC_ASSERT(vec == unparsed);
	RC_ASSERT(ctx.done());
}
RC_GTEST_PROP(ezpz,decimal_parse_identity,(long long val)){
	context ctx;
	ctx.input = std::to_string(val);
	long long parsed = -1;
	match(ctx,decimal<long long> * assign(parsed));
	RC_ASSERT(val == parsed);
	RC_ASSERT(ctx.done());
}
RC_GTEST_PROP(ezpz,text_parse_identity,(std::string s)){
	context ctx;
	ctx.input = s;
	RC_ASSERT(match(ctx,text(s)));
	RC_ASSERT(ctx.done());
}
