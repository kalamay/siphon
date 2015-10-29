#include "../include/siphon/msgpack.h"
#include "../include/siphon/error.h"
#include "../include/siphon/endian.h"

#include <stdio.h>
#include <assert.h>



/**
 * tag bytes patterns
 */
#define B_NIL     0xc0 /* nil */
#define B_FALSE   0xc2 /* false */
#define B_TRUE    0xc3 /* true */
#define B_BIN8    0xc4 /* binary with 8-bit length */
#define B_BIN16   0xc5 /* binary with 16-bit length */
#define B_BIN32   0xc6 /* binary with 32-bit length */
#define B_EXT8    0xc7 /* extention with 8-bit length */
#define B_EXT16   0xc8 /* extention with 16-bit length */
#define B_EXT32   0xc9 /* extention with 32-bit length */
#define B_FLOAT   0xca /* float */
#define B_DOUBLE  0xcb /* double */
#define B_UINT8   0xcc /* 8-bit unsigned integer */
#define B_UINT16  0xcd /* 16-bit unsigned integer */
#define B_UINT32  0xce /* 32-bit unsigned integer */
#define B_UINT64  0xcf /* 64-bit unsigned integer */
#define B_INT8    0xd0 /* 8-bit signed integer */
#define B_INT16   0xd1 /* 16-bit signed integer */
#define B_INT32   0xd2 /* 32-bit signed integer */
#define B_INT64   0xd3 /* 64-bit signed integer */
#define B_FEXT8   0xd4 /* 8-bit extension */
#define B_FEXT16  0xd5 /* 16-bit extension */
#define B_FEXT32  0xd6 /* 32-bit extension */
#define B_FEXT64  0xd7 /* 64-bit extension */
#define B_FEXT128 0xd8 /* 128-bit extension */
#define B_STR8    0xd9 /* string with 8-bit length */
#define B_STR16   0xda /* string with 16-bit length */
#define B_STR32   0xdb /* string with 32-bit length */
#define B_ARR16   0xdc /* array with 16-bit count */
#define B_ARR32   0xdd /* array with 32-bit count */
#define B_MAP16   0xde /* map with 16-bit count */
#define B_MAP32   0xdf /* map with 32-bit count */
#define B_IS_NF(n)     /* test if the tag is non-fixed */ \
	(((n) & 0xc0) == 0xc0 && (n) != 0xc1)

/**
 * "fixed" byte patterns (value is encoded in tag)
 */
#define B_FINT_MIN -31                   /* smallest fixed integer */
#define B_IS_FINT(n) (((n)&0xe0)==0xe0)  /* test if the tag is a fixed integer */
#define B_FINT(n) ((uint8_t)(n) | 0xe0)  /* convert fixed integer into tag */
#define B_FINT_VAL(n) ((int8_t)(n))      /* convert fixed inteter tag into value */

#define B_FUINT_MAX 127                  /* largest fixed unsigned */
#define B_IS_FUINT(n) (((n) >> 7) == 0)  /* test if the tag is a fixed unsigned */
#define B_FUINT(n) ((uint8_t)(n))        /* convert fixed unsigned into tag */
#define B_FUINT_VAL(n) ((uint8_t)(n))    /* convert fixed unsigned tag into value */

#define B_FSTR_MAX 31                    /* longest fixed string */
#define B_IS_FSTR(n) (((n)&0xe0)==0xa0)  /* test if the tag is a fixed string */
#define B_FSTR(n) ((uint8_t)(n) | 0xa0)  /* convert fixed string into tag */
#define B_FSTR_LEN(n) ((n) & B_FSTR_MAX) /* convert fixed string tag into length */

#define B_FARR_MAX 15                    /* longest fixed array */
#define B_IS_FARR(n) (((n)&0xf0)==0x90)  /* test if the tag is a fixed array */
#define B_FARR(n) ((uint8_t)(n) | 0x90)  /* convert fixed array into tag */
#define B_FARR_LEN(n) ((n) & B_FARR_MAX) /* convert fixed array tag into count */

