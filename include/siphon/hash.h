#ifndef SIPHON_HASH_H
#define SIPHON_HASH_H

#include "common.h"
#include "seed.h"

SP_EXPORT uint32_t
sp_mix_uint32 (uint32_t x);

SP_EXPORT uint32_t
sp_mix_uint32s (uint32_t x, uint32_t y);

SP_EXPORT uint64_t
sp_mix_uint64 (uint64_t x);

SP_EXPORT uint64_t
sp_mix_uint64s (uint64_t x, uint64_t y);

SP_EXPORT uint64_t
sp_metrohash64 (const void *restrict s, size_t len, const SpSeed *restrict seed);

SP_EXPORT uint64_t
sp_siphash (const void *restrict s, size_t len, const SpSeed *restrict seed);

SP_EXPORT uint64_t
sp_siphash_case (const void *restrict s, size_t len, const SpSeed *restrict seed);

SP_EXPORT uint64_t
sp_xxhash64 (const void *input, size_t len, const SpSeed *restrict seed);

#endif

