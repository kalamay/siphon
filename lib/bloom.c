#include "../include/siphon/bloom.h"
#include "../include/siphon/alloc.h"
#include "../include/siphon/hash.h"

#include <assert.h>

#define BASE_SIZE (sizeof (SpBloom) - sizeof (((SpBloom *)0)->bytes))
#define MIN_BYTES (64 - BASE_SIZE)
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
		nbytes += (16 - ((nbytes + BASE_SIZE) % 16));
		*bits = nbytes * 8;
		*bytes = nbytes;
	}
	*hashes = (uint8_t)ceil (0.693147180559945 * bpe);
}

uint64_t
sp_bloom_hash (const void *restrict buf, size_t len)
{
	assert (buf != NULL);

	return sp_metrohash64 (buf, len, SP_SEED_DEFAULT);
}

SpBloom *
sp_bloom_new (size_t hint, double fpp)
{
	uint64_t bits;
	uint64_t bytes;
	uint8_t hashes;

	bloom_type (hint, fpp, &bits, &bytes, &hashes);

	SpBloom *self = sp_malloc (BASE_SIZE + bytes);
	if (self != NULL) {
		self->fpp = fpp;
		self->count = 0;
		self->bits = bits;
		self->hashes = hashes;
		memset (self->bytes, 0, bytes);
	}
	return self;
}

void
sp_bloom_free (SpBloom *self)
{
	if (self != NULL) {
		//sp_free (self, BASE_SIZE + self->bits/8);
	}
}

bool
sp_bloom_is_capable (SpBloom *self, size_t hint, double fpp)
{
	if (self == NULL) return false;

	uint64_t bits;
	uint64_t bytes;
	uint8_t hashes;

	bloom_type (hint, fpp, &bits, &bytes, &hashes);

	return self->bits >= bits && self->hashes >= hashes;
}

bool
sp_bloom_can_hold (SpBloom *self, size_t more)
{
	if (self == NULL) return false;

	uint64_t count = self->count * (1.0 + self->fpp);
	return sp_bloom_is_capable (self, count + more, self->fpp);
}

bool
sp_bloom_maybe (SpBloom *self, const void *restrict buf, size_t len)
{
	if (self == NULL) return true;

	assert (buf != NULL);

	uint64_t hash = sp_bloom_hash (buf, len);
	return sp_bloom_maybe_hash (self, hash);
}

bool
sp_bloom_maybe_hash (SpBloom *self, uint64_t hash)
{
	if (self == NULL) return false;

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
	if (self == NULL) return;

	assert (buf != NULL);

	uint64_t hash = sp_bloom_hash (buf, len);
	sp_bloom_put_hash (self, hash);
}

void
sp_bloom_put_hash (SpBloom *self, uint64_t hash)
{
	if (self == NULL) return;

	register uint32_t a = (uint32_t)hash;
	register uint32_t b = (uint32_t)(hash >> 32);
	register uint32_t i = self->hashes;
	register uint32_t x;
	register uint32_t n = 0;

	while (i-- > 0) {
		x = (a + i*b) % self->bits;
		n += !!(self->bytes[x >> 3] & (1 << (x % 8)));
		self->bytes[x >> 3] |= (1 << (x % 8));
	}

	if (n != self->hashes) {
		self->count++;
	}
}

void
sp_bloom_clear (SpBloom *self)
{
	if (self != NULL) {
		memset (self->bytes, 0, self->bits / 8);
	}
}

SpBloom *
sp_bloom_copy (SpBloom *self)
{
	if (self == NULL) {
		return NULL;
	}

	size_t size = BASE_SIZE + self->bits/8;
	SpBloom *copy = sp_malloc (size);
	if (copy != NULL) {
		memcpy (copy, self, size);
	}
	return copy;
}

void
sp_bloom_print (const SpBloom *self, FILE *out)
{
	if (out == NULL) {
		out = stderr;
	}

	if (self == NULL) {
		fprintf (out, "#<SpBloom:(null)>\n");
	}
	else {
		fprintf (out, "#<SpBloom:%p fpp=%f, count=%llu, bits=%llu, hashes=%u>\n",
				(void *)self, self->fpp, self->count, self->bits, self->hashes);
	}
}

