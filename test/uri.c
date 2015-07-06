#include "siphon/siphon.h"
#include "mu/mu.h"

#define assert_uri(u, s) do {                                                \
	ssize_t rc = sp_uri_parse (u, s, strlen (s));                            \
	mu_assert_msg (rc >= 0, "Failed to parse URI \"%s\"", s);                \
	if (rc < 0) mu_exit ("uri");                                             \
} while (0)

#define assert_uri_segment(uri, buf, typ, str) do {                          \
	SpUriSegment uri_seg = SP_URI_##typ;                                     \
	SpRange16 rng = (uri)->seg[uri_seg];                                     \
	if (str == NULL) {                                                       \
		mu_assert_msg(!sp_uri_has_segment (uri, uri_seg),                    \
				"Assertion '" #typ "==(null)' failed: " #typ "==\"%.*s\"",   \
				(int)rng.len, (const char *)buf+rng.off);                    \
	}                                                                        \
	else if (!sp_uri_has_segment (uri, uri_seg)) {                           \
		mu_fail ("Assertion '" #typ "==\"%s\"' failed: " #typ "==(null)",    \
				(str));                                                      \
	}                                                                        \
	else {                                                                   \
		mu_assert_msg(SP_RANGE_EQ_STR (rng, buf, str),                       \
				"Assertion '" #typ "==\"%s\"' failed: " #typ "==\"%.*s\"",   \
				(str), (int)rng.len, (const char *)buf+rng.off);             \
	}                                                                        \
} while (0)

#define TEST_PARSE(u, sc, us, pw, hn, pt, pa, q, f) do { \
	SpUri parsed;                                        \
	assert_uri (&parsed, u);                             \
	assert_uri_segment (&parsed, u, SCHEME, sc);         \
	assert_uri_segment (&parsed, u, USER, us);           \
	assert_uri_segment (&parsed, u, PASSWORD, pw);       \
	assert_uri_segment (&parsed, u, HOST, hn);           \
	assert_uri_segment (&parsed, u, PORT, pt);           \
	assert_uri_segment (&parsed, u, PATH, pa);           \
	assert_uri_segment (&parsed, u, QUERY, q);           \
	assert_uri_segment (&parsed, u, FRAGMENT, f);        \
} while (0)

#define TEST_SUB(u, s1, s2, cmp) do {                    \
	SpUri parsed;                                        \
	assert_uri (&parsed, u);                             \
	SpRange16 rng;                                       \
	int rc = sp_uri_sub (&parsed, s1, s2, &rng);         \
	mu_assert_int_eq (rc, 0);                            \
	if (rc == 0) {                                       \
		char buf[4096];                                  \
		strncpy (buf, (const char *)u+rng.off, rng.len); \
		buf[rng.len] = '\0';                             \
		mu_assert_str_eq (buf, cmp);                     \
	}                                                    \
} while (0)

#define TEST_RANGE(u, s1, s2, cmp) do {                  \
	SpUri parsed;                                        \
	assert_uri (&parsed, u);                             \
	SpRange16 rng;                                       \
	int rc = sp_uri_range (&parsed, s1, s2, &rng);       \
	mu_assert_int_eq (rc, 0);                            \
	if (rc == 0) {                                       \
		char buf[4096];                                  \
		strncpy (buf, (const char *)u+rng.off, rng.len); \
		buf[rng.len] = '\0';                             \
		mu_assert_str_eq (buf, cmp);                     \
	}                                                    \
} while (0)

#define TEST_JOIN(a, b, exp) do {                            \
	char buf[4096];                                          \
	SpUri ua, ub, uexp, join;                                \
	assert_uri (&ua, a);                                     \
	assert_uri (&ub, b);                                     \
	assert_uri (&uexp, exp);                                 \
	sp_uri_join (&ua, a, &ub, b, &join, buf, sizeof buf -1); \
	mu_assert_str_eq (buf, exp);                             \
	mu_assert (sp_uri_eq (&join, buf, &uexp, exp));          \
} while (0)

#define TEST_JOIN_SEGMENTS(a, b, sc, us, pw, hn, pt, pa, q, f) do { \
	char buf[4096];                                                 \
	SpUri ua, ub, join;                                             \
	assert_uri (&ua, a);                                            \
	assert_uri (&ub, b);                                            \
	sp_uri_join (&ua, a, &ub, b, &join, buf, sizeof buf -1);        \
	assert_uri_segment (&join, buf, SCHEME, sc);                    \
	assert_uri_segment (&join, buf, USER, us);                      \
	assert_uri_segment (&join, buf, PASSWORD, pw);                  \
	assert_uri_segment (&join, buf, HOST, hn);                      \
	assert_uri_segment (&join, buf, PORT, pt);                      \
	assert_uri_segment (&join, buf, PATH, pa);                      \
	assert_uri_segment (&join, buf, QUERY, q);                      \
	assert_uri_segment (&join, buf, FRAGMENT, f);                   \
} while (0)

#define TEST_IPv6_VALID(str) do {                             \
	SpUri uri;                                                \
	const char full[] = "http://[" str "]:1234/path";         \
	assert_uri (&uri, full);                                  \
	mu_assert_int_eq (uri.host, SP_HOST_IPV6);                \
	assert_uri_segment (&uri, full, HOST, str);               \
	assert_uri_segment (&uri, full, PATH, "/path");           \
	assert_uri_segment (&uri, full, PORT, "1234");            \
} while (0)

#define TEST_IPv6_INVALID(str) do {                           \
	SpUri uri;                                                \
	const char full[] = "http://[" str "]:1234/path";         \
	ssize_t rc = sp_uri_parse (&uri, full, sizeof full - 1);  \
	mu_assert_int_le (rc, -1);                                \
} while (0)

#define TEST_IPv4_MAP_VALID(str) do {                         \
	SpUri uri;                                                \
	const char full[] = "http://[" str "]:1234/path";         \
	assert_uri (&uri, full);                                  \
	mu_assert_int_eq (uri.host, SP_HOST_IPV4_MAPPED);         \
	assert_uri_segment (&uri, full, HOST, str);               \
	assert_uri_segment (&uri, full, PATH, "/path");           \
	assert_uri_segment (&uri, full, PORT, "1234");            \
} while (0)

#define TEST_IPv4_MAP_INVALID(str) do {                       \
	SpUri uri;                                                \
	const char full[] = "http://[" str "]:1234/path";         \
	ssize_t rc = sp_uri_parse (&uri, full, sizeof full - 1);  \
	mu_assert_int_le (rc, -1);                                \
} while (0)


static void
test_parse (void)
{
	TEST_PARSE ("http://user:pass@www.test.com:80/test/file.txt?a=b&c=def&x%26y=%C2%AE#frag",
		"http", "user", "pass", "www.test.com", "80", "/test/file.txt", "a=b&c=def&x%26y=%C2%AE", "frag");
	TEST_PARSE ("//user:pass@www.test.com:80/test/file.txt?a=b&c=def&x%26y=%C2%AE#frag",
		NULL, "user", "pass", "www.test.com", "80", "/test/file.txt", "a=b&c=def&x%26y=%C2%AE", "frag");
	TEST_PARSE ("//www.test.com:80/test/file.txt?a=b&c=def&x%26y=%C2%AE#frag",
		NULL, NULL, NULL, "www.test.com", "80", "/test/file.txt", "a=b&c=def&x%26y=%C2%AE", "frag");
	TEST_PARSE ("//www.test.com/test/file.txt?a=b&c=def&x%26y=%C2%AE#frag",
		NULL, NULL, NULL, "www.test.com", NULL, "/test/file.txt", "a=b&c=def&x%26y=%C2%AE", "frag");
	TEST_PARSE ("/test/file.txt?a=b&c=def&x%26y=%C2%AE#frag",
		NULL, NULL, NULL, NULL, NULL, "/test/file.txt", "a=b&c=def&x%26y=%C2%AE", "frag");
	TEST_PARSE ("../file.txt?a=b&c=def&x%26y=%C2%AE#frag",
		NULL, NULL, NULL, NULL, NULL, "../file.txt", "a=b&c=def&x%26y=%C2%AE", "frag");
	TEST_PARSE ("?a=b&c=def&x%26y=%C2%AE#frag",
		NULL, NULL, NULL, NULL, NULL, "", "a=b&c=def&x%26y=%C2%AE", "frag");
	TEST_PARSE ("#frag",
		NULL, NULL, NULL, NULL, NULL, "", NULL, "frag");
	TEST_PARSE ("http://host:8080/http://www.site.com/images/Mapy%20Region%C3%B3w/Polska.png?a=b",
		"http", NULL, NULL, "host", "8080", "/http://www.site.com/images/Mapy%20Region%C3%B3w/Polska.png", "a=b", NULL);
	TEST_PARSE ("http://fonts.googleapis.com/css?family=Cabin+Condensed:600|Cabin:500,700,700italic,500italic",
		"http", NULL, NULL, "fonts.googleapis.com", NULL, "/css", "family=Cabin+Condensed:600|Cabin:500,700,700italic,500italic", NULL);
	TEST_PARSE ("//host.com?a=b",
		NULL, NULL, NULL, "host.com", NULL, "", "a=b", NULL);
}

