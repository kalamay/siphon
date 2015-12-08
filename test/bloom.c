#include "../include/siphon/bloom.h"
#include "../include/siphon/alloc.h"
#include "mu.h"

static void
test_basic (void)
{
	SpBloom *b = sp_bloom_new (3, 0.01);
	sp_bloom_put (b, "test", 4);
	sp_bloom_put (b, "value", 5);
	sp_bloom_put (b, "stuff", 5);

	mu_assert (sp_bloom_maybe (b, "test", 4))
	mu_assert (sp_bloom_maybe (b, "value", 5))
	mu_assert (sp_bloom_maybe (b, "stuff", 5))
	mu_assert (!sp_bloom_maybe (b, "other", 5))

	mu_assert_uint_eq (b->count, 3);

	sp_bloom_put (b, "test", 4);
	sp_bloom_put (b, "value", 5);
	sp_bloom_put (b, "stuff", 5);

	mu_assert_uint_eq (b->count, 3);

	sp_bloom_free (b);
}

static void
test_large (void)
{
	SpBloom *b = sp_bloom_new (1000, 0.01);

	char buf[16];
	for (int i = 0; i < 1000; i++) {
		int len = snprintf (buf, sizeof buf, "val %d", i);
		sp_bloom_put (b, buf, len);
	}

	mu_assert_uint_gt (b->count, (uint64_t)(1000 * b->fpp));

	sp_bloom_free (b);
}

int
main (void)
{
	mu_init ("bloom");

	test_basic ();
	test_large ();

	mu_assert (sp_alloc_summary ());
}

