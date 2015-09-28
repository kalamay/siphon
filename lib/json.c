#include "siphon/json.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>


static const uint8_t rng_non_ws[] = "\x00\x08\x0b\x0c\x0e\x1f\x21\xff";


#include "stack.h"

#define STACK_ARR 0
#define STACK_OBJ 1

#define STACK_PUSH_ARR STACK_PUSH_FALSE
#define STACK_PUSH_OBJ STACK_PUSH_TRUE

#define STACK_IN_ARR(p) (STACK_TOP(p) == STACK_ARR)
#define STACK_IN_OBJ(p) (STACK_TOP(p) == STACK_OBJ)

#define STACK_POP_ARR(p) STACK_POP(p, STACK_ARR)
#define STACK_POP_OBJ(p) STACK_POP(p, STACK_OBJ)


#define KIND_MASK    0x00000F
#define KIND_ANY     0x000000
#define KIND_KEY     0x000001
#define KIND_STRING  0x000002
#define KIND_NUMBER  0x000003
#define KIND_TRUE    0x000004
#define KIND_FALSE   0x000005
#define KIND_NULL    0x000006
 
#define ARRAY_MASK   0x0000F0
#define ARRAY_FIRST  0x000010
#define ARRAY_NEXT   0x000020
 
#define OBJECT_MASK  0x000F00
#define OBJECT_FIRST 0x000100
#define OBJECT_KEY   0x000200
#define OBJECT_SEP   0x000300
#define OBJECT_NEXT  0x000400

#define STACK_CS(p) \
	(STACK_IN_OBJ (p) ? OBJECT_NEXT : ARRAY_NEXT)

#define YIELD_CS(p) \
	(p->depth ? STACK_CS (p) : DONE)

#define YIELD_STACK(typ) \
	YIELD (typ, YIELD_CS (p))

#define YIELD_IF_POP_ARR() do {            \
	if (pcmp_likely (STACK_POP_ARR (p))) { \
		end++;                             \
		p->off++;                          \
		YIELD_STACK (SP_JSON_ARRAY_END);   \
	}                                      \
} while (0)

#define YIELD_IF_POP_OBJ() do {            \
	if (pcmp_likely (STACK_POP_OBJ (p))) { \
		end++;                             \
		p->off++;                          \
		YIELD_STACK (SP_JSON_OBJECT_END);  \
	}                                      \
} while (0)

#define SKIP_WHITESPACE(done) do {                                           \
	end = pcmp_range16 (end, len-p->off, rng_non_ws, sizeof rng_non_ws - 1); \
	if (pcmp_unlikely (end == NULL)) {                                       \
		if (pcmp_unlikely (done)) {                                          \
			YIELD_ERROR (SP_ESYNTAX);                                        \
		}                                                                    \
		p->off = 0;                                                          \
		return len;                                                          \
	}                                                                        \
	p->off = end - m;                                                        \
} while (0)



static ssize_t
parse_string (SpJson *restrict p, const uint8_t *restrict m, size_t len, bool eof)
{
	// matches escape sequences, ASCII control, quote, or start of UTF-8 sequence
#ifdef SP_JSON_STRICT
	static const uint8_t rng_check[] = SP_UTF8_JSON_RANGE "\"\"";
#else
	static const uint8_t rng_check[] = "\\\\\x00\x1F\"\"\x7F\x7F";
#endif

	const uint8_t *start = m + p->mark;
	const uint8_t *end = m + p->off;
	ssize_t n;

again:
	EXPECT_RANGE (rng_check, SP_JSON_MAX_STRING, eof);
	end = pcmp_range16 (end, len-p->off, rng_check, sizeof rng_check - 1);
	if (end == NULL) {
		end = m + len;
	}
	p->off = end - m;
	EXPECT_MAX_OFFSET (SP_JSON_MAX_STRING);

	n = sp_utf8_add_raw (&p->utf8, start, end - start);
	if (n < 0) {
		YIELD_ERROR (n);
	}

	start = end;
	p->mark += n;

	if (*end == '"') {
		end++;
		YIELD (SP_JSON_STRING, p->cs == KIND_KEY ? OBJECT_SEP : YIELD_CS (p));
	}

	n = sp_utf8_json_decode_next (&p->utf8, end, len - p->off);
	if (n == SP_ETOOSHORT) {
		if (!eof) {
			n = (ssize_t)p->off;
			p->off = 0;
			p->mark = 0;
			return n;
		}
		n = SP_EESCAPE;
	}
	if (n < 0) {
		YIELD_ERROR (n);
	}

	end += n;
	start = end;
	p->off += n;
	p->mark += n;
	goto again;
}

