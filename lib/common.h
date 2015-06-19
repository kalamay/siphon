#include "siphon/error.h"

#include "pcmp/eq16.h"
#include "pcmp/leq.h"
#include "pcmp/leq16.h"
#include "pcmp/set16.h"
#include "pcmp/range16.h"


#define DONE 0xFFFFFFFFU
#define IS_DONE(cs) (((cs) & 0xF0000000U) != 0)

#define LEQ(cmp, off, len) \
	(len == sizeof cmp - 1 && pcmp_leq (off, (uint8_t *)cmp, sizeof cmp - 1))

#define LEQ16(cmp, off, len) \
	(len == sizeof cmp - 1 && pcmp_leq16 (off, (uint8_t *)cmp, sizeof cmp - 1))


#define YIELD_ERROR(err) do { \
	p->cs = DONE;             \
	return err;               \
} while (0)

#define YIELD(typ, next) do { \
	p->cs = next;             \
	p->off = 0;               \
	p->type = typ;            \
	return end - m;           \
} while (0)


#define VERIFY_END(max, done) do {       \
	if (pcmp_unlikely (end == NULL)) {   \
		if (pcmp_unlikely (done)) {      \
			YIELD_ERROR (SP_ESYNTAX);    \
		}                                \
		p->off = len;                    \
		EXPECT_MAX_OFFSET (max);         \
		return 0;                        \
	}                                    \
} while (0)


#define EXPECT_MAX_OFFSET(max) do {                            \
	if (pcmp_unlikely (p->off > (size_t)max)) {                \
		YIELD_ERROR (SP_ESIZE);                                \
	}                                                          \
} while (0)

#define EXPECT_RANGE(rng, max, done) do {                      \
	end = pcmp_range16 (end, len-p->off, rng, sizeof rng - 1); \
	VERIFY_END (max, done);                                    \
	p->off = end - m;                                          \
	EXPECT_MAX_OFFSET (max);                                   \
} while (0)

#define EXPECT_SET(set, max, done) do {                        \
	end = pcmp_set16 (end, len-p->off, set, sizeof set - 1);   \
	VERIFY_END (max, done);                                    \
	p->off = end - m;                                          \
	EXPECT_MAX_OFFSET (max);                                   \
} while (0)

#define EXPECT_RANGE_THEN_CHAR(rng, ch, max, done) do {        \
	end = pcmp_range16 (end, len-p->off, rng, sizeof rng - 1); \
	VERIFY_END (max, done);                                    \
	if (pcmp_unlikely (*end != ch)) {                          \
		YIELD_ERROR (SP_ESYNTAX);                              \
	}                                                          \
	end++;                                                     \
	p->off = end - m;                                          \
	EXPECT_MAX_OFFSET (max+1);                                 \
} while (0)

#define EXPECT_SIZE(sz, done) do {                             \
	if (pcmp_unlikely (p->off + (sz) > len)) {                 \
		if (pcmp_unlikely (done)) {                            \
			YIELD_ERROR (SP_ESYNTAX);                          \
		}                                                      \
		return 0;                                              \
	}                                                          \
} while (0)

#define EXPECT_CHAR(ch, done) do {                             \
	EXPECT_SIZE (1, done);                                     \
	if (pcmp_unlikely (m[p->off] != ch)) {                     \
		YIELD_ERROR (SP_ESYNTAX);                              \
	}                                                          \
	end++;                                                     \
	p->off++;                                                  \
} while (0)

#define EXPECT_PREFIX(pre, extra, done) do {                   \
	if (len-p->off < sizeof pre - 1 + extra) {                 \
		if (pcmp_unlikely (done)) {                            \
			YIELD_ERROR (SP_ESYNTAX);                          \
		}                                                      \
		return 0;                                              \
	}                                                          \
	if (!pcmp_eq16 (end, pre, sizeof pre - 1)) {               \
		YIELD_ERROR (SP_ESYNTAX);                              \
	}                                                          \
	end = m + p->off + sizeof pre - 1;                         \
	p->off += (sizeof pre - 1) + extra;                        \
} while (0)

#define EXPECT_CRLF(max, done) do {                            \
	end = pcmp_set16 (end, len-p->off, (uint8_t *)"\r", 1);    \
	VERIFY_END (max, done);                                    \
	if (pcmp_unlikely ((size_t)(end - m) == len - 1)) {        \
		p->off = len - 1;                                      \
		return 0;                                              \
	}                                                          \
	if (pcmp_unlikely (end[1] != '\n')) {                      \
		YIELD_ERROR (SP_ESYNTAX);                              \
	}                                                          \
	end += 2;                                                  \
	p->off = end - m;                                          \
	EXPECT_MAX_OFFSET (max+2);                                 \
} while (0)