#define B_FMAP_MAX 15                    /* longest fixed map */
#define B_IS_FMAP(n) (((n)&0xf0)==0x80)  /* test if the tag is a fixed map */
#define B_FMAP(n) ((uint8_t)(n) | 0x80)  /* convert fixed map into tag */
#define B_FMAP_LEN(n) ((n) & B_FMAP_MAX) /* convert fixed map tag into count */



#define DONE 0xFFFFFFFFU
#define IS_DONE(cs) (((cs) & 0xF0000000U) != 0)

#define YIELD_ERROR(err) do { \
	p->cs = DONE;             \
	return err;               \
} while (0)

#define EXPECT_SIZE(sz, remain, done) do { \
	if ((sz) > remain) {                   \
		if (done) {                        \
			YIELD_ERROR (SP_MSGPACK_ESYNTAX);      \
		}                                  \
		return 0;                          \
	}                                      \
} while (0)

#define VERIFY_LEN(off, exp, remain, done) do {                \
	ssize_t _off = (ssize_t)(off);                             \
	if ((done) && _off + (ssize_t)(exp) > (ssize_t)(remain)) { \
		YIELD_ERROR (SP_MSGPACK_ESYNTAX);                              \
	}                                                          \
	return _off;                                               \
} while (0)

/**
 * Loads an unsigned integer into the tag.
 * @p: parser pointer
 * @bits: integer bit size constant
 * @src: byte stream source
 * @len: byte length available on stream
 * @eof: if the byte length mark the end of input
 */
#define LOAD_UINT(p, bits, src, len, eof) do {                                    \
	EXPECT_SIZE (bits/8 + 1, len, eof);                                           \
	p->type = SP_MSGPACK_UNSIGNED;                                                \
	p->tag.u64 = (uint##bits##_t)sp_be##bits##toh (*(uint##bits##_t *)((src)+1)); \
	return bits/8 + 1;                                                            \
} while (0)

/**
 * Loads a signed integer into the tag.
 * @p: parser pointer
 * @bits: integer bit size constant
 * @src: byte stream source
 * @len: byte length available on stream
 * @eof: if the byte length mark the end of input
 */
#define LOAD_SINT(p, bits, src, len, eof) do {                                   \
	EXPECT_SIZE (bits/8 + 1, len, eof);                                          \
	int64_t val = (int##bits##_t)sp_be##bits##toh (*(int##bits##_t *)((src)+1)); \
	if (val < 0) {                                                               \
		p->type = SP_MSGPACK_SIGNED;                                             \
		p->tag.i64 = val;                                                        \
	}                                                                            \
	else {                                                                       \
		p->type = SP_MSGPACK_UNSIGNED;                                           \
		p->tag.u64 = (uint64_t)val;                                              \
	}                                                                            \
	return bits/8 + 1;                                                           \
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
	(dst) = sp_be64toh (out);                           \
} while (0)

#if BYTE_ORDER == LITTLE_ENDIAN

/**
 * Reads a 32-bit float
 * @dst: value to store the read value
 * @src: the read position of the buffer
 */
# define READ_FLT(dst, src) do {         \
	union { uint32_t i; float f; } mem;  \
	mem.f = *(float *)(src);             \
	mem.i = __builtin_bswap32 (mem.i);   \
	(dst) = mem.f;                       \
} while (0)

/**
 * Reads a 64-bit double
 * @dst: value to store the read value
 * @src: the read position of the buffer
 */
# define READ_DBL(dst, src) do {         \
	union { uint64_t i; double f; } mem; \
	mem.f = *(double *)(src);            \
	mem.i = __builtin_bswap64 (mem.i);   \
	(dst) = mem.f;                       \
} while (0)

#elif BYTE_ORDER == BIG_ENDIAN

/**
 * Reads a 32-bit float
 * @dst: value to store the read value
 * @src: the read position of the buffer
 */
# define READ_FLT(dst, src) do { \
	(dst) = (*(float *)(src));   \
} while (0)

/**
 * Reads a 64-bit double
 * @dst: value to store the read value
 * @src: the read position of the buffer
 */
# define READ_DBL(dst, src) do { \
	(dst) = (*(double *)(src));  \
} while (0)

#endif

/**
 * Writes a big-endian integer into the buffer
 * @buf: buffer to write into
 * @tag: the tag byte
 * @T: integer type (uint32_t, int16_t, etc.)
 * @val: big-endian integer value
 */