static void
test_sub (void)
{
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_FRAGMENT,
			"http://user:pass@host.com:80/some/path?a=1#frag");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_QUERY,
			"http://user:pass@host.com:80/some/path?a=1");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_PATH,
			"http://user:pass@host.com:80/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_PORT,
			"http://user:pass@host.com:80");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_HOST,
			"http://user:pass@host.com");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_PASSWORD,
			"http://user:pass");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_USER,
			"http://user");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_SCHEME,
			"http:");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_USER, SP_URI_PATH,
			"//user:pass@host.com:80/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PASSWORD, SP_URI_PATH,
			"//user:pass@host.com:80/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_HOST, SP_URI_PATH,
			"//user:pass@host.com:80/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PORT, SP_URI_PATH,
			"//user:pass@host.com:80/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PATH, SP_URI_PATH,
			"/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PATH, SP_URI_QUERY,
			"/some/path?a=1");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PATH, SP_URI_FRAGMENT,
			"/some/path?a=1#frag");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_QUERY, SP_URI_QUERY,
			"?a=1");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_QUERY, SP_URI_FRAGMENT,
			"?a=1#frag");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_FRAGMENT, SP_URI_FRAGMENT,
			"#frag");
}

static void
test_sub_no_fragment (void)
{
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_SCHEME, SP_URI_FRAGMENT,
			"http://user:pass@host.com:80/some/path?a=1");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_SCHEME, SP_URI_QUERY,
			"http://user:pass@host.com:80/some/path?a=1");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_SCHEME, SP_URI_PATH,
			"http://user:pass@host.com:80/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_SCHEME, SP_URI_PORT,
			"http://user:pass@host.com:80");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_SCHEME, SP_URI_HOST,
			"http://user:pass@host.com");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_SCHEME, SP_URI_PASSWORD,
			"http://user:pass");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_SCHEME, SP_URI_USER,
			"http://user");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_SCHEME, SP_URI_SCHEME,
			"http:");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_USER, SP_URI_PATH,
			"//user:pass@host.com:80/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_PASSWORD, SP_URI_PATH,
			"//user:pass@host.com:80/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_HOST, SP_URI_PATH,
			"//user:pass@host.com:80/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_PORT, SP_URI_PATH,
			"//user:pass@host.com:80/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_PATH, SP_URI_PATH,
			"/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_PATH, SP_URI_QUERY,
			"/some/path?a=1");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_PATH, SP_URI_FRAGMENT,
			"/some/path?a=1");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_QUERY, SP_URI_QUERY,
			"?a=1");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_QUERY, SP_URI_FRAGMENT,
			"?a=1");
	TEST_SUB ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_FRAGMENT, SP_URI_FRAGMENT,
			"");
}

static void
test_sub_no_query (void)
{
	TEST_SUB ("http://user:pass@host.com:80/some/path",
			SP_URI_SCHEME, SP_URI_FRAGMENT,
			"http://user:pass@host.com:80/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path",
			SP_URI_SCHEME, SP_URI_QUERY,
			"http://user:pass@host.com:80/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path",
			SP_URI_SCHEME, SP_URI_PATH,
			"http://user:pass@host.com:80/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path",
			SP_URI_SCHEME, SP_URI_PORT,
			"http://user:pass@host.com:80");
	TEST_SUB ("http://user:pass@host.com:80/some/path",
			SP_URI_SCHEME, SP_URI_HOST,
			"http://user:pass@host.com");
	TEST_SUB ("http://user:pass@host.com:80/some/path",
			SP_URI_SCHEME, SP_URI_PASSWORD,
			"http://user:pass");
	TEST_SUB ("http://user:pass@host.com:80/some/path",
			SP_URI_SCHEME, SP_URI_USER,
			"http://user");
	TEST_SUB ("http://user:pass@host.com:80/some/path",
			SP_URI_SCHEME, SP_URI_SCHEME,
			"http:");
	TEST_SUB ("http://user:pass@host.com:80/some/path",
			SP_URI_USER, SP_URI_PATH,
			"//user:pass@host.com:80/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path",
			SP_URI_PASSWORD, SP_URI_PATH,
			"//user:pass@host.com:80/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path",
			SP_URI_HOST, SP_URI_PATH,
			"//user:pass@host.com:80/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path",
			SP_URI_PORT, SP_URI_PATH,
			"//user:pass@host.com:80/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path",
			SP_URI_PATH, SP_URI_PATH,
			"/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path",
			SP_URI_PATH, SP_URI_QUERY,
			"/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path",
			SP_URI_PATH, SP_URI_FRAGMENT,
			"/some/path");
	TEST_SUB ("http://user:pass@host.com:80/some/path",
			SP_URI_QUERY, SP_URI_QUERY,
			"");
	TEST_SUB ("http://user:pass@host.com:80/some/path",
			SP_URI_QUERY, SP_URI_FRAGMENT,
			"");
	TEST_SUB ("http://user:pass@host.com:80/some/path",
			SP_URI_FRAGMENT, SP_URI_FRAGMENT,
			"");
}

static void
test_sub_scheme_rel (void)
{
	TEST_SUB ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_FRAGMENT,
			"//user:pass@host.com:80/some/path?a=1#frag");
	TEST_SUB ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_USER, SP_URI_PATH,
			"//user:pass@host.com:80/some/path");
	TEST_SUB ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PASSWORD, SP_URI_PATH,
			"//user:pass@host.com:80/some/path");
	TEST_SUB ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_HOST, SP_URI_PATH,
			"//user:pass@host.com:80/some/path");
	TEST_SUB ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PORT, SP_URI_PATH,
			"//user:pass@host.com:80/some/path");
	TEST_SUB ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PATH, SP_URI_PATH,
			"/some/path");
	TEST_SUB ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PATH, SP_URI_QUERY,
			"/some/path?a=1");
	TEST_SUB ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PATH, SP_URI_FRAGMENT,
			"/some/path?a=1#frag");
	TEST_SUB ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_QUERY, SP_URI_QUERY,
			"?a=1");
	TEST_SUB ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_QUERY, SP_URI_FRAGMENT,
			"?a=1#frag");
	TEST_SUB ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_FRAGMENT, SP_URI_FRAGMENT,
			"#frag");
}

static void
test_sub_path_rel (void)
{
	TEST_SUB ("/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_FRAGMENT,
			"/some/path?a=1#frag");
	TEST_SUB ("/some/path?a=1#frag",
			SP_URI_USER, SP_URI_PATH,
			"/some/path");
	TEST_SUB ("/some/path?a=1#frag",
			SP_URI_PASSWORD, SP_URI_PATH,
			"/some/path");
	TEST_SUB ("/some/path?a=1#frag",
			SP_URI_HOST, SP_URI_PATH,
			"/some/path");
	TEST_SUB ("/some/path?a=1#frag",
			SP_URI_PORT, SP_URI_PATH,
			"/some/path");
	TEST_SUB ("/some/path?a=1#frag",
			SP_URI_PATH, SP_URI_PATH,
			"/some/path");
	TEST_SUB ("/some/path?a=1#frag",
			SP_URI_PATH, SP_URI_QUERY,
			"/some/path?a=1");
	TEST_SUB ("/some/path?a=1#frag",
			SP_URI_PATH, SP_URI_FRAGMENT,
			"/some/path?a=1#frag");
	TEST_SUB ("/some/path?a=1#frag",
			SP_URI_QUERY, SP_URI_QUERY,
			"?a=1");
	TEST_SUB ("/some/path?a=1#frag",
			SP_URI_QUERY, SP_URI_FRAGMENT,
			"?a=1#frag");
	TEST_SUB ("/some/path?a=1#frag",
			SP_URI_FRAGMENT, SP_URI_FRAGMENT,
			"#frag");
}

static void
test_sub_query_rel (void)
{
	TEST_SUB ("?a=1#frag",
			SP_URI_SCHEME, SP_URI_FRAGMENT,
			"?a=1#frag");
	TEST_SUB ("?a=1#frag",
			SP_URI_USER, SP_URI_PATH,
			"");
	TEST_SUB ("?a=1#frag",
			SP_URI_PASSWORD, SP_URI_PATH,
			"");
	TEST_SUB ("?a=1#frag",
			SP_URI_HOST, SP_URI_PATH,
			"");
	TEST_SUB ("?a=1#frag",
			SP_URI_PORT, SP_URI_PATH,
			"");
	TEST_SUB ("?a=1#frag",
			SP_URI_PATH, SP_URI_PATH,
			"");
	TEST_SUB ("?a=1#frag",
			SP_URI_PATH, SP_URI_QUERY,
			"?a=1");
	TEST_SUB ("?a=1#frag",
			SP_URI_PATH, SP_URI_FRAGMENT,
			"?a=1#frag");
	TEST_SUB ("?a=1#frag",
			SP_URI_QUERY, SP_URI_QUERY,
			"?a=1");
	TEST_SUB ("?a=1#frag",
			SP_URI_QUERY, SP_URI_FRAGMENT,
			"?a=1#frag");
	TEST_SUB ("?a=1#frag",
			SP_URI_FRAGMENT, SP_URI_FRAGMENT,
			"#frag");
}

