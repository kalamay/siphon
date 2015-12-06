#include "../include/siphon/hash.h"
#include "../include/siphon/endian.h"

#include <ctype.h>

uint32_t
sp_mix_uint32 (uint32_t x)
{
	x ^= x >> 16;
	x *= 0x85ebca6b;
	x ^= x >> 13;
	x *= 0xc2b2ae35;
	x ^= x >> 16;
	return x;
}

uint32_t
sp_mix_uint32s (uint32_t x, uint32_t y)
{
	static const uint32_t mul = 0xdeadfa11;
	uint32_t a = (x ^ y) * mul;
	a ^= (a >> 19);
	uint32_t b = (y ^ a) * mul;
	b ^= (b >> 19);
	b *= mul;
	return b;
}

uint64_t
sp_mix_uint64 (uint64_t x)
{
	x ^= x >> 33;
	x *= 0xff51afd7ed558ccdULL;
	x ^= x >> 33;
	x *= 0xc4ceb9fe1a85ec53ULL;
	x ^= x >> 33;
	return x;
}

uint64_t
sp_mix_uint64s (uint64_t x, uint64_t y)
{
	static const uint64_t mul = 0x9ddfea08eb382d69ULL;
	uint64_t a = (x ^ y) * mul;
	a ^= (a >> 47);
	uint64_t b = (y ^ a) * mul;
	b ^= (b >> 47);
	b *= mul;
	return b;
}

inline static uint64_t
rotr (uint64_t v, unsigned k)
{
    return (v >> k) | (v << (64 - k));
}

inline static uint64_t
rotl (uint64_t v, unsigned k)
{
    return (v << k) | (v >> (64 - k));
}

inline static uint64_t
read_u64 (const void *const ptr)
{
    uint64_t val;
    memcpy (&val, ptr, sizeof val);
    return val;
}

inline static uint64_t
read_u32 (const void *const ptr)
{
    uint32_t val;
    memcpy (&val, ptr, sizeof val);
    return val;
}

inline static uint64_t
read_u16 (const void * const ptr)
{
    uint16_t val;
    memcpy (&val, ptr, sizeof val);
    return val;
}

inline static uint64_t
read_u8 (const void * const ptr)
{
	return (uint64_t)*(const uint8_t *)ptr;
}

// MetroHash Copyright (c) 2015 J. Andrew Rogers
// https://github.com/jandrewrogers/MetroHash
uint64_t
sp_metrohash64 (const void *restrict s, size_t len, const SpSeed *restrict seed)
{
	static const uint64_t k0 = 0xC83A91E1;
	static const uint64_t k1 = 0x8648DBDB;
	static const uint64_t k2 = 0x7BDEC03B;
	static const uint64_t k3 = 0x2F5870A5;

	const uint8_t *restrict ptr = s;
	const uint8_t *restrict const end = ptr + len;

	uint64_t hash = (((uint64_t)seed->u32 + k2) * k0) + len;

	if (len >= 32) {
		uint64_t v[4];
		v[0] = hash;
		v[1] = hash;
		v[2] = hash;
		v[3] = hash;

		do {
			v[0] += read_u64 (ptr) * k0; ptr += 8; v[0] = rotr (v[0],29) + v[2];
			v[1] += read_u64 (ptr) * k1; ptr += 8; v[1] = rotr (v[1],29) + v[3];
			v[2] += read_u64 (ptr) * k2; ptr += 8; v[2] = rotr (v[2],29) + v[0];
			v[3] += read_u64 (ptr) * k3; ptr += 8; v[3] = rotr (v[3],29) + v[1];
		}
		while (ptr <= (end - 32));

		v[2] ^= rotr (((v[0] + v[3]) * k0) + v[1], 33) * k1;
		v[3] ^= rotr (((v[1] + v[2]) * k1) + v[0], 33) * k0;
		v[0] ^= rotr (((v[0] + v[2]) * k0) + v[3], 33) * k1;
		v[1] ^= rotr (((v[1] + v[3]) * k1) + v[2], 33) * k0;
		hash += v[0] ^ v[1];
	}

	if ((end - ptr) >= 16) {
		uint64_t v0 = hash + (read_u64 (ptr) * k0); ptr += 8; v0 = rotr (v0,33) * k1;
		uint64_t v1 = hash + (read_u64 (ptr) * k1); ptr += 8; v1 = rotr (v1,33) * k2;
		v0 ^= rotr (v0 * k0, 35) + v1;
		v1 ^= rotr (v1 * k3, 35) + v0;
		hash += v1;
	}

	if ((end - ptr) >= 8) {
		hash += read_u64 (ptr) * k3; ptr += 8;
		hash ^= rotr (hash, 33) * k1;
	}

	if ((end - ptr) >= 4) {
		hash += read_u32 (ptr) * k3; ptr += 4;
		hash ^= rotr (hash, 15) * k1;
	}

	if ((end - ptr) >= 2) {
		hash += read_u16 (ptr) * k3; ptr += 2;
		hash ^= rotr (hash, 13) * k1;
	}

	if ((end - ptr) >= 1) {
		hash += read_u8 (ptr) * k3;
		hash ^= rotr (hash, 25) * k1;
	}

	hash ^= rotr (hash, 33);
	hash *= k0;
	hash ^= rotr (hash, 33);

	return hash;
}

