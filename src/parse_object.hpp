#pragma once
#include "meta.hpp"
#include "context.hpp"
#include <string>
#include <string_view>
#include <memory>

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
class parse_object_ref : public parse_object {
public:
	using active = active_f;
	using UNPARSED_LIST = TLIST<EOL>;

	parse_object_ref(std::shared_ptr<parse_object> ptr);

	bool _match(context&) override;
	void _undo(context&) override;
	bool match_or_undo(context& ctx);
	bool operator()(std::string_view sv);
	bool operator()(context& ctx);
	bool match(context& ctx);

	std::shared_ptr<parse_object> impl;
};
class f_parser_t : public parse_object {
public:
	std::function<bool(context&)> f;
	f_parser_t(decltype(f) f);
	bool _match(context& ctx) override;
};
auto f_parser(std::function<bool(context& ctx)> f, bool dbg_inline = false, std::string comment = "") -> parse_object_ref;
auto operator+(parse_object_ref lhs, parse_object_ref rhs) -> parse_object_ref;
auto operator+(std::string_view sv, parse_object_ref rhs) -> parse_object_ref;
auto operator+(parse_object_ref lhs, std::string_view sv) -> parse_object_ref;
auto operator|(parse_object_ref lhs, parse_object_ref rhs) -> parse_object_ref;
auto operator|(parse_object_ref lhs, std::string_view rhs) -> parse_object_ref;
auto operator|(std::string_view lhs, parse_object_ref rhs) -> parse_object_ref;
