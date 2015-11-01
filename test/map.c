#include "../include/siphon/map.h"
#include "mu/mu.h"

/*
 * All these tests assume this utterly terrible hash function
 * is used so that collisions can be easily predicted.
 * It should go without saying, but NEVER USE THIS HASH FUNCTION.
 */
static uint64_t
junk_hash (const void *restrict key, size_t len, const SpSeed *restrict seed)
{
	(void)len;
	(void)seed;
	return *(const char *)key;
}

static bool
key_equals (const void *restrict val, const void *restrict key, size_t len)
{
	return strncmp (val, key, len) == 0;
}

SpMapType junk_type = {
	.hash = junk_hash,
	.equals = key_equals
};

SpMapType good_type = {
	.hash = sp_metrohash64,
	.equals = key_equals
};

#define TEST_ADD_NEW(map, key, count) do {             \
	int rc = sp_map_put (map, key, strlen (key), key); \
	mu_assert_int_eq (rc, 0);                          \
	mu_assert_uint_eq (sp_map_count (map), count);     \
} while (0)

#define TEST_ADD_OLD(map, key, count) do {             \
	int rc = sp_map_put (map, key, strlen (key), key); \
	mu_assert_int_eq (rc, 1);                          \
	mu_assert_uint_eq (sp_map_count (map), count);     \
} while (0)

#define TEST_REM_NEW(map, key, count) do {         \
	bool rc = sp_map_del (map, key, strlen (key)); \
	mu_assert_int_eq (rc, false);                  \
	mu_assert_uint_eq (sp_map_count (map), count); \
} while (0)

#define TEST_REM_OLD(map, key, count) do {         \
	bool rc = sp_map_del (map, key, strlen (key)); \
	mu_assert_int_eq (rc, true);                   \
	mu_assert_uint_eq (sp_map_count (map), count); \
} while (0)

static void
test_empty (void)
{
	SpMap map = SP_MAP_MAKE (&junk_type);
	mu_assert_ptr_eq (sp_map_get (&map, "test", 4), NULL);
}


static void
test_single_collision (void)
{
	SpMap map = SP_MAP_MAKE (&junk_type);

	TEST_ADD_NEW (&map, "a1", 1);
	TEST_ADD_NEW (&map, "a2", 2);
	TEST_REM_OLD (&map, "a1", 1);
	TEST_REM_OLD (&map, "a2", 0);

	TEST_ADD_NEW (&map, "a1", 1);
	TEST_ADD_NEW (&map, "a2", 2);
	TEST_REM_OLD (&map, "a2", 1);
	TEST_REM_OLD (&map, "a1", 0);

	sp_map_final (&map);
}

static void
test_single_bucket_collision (void)
{
	SpMap map = SP_MAP_MAKE (&junk_type);

	TEST_ADD_NEW (&map, "a", 1);
	TEST_ADD_NEW (&map, "q", 2);
	TEST_REM_OLD (&map, "a", 1);
	TEST_REM_OLD (&map, "q", 0);

	TEST_ADD_NEW (&map, "a", 1);
	TEST_ADD_NEW (&map, "q", 2);
	TEST_REM_OLD (&map, "q", 1);
	TEST_REM_OLD (&map, "a", 0);

	sp_map_final (&map);
}

static void
test_single_collision_wrap (void)
{
	SpMap map = SP_MAP_MAKE (&junk_type);

	// o1 and o2 will result in a hash collision

	TEST_ADD_NEW (&map, "o1", 1);
	TEST_ADD_NEW (&map, "o2", 2);
	TEST_REM_OLD (&map, "o1", 1);
	TEST_REM_OLD (&map, "o2", 0);

	TEST_ADD_NEW (&map, "o1", 1);
	TEST_ADD_NEW (&map, "o2", 2);
	TEST_REM_OLD (&map, "o2", 1);
	TEST_REM_OLD (&map, "o1", 0);

	sp_map_final (&map);
}

static void
test_bucket_collision_steal (void)
{
	SpMap map = SP_MAP_MAKE (&junk_type);

	// a and q will collide in bucket positions
	// b will sit after a

	TEST_ADD_NEW (&map, "b", 1);
	mu_assert_str_eq (map.entries[2].value, "b");
	TEST_ADD_NEW (&map, "q", 2);
	mu_assert_str_eq (map.entries[1].value, "q");
	mu_assert_str_eq (map.entries[2].value, "b");
	TEST_ADD_NEW (&map, "a", 3);
	mu_assert_str_eq (map.entries[1].value, "q");
	mu_assert_str_eq (map.entries[2].value, "a");
	mu_assert_str_eq (map.entries[3].value, "b");

	TEST_REM_OLD (&map, "a", 2);
	mu_assert_str_eq (map.entries[1].value, "q");
	mu_assert_str_eq (map.entries[2].value, "b");
	TEST_REM_OLD (&map, "q", 1);
	mu_assert_str_eq (map.entries[2].value, "b");
	TEST_REM_OLD (&map, "b", 0);

	sp_map_final (&map);
}