static void
test_sub_fragment_rel (void)
{
	TEST_SUB ("#frag",
			SP_URI_SCHEME, SP_URI_FRAGMENT,
			"#frag");
	TEST_SUB ("#frag",
			SP_URI_USER, SP_URI_PATH,
			"");
	TEST_SUB ("#frag",
			SP_URI_PASSWORD, SP_URI_PATH,
			"");
	TEST_SUB ("#frag",
			SP_URI_HOST, SP_URI_PATH,
			"");
	TEST_SUB ("#frag",
			SP_URI_PORT, SP_URI_PATH,
			"");
	TEST_SUB ("#frag",
			SP_URI_PATH, SP_URI_PATH,
			"");
	TEST_SUB ("#frag",
			SP_URI_PATH, SP_URI_QUERY,
			"");
	TEST_SUB ("#frag",
			SP_URI_PATH, SP_URI_FRAGMENT,
			"#frag");
	TEST_SUB ("#frag",
			SP_URI_QUERY, SP_URI_QUERY,
			"");
	TEST_SUB ("#frag",
			SP_URI_QUERY, SP_URI_FRAGMENT,
			"#frag");
	TEST_SUB ("#frag",
			SP_URI_FRAGMENT, SP_URI_FRAGMENT,
			"#frag");
}

static void
test_range (void)
{
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_FRAGMENT,
			"http://user:pass@host.com:80/some/path?a=1#frag");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_QUERY,
			"http://user:pass@host.com:80/some/path?a=1");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_PATH,
			"http://user:pass@host.com:80/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_PORT,
			"http://user:pass@host.com:80");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_HOST,
			"http://user:pass@host.com");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_PASSWORD,
			"http://user:pass");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_USER,
			"http://user");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_SCHEME,
			"http");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_USER, SP_URI_PATH,
			"user:pass@host.com:80/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PASSWORD, SP_URI_PATH,
			"pass@host.com:80/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_HOST, SP_URI_PATH,
			"host.com:80/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PORT, SP_URI_PATH,
			"80/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PATH, SP_URI_PATH,
			"/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PATH, SP_URI_QUERY,
			"/some/path?a=1");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PATH, SP_URI_FRAGMENT,
			"/some/path?a=1#frag");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_QUERY, SP_URI_QUERY,
			"a=1");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_QUERY, SP_URI_FRAGMENT,
			"a=1#frag");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_FRAGMENT, SP_URI_FRAGMENT,
			"frag");
}

static void
test_range_no_fragment (void)
{
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_SCHEME, SP_URI_FRAGMENT,
			"http://user:pass@host.com:80/some/path?a=1");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_SCHEME, SP_URI_QUERY,
			"http://user:pass@host.com:80/some/path?a=1");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_SCHEME, SP_URI_PATH,
			"http://user:pass@host.com:80/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_SCHEME, SP_URI_PORT,
			"http://user:pass@host.com:80");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_SCHEME, SP_URI_HOST,
			"http://user:pass@host.com");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_SCHEME, SP_URI_PASSWORD,
			"http://user:pass");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_SCHEME, SP_URI_USER,
			"http://user");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_SCHEME, SP_URI_SCHEME,
			"http");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_USER, SP_URI_PATH,
			"user:pass@host.com:80/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_PASSWORD, SP_URI_PATH,
			"pass@host.com:80/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_HOST, SP_URI_PATH,
			"host.com:80/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_PORT, SP_URI_PATH,
			"80/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_PATH, SP_URI_PATH,
			"/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_PATH, SP_URI_QUERY,
			"/some/path?a=1");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_PATH, SP_URI_FRAGMENT,
			"/some/path?a=1");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_QUERY, SP_URI_QUERY,
			"a=1");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1",
			SP_URI_QUERY, SP_URI_FRAGMENT,
			"a=1");
	TEST_RANGE ("http://user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_FRAGMENT, SP_URI_FRAGMENT,
			"frag");
}

static void
test_range_no_query (void)
{
	TEST_RANGE ("http://user:pass@host.com:80/some/path",
			SP_URI_SCHEME, SP_URI_FRAGMENT,
			"http://user:pass@host.com:80/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path",
			SP_URI_SCHEME, SP_URI_QUERY,
			"http://user:pass@host.com:80/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path",
			SP_URI_SCHEME, SP_URI_PATH,
			"http://user:pass@host.com:80/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path",
			SP_URI_SCHEME, SP_URI_PORT,
			"http://user:pass@host.com:80");
	TEST_RANGE ("http://user:pass@host.com:80/some/path",
			SP_URI_SCHEME, SP_URI_HOST,
			"http://user:pass@host.com");
	TEST_RANGE ("http://user:pass@host.com:80/some/path",
			SP_URI_SCHEME, SP_URI_PASSWORD,
			"http://user:pass");
	TEST_RANGE ("http://user:pass@host.com:80/some/path",
			SP_URI_SCHEME, SP_URI_USER,
			"http://user");
	TEST_RANGE ("http://user:pass@host.com:80/some/path",
			SP_URI_SCHEME, SP_URI_SCHEME,
			"http");
	TEST_RANGE ("http://user:pass@host.com:80/some/path",
			SP_URI_USER, SP_URI_PATH,
			"user:pass@host.com:80/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path",
			SP_URI_PASSWORD, SP_URI_PATH,
			"pass@host.com:80/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path",
			SP_URI_HOST, SP_URI_PATH,
			"host.com:80/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path",
			SP_URI_PORT, SP_URI_PATH,
			"80/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path",
			SP_URI_PATH, SP_URI_PATH,
			"/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path",
			SP_URI_PATH, SP_URI_QUERY,
			"/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path",
			SP_URI_PATH, SP_URI_FRAGMENT,
			"/some/path");
	TEST_RANGE ("http://user:pass@host.com:80/some/path",
			SP_URI_QUERY, SP_URI_QUERY,
			"");
	TEST_RANGE ("http://user:pass@host.com:80/some/path",
			SP_URI_QUERY, SP_URI_FRAGMENT,
			"");
	TEST_RANGE ("http://user:pass@host.com:80/some/path",
			SP_URI_FRAGMENT, SP_URI_FRAGMENT,
			"");
}

static void
test_range_scheme_rel (void)
{
	TEST_RANGE ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_FRAGMENT,
			"user:pass@host.com:80/some/path?a=1#frag");
	TEST_RANGE ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_USER, SP_URI_PATH,
			"user:pass@host.com:80/some/path");
	TEST_RANGE ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PASSWORD, SP_URI_PATH,
			"pass@host.com:80/some/path");
	TEST_RANGE ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_HOST, SP_URI_PATH,
			"host.com:80/some/path");
	TEST_RANGE ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PORT, SP_URI_PATH,
			"80/some/path");
	TEST_RANGE ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PATH, SP_URI_PATH,
			"/some/path");
	TEST_RANGE ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PATH, SP_URI_QUERY,
			"/some/path?a=1");
	TEST_RANGE ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_PATH, SP_URI_FRAGMENT,
			"/some/path?a=1#frag");
	TEST_RANGE ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_QUERY, SP_URI_QUERY,
			"a=1");
	TEST_RANGE ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_QUERY, SP_URI_FRAGMENT,
			"a=1#frag");
	TEST_RANGE ("//user:pass@host.com:80/some/path?a=1#frag",
			SP_URI_FRAGMENT, SP_URI_FRAGMENT,
			"frag");
}

static void
test_range_path_rel (void)
{
	TEST_RANGE ("/some/path?a=1#frag",
			SP_URI_SCHEME, SP_URI_FRAGMENT,
			"/some/path?a=1#frag");
	TEST_RANGE ("/some/path?a=1#frag",
			SP_URI_USER, SP_URI_PATH,
			"/some/path");
	TEST_RANGE ("/some/path?a=1#frag",
			SP_URI_PASSWORD, SP_URI_PATH,
			"/some/path");
	TEST_RANGE ("/some/path?a=1#frag",
			SP_URI_HOST, SP_URI_PATH,
			"/some/path");
	TEST_RANGE ("/some/path?a=1#frag",
			SP_URI_PORT, SP_URI_PATH,
			"/some/path");
	TEST_RANGE ("/some/path?a=1#frag",
			SP_URI_PATH, SP_URI_PATH,
			"/some/path");
	TEST_RANGE ("/some/path?a=1#frag",
			SP_URI_PATH, SP_URI_QUERY,
			"/some/path?a=1");
	TEST_RANGE ("/some/path?a=1#frag",
			SP_URI_PATH, SP_URI_FRAGMENT,
			"/some/path?a=1#frag");
	TEST_RANGE ("/some/path?a=1#frag",
			SP_URI_QUERY, SP_URI_QUERY,
			"a=1");
	TEST_RANGE ("/some/path?a=1#frag",
			SP_URI_QUERY, SP_URI_FRAGMENT,
			"a=1#frag");
	TEST_RANGE ("/some/path?a=1#frag",
			SP_URI_FRAGMENT, SP_URI_FRAGMENT,
			"frag");
}

