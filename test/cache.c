#include "../include/siphon/cache.h"
#include "../include/siphon/alloc.h"
#include "mu.h"

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
test_private (void)
{
	SpCacheControl cc;
	ssize_t n;

	const char buf1[] = "private";
	const char buf2[] = "private=\"string\"";
	const char buf3[] = "private=token"; // non-standard
	char val[16];

	n = sp_cache_control_parse (&cc, buf1, (sizeof buf1) - 1);
	mu_assert_int_eq (n, (sizeof buf1) - 1);
	if (n < 0) {
		printf ("ERROR: %s\n", buf1 + (-1 - n));
		return;
	}
	mu_assert_int_eq (cc.type & SP_CACHE_PRIVATE, SP_CACHE_PRIVATE);
	mu_assert_uint_eq (cc.private.len, 0);

	n = sp_cache_control_parse (&cc, buf2, (sizeof buf2) - 1);
	mu_assert_int_eq (n, (sizeof buf2) - 1);
	if (n < 0) {
		printf ("ERROR: %s\n", buf2 + (-1 - n));
		return;
	}
	mu_assert_int_eq (cc.type & SP_CACHE_PRIVATE, SP_CACHE_PRIVATE);
	mu_assert_uint_eq (cc.private.len, 6);
	memcpy (val, buf2+cc.private.off, cc.private.len);
	val[cc.private.len] = '\0';
	mu_assert_str_eq (val, "string");

	n = sp_cache_control_parse (&cc, buf3, (sizeof buf3) - 1);
	mu_assert_int_eq (n, (sizeof buf3) - 1);
	if (n < 0) {
		printf ("ERROR: %s\n", buf3 + (-1 - n));
		return;
	}
	mu_assert_int_eq (cc.type & SP_CACHE_PRIVATE, SP_CACHE_PRIVATE);
	mu_assert_uint_eq (cc.private.len, 5);
	memcpy (val, buf3+cc.private.off, cc.private.len);
	val[cc.private.len] = '\0';
	mu_assert_str_eq (val, "token");
}

static void
test_no_cache (void)
{
	SpCacheControl cc;
	ssize_t n;

	const char buf1[] = "no-cache";
	const char buf2[] = "no-cache=\"string\"";
	const char buf3[] = "no-cache=token"; // non-standard
	char val[16];

	n = sp_cache_control_parse (&cc, buf1, (sizeof buf1) - 1);
	mu_assert_int_eq (n, (sizeof buf1) - 1);
	if (n < 0) {
		printf ("ERROR: %s\n", buf1 + (-1 - n));
		return;
	}
	mu_assert_int_eq (cc.type & SP_CACHE_NO_CACHE, SP_CACHE_NO_CACHE);
	mu_assert_uint_eq (cc.no_cache.len, 0);

	n = sp_cache_control_parse (&cc, buf2, (sizeof buf2) - 1);
	mu_assert_int_eq (n, (sizeof buf2) - 1);
	if (n < 0) {
		printf ("ERROR: %s\n", buf2 + (-1 - n));
		return;
	}
	mu_assert_int_eq (cc.type & SP_CACHE_NO_CACHE, SP_CACHE_NO_CACHE);
	mu_assert_uint_eq (cc.no_cache.len, 6);
	memcpy (val, buf2+cc.no_cache.off, cc.no_cache.len);
	val[cc.no_cache.len] = '\0';
	mu_assert_str_eq (val, "string");

	n = sp_cache_control_parse (&cc, buf3, (sizeof buf3) - 1);
	mu_assert_int_eq (n, (sizeof buf3) - 1);
	if (n < 0) {
		printf ("ERROR: %s\n", buf3 + (-1 - n));
		return;
	}
	mu_assert_int_eq (cc.type & SP_CACHE_NO_CACHE, SP_CACHE_NO_CACHE);
	mu_assert_uint_eq (cc.no_cache.len, 5);
	memcpy (val, buf3+cc.no_cache.off, cc.no_cache.len);
	val[cc.no_cache.len] = '\0';
	mu_assert_str_eq (val, "token");
}

static void
test_group (void)
{
	SpCacheControl cc;
	ssize_t n;

	const char buf[] = "max-age=0, no-cache, no-store";
	n = sp_cache_control_parse (&cc, buf, (sizeof buf) - 1);
	mu_assert_int_eq (n, (sizeof buf) - 1);
	if (n < 0) {
		printf ("ERROR: %s\n", buf + (-1 - n));
		return;
	}

	mu_assert_int_eq (cc.type & SP_CACHE_MAX_AGE, SP_CACHE_MAX_AGE);
	mu_assert_int_eq (cc.max_age, 0);

	mu_assert_int_eq (cc.type & SP_CACHE_NO_CACHE, SP_CACHE_NO_CACHE);

	mu_assert_int_eq (cc.type & SP_CACHE_NO_STORE, SP_CACHE_NO_STORE);

	mu_assert_int_eq (cc.type, SP_CACHE_MAX_AGE | SP_CACHE_NO_CACHE | SP_CACHE_NO_STORE);
}

