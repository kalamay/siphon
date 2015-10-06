#include "siphon/msgpack.h"
#include "siphon/error.h"
#include "siphon/endian.h"

#include <stdio.h>
#include <assert.h>

#define DONE 0xFFFFFFFFU
#define IS_DONE(cs) (((cs) & 0xF0000000U) != 0)

#define YIELD_ERROR(err) do { \
	p->cs = DONE;             \
	return err;               \
} while (0)

#define EXPECT_SIZE(sz, remain, done) do { \
	if ((sz) > remain) {                   \
		if (done) {                        \
			YIELD_ERROR (SP_ESYNTAX);      \
		}                                  \
		return 0;                          \
	}                                      \
} while (0)

/**
 * Loads an unsigned integer into the tag.
 * @p: parser pointer
 * @bits: integer bit size constant
 * @src: byte stream source
 * @len: byte length available on stream
 * @eof: if the byte length mark the end of input
 */
#define LOAD_UINT(p, bits, src, len, eof) do {                                 \
	EXPECT_SIZE (bits/8 + 1, len, eof);                                        \
	p->type = SP_MSGPACK_POSITIVE;                                             \
	p->tag.u64 = (uint##bits##_t)be##bits##toh (*(uint##bits##_t *)((src)+1)); \
	return bits/8 + 1;                                                         \
} while (0)

/**
 * Loads a signed integer into the tag.
 * @p: parser pointer
 * @bits: integer bit size constant
 * @src: byte stream source
 * @len: byte length available on stream
 * @eof: if the byte length mark the end of input
 */
#define LOAD_SINT(p, bits, src, len, eof) do {                                \
	EXPECT_SIZE (bits/8 + 1, len, eof);                                       \
	int64_t val = (int##bits##_t)be##bits##toh (*(int##bits##_t *)((src)+1)); \
	if (val < 0) {                                                            \
		p->type = SP_MSGPACK_NEGATIVE;                                        \
		p->tag.i64 = val;                                                     \
	}                                                                         \
	else {                                                                    \
		p->type = SP_MSGPACK_POSITIVE;                                        \
		p->tag.u64 = (uint64_t)val;                                           \
	}                                                                         \
	return bits/8 + 1;                                                        \
} while (0)

/**
 * Reads a dynamically sized unsigned integer.
 * @dst: value to store the read value
 * @src: the read position of the buffer
 * @len: length of bytes to read
 */
#define READ_DINT(dst, src, len) do {                   \
	uint64_t out = 0;                                   \
	memcpy ((uint8_t *)(&out)+(8-(len)), (src), (len)); \
	(dst) = be64toh (out);                              \
} while (0)

#if BYTE_ORDER == LITTLE_ENDIAN

# define READ_FLT(dst, src) do {         \
	union { uint32_t i; float f; } mem;  \
	mem.f = *(float *)(src);             \
	mem.i = __builtin_bswap32 (mem.i);   \
	(dst) = mem.f;                       \
} while (0)

# define READ_DBL(dst, src) do {         \
	union { uint64_t i; double f; } mem; \
	mem.f = *(double *)(src);            \
	mem.i = __builtin_bswap64 (mem.i);   \
	(dst) = mem.f;                       \
} while (0)

#elif BYTE_ORDER == BIG_ENDIAN

# define READ_FLT(dst, src) do { \
	(dst) = (*(float *)(src));   \
} while (0)

# define READ_DBL(dst, src) do { \
	(dst) = (*(double *)(src));  \
} while (0)

#endif

#define WRITE_INT(buf, tag, T, val) do {   \
	((uint8_t *)buf)[0] = (tag);           \
	*(T *)((uint8_t *)(buf) + 1) = (T)val; \
} while (0)

#include "stack.h"

#define STACK_ARR 0
#define STACK_MAP 1

#define PUSH_COUNT(p, n) do { p->counts[p->depth-1] = (n); } while (0)

#define STACK_PUSH_ARR(p, n) do { STACK_PUSH_FALSE(p); PUSH_COUNT(p, n); } while (0)
#define STACK_PUSH_MAP(p, n) do { STACK_PUSH_TRUE(p); PUSH_COUNT(p, n); } while (0)

#define STACK_IN_ARR(p) (STACK_TOP(p) == STACK_ARR)
#define STACK_IN_MAP(p) (STACK_TOP(p) == STACK_MAP)

#define STACK_POP_ARR(p) STACK_POP(p, STACK_ARR)
#define STACK_POP_MAP(p) STACK_POP(p, STACK_MAP)