#define SIPROUND(n, a, b, c, d) do {                  \
	for (int i = 0; i < n; i++) {                     \
		a += b; b=rotl (b,13); b ^= a; a=rotl (a,32); \
		c += d; d=rotl (d,16); d ^= c;                \
		a += d; d=rotl (d,21); d ^= a;                \
		c += b; b=rotl (b,17); b ^= c; c=rotl (c,32); \
	}                                                 \
} while(0)

uint64_t
sp_siphash (const void *restrict s, size_t len, const SpSeed *restrict seed)
{
	uint64_t b = ((uint64_t)len) << 56;
	uint64_t k0 = sp_le64toh (seed->u128.low);
	uint64_t k1 = sp_le64toh (seed->u128.high);
	uint64_t v0 = 0x736f6d6570736575ULL ^ k0;
	uint64_t v1 = 0x646f72616e646f6dULL ^ k1;
	uint64_t v2 = 0x6c7967656e657261ULL ^ k0;
	uint64_t v3 = 0x7465646279746573ULL ^ k1;
	const uint8_t *end = (uint8_t *)s + len - (len % 8);

	for (; s != end; s = (uint8_t*)s + 8) {
		uint64_t m = sp_le64toh (*(uint64_t *)s);
		v3 ^= m;
		SIPROUND (2, v0, v1, v2, v3);
		v0 ^= m;
	}

	switch (len & 7) {
		case 7: b |= ((uint64_t)((uint8_t *)s)[6])  << 48;
		case 6: b |= ((uint64_t)((uint8_t *)s)[5])  << 40;
		case 5: b |= ((uint64_t)((uint8_t *)s)[4])  << 32;
		case 4: b |= ((uint64_t)((uint8_t *)s)[3])  << 24;
		case 3: b |= ((uint64_t)((uint8_t *)s)[2])  << 16;
		case 2: b |= ((uint64_t)((uint8_t *)s)[1])  <<  8;
		case 1: b |= ((uint64_t)((uint8_t *)s)[0]); break;
		case 0: break;
	}

	v3 ^= b;
	SIPROUND (2, v0, v1, v2, v3);
	v0 ^= b;
	v2 ^= 0xff;
	SIPROUND (4, v0, v1, v2, v3);
	return v0 ^ v1 ^ v2  ^ v3;
}

static const uint8_t lower[256] = {
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x5b,0x5c,0x5d,0x5e,0x5f,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
	0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
	0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,
};

uint64_t
sp_siphash_case (const void *restrict s, size_t len, const SpSeed *restrict seed)
{
	uint64_t b = ((uint64_t)len) << 56;
	uint64_t k0 = sp_le64toh (seed->u128.low);
	uint64_t k1 = sp_le64toh (seed->u128.high);
	uint64_t v0 = 0x736f6d6570736575ULL ^ k0;
	uint64_t v1 = 0x646f72616e646f6dULL ^ k1;
	uint64_t v2 = 0x6c7967656e657261ULL ^ k0;
	uint64_t v3 = 0x7465646279746573ULL ^ k1;
	const uint8_t *end = (uint8_t *)s + len - (len % 8);

	for (; s != end; s = (uint8_t *)s + 8) {
		uint64_t m = *(uint64_t *)s;
		for (int i = 0; i < 8; i++) {
			((uint8_t *)&m)[i] = lower[((uint8_t *)&m)[i]];
		}
		m = sp_le64toh (m);
		v3 ^= m;
		SIPROUND (2, v0, v1, v2, v3);
		v0 ^= m;
	}

	switch (len & 7) {
		case 7: b |= ((uint64_t)lower[((uint8_t *)s)[6]])  << 48;
		case 6: b |= ((uint64_t)lower[((uint8_t *)s)[5]])  << 40;
		case 5: b |= ((uint64_t)lower[((uint8_t *)s)[4]])  << 32;
		case 4: b |= ((uint64_t)lower[((uint8_t *)s)[3]])  << 24;
		case 3: b |= ((uint64_t)lower[((uint8_t *)s)[2]])  << 16;
		case 2: b |= ((uint64_t)lower[((uint8_t *)s)[1]])  <<  8;
		case 1: b |= ((uint64_t)lower[((uint8_t *)s)[0]]); break;
		case 0: break;
	}

	v3 ^= b;
	SIPROUND (2, v0, v1, v2, v3);
	v0 ^= b;
	v2 ^= 0xff;
	SIPROUND (4, v0, v1, v2, v3);
	return v0 ^ v1 ^ v2  ^ v3;
}


