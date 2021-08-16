#include "helper.hpp"
#include <iostream>

parse_object_ref print_t::operator()(std::string_view text){
	return f_parser([=](auto){
				std::cout << text << std::endl;
				return true;
			});
}