static void
test_group_semi (void)
{
	SpCacheControl cc;
	ssize_t n;

	const char buf[] = "max-age=0; no-cache; no-store";
	n = sp_cache_control_parse (&cc, buf, (sizeof buf) - 1);
	mu_assert_int_eq (n, (sizeof buf) - 1);
	if (n < 0) {
		printf ("ERROR: %s\n", buf + (-1 - n));
		return;
	}

	mu_assert_int_eq (cc.type & SP_CACHE_MAX_AGE, SP_CACHE_MAX_AGE);
	mu_assert_int_eq (cc.max_age, 0);

	mu_assert_int_eq (cc.type & SP_CACHE_NO_CACHE, SP_CACHE_NO_CACHE);

	mu_assert_int_eq (cc.type & SP_CACHE_NO_STORE, SP_CACHE_NO_STORE);

	mu_assert_int_eq (cc.type, SP_CACHE_MAX_AGE | SP_CACHE_NO_CACHE | SP_CACHE_NO_STORE);
}

static void
test_all (void)
{
	const char buf[] =
		"public,"
		"private,"
		"private=\"stuff\","
		"no-cache,"
		"no-cache=\"thing\","
		"no-store,"
		"max-age=12,"
		"s-maxage=34,"
		"max-stale,"
		"max-stale=56,"
		"min-fresh=78,"
		"no-transform,"
		"only-if-cached,"
		"must-revalidate,"
		"proxy-revalidate,"
		"ext1,"
		"ext2=something,"
		"ext3=\"other\""
		;

	SpCacheControl cc;
	ssize_t n = sp_cache_control_parse (&cc, buf, (sizeof buf) - 1);
	mu_assert_int_eq (n, (sizeof buf) - 1);
	if (n < 0) {
		printf ("ERROR: %s\n", buf + (-1 - n));
		return;
	}

	mu_assert_int_eq (cc.type & SP_CACHE_PUBLIC, SP_CACHE_PUBLIC);
	mu_assert_int_eq (cc.type & SP_CACHE_PRIVATE, SP_CACHE_PRIVATE);
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

	char val[64];

	memcpy (val, buf+cc.private.off, cc.private.len);
	val[cc.private.len] = '\0';
	mu_assert_str_eq (val, "stuff");

	memcpy (val, buf+cc.no_cache.off, cc.no_cache.len);
	val[cc.no_cache.len] = '\0';
	mu_assert_str_eq (val, "thing");
}

static void
test_all_semi (void)
{
	const char buf[] =
		"public;"
		"private;"
		"private=\"stuff\";"
		"no-cache;"
		"no-cache=\"thing\","
		"no-store;"
		"max-age=12;"
		"s-maxage=34;"
		"max-stale;"
		"max-stale=56;"
		"min-fresh=78;"
		"no-transform;"
		"only-if-cached;"
		"must-revalidate;"
		"proxy-revalidate;"
		"ext1;"
		"ext2=something;"
		"ext3=\"other\""
		;

	SpCacheControl cc;
	ssize_t n = sp_cache_control_parse (&cc, buf, (sizeof buf) - 1);
	mu_assert_int_eq (n, (sizeof buf) - 1);
	if (n < 0) {
		printf ("ERROR: %s\n", buf + (-1 - n));
		return;
	}

	mu_assert_int_eq (cc.type & SP_CACHE_PUBLIC, SP_CACHE_PUBLIC);
	mu_assert_int_eq (cc.type & SP_CACHE_PRIVATE, SP_CACHE_PRIVATE);
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

	char val[64];

	memcpy (val, buf+cc.private.off, cc.private.len);
	val[cc.private.len] = '\0';
	mu_assert_str_eq (val, "stuff");

	memcpy (val, buf+cc.no_cache.off, cc.no_cache.len);
	val[cc.no_cache.len] = '\0';
	mu_assert_str_eq (val, "thing");
}

int
main (void)
{
	mu_init ("cache");

	test_max_stale ();
	test_private ();
	test_no_cache ();
	test_group ();
	test_group_semi ();
	test_all ();
	test_all_semi ();

	mu_assert (sp_alloc_summary ());
}

