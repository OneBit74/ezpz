#include <gtest/gtest.h>

TEST(ezpz,text_parser){
	bool success = false;
	(text_parser("hello") + f_parser([&](auto&){
		success = true;
	}))("hello");
	EXPECT_TRUE(success);
}
