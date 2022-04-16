#include "matcher.hpp"
#include "ezpz.hpp"
#include "unparser.hpp"
#include <iostream>

using namespace ezpz;

int main(int argc, char** argv){
	// arguments
	// path ( "cmd1" | "cmd2" | "cmd3" )
	// --help
	// --threads [num]
	// --
	auto check_cmd = [](auto cmd){
		if(cmd != "cmd1" && cmd != "cmd2" && cmd != "cmd3"){
			std::cout << "invalid command: " << cmd << std::endl;
		}
	};
	auto print_help = [](){
		std::cout << "help" << std::endl;
	};
	int threads = 1;
	auto nws_string = capture(string | plus >> ((not_v >> (" "_p | "\t" | "\n")) + single));
	auto parser = ws+nws_string*print_all + ws + nws_string*check_cmd + (any >>(
				ws+"--"+
				( ("help"_p * print_help)
				| ("threads" + ws + decimal<int>*assign(threads))
				)
			));	
	context ctx("./my/file asd --threads 4 --help");
	ctx.debug = false;
	match(ctx,(parser+eoi)|print("error"));
}
