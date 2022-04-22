#include "ezpz/ezpz.hpp"
#include <gtest/gtest.h>
using namespace ezpz;

TEST(helper,must){
	EXPECT_TRUE(parse("abc", must("abc"_p)));
	EXPECT_THROW(parse("def", must("abc"_p)), parse_error);
}
TEST(helper,recover){
	auto p = recover("bbb"_p,"aaa"_p);
	EXPECT_TRUE(parse("aaa", p));
	EXPECT_TRUE(parse("bbb", p));
	EXPECT_FALSE(parse("ccc", p));

	int val = 0;
	auto p2 = recover(decimal<int>,"a"+!decimal<int>)*assign(val);
	EXPECT_TRUE(parse("a256",p2));
	EXPECT_EQ(val,256);
}
TEST(helper,fail){
	EXPECT_FALSE(parse("abdsd",fail));
	EXPECT_FALSE(parse("8392842",fail));
	EXPECT_FALSE(parse(":)",fail));
	EXPECT_FALSE(parse("",fail));
}
TEST(helper,poly_recursion){
	polymorphic_rpo<basic_context> parser;
	auto p1 = "a"+optional(ref(parser));
	auto p2 = make_poly<basic_context>(p1);
	parser = p2;
	EXPECT_TRUE(parse("aaaaaaaaaaaaaaaa",ref(parser)+eoi));
	EXPECT_TRUE(parse("aaa",ref(parser)+eoi));
	EXPECT_FALSE(parse("",ref(parser)+eoi));
}
TEST(helper,rpo_recursion){
	basic_rpo<> parser;
	parser = "a"+optional(ref(parser));
	EXPECT_TRUE(parse("aaaaaaaaaaaaaaaa",ref(parser)+eoi));
	EXPECT_TRUE(parse("aaa",ref(parser)+eoi));
	EXPECT_FALSE(parse("",ref(parser)+eoi));
}
TEST(helper,capture){
	std::string_view subtext;
	basic_context ctx("abcdefghi");
	EXPECT_TRUE(parse(ctx,"abc"+capture("def"_p)*assign(subtext)+"ghi"));
	EXPECT_EQ(subtext,"def");
}
TEST(helper,ref){
	auto my_parser = decimal<int>+ws+decimal<int>;
	auto r_p = ref(my_parser);
	auto rr_p = ref(r_p);
	auto rrr_p = ref(rr_p);
	basic_context ctx("123 456");
	EXPECT_TRUE(parse(ctx,r_p));
	ctx.pos = 0;
	EXPECT_TRUE(parse(ctx,rrr_p));
}
TEST(helper,dynamic_ret){
	int val = 0;
	EXPECT_TRUE(parse(" "," "_p+!retd(2),val));
	EXPECT_EQ(val,2);
}