static void
test_range_query_rel (void)
{
	TEST_RANGE ("?a=1#frag",
			SP_URI_SCHEME, SP_URI_FRAGMENT,
			"a=1#frag");
	TEST_RANGE ("?a=1#frag",
			SP_URI_USER, SP_URI_PATH,
			"");
	TEST_RANGE ("?a=1#frag",
			SP_URI_PASSWORD, SP_URI_PATH,
			"");
	TEST_RANGE ("?a=1#frag",
			SP_URI_HOST, SP_URI_PATH,
			"");
	TEST_RANGE ("?a=1#frag",
			SP_URI_PORT, SP_URI_PATH,
			"");
	TEST_RANGE ("?a=1#frag",
			SP_URI_PATH, SP_URI_PATH,
			"");
	TEST_RANGE ("?a=1#frag",
			SP_URI_PATH, SP_URI_QUERY,
			"a=1");
	TEST_RANGE ("?a=1#frag",
			SP_URI_PATH, SP_URI_FRAGMENT,
			"a=1#frag");
	TEST_RANGE ("?a=1#frag",
			SP_URI_QUERY, SP_URI_QUERY,
			"a=1");
	TEST_RANGE ("?a=1#frag",
			SP_URI_QUERY, SP_URI_FRAGMENT,
			"a=1#frag");
	TEST_RANGE ("?a=1#frag",
			SP_URI_FRAGMENT, SP_URI_FRAGMENT,
			"frag");
}

static void
test_range_fragment_rel (void)
{
	TEST_RANGE ("#frag",
			SP_URI_SCHEME, SP_URI_FRAGMENT,
			"frag");
	TEST_RANGE ("#frag",
			SP_URI_USER, SP_URI_PATH,
			"");
	TEST_RANGE ("#frag",
			SP_URI_PASSWORD, SP_URI_PATH,
			"");
	TEST_RANGE ("#frag",
			SP_URI_HOST, SP_URI_PATH,
			"");
	TEST_RANGE ("#frag",
			SP_URI_PORT, SP_URI_PATH,
			"");
	TEST_RANGE ("#frag",
			SP_URI_PATH, SP_URI_PATH,
			"");
	TEST_RANGE ("#frag",
			SP_URI_PATH, SP_URI_QUERY,
			"");
	TEST_RANGE ("#frag",
			SP_URI_PATH, SP_URI_FRAGMENT,
			"frag");
	TEST_RANGE ("#frag",
			SP_URI_QUERY, SP_URI_QUERY,
			"");
	TEST_RANGE ("#frag",
			SP_URI_QUERY, SP_URI_FRAGMENT,
			"frag");
	TEST_RANGE ("#frag",
			SP_URI_FRAGMENT, SP_URI_FRAGMENT,
			"frag");
}

static void
test_join (void)
{
	TEST_JOIN ("http://a/b/c/d;p?q", "g:h"           , "g:h");
	TEST_JOIN ("http://a/b/c/d;p?q", "g"             , "http://a/b/c/g");
	TEST_JOIN ("http://a/b/c/d;p?q", "./g"           , "http://a/b/c/g");
	TEST_JOIN ("http://a/b/c/d;p?q", "g/"            , "http://a/b/c/g/");
	TEST_JOIN ("http://a/b/c/d;p?q", "/g"            , "http://a/g");
	TEST_JOIN ("http://a/b/c/d;p?q", "//g"           , "http://g");
	TEST_JOIN ("http://a/b/c/d;p?q", "?y"            , "http://a/b/c/d;p?y");
	TEST_JOIN ("http://a/b/c/d;p?q", "g?y"           , "http://a/b/c/g?y");
	TEST_JOIN ("http://a/b/c/d;p?q", "#s"            , "http://a/b/c/d;p?q#s");
	TEST_JOIN ("http://a/b/c/d;p?q", "g#s"           , "http://a/b/c/g#s");
	TEST_JOIN ("http://a/b/c/d;p?q", "g?y#s"         , "http://a/b/c/g?y#s");
	TEST_JOIN ("http://a/b/c/d;p?q", ";x"            , "http://a/b/c/;x");
	TEST_JOIN ("http://a/b/c/d;p?q", "g;x"           , "http://a/b/c/g;x");
	TEST_JOIN ("http://a/b/c/d;p?q", "g;x?y#s"       , "http://a/b/c/g;x?y#s");
	TEST_JOIN ("http://a/b/c/d;p?q", ""              , "http://a/b/c/d;p?q");
	TEST_JOIN ("http://a/b/c/d;p?q", "."             , "http://a/b/c/");
	TEST_JOIN ("http://a/b/c/d;p?q", "./"            , "http://a/b/c/");
	TEST_JOIN ("http://a/b/c/d;p?q", ".."            , "http://a/b/");
	TEST_JOIN ("http://a/b/c/d;p?q", "../"           , "http://a/b/");
	TEST_JOIN ("http://a/b/c/d;p?q", "../g"          , "http://a/b/g");
	TEST_JOIN ("http://a/b/c/d;p?q", "../.."         , "http://a/");
	TEST_JOIN ("http://a/b/c/d;p?q", "../../"        , "http://a/");
	TEST_JOIN ("http://a/b/c/d;p?q", "../../g"       , "http://a/g");
	TEST_JOIN ("http://a/b/c/d;p?q", "../../../g"    , "http://a/g");
	TEST_JOIN ("http://a/b/c/d;p?q", "../../../../g" , "http://a/g");
	TEST_JOIN ("http://a/b/c/d;p?q", "/./g"          , "http://a/g");
	TEST_JOIN ("http://a/b/c/d;p?q", "/../g"         , "http://a/g");
	TEST_JOIN ("http://a/b/c/d;p?q", "g."            , "http://a/b/c/g.");
	TEST_JOIN ("http://a/b/c/d;p?q", ".g"            , "http://a/b/c/.g");
	TEST_JOIN ("http://a/b/c/d;p?q", "g.."           , "http://a/b/c/g..");
	TEST_JOIN ("http://a/b/c/d;p?q", "..g"           , "http://a/b/c/..g");
	TEST_JOIN ("http://a/b/c/d;p?q", "./../g"        , "http://a/b/g");
	TEST_JOIN ("http://a/b/c/d;p?q", "./g/."         , "http://a/b/c/g/");
	TEST_JOIN ("http://a/b/c/d;p?q", "g/./h"         , "http://a/b/c/g/h");
	TEST_JOIN ("http://a/b/c/d;p?q", "g/../h"        , "http://a/b/c/h");
	TEST_JOIN ("http://a/b/c/d;p?q", "g;x=1/./y"     , "http://a/b/c/g;x=1/y");
	TEST_JOIN ("http://a/b/c/d;p?q", "g;x=1/../y"    , "http://a/b/c/y");
	TEST_JOIN ("http://a/b/c/d;p?q", "g?y/./x"       , "http://a/b/c/g?y/./x");
	TEST_JOIN ("http://a/b/c/d;p?q", "g?y/../x"      , "http://a/b/c/g?y/../x");
	TEST_JOIN ("http://a/b/c/d;p?q", "g#s/./x"       , "http://a/b/c/g#s/./x");
	TEST_JOIN ("http://a/b/c/d;p?q", "g#s/../x"      , "http://a/b/c/g#s/../x");
	TEST_JOIN ("http://a/b/c/d;p?q", "http:g"        , "http:g");

	TEST_JOIN_SEGMENTS (
			"http://base.com/some/path.html?a=b#c:w",
			"//user:pass@www.test.com:80/test/file.txt?a=b&c=def&x%26y=%C2%AE#frag",
		"http", "user", "pass", "www.test.com", "80", "/test/file.txt", "a=b&c=def&x%26y=%C2%AE", "frag");

	TEST_JOIN_SEGMENTS (
			"http://base.com/some/path.html?a=b#c:w",
			"//www.test.com:80/test/file.txt?a=b&c=def&x%26y=%C2%AE#frag",
		"http", NULL, NULL, "www.test.com", "80", "/test/file.txt", "a=b&c=def&x%26y=%C2%AE", "frag");

	TEST_JOIN_SEGMENTS (
			"http://base.com/some/path.html?a=b#c:w",
			"//www.test.com/test/file.txt?a=b&c=def&x%26y=%C2%AE#frag",
		"http", NULL, NULL, "www.test.com", NULL, "/test/file.txt", "a=b&c=def&x%26y=%C2%AE", "frag");

	TEST_JOIN_SEGMENTS (
			"//www.test.com/some/path.html",
			"../other.html",
		NULL, NULL, NULL, "www.test.com", NULL, "/other.html", NULL, NULL);

	TEST_JOIN_SEGMENTS (
			"http://base.com/some/path.html?a=b#c:w",
			"/test/file.txt?a=b&c=def&x%26y=%C2%AE#frag",
		"http", NULL, NULL, "base.com", NULL, "/test/file.txt", "a=b&c=def&x%26y=%C2%AE", "frag");

	TEST_JOIN_SEGMENTS (
			"/some/path.html",
			"../other.html",
		NULL, NULL, NULL, NULL, NULL, "/other.html", NULL, NULL);

	TEST_JOIN_SEGMENTS (
			"http://base.com/some/path.html?a=b#c:w",
			"../file.txt?a=b&c=def&x%26y=%C2%AE#frag",
		"http", NULL, NULL, "base.com", NULL, "/file.txt", "a=b&c=def&x%26y=%C2%AE", "frag");

	TEST_JOIN_SEGMENTS (
			"http://base.com/some/path.html?a=b#c:w",
			"?a=b&c=def&x%26y=%C2%AE#frag",
		"http", NULL, NULL, "base.com", NULL, "/some/path.html", "a=b&c=def&x%26y=%C2%AE", "frag");

	TEST_JOIN_SEGMENTS (
			"/some/path.html",
			"?a=b",
		NULL, NULL, NULL, NULL, NULL, "/some/path.html", "a=b", NULL);

	TEST_JOIN_SEGMENTS (
			"http://base.com/some/path.html?a=b#c:w",
			"#frag",
		"http", NULL, NULL, "base.com", NULL, "/some/path.html", "a=b", "frag");

	TEST_JOIN_SEGMENTS (
			"http://www.test.com/",
			"http://www.test.com/dir/",
		"http", NULL, NULL, "www.test.com", NULL, "/dir/", NULL, NULL);

	TEST_JOIN_SEGMENTS (
			"http://www.test.com/dir/",
			"file.html",
		"http", NULL, NULL, "www.test.com", NULL, "/dir/file.html", NULL, NULL);
}

