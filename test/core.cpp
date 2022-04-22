#include "ezpz/ezpz.hpp"
#include <gtest/gtest.h>
using namespace ezpz;

namespace consume_with_function_ptr {
	void nothing(){
	}
	int ret2(){
		return 2;
	}
	int ret_times_two(int x){
		return 2*x;
	}
	TEST(core, consume_with_function_ptr){
		EXPECT_TRUE(parse("",no_parser*nothing));
		int res = 0;
		EXPECT_TRUE(parse("",no_parser*ret2,res));
		EXPECT_EQ(res,2);
		res = 0;
		EXPECT_TRUE(parse("",no_parser*ret2*ret_times_two,res));
		EXPECT_EQ(res,4);
	}
}
TEST(core, multi_returning_alternative){
	auto parser = 
		"a"_p*ret<int8_t(2)>
		| "b"_p*ret<int16_t(3)>
		| "c"_p*ret<int32_t(4)>;
	auto casted_parser = parser*cast<int>;

	static_assert(std::is_same_v<decltype(parser)::UNPARSED_LIST,
			TLIST<std::variant<int8_t,int16_t,int32_t>>>);
	int i = 0;
	EXPECT_TRUE(parse("a",casted_parser,i));
	EXPECT_EQ(i,2);
	EXPECT_TRUE(parse("b",casted_parser,i));
	EXPECT_EQ(i,3);
	EXPECT_TRUE(parse("c",casted_parser,i));
	EXPECT_EQ(i,4);

	auto p2 = fail | "z"_p*ret<2> | "x"_p*ret<3>;
	/* print_types<typename decltype(p2)::UNPARSED_LIST> asd; */
	static_assert(std::is_same_v<
			typename decltype(p2)::UNPARSED_LIST,
			TLIST<std::optional<int>>>);
	std::optional<int> res;
	EXPECT_TRUE(parse("z",p2,res));
	EXPECT_EQ(res,2);
	EXPECT_TRUE(parse("x",p2,res));
	EXPECT_EQ(res,3);
}
TEST(core,and){
	EXPECT_TRUE(parse("ab","a"_p+"b"_p));
	int val = 0;
	parse("",((no_parser*ret<1>)+!(no_parser*ret<2>))*assign(val));
	EXPECT_EQ(val,2);
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
TEST(core,make_rpo){
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
TEST(core,or_undo_lhs){
	basic_context ctx("aab");;
	EXPECT_TRUE(parse(ctx,"aaa"_p | "aab"));
}
TEST(core,make_rpo_recursive){
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
