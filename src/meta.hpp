#pragma once

struct active_t;
struct active_f;

struct EOL {};
template<typename FIRST=EOL, typename ... REST>
struct TLIST : TLIST<REST...>{
	using type = FIRST;
	using rest = TLIST<REST...>;
};
template<>
struct TLIST<EOL> : EOL {
	using type = EOL;
	using rest = EOL;
};