#define IS_AT_KEY(p) (             \
	p->depth > 0 &&                \
	STACK_IN_MAP (p) &&            \
	p->counts[p->depth-1] % 2 == 0 \
)

void
sp_msgpack_init (SpMsgpack *p)
{
	memset (p, 0, sizeof *p);
	p->type = SP_MSGPACK_NONE;
}

static inline ssize_t
next_tag (SpMsgpack *p, const uint8_t *restrict buf, size_t len, bool eof)
{
	unsigned blen = 0;

	switch (*buf & 0x1f) {

	case 0x00: // nil
		p->type = SP_MSGPACK_NIL;
		return 1;

	case 0x01: // invalid
		break;

	case 0x02: // false
		p->type = SP_MSGPACK_FALSE;
		return 1;

	case 0x03: // true
		p->type = SP_MSGPACK_TRUE;
		return 1;

	case 0x04: // bin 8
	case 0x05: // bin 16
	case 0x06: // bin 32
		blen = 1 << (((unsigned int)*buf) & 0x03);
		EXPECT_SIZE (blen + 1, len, eof);
		READ_DINT (p->tag.count, buf+1, blen);
		p->type = SP_MSGPACK_BINARY;
		return blen + 1;

	case 0x07: // ext 8
	case 0x08: // ext 16
	case 0x09: // ext 32
		blen = 1 << ((((unsigned int)*buf) + 1) & 0x03);
		EXPECT_SIZE (blen + 2, len, eof);
		READ_DINT (p->tag.ext.len, buf+1, blen);
		p->tag.ext.type = buf[blen+1];
		p->type = SP_MSGPACK_EXT;
		return blen + 2;

	case 0x0a: // float
		EXPECT_SIZE (5, len, eof);
		READ_FLT (p->tag.f32, buf+1);
		p->type = SP_MSGPACK_FLOAT;
		return 5;

	case 0x0b: // double
		EXPECT_SIZE (9, len, eof);
		READ_DBL (p->tag.f64, buf+1);
		p->type = SP_MSGPACK_DOUBLE;
		return 9;

	case 0x0c: // unsigned int  8
		EXPECT_SIZE (2, len, eof);
		p->type = SP_MSGPACK_POSITIVE;
		p->tag.u64 = buf[1];
		return 2;
	case 0x0d: LOAD_UINT (p, 16, buf, len, eof);
	case 0x0e: LOAD_UINT (p, 32, buf, len, eof);
	case 0x0f: LOAD_UINT (p, 64, buf, len, eof);

	case 0x10: // signed int  8
		EXPECT_SIZE (2, len, eof);
		int8_t val = *(int8_t *)(buf+1);
		if (val < 0) {
			p->type = SP_MSGPACK_NEGATIVE;
			p->tag.i64 = val;
		}
		else {
			p->type = SP_MSGPACK_POSITIVE;
			p->tag.u64 = (uint8_t)val;
		}
		return 2;
	case 0x11: LOAD_SINT (p, 16, buf, len, eof);
	case 0x12: LOAD_SINT (p, 32, buf, len, eof);
	case 0x13: LOAD_SINT (p, 64, buf, len, eof);

	case 0x14: // fixext 1
	case 0x15: // fixext 2
	case 0x16: // fixext 4
	case 0x17: // fixext 8
		EXPECT_SIZE (2, len, eof);
		p->tag.ext.type = buf[1];
		p->tag.ext.len = (1 << (((unsigned int)*buf) & 0x03)) + 1;
		p->type = SP_MSGPACK_EXT;
		return 2;
	case 0x18: // fixext 16
		EXPECT_SIZE (2, len, eof);
		p->tag.ext.type = buf[1];
		p->tag.ext.len = 16;
		p->type = SP_MSGPACK_EXT;
		return 2;

	case 0x19: // str 8
	case 0x1a: // str 16
	case 0x1b: // str 32
		blen = 1 << ((((unsigned int)*buf) & 0x03) - 1);
		EXPECT_SIZE (blen + 1, len, eof);
		READ_DINT (p->tag.count, buf+1, blen);
		p->type = SP_MSGPACK_STRING;
		return blen + 1;

	case 0x1c: // array 16
	case 0x1d: // array 32
		blen = 2u << (((unsigned int)*buf) & 0x01);
		EXPECT_SIZE (blen + 1, len, eof);
		READ_DINT (p->tag.count, buf+1, blen);
		p->type = SP_MSGPACK_ARRAY;
		STACK_PUSH_ARR (p, p->tag.count);
		return blen + 1;

	case 0x1e: // map 16
	case 0x1f: // map 32
		blen = 2u << (((unsigned int)*buf) & 0x01);
		EXPECT_SIZE (blen + 1, len, eof);
		READ_DINT (p->tag.count, buf+1, blen);
		p->type = SP_MSGPACK_MAP;
		STACK_PUSH_MAP (p, p->tag.count);
		return blen + 1;

	}

	YIELD_ERROR (SP_ESYNTAX);
}

