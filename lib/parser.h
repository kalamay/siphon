#include "../include/siphon/error.h"

#include "pcmp/eq16.h"
#include "pcmp/leq.h"
#include "pcmp/leq16.h"
#include "pcmp/set16.h"
#include "pcmp/range16.h"


#define LEQ(cmp, off, len) \
	(len == sizeof cmp - 1 && pcmp_leq (off, (uint8_t *)cmp, sizeof cmp - 1))

#define LEQ16(cmp, off, len) \
	(len == sizeof cmp - 1 && pcmp_leq16 (off, (uint8_t *)cmp, sizeof cmp - 1))


#define DONE 0xFFFFFFFFU
#define IS_DONE(cs) (((cs) & 0xF0000000U) != 0)

// intra-function scan offset
#define SCAN 0
// bytes remaining to be scanned
#define REMAIN (len - p->off - SCAN)


#define CAPTURE_OFFSET() do {  \
	p->off = (end - m - SCAN); \
} while (0)

#define YIELD_ERROR(err) do {  \
	p->cs = DONE;              \
	return err;                \
} while (0)

#define CHECK_ERROR(exp) do {  \
	int rc = (exp);            \
	if (rc < 0) {              \
		YIELD_ERROR (rc);      \
	}                          \
} while (0)

#define YIELD(typ, next) do {  \
	p->cs = next;              \
	p->off = 0;                \
	p->type = typ;             \
	return end - m;            \
} while (0)


#define VERIFY_END(max, done, esyn, esize) do {                \
	if (pcmp_unlikely (end == NULL)) {                         \
		if (pcmp_unlikely (done)) {                            \
			YIELD_ERROR (esyn);                                \
		}                                                      \
		p->off = len - SCAN;                                   \
		EXPECT_MAX_OFFSET (max, esize);                        \
		return SCAN;                                           \
	}                                                          \
} while (0)


#define EXPECT_MAX_OFFSET(max, esize) do {                     \
	if (pcmp_unlikely (p->off > (size_t)max)) {                \
		YIELD_ERROR (esize);                                   \
	}                                                          \
} while (0)

#define EXPECT_RANGE(rng, max, done, esyn, esize) do {         \
	end = pcmp_range16 (end, REMAIN, rng, sizeof rng - 1);     \
	VERIFY_END (max, done, esyn, esize);                       \
	CAPTURE_OFFSET ();                                         \
	EXPECT_MAX_OFFSET (max, esize);                            \
} while (0)

#define EXPECT_SET(set, max, done, esyn, esize) do {           \
	end = pcmp_set16 (end, REMAIN, set, sizeof set - 1);       \
	VERIFY_END (max, done, esyn, esize);                       \
	CAPTURE_OFFSET ();                                         \
	EXPECT_MAX_OFFSET (max, esize);                            \
} while (0)

#define EXPECT_RANGE_THEN_CHAR(rng, ch, max, done, esyn, esize) do { \
	end = pcmp_range16 (end, REMAIN, rng, sizeof rng - 1);           \
	VERIFY_END (max, done, esyn, esize);                             \
	if (pcmp_unlikely (*end != ch)) {                                \
		YIELD_ERROR (esyn);                                          \
	}                                                                \
	end++;                                                           \
	CAPTURE_OFFSET ();                                               \
	EXPECT_MAX_OFFSET (max+1, esize);                                \
} while (0)

#define EXPECT_SIZE(sz, done, esyn) do {                       \
	if (pcmp_unlikely (p->off + (sz) > len)) {                 \
		if (pcmp_unlikely (done)) {                            \
			YIELD_ERROR (esyn);                                \
		}                                                      \
		return SCAN;                                           \
	}                                                          \
} while (0)

#define EXPECT_CHAR(ch, done, esyn) do {                       \
	EXPECT_SIZE (1, done, esyn);                               \
	if (pcmp_unlikely (*end != ch)) {                          \
		YIELD_ERROR (esyn);                                    \
	}                                                          \
	end++;                                                     \
	p->off++;                                                  \
} while (0)

#define EXPECT_PREFIX(pre, extra, done, esyn) do {             \
	if (REMAIN < sizeof pre - 1 + extra) {                     \
		if (pcmp_unlikely (done)) {                            \
			YIELD_ERROR (esyn);                                \
		}                                                      \
		return SCAN;                                           \
	}                                                          \
	if (!pcmp_eq16 (end, pre, sizeof pre - 1)) {               \
		YIELD_ERROR (esyn);                                    \
	}                                                          \
	end = m + p->off + SCAN + sizeof pre - 1;                  \
	p->off += (sizeof pre - 1) + extra;                        \
} while (0)

#define EXPECT_CRLF(max, done, esyn, esize) do {               \
	end = pcmp_set16 (end, REMAIN, (uint8_t *)"\r", 1);        \
	VERIFY_END (max, done, esyn, esize);                       \
	if (pcmp_unlikely ((size_t)(end - m) == len - 1)) {        \
		p->off = len - 1 - SCAN;                               \
		return SCAN;                                           \
	}                                                          \
	if (pcmp_unlikely (end[1] != '\n')) {                      \
		YIELD_ERROR (esyn);                                    \
	}                                                          \
	end += 2;                                                  \
	CAPTURE_OFFSET ();                                         \
	EXPECT_MAX_OFFSET (max+2, esize);                          \
} while (0)

