#ifndef SIPHON_BLOOM_H
#define SIPHON_BLOOM_H

#include "common.h"

typedef struct {
	uint64_t bits;
	uint32_t seed;
	uint8_t hashes;
	uint8_t bytes[1];
} SpBloom;

SP_EXPORT SpBloom *
sp_bloom_create (size_t hint, double fpp, uint32_t seed);

SP_EXPORT bool
sp_bloom_is_capable (SpBloom *self, size_t hint, double fpp);

SP_EXPORT bool
sp_bloom_maybe (SpBloom *self, const void *restrict buf, size_t len);

SP_EXPORT bool
sp_bloom_maybe_hash (SpBloom *self, uint64_t hash);

SP_EXPORT void
sp_bloom_put (SpBloom *self, const void *restrict buf, size_t len);

SP_EXPORT void
sp_bloom_put_hash (SpBloom *self, uint64_t hash);

SP_EXPORT void
sp_bloom_clear (SpBloom *self);

#endif

