#include "../include/siphon/hash/map.h"
#include "mu.h"

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

void
dump (const ThingMap *map)
{
	for (size_t n = 0; n < sp_len (map->tiers); n++) {
		if (map->tiers[n] == NULL) { break; }
		for (size_t i = 0; i < map->tiers[n]->size; i++) {
			if (map->tiers[n]->arr[i].h == 0) { continue; }
			printf ("  #%zu/%zu = { key = %d, value = %d, hash = %" PRIu64 " }\n",
				n, i,
				map->tiers[n]->arr[i].entry.key,
				map->tiers[n]->arr[i].entry.value,
				map->tiers[n]->arr[i].h);
		}
	}
}

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

typedef struct {
	uint64_t key;
	int value;
} Hashed;

uint64_t
hashed_hash (uint64_t key, size_t len)
{
	(void)len;

	// We using siphash as the only identity 😳
	return key;
}

bool
hashed_has_key (Hashed *h, uint64_t key, size_t len)
{
	(void)h;
	(void)key;
	(void)len;

	// This can just return `true` because the map won't call this function
	// unless the hashes are equal;
	return true;
}

typedef SP_HMAP (Hashed, 2, hashed) HashedMap;

SP_HMAP_PROTOTYPE_STATIC (HashedMap, uint64_t, Hashed, hashed)
SP_HMAP_GENERATE (HashedMap, uint64_t, Hashed, hashed)

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
	HashedMap map;
	hashed_init (&map, 0.85, hint);

#define CREATE(n) do {                               \
	uint64_t key = make_key (n);                     \
	bool new;                                        \
	Hashed *h = hashed_reserve (&map, key, 0, &new); \
	mu_assert_ptr_ne (h, NULL);                      \
	mu_assert (new);                                 \
	if (h != NULL) {                                 \
		h->key = key;                                \
		h->value = n;                                \
	}                                                \
} while (0)

#define REMOVE(n) do {                               \
	uint64_t key = make_key (n);                     \
	mu_assert (hashed_del (&map, key, 0, NULL));     \
} while (0)

#define CHECK(n) do {                                \
	uint64_t key = make_key (n);                     \
	Hashed *h = hashed_get (&map, key, 0);           \
	mu_assert_ptr_ne (h, NULL);                      \
	if (h != NULL) {                                 \
		mu_assert_int_eq (h->value, n);              \
	}                                                \
} while (0)

#define NONE(n) do {                                 \
	uint64_t key = make_key (n);                     \
	Hashed *h = hashed_get (&map, key, 0);           \
	mu_assert_ptr_eq (h, NULL);                      \
	mu_assert (!hashed_has (&map, key, 0));          \
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

	test_set ();
	test_resize ();
	test_grow ();
	test_each ();
	test_remove ();
	test_pre_hash (0);
	test_pre_hash (100);

	return 0;
}

