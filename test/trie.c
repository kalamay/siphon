#include "../include/siphon/trie.h"
#include "../include/siphon/alloc.h"
#include "mu/mu.h"

static const SpType type = {
	.print = sp_print_str
};

static void
test_put1 (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);

	sp_trie_put (&trie, "abc", 3, "first");
	sp_trie_put (&trie, "abcd", 4, "second");
	sp_trie_put (&trie, "abcd\0", 5, "third");
	sp_trie_put (&trie, "abcd1", 5, "fourth");

	mu_assert_ptr_eq (sp_trie_get (&trie, "ab", 2), NULL);
	mu_assert_str_eq (sp_trie_get (&trie, "abc", 3), "first");
	mu_assert_str_eq (sp_trie_get (&trie, "abcd", 4), "second");
	mu_assert_str_eq (sp_trie_get (&trie, "abcd\0", 5), "third");
	mu_assert_str_eq (sp_trie_get (&trie, "abcd1", 5), "fourth");
	mu_assert_ptr_eq (sp_trie_get (&trie, "abcd~", 5), NULL);

	sp_trie_clear (&trie);
}

static void
test_put2 (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);

	sp_trie_put (&trie, "abcd1", 5, "first");
	sp_trie_put (&trie, "abcd\0", 5, "second");
	sp_trie_put (&trie, "abcd", 4, "third");
	sp_trie_put (&trie, "abc", 3, "fourth");

	mu_assert_ptr_eq (sp_trie_get (&trie, "ab", 2), NULL);
	mu_assert_str_eq (sp_trie_get (&trie, "abc", 3), "fourth");
	mu_assert_str_eq (sp_trie_get (&trie, "abcd", 4), "third");
	mu_assert_str_eq (sp_trie_get (&trie, "abcd\0", 5), "second");
	mu_assert_str_eq (sp_trie_get (&trie, "abcd1", 5), "first");
	mu_assert_ptr_eq (sp_trie_get (&trie, "abcd~", 5), NULL);

	sp_trie_clear (&trie);
}

static void
test_put3 (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);

	sp_trie_put (&trie, "abcdefghijklmnopqrstuvwxyz1", 27, "first");
	sp_trie_put (&trie, "abcdefghijklmnopqrstuvwxyz2", 27, "second");

	mu_assert_str_eq (sp_trie_get (&trie, "abcdefghijklmnopqrstuvwxyz1", 27), "first");
	mu_assert_str_eq (sp_trie_get (&trie, "abcdefghijklmnopqrstuvwxyz2", 27), "second");

	sp_trie_clear (&trie);
}

static void
test_put4 (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);

	char buf[] = "value*";
	for (uintptr_t i = 0; i < 256; i++) {
		buf[5] = i;
		sp_trie_put (&trie, buf, 6, (void *)i);
	}

	for (uintptr_t i = 0; i < 256; i++) {
		buf[5] = i;
		mu_assert_ptr_eq (sp_trie_get (&trie, buf, 6), (void *)i);
	}

	sp_trie_clear (&trie);
}

static void
test_put5 (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);

	char buf[] = "value*";
	for (intptr_t i = 255; i >= 0; i--) {
		buf[5] = i;
		sp_trie_put (&trie, buf, 6, (void *)i);
	}
	for (intptr_t i = 255; i >= 0; i--) {
		buf[5] = i;
		mu_assert_ptr_eq (sp_trie_get (&trie, buf, 6), (void *)i);
	}

	sp_trie_clear (&trie);
}

static void
test_replace (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);

	mu_assert_int_eq (sp_trie_put (&trie, "test", 4, "first"), 0);
	mu_assert_int_eq (sp_trie_put (&trie, "test", 4, "second"), 1);
	mu_assert_uint_eq (sp_trie_count (&trie), 1);

	mu_assert_int_eq (sp_trie_put (&trie, "te", 2, "start"), 0);
	mu_assert_int_eq (sp_trie_put (&trie, "test", 4, "third"), 1);
	mu_assert_uint_eq (sp_trie_count (&trie), 2);

	sp_trie_clear (&trie);
}

