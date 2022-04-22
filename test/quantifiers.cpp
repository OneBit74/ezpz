#include "ezpz/ezpz.hpp"
#include <gtest/gtest.h>
using namespace ezpz;

TEST(quantifiers,reduce){
	std::vector<char> result;
	EXPECT_TRUE(parse("1234",any(capture(single)).reduce(
			[](){return std::vector<char>{};},
			[](auto& vec, auto sv){vec.push_back(sv[0]);})
			, result
		));
	ASSERT_EQ(result.size(), 4);
	EXPECT_EQ(result[0], '1');
	EXPECT_EQ(result[1], '2');
	EXPECT_EQ(result[2], '3');
	EXPECT_EQ(result[3], '4');
}
TEST(quantifiers,reduce2){
	std::vector<char> result;
	EXPECT_TRUE(parse("1234",reduce(min<2>(capture(single)),
			[](){return std::vector<char>{};},
			[](auto& vec, auto sv){vec.push_back(sv[0]);})
			, result
		));
	ASSERT_EQ(result.size(), 4);
	EXPECT_EQ(result[0], '1');
	EXPECT_EQ(result[1], '2');
	EXPECT_EQ(result[2], '3');
	EXPECT_EQ(result[3], '4');
}
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
	EXPECT_FALSE(parse("aaaa",times(3,"a"_p)+eoi));
	EXPECT_FALSE(parse("aa",times(3,"a"_p)+eoi));
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
