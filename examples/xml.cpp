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
	using ezpz_output = TLIST<node>;

	bool _parse(auto& ctx, node& ret){
		auto attribute_list = any((ident+recover(ws+"="+ws+ident+ws))*[&](auto&& key, auto&& value){ret.attributes.emplace_back(key,value);});
		return parse(ctx,
			"<" + recover(ws + (ident*assign(ret.name)) + ws + attribute_list + ">"_p) + ws +
			(
				plus(*this * insert(ret.children)) |
				(capture(any(notf("</"_p) + single)) * assign(ret.text_content))
			) + ws + 
			recover("</"+ws+text(ret.name)+ws+">"));
	}
} xml_node;

int main(){
	node root;
	using ctx_t = basic_context;
	/* using ctx_t = graph_context<basic_context, decltype(ident),text_p,xml_node_p,ref_text_p>; */
	ctx_t ctx("<hey k1=v1 k2 = v2><p>This is</paa><b>xml</b></hey>");
	/* ctx.debug = true; */
	parse(ctx,(xml_node*assign(root)+eoi)| print("error"));
	/* auto p = recover(regex("\\w+")); */
	/* using P = decltype(p); */
	/* static_assert(parseable<ctx_t,decltype(regex("\\w+")),std::string_view>); */
	/* p._parse(ctx); */

	/* parse(ctx,); */
	/* parse(ctx,regex("\\w+")); */
	/* std::cout << root.name << std::endl; */
	/* std::cout << root.children.size() << std::endl; */
	/* for(auto& child : root.children)std::cout << "child: " <<  child.name << std::endl; */
	/* ctx.print(); */
}
