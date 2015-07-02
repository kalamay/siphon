#include "siphon/bloom.h"
#include "siphon/hash.h"

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

#define MIN_BYTES (64 - (sizeof (SpBloom) - sizeof (((SpBloom *)0)->bytes)))
#define MIN_BITS (MIN_BYTES * 8)

static inline void
bloom_type (size_t hint, double fpp, uint64_t *bits, uint64_t *bytes, uint8_t *hashes)
{
	double bpe = log (fpp) / -0.480453013918201;
	uint64_t nbits = (uint64_t)round ((double)hint * bpe);
	uint64_t nbytes;

	if (nbits < MIN_BITS) {
		*bits = MIN_BITS;
		*bytes = MIN_BYTES;
	}
	else {
		nbytes = nbits % 8 ? (nbits / 8) + 1 : nbits / 8;
		nbytes += (16 - (nbytes % 16));
		*bits = nbytes * 8;
		*bytes = nbytes;
	}
	*hashes = (uint8_t)ceil (0.693147180559945 * bpe);
}

SpBloom *
sp_bloom_create (size_t hint, double fpp, uint32_t seed)
{
	uint64_t bits;
	uint64_t bytes;
	uint8_t hashes;

	bloom_type (hint, fpp, &bits, &bytes, &hashes);

	SpBloom *self = calloc (1, sizeof *self + (bytes - 1));
	if (self != NULL) {
		self->bits = bits;
		self->seed = seed;
		self->hashes = hashes;
	}
	return self;
}

void
sp_bloom_destroy (SpBloom *self)
{
	if (self != NULL) {
		free (self);
	}
}

bool
sp_bloom_is_capable (SpBloom *self, size_t hint, double fpp)
{
	assert (self != NULL);

	uint64_t bits;
	uint64_t bytes;
	uint8_t hashes;

	bloom_type (hint, fpp, &bits, &bytes, &hashes);

	return self->bits >= bits && self->hashes >= hashes;
}

bool
sp_bloom_maybe (SpBloom *self, const void *restrict buf, size_t len)
{
	assert (self != NULL);
	assert (buf != NULL);

	uint64_t hash = sp_metrohash64 (buf, len, self->seed);
	return sp_bloom_maybe_hash (self, hash);
}

bool
sp_bloom_maybe_hash (SpBloom *self, uint64_t hash)
{
	assert (self != NULL);
	assert (hash != 0);

	register uint32_t a = (uint32_t)hash;
	register uint32_t b = (uint32_t)(hash >> 32);
	register uint32_t i = self->hashes;
	register uint32_t x;

	while (i-- > 0) {
		x = (a + i*b) % self->bits;
		if (!(self->bytes[x >> 3] & (1 << (x % 8)))) {
			return false;
		}
	}

	return true;
}

void
sp_bloom_put (SpBloom *self, const void *restrict buf, size_t len)
{
	assert (self != NULL);
	assert (buf != NULL);

	uint64_t hash = sp_metrohash64 (buf, len, self->seed);
	sp_bloom_put_hash (self, hash);
}

void
sp_bloom_put_hash (SpBloom *self, uint64_t hash)
{
	assert (self);

	register uint32_t a = (uint32_t)hash;
	register uint32_t b = (uint32_t)(hash >> 32);
	register uint32_t i = self->hashes;
	register uint32_t x;
	register uint32_t n = 0;

	while (i-- > 0) {
		x = (a + i*b) % self->bits;
		n += !(self->bytes[x >> 3] & (1 << (x % 8)));
		self->bytes[x >> 3] |= (1 << (x % 8));
	}

	if (n) {
		self->count++;
	}
}

void
sp_bloom_clear (SpBloom *self)
{
	assert (self != NULL);

	memset (self->bytes, 0, self->bits / 8);
}

