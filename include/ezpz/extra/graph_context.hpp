#include "ezpz/context.hpp"
#include <iostream>
#include <vector>

struct graph_option_show_failures;
struct graph_option_show_all;

template<basic_context_c base, typename...TS>
struct graph_context : public base {
	using LIST = TLIST<TS...>;

	static constexpr bool show_failures = contains<LIST,graph_option_show_failures>::value;
	static constexpr bool show_all = contains<LIST,graph_option_show_all>::value;
	
	struct node {
		bool invisible = true;
		bool success = false;
		std::string label;
		std::string_view matched_text;
		std::vector<node> children;
	};
	std::stack<node> nodes;

	graph_context(std::string_view sv) : base(std::string{sv}) {
		nodes.push(node{false,true,"root","",{}});
	}

	void print(){
		std::cout << "strict graph {\n";
		int counter = 0;
		print_dfs(nodes.top(),counter);
		std::cout << "}\n";
	}
	void print_dfs(const node& cur, int& counter){
		auto id = counter;
		std::cout << "\t"<< id <<" [label=\"" << cur.label;
		if(cur.matched_text.size() != 0){
			std::cout << "\\n\\\"" << cur.matched_text << "\\\"";
		}
		std::cout << "\", color=" << (cur.success ? "green" : "red");
		std::cout << "];\n";
		counter++;
		for(const auto& child : cur.children){
			auto child_id = counter;
			print_dfs(child,counter);
			std::cout << "\t" << id << " -- " << child_id << ";\n";
		}
	}

	int notify_enter(auto& p){
		using parser_t = std::decay_t<decltype(p)>;
		bool wanted = show_all || contains<LIST,parser_t>::value;

		if(is_dbg_inline(p) && !wanted) return 0;
		node new_node;
		new_node.invisible = !wanted;
		new_node.label = type_name<parser_t>();
		if(new_node.label.size() > 15)new_node.label = new_node.label.substr(0,15)+"...";
		nodes.push(new_node);
		return base::pos;
	}
	void notify_leave(auto& p, bool success, int start){
		using parser_t = std::decay_t<decltype(p)>;
		bool wanted = show_all || contains<LIST,parser_t>::value;

		if(is_dbg_inline(p) && !wanted) return;
		auto cur = std::move(nodes.top());
		nodes.pop();
		cur.success = success;
		if(!show_failures && !success)return;
		if(cur.invisible){
			std::move(
					std::begin(cur.children),
					std::end(cur.children),
					std::back_inserter(nodes.top().children));
		}else{
			cur.matched_text = std::string_view{std::begin(base::input)+start,std::begin(base::input)+base::pos};
			nodes.top().children.push_back(std::move(cur));
		}
	}
};
