#ifndef SIPHON_CACHE_H
#define SIPHON_CACHE_H

#include "common.h"

typedef enum {
	SP_CACHE_PUBLIC            = 1 << 0,
	SP_CACHE_PRIVATE           = 1 << 1,
	SP_CACHE_NO_CACHE          = 1 << 2,
	SP_CACHE_NO_STORE          = 1 << 3,
	SP_CACHE_MAX_AGE           = 1 << 4,
	SP_CACHE_S_MAXAGE          = 1 << 5,
	SP_CACHE_MAX_STALE         = 1 << 6,
	SP_CACHE_MAX_STALE_TIME    = 1 << 7,
	SP_CACHE_MIN_FRESH         = 1 << 8,
	SP_CACHE_NO_TRANSFORM      = 1 << 9,
	SP_CACHE_ONLY_IF_CACHED    = 1 << 10,
	SP_CACHE_MUST_REVALIDATE   = 1 << 11,
	SP_CACHE_PROXY_REVALIDATE  = 1 << 12
} SpCacheType;

typedef struct {
	time_t max_age, s_maxage, max_stale, min_fresh;
	SpCacheType type;
	SpRange16 private, no_cache;
} SpCacheControl;

#define SP_CACHE_CONTROL_HAS(s, k) ((s) && ((s)->type & (k)))

SP_EXPORT ssize_t
sp_cache_control_parse (SpCacheControl *cc, const char *buf, size_t len);

#endif

