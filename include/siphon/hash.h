#ifndef SIPHON_HASH_H
#define SIPHON_HASH_H

#include "common.h"
#include "seed.h"

SP_EXPORT uint64_t
sp_metrohash64 (const void *restrict s, size_t len, const SpSeed *restrict seed);

SP_EXPORT uint64_t
sp_siphash (const void *restrict s, size_t len, const SpSeed *restrict seed);

SP_EXPORT uint64_t
sp_siphash_case (const void *restrict s, size_t len, const SpSeed *restrict seed);

#endif