#define WRITE_INT(buf, tag, T, val) do {   \
	*(uint8_t *)buf = (tag);               \
	*(T *)((uint8_t *)(buf) + 1) = (T)val; \
} while (0)

#include "stack.h"

#define STACK_ARR 0
#define STACK_MAP 1

#define PUSH_COUNT(p, n) do { p->counts[p->depth-1] = (n); } while (0)

#define STACK_PUSH_ARR(p, n) do {           \
	STACK_PUSH_FALSE(p, SP_MSGPACK_ESTACK); \
	PUSH_COUNT(p, n);                       \
} while (0)
#define STACK_PUSH_MAP(p, n) do {           \
	STACK_PUSH_TRUE(p, SP_MSGPACK_ESTACK);  \
	PUSH_COUNT(p, n);                       \
} while (0)

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
next (SpMsgpack *p, const uint8_t *restrict buf, size_t len, bool eof)
{
	uint8_t c = *(uint8_t *)buf;

	if (B_IS_FUINT (c)) {
		p->type = SP_MSGPACK_UNSIGNED;
		p->tag.u64 = B_FUINT_VAL (c);
		return 1;
	}

	if (B_IS_FINT (c)) {
		p->type = SP_MSGPACK_SIGNED;
		p->tag.i64 = B_FINT_VAL (c);
		return 1;
	}

	if (B_IS_FSTR (c)) {
		p->type = SP_MSGPACK_STRING;
		p->tag.count = B_FSTR_LEN (c);
		VERIFY_LEN (1, p->tag.count, len, eof);
		return 1;
	}

	if (B_IS_FMAP (c)) {
		p->type = SP_MSGPACK_MAP;
		p->tag.count = B_FMAP_LEN (c);
		p->key = true;
		STACK_PUSH_MAP (p, p->tag.count);
		return 1;
	}

	if (B_IS_FARR (c)) {
		p->type = SP_MSGPACK_ARRAY;
		p->tag.count = B_FARR_LEN (c);
		STACK_PUSH_ARR (p, p->tag.count);
		return 1;
	}

	unsigned blen = 0;

	switch (c) {

	case B_NIL:
		p->type = SP_MSGPACK_NIL;
		return 1;

	case B_FALSE:
		p->type = SP_MSGPACK_FALSE;
		return 1;

	case B_TRUE:
		p->type = SP_MSGPACK_TRUE;
		return 1;

	case B_BIN8:
	case B_BIN16:
	case B_BIN32:
		blen = 1 << ((unsigned)c & 0x03);
		EXPECT_SIZE (blen + 1, len, eof);
		READ_DINT (p->tag.count, buf+1, blen);
		p->type = SP_MSGPACK_BINARY;
		VERIFY_LEN (blen + 1, p->tag.count, len, eof);

	case B_EXT8:
	case B_EXT16:
	case B_EXT32:
		blen = 1 << (((unsigned)c + 1) & 0x03);
		EXPECT_SIZE (blen + 2, len, eof);
		READ_DINT (p->tag.ext.len, buf+1, blen);
		p->tag.ext.type = buf[blen+1];
		p->type = SP_MSGPACK_EXT;
		VERIFY_LEN (blen + 2, p->tag.ext.len, len, eof);

	case B_FLOAT:
		EXPECT_SIZE (5, len, eof);
		READ_FLT (p->tag.f32, buf+1);
		p->type = SP_MSGPACK_FLOAT;
		return 5;

	case B_DOUBLE:
		EXPECT_SIZE (9, len, eof);
		READ_DBL (p->tag.f64, buf+1);
		p->type = SP_MSGPACK_DOUBLE;
		return 9;

	case B_UINT8:
		EXPECT_SIZE (2, len, eof);
		p->type = SP_MSGPACK_UNSIGNED;
		p->tag.u64 = buf[1];
		return 2;
	case B_UINT16: LOAD_UINT (p, 16, buf, len, eof);
	case B_UINT32: LOAD_UINT (p, 32, buf, len, eof);
	case B_UINT64: LOAD_UINT (p, 64, buf, len, eof);

	case B_INT8:
		EXPECT_SIZE (2, len, eof);
		int8_t val = *(int8_t *)(buf+1);
		if (val < 0) {
			p->type = SP_MSGPACK_SIGNED;
			p->tag.i64 = val;
		}
		else {
			p->type = SP_MSGPACK_UNSIGNED;
			p->tag.u64 = (uint8_t)val;
		}
		return 2;
	case B_INT16: LOAD_SINT (p, 16, buf, len, eof);
	case B_INT32: LOAD_SINT (p, 32, buf, len, eof);
	case B_INT64: LOAD_SINT (p, 64, buf, len, eof);

	case B_FEXT8:
	case B_FEXT16:
	case B_FEXT32:
	case B_FEXT64:
		EXPECT_SIZE (2, len, eof);
		p->tag.ext.type = buf[1];
		p->tag.ext.len = 1 << ((unsigned)c & 0x03);
		p->type = SP_MSGPACK_EXT;
		VERIFY_LEN (2, p->tag.ext.len, len, eof);
	case B_FEXT128:
		EXPECT_SIZE (2, len, eof);
		p->tag.ext.type = buf[1];
		p->tag.ext.len = 16;
		p->type = SP_MSGPACK_EXT;
		VERIFY_LEN (2, p->tag.ext.len, len, eof);

	case B_STR8:
	case B_STR16:
	case B_STR32:
		blen = 1 << (((unsigned)c & 0x03) - 1);
		EXPECT_SIZE (blen + 1, len, eof);
		READ_DINT (p->tag.count, buf+1, blen);
		p->type = SP_MSGPACK_STRING;
		VERIFY_LEN (blen + 1, p->tag.count, len, eof);

	case B_ARR16:
	case B_ARR32:
		blen = 2u << ((unsigned)c & 0x01);
		EXPECT_SIZE (blen + 1, len, eof);
		READ_DINT (p->tag.count, buf+1, blen);
		p->type = SP_MSGPACK_ARRAY;
		STACK_PUSH_ARR (p, p->tag.count);
		return blen + 1;

	case B_MAP16:
	case B_MAP32:
		blen = 2u << ((unsigned)c & 0x01);
		EXPECT_SIZE (blen + 1, len, eof);
		READ_DINT (p->tag.count, buf+1, blen);
		p->type = SP_MSGPACK_MAP;
		STACK_PUSH_MAP (p, p->tag.count);
		return blen + 1;

	}

	YIELD_ERROR (SP_MSGPACK_ESYNTAX);
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
		else if (eof) {
			YIELD_ERROR (SP_MSGPACK_ESYNTAX);
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
	case SP_MSGPACK_SIGNED:
		return sp_msgpack_enc_signed (buf, tag->i64);
	case SP_MSGPACK_UNSIGNED:
		return sp_msgpack_enc_unsigned (buf, tag->u64);
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
	default:
		 return 0;
	}
}

