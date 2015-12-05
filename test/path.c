#include "../include/siphon/path.h"
#include "../include/siphon/error.h"
#include "mu/mu.h"

#include <errno.h>

#define TEST_JOIN(base, rel, exp) do {                               \
	char buf[1024];                                                  \
	int rc = sp_path_join (buf, sizeof buf,                          \
			base, (sizeof base) - 1,                                 \
			rel, (sizeof rel) - 1,                                   \
			0);                                                      \
	mu_assert_int_gt (rc, 0);                                        \
	mu_assert_str_eq (buf, exp);                                     \
} while (0)

#define TEST_CLEAN(path, exp) do {                                   \
	char buf[4096] = { 0 };                                          \
	memcpy (buf, path, sizeof path);                                 \
	uint16_t rc = sp_path_clean (buf, sizeof path - 1, 0);           \
	mu_assert_uint_gt (rc, 0);                                       \
	mu_assert_str_eq (buf, exp);                                     \
} while (0)

#define TEST_CLEAN_URI(path, exp) do {                               \
	char buf[4096] = { 0 };                                          \
	memcpy (buf, path, sizeof path);                                 \
	uint16_t rc = sp_path_clean (buf, sizeof path - 1, SP_PATH_URI); \
	mu_assert_uint_gt (rc, 0);                                       \
	mu_assert_str_eq (buf, exp);                                     \
} while (0)

#define TEST_POP(path, cnt, exp) do {                                \
	char buf[4096] = { 0 };                                          \
	SpRange16 popped = { 0, sizeof path - 1 };                       \
	sp_path_pop (path, &popped, cnt);                                \
	memcpy (buf, (const char *)path + popped.off, popped.len);       \
	buf[popped.len] = '\0';                                          \
	mu_assert_str_eq (buf, exp);                                     \
} while (0)

#define TEST_SPLIT(path, cnt, a, b) do {                             \
	SpRange16 ra, rb;                                                \
	char buf[4096];                                                  \
	sp_path_split (&ra, &rb, path, strlen (path), cnt);              \
	memcpy (buf, (const char *)path + ra.off, ra.len);               \
	buf[ra.len] = '\0';                                              \
	mu_assert_str_eq (buf, a);                                       \
	memcpy (buf, (const char *)path + rb.off, rb.len);               \
	buf[rb.len] = '\0';                                              \
	mu_assert_str_eq (buf, b);                                       \
} while (0)

#define TEST_SPLITEXT(path, a, b) do {                               \
	SpRange16 ra, rb;                                                \
	char buf[4096];                                                  \
	sp_path_splitext (&ra, &rb, path, strlen (path));                \
	memcpy (buf, (const char *)path + ra.off, ra.len);               \
	buf[ra.len] = '\0';                                              \
	mu_assert_str_eq (buf, a);                                       \
	memcpy (buf, (const char *)path + rb.off, rb.len);               \
	buf[rb.len] = '\0';                                              \
	mu_assert_str_eq (buf, b);                                       \
} while (0)

#define TEST_MATCH_VALID(a, b) do {                                  \
	mu_assert_msg (sp_path_match (a, b),                             \
			"Assertion '%s == %s' failed", a, b);                    \
} while (0)

#define TEST_MATCH_INVALID(a, b) do {                                \
	mu_assert_msg (!sp_path_match (a, b),                            \
			"Assertion '%s != %s' failed", a, b);                    \
} while (0)


static void
test_join (void)
{
	TEST_JOIN ("/some/path/to/", "../up1.txt", "/some/path/to/../up1.txt");
	TEST_JOIN ("/some/path/to/", "../../up2.txt", "/some/path/to/../../up2.txt");
	TEST_JOIN ("/some/path/to/", "/root.txt", "/root.txt");
	TEST_JOIN ("/some/path/to/", "./current.txt", "/some/path/to/./current.txt");
	TEST_JOIN ("", "file.txt", "file.txt");
	TEST_JOIN ("some", "file.txt", "some/file.txt");
	TEST_JOIN ("some/", "../file.txt", "some/../file.txt");
	TEST_JOIN ("/", "file.txt", "/file.txt");
	TEST_JOIN ("/test", "", "/test");

	char buf[16];
	char base[] = "/some/longer/named/path.txt";
	char rel[] = "../../longothernamedfile.txt";
	int rc = sp_path_join (buf, sizeof buf,
			base, (sizeof base) - 1,
			rel, (sizeof rel) - 1,
			0);
	mu_assert_int_eq (rc, SP_PATH_EBUFS);
}

