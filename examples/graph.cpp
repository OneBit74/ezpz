#include "matcher.hpp"
#include "ezpz.hpp"
#include "unparser.hpp"
#include <iostream>

struct GraphFactory {
	size_t uid_counter = 0;
	struct Node {
		std::string_view name;
		std::string_view captured;
		std::vector<Node> children;
		size_t uid;

		void print() const {
			std::cout << "\t" << uid << "[label=\"" << name << "\\n" << captured << "\"];\n";
			for(const auto& child : children) {
				child.print();
				std::cout << "\t" << uid << " -- " << child.uid << "\;\n";
			}
		}
	};
	GraphFactory(){
		push_node("root");
	}
	std::stack<Node> state;

	using node_handle = int;
	void push_node(std::string_view name){
		state.emplace(name,std::string_view{},std::vector<Node>{},uid_counter++);
	}
	void pop_node(bool success, std::string_view captured){
		auto cur = state.top();
		state.pop();
		if(success && !state.empty()){
			cur.captured = captured;
			state.top().children.emplace_back(std::move(cur));
		}
	}
	void print_all(){
		std::cout << "strict graph {\n";
		state.top().print();
		std::cout << "}\n";
	}
} graph_factory;

template<parser P>
struct node_parser : public parse_object {
	using UNPARSED_LIST = typename P::UNPARSED_LIST;
	using active = typename P::active;

	std::string_view _name;
	P p;
	
	node_parser(std::string_view name, P&& p) : _name(name), p(std::move(p)) {}
	node_parser(std::string_view name, P& p) : _name(name), p(p) {}

	bool _parse(context& ctx, auto& ... args){
		graph_factory.push_node(_name);
		auto start = ctx.pos;
		auto ret = parse(ctx,p,args...);
		std::string_view captured = std::string_view{ctx.input.c_str()+start,ctx.pos-start};
		graph_factory.pop_node(ret,captured);
		return ret;
	}
	void _undo(context& ctx) {
		p._undo(ctx);
	}
	bool _match(context& ctx){
		graph_factory.push_node(_name);
		auto start = ctx.pos;
		auto ret = match(ctx,p);
		std::string_view captured = std::string_view{ctx.input.c_str()+start,ctx.pos-start};
		graph_factory.pop_node(ret,captured);
		return ret;
	}
	bool dbg_inline() {
		return true;
	}
};
parser auto node(std::string_view name, parser auto&& parser){
	using P_t = typename std::decay_t<decltype(parser)>;
	return node_parser<P_t>{name,std::forward<P_t>(parser)};
}
int main(){
	auto my_num = node("number",decimal<int>);
	rpo<> all;
	all = my_num+(optional >> (ws+ref(all)));
	context ctx("123 456 789 420 69");
	match(ctx, all);
	graph_factory.print_all();
}