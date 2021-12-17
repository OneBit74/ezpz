#pragma once
#include "ezpz/core.hpp"
#include "ezpz/parse_object.hpp"
#include "ezpz/helper.hpp"

template<parser T>
parser auto plus(T&& rhs) {
	using TT = std::decay_t<T>;
	return make_rpo([r=std::forward<TT>(rhs)](auto& ctx) mutable {
		if(!parse(ctx,r))return false;
		while(parse_or_undo(ctx,r) && !ctx.done()){}
		return true;
	});
}
template<parser T>
parser auto any(T&& rhs) {
	using TT = std::decay_t<T>;
	return make_rpo([r=std::forward<TT>(rhs)](auto& ctx) mutable {
		while(parse_or_undo(ctx,r) && !ctx.done()){}
		return true;
	});
}
template<typename T>
parser auto notf(T&& rhs) requires parser<std::decay_t<T>>{
	using TT = std::decay_t<T>;
	return make_rpo([r=std::forward<TT>(rhs)](auto& ctx) mutable {
		return !parse_or_undo(ctx,r);
	});
}
template<parser T>
parser auto optional(T&& rhs) {
	using TT = std::decay_t<T>;
	auto ret = std::forward<TT>(rhs) | notf(fail);
	return ret;
}

auto times(int amount,auto&& parser){
	return make_rpo([=](auto& ctx) mutable {
			for(int i = 0; i < amount; ++i){
				if(!parse(ctx,parser))return false;
			}
			return true;
		}
	);
}
auto max(int amount,auto&& parser){
	return make_rpo([=](auto& ctx) mutable {
		int counter = 0;
		while(true){
			auto start = ctx.getPosition();
			if(!parse(ctx,parser)){
				return true;
			}else if (counter >= amount) {
				ctx.setPosition(start);
				return false;
			}
			counter++;
		}
	});
}
auto min(int amount,auto&& parser){
	return make_rpo([=](auto& ctx) mutable {
		int counter = 0;
		while(true){
			auto start = ctx.getPosition();
			if(!parse(ctx,parser)){
				if(counter < amount){
					ctx.setPosition(start);
				}
				return counter >= amount;
			}
			counter++;
		}
	});
}


