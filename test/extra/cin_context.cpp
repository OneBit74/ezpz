/* #include "ezpz/ezpz.hpp" */
#include "ezpz/ezpz.hpp"
#include "ezpz/extra/cin_context.hpp"
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>
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
RC_GTEST_PROP(cin_context,mock,(std::string&& some_text)){
	mock_cin mock(some_text.data());
	std::istreambuf_iterator<char> begin(std::cin), end;
	std::string s(begin, end);
	RC_ASSERT(s == some_text);
}
RC_GTEST_PROP(cin_context,parser_prop,(std::string&& some_text)){
	std::cin.unsetf(std::ios_base::skipws);
	mock_cin mock(some_text.data());
	auto ctx = make_cin_context();
	if(some_text.size() > 0){
		RC_ASSERT(!ctx.done());
		RC_ASSERT(ctx.token() == some_text[0]);
	}
	RC_ASSERT(parse(ctx,text_p{some_text}));
	RC_ASSERT(ctx.done());
	std::cin.setf(std::ios_base::skipws);
}
TEST(cin_context, whitespace){
	auto data = "\t  \t\n  \t";
	std::cin.unsetf(std::ios_base::skipws);
	mock_cin mock(data);
	auto ctx = make_cin_context();
	EXPECT_FALSE(ctx.done());
	EXPECT_EQ(ctx.token(),'\t');
	EXPECT_TRUE(parse(ctx,text_p{data}));
	EXPECT_TRUE(ctx.done());
	std::cin.setf(std::ios_base::skipws);
}
TEST(cin_context, parser){
	mock_cin mock("hey");
	auto ctx = make_cin_context();
	EXPECT_FALSE(ctx.done());
	EXPECT_EQ(ctx.token(),'h');
	EXPECT_TRUE(parse(ctx,"hey"_p));
	EXPECT_TRUE(ctx.done());
}
TEST(cin_context, complex_parser){
	mock_cin mock("1 2 3 4");
	auto ctx = make_cin_context();
	EXPECT_TRUE(parse(ctx,any(decimal<int>+ws)+eoi));
}
