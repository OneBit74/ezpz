#include <ezpz/ezpz.hpp>
#include <iostream>

using namespace ezpz;

int main(){
	// arguments
	// path ( "cmd1" | "cmd2" | "cmd3" )
	// --help
	// --threads [num]
	
	int threads = 1; // this is the default

	auto cmd = recover("cmd1"_p | "cmd2" | "cmd3");
	auto print_help = [](){
		std::cout << "help" << std::endl;
	};
	auto nws_string = capture(string | plus(notf(" "_p | "\t" | "\n") + single));
	auto parser = ws + nws_string + ws + cmd +
		any(
			ws+"--"+
			recover( ("help"_p * print_help)
			| ("threads" + ws + decimal<int>*assign(threads))
			)
		);	
	basic_context ctx("./my/file asd --thread 4 --help");
	ctx.debug = false;
	if(!parse(ctx,parser+eoi) || ctx.failed()){
		// parse unsuccessful
	}
}
