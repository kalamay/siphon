#include "siphon/cache.h"

#include <string.h>
#include <assert.h>

%%{
	machine cache_control_parser;

	action public           { SET(PUBLIC); }
	action private          { SETSTR(PRIVATE, private); }
	action no_cache         { SETSTR(NO_CACHE, no_cache); }
	action no_store         { SET(NO_STORE); }
	action max_age          { SETNUM(MAX_AGE, max_age); }
	action s_maxage         { SETNUM(S_MAXAGE, s_maxage); }
	action min_fresh        { SETNUM(MIN_FRESH, min_fresh); }
	action no_transform     { SET(NO_TRANSFORM); }
	action only_if_cached   { SET(ONLY_IF_CACHED); }
	action must_revalidate  { SET(MUST_REVALIDATE); }
	action proxy_revalidate { SET(PROXY_REVALIDATE); }

	action max_stale {
		SET(MAX_STALE);
		if (val) {
			SETNUM(MAX_STALE_TIME, max_stale);
		}
	}

	action str_init { str.off = p - buf; }
	action str_end  { str.len = (p - buf) - str.off; }
	action sec_neg  { neg = -1; }
	action sec_incr { num = num*10 + (fc-'0'); }

	token = [!#-'*-+\-.0-:@A-Z^-`a-z|~]+ >str_init %str_end ;
	string = '"' ( [^"]* >str_init %str_end ) '"' ;
	value = '=' ( token | string ) ;
	integer = '0' | [1-9] digit* ;
	seconds = '=' ( ( "-" @sec_neg )? integer >{ val = true; } @sec_incr ) ;
	extension = ( token ) ( value )? ;

	dir = "public"i %public
		| "private"i value? %private
		| "no-cache"i value? %no_cache
		| "no-store"i %no_store
		| "max-age"i seconds %max_age
		| "s-maxage"i seconds %s_maxage
		| "max-stale"i ( seconds )? %max_stale
		| "min-fresh"i seconds %min_fresh
		| "no-transform"i %no_transform
		| "only-if-cached"i %only_if_cached
		| "must-revalidate"i %must_revalidate
		| "proxy-revalidate"i %proxy_revalidate
		| extension
		;

	main := space* dir ( space* "," space* dir )* ;
}%%

ssize_t
sp_cache_control_parse (SpCacheControl *cc, const char *buf, size_t len)
{
	assert (cc != NULL);
	assert (buf != NULL);

	memset (cc, 0, sizeof *cc);

	const char *restrict p = buf;
	const char *restrict pe = p + len;
	const char *restrict eof = pe;
	int64_t neg = 1;
	uint64_t num = 0;
	SpRange16 str = { 0, 0 };
	bool val = false;
	int cs = %%{ write start; }%%;

#define SET(n) do { cc->type |= SP_CACHE_##n; } while (0)
#define SETNUM(n, key) do {       \
	SET(n);                       \
	cc->key = neg * (int64_t)num; \
	num = 0;                      \
	neg = 1;                      \
	val = false;                  \
} while (0)
#define SETSTR(n, key) do {       \
	SET(n);                       \
	cc->key = str;                \
	str.off = 0;                  \
	str.len = 0;                  \
	val = false;                  \
} while (0)
	%% write exec;

	if (cs < %%{ write first_final; }%%) {
		return -1 - (p - buf);
	}
	else {
		return p - buf;
	}
}

