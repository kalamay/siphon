#include "../include/siphon/hash/map.h"
#include "mu.h"



uint64_t
junk_hash (const char *key)
{
	return (uint64_t)*key + 1;
}

bool
junk_has_key (const char **junk, const char *key, size_t kn)
{
	(void)kn;
	return strcmp (*junk, key) == 0;
}

typedef SP_HMAP (const char *, 2, junk) JunkMap;

SP_HMAP_VALUE_PROTOTYPE_STATIC (JunkMap, const char *, const char *, junk)
SP_HMAP_VALUE_GENERATE (JunkMap, const char *, const char *, junk)

#define TEST_ADD_NEW(map, key, resultcount) do {       \
	const char *k = key;                               \
	int rc = junk_put (map, k, &k);                    \
	mu_assert_int_eq (rc, 0);                          \
	mu_assert_uint_eq ((map)->count, resultcount);     \
} while (0)

#define TEST_ADD_OLD(map, key, resultcount) do {       \
	const char *k = key;                               \
	int rc = junk_put (map, k, &k);                    \
	mu_assert_int_eq (rc, 1);                          \
	mu_assert_str_eq (k, key);                         \
	mu_assert_uint_eq ((map)->count, resultcount);     \
} while (0)

#define TEST_REM_NEW(map, key, resultcount) do {       \
	const char *old = NULL;                            \
	bool rc = junk_del (map, key, &old);               \
	mu_assert_int_eq (rc, false);                      \
	mu_assert_ptr_eq (old, NULL);                      \
	mu_assert_uint_eq ((map)->count, resultcount);     \
} while (0)

#define TEST_REM_OLD(map, key, resultcount) do {       \
	const char *old = NULL;                            \
	bool rc = junk_del (map, key, &old);               \
	mu_assert_int_eq (rc, true);                       \
	mu_assert_str_eq (old, key);                       \
	mu_assert_uint_eq ((map)->count, resultcount);     \
} while (0)

static void
test_empty (void)
{
	JunkMap map;
	junk_init (&map, 0.8, 0);
	mu_assert_ptr_eq (junk_get (&map, "test"), NULL);
}

static void
test_single_collision (void)
{
	JunkMap map;
	junk_init (&map, 0.8, 0);

	TEST_ADD_NEW (&map, "a1", 1);
	TEST_ADD_NEW (&map, "a2", 2);
	TEST_REM_OLD (&map, "a1", 1);
	TEST_REM_OLD (&map, "a2", 0);

	TEST_ADD_NEW (&map, "a1", 1);
	TEST_ADD_NEW (&map, "a2", 2);
	TEST_REM_OLD (&map, "a2", 1);
	TEST_REM_OLD (&map, "a1", 0);

	junk_final (&map);
}

static void
test_single_bucket_collision (void)
{
	JunkMap map;
	junk_init (&map, 0.8, 0);

	TEST_ADD_NEW (&map, "a", 1);
	TEST_ADD_NEW (&map, "q", 2);
	TEST_REM_OLD (&map, "a", 1);
	TEST_REM_OLD (&map, "q", 0);

	TEST_ADD_NEW (&map, "a", 1);
	TEST_ADD_NEW (&map, "q", 2);
	TEST_REM_OLD (&map, "q", 1);
	TEST_REM_OLD (&map, "a", 0);

	junk_final (&map);
}

static void
test_single_collision_wrap (void)
{
	JunkMap map;
	junk_init (&map, 0.8, 0);

	// o1 and o2 will result in a hash collision

	TEST_ADD_NEW (&map, "o1", 1);
	TEST_ADD_NEW (&map, "o2", 2);
	TEST_REM_OLD (&map, "o1", 1);
	TEST_REM_OLD (&map, "o2", 0);

	TEST_ADD_NEW (&map, "o1", 1);
	TEST_ADD_NEW (&map, "o2", 2);
	TEST_REM_OLD (&map, "o2", 1);
	TEST_REM_OLD (&map, "o1", 0);

	junk_final (&map);
}

