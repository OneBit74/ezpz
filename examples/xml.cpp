#include <bits/stdc++.h>
#include "ezpz/ezpz.hpp"


int main(){
	
	struct node {
		std::string_view name;
		std::vector<node> children;
		std::string_view text_content;
	};
	basic_rpo<node> xml_node;
	xml_node = fr_parser<node>([&](auto& ctx, node& ret){
		return match(ctx,
			"<"+ws+(capture(regex("\\w+")) * assign(ret.name)) + ws +">"+ws+
			(
				plus(ref(xml_node) * insert(ret.children)) |
				(capture(any(notf("</"_p) + single)) * assign(ret.text_content))
			) +
			ws + "</"+ws+text(ret.name)+ws+">");
	});



	node root;
	basic_context ctx("<hey><p>This is</p><b>xml</b></hey>");
	ctx.debug = true;
	match(ctx,ref(xml_node) * assign(root)+eoi | print("error"));
	std::cout << root.name << std::endl;
	std::cout << root.children.size() << std::endl;
	for(auto& child : root.children)std::cout << "child: " <<  child.name << std::endl;
}
