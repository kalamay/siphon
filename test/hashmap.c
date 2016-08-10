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

int
main (void)
{
	mu_init ("hash/map");

	test_set ();
	test_resize ();
	test_grow ();
	test_each ();
	test_remove ();

	return 0;
}

