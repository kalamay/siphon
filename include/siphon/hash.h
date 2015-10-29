#ifndef SIPHON_HASH_H
#define SIPHON_HASH_H

#include "common.h"

typedef union {
	struct { uint64_t low, high; } u128;
	uint64_t u64;
	uint32_t u32;
	uint8_t bytes[16];
} SpSeed;

SP_EXPORT const SpSeed *const restrict SP_SEED_DEFAULT;
SP_EXPORT const SpSeed *const restrict SP_SEED_RANDOM;


typedef uint64_t (*KDHash)(const void *restrict key, size_t len, const SpSeed *restrict seed);
typedef bool (*KDEquals)(const void *val, const void *restrict key, size_t len);

typedef struct {
	KDHash hash;
	KDEquals equals;
} SpHashType;

SP_EXPORT const SpHashType *const SP_HASH_RAW;


SP_EXPORT uint64_t
sp_metrohash64 (const void *restrict s, size_t len, const SpSeed *restrict seed);

SP_EXPORT uint64_t
sp_siphash (const void *restrict s, size_t len, const SpSeed *restrict seed);

SP_EXPORT uint64_t
sp_siphash_case (const void *restrict s, size_t len, const SpSeed *restrict seed);

#endif