static inline ssize_t
next (SpMsgpack *p, const uint8_t *restrict buf, size_t len, bool eof)
{
	const uint8_t register c = *(uint8_t *)buf;

	if (c >> 7 == 0) {
		p->type = SP_MSGPACK_POSITIVE;
		p->tag.u64 = c;
		return 1;
	}

	switch (c >> 5) {
	case 5:
		p->type = SP_MSGPACK_STRING;
		p->tag.count = c & 0x1f;
		return 1;
	case 6:
		return next_tag (p, buf, len, eof);
	case 7:
		p->type = SP_MSGPACK_NEGATIVE;
		p->tag.i64 = (int8_t)c;
		return 1;
	}

	switch (c >> 4) {
	case 8:
		p->type = SP_MSGPACK_MAP;
		p->tag.count = c & 0xf;
		p->key = true;
		STACK_PUSH_MAP (p, p->tag.count);
		return 1;
	case 9:
		p->type = SP_MSGPACK_ARRAY;
		p->tag.count = c & 0xf;
		STACK_PUSH_ARR (p, p->tag.count);
		return 1;
	}

	YIELD_ERROR (SP_ESYNTAX);
}

ssize_t
sp_msgpack_next (SpMsgpack *p, const void *restrict buf, size_t len, bool eof)
{
	assert (p != NULL);
	assert (buf != NULL);

	ssize_t rc = 0;

	if (p->cs && !IS_DONE (p->cs)) {
		p->type = p->cs;
		p->cs = 0;
		if (p->depth == 0) {
			p->cs = DONE;
		}
	}
	else {
		p->type = SP_MSGPACK_NONE;
		if (len > 0) {
			rc = next (p, buf, len, eof);
		}
	}

	if (rc >= 0 && p->type > SP_MSGPACK_ARRAY) {
		if (p->depth > 0 && 
			(STACK_IN_ARR (p) || (p->key = !p->key)) &&
			--p->counts[p->depth-1] == 0)
		{
			p->cs = STACK_IN_ARR (p) ? SP_MSGPACK_ARRAY_END : SP_MSGPACK_MAP_END;
			p->depth--;
			p->key = IS_AT_KEY (p);
		}
	}

	return rc;
}

bool
sp_msgpack_is_done (const SpMsgpack *p)
{
	assert (p != NULL);

	return IS_DONE (p->cs);
}

size_t 
sp_msgpack_enc (SpMsgpackType type, const SpMsgpackTag *tag, void *buf)
{
	switch (type) {
	case SP_MSGPACK_NIL:
		return sp_msgpack_enc_nil (buf);
	case SP_MSGPACK_TRUE:
		return sp_msgpack_enc_true (buf);
	case SP_MSGPACK_FALSE:
		return sp_msgpack_enc_false (buf);
	case SP_MSGPACK_NEGATIVE:
		return sp_msgpack_enc_negative (buf, tag->i64);
	case SP_MSGPACK_POSITIVE:
		return sp_msgpack_enc_positive (buf, tag->u64);
	case SP_MSGPACK_FLOAT:
		return sp_msgpack_enc_float (buf, tag->f32);
	case SP_MSGPACK_DOUBLE:
		return sp_msgpack_enc_double (buf, tag->f64);
	case SP_MSGPACK_STRING:
		return sp_msgpack_enc_string (buf, tag->count);
	case SP_MSGPACK_BINARY:
		return sp_msgpack_enc_binary (buf, tag->count);
	case SP_MSGPACK_ARRAY:
		return sp_msgpack_enc_array (buf, tag->count);
	case SP_MSGPACK_MAP:
		return sp_msgpack_enc_map (buf, tag->count);
	case SP_MSGPACK_EXT:
		 return sp_msgpack_enc_ext (buf, tag->ext.type, tag->ext.len);
	default:             return 0;
	}
}

size_t 
sp_msgpack_enc_nil (void *buf)
{
	((uint8_t *)buf)[0] = 0xc0;
	return 1;
}

