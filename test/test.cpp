#include "ezpz/ezpz.hpp"

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

TEST(context,done){
	basic_context ctx;
	EXPECT_TRUE(ctx.done());
}
TEST(matcher,ws){
	basic_context ctx;
	ctx.input = "   \t \t \n\t\n  ";
	EXPECT_TRUE(match(ctx,ws));
	EXPECT_TRUE(ctx.done());
}
TEST(consumer,assign){
	int res;
	ASSERT_TRUE(match("123",decimal<int> * assign(res)));
	EXPECT_EQ(res,123);
}
TEST(consumer,insert){
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

TEST(consumer,ret){
	int val = 0;
	match("   ",ws*ret(2)*assign(val));
	EXPECT_EQ(val,2);

	std::string_view text;
	match("   ",ws*ret<std::string_view>("hey")*assign(text));
	EXPECT_EQ(text,"hey");
}
TEST(quantifiers,any){
	{
		basic_context ctx("aaaa");
		EXPECT_TRUE(match(ctx,any("a"_p)));
		EXPECT_TRUE(ctx.done());
	}
	{
		basic_context ctx("");
		EXPECT_TRUE(match(ctx,any("a"_p)));
		EXPECT_TRUE(ctx.done());
	}
	{
		basic_context ctx("bbbb");
		EXPECT_TRUE(match(ctx,any("a"_p)));
		EXPECT_FALSE(ctx.done());
	}
	{
		basic_context ctx("aabb");
		EXPECT_TRUE(match(ctx,any("a"_p)));
		EXPECT_FALSE(ctx.done());
	}
	{
		basic_context ctx("bbaa");
		EXPECT_TRUE(match(ctx,any("a"_p)));
		EXPECT_FALSE(ctx.done());
	}
}
TEST(quantifiers,plus){
	{
		basic_context ctx("");
		EXPECT_FALSE(match(ctx,plus("a"_p)));
	}
	{
		basic_context ctx("a");
		EXPECT_TRUE(match(ctx,plus("a"_p)));
		EXPECT_TRUE(ctx.done());
	}
	{
		basic_context ctx("aaa");
		EXPECT_TRUE(match(ctx,plus("a"_p)));
		EXPECT_TRUE(ctx.done());
	}
}
TEST(quantifiers,optional){
	{
		basic_context ctx("abcdefghc");
		EXPECT_TRUE(match(ctx,"abc"+optional("def"_p)+"ghc"));
		EXPECT_TRUE(ctx.done());
	}
	{
		basic_context ctx("abcghc");
		EXPECT_TRUE(match(ctx,"abc"+optional("def"_p)+"ghc"));
		EXPECT_TRUE(ctx.done());
	}
}
TEST(quantifiers,not){
	EXPECT_TRUE(match("a",notf("b"_p)));
	EXPECT_FALSE(match("a",notf("a"_p)));
}
TEST(core,match_or_undo){
	basic_context ctx("123 wasd");
	EXPECT_FALSE(match_or_undo(ctx,"123 wast"_p));
	EXPECT_EQ(ctx.pos,0);
	EXPECT_FALSE(ctx.done());

	EXPECT_TRUE(match_or_undo(ctx,"123 wasd"_p));
	EXPECT_TRUE(ctx.done());
	ctx.pos = 0;

	EXPECT_FALSE(match_or_undo(ctx,decimal<int>+" was"+eoi));
	EXPECT_EQ(ctx.pos,0);
}
TEST(matcher,token){
	EXPECT_TRUE(match("aaa",token('a')));
	EXPECT_FALSE(match("bbb",token('a')));
	EXPECT_TRUE(match("aaa",token('a')+token('a')+token('a')+eoi));
}
TEST(quantifiers,times){
	EXPECT_TRUE(match("aaa",times(3,"a"_p)+eoi));
	EXPECT_FALSE(match("aaaa",times(3,"a"_p)+eoi));
	EXPECT_FALSE(match("aa",times(3,"a"_p)+eoi));
}
TEST(quantifiers,max){
	EXPECT_FALSE(match("aaa",max(2,"a"_p)+eoi));
	EXPECT_TRUE(match("aaa",max(3,"a"_p)+eoi));
	EXPECT_TRUE(match("aa",max(3,"a"_p)+eoi));
}
TEST(quantifiers,min){
	EXPECT_FALSE(match("aa",min(3,"a"_p)+eoi));
	EXPECT_TRUE(match("aaa",min(3,"a"_p)+eoi));
	EXPECT_FALSE(match("aaa",min(4,"a"_p)+eoi));
}
TEST(helper,fail){
	EXPECT_FALSE(match("abdsd",fail));
	EXPECT_FALSE(match("8392842",fail));
	EXPECT_FALSE(match(":)",fail));
	EXPECT_FALSE(match("",fail));
}
TEST(helper,rpo_recursion){
	basic_rpo<> parser;
	parser = "a"+optional(ref(parser));
	EXPECT_TRUE(match("aaaaaaaaaaaaaaaa",ref(parser)+eoi));
	EXPECT_TRUE(match("aaa",ref(parser)+eoi));
	EXPECT_FALSE(match("",ref(parser)+eoi));
}
TEST(helper,capture){
	std::string_view subtext;
	basic_context ctx("abcdefghi");
	EXPECT_TRUE(match(ctx,"abc"+capture("def"_p)*assign(subtext)+"ghi"));
	EXPECT_EQ(subtext,"def");
}
TEST(helper,ref){
	auto my_parser = decimal<int>+ws+decimal<int>;
	auto r_p = ref(my_parser);
	auto rr_p = ref(r_p);
	auto rrr_p = ref(rr_p);
	basic_context ctx("123 456");
	EXPECT_TRUE(match(ctx,r_p));
	ctx.pos = 0;
	EXPECT_TRUE(match(ctx,rrr_p));
}
TEST(matcher,text_parser){
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
	basic_context ctx(oss.str());
	match(ctx,any(decimal<int> * insert(unparsed) + ", "));

	RC_ASSERT(vec == unparsed);
	RC_ASSERT(ctx.done());
}
RC_GTEST_PROP(ezpz,integer_decimal_parse_identity,(long long val)){
	basic_context ctx;
	ctx.input = std::to_string(val);
	long long parsed = -1;
	RC_ASSERT(match(ctx,decimal<long long> * assign(parsed)));
	RC_ASSERT(val == parsed);
	RC_ASSERT(ctx.done());
}
RC_GTEST_PROP(ezpz,floating_decimal_parse_identity,(double val)){
	std::ostringstream out;
	out.precision(32);
    out << std::fixed << val;
	basic_context ctx;
    ctx.input = out.str();
	double parsed = -1;
	RC_ASSERT(match(ctx,decimal<double> * assign(parsed)));
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
	RC_ASSERT(match(ctx,text(s)));
	RC_ASSERT(ctx.done());
}
