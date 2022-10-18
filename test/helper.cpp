#include "ezpz/ezpz.hpp"
#include <gtest/gtest.h>
#include "gmock/gmock.h"  // Brings in gMock.
using namespace ezpz;

TEST(helper,must){
	EXPECT_TRUE(parse("abc", must("abc"_p)));
	EXPECT_THROW(parse("def", must("abc"_p)), parse_error);
}
class mock_basic_context : public basic_context {
public:
	using basic_context::basic_context;
	virtual void error_mock(){}
	void error(auto& p){
		basic_context::error(p);
		error_mock();
	}
};
class mockmock_basic_context : public mock_basic_context {
public:
	using mock_basic_context::mock_basic_context;
	MOCK_METHOD(void, error_mock, ());
};
TEST(helper,recover){
	mockmock_basic_context ctx("a");
	EXPECT_CALL(ctx,error_mock()).Times(1);
	EXPECT_TRUE(parse(ctx,recover("b"_p)));
	parse("aa",recover("ab"_p | "aab"_p));

	auto p = "hallo "_p + recover("du "_p + "da"_p);
	EXPECT_TRUE(parse("hallo da da",p));
	EXPECT_TRUE(parse("hallo du du",p));
	EXPECT_TRUE(parse("hallo du da",p));
	EXPECT_TRUE(parse("hallo du da\nhallo du der",any(p+optional("\n"_p))));
	EXPECT_TRUE(parse("hallo ",p));
}
/* TEST(helper,recover){ */
/* 	auto p = recover("bbb"_p,"aaa"_p); */
/* 	EXPECT_TRUE(parse("aaa", p)); */
/* 	EXPECT_TRUE(parse("bbb", p)); */
/* 	EXPECT_FALSE(parse("ccc", p)); */

/* 	int val = 0; */
/* 	auto p2 = recover(decimal<int>,"a"+!decimal<int>)*assign(val); */
/* 	EXPECT_TRUE(parse("a256",p2)); */
/* 	EXPECT_EQ(val,256); */
/* } */
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
	EXPECT_TRUE(parse(" "," "_p+retd(2),val));
	EXPECT_EQ(val,2);
}
/* TEST(helper,eoi_fail_msg2){ */
/* 	parse("word1 wort2",must("word1"_p+ws+"word2"_p)); */
/* } */
/* TEST(helper,eoi_fail_msg){ */
/* 	EXPECT_TRUE(parse("as","as"_p+must(eoi))); */
/* 	EXPECT_THROW({ */
/* 		try{ */
/* 			parse("asd","as"_p+must(eoi)); */
/* 		}catch(parse_error& pe){ */
/* 			EXPECT_EQ(std::string_view{pe.what()}, std::string_view{"error: 1:3 expected end of input"}); */
/* 			throw; */
/* 		} */
/* 	},parse_error); */
/* 	EXPECT_THROW({ */
/* 		try{ */
/* 			parse("as\nasd","as\nas"_p+must(eoi)); */
/* 		}catch(parse_error& pe){ */
/* 			EXPECT_EQ(std::string_view{pe.what()}, std::string_view{"error: 2:3 expected end of input"}); */
/* 			throw; */
/* 		} */
/* 	},parse_error); */
/* } */
