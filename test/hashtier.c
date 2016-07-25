#include "../include/siphon/hash/tier.h"
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

uint64_t
thing_hash (int key, size_t len)
{
	(void)len;
	uint64_t x = ((uint64_t)key << 1) | 1;
	x ^= x >> 33;
	x *= 0xff51afd7ed558ccdULL;
	x ^= x >> 33;
	x *= 0xc4ceb9fe1a85ec53ULL;
	x ^= x >> 33;
	return x;
}

typedef SP_HTIER (Thing) ThingTier;

SP_HTIER_PROTOTYPE_STATIC (ThingTier, int, thing_tier)
SP_HTIER_GENERATE (ThingTier, int, thing_tier, thing_has_key)

static bool
verify_tier (ThingTier *tier)
{
	bool ok = true;
	size_t count = 0;
	for (size_t i = 0; i < tier->size; i++) {
		if (tier->arr[i].h == 0) { continue; }
		ssize_t idx = thing_tier_get (tier, tier->arr[i].entry.key, 0,
				thing_hash (tier->arr[i].entry.key, 0));
		mu_assert_int_eq (idx, i);
		if (idx != (ssize_t)i) {
			ok = false;
		}
		count++;
	}
	mu_assert_uint_eq (count, tier->count);
	return ok;
}

static void
test_tier (void)
{
	bool new;
	ThingTier *tier = NULL, *next = NULL;
	thing_tier_create (&tier, 10);
	thing_tier_create (&next, 20);

	mu_assert_uint_eq (tier->size, 16);
	mu_assert_uint_eq (tier->mod, 13);

	mu_assert_uint_eq (next->size, 32);
	mu_assert_uint_eq (next->mod, 31);

	srand (7);

	for (int i = 0; i < 15; i++) {
		int key = rand ();
		ssize_t idx = thing_tier_reserve (tier, key, 0, thing_hash (key, 0), &new);
		if (idx >= 0) {
			tier->arr[idx].entry.key = key;
			tier->arr[idx].entry.value = rand ();
		}
	}

	mu_assert_uint_eq (tier->count, 15);
	mu_assert_uint_eq (next->count, 0);

	mu_assert (verify_tier (tier));
	mu_assert (verify_tier (next));

	mu_assert_uint_eq (thing_tier_nremap (tier, next, 5), 5);

	mu_assert_uint_eq (tier->count, 10);
	mu_assert_uint_eq (next->count, 5);

	mu_assert (verify_tier (tier));
	mu_assert (verify_tier (next));

	mu_assert_uint_eq (thing_tier_nremap (tier, next, 5), 5);

	mu_assert_uint_eq (tier->count, 5);
	mu_assert_uint_eq (next->count, 10);

	mu_assert (verify_tier (tier));
	mu_assert (verify_tier (next));

	mu_assert_uint_eq (thing_tier_remap (tier, next), 5);

	mu_assert_uint_eq (tier->count, 0);
	mu_assert_uint_eq (next->count, 15);

	mu_assert (verify_tier (tier));
	mu_assert (verify_tier (next));

	srand (7);

	for (int i = 0; i < 15; i++) {
		int key = rand ();
		ssize_t idx = thing_tier_get (next, key, 0, thing_hash (key, 0));
		mu_assert_int_ge (idx, 0);
		if (idx >= 0) {
			mu_assert_int_eq (next->arr[idx].entry.key, key);
			mu_assert_int_eq (next->arr[idx].entry.value, rand ());
		}
	}

	free (tier);
	free (next);
}

int
main (void)
{
	mu_init ("hash/tier");

	test_tier ();

	return 0;
}