size_t 
sp_msgpack_enc_nil (void *buf)
{
	*(uint8_t *)buf = B_NIL;
	return 1;
}

size_t
sp_msgpack_enc_false (void *buf)
{
	*(uint8_t *)buf = B_FALSE;
	return 1;
}

size_t
sp_msgpack_enc_true (void *buf)
{
	*(uint8_t *)buf = B_TRUE;
	return 1;
}

size_t
sp_msgpack_enc_signed (void *buf, int64_t val)
{
	if (val >= 0) {
		return sp_msgpack_enc_unsigned (buf, (uint64_t)val);
	}
	if (val >= B_FINT_MIN) {
		*(uint8_t *)buf = B_FINT (val);
		return 1;
	}
	if (val >= INT8_MIN) {
		WRITE_INT (buf, B_INT8, int8_t, val);
		return 2;
	}
	if (val >= INT16_MIN) {
		WRITE_INT (buf, B_INT16, int16_t, sp_htobe16 (val));
		return 3;
	}
	if (val >= INT32_MIN) {
		WRITE_INT (buf, B_INT32, int32_t, sp_htobe32 (val));
		return 5;
	}
	WRITE_INT (buf, B_INT64, int64_t, sp_htobe64 (val));
	return 9;
}

size_t
sp_msgpack_enc_unsigned (void *buf, uint64_t val)
{
	if (val <= B_FUINT_MAX) {
		*(uint8_t *)buf = B_FUINT (val);
		return 1;
	}
	if (val <= UINT8_MAX) {
		WRITE_INT (buf, B_UINT8, uint8_t, val);
		return 2;
	}
	if (val <= UINT16_MAX) {
		WRITE_INT (buf, B_UINT16, uint16_t, sp_htobe16 (val));
		return 3;
	}
	if (val <= UINT32_MAX) {
		WRITE_INT (buf, B_UINT32, uint32_t, sp_htobe32 (val));
		return 5;
	}
	WRITE_INT (buf, B_UINT64, uint64_t, sp_htobe64 (val));
	return 9;
}

