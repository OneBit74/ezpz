#include "parse_object.hpp"
#include "matcher.hpp"
#include "consumer.hpp"

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

TEST(ezpz,match){
	auto parser = match("\\a+");
	
	EXPECT_FALSE(parser("123"));
	EXPECT_TRUE(parser("abcDEF"));
	EXPECT_TRUE(parser("abcDEF"));
	EXPECT_FALSE(parser(""));

	parser = match("\\a*");
	EXPECT_TRUE(parser("123"));
	EXPECT_TRUE(parser(""));
}
TEST(ezpz,done){
	context ctx;
	EXPECT_TRUE(ctx.done());
}
TEST(ezpz,ws){
	context ctx;
	ctx.input = "   \t \t \n\t\n  ";
	EXPECT_TRUE(ws(ctx));
	EXPECT_TRUE(ctx.done());
}
TEST(ezpz,assign){
	int res;
	ASSERT_TRUE((decimal<int> >> assign(res))("123"));
	EXPECT_EQ(res,123);
}
TEST(ezpz,push){
	std::vector<int> vec;
	ASSERT_TRUE((decimal<int> >> push(vec))("123"));
	EXPECT_EQ(vec.size(),1);
	EXPECT_EQ(vec[0],123);
}
TEST(ezpz,text_parser){
	bool success;
	auto parser = text_parser("hello") + f_parser([&](auto&){
		success = true;
		return true;
	});

	success = false;
	EXPECT_TRUE(parser("hello"));
	EXPECT_TRUE(success);
	success = false;

	EXPECT_FALSE(parser("hell"));
	EXPECT_FALSE(success);
	success = false;

	EXPECT_TRUE(parser("helloo"));
	EXPECT_TRUE(success);
	success = false;

	EXPECT_FALSE(parser(""));
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
	(any >> ((decimal<int> >> push(unparsed)) + ", "))(ctx);

	RC_ASSERT(vec == unparsed);
	RC_ASSERT(ctx.done());
}
RC_GTEST_PROP(ezpz,decimal_parse_identity,(long long val)){
	context ctx;
	ctx.input = std::to_string(val);
	long long parsed = -1;
	(decimal<long long> >> assign(parsed))(ctx);
	RC_ASSERT(val == parsed);
	RC_ASSERT(ctx.done());
}
