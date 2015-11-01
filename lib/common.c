#include "../include/siphon/common.h"
#include "../include/siphon/fmt.h"
#include "../include/siphon/rand.h"

static SpSeed SEED_DEFAULT = { .u128 = { 0x9ae16a3b2f90404fULL, 0xc3a5c85c97cb3127ULL } };
static SpSeed SEED_RANDOM;

const SpSeed *const restrict SP_SEED_DEFAULT = &SEED_DEFAULT;
const SpSeed *const restrict SP_SEED_RANDOM = &SEED_RANDOM;

static void __attribute__((constructor))
init (void)
{
	sp_rand (&SEED_RANDOM, sizeof SEED_RANDOM);
}

void
sp_print_ptr (const void *val, FILE *out)
{
	fprintf (out, "%p", val);
}

void
sp_print_str (const void *val, FILE *out)
{
	sp_fmt_str (out, val, strlen (val), true);
}

