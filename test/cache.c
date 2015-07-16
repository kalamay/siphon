#include <stdio.h>
#include "siphon/cache.h"

#include "mu/mu.h"

static void
test_max_stale (void)
{
	SpCacheControl cc;
	ssize_t n;

	const char buf1[] = "max-stale";
	const char buf2[] = "max-stale=123";

	n = sp_cache_control_parse (&cc, buf1, (sizeof buf1) - 1);
	mu_assert_int_eq (n, (sizeof buf1) - 1);
	if (n < 0) {
		printf ("ERROR: %s\n", buf1 + (-1 - n));
		return;
	}
	mu_assert_int_eq (cc.type & SP_CACHE_MAX_STALE, SP_CACHE_MAX_STALE);
	mu_assert_int_ne (cc.type & SP_CACHE_MAX_STALE_TIME, SP_CACHE_MAX_STALE_TIME);

	n = sp_cache_control_parse (&cc, buf2, (sizeof buf2) - 1);
	mu_assert_int_eq (n, (sizeof buf2) - 1);
	if (n < 0) {
		printf ("ERROR: %s\n", buf2 + (-1 - n));
		return;
	}
	mu_assert_int_eq (cc.type & SP_CACHE_MAX_STALE, SP_CACHE_MAX_STALE);
	mu_assert_int_eq (cc.type & SP_CACHE_MAX_STALE_TIME, SP_CACHE_MAX_STALE_TIME);
	mu_assert_int_eq (cc.max_stale, 123);
}

static void
test_all (void)
{
	const char buf[] =
		"no-cache;"
		"no-store;"
		"max-age=12;"
		"s-maxage=34;"
		"max-stale;"
		"max-stale=56;"
		"min-fresh=78;"
		"no-transform;"
		"only-if-cached;"
		"must-revalidate;"
		"proxy-revalidate"
		;

	SpCacheControl cc;
	ssize_t n = sp_cache_control_parse (&cc, buf, (sizeof buf) - 1);
	mu_assert_int_eq (n, (sizeof buf) - 1);
	if (n < 0) {
		printf ("ERROR: %s\n", buf + (-1 - n));
		return;
	}

	mu_assert_int_eq (cc.type & SP_CACHE_NO_CACHE, SP_CACHE_NO_CACHE);
	mu_assert_int_eq (cc.type & SP_CACHE_NO_STORE, SP_CACHE_NO_STORE);
	mu_assert_int_eq (cc.type & SP_CACHE_MAX_AGE, SP_CACHE_MAX_AGE);
	mu_assert_int_eq (cc.type & SP_CACHE_S_MAXAGE, SP_CACHE_S_MAXAGE);
	mu_assert_int_eq (cc.type & SP_CACHE_MAX_STALE, SP_CACHE_MAX_STALE);
	mu_assert_int_eq (cc.type & SP_CACHE_MAX_STALE_TIME, SP_CACHE_MAX_STALE_TIME);
	mu_assert_int_eq (cc.type & SP_CACHE_MIN_FRESH, SP_CACHE_MIN_FRESH);
	mu_assert_int_eq (cc.type & SP_CACHE_NO_TRANSFORM, SP_CACHE_NO_TRANSFORM);
	mu_assert_int_eq (cc.type & SP_CACHE_ONLY_IF_CACHED, SP_CACHE_ONLY_IF_CACHED);
	mu_assert_int_eq (cc.type & SP_CACHE_MUST_REVALIDATE, SP_CACHE_MUST_REVALIDATE);
	mu_assert_int_eq (cc.type & SP_CACHE_PROXY_REVALIDATE, SP_CACHE_PROXY_REVALIDATE);

	mu_assert_int_eq (cc.max_age, 12);
	mu_assert_int_eq (cc.s_maxage, 34);
	mu_assert_int_eq (cc.max_stale, 56);
	mu_assert_int_eq (cc.min_fresh, 78);
}

int
main (void)
{
	test_max_stale ();
	test_all ();

	mu_exit ("cache");
}

