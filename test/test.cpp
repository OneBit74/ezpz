#include "ezpz/ezpz.hpp"
#include "ezpz/macros.hpp"

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

TEST(context,range_context){
	std::vector<int> range = {1,2,3,3,2,1};
	forward_range_context ctx(range);
	auto v1 = EZPZ_SINGLE_TOKEN((token == 1));
	auto v2 = EZPZ_SINGLE_TOKEN((token == 2));
	auto v3 = EZPZ_SINGLE_TOKEN((token == 3));
	EXPECT_TRUE(parse(ctx,v1+v2+v3+v3+v2+v1));
}
TEST(quantifiers,reduce){
	std::vector<char> result;
	EXPECT_TRUE(parse("1234",any(capture(single)).reduce(
			[](){return std::vector<char>{};},
			[](auto& vec, auto sv){vec.push_back(sv[0]);})
			, result
		));
	ASSERT_TRUE(result.size() == 4);
	EXPECT_TRUE(result[0] == '1');
	EXPECT_TRUE(result[1] == '2');
	EXPECT_TRUE(result[2] == '3');
	EXPECT_TRUE(result[3] == '4');
}
TEST(matcher,accept_if){
	std::vector<int> range = {1,2,3,4,5,6};
	forward_range_context ctx(range);

	auto even = accept_if([](int val){return (val+1)%2;});
	auto odd = accept_if([](int val){return val%2;});

	EXPECT_TRUE(parse(ctx,odd+even+odd+even+odd+even+eoi));
}
TEST(ezpz,make_rpo){
	basic_context ctx;
	auto p1 = make_rpo([](auto& ctx){
		return parse(ctx,ws);
	});
	EXPECT_TRUE(parse("  ",p1+eoi));
	auto p2 = make_rpo<int>([](auto& ctx,int& ret){
		return parse(ctx,decimal<int>,ret);
	});
	EXPECT_TRUE(parse("123",p2+eoi));
}
TEST(ezpz,or_undo_lhs){
	basic_context ctx("aab");;
	EXPECT_TRUE(parse(ctx,"aaa"_p | "aab"));
}
TEST(ezpz,make_rpo_recursive){
	basic_context ctx("123 456 789");;
	auto number_list = make_rpo<std::vector<int>>([](auto& ctx, auto& self, auto& ret){
		return parse(ctx,(!decimal<int>+optional(ws+!::ref(self)))*
		[&](int num, std::optional<std::vector<int>> list){
			if(list)ret = std::move(*list);
			ret.insert(std::begin(ret),num);
		});
	});
	std::vector<int> result;
	std::vector<int> expected = {123,456,789};
	EXPECT_TRUE(parse(ctx,number_list*assign(result)+eoi));
	EXPECT_EQ(result,expected);
}
TEST(context,done){
	basic_context ctx;
	EXPECT_TRUE(ctx.done());
}
TEST(matcher,ws){
	basic_context ctx;
	ctx.input = "   \t \t \n\t\n  ";
	EXPECT_TRUE(parse(ctx,ws));
	EXPECT_TRUE(ctx.done());
}
TEST(consumer,assign){
	int res;
	ASSERT_TRUE(parse("123",decimal<int> * assign(res)));
	EXPECT_EQ(res,123);
}
TEST(consumer,insert){
	std::vector<int> vec;
	ASSERT_TRUE(parse("123",decimal<int> * insert(vec)));

	EXPECT_EQ(vec.size(),1);
	EXPECT_EQ(vec[0],123);

	std::map<int,int> mop;
	ASSERT_TRUE(parse("123 456",(!decimal<int>+ws+!decimal<int>) * insert(mop)));

	EXPECT_EQ(mop.size(),1);
	EXPECT_EQ(mop[123],456);

	std::set<int> s;;
	ASSERT_TRUE(parse("123",decimal<int> * insert(s)));

	EXPECT_EQ(s.size(),1);
	EXPECT_EQ(*begin(s),123);

}

TEST(consumer,ret){
	int val = 0;
	parse("   ",ws*ret(2)*assign(val));
	EXPECT_EQ(val,2);

	std::string_view text;
	parse("   ",ws*ret<std::string_view>("hey")*assign(text));
	EXPECT_EQ(text,"hey");
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
TEST(quantifiers,not){
	EXPECT_TRUE(parse("a",notf("b"_p)));
	EXPECT_FALSE(parse("a",notf("a"_p)));
}
TEST(core,parse_or_undo){
	basic_context ctx("123 wasd");
	EXPECT_FALSE(parse_or_undo(ctx,"123 wast"_p));
	EXPECT_EQ(ctx.pos,0);
	EXPECT_FALSE(ctx.done());

	EXPECT_TRUE(parse_or_undo(ctx,"123 wasd"_p));
	EXPECT_TRUE(ctx.done());
	ctx.pos = 0;

	EXPECT_FALSE(parse_or_undo(ctx,decimal<int>+" was"+eoi));
	EXPECT_EQ(ctx.pos,0);
}
TEST(matcher,token){
	EXPECT_TRUE(parse("aaa",token('a')));
	EXPECT_FALSE(parse("bbb",token('a')));
	EXPECT_TRUE(parse("aaa",token('a')+token('a')+token('a')+eoi));
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
RC_GTEST_PROP(ezpz,simple_list_parser,(std::vector<int> vec)){
	std::ostringstream oss;
	for(auto x : vec){
		oss << x << ", ";
	}
	std::vector<int> unparsed;
	basic_context ctx(oss.str());
	parse(ctx,any(decimal<int> * insert(unparsed) + ", "));

	RC_ASSERT(vec == unparsed);
	RC_ASSERT(ctx.done());
}
RC_GTEST_PROP(ezpz,integer_decimal_parse_identity,(long long val)){
	basic_context ctx;
	ctx.input = std::to_string(val);
	long long parsed = -1;
	RC_ASSERT(parse(ctx,decimal<long long> * assign(parsed)));
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
	RC_ASSERT(parse(ctx,decimal<double> * assign(parsed)));
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
	RC_ASSERT(parse(ctx,text(s)));
	RC_ASSERT(ctx.done());
}