static void
test_clean (void)
{
	TEST_CLEAN ("/some/path/../other/file.txt", "/some/other/file.txt");
	TEST_CLEAN ("/some/path/../../other/file.txt", "/other/file.txt");
	TEST_CLEAN ("/some/path/../../../other/file.txt", "/other/file.txt");
	TEST_CLEAN ("../file.txt", "../file.txt");
	TEST_CLEAN ("../../file.txt", "../../file.txt");
	TEST_CLEAN ("/../file.txt", "/file.txt");
	TEST_CLEAN ("/../../file.txt", "/file.txt");
	TEST_CLEAN ("/some/./file.txt", "/some/file.txt");
	TEST_CLEAN ("/some/././file.txt", "/some/file.txt");
	TEST_CLEAN ("//some/file.txt", "/some/file.txt");
	TEST_CLEAN ("/some//file.txt", "/some/file.txt");
	TEST_CLEAN ("/a/b/c/./../../g", "/a/g");
	TEST_CLEAN (".", ".");
	TEST_CLEAN ("/", "/");
	TEST_CLEAN ("", ".");
	TEST_CLEAN ("//", "/");

	TEST_CLEAN_URI ("/a/b/c/", "/a/b/c/");
}

static void
test_pop (void)
{
	TEST_POP ("/path/to/file.txt", 1, "/path/to");
	TEST_POP ("/path/to/file.txt", 2, "/path");
	TEST_POP ("/path/to/file.txt", 3, "/");
	TEST_POP ("/path/to/file.txt", 4, "");
	TEST_POP ("path/to/file.txt", 1, "path/to");
	TEST_POP ("path/to/file.txt", 2, "path");
	TEST_POP ("path/to/file.txt", 3, "");
}

static void
test_split (void)
{
	TEST_SPLIT ("/path/to/file.txt", 1, "/path/to", "file.txt");
	TEST_SPLIT ("/path/to/file.txt", 2, "/path", "to/file.txt");
	TEST_SPLIT ("/path/to/file.txt", 3, "/", "path/to/file.txt");
	TEST_SPLIT ("/path/to/file.txt", 4, "", "/path/to/file.txt");
	TEST_SPLIT ("/path/to/file.txt", 5, "", "/path/to/file.txt");
	TEST_SPLIT ("/test/path/", 1, "/test/path", "");
	TEST_SPLIT ("/test/path/", 2, "/test", "path/");
	TEST_SPLIT ("/path/to/file.txt", -1, "/", "path/to/file.txt");
	TEST_SPLIT ("/path/to/file.txt", -2, "/path", "to/file.txt");
	TEST_SPLIT ("/path/to/file.txt", -3, "/path/to", "file.txt");
	TEST_SPLIT ("/path/to/file.txt", -4, "/path/to/file.txt", "");
	TEST_SPLIT ("/path/to/file.txt", -5, "/path/to/file.txt", "");
	TEST_SPLIT ("path/to/file.txt", -1, "path", "to/file.txt");
	TEST_SPLIT ("path/to/file.txt", -2, "path/to", "file.txt");
	TEST_SPLIT ("path/to/file.txt", -3, "path/to/file.txt", "");
	TEST_SPLIT ("path/to/file.txt", -4, "path/to/file.txt", "");
	TEST_SPLIT ("/test/path", -1, "/", "test/path");
	TEST_SPLIT ("/test/path", -2, "/test", "path");
	TEST_SPLIT ("/test/path", -3, "/test/path", "");
	TEST_SPLIT ("/test/path", -4, "/test/path", "");
	TEST_SPLIT ("/test/path/", -1, "/", "test/path/");
	TEST_SPLIT ("/test/path/", -2, "/test", "path/");
	TEST_SPLIT ("/test/path/", -3, "/test/path", "");
	TEST_SPLIT ("/test/path/", -4, "/test/path", "");
	TEST_SPLIT ("/test", 1, "/", "test");
	TEST_SPLIT ("/test", 2, "", "/test");
	TEST_SPLIT ("/test", -1, "/", "test");
	TEST_SPLIT ("/test", -2, "/test", "");
	TEST_SPLIT ("/", 1, "/", "");
	TEST_SPLIT ("/", 2, "", "/");
	TEST_SPLIT ("/", -1, "", "/");
	TEST_SPLIT ("/", -2, "/", "");
	TEST_SPLIT ("test", 1, "", "test");
	TEST_SPLIT ("test", 2, "", "test");
	TEST_SPLIT ("test", -1, "test", "");
	TEST_SPLIT ("test", -2, "test", "");
	TEST_SPLIT ("", 1, "", "");
	TEST_SPLIT ("", 2, "", "");
	TEST_SPLIT ("", -1, "", "");
	TEST_SPLIT ("", -2, "", "");
}