static void
test_hash_to_bucket_collision (void)
{
	SpMap map = SP_MAP_MAKE (&junk_type);

	// p1 and p2 should hash collide
	// p2 should bucket collide with a

	TEST_ADD_NEW (&map, "a", 1);
	TEST_ADD_NEW (&map, "b", 2);
	TEST_ADD_NEW (&map, "c", 3);
	mu_assert_str_eq (map.entries[1].value, "a");
	mu_assert_str_eq (map.entries[2].value, "b");
	mu_assert_str_eq (map.entries[3].value, "c");
	TEST_ADD_NEW (&map, "p1", 4);
	TEST_ADD_NEW (&map, "p2", 5);
	mu_assert_str_eq (map.entries[0].value, "p1");
	mu_assert_str_eq (map.entries[1].value, "p2");
	mu_assert_str_eq (map.entries[2].value, "a");
	mu_assert_str_eq (map.entries[3].value, "b");
	mu_assert_str_eq (map.entries[4].value, "c");

	TEST_REM_OLD (&map, "p2", 4);
	TEST_REM_OLD (&map, "p1", 3);
	mu_assert_str_eq (map.entries[1].value, "a");
	mu_assert_str_eq (map.entries[2].value, "b");
	mu_assert_str_eq (map.entries[3].value, "c");

	sp_map_final (&map);
}

static void
test_hash_to_bucket_collision2 (void)
{
	SpMap map = SP_MAP_MAKE (&junk_type);

	// p1, p2, p3, and p4 should hash collide
	// p2+ should bucket collide with a+

	TEST_ADD_NEW (&map, "p1", 1);
	TEST_ADD_NEW (&map, "p2", 2);
	TEST_ADD_NEW (&map, "a", 3);
	TEST_ADD_NEW (&map, "b", 4);
	TEST_ADD_NEW (&map, "c", 5);

	mu_assert_str_eq (map.entries[0].value, "p1");
	mu_assert_str_eq (map.entries[1].value, "p2");
	mu_assert_str_eq (map.entries[2].value, "a");
	mu_assert_str_eq (map.entries[3].value, "b");
	mu_assert_str_eq (map.entries[4].value, "c");

	TEST_ADD_NEW (&map, "p3", 6);
	TEST_ADD_NEW (&map, "p4", 7);

	mu_assert_str_eq (map.entries[0].value, "p1");
	mu_assert_str_eq (map.entries[1].value, "p2");
	mu_assert_str_eq (map.entries[2].value, "p3");
	mu_assert_str_eq (map.entries[3].value, "p4");
	mu_assert_str_eq (map.entries[4].value, "a");
	mu_assert_str_eq (map.entries[5].value, "b");
	mu_assert_str_eq (map.entries[6].value, "c");

	TEST_REM_OLD (&map, "p2", 6);
	TEST_REM_OLD (&map, "p4", 5);

	mu_assert_str_eq (map.entries[0].value, "p1");
	mu_assert_str_eq (map.entries[1].value, "p3");
	mu_assert_str_eq (map.entries[2].value, "a");
	mu_assert_str_eq (map.entries[3].value, "b");
	mu_assert_str_eq (map.entries[4].value, "c");

	TEST_REM_OLD (&map, "p1", 4);
	TEST_REM_OLD (&map, "p3", 3);

	mu_assert_str_eq (map.entries[1].value, "a");
	mu_assert_str_eq (map.entries[2].value, "b");
	mu_assert_str_eq (map.entries[3].value, "c");

	sp_map_final (&map);
}

static void
test_lookup_collision (void)
{
	SpMap map = SP_MAP_MAKE (&junk_type);

	TEST_ADD_NEW (&map, "a1", 1);
	mu_assert_ptr_eq (sp_map_get (&map, "a2", 2), NULL);

	sp_map_final (&map);
}

static void
test_downsize (void)
{
	SpMap map = SP_MAP_MAKE (&good_type);

	TEST_ADD_NEW (&map, "k1", 1);
	TEST_ADD_NEW (&map, "k2", 2);
	TEST_ADD_NEW (&map, "k3", 3);
	TEST_ADD_NEW (&map, "k4", 4);

	sp_map_resize (&map, sp_map_count (&map));
	mu_assert_uint_eq (sp_map_count (&map), 4);
	mu_assert_uint_eq (sp_map_capacity (&map), 16);
	mu_assert_str_eq (sp_map_get (&map, "k1", 2), "k1");
	mu_assert_str_eq (sp_map_get (&map, "k2", 2), "k2");
	mu_assert_str_eq (sp_map_get (&map, "k3", 2), "k3");
	mu_assert_str_eq (sp_map_get (&map, "k4", 2), "k4");
	sp_map_final (&map);
}