static void
test_bucket_collision_steal (void)
{
	JunkMap map;
	junk_init (&map, 0.8, 0);

	// c and j will collide in bucket positions
	// d will sit after c

	TEST_ADD_NEW (&map, "d", 1);
	mu_assert_str_eq (map.tiers[0]->arr[3].entry, "d");
	TEST_ADD_NEW (&map, "j", 2);
	mu_assert_str_eq (map.tiers[0]->arr[2].entry, "j");
	mu_assert_str_eq (map.tiers[0]->arr[3].entry, "d");
	TEST_ADD_NEW (&map, "c", 3);
	mu_assert_str_eq (map.tiers[0]->arr[2].entry, "j");
	mu_assert_str_eq (map.tiers[0]->arr[3].entry, "c");
	mu_assert_str_eq (map.tiers[0]->arr[4].entry, "d");

	TEST_REM_OLD (&map, "c", 2);
	mu_assert_str_eq (map.tiers[0]->arr[2].entry, "j");
	mu_assert_str_eq (map.tiers[0]->arr[3].entry, "d");
	TEST_REM_OLD (&map, "j", 1);
	mu_assert_str_eq (map.tiers[0]->arr[3].entry, "d");
	TEST_REM_OLD (&map, "d", 0);

	junk_final (&map);
}

static void
test_hash_to_bucket_collision (void)
{
	JunkMap map;
	junk_init (&map, 0.8, 0);

	// a1 and a2 should hash collide
	// a2 should bucket collide with b

	TEST_ADD_NEW (&map, "b", 1);
	TEST_ADD_NEW (&map, "c", 2);
	TEST_ADD_NEW (&map, "d", 3);
	mu_assert_str_eq (map.tiers[0]->arr[1].entry, "b");
	mu_assert_str_eq (map.tiers[0]->arr[2].entry, "c");
	mu_assert_str_eq (map.tiers[0]->arr[3].entry, "d");
	TEST_ADD_NEW (&map, "a1", 4);
	TEST_ADD_NEW (&map, "a2", 5);
	mu_assert_str_eq (map.tiers[0]->arr[0].entry, "a1");
	mu_assert_str_eq (map.tiers[0]->arr[1].entry, "a2");
	mu_assert_str_eq (map.tiers[0]->arr[2].entry, "b");
	mu_assert_str_eq (map.tiers[0]->arr[3].entry, "c");
	mu_assert_str_eq (map.tiers[0]->arr[4].entry, "d");

	TEST_REM_OLD (&map, "a2", 4);
	TEST_REM_OLD (&map, "a1", 3);
	mu_assert_str_eq (map.tiers[0]->arr[1].entry, "b");
	mu_assert_str_eq (map.tiers[0]->arr[2].entry, "c");
	mu_assert_str_eq (map.tiers[0]->arr[3].entry, "d");

	junk_final (&map);
}

static void
test_hash_to_bucket_collision2 (void)
{
	JunkMap map;
	junk_init (&map, 0.95, 0);

	// a1, a2, a3, and a4 should hash collide
	// a2+ should bucket collide with c+

	TEST_ADD_NEW (&map, "a1", 1);
	TEST_ADD_NEW (&map, "a2", 2);
	TEST_ADD_NEW (&map, "b", 3);
	TEST_ADD_NEW (&map, "c", 4);
	TEST_ADD_NEW (&map, "d", 5);

	mu_assert_str_eq (map.tiers[0]->arr[0].entry, "a1");
	mu_assert_str_eq (map.tiers[0]->arr[1].entry, "a2");
	mu_assert_str_eq (map.tiers[0]->arr[2].entry, "b");
	mu_assert_str_eq (map.tiers[0]->arr[3].entry, "c");
	mu_assert_str_eq (map.tiers[0]->arr[4].entry, "d");

	TEST_ADD_NEW (&map, "a3", 6);
	TEST_ADD_NEW (&map, "a4", 7);

	mu_assert_str_eq (map.tiers[0]->arr[0].entry, "a1");
	mu_assert_str_eq (map.tiers[0]->arr[1].entry, "a2");
	mu_assert_str_eq (map.tiers[0]->arr[2].entry, "a3");
	mu_assert_str_eq (map.tiers[0]->arr[3].entry, "a4");
	mu_assert_str_eq (map.tiers[0]->arr[4].entry, "b");
	mu_assert_str_eq (map.tiers[0]->arr[5].entry, "c");
	mu_assert_str_eq (map.tiers[0]->arr[6].entry, "d");

	TEST_REM_OLD (&map, "a2", 6);
	TEST_REM_OLD (&map, "a4", 5);

	mu_assert_str_eq (map.tiers[0]->arr[0].entry, "a1");
	mu_assert_str_eq (map.tiers[0]->arr[1].entry, "a3");
	mu_assert_str_eq (map.tiers[0]->arr[2].entry, "b");
	mu_assert_str_eq (map.tiers[0]->arr[3].entry, "c");
	mu_assert_str_eq (map.tiers[0]->arr[4].entry, "d");

	TEST_REM_OLD (&map, "a1", 4);
	TEST_REM_OLD (&map, "a3", 3);

	mu_assert_str_eq (map.tiers[0]->arr[1].entry, "b");
	mu_assert_str_eq (map.tiers[0]->arr[2].entry, "c");
	mu_assert_str_eq (map.tiers[0]->arr[3].entry, "d");

	junk_final (&map);
}

