#ifndef SIPHON_BLOOM_H
#define SIPHON_BLOOM_H

#include "common.h"

typedef struct {
	double fpp;
	uint64_t count;
	uint64_t bits;
	uint32_t seed;
	uint8_t hashes;
	uint8_t bytes[3];
} SpBloom;

SP_EXPORT SpBloom *
sp_bloom_create (size_t hint, double fpp, uint32_t seed);

SP_EXPORT void
sp_bloom_destroy (SpBloom *self);

SP_EXPORT bool
sp_bloom_is_capable (SpBloom *self, size_t hint, double fpp);

SP_EXPORT bool
sp_bloom_can_hold (SpBloom *self, size_t more);

SP_EXPORT uint64_t
sp_bloom_hash (SpBloom *self, const void *restrict buf, size_t len);

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

SP_EXPORT SpBloom *
sp_bloom_copy (SpBloom *self);

#endif

