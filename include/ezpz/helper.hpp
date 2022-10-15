#pragma once
#include "ezpz/parse_object.hpp"
#include <fmt/core.h>
#include <iostream>
#include <stdexcept>

namespace ezpz{

struct print_p {
	using active = active_f;
	using UNPARSED_LIST = TLIST<>;

	std::string_view text;
	bool _parse(auto&){
		std::cout << text << std::endl;
		return true;
	}
};
inline parser auto print(std::string_view text){
	return print_p{text};
};

inline struct fail_p {
	using UNPARSED_LIST = TLIST<>;
	using active = active_f;
	using ezpz_prop = TLIST<always_false>;

	bool _parse(auto&){
		return false;
	}
} fail;
inline struct eoi_p {
	using active = active_f;
	using UNPARSED_LIST = TLIST<>;

	bool _parse(auto& ctx){
		return ctx.done();
	}
	std::string fail_msg(){
		return "expected end of input";
	}
} eoi;


template<parser P>
struct capture_p {
	using active = active_f;
	using UNPARSED_LIST = TLIST<std::string_view>;

	P parent;
	capture_p(P&& op) : parent(std::move(op)) {}
	capture_p(const P& op) : parent(op) {}
	bool _parse(auto& ctx, std::string_view& sv){
		auto start = ctx.getPosition();
		auto ret = parse(ctx,parent);
		auto length = static_cast<long unsigned int>(ctx.getPosition()-start);
		if constexpr(contains<typename get_prop_tag<std::decay_t<decltype(ctx)>>::type, is_recover_ctx>::value){
			sv = std::string_view{ctx.get_inner_ctx().input.data()+start,length};
		} else {
			sv = std::string_view{ctx.input.data()+start,length};
		}
		return ret;
	}
	bool dbg_inline(){
		return true;
	}
};
template<parser T>
rparser auto capture(T&& p){
	using P = std::decay_t<T>;
	return capture_p<P>(std::forward<P>(p));
}

struct parse_error : public std::runtime_error {
	using std::runtime_error::runtime_error;

	std::string expression_description = "";
	
	parse_error() noexcept : std::runtime_error("parse error") {}
	parse_error(std::string&& msg) noexcept : std::runtime_error(msg) {}
	parse_error(const parse_error&) noexcept = default;
	parse_error(parse_error&&) noexcept = default;
};

template<typename ctx_t, typename visitor_t>
struct recover_ctx {
	using ezpz_prop = TLIST<is_recover_ctx,has_inner_ctx>;
	ctx_t* ctx;
	visitor_t visitor;
	auto& get_inner_ctx(){
		return *ctx;
	}

	recover_ctx(ctx_t* ctx, auto&& visitor) 
		: ctx(ctx)
		, visitor(std::forward<visitor_t>(visitor))
	{}

	void advance(){
		ctx->advance();
	}
	bool done(){
		return ctx->done();
	}
	auto token(){
		return ctx->token();
	}
	auto getPosition(){
		return ctx->getPosition();
	}
	void setPosition(decltype(ctx->getPosition()) pos){
		ctx->setPosition(pos);
	}


	int level = 0;
	int last_level = 0;
	auto notify_enter(auto&) {
		++level;
		last_level = level;
		return ctx->getPosition();
	}
	void notify_leave(auto& parser, bool success, auto prev_pos){
		if(last_level == level && !success){
			visitor(*ctx,parser,prev_pos);
		}
		--level;
	}
};

namespace highlight_strategy {
	struct start_of_recover;
	struct at_failure_parser;
	struct at_last_token_position;
}
namespace continuation_strategy {
	struct start_of_recover;
	struct at_failure_parser;
	struct after_secondary_parser;
}
struct recover_default_config {
	using highlight_strategy = highlight_strategy::at_failure_parser;
	using continuation_strategy = continuation_strategy::after_secondary_parser;
	static int match(auto& ctx, auto&, auto prev_pos){
		return ctx.getPosition()-prev_pos;
	}
	static bool keep(int, int){
		return true;
	}
	static void perform_secondary_parse(auto& ctx){
		if constexpr (std::same_as<decltype(ctx.token()),char>){
			while(!ctx.done() && ctx.token() != ' ' && ctx.token() != '\t')
				ctx.advance();
		}
	}

};
template<parser P, typename recover_config>
struct recover_p {
	static_assert(! contains<typename get_prop_tag<P>::type, always_true>::value,
			"recovering from a parser that never fails does not make sense");
	using UNPARSED_LIST = typename P::UNPARSED_LIST;
	using active = typename P::active;
	using ezpz_prop = TLIST<always_true>;