static void
test_lookup_collision (void)
{
	JunkMap map;
	junk_init (&map, 0.8, 0);

	TEST_ADD_NEW (&map, "a1", 1);
	mu_assert_ptr_eq (junk_get (&map, "a2"), NULL);

	junk_final (&map);
}



typedef struct {
	int key, value;
} Thing;

bool
thing_has_key (Thing *t, int key, size_t len)
{
	(void)len;
	return t->key == key;
}

typedef SP_HMAP (Thing, 2, thing) ThingMap;

SP_HMAP_DIRECT_PROTOTYPE_STATIC (ThingMap, int, Thing, thing)
SP_HMAP_DIRECT_GENERATE (ThingMap, int, Thing, thing)

static void
test_set (void)
{
	Thing *e;
	bool isnew;
	ThingMap map;
	thing_init (&map, 0.8, 10);

	e = thing_reserve (&map, 10, &isnew);
	mu_fassert_ptr_ne (e, NULL);
	mu_assert (isnew);
	mu_assert_uint_eq (map.count, 1);
	e->key = 10;
	e->value = 123;

	e = thing_reserve (&map, 20, &isnew);
	mu_fassert_ptr_ne (e, NULL);
	mu_assert (isnew);
	mu_assert_uint_eq (map.count, 2);
	e->key = 20;
	e->value = 456;

	e = thing_reserve (&map, 10, &isnew);
	mu_fassert_ptr_ne (e, NULL);
	mu_assert (!isnew);
	mu_assert_uint_eq (map.count, 2);
	e->value = 789;

	e = thing_get (&map, 10);
	mu_assert_ptr_ne (e, NULL);
	if (e != NULL) {
		mu_assert_int_eq (e->value, 789);
	}

	e = thing_get (&map, 20);
	mu_assert_ptr_ne (e, NULL);
	if (e != NULL) {
		mu_assert_int_eq (e->value, 456);
	}
}

static void
test_resize (void)
{
	Thing *e;
	bool isnew;
	ThingMap map;
	thing_init (&map, 0.8, 10);

	e = thing_reserve (&map, 10, &isnew);
	mu_fassert_ptr_ne (e, NULL);
	e->key = 10;
	e->value = 123;

	e = thing_get (&map, 10);
	mu_assert_ptr_ne (e, NULL);
	if (e != NULL) {
		mu_assert_int_eq (e->value, 123);
	}
}