static void
test_ipv6 (void)
{
	TEST_IPv6_VALID ("0000:0000:0000:0000:0000:0000:0000:0000");
	TEST_IPv6_VALID ("0000:0000:0000:0000:0000:0000:0000:0001");
	TEST_IPv6_VALID ("0000:0000:0000:0000:0000:0000:0000:0001");
	TEST_IPv6_VALID ("0:0:0:0:0:0:0:0");// unspecified, full 
	TEST_IPv6_VALID ("0:0:0:0:0:0:0:1");// loopback, full 
	TEST_IPv6_VALID ("0:0:0:0:0:0:0::");
	TEST_IPv6_VALID ("0:0:0:0:0:0::");
	TEST_IPv6_VALID ("0:0:0:0:0::");
	TEST_IPv6_VALID ("0:0:0:0::");
	TEST_IPv6_VALID ("0:0:0::");
	TEST_IPv6_VALID ("0:0::");
	TEST_IPv6_VALID ("0::");
	TEST_IPv6_VALID ("0:a:b:c:d:e:f::");
	TEST_IPv6_VALID ("1111:2222:3333:4444:5555:6666:7777:8888");
	TEST_IPv6_VALID ("1111:2222:3333:4444:5555:6666:7777::");
	TEST_IPv6_VALID ("1111:2222:3333:4444:5555:6666::");
	TEST_IPv6_VALID ("1111:2222:3333:4444:5555:6666::8888");
	TEST_IPv6_VALID ("1111:2222:3333:4444:5555::");
	TEST_IPv6_VALID ("1111:2222:3333:4444:5555::7777:8888");
	TEST_IPv6_VALID ("1111:2222:3333:4444:5555::8888");
	TEST_IPv6_VALID ("1111:2222:3333:4444::");
	TEST_IPv6_VALID ("1111:2222:3333:4444::6666:7777:8888");
	TEST_IPv6_VALID ("1111:2222:3333:4444::7777:8888");
	TEST_IPv6_VALID ("1111:2222:3333:4444::8888");
	TEST_IPv6_VALID ("1111:2222:3333::");
	TEST_IPv6_VALID ("1111:2222:3333::5555:6666:7777:8888");
	TEST_IPv6_VALID ("1111:2222:3333::6666:7777:8888");
	TEST_IPv6_VALID ("1111:2222:3333::7777:8888");
	TEST_IPv6_VALID ("1111:2222:3333::8888");
	TEST_IPv6_VALID ("1111:2222::");
	TEST_IPv6_VALID ("1111:2222::4444:5555:6666:7777:8888");
	TEST_IPv6_VALID ("1111:2222::5555:6666:7777:8888");
	TEST_IPv6_VALID ("1111:2222::6666:7777:8888");
	TEST_IPv6_VALID ("1111:2222::7777:8888");
	TEST_IPv6_VALID ("1111:2222::8888");
	TEST_IPv6_VALID ("1111::");
	TEST_IPv6_VALID ("1111::3333:4444:5555:6666:7777:8888");
	TEST_IPv6_VALID ("1111::4444:5555:6666:7777:8888");
	TEST_IPv6_VALID ("1111::5555:6666:7777:8888");
	TEST_IPv6_VALID ("1111::6666:7777:8888");
	TEST_IPv6_VALID ("1111::7777:8888");
	TEST_IPv6_VALID ("1111::8888");
	TEST_IPv6_VALID ("1:2:3:4:5:6:7:8");
	TEST_IPv6_VALID ("1:2:3:4:5:6::");
	TEST_IPv6_VALID ("1:2:3:4:5:6::8");
	TEST_IPv6_VALID ("1:2:3:4:5::");
	TEST_IPv6_VALID ("1:2:3:4:5::7:8");
	TEST_IPv6_VALID ("1:2:3:4:5::8");
	TEST_IPv6_VALID ("1:2:3:4::");
	TEST_IPv6_VALID ("1:2:3:4::7:8");
	TEST_IPv6_VALID ("1:2:3:4::8");
	TEST_IPv6_VALID ("1:2:3::");
	TEST_IPv6_VALID ("1:2:3::7:8");
	TEST_IPv6_VALID ("1:2:3::8");
	TEST_IPv6_VALID ("1:2::");
	TEST_IPv6_VALID ("1:2::7:8");
	TEST_IPv6_VALID ("1:2::8");
	TEST_IPv6_VALID ("1::");
	TEST_IPv6_VALID ("1::2:3");
	TEST_IPv6_VALID ("1::2:3:4");
	TEST_IPv6_VALID ("1::2:3:4:5");
	TEST_IPv6_VALID ("1::2:3:4:5:6");
	TEST_IPv6_VALID ("1::2:3:4:5:6:7");
	TEST_IPv6_VALID ("1::7:8");
	TEST_IPv6_VALID ("1::8");
	TEST_IPv6_VALID ("1::8");
	TEST_IPv6_VALID ("2001:0000:1234:0000:0000:C1C0:ABCD:0876");
	TEST_IPv6_VALID ("2001:0db8:0000:0000:0000:0000:1428:57ab");
	TEST_IPv6_VALID ("2001:0db8:0000:0000:0000::1428:57ab");
	TEST_IPv6_VALID ("2001:0db8:0:0:0:0:1428:57ab");
	TEST_IPv6_VALID ("2001:0db8:0:0::1428:57ab");
	TEST_IPv6_VALID ("2001:0db8:1234:0000:0000:0000:0000:0000");
	TEST_IPv6_VALID ("2001:0db8:1234::");
	TEST_IPv6_VALID ("2001:0db8:1234:ffff:ffff:ffff:ffff:ffff");
	TEST_IPv6_VALID ("2001:0db8:85a3:0000:0000:8a2e:0370:7334");
	TEST_IPv6_VALID ("2001:0db8::1428:57ab");
	TEST_IPv6_VALID ("2001:DB8:0:0:8:800:200C:417A");// unicast, full 
	TEST_IPv6_VALID ("2001:DB8::8:800:200C:417A");// unicast, compressed 
	TEST_IPv6_VALID ("2001:db8:85a3:0:0:8a2e:370:7334");
	TEST_IPv6_VALID ("2001:db8:85a3::8a2e:370:7334");
	TEST_IPv6_VALID ("2001:db8::");
	TEST_IPv6_VALID ("2001:db8::1428:57ab");
	TEST_IPv6_VALID ("2001:db8:a::123");
	TEST_IPv6_VALID ("2002::");
	TEST_IPv6_VALID ("2::10");
	TEST_IPv6_VALID ("3ffe:0b00:0000:0000:0001:0000:0000:000a");
	TEST_IPv6_VALID ("::");
	TEST_IPv6_VALID ("::");// unspecified, compressed, non-routable 
	TEST_IPv6_VALID ("::0");
	TEST_IPv6_VALID ("::0:0");
	TEST_IPv6_VALID ("::0:0:0");
	TEST_IPv6_VALID ("::0:0:0:0");
	TEST_IPv6_VALID ("::0:0:0:0:0");
	TEST_IPv6_VALID ("::0:0:0:0:0:0");
	TEST_IPv6_VALID ("::0:0:0:0:0:0:0");
	TEST_IPv6_VALID ("::0:a:b:c:d:e:f"); 	// syntactically correct, but bad form (::0:... could be combined)
	TEST_IPv6_VALID ("::1");
	TEST_IPv6_VALID ("::1");
	TEST_IPv6_VALID ("::1");
	TEST_IPv6_VALID ("::1");// loopback, compressed, non-routable 
	TEST_IPv6_VALID ("::2222:3333:4444:5555:6666:7777:8888");
	TEST_IPv6_VALID ("::2:3");
	TEST_IPv6_VALID ("::2:3:4");
	TEST_IPv6_VALID ("::2:3:4:5");
	TEST_IPv6_VALID ("::2:3:4:5:6");
	TEST_IPv6_VALID ("::2:3:4:5:6:7");
	TEST_IPv6_VALID ("::2:3:4:5:6:7:8");
	TEST_IPv6_VALID ("::3333:4444:5555:6666:7777:8888");
	TEST_IPv6_VALID ("::4444:5555:6666:7777:8888");
	TEST_IPv6_VALID ("::5555:6666:7777:8888");
	TEST_IPv6_VALID ("::6666:7777:8888");
	TEST_IPv6_VALID ("::7777:8888");
	TEST_IPv6_VALID ("::8");
	TEST_IPv6_VALID ("::8888");
	TEST_IPv6_VALID ("::ffff:0:0");
	TEST_IPv6_VALID ("::ffff:0c22:384e");
	TEST_IPv6_VALID ("::ffff:c000:280");
	TEST_IPv6_VALID ("FF01:0:0:0:0:0:0:101");// multicast, full 
	TEST_IPv6_VALID ("FF01::101");// multicast, compressed 
	TEST_IPv6_VALID ("FF02:0000:0000:0000:0000:0000:0000:0001");
	TEST_IPv6_VALID ("a:b:c:d:e:f:0::");
	TEST_IPv6_VALID ("fe80:0000:0000:0000:0204:61ff:fe9d:f156");
	TEST_IPv6_VALID ("fe80:0:0:0:204:61ff:fe9d:f156");
	TEST_IPv6_VALID ("fe80::");
	TEST_IPv6_VALID ("fe80::");
	TEST_IPv6_VALID ("fe80::");
	TEST_IPv6_VALID ("fe80::1");
	TEST_IPv6_VALID ("fe80::204:61ff:fe9d:f156");
	TEST_IPv6_VALID ("fe80::217:f2ff:fe07:ed62");
	TEST_IPv6_VALID ("ff02::1");

	TEST_IPv6_INVALID ("02001:0000:1234:0000:0000:C1C0:ABCD:0876");	// extra 0 not allowed! 
	TEST_IPv6_INVALID ("1111");
	TEST_IPv6_INVALID ("11112222:3333:4444:5555:6666:7777:8888");
	TEST_IPv6_INVALID ("1111:");
	TEST_IPv6_INVALID ("1111:");
	TEST_IPv6_INVALID ("1111:2222");
	TEST_IPv6_INVALID ("1111:22223333:4444:5555:6666:7777:8888");
	TEST_IPv6_INVALID ("1111:2222:");
	TEST_IPv6_INVALID ("1111:2222:3333");
	TEST_IPv6_INVALID ("1111:2222:33334444:5555:6666:7777:8888");
	TEST_IPv6_INVALID ("1111:2222:3333:");
	TEST_IPv6_INVALID ("1111:2222:3333:4444");
	TEST_IPv6_INVALID ("1111:2222:3333:44445555:6666:7777:8888");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:55556666:7777:8888");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555:");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555:6666");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555:66667777:8888");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555:6666:");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555:6666:7777");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555:6666:77778888");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555:6666:7777:");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555:6666:7777:8888:");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555:6666:7777:8888:9999");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555:6666:7777:8888::");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555:6666:7777:::");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555:6666:7777:::");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555:6666::8888:");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555:6666:::");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555:6666:::8888");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555::7777:8888:");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555::7777::");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555::8888:");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555:::");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:5555:::7777:8888");
	TEST_IPv6_INVALID ("1111:2222:3333:4444::5555:");
	TEST_IPv6_INVALID ("1111:2222:3333:4444::6666:7777:8888:");
	TEST_IPv6_INVALID ("1111:2222:3333:4444::6666:7777::");
	TEST_IPv6_INVALID ("1111:2222:3333:4444::6666::8888");
	TEST_IPv6_INVALID ("1111:2222:3333:4444::7777:8888:");
	TEST_IPv6_INVALID ("1111:2222:3333:4444::8888:");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:::");
	TEST_IPv6_INVALID ("1111:2222:3333:4444:::6666:7777:8888");
	TEST_IPv6_INVALID ("1111:2222:3333::5555:");
	TEST_IPv6_INVALID ("1111:2222:3333::5555:6666:7777:8888:");
	TEST_IPv6_INVALID ("1111:2222:3333::5555:6666:7777::");
	TEST_IPv6_INVALID ("1111:2222:3333::5555:6666::8888");
	TEST_IPv6_INVALID ("1111:2222:3333::5555::7777:8888");
	TEST_IPv6_INVALID ("1111:2222:3333::6666:7777:8888:");
	TEST_IPv6_INVALID ("1111:2222:3333::7777:8888:");
	TEST_IPv6_INVALID ("1111:2222:3333::8888:");
	TEST_IPv6_INVALID ("1111:2222:3333:::");
	TEST_IPv6_INVALID ("1111:2222:3333:::5555:6666:7777:8888");
	TEST_IPv6_INVALID ("1111:2222::4444:5555:6666:7777:8888:");
	TEST_IPv6_INVALID ("1111:2222::4444:5555:6666:7777::");
	TEST_IPv6_INVALID ("1111:2222::4444:5555:6666::8888");
	TEST_IPv6_INVALID ("1111:2222::4444:5555::7777:8888");
	TEST_IPv6_INVALID ("1111:2222::4444::6666:7777:8888");
	TEST_IPv6_INVALID ("1111:2222::5555:");
	TEST_IPv6_INVALID ("1111:2222::5555:6666:7777:8888:");
	TEST_IPv6_INVALID ("1111:2222::6666:7777:8888:");
	TEST_IPv6_INVALID ("1111:2222::7777:8888:");
	TEST_IPv6_INVALID ("1111:2222::8888:");
	TEST_IPv6_INVALID ("1111:2222:::");
	TEST_IPv6_INVALID ("1111:2222:::4444:5555:6666:7777:8888");
	TEST_IPv6_INVALID ("1111::3333:4444:5555:6666:7777:8888:");
	TEST_IPv6_INVALID ("1111::3333:4444:5555:6666:7777::");
	TEST_IPv6_INVALID ("1111::3333:4444:5555:6666::8888");
	TEST_IPv6_INVALID ("1111::3333:4444:5555::7777:8888");
	TEST_IPv6_INVALID ("1111::3333:4444::6666:7777:8888");
	TEST_IPv6_INVALID ("1111::3333::5555:6666:7777:8888");
	TEST_IPv6_INVALID ("1111::4444:5555:6666:7777:8888:");
	TEST_IPv6_INVALID ("1111::5555:");
	TEST_IPv6_INVALID ("1111::5555:6666:7777:8888:");
	TEST_IPv6_INVALID ("1111::6666:7777:8888:");
	TEST_IPv6_INVALID ("1111::7777:8888:");
	TEST_IPv6_INVALID ("1111::8888:");
	TEST_IPv6_INVALID ("1111:::");
	TEST_IPv6_INVALID ("1111:::3333:4444:5555:6666:7777:8888");
	TEST_IPv6_INVALID ("123");
	TEST_IPv6_INVALID ("12345::6:7:8");
	TEST_IPv6_INVALID ("1:2:3:4:5:6:7:8:9");
	TEST_IPv6_INVALID ("1:2:3::4:5:6:7:8:9");
	TEST_IPv6_INVALID ("1:2:3::4:5::7:8");							// Double "::"
	TEST_IPv6_INVALID ("1::2::3");
	TEST_IPv6_INVALID ("1:::3:4:5");
	TEST_IPv6_INVALID ("2001:0000:1234: 0000:0000:C1C0:ABCD:0876");	// internal space
	TEST_IPv6_INVALID ("2001:0000:1234:0000:00001:C1C0:ABCD:0876");	// extra 0 not allowed! 
	TEST_IPv6_INVALID ("2001:0000:1234:0000:0000:C1C0:ABCD:0876  0");	// junk after valid address
	TEST_IPv6_INVALID ("2001::FFD3::57ab");
	TEST_IPv6_INVALID ("2001:DB8:0:0:8:800:200C:417A:221");// unicast, full 
	TEST_IPv6_INVALID ("2001:db8:85a3::8a2e:37023:7334");
	TEST_IPv6_INVALID ("2001:db8:85a3::8a2e:370k:7334");
	TEST_IPv6_INVALID ("3ffe:0b00:0000:0001:0000:0000:000a");			// seven segments
	TEST_IPv6_INVALID ("3ffe:b00::1::a");								// double "::"
	TEST_IPv6_INVALID (":");
	TEST_IPv6_INVALID (":");
	TEST_IPv6_INVALID (":");
	TEST_IPv6_INVALID (":1111:2222:3333:4444:5555:6666:7777:8888");
	TEST_IPv6_INVALID (":1111:2222:3333:4444:5555:6666:7777::");
	TEST_IPv6_INVALID (":1111:2222:3333:4444:5555:6666::");
	TEST_IPv6_INVALID (":1111:2222:3333:4444:5555:6666::8888");
	TEST_IPv6_INVALID (":1111:2222:3333:4444:5555::");
	TEST_IPv6_INVALID (":1111:2222:3333:4444:5555::7777:8888");
	TEST_IPv6_INVALID (":1111:2222:3333:4444:5555::8888");
	TEST_IPv6_INVALID (":1111:2222:3333:4444::");
	TEST_IPv6_INVALID (":1111:2222:3333:4444::5555");
	TEST_IPv6_INVALID (":1111:2222:3333:4444::6666:7777:8888");
	TEST_IPv6_INVALID (":1111:2222:3333:4444::7777:8888");
	TEST_IPv6_INVALID (":1111:2222:3333:4444::8888");
	TEST_IPv6_INVALID (":1111:2222:3333::");
	TEST_IPv6_INVALID (":1111:2222:3333::5555");
	TEST_IPv6_INVALID (":1111:2222:3333::5555:6666:7777:8888");
	TEST_IPv6_INVALID (":1111:2222:3333::6666:7777:8888");
	TEST_IPv6_INVALID (":1111:2222:3333::7777:8888");
	TEST_IPv6_INVALID (":1111:2222:3333::8888");
	TEST_IPv6_INVALID (":1111:2222::");
	TEST_IPv6_INVALID (":1111:2222::4444:5555:6666:7777:8888");
	TEST_IPv6_INVALID (":1111:2222::5555");
	TEST_IPv6_INVALID (":1111:2222::5555:6666:7777:8888");
	TEST_IPv6_INVALID (":1111:2222::6666:7777:8888");
	TEST_IPv6_INVALID (":1111:2222::7777:8888");
	TEST_IPv6_INVALID (":1111:2222::8888");
	TEST_IPv6_INVALID (":1111::");
	TEST_IPv6_INVALID (":1111::3333:4444:5555:6666:7777:8888");
	TEST_IPv6_INVALID (":1111::4444:5555:6666:7777:8888");
	TEST_IPv6_INVALID (":1111::5555");
	TEST_IPv6_INVALID (":1111::5555:6666:7777:8888");
	TEST_IPv6_INVALID (":1111::6666:7777:8888");
	TEST_IPv6_INVALID (":1111::7777:8888");
	TEST_IPv6_INVALID (":1111::8888");
	TEST_IPv6_INVALID (":2222:3333:4444:5555:6666:7777:8888");
	TEST_IPv6_INVALID (":3333:4444:5555:6666:7777:8888");
	TEST_IPv6_INVALID (":4444:5555:6666:7777:8888");
	TEST_IPv6_INVALID (":5555:6666:7777:8888");
	TEST_IPv6_INVALID (":6666:7777:8888");
	TEST_IPv6_INVALID (":7777:8888");
	TEST_IPv6_INVALID (":8888");
	TEST_IPv6_INVALID ("::1111:2222:3333:4444:5555:6666::");			// double "::"
	TEST_IPv6_INVALID ("::2222:3333:4444:5555:6666:7777:8888:");
	TEST_IPv6_INVALID ("::2222:3333:4444:5555:6666:7777:8888:9999");
	TEST_IPv6_INVALID ("::2222:3333:4444:5555:7777:8888::");
	TEST_IPv6_INVALID ("::2222:3333:4444:5555:7777::8888");
	TEST_IPv6_INVALID ("::2222:3333:4444:5555::7777:8888");
	TEST_IPv6_INVALID ("::2222:3333:4444::6666:7777:8888");
	TEST_IPv6_INVALID ("::2222:3333::5555:6666:7777:8888");
	TEST_IPv6_INVALID ("::2222::4444:5555:6666:7777:8888");
	TEST_IPv6_INVALID ("::3333:4444:5555:6666:7777:8888:");
	TEST_IPv6_INVALID ("::4444:5555:6666:7777:8888:");
	TEST_IPv6_INVALID ("::5555:");
	TEST_IPv6_INVALID ("::5555:6666:7777:8888:");
	TEST_IPv6_INVALID ("::6666:7777:8888:");
	TEST_IPv6_INVALID ("::7777:8888:");
	TEST_IPv6_INVALID ("::8888:");
	TEST_IPv6_INVALID (":::");
	TEST_IPv6_INVALID (":::");
	TEST_IPv6_INVALID (":::");
	TEST_IPv6_INVALID (":::");
	TEST_IPv6_INVALID (":::2222:3333:4444:5555:6666:7777:8888");
	TEST_IPv6_INVALID (":::2222:3333:4444:5555:6666:7777:8888");
	TEST_IPv6_INVALID (":::3333:4444:5555:6666:7777:8888");
	TEST_IPv6_INVALID (":::4444:5555:6666:7777:8888");
	TEST_IPv6_INVALID (":::5555");
	TEST_IPv6_INVALID (":::5555:6666:7777:8888");
	TEST_IPv6_INVALID (":::6666:7777:8888");
	TEST_IPv6_INVALID (":::7777:8888");
	TEST_IPv6_INVALID (":::8888");
	TEST_IPv6_INVALID ("FF01::101::2");// multicast, compressed 
	TEST_IPv6_INVALID ("FF02:0000:0000:0000:0000:0000:0000:0000:0001");	// nine segments
	TEST_IPv6_INVALID ("XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX");
	TEST_IPv6_INVALID ("ldkfj");

	TEST_IPv4_MAP_VALID ("0:0:0:0:0:0:13.1.68.3");// IPv4-compatible IPv6 address, full, deprecated 
	TEST_IPv4_MAP_VALID ("0:0:0:0:0:FFFF:129.144.52.38");// IPv4-mapped IPv6 address, full 
	TEST_IPv4_MAP_VALID ("1111:2222:3333:4444:5555:6666:123.123.123.123");
	TEST_IPv4_MAP_VALID ("1111:2222:3333:4444:5555::123.123.123.123");
	TEST_IPv4_MAP_VALID ("1111:2222:3333:4444::123.123.123.123");
	TEST_IPv4_MAP_VALID ("1111:2222:3333:4444::6666:123.123.123.123");
	TEST_IPv4_MAP_VALID ("1111:2222:3333::123.123.123.123");
	TEST_IPv4_MAP_VALID ("1111:2222:3333::5555:6666:123.123.123.123");
	TEST_IPv4_MAP_VALID ("1111:2222:3333::6666:123.123.123.123");
	TEST_IPv4_MAP_VALID ("1111:2222::123.123.123.123");
	TEST_IPv4_MAP_VALID ("1111:2222::4444:5555:6666:123.123.123.123");
	TEST_IPv4_MAP_VALID ("1111:2222::5555:6666:123.123.123.123");
	TEST_IPv4_MAP_VALID ("1111:2222::6666:123.123.123.123");
	TEST_IPv4_MAP_VALID ("1111::123.123.123.123");
	TEST_IPv4_MAP_VALID ("1111::3333:4444:5555:6666:123.123.123.123");
	TEST_IPv4_MAP_VALID ("1111::4444:5555:6666:123.123.123.123");
	TEST_IPv4_MAP_VALID ("1111::5555:6666:123.123.123.123");
	TEST_IPv4_MAP_VALID ("1111::6666:123.123.123.123");
	TEST_IPv4_MAP_VALID ("1:2:3:4:5:6:1.2.3.4");
	TEST_IPv4_MAP_VALID ("1:2:3:4:5::1.2.3.4");
	TEST_IPv4_MAP_VALID ("1:2:3:4::1.2.3.4");
	TEST_IPv4_MAP_VALID ("1:2:3:4::5:1.2.3.4");
	TEST_IPv4_MAP_VALID ("1:2:3::1.2.3.4");
	TEST_IPv4_MAP_VALID ("1:2:3::5:1.2.3.4");
	TEST_IPv4_MAP_VALID ("1:2::1.2.3.4");
	TEST_IPv4_MAP_VALID ("1:2::5:1.2.3.4");
	TEST_IPv4_MAP_VALID ("1::1.2.3.4");
	TEST_IPv4_MAP_VALID ("1::5:1.2.3.4");
	TEST_IPv4_MAP_VALID ("1::5:11.22.33.44");
	TEST_IPv4_MAP_VALID ("::123.123.123.123");
	TEST_IPv4_MAP_VALID ("::13.1.68.3");// IPv4-compatible IPv6 address, compressed, deprecated 
	TEST_IPv4_MAP_VALID ("::2222:3333:4444:5555:6666:123.123.123.123");
	TEST_IPv4_MAP_VALID ("::4444:5555:6666:123.123.123.123");
	TEST_IPv4_MAP_VALID ("::5555:6666:123.123.123.123");
	TEST_IPv4_MAP_VALID ("::6666:123.123.123.123");
	TEST_IPv4_MAP_VALID ("::FFFF:129.144.52.38");// IPv4-mapped IPv6 address, compressed 
	TEST_IPv4_MAP_VALID ("::ffff:12.34.56.78");
	TEST_IPv4_MAP_VALID ("::ffff:192.0.2.128");   // but this is OK, since there's a single digit
	TEST_IPv4_MAP_VALID ("::ffff:192.168.1.1");
	TEST_IPv4_MAP_VALID ("::ffff:192.168.1.26");
	TEST_IPv4_MAP_VALID ("fe80:0:0:0:204:61ff:254.157.241.86");
	TEST_IPv4_MAP_VALID ("fe80::204:61ff:254.157.241.86");
	TEST_IPv4_MAP_VALID ("fe80::217:f2ff:254.7.237.98");

	TEST_IPv4_MAP_INVALID ("':10.0.0.1");
	TEST_IPv4_MAP_INVALID ("1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1.2.3.4:1111:2222:3333:4444::5555");  // Aeron
	TEST_IPv4_MAP_INVALID ("1.2.3.4:1111:2222:3333::5555");
	TEST_IPv4_MAP_INVALID ("1.2.3.4:1111:2222::5555");
	TEST_IPv4_MAP_INVALID ("1.2.3.4:1111::5555");
	TEST_IPv4_MAP_INVALID ("1.2.3.4::");
	TEST_IPv4_MAP_INVALID ("1.2.3.4::5555");
	TEST_IPv4_MAP_INVALID ("11112222:3333:4444:5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:22223333:4444:5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:2222:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:2222:33334444:5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:44445555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:4444:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:4444:55556666:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:4444:5555:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:4444:5555:66661.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:4444:5555:6666:00.00.00.00");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:4444:5555:6666:000.000.000.000");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:4444:5555:6666:1.2.3.4.5");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:4444:5555:6666:255.255.255255");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:4444:5555:6666:255.255255.255");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:4444:5555:6666:255255.255.255");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:4444:5555:6666:256.256.256.256");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:4444:5555:6666:7777:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:4444:5555:6666:7777:8888:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:4444:5555:6666::1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:4444:5555:::1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:4444:::6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333::5555::1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:2222:3333:::5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:2222::4444:5555::1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:2222::4444::6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:2222:::4444:5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111::3333:4444:5555::1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111::3333:4444::6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111::3333::5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1111:::3333:4444:5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("1::1.2.256.4");
	TEST_IPv4_MAP_INVALID ("1::1.2.3.256");
	TEST_IPv4_MAP_INVALID ("1::1.2.3.300");
	TEST_IPv4_MAP_INVALID ("1::1.2.3.900");
	TEST_IPv4_MAP_INVALID ("1::1.2.300.4");
	TEST_IPv4_MAP_INVALID ("1::1.2.900.4");
	TEST_IPv4_MAP_INVALID ("1::1.256.3.4");
	TEST_IPv4_MAP_INVALID ("1::1.300.3.4");
	TEST_IPv4_MAP_INVALID ("1::1.900.3.4");
	TEST_IPv4_MAP_INVALID ("1::256.2.3.4");
	TEST_IPv4_MAP_INVALID ("1::260.2.3.4");
	TEST_IPv4_MAP_INVALID ("1::300.2.3.4");
	TEST_IPv4_MAP_INVALID ("1::300.300.300.300");
	TEST_IPv4_MAP_INVALID ("1::3000.30.30.30");
	TEST_IPv4_MAP_INVALID ("1::400.2.3.4");
	TEST_IPv4_MAP_INVALID ("1::5:1.2.256.4");
	TEST_IPv4_MAP_INVALID ("1::5:1.2.3.256");
	TEST_IPv4_MAP_INVALID ("1::5:1.2.3.300");
	TEST_IPv4_MAP_INVALID ("1::5:1.2.3.900");
	TEST_IPv4_MAP_INVALID ("1::5:1.2.300.4");
	TEST_IPv4_MAP_INVALID ("1::5:1.2.900.4");
	TEST_IPv4_MAP_INVALID ("1::5:1.256.3.4");
	TEST_IPv4_MAP_INVALID ("1::5:1.300.3.4");
	TEST_IPv4_MAP_INVALID ("1::5:1.900.3.4");
	TEST_IPv4_MAP_INVALID ("1::5:256.2.3.4");
	TEST_IPv4_MAP_INVALID ("1::5:260.2.3.4");
	TEST_IPv4_MAP_INVALID ("1::5:300.2.3.4");
	TEST_IPv4_MAP_INVALID ("1::5:300.300.300.300");
	TEST_IPv4_MAP_INVALID ("1::5:3000.30.30.30");
	TEST_IPv4_MAP_INVALID ("1::5:400.2.3.4");
	TEST_IPv4_MAP_INVALID ("1::5:900.2.3.4");
	TEST_IPv4_MAP_INVALID ("1::900.2.3.4");
	TEST_IPv4_MAP_INVALID ("2001:1:1:1:1:1:255Z255X255Y255");				// garbage instead of "." in IPv4
	TEST_IPv4_MAP_INVALID (":1.2.3.4");
	TEST_IPv4_MAP_INVALID (":1111:2222:3333:4444:5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":1111:2222:3333:4444:5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":1111:2222:3333:4444:5555::1.2.3.4");
	TEST_IPv4_MAP_INVALID (":1111:2222:3333:4444::1.2.3.4");
	TEST_IPv4_MAP_INVALID (":1111:2222:3333:4444::6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":1111:2222:3333::1.2.3.4");
	TEST_IPv4_MAP_INVALID (":1111:2222:3333::5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":1111:2222:3333::6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":1111:2222::1.2.3.4");
	TEST_IPv4_MAP_INVALID (":1111:2222::4444:5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":1111:2222::5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":1111:2222::6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":1111::1.2.3.4");
	TEST_IPv4_MAP_INVALID (":1111::3333:4444:5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":1111::4444:5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":1111::5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":1111::6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":2222:3333:4444:5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":3333:4444:5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":4444:5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("::.");
	TEST_IPv4_MAP_INVALID ("::..");
	TEST_IPv4_MAP_INVALID ("::...");
	TEST_IPv4_MAP_INVALID ("::...4");
	TEST_IPv4_MAP_INVALID ("::..3.");
	TEST_IPv4_MAP_INVALID ("::..3.4");
	TEST_IPv4_MAP_INVALID ("::.2..");
	TEST_IPv4_MAP_INVALID ("::.2.3.");
	TEST_IPv4_MAP_INVALID ("::.2.3.4");
	TEST_IPv4_MAP_INVALID ("::1...");
	TEST_IPv4_MAP_INVALID ("::1.2..");
	TEST_IPv4_MAP_INVALID ("::1.2.256.4");
	TEST_IPv4_MAP_INVALID ("::1.2.3.");
	TEST_IPv4_MAP_INVALID ("::1.2.3.256");
	TEST_IPv4_MAP_INVALID ("::1.2.3.300");
	TEST_IPv4_MAP_INVALID ("::1.2.3.900");
	TEST_IPv4_MAP_INVALID ("::1.2.300.4");
	TEST_IPv4_MAP_INVALID ("::1.2.900.4");
	TEST_IPv4_MAP_INVALID ("::1.256.3.4");
	TEST_IPv4_MAP_INVALID ("::1.300.3.4");
	TEST_IPv4_MAP_INVALID ("::1.900.3.4");
	TEST_IPv4_MAP_INVALID ("::2222:3333:4444:5555:6666:7777:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("::2222:3333:4444:5555::1.2.3.4");
	TEST_IPv4_MAP_INVALID ("::2222:3333:4444::6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("::2222:3333::5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("::2222::4444:5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("::256.2.3.4");
	TEST_IPv4_MAP_INVALID ("::260.2.3.4");
	TEST_IPv4_MAP_INVALID ("::300.2.3.4");
	TEST_IPv4_MAP_INVALID ("::300.300.300.300");
	TEST_IPv4_MAP_INVALID ("::3000.30.30.30");
	TEST_IPv4_MAP_INVALID ("::400.2.3.4");
	TEST_IPv4_MAP_INVALID ("::900.2.3.4");
	TEST_IPv4_MAP_INVALID (":::1.2.3.4");
	TEST_IPv4_MAP_INVALID (":::2222:3333:4444:5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":::2222:3333:4444:5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":::4444:5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":::5555:6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID (":::6666:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("::ffff:192x168.1.26");							// ditto
	TEST_IPv4_MAP_INVALID ("::ffff:2.3.4");
	TEST_IPv4_MAP_INVALID ("::ffff:257.1.2.3");
	TEST_IPv4_MAP_INVALID ("XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:1.2.3.4");
	TEST_IPv4_MAP_INVALID ("fe80:0000:0000:0000:0204:61ff:254.157.241.086");
}

int
main (void)
{
	test_parse ();
	test_sub ();
	test_sub_no_fragment ();
	test_sub_no_query ();
	test_sub_scheme_rel ();
	test_sub_path_rel ();
	test_sub_query_rel ();
	test_sub_fragment_rel ();
	test_range ();
	test_range_no_fragment ();
	test_range_no_query ();
	test_range_scheme_rel ();
	test_range_path_rel ();
	test_range_query_rel ();
	test_range_fragment_rel ();
	test_join ();
	test_ipv6 ();

	mu_exit ("uri");
}