	[[no_unique_address]] P p;

	recover_p(auto&& p) : p(std::forward<P>(p)) {}

	bool _parse(auto& ctx, auto&...args){
		constexpr bool nested_recover = contains<typename get_prop_tag<std::decay_t<decltype(ctx)>>::type, is_recover_ctx>::value;
		bool success;
		if constexpr(nested_recover){
			success = parse_or_undo(*ctx.ctx,p,args...);
		}else{
			success = parse_or_undo(ctx,p,args...);
		}
		if(!success){
			auto recover_start_pos = ctx.getPosition();
			/* ctx.error_missing(p); */

			int max_goodness = 0;
			struct candidate {
				std::string parser_description;
				int goodness;
				decltype(ctx.getPosition()) start_pos, end_pos;
			};
			std::vector<candidate> candidates;
			auto cb = [&](auto& ctx, auto& parser, auto prev_pos){
				int goodness = recover_config::match(ctx,parser,prev_pos);
				if(goodness >= max_goodness){
					max_goodness = goodness;
				}

				using hls = recover_config::highlight_strategy;
				if constexpr (std::same_as<hls,highlight_strategy::start_of_recover>){
					prev_pos = recover_start_pos;
				} else if constexpr( std::same_as<hls,highlight_strategy::at_last_token_position>){
					prev_pos = ctx.getPosition();
				}
				auto prev = ctx.getPosition();
				recover_config::perform_secondary_parse(ctx);

				candidates.emplace_back(std::string{description(parser)},goodness, prev_pos, ctx.getPosition());
				ctx.setPosition(prev);
			};
			if constexpr(nested_recover){
				recover_ctx<std::decay_t<decltype(*ctx.ctx)>, decltype(cb)> rctx(ctx.ctx,cb);
				parse_or_undo(rctx, p);
			}else{
				recover_ctx<std::decay_t<decltype(ctx)>, decltype(cb)> rctx(&ctx,cb);
				parse_or_undo(rctx, p);
			}

			for(size_t i = 0; i < candidates.size(); ++i){
				if(!recover_config::keep(max_goodness, candidates[i].goodness)){
					std::swap(candidates[i],candidates.back());
					candidates.pop_back();
				}
			}

			//TODO sort candidates
			if constexpr(!nested_recover){
				ctx.error(candidates);
			}

			using cos = recover_config::continuation_strategy;
			using namespace continuation_strategy;
			if constexpr(std::same_as<cos,start_of_recover>){
				ctx.setPosition(recover_start_pos);
			}else if constexpr(std::same_as<cos,at_failure_parser>){
				if(!candidates.empty()){
					ctx.setPosition(candidates[0].start_pos);
				}
			} else if constexpr(std::same_as<cos,after_secondary_parser>){
				if(!candidates.empty()){
					ctx.setPosition(candidates[0].end_pos);
				}
			}
		}
		return true;
	}
};
template<parser P1, parser P2>
struct recover_and_else_p {
	using UNPARSED_LIST = typename t_if_else<
		std::same_as<typename P1::UNPARSED_LIST, typename P2::UNPARSED_LIST>,
		typename P1::UNPARSED_LIST,
		TLIST<>
	>::type;
	using active = active_f;
	using ezpz_prop = TLIST<>;

