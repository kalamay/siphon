#include "../include/siphon/seed.h"
#include "../include/siphon/rand.h"

static SpSeed SEED_RANDOM;
static SpSeed SEED_DEFAULT = {
	.u128 = {
		0x9ae16a3b2f90404fULL,
		0xc3a5c85c97cb3127ULL
	}
};

const SpSeed *const restrict SP_SEED_RANDOM = &SEED_RANDOM;
const SpSeed *const restrict SP_SEED_DEFAULT = &SEED_DEFAULT;

static void __attribute__((constructor(102)))
init (void)
{
	sp_rand (&SEED_RANDOM, sizeof SEED_RANDOM);
}

