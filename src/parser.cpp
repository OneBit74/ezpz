#include <bits/stdc++.h>
#include <fmt/core.h>
#include "context.hpp"
#include "parse_object.hpp"
#include "matcher.hpp"
#include "consumer.hpp"


/* class list_parser : public parser { */
	
/* 	auto parse_list(ctx){ */
/* 		list ret; */
/* 		(ws + "[" + optional >> (list_elem >> push(ret) + any >> ("," + ws + list_elem >> push(ret) + ws)) + "]")(ctx); */
/* 		return ret; */
/* 	} */
/* 	auto parse_list_elem(ctx){ */
/* 		list_elem ret; */
/* 		(at_least_one >> regex("\w") >> assign(ret.value))(ctx); */
/* 		return ret; */
/* 	} */
/* } */

/* template<typename FIRST, typename...ARGS> */
/* class ret_parse_object : public parse_object_ref { */
/* public: */
/* 	ret_parse_object(parse_object_ref self) : parse_object_ref(self) {} */
/* 	std::function<void(FIRST&&)> apply; */
/* 	ret_parse_object<ARGS...> operator>>(std::function<FIRST&&> apply){ */
/* 		this->apply = apply; */
/* 		return ret_parse_object<ARGS...> */
/* 	} */
/* 	ret_parse_object<ARGS...> operator>>(FIRST& ref){ */
/* 		apply = [&](FIRST&& val){ */
/* 			ref = val; */
/* 		}; */
/* 	} */
/* }; */
/* auto number = (optional >> "+" | "-") + (star >> digit); */

//(r_parser <- rf_parser) => operator>> to assign ret value destinations
//new f_parser feeds destinations to old rf_parser
int main(){
	context ctx;
	/* ctx.input = "  hey  "; */
	/* ((ws + (optional >> "hey" + print("inner\n") + fail) + ws + print("done\n")) | print("fail\n"))(ctx); */
	/* ctx.input = "heyheyhey"; */
	/* (((any >> "hey" + print("itter")) + eoi + print("done")) | print("fail"))(ctx); */
	/* ctx.input="\"hey you rock\""; */
	/* (string + print("accept") | print("fail"))(ctx); */
	/* ctx.input = "-2342, 4"; */
	/* ((number ) + eoi + print("accept"))(ctx); */
	/* ((r_parser<int>([](context&,output_wraper<int>&& output)->bool{ */
	/* 	output << 2; */
	/* 	return true; */
	/*  }) >> [](int x){std::cout << x << std::endl;}))(ctx); */
	//presedence
	//1.output
	//2.concatenation of parse_obj
	//3.modifiers
	/* (any >> (decimal<int> >> print) + ", ")(ctx); */
	/* std::vector<int> res; */
	/* (decimal<int> >> push(res))(ctx); */
	/* ((decimal<int> >> push(res))+", "+(decimal<int> >> print))(ctx); */
	/* std::cout << res.size() << std::endl; */
	/* for(auto x : res)std::cout << x << std::endl; */


	/* ctx.input = "bb"; */
	/* ((optional>>"a") + print("accepted") | print("failed"))(ctx); */
	/* (((re_match("ab*c") >> print) + print("accepted")) | print("failed"))("abbdbc"); */

	//output composition decimal<int> + ", "+ decimal<int> >> print >> print
	
	(match("abc") + print("accepted"))("abc");

	struct node {
		std::string_view name;
		std::vector<node> children;
		std::string_view text_content;
	};
	ret_parse_object<node> xml_node = r_parser<node>([&](auto& ctx,auto&& output){
		node ret;

		if((
			"<"+ws+(match("\\w+") >> assign(ret.name)) + ws +">"+ws+
			(
				(min(1) >> (xml_node >> push(ret.children))) |
				(until("</") >> assign(ret.text_content))
			) +
			ws + "</"+ws+text(ret.name)+ws+">")(ctx)){

			output << ret;
			return true;
		}
		return false;
	},false,"xml_node");



	node root;
	ctx.debug = true;
	ctx.input = "<hey><p>you suck </p><b>big time! </b></hey>";
	(xml_node >> assign(root))(ctx);
	std::cout << root.name << std::endl;
	std::cout << root.children.size() << std::endl;
	for(auto& child : root.children)std::cout << "child: " <<  child.name << std::endl;
}