static void
test_get_prefix (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);

	sp_trie_put (&trie, "/test", 5, "first");
	sp_trie_put (&trie, "/test/other", 11, "second");
	sp_trie_put (&trie, "", 0, "root");

	size_t off;

	mu_assert_str_eq (sp_trie_prefix (&trie, "/test", 5, &off, '/'), "first");
	mu_assert_uint_eq (off, 5);

	mu_assert_str_eq (sp_trie_prefix (&trie, "/test/value", 11, &off, '/'), "first");
	mu_assert_uint_eq (off, 5);

	mu_assert_str_eq (sp_trie_prefix (&trie, "/test/other", 11, &off, '/'), "second");
	mu_assert_uint_eq (off, 11);

	mu_assert_str_eq (sp_trie_prefix (&trie, "/test/other/stuff", 17, &off, '/'), "second");
	mu_assert_uint_eq (off, 11);

	mu_assert_str_eq (sp_trie_prefix (&trie, "/", 1, &off, '/'), "root");
	mu_assert_uint_eq (off, 0);

	mu_assert_str_eq (sp_trie_prefix (&trie, "/other", 6, &off, '/'), "root");
	mu_assert_uint_eq (off, 0);

	sp_trie_clear (&trie);
}

static void
test_get_prefix_no_root (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);

	sp_trie_put (&trie, "/test", 5, "first");
	sp_trie_put (&trie, "/test/other", 11, "second");

	size_t off;

	mu_assert_str_eq (sp_trie_prefix (&trie, "/test", 5, &off, '/'), "first");
	mu_assert_uint_eq (off, 5);

	mu_assert_str_eq (sp_trie_prefix (&trie, "/test/", 6, &off, '/'), "first");
	mu_assert_uint_eq (off, 5);

	mu_assert_str_eq (sp_trie_prefix (&trie, "/test/value", 11, &off, '/'), "first");
	mu_assert_uint_eq (off, 5);

	mu_assert_str_eq (sp_trie_prefix (&trie, "/test/other", 11, &off, '/'), "second");
	mu_assert_uint_eq (off, 11);

	mu_assert_str_eq (sp_trie_prefix (&trie, "/test/other/stuff", 17, &off, '/'), "second");
	mu_assert_uint_eq (off, 11);

	mu_assert_ptr_eq (sp_trie_prefix (&trie, "/", 1, NULL, '/'), NULL);
	mu_assert_ptr_eq (sp_trie_prefix (&trie, "/other", 6, NULL, '/'), NULL);

	sp_trie_clear (&trie);
}

static void
test_del1 (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);
	
	sp_trie_put (&trie, "test", 4, "first");
	sp_trie_put (&trie, "value", 5, "second");

	mu_assert_str_eq (sp_trie_get (&trie, "test", 4), "first");
	mu_assert_str_eq (sp_trie_get (&trie, "value", 5), "second");
	mu_assert_uint_eq (sp_trie_count (&trie), 2);

	mu_assert_int_eq (sp_trie_del (&trie, "test", 4), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "test", 4), NULL);
	mu_assert_str_eq (sp_trie_get (&trie, "value", 5), "second");
	mu_assert_uint_eq (sp_trie_count (&trie), 1);

	mu_assert_int_eq (sp_trie_del (&trie, "value", 5), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "test", 4), NULL);
	mu_assert_ptr_eq (sp_trie_get (&trie, "value", 5), NULL);
	mu_assert_uint_eq (sp_trie_count (&trie), 0);

	mu_assert_int_eq (sp_trie_del (&trie, "test", 4), false);
	mu_assert_ptr_eq (sp_trie_get (&trie, "value", 5), NULL);

	sp_trie_clear (&trie);
}

