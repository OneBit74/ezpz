#pragma once
#include "meta.hpp"
#include "context.hpp"
#include <concepts>
#include <string>
#include <string_view>
#include <memory>

template<typename T>
concept parser = requires(T t, context& ctx, std::string_view sv){
	{ 
		t.undo(ctx)
	} -> std::same_as<void>;
	{ 
		t.match(ctx)
	} -> std::same_as<bool>;
	{ 
		t.match_or_undo(ctx)
	} -> std::same_as<bool>;
	{ 
		t(ctx)
	} -> std::same_as<bool>;
	{ 
		t(sv)
	} -> std::same_as<bool>;
};
template<typename T>
concept rparser = parser<T> && requires(T t,context& ctx){
	typename T::UNPARSED_LIST;
	typename T::active;
};

class parse_object {
public:
	using active = active_f;
	using UNPARSED_LIST = TLIST<EOL>;

	std::string comment;
	bool dbg_inline = false;

	virtual ~parse_object() = default;
	virtual bool _match(context&);
	virtual void _undo(context&);

	bool operator()(std::string_view sv);
	bool operator()(context& ctx);
	bool match_or_undo(context& ctx);
	bool match(context& ctx);
	void undo(context& ctx);

	size_t dbg_log_enter(context&);
	void dbg_log_leave(context&);
	void dbg_log_comment(context&,size_t);
};
template<typename parser>
class parse_object_ref : public parse_object {
public:
	using active = typename parser::active;
	using UNPARSED_LIST = typename parser::UNPARSED_LIST;

	parser& p;
	parse_object_ref(parser& op) : p(op) {}

	bool _match(context& ctx) override {
		return p._match(ctx);
	}
	void _undo(context& ctx) override{
		p._undo(ctx);
	}
	bool parse(context& ctx, auto&...args) requires rparser<parser>{
		return p.parse(ctx,args...);
	}
};
template<typename F_TYPE> requires std::invocable<F_TYPE,context&>
class f_parser_t : public parse_object {
public:
	F_TYPE f;

	f_parser_t(F_TYPE& of) : f(of) {}
	f_parser_t(F_TYPE&& of) : f(std::move(of)) {}
	bool _match(context& ctx) override {
		return f(ctx);
	}
};
template<typename F_TYPE> requires std::invocable<F_TYPE,context&>
parser auto f_parser(F_TYPE&& f, bool dbg_inline=false, std::string comment=""){
	auto ret = f_parser_t<F_TYPE>{std::forward<F_TYPE>(f)};
	ret.dbg_inline = dbg_inline;
	ret.comment = comment;
	return ret;
}
template<parser T1, parser T2>
struct simple_join_parser : public parse_object {
	T1 t1;
	T2 t2;
	simple_join_parser(auto&& p1, auto&& p2) : t1(std::forward<T1>(p1)), t2(std::forward<T2>(p2)) {}

	bool _match(context& ctx) override{
		return t1.match(ctx) && t2.match(ctx);
	};
};
template<parser T1, parser T2>
parser auto operator+(T1&& lhs, T2&& rhs){
	using p_type = simple_join_parser<T1,T2>;
	return p_type{std::forward<T1>(lhs), std::forward<T2>(rhs)};
	/* return f_parser( */
	/* 	[l=std::forward<T1>(lhs), r=std::forward<T2>(rhs)] */
	/* 	(context& ctx) mutable */
	/* 	{ */
	/* 		return l.match(ctx) && r.match(ctx); */
	/* 	},true */
	/* ); */
}
template<parser T1, parser T2>
parser auto operator|(T1&& lhs, T2&& rhs){
	return f_parser(
		[l=std::forward<T1>(lhs), r=std::forward<T2>(rhs)]
		(context& ctx) mutable
		{
			return l.match(ctx) || r.match(ctx);
		}
	);
}
