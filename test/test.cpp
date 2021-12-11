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
TEST(ezpz,insert){
	std::vector<int> vec;
	ASSERT_TRUE(match("123",decimal<int> * insert(vec)));

	EXPECT_EQ(vec.size(),1);
	EXPECT_EQ(vec[0],123);

	std::map<int,int> mop;
	ASSERT_TRUE(match("123 456",(!decimal<int>+ws+!decimal<int>) * insert(mop)));

	EXPECT_EQ(mop.size(),1);
	EXPECT_EQ(mop[123],456);

	std::set<int> s;;
	ASSERT_TRUE(match("123",decimal<int> * insert(s)));

	EXPECT_EQ(s.size(),1);
	EXPECT_EQ(*begin(s),123);

}

TEST(ezpz,ret){
	int val = 0;
	match("   ",ws*ret(2)*assign(val));
	EXPECT_EQ(val,2);

	std::string_view text;
	match("   ",ws*ret<std::string_view>("hey")*assign(text));
	EXPECT_EQ(text,"hey");
}
TEST(quantifiers,any){
	{
		context ctx("aaaa");
		EXPECT_TRUE(match(ctx,any("a"_p)));
		EXPECT_TRUE(ctx.done());
	}
	{
		context ctx("");
		EXPECT_TRUE(match(ctx,any("a"_p)));
		EXPECT_TRUE(ctx.done());
	}
	{
		context ctx("bbbb");
		EXPECT_TRUE(match(ctx,any("a"_p)));
		EXPECT_FALSE(ctx.done());
	}
	{
		context ctx("aabb");
		EXPECT_TRUE(match(ctx,any("a"_p)));
		EXPECT_FALSE(ctx.done());
	}
	{
		context ctx("bbaa");
		EXPECT_TRUE(match(ctx,any("a"_p)));
		EXPECT_FALSE(ctx.done());
	}
}
TEST(quantifiers,plus){
	{
		context ctx("");
		EXPECT_FALSE(match(ctx,plus("a"_p)));
	}
	{
		context ctx("a");
		EXPECT_TRUE(match(ctx,plus("a"_p)));
		EXPECT_TRUE(ctx.done());
	}
	{
		context ctx("aaa");
		EXPECT_TRUE(match(ctx,plus("a"_p)));
		EXPECT_TRUE(ctx.done());
	}
}
TEST(quantifiers,optional){
	{
		context ctx("abcdefghc");
		EXPECT_TRUE(match(ctx,"abc"+optional("def"_p)+"ghc"));
		EXPECT_TRUE(ctx.done());
	}
	{
		context ctx("abcghc");
		EXPECT_TRUE(match(ctx,"abc"+optional("def"_p)+"ghc"));
		EXPECT_TRUE(ctx.done());
	}
}
TEST(quantifiers,not){
	EXPECT_TRUE(match("a",notf("b"_p)));
	EXPECT_FALSE(match("a",notf("a"_p)));
}
TEST(core,match_or_undo){
	context ctx("123 wasd");
	EXPECT_FALSE(match_or_undo(ctx,"123 wast"_p));
	EXPECT_EQ(ctx.pos,0);
	EXPECT_FALSE(ctx.done());

	EXPECT_TRUE(match_or_undo(ctx,"123 wasd"_p));
	EXPECT_TRUE(ctx.done());
	ctx.pos = 0;

	EXPECT_FALSE(match_or_undo(ctx,decimal<int>+" was"+eoi));
	EXPECT_EQ(ctx.pos,0);
}
TEST(helper,fail){
	EXPECT_FALSE(match("abdsd",fail));
	EXPECT_FALSE(match("8392842",fail));
	EXPECT_FALSE(match(":)",fail));
	EXPECT_FALSE(match("",fail));
}
TEST(helper,rpo_recursion){
	rpo<> parser;
	parser = "a"+optional(ref(parser));
	EXPECT_TRUE(match("aaaaaaaaaaaaaaaa",ref(parser)+eoi));
	EXPECT_TRUE(match("aaa",ref(parser)+eoi));
	EXPECT_FALSE(match("",ref(parser)+eoi));
}
TEST(helper,capture){
	std::string_view subtext;
	context ctx("abcdefghi");
	EXPECT_TRUE(match(ctx,"abc"+capture("def"_p)*assign(subtext)+"ghi"));
	EXPECT_EQ(subtext,"def");
}
TEST(helper,ref){
	auto my_parser = decimal<int>+ws+decimal<int>;
	auto r_p = ref(my_parser);
	auto rr_p = ref(r_p);
	auto rrr_p = ref(rr_p);
	context ctx("123 456");
	EXPECT_TRUE(match(ctx,r_p));
	ctx.pos = 0;
	EXPECT_TRUE(match(ctx,rrr_p));
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
