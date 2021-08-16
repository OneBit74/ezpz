#include <gtest/gtest.h>
#include "parse_object.hpp"
#include "matcher.hpp"

TEST(ezpz,text_parser){
	bool success;
	auto parser = text_parser("hello") + f_parser([&](auto&){
		success = true;
		return true;
	});

	success = false;
	parser("hello");
	EXPECT_TRUE(success);
	success = false;

	parser("hell");
	EXPECT_FALSE(success);
	success = false;

	parser("helloo");
	EXPECT_TRUE(success);
	success = false;

	parser("");
	EXPECT_FALSE(success);
	success = false;
}
