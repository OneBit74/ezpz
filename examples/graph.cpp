#include "ezpz/ezpz.hpp"
#include <iostream>

template<typename...TS>
struct graph_context : public basic_context {
	using LIST = TLIST<TS...>;
	
	std::stack<int> nodes;
	int counter = 0;
	std::ostringstream oss;

	graph_context(std::string_view sv) : basic_context(std::string{sv}) {
		oss << "strict graph {\n";
		oss << "\t0[label=\"root\"];\n";
		nodes.push(counter++);
	}

	void print(){
		std::cout << oss.str() << "}\n";
	}

	int notify_enter(auto& p){
		if constexpr(contains<LIST,std::decay_t<decltype(p)>>::value){
			nodes.push(counter++);
		}
		return basic_context::notify_enter(p);
	}
	void notify_leave(auto& p, bool success, int start){
		basic_context::notify_leave(p,success,start);
		using parser_t = std::decay_t<decltype(p)>;
		if constexpr(contains<LIST,parser_t>::value){
			auto id = nodes.top();
			oss << "\t" << id << "[label=\""<< type_name<parser_t>() << "\"]" << ";\n";
			nodes.pop();
			oss << "\t" << nodes.top() << " -- " << id << ";\n";
			
		}
	}
};
int main(){
	graph_context<ws_p,std::decay_t<decltype(decimal<int>)>> ctx("123 456");
	parse(ctx,decimal<int>+ws+decimal<int>);
	ctx.print();
}
