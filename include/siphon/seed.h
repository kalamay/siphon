#ifndef SIPHON_SEED_H
#define SIPHON_SEED_H

#include "common.h"

typedef union {
	struct { uint64_t low, high; } u128;
	uint64_t u64;
	uint32_t u32;
	uint8_t bytes[16];
} SpSeed;

SP_EXPORT const SpSeed *const restrict SP_SEED_RANDOM;
SP_EXPORT const SpSeed *const restrict SP_SEED_DEFAULT;

#endif

