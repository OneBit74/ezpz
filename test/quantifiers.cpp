#include "ezpz/ezpz.hpp"
#include <gtest/gtest.h>
using namespace ezpz;

TEST(quantifiers,any){
	{
		basic_context ctx("aaaa");
		EXPECT_TRUE(parse(ctx,any("a"_p)));
		EXPECT_TRUE(ctx.done());
	}
	{
		basic_context ctx("");
		EXPECT_TRUE(parse(ctx,any("a"_p)));
		EXPECT_TRUE(ctx.done());
	}
	{
		basic_context ctx("bbbb");
		EXPECT_TRUE(parse(ctx,any("a"_p)));
		EXPECT_FALSE(ctx.done());
	}
	{
		basic_context ctx("aabb");
		EXPECT_TRUE(parse(ctx,any("a"_p)));
		EXPECT_FALSE(ctx.done());
	}
	{
		basic_context ctx("bbaa");
		EXPECT_TRUE(parse(ctx,any("a"_p)));
		EXPECT_FALSE(ctx.done());
	}
}
TEST(quantifiers,plus){
	{
		basic_context ctx("");
		EXPECT_FALSE(parse(ctx,plus("a"_p)));
	}
	{
		basic_context ctx("a");
		EXPECT_TRUE(parse(ctx,plus("a"_p)));
		EXPECT_TRUE(ctx.done());
	}
	{
		basic_context ctx("aaa");
		EXPECT_TRUE(parse(ctx,plus("a"_p)));
		EXPECT_TRUE(ctx.done());
	}
}
TEST(quantifiers,optional){
	{
		basic_context ctx("abcdefghc");
		EXPECT_TRUE(parse(ctx,"abc"+optional("def"_p)+"ghc"));
		EXPECT_TRUE(ctx.done());
	}
	{
		basic_context ctx("abcghc");
		EXPECT_TRUE(parse(ctx,"abc"+optional("def"_p)+"ghc"));
		EXPECT_TRUE(ctx.done());
	}
	{
		std::optional<int> res;
		parse("123",optional(decimal<int>),res);
		EXPECT_EQ(res,123);
	}
	{
		std::optional<std::tuple<int,int>> res;
		parse("123 456",optional(decimal<int>+" "+decimal<int>),res);
		ASSERT_TRUE(res);
		EXPECT_EQ(std::get<0>(*res),123);
		EXPECT_EQ(std::get<1>(*res),456);
	}
}
TEST(quantifiers,peek){
	EXPECT_TRUE(parse("a",notf("b"_p)));
	EXPECT_FALSE(parse("a",notf("a"_p)));
	EXPECT_TRUE(parse("ba",any(notf("a"_p)+single)+"a"));
}
TEST(quantifiers,not){
	EXPECT_TRUE(parse("a",peek("a"_p)));
	EXPECT_FALSE(parse("b",peek("a"_p)));
	basic_context ctx("a");
	EXPECT_TRUE(parse(ctx,peek("a"_p)));
	EXPECT_TRUE(parse(ctx,peek("a"_p)));
	EXPECT_TRUE(parse(ctx,peek("a"_p)+peek("a"_p)));
	EXPECT_FALSE(parse(ctx,"a"_p+peek("a"_p)));
}
TEST(quantifiers,times){
	EXPECT_TRUE(parse("aaa",times(3,"a"_p)+eoi));
	EXPECT_TRUE(parse("aaa",times<3>("a"_p)+eoi));
	EXPECT_FALSE(parse("aaaa",times(3,"a"_p)+eoi));
	EXPECT_FALSE(parse("aaaa",times<3>("a"_p)+eoi));
	EXPECT_FALSE(parse("aa",times(3,"a"_p)+eoi));
	EXPECT_FALSE(parse("aa",times<3>("a"_p)+eoi));
}
TEST(quantifiers,max){
	EXPECT_FALSE(parse("aaa",max(2,"a"_p)+eoi));
	EXPECT_TRUE(parse("aaa",max(3,"a"_p)+eoi));
	EXPECT_TRUE(parse("aa",max(3,"a"_p)+eoi));
}
TEST(quantifiers,min){
	EXPECT_FALSE(parse("aa",min(3,"a"_p)+eoi));
	EXPECT_TRUE(parse("aaa",min(3,"a"_p)+eoi));
	EXPECT_FALSE(parse("aaa",min(4,"a"_p)+eoi));
}
