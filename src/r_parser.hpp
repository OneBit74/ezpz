#pragma once
#include <iostream>
#include <functional>
#include "helper.hpp"

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

		auto ret = ret_parse_object{inner,false,""};
		ret.dest = [](ARGS...args){
			((std::cout << std::forward<ARGS>(args)),...);
			std::cout << std::endl;
		};
		return ret;
	}
	ret_parse_object operator>>(std::function<void(ARGS...)> dest){
		auto ret = ret_parse_object{inner,false,""};
		ret.dest = dest;
		return ret;
		/* this->dest = dest;//bug this object is a factory */
		/* return *this; */
	}
	ret_parse_object(std::function<bool(context&,output_wraper<ARGS...>&&)> inner, bool dbg_inline, std::string comment) : 
		parse_object_ref(
			f_parser([&](context& ctx){
				if(!dest){
					dest = [](ARGS...){};
				}
				return this->inner(ctx,output_wraper<ARGS...>{dest});
			},dbg_inline,comment)),
		inner(inner) 
	{
	}
};

template<typename...ARGS>
auto r_parser(std::invocable<context&,output_wraper<ARGS...>&&> auto in_f, bool dbg_inline = false, const std::string& comment = ""){
	std::function<bool(context&,output_wraper<ARGS...>&&)> f = in_f;
	return ret_parse_object<ARGS...>{f,dbg_inline,comment};
}
auto r_parser(std::invocable<context&> auto in_f, bool dbg_inline = false, std::string comment = ""){
	std::function<bool(context&)> f = in_f;
	return r_parser<std::string_view>([=](context& ctx,output_wraper<std::string_view>&& output) -> bool{
		int start_pos = ctx.pos;
		auto success = f(ctx);
		if(success){
			output << std::string_view{ctx.input.begin()+start_pos,ctx.input.begin()+ctx.pos};
		}
		return success;
	},dbg_inline,comment);
}