static void
test_grow (void)
{
	ThingMap map;

	thing_init (&map, 0.8, 10);

	srand (0);
	for (int i = 0; i < 100; i++) {
		int key = rand ();
		int val = rand ();
		bool isnew;
		Thing *t = thing_reserve (&map, key, &isnew);
		mu_fassert_ptr_ne (t, NULL);
		t->key = key;
		t->value = val;
	}

	mu_assert_uint_eq (map.count, 100);
	mu_assert_uint_eq (thing_condense (&map, 20), 20);
	mu_assert_uint_eq (map.count, 100);

	srand (0);
	for (int i = 0; i < 100; i++) {
		int key = rand ();
		int val = rand ();
		Thing *t = thing_get (&map, key);
		mu_assert_ptr_ne (t, NULL);
		if (t != NULL) {
			mu_assert_int_eq (t->value, val);
		}
	}

	srand (0);
	for (int i = 0; i < 100; i++) {
		int key = rand ();
		int val = rand ();
		if (i % 2 == 0) {
			Thing t;
			bool removed = thing_del (&map, key, &t);
			mu_assert (removed);
			if (removed) {
				mu_assert_int_eq (t.value, val);
			}
		}
	}

	mu_assert_uint_eq (map.count, 50);
	mu_assert_uint_eq (thing_condense (&map, 20), 0);
	mu_assert_uint_eq (map.count, 50);

	srand (0);
	for (int i = 0; i < 100; i++) {
		int key = rand ();
		int val = rand ();
		Thing *t = thing_get (&map, key);
		if (i % 2 == 1) {
			mu_assert_ptr_ne (t, NULL);
			if (t != NULL) {
				mu_assert_int_eq (t->value, val);
			}
		}
	}
}

#if 0
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
	mu_assert_uint_eq (sp_map_size (&map), 8);
	mu_assert_str_eq (sp_map_get (&map, "k1", 2), "k1");
	mu_assert_str_eq (sp_map_get (&map, "k2", 2), "k2");
	mu_assert_str_eq (sp_map_get (&map, "k3", 2), "k3");
	mu_assert_str_eq (sp_map_get (&map, "k4", 2), "k4");
	sp_map_final (&map);
}
#endif

static void
test_each (void)
{
	ThingMap map1, map2;
	Thing *t;

	thing_init (&map1, 0.8, 10);
	thing_init (&map2, 0.8, 10);

	srand (0);
	for (int i = 0; i < 100; i++) {
		int key = rand ();
		int val = rand ();
		bool isnew;
		t = thing_reserve (&map1, key, &isnew);
		mu_fassert_ptr_ne (t, NULL);
		t->key = key;
		t->value = val;
	}

	mu_assert_uint_eq (map1.count, 100);

	SP_HMAP_EACH (&map1, t) {
		int rc = thing_put (&map2, t->key, t);
		mu_assert_int_eq (rc, 0);
	}

	mu_assert_uint_eq (map2.count, 100);

	srand (0);
	for (int i = 0; i < 100; i++) {
		int key = rand ();
		int val = rand ();
		t = thing_get (&map2, key);
		mu_assert_ptr_ne (t, NULL);
		if (t != NULL) {
			mu_assert_int_eq (t->value, val);
		}
	}
}

static void
test_remove (void)
{
	Thing *t;
	ThingMap map;
	thing_init (&map, 0.8, 10);

	thing_put (&map, 1, &((Thing) { 1, 100 }));
	thing_put (&map, 2, &((Thing) { 2, 200 }));
	thing_put (&map, 3, &((Thing) { 3, 300 }));

	mu_assert_uint_eq (map.count, 3);

	t = thing_get (&map, 2);
	mu_fassert_ptr_ne (t, NULL);
	mu_assert_int_eq (t->value, 200);

	mu_assert (thing_remove (&map, t));

	mu_assert_uint_eq (map.count, 2);
	t = thing_get (&map, 2);
	mu_fassert_ptr_eq (t, NULL);
}

static void
test_remove_all (void)
{
	Thing *t;
	ThingMap map;
	thing_init (&map, 0.8, 10);

	srand (0);
	for (int i = 0; i < 1000; i++) {
		int key = rand ();
		int val = rand ();
		thing_put (&map, key, &((Thing) { key, val }));
	}

	mu_assert_uint_eq (map.count, 1000);

	srand (0);
	for (int i = 0; i < 1000; i++) {
		int key = rand ();
		int val = rand ();
		t = thing_get (&map, key);
		mu_fassert_ptr_ne (t, NULL);
		mu_assert_int_eq (t->value, val);
		mu_assert (thing_remove (&map, t));
	}

	mu_fassert_uint_eq (map.count, 0);

	for (int i = 0; i < 1000; i++) {
		thing_put (&map, i, &((Thing) { i, i * 100 }));
	}

	mu_assert_uint_eq (map.count, 1000);

	for (int i = 0; i < 1000; i++) {
		t = thing_get (&map, i);
		mu_fassert_ptr_ne (t, NULL);
		mu_assert_int_eq (t->value, i * 100);
		mu_assert (thing_remove (&map, t));
	}

	mu_fassert_uint_eq (map.count, 0);
}