static void
test_del2 (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);
	
	sp_trie_put (&trie, "abc", 3, "first");
	sp_trie_put (&trie, "abcdef", 6, "second");
	sp_trie_put (&trie, "abcdefghi", 9, "third");

	mu_assert_uint_eq (sp_trie_count (&trie), 3);

	mu_assert_int_eq (sp_trie_del (&trie, "abc", 3), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "abc", 3), NULL);
	mu_assert_str_eq (sp_trie_get (&trie, "abcdef", 6), "second");
	mu_assert_str_eq (sp_trie_get (&trie, "abcdefghi", 9), "third");
	mu_assert_uint_eq (sp_trie_count (&trie), 2);

	mu_assert_int_eq (sp_trie_del (&trie, "abcdef", 6), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "abcdef", 6), NULL);
	mu_assert_str_eq (sp_trie_get (&trie, "abcdefghi", 9), "third");
	mu_assert_uint_eq (sp_trie_count (&trie), 1);

	mu_assert_int_eq (sp_trie_del (&trie, "abcdefghi", 9), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "abc", 3), NULL);
	mu_assert_uint_eq (sp_trie_count (&trie), 0);

	sp_trie_clear (&trie);
}

static void
test_del3 (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);
	
	sp_trie_put (&trie, "abcdefghi", 9, "first");
	sp_trie_put (&trie, "abcdef", 6, "second");
	sp_trie_put (&trie, "abc", 3, "third");

	mu_assert_uint_eq (sp_trie_count (&trie), 3);

	mu_assert_int_eq (sp_trie_del (&trie, "abcdefghi", 9), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "abcdefghi", 9), NULL);
	mu_assert_str_eq (sp_trie_get (&trie, "abcdef", 6), "second");
	mu_assert_str_eq (sp_trie_get (&trie, "abc", 3), "third");
	mu_assert_uint_eq (sp_trie_count (&trie), 2);

	mu_assert_int_eq (sp_trie_del (&trie, "abcdef", 6), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "abcdef", 6), NULL);
	mu_assert_str_eq (sp_trie_get (&trie, "abc", 3), "third");
	mu_assert_uint_eq (sp_trie_count (&trie), 1);

	mu_assert_int_eq (sp_trie_del (&trie, "abc", 3), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "abc", 3), NULL);
	mu_assert_uint_eq (sp_trie_count (&trie), 0);

	sp_trie_clear (&trie);
}

static void
test_del4 (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);
	
	sp_trie_put (&trie, "abc", 3, "first");
	sp_trie_put (&trie, "abcd", 4, "second");
	sp_trie_put (&trie, "abcde", 5, "third");

	mu_assert_uint_eq (sp_trie_count (&trie), 3);

	mu_assert_int_eq (sp_trie_del (&trie, "abc", 3), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "abc", 3), NULL);
	mu_assert_str_eq (sp_trie_get (&trie, "abcd", 4), "second");
	mu_assert_str_eq (sp_trie_get (&trie, "abcde", 5), "third");
	mu_assert_uint_eq (sp_trie_count (&trie), 2);

	mu_assert_int_eq (sp_trie_del (&trie, "abcd", 4), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "abcd", 4), NULL);
	mu_assert_str_eq (sp_trie_get (&trie, "abcde", 5), "third");
	mu_assert_uint_eq (sp_trie_count (&trie), 1);

	mu_assert_int_eq (sp_trie_del (&trie, "abcde", 5), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "abc", 3), NULL);
	mu_assert_uint_eq (sp_trie_count (&trie), 0);

	sp_trie_clear (&trie);
}

