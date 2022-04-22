#include "ezpz/ezpz.hpp"
#include <gtest/gtest.h>
using namespace ezpz;
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
TEST(consumer,compress){
	struct Foo {
		int a = 0, b = 0;
	} target;
	auto foo_p = (!decimal<int>+" "_p+!decimal<int>)*compress<Foo>;

	EXPECT_TRUE(parse("1 2",foo_p,target));
	EXPECT_EQ(target.a,1);
	EXPECT_EQ(target.b,2);

	EXPECT_FALSE(parse("1 a",foo_p,target));
}

TEST(consumer,ret){
	int val = 0;
	parse("   ",ws*retd(2)*assign(val));
	EXPECT_EQ(val,2);

	val = 0;
	parse("   ",ws+ret<2>*assign(val));
	EXPECT_EQ(val,2);

	std::string_view text;
	parse("   ",ws*retd<std::string_view>("hey")*assign(text));
	EXPECT_EQ(text,"hey");
}
