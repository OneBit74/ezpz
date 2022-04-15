#include "ezpz/ezpz.hpp"
#include <iostream>

using namespace ezpz;

int main(){
	using ctx_t = basic_context;

	auto whitespace = any(" "_p | "\r" | "\n" | "\t");
	auto onenine = "1"_p | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9";
	auto digit = "0" | onenine;
	auto hex = digit | "A" | "B" | "C" | "D" | "E" | "F" | "a" | "b" | "c" | "d" | "e" | "f";
	auto number = optional("-"_p) 
		+ ("0"_p | onenine+any(digit)) 
		+ optional("."+plus(digit)) 
		+ optional(("E"_p | "e") + optional("-"_p|"+"_p) + any(digit));

	auto string = "\""_p + any(
			  notf("\""_p) + notf("\\") + single
			| "\\" + ("\""_p | "\\" | "/" | "b" | "f" | "n" | "r" | "t" | "u"+hex+hex+hex+hex)
			) + "\"";

	polymorphic_rpo<ctx_t> value;
	auto value_r = ref(value);
	auto element = ws + value_r + ws;
	auto elements = element+any(","_p+element);
	auto array = "["_p + (elements | whitespace) + "]";
	auto member = whitespace + string + whitespace + ":" + element;
	auto members = member + any(","+member);
	auto object = "{"_p + (members | whitespace) + "}";
	auto value_impl = make_poly<ctx_t>(whitespace + (string | number | object | array | "true" | "false" | "null") + whitespace);
	value = value_impl;

	auto json = element;

	std::cout << parse("{\"hey\":\"i'm json\"}",json+eoi) << std::endl;
	std::cout << parse("[1,2,3,4]",json+eoi) << std::endl;
	std::cout << parse("not json",json+eoi) << std::endl;
	
}
