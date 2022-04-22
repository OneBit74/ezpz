/* #include "ezpz/ezpz.hpp" */
#include "ezpz/ezpz.hpp"
#include "ezpz/extra/cin_context.hpp"
#include <gtest/gtest.h>
using namespace ezpz;

struct mock_cin {
	std::streambuf* orig;
	std::istringstream input;
	mock_cin(const char* mock_input)
		: orig(std::cin.rdbuf())
		, input(mock_input)
	{
		std::cin.rdbuf(input.rdbuf());
	}
	~mock_cin(){
		std::cin.rdbuf(orig);
	}
};
TEST(cin_context, mock_test){
	mock_cin mock("hey");
	std::string s;
	std::cin >> s;
	EXPECT_EQ(s,"hey");
}
TEST(cin_context, parser_test){
	{
		mock_cin mock("hey");
		auto ctx = make_cin_context();
		EXPECT_FALSE(ctx.done());
		EXPECT_EQ(ctx.token(),'h');
		EXPECT_TRUE(parse(ctx,"hey"_p));
		EXPECT_TRUE(ctx.done());
	}
	{
		mock_cin mock("1 2 3 4");
		auto ctx = make_cin_context();
		EXPECT_TRUE(parse(ctx,any(decimal<int>+ws)+eoi));
	}
}
