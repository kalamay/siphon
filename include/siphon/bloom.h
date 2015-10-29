#ifndef SIPHON_BLOOM_H
#define SIPHON_BLOOM_H

#include "common.h"
#include "hash.h"

typedef struct {
	const SpSeed *seed;
	double fpp;
	uint64_t count;
	uint64_t bits;
	uint8_t hashes;
	uint8_t bytes[31];
} SpBloom;

SP_EXPORT SpBloom *
sp_bloom_new (size_t hint, double fpp, const SpSeed *restrict seed);

SP_EXPORT void
sp_bloom_free (SpBloom *self);

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

SP_EXPORT void
sp_bloom_print (const SpBloom *self, FILE *out);

#endif