size_t
sp_msgpack_enc_false (void *buf)
{
	((uint8_t *)buf)[0] = 0xc2;
	return 1;
}

size_t
sp_msgpack_enc_true (void *buf)
{
	((uint8_t *)buf)[0] = 0xc3;
	return 1;
}

size_t
sp_msgpack_enc_negative (void *buf, int64_t val)
{
	if (val >= 0) {
		return sp_msgpack_enc_positive (buf, (uint64_t)val);
	}
	if (val >= -0x1f) {
		((uint8_t *)buf)[0] = (uint8_t)val | 0xe0;
		return 1;
	}
	if (val >= INT8_MIN) {
		WRITE_INT (buf, 0xd0, int8_t, val);
		return 2;
	}
	if (val >= INT16_MIN) {
		WRITE_INT (buf, 0xd1, int16_t, htobe16 (val));
		return 3;
	}
	if (val >= INT32_MIN) {
		WRITE_INT (buf, 0xd2, int32_t, htobe32 (val));
		return 5;
	}
	WRITE_INT (buf, 0xd3, int64_t, htobe64 (val));
	return 9;
}

size_t
sp_msgpack_enc_positive (void *buf, uint64_t val)
{
	if (val <= 0x7f) {
		((uint8_t *)buf)[0] = (uint8_t)val;
		return 1;
	}
	if (val <= UINT8_MAX) {
		WRITE_INT (buf, 0xcc, uint8_t, val);
		return 2;
	}
	if (val <= UINT16_MAX) {
		WRITE_INT (buf, 0xcd, uint16_t, htobe16 (val));
		return 3;
	}
	if (val <= UINT32_MAX) {
		WRITE_INT (buf, 0xce, uint32_t, htobe32 (val));
		return 5;
	}
	WRITE_INT (buf, 0xcf, uint64_t, htobe64 (val));
	return 9;
}

size_t
sp_msgpack_enc_float (void *buf, float val)
{
	uint8_t *p = buf;
	*p = 0xca;
	READ_FLT (*(float *)(p+1), &val);
	return 5;
}

size_t
sp_msgpack_enc_double (void *buf, double val)
{
	uint8_t *p = buf;
	*p = 0xcb;
	READ_DBL (*(double *)(p+1), &val);
	return 9;
}

size_t
sp_msgpack_enc_string (void *buf, uint32_t len)
{
	if (len <= 0x1f) {
		((uint8_t *)buf)[0] = (uint8_t)len | 0xa0;
		return 1;
	}
	if (len <= UINT8_MAX) {
		WRITE_INT (buf, 0xd9, uint8_t, len);
		return 2;
	}
	if (len <= UINT16_MAX) {
		WRITE_INT (buf, 0xda, uint16_t, htobe16 (len));
		return 3;
	}
	WRITE_INT (buf, 0xdb, uint32_t, htobe32 (len));
	return 5;
}

size_t
sp_msgpack_enc_binary (void *buf, uint32_t len)
{
	if (len <= UINT8_MAX) {
		WRITE_INT (buf, 0xc4, uint8_t, len);
		return 2;
	}
	if (len <= UINT16_MAX) {
		WRITE_INT (buf, 0xc5, uint16_t, htobe16 (len));
		return 3;
	}
	WRITE_INT (buf, 0xc6, uint32_t, htobe32 (len));
	return 5;
}

size_t
sp_msgpack_enc_array (void *buf, uint32_t count)
{
	if (count <= 0xf) {
		((uint8_t *)buf)[0] = (uint8_t)count | 0x90;
		return 1;
	}
	if (count <= UINT16_MAX) {
		WRITE_INT (buf, 0xdc, uint16_t, htobe16 (count));
		return 3;
	}
	WRITE_INT (buf, 0xdd, uint32_t, htobe32 (count));
	return 5;
}

size_t
sp_msgpack_enc_map (void *buf, uint32_t count)
{
	if (count <= 0xf) {
		((uint8_t *)buf)[0] = (uint8_t)count | 0x80;
		return 1;
	}
	if (count <= UINT16_MAX) {
		WRITE_INT (buf, 0xde, uint16_t, htobe16 (count));
		return 3;
	}
	WRITE_INT (buf, 0xdf, uint32_t, htobe32 (count));
	return 5;
}

size_t
sp_msgpack_enc_ext (void *buf, int8_t type, size_t len)
{
	(void)buf;
	(void)type;
	(void)len;
	return 0;
}