size_t
sp_msgpack_enc_float (void *buf, float val)
{
	uint8_t *p = buf;
	*p = B_FLOAT;
	READ_FLT (*(float *)(p+1), &val);
	return 5;
}

size_t
sp_msgpack_enc_double (void *buf, double val)
{
	uint8_t *p = buf;
	*p = B_DOUBLE;
	READ_DBL (*(double *)(p+1), &val);
	return 9;
}

size_t
sp_msgpack_enc_string (void *buf, uint32_t len)
{
	if (len <= B_FSTR_MAX) {
		*(uint8_t *)buf = B_FSTR (len);
		return 1;
	}
	if (len <= UINT8_MAX) {
		WRITE_INT (buf, B_STR8, uint8_t, len);
		return 2;
	}
	if (len <= UINT16_MAX) {
		WRITE_INT (buf, B_STR16, uint16_t, sp_htobe16 (len));
		return 3;
	}
	WRITE_INT (buf, B_STR32, uint32_t, sp_htobe32 (len));
	return 5;
}

size_t
sp_msgpack_enc_binary (void *buf, uint32_t len)
{
	if (len <= UINT8_MAX) {
		WRITE_INT (buf, B_BIN8, uint8_t, len);
		return 2;
	}
	if (len <= UINT16_MAX) {
		WRITE_INT (buf, B_BIN16, uint16_t, sp_htobe16 (len));
		return 3;
	}
	WRITE_INT (buf, B_BIN32, uint32_t, sp_htobe32 (len));
	return 5;
}

size_t
sp_msgpack_enc_array (void *buf, uint32_t count)
{
	if (count <= B_FARR_MAX) {
		*(uint8_t *)buf = B_FARR (count);
		return 1;
	}
	if (count <= UINT16_MAX) {
		WRITE_INT (buf, B_ARR16, uint16_t, sp_htobe16 (count));
		return 3;
	}
	WRITE_INT (buf, B_ARR32, uint32_t, sp_htobe32 (count));
	return 5;
}

size_t
sp_msgpack_enc_map (void *buf, uint32_t count)
{
	if (count <= B_FMAP_MAX) {
		*(uint8_t *)buf = B_FMAP (count);
		return 1;
	}
	if (count <= UINT16_MAX) {
		WRITE_INT (buf, B_MAP16, uint16_t, sp_htobe16 (count));
		return 3;
	}
	WRITE_INT (buf, B_MAP32, uint32_t, sp_htobe32 (count));
	return 5;
}

size_t
sp_msgpack_enc_ext (void *buf, int8_t type, uint32_t len)
{
	if (type < 0) {
		return 0;
	}

	size_t off = 1;

	if (len == 1)       { *(uint8_t *)buf = B_FEXT8; }
	else if (len == 2)  { *(uint8_t *)buf = B_FEXT16; }
	else if (len == 4)  { *(uint8_t *)buf = B_FEXT32; }
	else if (len == 8)  { *(uint8_t *)buf = B_FEXT64; }
	else if (len == 16) { *(uint8_t *)buf = B_FEXT128; }
	else if (len <= UINT8_MAX) {
		WRITE_INT (buf, B_EXT8, uint8_t, len);
		off = 2;
	}
	else if (len <= UINT16_MAX) {
		WRITE_INT (buf, B_EXT16, uint16_t, sp_htobe16 (len));
		off = 3;
	}
	else {
		WRITE_INT (buf, B_EXT32, uint32_t, sp_htobe32 (len));
		off = 5;
	}

	((int8_t *)buf)[off] = type;
	return off + 1;
}