static void
test_del5 (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);
	
	sp_trie_put (&trie, "abcde", 5, "first");
	sp_trie_put (&trie, "abcd", 4, "second");
	sp_trie_put (&trie, "abc", 3, "third");

	mu_assert_uint_eq (sp_trie_count (&trie), 3);

	mu_assert_int_eq (sp_trie_del (&trie, "abcde", 5), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "abcde", 5), NULL);
	mu_assert_str_eq (sp_trie_get (&trie, "abcd", 4), "second");
	mu_assert_str_eq (sp_trie_get (&trie, "abc", 3), "third");
	mu_assert_uint_eq (sp_trie_count (&trie), 2);

	mu_assert_int_eq (sp_trie_del (&trie, "abcd", 4), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "abcd", 4), NULL);
	mu_assert_str_eq (sp_trie_get (&trie, "abc", 3), "third");
	mu_assert_uint_eq (sp_trie_count (&trie), 1);

	mu_assert_int_eq (sp_trie_del (&trie, "abc", 3), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "abc", 3), NULL);
	mu_assert_uint_eq (sp_trie_count (&trie), 0);

	sp_trie_clear (&trie);
}

static void
test_del6 (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);
	
	sp_trie_put (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaa", 25, "first");
	sp_trie_put (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbb", 50, "second");
	sp_trie_put (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccc", 75, "third");

	mu_assert_uint_eq (sp_trie_count (&trie), 3);

	mu_assert_int_eq (sp_trie_del (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaa", 25), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaa", 25), NULL);
	mu_assert_str_eq (sp_trie_get (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbb", 50), "second");
	mu_assert_str_eq (sp_trie_get (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccc", 75), "third");
	mu_assert_uint_eq (sp_trie_count (&trie), 2);

	mu_assert_int_eq (sp_trie_del (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbb", 50), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbb", 50), NULL);
	mu_assert_str_eq (sp_trie_get (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccc", 75), "third");
	mu_assert_uint_eq (sp_trie_count (&trie), 1);

	mu_assert_int_eq (sp_trie_del (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccc", 75), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaa", 25), NULL);
	mu_assert_uint_eq (sp_trie_count (&trie), 0);

	sp_trie_clear (&trie);
}

static void
test_del7 (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);
	
	sp_trie_put (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccc", 75, "first");
	sp_trie_put (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbb", 50, "second");
	sp_trie_put (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaa", 25, "third");

	mu_assert_uint_eq (sp_trie_count (&trie), 3);

	mu_assert_int_eq (sp_trie_del (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccc", 75), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbccccccccccccccccccccccccc", 75), NULL);
	mu_assert_str_eq (sp_trie_get (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbb", 50), "second");
	mu_assert_str_eq (sp_trie_get (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaa", 25), "third");
	mu_assert_uint_eq (sp_trie_count (&trie), 2);

	mu_assert_int_eq (sp_trie_del (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbb", 50), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbb", 50), NULL);
	mu_assert_str_eq (sp_trie_get (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaa", 25), "third");
	mu_assert_uint_eq (sp_trie_count (&trie), 1);

	mu_assert_int_eq (sp_trie_del (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaa", 25), true);
	mu_assert_ptr_eq (sp_trie_get (&trie, "aaaaaaaaaaaaaaaaaaaaaaaaa", 25), NULL);
	mu_assert_uint_eq (sp_trie_count (&trie), 0);

	sp_trie_clear (&trie);
}

typedef struct {
	int id, count;
} Item;

static void
test_destroy_callback (void *val)
{
	Item *item = val;
	item->count++;
}

static void
test_clear (void)
{
	static Item items[5];

	SpType test = type;
	test.free = test_destroy_callback;

	SpTrie trie = SP_TRIE_MAKE (&test);
	for (size_t i = 0; i < sp_len (items); i++) {
		items[i].id = i;
		items[i].count = 0;
		sp_trie_put (&trie, &i, sizeof i, &items[i]);
	}

	sp_trie_clear (&trie);

	for (size_t i = 0; i < sp_len (items); i++) {
		mu_assert_int_eq (items[i].count, 1);
	}
}

static int large_count = 0;

static void
test_large_callback (void *val)
{
	large_count++;
	free (val);
}

static void
test_large (void)
{
	SpType test = type;
	test.free = test_large_callback;

	SpTrie trie = SP_TRIE_MAKE (&test);

	for (int i = 0; i < 1 << 16; i++) {
		char buf[32];
		int len = snprintf (buf, sizeof buf, "item %d", i);
		int rc = sp_trie_put (&trie, buf, len, strndup (buf, len));
		mu_assert_int_eq (rc, 0);
	}

	for (int i = 0; i < 1 << 16; i++) {
		char buf[32];
		int len = snprintf (buf, sizeof buf, "item %d", i);
		void *val = sp_trie_get (&trie, buf, len);
		mu_assert_ptr_ne (val, NULL);
		mu_assert_ptr_ne (val, NULL);
		mu_assert_str_eq (val, buf);
	}

	sp_trie_clear (&trie);
	mu_assert_int_eq (large_count, 1 << 16);
}

static bool
test_each_callback (const void *key, size_t len, void *val, void *data)
{
	(void)key;
	(void)len;
	(void)val;
	int *count = data;
	*count = *count + 1;
	return *count < 10;
}

static void
test_each (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);

	char buf[] = "value*";
	for (uintptr_t i = 0; i < 15; i++) {
		buf[5] = i;
		sp_trie_put (&trie, buf, 6, (void *)i);
	}

	int count = 0;
	sp_trie_each (&trie, test_each_callback, &count);
	mu_assert_int_eq (count, 10);

	sp_trie_clear (&trie);
}


static bool
test_each_prefix_callback (const void *key, size_t len, void *val, void *data)
{
	(void)key;
	(void)len;
	bool *flags = data;
	flags[(uintptr_t)val] = true;
	return true;
}

static void
test_each_prefix (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);

	const char *keys[] = {
		"aardvark",
		"dog",
		"doge",
		"dogfish",
		"doglike",
		"zebra"
	};

	bool flags[sp_len (keys)];

	for (size_t i = 0; i < sp_len (keys); i++) {
		sp_trie_put (&trie, keys[i], strlen (keys[i]), (void *)i);
	}

	memset (flags, 0, sizeof flags);
	sp_trie_each_prefix (&trie, "do", 2, test_each_prefix_callback, flags);
	mu_assert (!flags[0]);
	mu_assert (flags[1]);
	mu_assert (flags[2]);
	mu_assert (flags[3]);
	mu_assert (flags[4]);
	mu_assert (!flags[5]);

	memset (flags, 0, sizeof flags);
	sp_trie_each_prefix (&trie, "dog", 3, test_each_prefix_callback, flags);
	mu_assert (!flags[0]);
	mu_assert (flags[1]);
	mu_assert (flags[2]);
	mu_assert (flags[3]);
	mu_assert (flags[4]);
	mu_assert (!flags[5]);

	memset (flags, 0, sizeof flags);
	sp_trie_each_prefix (&trie, "dogs", 4, test_each_prefix_callback, flags);
	mu_assert (!flags[0]);
	mu_assert (!flags[1]);
	mu_assert (!flags[2]);
	mu_assert (!flags[3]);
	mu_assert (!flags[4]);
	mu_assert (!flags[5]);

	sp_trie_clear (&trie);
}

static void
test_each_prefix_leaf (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);

	const char *keys[] = {
		"aardvark",
		"dog",
		"zebra"
	};

	bool flags[sp_len (keys)];

	for (size_t i = 0; i < sp_len (keys); i++) {
		sp_trie_put (&trie, keys[i], strlen (keys[i]), (void *)i);
	}

	memset (flags, 0, sizeof flags);
	sp_trie_each_prefix (&trie, "do", 2, test_each_prefix_callback, flags);
	mu_assert (!flags[0]);
	mu_assert (flags[1]);
	mu_assert (!flags[2]);

	memset (flags, 0, sizeof flags);
	sp_trie_each_prefix (&trie, "dog", 3, test_each_prefix_callback, flags);
	mu_assert (!flags[0]);
	mu_assert (flags[1]);
	mu_assert (!flags[2]);

	memset (flags, 0, sizeof flags);
	sp_trie_each_prefix (&trie, "dogs", 4, test_each_prefix_callback, flags);
	mu_assert (!flags[0]);
	mu_assert (!flags[1]);
	mu_assert (!flags[2]);

	sp_trie_clear (&trie);
}

#define TEST_PREFIX(t, key, expect, remain) do {                          \
	size_t off = 0;                                                           \
	const char *val = sp_trie_prefix (t, key, sizeof key - 1, &off, '/'); \
	if (expect == NULL) {                                                 \
		mu_assert_ptr_eq (val, NULL);                                     \
	}                                                                     \
	else {                                                                \
		mu_assert_str_eq (val, expect);                                   \
	}                                                                     \
	char buf[64];                                                         \
	memcpy (buf, (const char *)key+off, sizeof key - off);                \
	mu_assert_str_eq (buf, remain);                                       \
} while (0)

static void
test_prefix (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);

	sp_trie_put (&trie, "/abc", 4, "first");
	sp_trie_put (&trie, "/abc/def", 8, "second");

	TEST_PREFIX (&trie, "/ab", NULL, "/ab");
	TEST_PREFIX (&trie, "/abc", "first", "");
	TEST_PREFIX (&trie, "/abc/", "first", "/");
	TEST_PREFIX (&trie, "/abc/de", "first", "/de");
	TEST_PREFIX (&trie, "/abc/def", "second", "");
	TEST_PREFIX (&trie, "/abc/def/", "second", "/");
	TEST_PREFIX (&trie, "/abc/def/ghi", "second", "/ghi");

	sp_trie_final (&trie);
}

typedef struct {
	const char *match;
} MatchData;

static bool
matcher (const void *key, size_t len, size_t off, void *val, void *data)
{
	(void)val;

	// only match full key or at '/' boundary
	if (off < len && ((char *)key)[off] != '/') {
		return false;
	}

	MatchData *md = data;
	md->match = (const char *)key + off;
	return true;
}

#define TEST_MATCH(t, key, expect, remain) do {                               \
	MatchData data = { key };                                                 \
	const char *val = sp_trie_match (t, key, sizeof key - 1, matcher, &data); \
	if (expect == NULL) {                                                     \
		mu_assert_ptr_eq (val, NULL);                                         \
	}                                                                         \
	else {                                                                    \
		mu_assert_str_eq (val, expect);                                       \
	}                                                                         \
	if (remain == NULL) {                                                     \
		mu_assert_ptr_eq (data.match, NULL);                                  \
	}                                                                         \
	else {                                                                    \
		mu_assert_str_eq (data.match, remain);                                \
	}                                                                         \
} while (0)

static void
test_match (void)
{
	SpTrie trie = SP_TRIE_MAKE (&type);

	sp_trie_put (&trie, "/abc", 4, "first");
	sp_trie_put (&trie, "/abc/def", 8, "second");

	TEST_MATCH (&trie, "/ab", NULL, "/ab");
	TEST_MATCH (&trie, "/abc", "first", "");
	TEST_MATCH (&trie, "/abc/", "first", "/");
	TEST_MATCH (&trie, "/abc/de", "first", "/de");
	TEST_MATCH (&trie, "/abc/def", "second", "");
	TEST_MATCH (&trie, "/abc/def/", "second", "/");
	TEST_MATCH (&trie, "/abc/def/ghi", "second", "/ghi");

	sp_trie_final (&trie);
}


int
main (void)
{
	mu_init ("trie");

	test_put1 ();
	test_put2 ();
	test_put3 ();
	test_put4 ();
	test_put5 ();
	test_replace ();
	test_get_prefix ();
	test_get_prefix_no_root ();
	test_del1 ();
	test_del2 ();
	test_del3 ();
	test_del4 ();
	test_del5 ();
	test_del6 ();
	test_del7 ();
	test_clear ();
	test_large ();
	test_each ();
	test_each_prefix ();
	test_each_prefix_leaf ();
	test_prefix ();
	test_match ();

	mu_assert (sp_alloc_summary ());
}