uint64_t
prehash_hash (uint64_t key, size_t len)
{
	(void)len;

	// We are using siphash as the only identity 😳
	return key;
}

bool
prehash_has_key (int *v, uint64_t key, size_t len)
{
	(void)v;
	(void)key;
	(void)len;

	// This can just return `true` because the map won't call this function
	// unless the hashes are equal;
	return true;
}

typedef SP_HMAP (int, 2, prehash) PrehashMap;

SP_HMAP_PROTOTYPE_STATIC (PrehashMap, uint64_t, int, prehash)
SP_HMAP_GENERATE (PrehashMap, uint64_t, int, prehash)

static uint64_t
make_key (int n)
{
	char key[16];
	int len = snprintf (key, sizeof key, "key%u", n);
	return sp_siphash (key, len, SP_SEED_DEFAULT);
}

static void
test_pre_hash (size_t hint)
{
	PrehashMap map;
	prehash_init (&map, 0.85, hint);

#define CREATE(n) do {                               \
	uint64_t key = make_key (n);                     \
	bool new;                                        \
	int *v = prehash_reserve (&map, key, 0, &new);   \
	mu_assert_ptr_ne (v, NULL);                      \
	mu_assert (new);                                 \
	if (v != NULL) {                                 \
		*v = n;                                      \
	}                                                \
} while (0)

#define REMOVE(n) do {                               \
	uint64_t key = make_key (n);                     \
	mu_assert (prehash_del (&map, key, 0, NULL));    \
} while (0)

#define CHECK(n) do {                                \
	uint64_t key = make_key (n);                     \
	int *v = prehash_get (&map, key, 0);             \
	mu_assert_ptr_ne (v, NULL);                      \
	if (v != NULL) {                                 \
		mu_assert_int_eq (*v, n);                    \
	}                                                \
} while (0)

