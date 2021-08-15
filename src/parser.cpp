#include <bits/stdc++.h>
class context {
public:
	std::string input;
	bool debug = true;
	size_t pos = 0;
	size_t depth = 0;
	std::unordered_map<std::string,std::regex> regex_cache;
	char get(){
		return input[pos];
	}
	bool done(){
		return pos == input.size();
	}
	void indent(){
		for(size_t i = 0; i < depth; ++i)std::cout << '\t';
	}
};
class parse_object {
public:
	std::string comment;
	bool dbg_inline;
	size_t start_match;
	virtual ~parse_object() = default;
	virtual bool _match(context&) {return false;}
	virtual void _undo(context&) {};
	bool operator()(std::string_view sv){
		context ctx;
		ctx.input = sv;
		return (*this)(ctx);
	}
	bool operator()(context& ctx){
		return match_or_undo(ctx);
	}
	bool match_or_undo(context& ctx){
		size_t start_match = ctx.pos;
		if(match(ctx)){
			return true;
		}else{
			ctx.pos = start_match;
			_undo(ctx);
			return false;
		}
	}
	bool match(context& ctx){
		if(ctx.debug && !dbg_inline){
			ctx.indent();
			std::cout << "starting ";
			constexpr int lo = 7;
			constexpr size_t ro = 7;
			auto start_pos = std::max(0,int(ctx.pos)-lo);
			auto end_pos = std::min(ctx.input.size(),ctx.pos+ro);
			std::cout << '\"';
			std::cout << std::string_view{ctx.input.begin()+start_pos, ctx.input.begin()+end_pos};
			std::cout << '\"';
			std::cout << " " << comment;
			std::cout << std::endl;
			ctx.indent();
			std::cout << "          ";
			for(int i = start_pos; i <= end_pos; ++i){
				std::cout << (i == ctx.pos ? '^' : ' ');
			}
			std::cout << std::endl;
			ctx.depth++;
			auto prev_pos = ctx.pos;
			auto ret = _match(ctx);
			ctx.depth--;
			ctx.indent();
			if(!ret){
				std::cout << "failed ";
			}else{
				std::cout << "accepted ";
			}
			std::cout << '\"' << std::string_view{ctx.input.begin()+prev_pos,ctx.input.begin()+ctx.pos} <<'\"' << std::endl;
			return ret;
		}else{
			return _match(ctx);
		}
	}
	void undo(context& ctx){
		_undo(ctx);
	}
};
class f_parser_t : public parse_object {
public:
	std::function<bool(context&)> f;
	f_parser_t(decltype(f) f) : f(f) {};
	bool _match(context& ctx) override {
		return f(ctx);
	}
};
class parse_object_ref {
public:
	parse_object_ref(std::shared_ptr<parse_object> ptr) : 
		impl(ptr)
	{
	}
	bool match_or_undo(context& ctx){
		return impl->match_or_undo(ctx);
	}
	bool operator()(std::string_view sv){
		return (*impl)(sv);
	}
	bool operator()(context& ctx){
		return (*impl)(ctx);
	}
	bool match(context& ctx){
		return impl->match(ctx);
	}
	std::shared_ptr<parse_object> impl;
};
parse_object_ref f_parser(std::function<bool(context& ctx)> f, bool dbg_inline = false, std::string comment = ""){
	auto ret = parse_object_ref{std::make_shared<f_parser_t>(f)};
	ret.impl->dbg_inline = dbg_inline;
	ret.impl->comment = comment;
	return ret;
}
struct print_t {
	parse_object_ref operator()(std::string_view text){
		return f_parser([=](auto){
					std::cout << text << std::endl;
					return true;
				});
	}
} print;
template<typename FIRST = void, typename...ARGS>
class output_wraper{
	std::function<void(FIRST,ARGS...)> destination;
	public:
	output_wraper(auto dest) : destination(dest) {};
	output_wraper<ARGS...> operator<<(FIRST& val){
		return *this << std::move(val);
	}
	output_wraper<ARGS...> operator<<(FIRST&& val){
		return output_wraper<ARGS...>{[cb = destination,val = std::move(val)]
			(ARGS...args){
				cb(val,args...);
			}};
	}

};
template<>
class output_wraper<void> {
	std::function<void()> destination;
	public:
	output_wraper(auto dest) : destination(dest) {};
	~output_wraper(){
		if(destination){
			destination();
		}else{
			std::cout << "no dest";
		}
	}
};
template<typename...ARGS>
class ret_parse_object : public parse_object_ref {
	public:
	std::function<bool(context&, output_wraper<ARGS...>)> inner;
	std::function<void(ARGS...)> dest;
	ret_parse_object operator>>(print_t){

		auto ret = ret_parse_object{inner};
		ret.dest = [](ARGS...args){
			((std::cout << std::forward<ARGS>(args)),...);
			std::cout << std::endl;
		};
		return ret;
	}
	ret_parse_object operator>>(std::function<void(ARGS...)> dest){
		auto ret = ret_parse_object{inner};
		ret.dest = dest;
		return ret;
	}
	ret_parse_object(std::function<bool(context&,output_wraper<ARGS...>&&)> inner) : 
		parse_object_ref(
			f_parser([&](context& ctx){
				if(!dest){
					dest = [](ARGS...){};
				}
				return this->inner(ctx,output_wraper<ARGS...>{dest});
			})),
		inner(inner) 
	{
	}
};
template<typename...ARGS>
auto r_parser(std::invocable<context&,output_wraper<ARGS...>&&> auto in_f){
	std::function<bool(context&,output_wraper<ARGS...>&&)> f = in_f;
	return ret_parse_object<ARGS...>{f};
}
auto r_parser(std::invocable<context&> auto in_f){
	std::function<bool(context&)> f = in_f;
	return r_parser<std::string_view>([=](context& ctx,output_wraper<std::string_view>&& output) -> bool{
		int start_pos = ctx.pos;
		auto success = f(ctx);
		if(success){
			output << std::string_view{ctx.input.begin()+start_pos,ctx.input.begin()+ctx.pos};
		}
		return success;
	});
}
template<typename T>
auto assign(T& target){
	return [&](T&& t){
		target = std::forward<T>(t);
	};
}
parse_object_ref text_parser(std::string_view sv){
	return f_parser([=]
		(context& ctx){
			for(char c : sv){
				if(ctx.done())return false;
				char i = ctx.get();
				++ctx.pos;
				if(i != c)return false;
			}
			return true;
		}
	);
}
parse_object_ref operator+(parse_object_ref lhs, parse_object_ref rhs){
	return f_parser([=] 
				(context& ctx) mutable {
					return lhs.match(ctx) && rhs.match(ctx);
				},true);
}
parse_object_ref operator+(std::string_view sv, parse_object_ref rhs){
	return text_parser(sv) + rhs;
		
}
parse_object_ref operator+(parse_object_ref lhs, std::string_view sv){
	return lhs + text_parser(sv);
}
parse_object_ref operator|(parse_object_ref lhs, parse_object_ref rhs){
	return f_parser([=] 
				(context& ctx) mutable {
					return lhs.match_or_undo(ctx) || rhs.match(ctx);
				},true);
}
parse_object_ref operator|(parse_object_ref lhs, std::string_view rhs){
	return lhs | text_parser(rhs);
}
parse_object_ref operator|(std::string_view lhs, parse_object_ref rhs){
	return text_parser(lhs) | rhs;
}
class ws_t : public parse_object {
	bool _match(context& ctx){
		while(!ctx.done()){
			switch(ctx.get()){
			case ' ':
			case '\t':
			case '\n':
				++ctx.pos;
				break;
			default:
				return true;
			}
		}
		return true;
	}
};
parse_object_ref ws(std::make_shared<ws_t>());
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
class optional_t {} optional;
class any_t {} any;
auto operator>>(any_t,parse_object_ref rhs){
	return r_parser([=](context& ctx) mutable {
		while(rhs.match_or_undo(ctx) && !ctx.done()){}
		return true;
	});
}
parse_object_ref operator>>(any_t,std::string_view rhs){
	return any >> text_parser(rhs);
}
parse_object_ref operator>>(optional_t,parse_object_ref rhs){
	return f_parser([=](context& ctx) mutable {
			rhs.match(ctx);
			return true;
		});
}
parse_object_ref operator>>(optional_t,std::string_view text){
	return optional >> text_parser(text);
}
class star_t {} star;
parse_object_ref operator>>(star_t, parse_object_ref rhs){
	return rhs + (any >> rhs);
}
parse_object_ref operator>>(star_t,std::string_view text){
	return star >> text_parser(text);
}
struct not_t {} not_v;
parse_object_ref operator>>(not_t, parse_object_ref rhs){
	return f_parser([=](auto& ctx) mutable {
		return !rhs.match(ctx);
	});
}
parse_object_ref operator>>(not_t, std::string_view rhs){
	return not_v >> text_parser(rhs);
}
parse_object_ref fail = f_parser([](auto){return false;});
parse_object_ref eoi = f_parser([](context& ctx){return ctx.done();});
parse_object_ref digit = f_parser([](context& ctx){
	if(ctx.done())return false;
	if(std::isdigit(ctx.get())){
		++ctx.pos;
		return true;
	}
	return false;
});
auto graph_letter = r_parser([](context& ctx){
	if(ctx.done())return false;
	if(std::isgraph(ctx.get())){
		++ctx.pos;
		return true;
	}
	return false;
});
auto string = "\"" + (any >> (not_v >> "\"")) + "\"";
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
template<typename integer, int base>
auto number = r_parser<integer>([](context& ctx, auto&& output){
	static_assert(base <= 10);
	static_assert(base >= 2);
	if(ctx.done())return false;
	bool negative = false;
	if(ctx.get() == '-'){
		negative = true;
		++ctx.pos;
	}else if(ctx.get() == '+'){
		++ctx.pos;
	}
	if(ctx.done())return false;
	integer ret = 0;
	bool invalid = true;
	while(!ctx.done() && std::isdigit(ctx.get())){
		invalid = false;
		ret *= base;
		ret += ctx.get()-'0';
		++ctx.pos;
	}
	if(invalid)return false;
	if(negative)ret = -ret;
	output << ret;
	return true;
});
template<typename integer>
auto& decimal = number<integer,10>;
//(r_parser <- rf_parser) => operator>> to assign ret value destinations
//new f_parser feeds destinations to old rf_parser
template<typename vector>
auto push(vector& vec){
	return [vec = &vec](typename vector::value_type&& value){
		vec->push_back(std::forward<typename vector::value_type>(value));
	};
};
auto max(int val){
	struct type {
		int val;
		parse_object_ref operator>>(std::string_view sv){
			return *this >> text_parser(sv);
		}
		parse_object_ref operator>>(parse_object_ref inner){
			return f_parser([=](auto& ctx) mutable {
					for(int i = 0; i < val; ++i){
						if(!inner.match_or_undo(ctx))return true;
					}
					return true;
				});
		}
	};
	return type{val};
}
auto min(int val){
	struct type {
		int val;
		parse_object_ref operator>>(std::string_view sv){
			return *this >> text_parser(sv);
		}
		parse_object_ref operator>>(parse_object_ref inner){
			return f_parser([=](auto& ctx) mutable {
					for(int i = 0; i < val; ++i){
						if(!inner.match(ctx))return false;
					}
					while(inner.match_or_undo((ctx))){};
					return true;
				});
		}
	};
	return type{val};
}
auto minmax(int val1, int val2){
	struct type {
		int val1,val2;
		parse_object_ref operator>>(std::string_view sv){
			return *this >> text_parser(sv);
		}
		parse_object_ref operator>>(parse_object_ref inner){
			return f_parser([=](auto& ctx) mutable {
					for(int i = 0; i < val1; ++i){
						if(!inner.match(ctx))return false;
					}
					for(int i = 0; i < (val2-val1); ++i){
						if(!inner.match_or_undo(ctx))return true;
					}
					return true;
				});
		}
	};
	return type{val1,val2};
}
auto exact(int val){
	return minmax(val,val);
}
auto until(std::string_view pattern){
	return r_parser<std::string_view>([=](auto& ctx,auto&& output){
		const std::string str{pattern};
		auto [regex_iter,b] = ctx.regex_cache.try_emplace(str,str);
		std::smatch match;
		const auto& c_input = ctx.input;
		std::regex_search(c_input.begin()+ctx.pos,c_input.end(),match,regex_iter->second);
		if(match.empty())return false;
		ctx.pos += match.position();
		auto ret = std::string_view{
			c_input.begin() + ctx.pos,
			c_input.begin() + ctx.pos + match.length()
		};
		ctx.pos += match.length();
		output << ret;
		return true;
	});
}
auto match(std::string_view pattern){
	return r_parser<std::string_view>([=](auto& ctx,auto&& output){
		const std::string str{pattern};
		auto [regex_iter,b] = ctx.regex_cache.try_emplace(str,str);
		std::smatch match;
		const auto& c_input = ctx.input;
		std::regex_search(c_input.begin()+ctx.pos,c_input.end(),match,regex_iter->second);
		if(match.empty())return false;
		if(match.position() != 0)return false;
		auto ret = std::string_view{
			c_input.begin() + ctx.pos,
			c_input.begin() + ctx.pos + match.length()
		};
		output << ret;
		ctx.pos += match.position();
		return true;
	});
}
auto text(std::string_view& sv){
	return f_parser([&](context& ctx){
		for(int i = 0; i < sv.size(); ++i){
			if(sv[i] != ctx.get())return false;
			++ctx.pos;
		}
		return true;
	});
}
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
	

	struct node {
		std::string_view name;
		std::vector<node> children;
		std::string_view text_content;
	};
	ret_parse_object<node> xml_node = r_parser<node>([&](auto& ctx,auto&& output){
		node ret;
		if((
			"<"+ws+(match("\\w+") >> assign(ret.name)) + ws +">"+
			(
				(any >> (xml_node >> push(ret.children))) |
				(until("</") >> assign(ret.text_content))) +
			"</"+ws+text(ret.name)+ws+">")(ctx)){

			output << ret;
			return true;
		}
		return false;
	});
	node root;
	(xml_node >> assign(root))("<hey></hey>");
}
