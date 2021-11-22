#include "quantifiers.hpp"
#include "matcher.hpp"

/* parse_object_ref operator>>(optional_t,parse_object_ref rhs){ */
/* 	return f_parser([=](context& ctx) mutable { */
/* 			rhs.match(ctx); */
/* 			return true; */
/* 		}); */
/* } */
/* parse_object_ref operator>>(optional_t,std::string_view text){ */
/* 	return optional >> text_parser(text); */
/* } */
parse_object_ref operator>>(plus_t, parse_object_ref rhs){
	return rhs + (any >> rhs);
}
parse_object_ref operator>>(plus_t,std::string_view text){
	return plus >> text_parser(text);
}
parse_object_ref max_t::operator>>(std::string_view sv){
	return *this >> text_parser(sv);
}
parse_object_ref max_t::operator>>(parse_object_ref inner){
	return f_parser([=,this](auto& ctx) mutable {
			for(int i = 0; i < val; ++i){
				if(!inner.match_or_undo(ctx))return true;
			}
			return true;
		});
}
max_t max(int val){
	return max_t{val};
}
parse_object_ref min_t::operator>>(std::string_view sv){
	return *this >> text_parser(sv);
}
parse_object_ref min_t::operator>>(parse_object_ref inner){
	return f_parser([=,this](auto& ctx) mutable {
			for(int i = 0; i < val; ++i){
				if(!inner.match(ctx))return false;
			}
			while(inner.match_or_undo((ctx))){};
			return true;
		});
}
min_t min(int val){
	return min_t{val};
}
parse_object_ref minmax_t::operator>>(std::string_view sv){
	return *this >> text_parser(sv);
}
parse_object_ref minmax_t::operator>>(parse_object_ref inner){
	return f_parser([=,this](auto& ctx) mutable {
			for(int i = 0; i < val1; ++i){
				if(!inner.match(ctx))return false;
			}
			for(int i = 0; i < (val2-val1); ++i){
				if(!inner.match_or_undo(ctx))return true;
			}
			return true;
		});
}
minmax_t minmax(int val1, int val2){
	return minmax_t{val1,val2};
}
minmax_t exact(int val){
	return minmax(val,val);
}
