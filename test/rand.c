#include "../include/siphon/rand.h"
#include "../include/siphon/seed.h"
#include "mu/mu.h"

static void
test_seed (void)
{
	mu_assert_uint_ne (SP_SEED_RANDOM->u128.low, 0);
	mu_assert_uint_ne (SP_SEED_RANDOM->u128.high, 0);
}

int
main (void)
{
	mu_init ("rand");
	
	test_seed ();
}