	[[no_unique_address]] P1 p1;
	[[no_unique_address]] P2 p2;

	recover_and_else_p(auto&& p1, auto&& p2) : p1(std::forward<P1>(p1)), p2(std::forward<P2>(p2)) {}

	bool _parse(auto& ctx,auto&...args) {
		auto start = ctx.getPosition();
		try {
			if(parse(ctx,p1,args...)){
				return true;
			}
		}catch(parse_error& pe){
			//notify context 
		}
		ctx.setPosition(start);
		return parse(ctx,p2,args...);
	}
};
template<typename recover_config>
struct recover_t {
	auto operator()(auto&& p1){
		using P1_t = std::decay_t<decltype(p1)>;
		return recover_p<P1_t, recover_config>(std::forward<P1_t>(p1));
	}
};
inline recover_t<recover_default_config> recover;

/* parser auto recover(auto&& p1){ */
/* 	using P1_t = std::decay_t<decltype(p1)>; */
/* 	return recover_p<P1_t>(std::forward<P1_t>(p1)); */
/* } */


template<parser P, typename err_msg>
struct must_p {
	using UNPARSED_LIST = typename P::UNPARSED_LIST;
	using active = typename P::active;
	using ezpz_prop = typename get_prop_tag<P>::type;

	[[no_unique_address]] P p;
	[[no_unique_address]] err_msg err;
	
	must_p(auto&& p, err_msg&& err) : p(std::forward<P>(p)), err(std::forward<err_msg>(err)) {}

	bool _parse(auto& ctx, auto&&...args){
		if(!parse(ctx,p,args...)){
			err(ctx);
			std::string what = "error:";
			if constexpr( error_context_c<std::decay_t<decltype(ctx)>> ){
				what += " " + ctx.describePosition(ctx.getPosition());
			}
			if constexpr( requires(decltype(p) p){{p.fail_msg()} -> std::same_as<std::string>;}){
				what += " " + p.fail_msg();
			}
			auto err = parse_error(what);
			err.expression_description = description(p);
			throw err;
		}
		return true;
	}
};

parser auto must(auto&& p1){
	using P1_t = std::decay_t<decltype(p1)>;
	auto empty_msg = [](auto&){};
	return must_p<P1_t, decltype(empty_msg)>(std::forward<P1_t>(p1),{});
}
parser auto must(auto&& p1, auto&& err_msg){
	using P1_t = std::decay_t<decltype(p1)>;
	using msg_t = std::decay_t<decltype(err_msg)>;
	return must_p<P1_t, msg_t>(std::forward<P1_t>(p1),std::forward<msg_t>(err_msg));
}

template<typename...TS>
struct retd_p {
	using UNPARSED_LIST = TLIST<TS...>;
	using active = active_f;
	using ezpz_prop = TLIST<always_true>;

	std::tuple<TS...> hold;
	retd_p(auto&&... args) : hold{std::forward<std::decay_t<decltype(args)>>(args)...}
	{}
	auto operator()() requires (sizeof...(TS) == 1) {
		return std::get<0>(hold);
	}
	bool _parse(auto&, auto&...ret){
		std::tie(ret...) = hold;
		return true;
	}
};
template<auto...vals>
struct ret_p {
	using UNPARSED_LIST = TLIST<decltype(vals)...>;
	using ezpz_prop = TLIST<always_true>;
	using active = active_f;
	bool _parse(auto&, auto&...ret){
		((ret = vals),...);
		return true;
	}
	auto operator()() const requires (sizeof...(vals) == 1) {
		return (vals,...);
	}
};
template<auto...vals>
inline auto ret = ret_p<vals...>{};

auto retd(auto&&...vals){
	return retd_p<std::decay_t<decltype(vals)>...>(std::forward<decltype(vals)>(vals)...);
}

}