// xxHash Copyright (c) 2012-2015, Yann Collet
// https://github.com/Cyan4973/xxHash

#define XX64_PRIME_1 11400714785074694791ULL
#define XX64_PRIME_2 14029467366897019727ULL
#define XX64_PRIME_3  1609587929392839161ULL
#define XX64_PRIME_4  9650029242287828579ULL
#define XX64_PRIME_5  2870177450012600261ULL

uint64_t
sp_xxhash64 (const void *restrict input, size_t len, const SpSeed *restrict seed)
{
	const uint8_t *restrict p = input;
	const uint8_t *restrict pe = p + len;
	uint64_t h;

	if (len >= 32) {
		const uint8_t *restrict limit = pe - 32;
		uint64_t v1 = seed->u64 + XX64_PRIME_1 + XX64_PRIME_2;
		uint64_t v2 = seed->u64 + XX64_PRIME_2;
		uint64_t v3 = seed->u64 + 0;
		uint64_t v4 = seed->u64 - XX64_PRIME_1;

		do {
			v1 += sp_le64toh (read_u64 (p)) * XX64_PRIME_2;
			p+=8;
			v1 = rotl (v1, 31);
			v1 *= XX64_PRIME_1;
			v2 += sp_le64toh (read_u64 (p)) * XX64_PRIME_2;
			p+=8;
			v2 = rotl (v2, 31);
			v2 *= XX64_PRIME_1;
			v3 += sp_le64toh (read_u64 (p)) * XX64_PRIME_2;
			p+=8;
			v3 = rotl (v3, 31);
			v3 *= XX64_PRIME_1;
			v4 += sp_le64toh (read_u64 (p)) * XX64_PRIME_2;
			p+=8;
			v4 = rotl (v4, 31);
			v4 *= XX64_PRIME_1;
		} while (p <= limit);

		h = rotl (v1, 1) + rotl (v2, 7) + rotl (v3, 12) + rotl (v4, 18);

		v1 *= XX64_PRIME_2;
		v1 = rotl (v1, 31);
		v1 *= XX64_PRIME_1;
		h ^= v1;
		h = h * XX64_PRIME_1 + XX64_PRIME_4;

		v2 *= XX64_PRIME_2;
		v2 = rotl (v2, 31);
		v2 *= XX64_PRIME_1;
		h ^= v2;
		h = h * XX64_PRIME_1 + XX64_PRIME_4;

		v3 *= XX64_PRIME_2;
		v3 = rotl (v3, 31);
		v3 *= XX64_PRIME_1;
		h ^= v3;
		h = h * XX64_PRIME_1 + XX64_PRIME_4;

		v4 *= XX64_PRIME_2;
		v4 = rotl (v4, 31);
		v4 *= XX64_PRIME_1;
		h ^= v4;
		h = h * XX64_PRIME_1 + XX64_PRIME_4;
	}
	else {
		h = seed->u64 + XX64_PRIME_5;
	}

	h += (uint64_t)len;

	while (p+8 <= pe) {
		uint64_t k1 = sp_le64toh (read_u64 (p));
		k1 *= XX64_PRIME_2;
		k1 = rotl (k1,31);
		k1 *= XX64_PRIME_1;
		h ^= k1;
		h = rotl (h,27) * XX64_PRIME_1 + XX64_PRIME_4;
		p+=8;
	}

	if (p+4 <= pe) {
		h ^= sp_le64toh (read_u32 (p)) * XX64_PRIME_1;
		h = rotl (h, 23) * XX64_PRIME_2 + XX64_PRIME_3;
		p+=4;
	}

	while (p < pe) {
		h ^= (*p) * XX64_PRIME_5;
		h = rotl (h, 11) * XX64_PRIME_1;
		p++;
	}

	h ^= h >> 33;
	h *= XX64_PRIME_2;
	h ^= h >> 29;
	h *= XX64_PRIME_3;
	h ^= h >> 32;

	return h;
}

