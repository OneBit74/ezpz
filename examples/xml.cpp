#include <string_view>
#include <vector>
#include "ezpz/ezpz.hpp"
#include "ezpz/extra/graph_context.hpp"
#include <string>
using namespace ezpz;

struct node {
	std::string_view name;
	std::vector<node> children;
	std::vector<std::pair<std::string,std::string>> attributes;
	std::string_view text_content;
};

auto ident = capture(regex("\\w+"));
struct xml_node_p {
	using active = active_f;
	using UNPARSED_LIST = TLIST<node>;

	bool _parse(auto& ctx, node& ret){
		auto attribute_list = any((!ident+ws+"="+ws+!ident+ws)*[&](auto&& key, auto&& value){ret.attributes.emplace_back(key,value);});
		return parse(ctx,
			"<" + ws + (ident*assign(ret.name)) + ws + attribute_list + ">" + ws +
			(
				plus(*this * insert(ret.children)) |
				(capture(any(notf("</"_p) + single)) * assign(ret.text_content))
			) + ws + 
			"</"+ws+text(ret.name)+ws+">");
	}
} xml_node;

int main(){
	
	node root;
	graph_context<basic_context, decltype(ident),text_p,xml_node_p,ref_text_p> ctx("<hey k1=v1 k2 = v2><p>This is</p><b>xml</b></hey>");
	/* ctx.debug = true; */
	parse(ctx,(xml_node*assign(root)+eoi)| print("error"));
	/* std::cout << root.name << std::endl; */
	/* std::cout << root.children.size() << std::endl; */
	/* for(auto& child : root.children)std::cout << "child: " <<  child.name << std::endl; */
	ctx.print();
}
