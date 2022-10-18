#include "ezpz/ezpz.hpp"
#include <gtest/gtest.h>
using namespace ezpz;

int main(){


	// matching static text
	{
		auto parser = "only accept this text"_p;
		bool result = parse("this is a different text", parser);
		EXPECT_FALSE(result);

		result = parse("only accept this text", parser);
		EXPECT_TRUE(result);

		// same as
		basic_context ctx("only accept this text");
		result = parse(ctx, parser);
	}

	// + and |
	{
		auto p1 = "Hello "_p;
		auto p2 = "World"_p;
		auto p3 = p1+p2; // parse p1 and p2

		EXPECT_TRUE(parse("Hello World",p1));
		EXPECT_FALSE(parse("Hello World",p2));
		EXPECT_TRUE(parse("Hello World",p3));

		auto p4 = p1 | p2; // parse p1 or p2
		EXPECT_TRUE(parse("Hello ",p4));
		EXPECT_TRUE(parse("World",p4));
	}

	// * and !
	{
		EXPECT_TRUE(parse("123",decimal<int>));

		int val = 0;

		parse("123",decimal<int>,val);
		EXPECT_EQ(val, 123);
		val = 0;

		
		parse("123",decimal<int>*
			[&](int x){val = x;} // this is a consumer
		);
		EXPECT_EQ(val, 123);
		val = 0;

		auto times2 = [](int x){return x*2;};
		parse("123", decimal<int>*times2, val);
		EXPECT_EQ(val, 246);
		val = 0;

		auto add = [](int x, int y){return x+y;};
		parse("123 654",(decimal<int>+" "+decimal<int>)*add, val);
		EXPECT_EQ(val, 777);
		val = 0;

		// works with templates
		// matches maximum output parameters
		auto add_v2 = [](auto...nums){return (nums+...+0);};
		parse("123 654",(decimal<int>+" "+decimal<int>)*add_v2, val);
		EXPECT_EQ(val, 777);
		val = 0;

		// silence output with !
		parse("123 654",!decimal<int>+" "+decimal<int>, val);
		EXPECT_EQ(val, 654);
		val = 0;
	}

	// eoi matches end of input
	{
		EXPECT_FALSE(parse("hey",eoi));
		EXPECT_TRUE(parse("hey","hey"_p+eoi));
	}

	// quantifier
	{
		auto option = optional("hey"_p)+eoi;
		EXPECT_TRUE(parse("",option));
		EXPECT_TRUE(parse("hey",option));
		EXPECT_FALSE(parse("hey and more",option));

		auto anyp = any("a"_p)+eoi;
		EXPECT_TRUE(parse("aaaaaaaa", anyp));
		EXPECT_TRUE(parse("", anyp));

		auto plusp = plus("a"_p)+eoi;
		EXPECT_TRUE(parse("aaaaaaaa", plusp));
		EXPECT_TRUE(parse("a", plusp));
		EXPECT_FALSE(parse("", plusp));

		auto minp = min<3>("a"_p)+eoi;
		EXPECT_FALSE(parse("a",minp));
		EXPECT_FALSE(parse("aa",minp));
		EXPECT_TRUE (parse("aaa",minp));
		EXPECT_TRUE (parse("aaaa",minp));

		auto maxp = max<3>("a"_p)+eoi;
		EXPECT_TRUE(parse("a",maxp));
		EXPECT_TRUE(parse("aa",maxp));
		EXPECT_TRUE(parse("aaa",maxp));
		EXPECT_FALSE(parse("aaaa",maxp));
	}

	// recursion
	{
		using ctx_t = basic_context;
		// rpo stands for returning parser object
		{
			rpo<ctx_t> parser;
			// ref(...) takes a reference to a parser
			// because rpo<...> can't/shouldn't be coppied
			parser = "a"+optional(ref(parser));
			EXPECT_TRUE(parse("aaaaa",ref(parser)+eoi));
		}

		// rpo makes allocation
		// to avoid allocation we can do this
		{
			polymorphic_rpo<ctx_t> parser;
			auto parser_impl = make_poly<ctx_t>("a"+optional(ref(parser)));
			parser = parser_impl; 
			// parser does not take ownership of parser_impl
			// so parser_impl needs to stay alive on the stack

			EXPECT_TRUE(parse("aaaaa",parser+eoi));
		}
		// or this
		{
			auto parser = make_rpo<>([](auto& ctx, auto& self){
				return parse(ctx,"a"_p+optional(self));
			});
			EXPECT_TRUE(parse("aaaaa",parser+eoi));
		}
	}

	// debugging and error handeling
	{
		basic_context ctx("abc");
		ctx.debug = true;
		parse(ctx,"a"_p+"b"_p+"c"_p);
		// with the debug flag on basic_context outputs
		// usefull information to the console

		EXPECT_TRUE(parse("a","a" + recover("bc"_p)));
		// outputs something like "expected 'bc'"
		// and continues the parsing

		EXPECT_TRUE(parse("",recover("a"_p+"b"_p))); // expected a
		EXPECT_TRUE(parse("a",recover("a"_p+"b"_p))); // expected b
		EXPECT_TRUE(parse("ab",recover("a"_p+"b"_p)));

		EXPECT_TRUE(parse("",recover("a"_p | "b"_p))); // expected a or b
		EXPECT_TRUE(parse("a",recover("a"_p | "b"_p)));
		EXPECT_TRUE(parse("b",recover("a"_p | "b"_p)));
	}

}