static ssize_t
parse_number (SpJson *restrict p, const uint8_t *restrict m, size_t len, bool eof)
{
	// TODO: use better number parser

	static const uint8_t rng_non_num[] = "\x00\x2a\x2c\x2c\x2f\x2f\x3a\x44\x46\x64\x66\xff";

	const uint8_t *end = m + p->off;

	EXPECT_RANGE (rng_non_num, 511, eof);

	size_t n = p->off - p->mark;

	char buf[512];
	memcpy (buf, m+p->mark, n);
	buf[n] = '\0';

	char *e;
	p->number = strtod (buf, &e);
	if (*e) {
		YIELD_ERROR (SP_ESYNTAX);
	}
	YIELD_STACK (SP_JSON_NUMBER);
}

static size_t
parse_true (SpJson *restrict p, const uint8_t *restrict m, size_t len, bool eof)
{
	static const uint8_t pre_true[] = "rue";

	const uint8_t *end = m + p->off;

	EXPECT_PREFIX (pre_true, 0, eof);
	YIELD_STACK (SP_JSON_TRUE);
}

static size_t
parse_false (SpJson *restrict p, const uint8_t *restrict m, size_t len, bool eof)
{
	static const uint8_t pre_false[] = "alse";

	const uint8_t *end = m + p->off;

	EXPECT_PREFIX (pre_false, 0, eof);
	YIELD_STACK (SP_JSON_FALSE);
}

static size_t
parse_null (SpJson *restrict p, const uint8_t *restrict m, size_t len, bool eof)
{
	static const uint8_t pre_null[] = "ull";

	const uint8_t *end = m + p->off;

	EXPECT_PREFIX (pre_null, 0, eof);
	YIELD_STACK (SP_JSON_NULL);
}

static ssize_t
parse_any (SpJson *restrict p, const uint8_t *restrict m, size_t len, bool eof)
{
	const uint8_t *end = m + p->off;

	SKIP_WHITESPACE (eof);

	switch (*end) {
	case '{':
		STACK_PUSH_OBJ (p);
		end = m + ++p->off;
		YIELD (SP_JSON_OBJECT, OBJECT_FIRST);

	case '[':
		STACK_PUSH_ARR (p);
		end = m + ++p->off;
		YIELD (SP_JSON_ARRAY, ARRAY_FIRST);

	case '"':
		sp_utf8_reset (&p->utf8);
		p->mark = ++p->off;
		p->cs = KIND_STRING;
		return parse_string (p, m, len, eof);

	case '-': case '.': case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7': case '8': case '9':
		p->mark = p->off++;
		p->cs = KIND_NUMBER;
		return parse_number (p, m, len, eof);

	case 't':
		p->off++;
		p->cs = KIND_TRUE;
		return parse_true (p, m, len, eof);

	case 'f':
		p->off++;
		p->cs = KIND_FALSE;
		return parse_false (p, m, len, eof);

	case 'n':
		p->off++;
		p->cs = KIND_NULL;
		return parse_null (p, m, len, eof);
	}
	YIELD_ERROR (SP_ESYNTAX);
}