static void
test_large (void)
{
	SpMapType type = good_type;
	type.copy = (SpCopy)strdup;
	type.free = free;

	SpMap map = SP_MAP_MAKE (&type);

	for (int i = 0; i < 1 << 16; i++) {
		char buf[32];
		int len = snprintf (buf, sizeof buf, "item %d", i);
		int rc = sp_map_put (&map, buf, len, buf);
		mu_assert_int_eq (rc, 0);
	}

	for (int i = 0; i < 1 << 16; i++) {
		char buf[32];
		int len = snprintf (buf, sizeof buf, "item %d", i);
		bool rc = sp_map_del (&map, buf, len);
		mu_assert_int_eq (rc, true);
	}

	sp_map_final (&map);
}

static int copy_count = 0;
static int free_count = 0;

static void *
test_copy (void *val)
{
	copy_count++;
	return strdup (val);
}

static void
test_free (void *val)
{
	free_count++;
	free (val);
}

static void
test_copy_free (void)
{
	SpMapType type = good_type;
	type.copy = test_copy;
	type.free = test_free;

	SpMap map = SP_MAP_MAKE (&type);

	for (int i = 0; i < 1 << 8; i++) {
		char buf[32];
		int len = snprintf (buf, sizeof buf, "item %d", i);
		int rc = sp_map_put (&map, buf, len, buf);
		mu_assert_int_eq (rc, 0);
	}

	sp_map_final (&map);

	mu_assert_int_eq (copy_count, 1 << 8);
	mu_assert_int_eq (free_count, 1 << 8);
}

static void
test_each (void)
{
	SpMap map = SP_MAP_MAKE (&good_type);

	for (uintptr_t i = 0; i < 100; i++) {
		char buf[32];
		int len = snprintf (buf, sizeof buf, "item %lu", i);
		int rc = sp_map_put (&map, buf, len, (void *)i);
		mu_assert_int_eq (rc, 0);
	}
	mu_assert_uint_eq (sp_map_count (&map), 99);

	const SpMapEntry *entry;
	uintptr_t total = 0;
	int i = 0;
	sp_map_each (&map, entry) {
		total += (uintptr_t)entry->value;
		if (i++ > 10000) {
			break;
		}
	}
	mu_assert_int_eq (total, 4950);

	sp_map_final (&map);
}

static void
test_bloom (void)
{
	SpMapType type = good_type;
	type.copy = (SpCopy)strdup;
	type.free = free;

	SpMap map = SP_MAP_MAKE (&type);
	sp_map_use_bloom (&map, 1 << 16, 0.01);

	for (int i = 0; i < 1 << 16; i++) {
		char buf[32];
		int len = snprintf (buf, sizeof buf, "item %d", i);
		int rc = sp_map_put (&map, buf, len, buf);
		mu_assert_int_eq (rc, 0);
	}

	for (int i = 0; i < 1 << 16; i++) {
		char buf[32];
		int len = snprintf (buf, sizeof buf, "item %d", i);
		void *val = sp_map_get (&map, buf, len);
		mu_assert_str_eq (val, buf);
		
		uint64_t h = sp_map_hash (&map, buf, len);
		mu_assert_int_eq (sp_bloom_maybe_hash (map.bloom, h), true);
	}

	int false_pos = 0;

	for (int i = 0; i < 1 << 16; i++) {
		char buf[32];
		int len = snprintf (buf, sizeof buf, "other %d", i);
		void *val = sp_map_get (&map, buf, len);
		mu_assert_ptr_eq (val, NULL);
		
		uint64_t h = sp_map_hash (&map, buf, len);
		if (sp_bloom_maybe_hash (map.bloom, h)) {
			false_pos++;
		}
	}
	mu_assert_int_gt (false_pos, 0);

	sp_map_final (&map);
}

int
main (void)
{
	mu_init ("map");

	test_empty ();
	test_single_collision ();
	test_single_bucket_collision ();
	test_single_collision_wrap ();
	test_bucket_collision_steal ();
	test_hash_to_bucket_collision ();
	test_hash_to_bucket_collision2 ();
	test_lookup_collision ();
	test_downsize ();
	test_large ();
	test_copy_free ();
	test_each ();
	test_bloom ();
}