#define NONE(n) do {                                 \
	uint64_t key = make_key (n);                     \
	int *v = prehash_get (&map, key, 0);             \
	mu_assert_ptr_eq (v, NULL);                      \
	mu_assert (!prehash_has (&map, key, 0));         \
} while (0)

	CREATE (1);
	CREATE (2);
	CREATE (3);
	CREATE (4);
	CREATE (5);
	CREATE (6);
	CREATE (7);
	CREATE (8);
	CREATE (9);
	CREATE (10);
	CREATE (11);
	CREATE (12);
	CREATE (13);
	CREATE (14);

	CHECK (1);
	CHECK (2);
	CHECK (3);
	CHECK (4);
	CHECK (5);
	CHECK (6);
	CHECK (7);
	CHECK (8);
	CHECK (9);
	CHECK (10);
	CHECK (11);
	CHECK (12);
	CHECK (13);
	CHECK (14);
	NONE (15);
	NONE (16);
	NONE (17);
	NONE (18);
	NONE (19);
	NONE (20);
	NONE (21);
	NONE (22);
	NONE (23);
	NONE (24);
	NONE (25);
	NONE (26);
	NONE (27);

	REMOVE (1);
	REMOVE (2);
	REMOVE (3);
	REMOVE (4);
	REMOVE (5);
	CREATE (15);

	NONE (1);
	NONE (2);
	NONE (3);
	NONE (4);
	NONE (5);
	CHECK (6);
	CHECK (7);
	CHECK (8);
	CHECK (9);
	CHECK (10);
	CHECK (11);
	CHECK (12);
	CHECK (13);
	CHECK (14);
	CHECK (15);
	NONE (16);
	NONE (17);
	NONE (18);
	NONE (19);
	NONE (20);
	NONE (21);
	NONE (22);
	NONE (23);
	NONE (24);
	NONE (25);
	NONE (26);
	NONE (27);

	CREATE (16);

	NONE (1);
	NONE (2);
	NONE (3);
	NONE (4);
	NONE (5);
	CHECK (6);
	CHECK (7);
	CHECK (8);
	CHECK (9);
	CHECK (10);
	CHECK (11);
	CHECK (12);
	CHECK (13);
	CHECK (14);
	CHECK (15);
	CHECK (16);
	NONE (17);
	NONE (18);
	NONE (19);
	NONE (20);
	NONE (21);
	NONE (22);
	NONE (23);
	NONE (24);
	NONE (25);
	NONE (26);
	NONE (27);

	REMOVE (6);
	REMOVE (7);
	REMOVE (8);
	REMOVE (9);
	CREATE (17);

	NONE (1);
	NONE (2);
	NONE (3);
	NONE (4);
	NONE (5);
	NONE (6);
	NONE (7);
	NONE (8);
	NONE (9);
	CHECK (10);
	CHECK (11);
	CHECK (12);
	CHECK (13);
	CHECK (14);
	CHECK (15);
	CHECK (16);
	CHECK (17);
	NONE (18);
	NONE (19);
	NONE (20);
	NONE (21);
	NONE (22);
	NONE (23);
	NONE (24);
	NONE (25);
	NONE (26);
	NONE (27);

	CREATE (18);

	NONE (1);
	NONE (2);
	NONE (3);
	NONE (4);
	NONE (5);
	NONE (6);
	NONE (7);
	NONE (8);
	NONE (9);
	CHECK (10);
	CHECK (11);
	CHECK (12);
	CHECK (13);
	CHECK (14);
	CHECK (15);
	CHECK (16);
	CHECK (17);
	CHECK (18);
	NONE (19);
	NONE (20);
	NONE (21);
	NONE (22);
	NONE (23);
	NONE (24);
	NONE (25);
	NONE (26);
	NONE (27);

	REMOVE (10);
	REMOVE (11);
	REMOVE (12);
	REMOVE (13);
	CREATE (19);

	NONE (1);
	NONE (2);
	NONE (3);
	NONE (4);
	NONE (5);
	NONE (6);
	NONE (7);
	NONE (8);
	NONE (9);
	NONE (10);
	NONE (11);
	NONE (12);
	NONE (13);
	CHECK (14);
	CHECK (15);
	CHECK (16);
	CHECK (17);
	CHECK (18);
	CHECK (19);
	NONE (20);
	NONE (21);
	NONE (22);
	NONE (23);
	NONE (24);
	NONE (25);
	NONE (26);
	NONE (27);

	CREATE (20);

	NONE (1);
	NONE (2);
	NONE (3);
	NONE (4);
	NONE (5);
	NONE (6);
	NONE (7);
	NONE (8);
	NONE (9);
	NONE (10);
	NONE (11);
	NONE (12);
	NONE (13);
	CHECK (14);
	CHECK (15);
	CHECK (16);
	CHECK (17);
	CHECK (18);
	CHECK (19);
	CHECK (20);
	NONE (21);
	NONE (22);
	NONE (23);
	NONE (24);
	NONE (25);
	NONE (26);
	NONE (27);

	REMOVE (14);
	REMOVE (15);
	CREATE (21);

	NONE (1);
	NONE (2);
	NONE (3);
	NONE (4);
	NONE (5);
	NONE (6);
	NONE (7);
	NONE (8);
	NONE (9);
	NONE (10);
	NONE (11);
	NONE (12);
	NONE (13);
	NONE (14);
	NONE (15);
	CHECK (16);
	CHECK (17);
	CHECK (18);
	CHECK (19);
	CHECK (20);
	CHECK (21);
	NONE (22);
	NONE (23);
	NONE (24);
	NONE (25);
	NONE (26);
	NONE (27);

	REMOVE (16);
	REMOVE (17);
	CREATE (22);

	NONE (1);
	NONE (2);
	NONE (3);
	NONE (4);
	NONE (5);
	NONE (6);
	NONE (7);
	NONE (8);
	NONE (9);
	NONE (10);
	NONE (11);
	NONE (12);
	NONE (13);
	NONE (14);
	NONE (15);
	NONE (16);
	NONE (17);
	CHECK (18);
	CHECK (19);
	CHECK (20);
	CHECK (21);
	CHECK (22);
	NONE (23);
	NONE (24);
	NONE (25);
	NONE (26);
	NONE (27);

	REMOVE (18);
	REMOVE (19);
	CREATE (23);

	NONE (1);
	NONE (2);
	NONE (3);
	NONE (4);
	NONE (5);
	NONE (6);
	NONE (7);
	NONE (8);
	NONE (9);
	NONE (10);
	NONE (11);
	NONE (12);
	NONE (13);
	NONE (14);
	NONE (15);
	NONE (16);
	NONE (17);
	NONE (18);
	NONE (19);
	CHECK (20);
	CHECK (21);
	CHECK (22);
	CHECK (23);
	NONE (24);
	NONE (25);
	NONE (26);
	NONE (27);

	CREATE (24);

	NONE (1);
	NONE (2);
	NONE (3);
	NONE (4);
	NONE (5);
	NONE (6);
	NONE (7);
	NONE (8);
	NONE (9);
	NONE (10);
	NONE (11);
	NONE (12);
	NONE (13);
	NONE (14);
	NONE (15);
	NONE (16);
	NONE (17);
	NONE (18);
	NONE (19);
	CHECK (20);
	CHECK (21);
	CHECK (22);
	CHECK (23);
	CHECK (24);
	NONE (25);
	NONE (26);
	NONE (27);

	REMOVE (20);
	REMOVE (21);
	CREATE (25);

	NONE (1);
	NONE (2);
	NONE (3);
	NONE (4);
	NONE (5);
	NONE (6);
	NONE (7);
	NONE (8);
	NONE (9);
	NONE (10);
	NONE (11);
	NONE (12);
	NONE (13);
	NONE (14);
	NONE (15);
	NONE (16);
	NONE (17);
	NONE (18);
	NONE (19);
	NONE (20);
	NONE (21);
	CHECK (22);
	CHECK (23);
	CHECK (24);
	CHECK (25);
	NONE (26);
	NONE (27);

	CREATE (26);

	NONE (1);
	NONE (2);
	NONE (3);
	NONE (4);
	NONE (5);
	NONE (6);
	NONE (7);
	NONE (8);
	NONE (9);
	NONE (10);
	NONE (11);
	NONE (12);
	NONE (13);
	NONE (14);
	NONE (15);
	NONE (16);
	NONE (17);
	NONE (18);
	NONE (19);
	NONE (20);
	NONE (21);
	CHECK (22);
	CHECK (23);
	CHECK (24);
	CHECK (25);
	CHECK (26);
	NONE (27);

	REMOVE (22);
	REMOVE (23);
	CREATE (27);

	NONE (1);
	NONE (2);
	NONE (3);
	NONE (4);
	NONE (5);
	NONE (6);
	NONE (7);
	NONE (8);
	NONE (9);
	NONE (10);
	NONE (11);
	NONE (12);
	NONE (13);
	NONE (14);
	NONE (15);
	NONE (16);
	NONE (17);
	NONE (18);
	NONE (19);
	NONE (20);
	NONE (21);
	NONE (22);
	NONE (23);
	CHECK (24);
	CHECK (25);
	CHECK (26);
	CHECK (27);

#undef CREATE
#undef REMOVE
#undef CHECK
#undef NONE
}

int
main (void)
{
	mu_init ("hash/map");

	test_empty ();
	test_single_collision ();
	test_single_bucket_collision ();
	test_single_collision_wrap ();
	test_bucket_collision_steal ();
	test_hash_to_bucket_collision ();
	test_hash_to_bucket_collision2 ();
	test_lookup_collision ();
	test_set ();
	test_resize ();
	test_grow ();
	test_each ();
	test_remove ();
	test_remove_all ();
	test_pre_hash (0);
	test_pre_hash (100);

	return 0;
}