static ssize_t
parse_array (SpJson *restrict p, const uint8_t *restrict m, size_t len, bool eof)
{
	const uint8_t *end = m + p->off;

	SKIP_WHITESPACE (eof);
	EXPECT_SIZE (1, eof);

	switch (p->cs & ARRAY_MASK) {
	case ARRAY_FIRST:
		if (*end == ']') {
			YIELD_IF_POP_ARR ();
		}
		else {
			p->cs = KIND_ANY;
			return parse_any (p, m, len, eof);
		}
		break;

	case ARRAY_NEXT:
		switch (*end) {
		case ']':
			YIELD_IF_POP_ARR ();
			break;
		case ',':
			p->off++;
			p->cs = KIND_ANY;
			return parse_any (p, m, len, eof);
		}
		break;
	}

	YIELD_ERROR (SP_ESYNTAX);
}

static ssize_t
parse_object (SpJson *restrict p, const uint8_t *restrict m, size_t len, bool eof)
{
	const uint8_t *end = m + p->off;

again:
	SKIP_WHITESPACE (eof);


	switch (p->cs & OBJECT_MASK) {
	case OBJECT_FIRST:
		EXPECT_SIZE (1, eof);
		switch (*end) {
			case '}':
				YIELD_IF_POP_OBJ ();
				break;
			case '"':
				sp_utf8_reset (&p->utf8);
				p->mark = ++p->off;
				p->cs = KIND_KEY;
				return parse_string (p, m, len, eof);
		}
		break;

	case OBJECT_KEY:
		EXPECT_CHAR ('"', eof);
		sp_utf8_reset (&p->utf8);
		p->mark = p->off;
		p->cs = KIND_KEY;
		return parse_string (p, m, len, eof);

	case OBJECT_SEP:
		EXPECT_CHAR (':', eof);
		p->cs = KIND_ANY;
		return parse_any (p, m, len, eof);

	case OBJECT_NEXT:
		EXPECT_SIZE (1, eof);
		switch (*end) {
			case '}':
				YIELD_IF_POP_OBJ ();
				break;
			case ',':
				p->off++;
				end++;
				p->cs = OBJECT_KEY;
				goto again;
		}
		break;
	}

	YIELD_ERROR (SP_ESYNTAX);
}

static inline void
init_values (SpJson *p)
{
	p->number = 0.0;
	p->type = SP_JSON_NONE;
	p->cs = 0;
	p->off = 0;
	p->mark = 0;
	p->depth = 0;
}

void
sp_json_init (SpJson *p)
{
	assert (p != NULL);

	sp_utf8_init (&p->utf8);
	init_values (p);
}

void
sp_json_reset (SpJson *p)
{
	assert (p != NULL);

	sp_utf8_reset (&p->utf8);
	init_values (p);
}

void
sp_json_final (SpJson *p)
{
	assert (p != NULL);

	sp_utf8_final (&p->utf8);
}

ssize_t
sp_json_next (SpJson *p, const void *restrict buf, size_t len, bool eof)
{
	assert (p != NULL);

	p->type = SP_JSON_NONE;
	EXPECT_SIZE (1, eof);

	if (p->cs & ARRAY_MASK) {
		return parse_array (p, buf, len, eof);
	}
	if (p->cs & OBJECT_MASK) {
		return parse_object (p, buf, len, eof);
	}
	switch (p->cs) {
	case KIND_ANY:    return parse_any (p, buf, len, eof);
	case KIND_KEY:    /* fallthrough */
	case KIND_STRING: return parse_string (p, buf, len, eof);
	case KIND_NUMBER: return parse_number (p, buf, len, eof);
	case KIND_TRUE:   return parse_true (p, buf, len, eof);
	case KIND_FALSE:  return parse_false (p, buf, len, eof);
	case KIND_NULL:   return parse_null (p, buf, len, eof);
	}
	YIELD_ERROR (SP_ESTATE);
}

bool
sp_json_is_done (const SpJson *p)
{
	assert (p != NULL);

	return IS_DONE (p->cs);
}

uint8_t *
sp_json_steal_string (SpJson *p, size_t *len, size_t *cap)
{
	assert (p != NULL);

	return sp_utf8_steal (&p->utf8, len, cap);
}