static void
test_splitext (void)
{
	TEST_SPLITEXT ("file.txt", "file", "txt");
	TEST_SPLITEXT ("/path/to/file.txt", "/path/to/file", "txt");
}

static void
test_match (void)
{
	TEST_MATCH_VALID ("test.c", "test.{cpp,c}");
	TEST_MATCH_VALID ("test.cpp", "test.{c,cpp}");
	TEST_MATCH_VALID ("test-abc.c", "test-abc.*");
	TEST_MATCH_VALID ("test-abc.c", "test-*.c");
	TEST_MATCH_VALID ("test-abc.c", "test-*.*");
	TEST_MATCH_VALID ("test-abc.c", "test-*.[ch]");
	TEST_MATCH_VALID ("test-abc.cpp", "test-*.[ch]pp");
	TEST_MATCH_VALID ("test-abc.c", "{test,value}-{abc,xyz}.{c,h}");
	TEST_MATCH_VALID ("value-xyz.h", "{test,value}-{abc,xyz}.{c,h}");

	TEST_MATCH_INVALID ("test.c", "test.cpp");
	TEST_MATCH_INVALID ("test.cpp", "test.{c,o}");
	TEST_MATCH_INVALID ("test.c", "test.{cpp,o}");
	TEST_MATCH_INVALID ("test.c", "test*");
}

static void
test_proc (void)
{
	char buf[SP_PATH_MAX] = {0};
	int len = sp_path_proc (buf, sizeof buf);
	printf ("DEBUG: process path '%s'\n", buf);
	mu_assert_int_gt (len, 0);
}

static void
test_env (void)
{
	char buf[SP_PATH_MAX] = {0};
	int len = sp_path_env ("vi", buf, sizeof buf);
	printf ("DEBUG: vi path '%s'\n", buf);
	mu_assert_int_gt (len, 0);
}

static void
test_dir_zero (void)
{
	const char *p = PROJECT_SOURCE_DIR "/test";

	SpDir dir;
	int rc = sp_dir_open (&dir, p, 0);
	mu_fassert_int_eq (rc, 0);
	int n = 0;

	do {
		rc = sp_dir_next (&dir);
		if (rc > 0) { n++; }
	} while (rc > 0);

	mu_assert_int_eq (n, 0);
	sp_dir_close (&dir);
}

static void
test_dir_single (void)
{
	const char *p = PROJECT_SOURCE_DIR "/test/dir";

	SpDir dir;
	int rc = sp_dir_open (&dir, p, 1);
	mu_fassert_int_eq (rc, 0);
	int n = 0;

	do {
		rc = sp_dir_next (&dir);
		if (rc > 0) {
			mu_assert_uint_eq (strlen (dir.path), dir.pathlen);
			if (dir.pathlen < 9 || memcmp (dir.path+dir.pathlen-9, ".DS_Store", 9)) {
				n++;
			}
		}
	} while (rc > 0);

	mu_assert_int_eq (n, 2);
	sp_dir_close (&dir);
}

static void
test_dir_tree (void)
{
	const char *p = PROJECT_SOURCE_DIR "/test/dir";

	SpDir dir;
	int rc = sp_dir_open (&dir, p, 16);
	mu_fassert_int_eq (rc, 0);
	int n = 0;

	do {
		rc = sp_dir_next (&dir);
		if (rc > 0) {
			mu_assert_uint_eq (strlen (dir.path), dir.pathlen);
			if (dir.pathlen < 9 || memcmp (dir.path+dir.pathlen-9, ".DS_Store", 9)) {
				n++;
			}
		}
	} while (rc > 0);

	mu_assert_int_eq (n, 12);

	sp_dir_close (&dir);
}


int
main (void)
{
	mu_init ("path");

	test_join ();
	test_clean ();
	test_split ();
	test_splitext ();
	test_pop ();
	test_match ();
	test_proc ();
	test_env ();
	test_dir_zero ();
	test_dir_single ();
	test_dir_tree ();
}

