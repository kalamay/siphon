#include <stdio.h>

#include "siphon/bloom.h"
#include "mu/mu.h"

static void
test_basic (void)
{
	SpBloom *b = sp_bloom_create (3, 0.01, 0);
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

	sp_bloom_destroy (b);
}

static void
test_large (void)
{
	SpBloom *b = sp_bloom_create (10000, 0.001, 0);

	char buf[16];
	for (int i = 0; i < 1000; i++) {
		int len = snprintf (buf, sizeof buf, "val %d", i);
		sp_bloom_put (b, buf, len);
	}

	mu_assert_uint_eq (b->count, 1000);

	sp_bloom_destroy (sp_bloom_copy (b));

	sp_bloom_destroy (b);
}

int
main (void)
{
	test_basic ();
	test_large ();

	mu_exit ("bloom");
}

