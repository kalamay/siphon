#include "siphon/utf8.h"
#include "siphon/error.h"
#include "common.h"

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <assert.h>

#define MAX_POWER_OF_2 16384

/*
 * Code point      Size  1st     2nd     3rd     4th   
 * --------------  ----  ------  ------  ------  ------
 * 000000..00007F  1     00..7F
 * 000080..0007FF  2     C0..DF  80..BF
 * 000800..000FFF  3     E0      A0..BF  80..BF
 * 001000..00FFFF  3     E1..EF  80..BF  80..BF
 * 010000..03FFFF  4     F0      90..BF  80..BF  80..BF
 * 040000..0FFFFF  4     F1..F3  80..BF  80..BF  80..BF
 * 100000..10FFFF  4     F4      80..8F  80..BF  80..BF
 */

static const uint8_t byte_counts[] = {
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0
};

int
sp_utf8_init (SpUtf8 *u)
{
	assert (u != NULL);

	u->buf = NULL;
	u->cap = 0;
	u->len = 0;
	return 0;
}

void
sp_utf8_reset (SpUtf8 *u)
{
	assert (u != NULL);

	if (u->buf != NULL) {
		u->buf[0] = '\0';
	}
	u->len = 0;
}

void
sp_utf8_final (SpUtf8 *u)
{
	assert (u != NULL);

	free (u->buf);
	u->buf = NULL;
	u->cap = 0;
	u->len = 0;
}

uint8_t *
sp_utf8_steal (SpUtf8 *u, size_t *len, size_t *cap)
{
	assert (u != NULL);

	uint8_t *buf = u->buf;
	if (len) *len = u->len;
	if (cap) *cap = u->cap;
	sp_utf8_init (u);
	return buf;
}

int
sp_utf8_ensure (SpUtf8 *u, size_t len)
{
	assert (u != NULL);

	if (len > SSIZE_MAX) {
		return SP_ESIZE;
	}

	// add 1 to ensure space for a NUL byte
	size_t cap = u->len + len + 1;

	// check if we already have enough space
	if (cap <= u->cap) {
		return 0;
	}

	// round up required capacity
	if (cap < MAX_POWER_OF_2) {
		// calculate the next power of 2
		cap = (size_t)pow (2, ceil (log2 (cap)));
	}
	else {
		// calculate the nearest multiple of MAX_POWER_OF_2
		cap = (((cap - 1) / MAX_POWER_OF_2) + 1) * MAX_POWER_OF_2;
	}

	uint8_t *buf = realloc (u->buf, cap);
	if (buf == NULL) {
		return SP_ESYSTEM;
	}
	u->buf = buf;
	u->cap = cap;
	return 0;
}

ssize_t
sp_utf8_add_raw (SpUtf8 *u, const void *src, size_t len)
{
	assert (u != NULL);

	if (sp_utf8_ensure (u, len) < 0) {
		return SP_ESYSTEM;
	}
	memcpy (u->buf + u->len, src, len);
	u->len += len;
	u->buf[u->len] = '\0';
	return (ssize_t)len;
}

ssize_t
sp_utf8_add_codepoint (SpUtf8 *u, int cp)
{
	assert (u != NULL);

	uint8_t buf[4], len = 0;

	if (cp < 0x80) {
		if (cp < 0) {
			return SP_ECODEPOINT;
		}
		buf[0] = cp;
		len = 1;
	}
	else if (cp < 0x800) {
		buf[0] = (cp >> 6) | 0xC0;
		buf[1] = (cp & 0x3F) | 0x80;
		len = 2;
	}
	else if (cp < 0x10000) {
		buf[0] = (cp >> 12) | 0xE0;
		buf[1] = ((cp >> 6) & 0x3F) | 0x80;
		buf[2] = (cp & 0x3F) | 0x80;
		len = 3;
	}
	else if (cp < 0x110000) {
		buf[0] = (cp >> 18) | 0xF0;
		buf[1] = ((cp >> 12) & 0x3F) | 0x80;
		buf[2] = ((cp >> 6) & 0x3F) | 0x80;
		buf[3] = (cp & 0x3F) | 0x80;
		len = 4;
	}
	else {
		return SP_ECODEPOINT;
	}

	return sp_utf8_add_raw (u, buf, len);
}

static bool
is_cont (uint8_t b)
{
	return 0x80 <= b && b <= 0xBF;
}

ssize_t
sp_utf8_add_char (SpUtf8 *u, const void *src, size_t len)
{
	assert (u != NULL);

	if (len == 0) {
		return 0;
	}


	const uint8_t *in = src;
	size_t charlen;

	if (!(*in & 0x80)) {
		return sp_utf8_add_raw (u, src, 1);
	}

	switch (in[0]) {

	case 0xE0:
		if (len < 3 || in[1] < 0xA0 || 0xBF < in[1]) {
			return SP_EENCODING;
		}
		if (!is_cont (in[2])) {
			return SP_EENCODING;
		}
		charlen = 3;
		break;

	case 0xF0:
		if (len < 4 || in[1] < 0x90 || 0xBF < in[1]) {
			return SP_EENCODING;
		}
		if (!is_cont (in[2]) || !is_cont (in[3])) {
			return SP_EENCODING;
		}
		charlen = 4;
		break;

	default:
		charlen = byte_counts[in[0]];
		if (charlen == 0 || len < charlen) {
			return SP_EENCODING;
		}
		switch (charlen) {
		case 0: return SP_EENCODING;
		case 4: if (!is_cont (in[3])) return SP_EENCODING;
		case 3: if (!is_cont (in[2])) return SP_EENCODING;
		case 2: if (!is_cont (in[1])) return SP_EENCODING;
		}
	}

	return sp_utf8_add_raw (u, src, charlen);
}

ssize_t
sp_utf8_json_decode (SpUtf8 *u, const void *src, size_t len)
{
	assert (u != NULL);

	const uint8_t *p = (const uint8_t *)src;
	const uint8_t *pe = p + len;
	ssize_t start = u->len;

	while (p < pe) {
		static const uint8_t rng[] = SP_UTF8_JSON_RANGE;
		const uint8_t *m = pcmp_range16 (p, pe - p, rng, sizeof rng - 1);
		if (m == NULL) {
			break;
		}

		sp_utf8_add_raw (u, p, m-p);
		p = m;

		ssize_t n = sp_utf8_json_decode_next (u, p, pe-p);
		if (n < 0) {
			return n;
		}
		p += n;
	}

	sp_utf8_add_raw (u, p, pe-p);
	return u->len - start;
}

static inline bool
is_surrogate (int cp)
{
	return 0xD800 <= cp && cp <= 0xDFFF;
}

static inline int
combine_surrogate (int hi, int lo)
{
	return (hi - 0xD800) * 0x400 + (lo - 0xDC00) + 0x10000;
}

static inline unsigned
hex4 (const uint8_t *p)
{
	static const long map[128] = {
		['0'] = 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
		['A'] = 10, 11, 12, 13, 14, 15,
		['a'] = 10, 11, 12, 13, 14, 15
	};
	return (map[p[0]] << 12) | (map[p[1]] << 8) | (map[p[2]] << 4) | map[p[3]];
}

static inline bool
is_hex4 (const uint8_t *p)
{
	return isxdigit (p[0]) && isxdigit (p[1]) &&
	       isxdigit (p[2]) && isxdigit (p[3]);
}

static ssize_t
unescape (SpUtf8 *u, const uint8_t *src, ssize_t rem)
{
	if (rem < 2) {
		return SP_ETOOSHORT;
	}

	int cp;
	size_t scan = 0;

	switch (src[1]) {
	case 'b':  scan = 2; cp = '\b'; break;
	case 'f':  scan = 2; cp = '\f'; break;
	case 'n':  scan = 2; cp = '\n'; break;
	case 'r':  scan = 2; cp = '\r'; break;
	case 't':  scan = 2; cp = '\t'; break;
	case '"':  scan = 2; cp = '"';  break;
	case '/':  scan = 2; cp = '/';  break;
	case '\\': scan = 2; cp = '\\'; break;
	case 'u':
		if (rem < 6) return SP_ETOOSHORT;
		if (!is_hex4 (src+2)) return SP_EESCAPE;

		cp = hex4 (src + 2);
		if (is_surrogate (cp)) {
			if ((rem > 6 && src[6] != '\\') || (rem > 7 && src[7] != 'u')) {
				return SP_ESURROGATE;
			}
			if (rem < 12) return SP_ETOOSHORT;
			if (!is_hex4 (src+8)) return SP_EESCAPE;
			cp = combine_surrogate (cp, hex4 (src+8));
			scan = 12;
		}
		else {
			scan = 6;
		}

		break;
	default:
		return SP_EESCAPE;
	}

	ssize_t rc = sp_utf8_add_codepoint (u, cp);
	return rc < 0 ? rc : scan;
}

static inline bool
is_valid_json_byte (const uint8_t *src)
{
	return *src >= 0x1F && *src != 0x7F;
}

ssize_t
sp_utf8_json_decode_next (SpUtf8 *u, const void *src, size_t len)
{
	assert (u != NULL);
	assert (src != NULL);
	assert (len > 0);

	// check for control characters
	if (!is_valid_json_byte (src)) {
		// TODO: better error code
		return SP_ESYNTAX;
	}

	return *(const uint8_t *)src == '\\' ?
		unescape (u, src, len) :
		sp_utf8_add_char (u, src, len);
}

ssize_t
sp_utf8_json_encode (SpUtf8 *u, const void *src, size_t len)
{
	assert (u != NULL);

	const uint8_t *p = (const uint8_t *)src;
	const uint8_t *pe = p + len;
	ssize_t start = u->len;

	while (p < pe) {
		static const uint8_t rng[] = SP_UTF8_JSON_RANGE;
		const uint8_t *m = pcmp_range16 (p, pe - p, rng, sizeof rng - 1);
		if (m == NULL) {
			break;
		}

		sp_utf8_add_raw (u, p, m-p);
		p = m;

		ssize_t n = sp_utf8_json_encode_next (u, p, pe-p);
		if (n < 0) {
			return n;
		}
		p += n;
	}

	sp_utf8_add_raw (u, p, pe-p);
	return u->len - start;
}

static inline ssize_t
add_escape (SpUtf8 *u, const void *esc)
{
	ssize_t rc = sp_utf8_add_raw (u, esc, 2);
	return rc < 0 ? rc : 1;
}

ssize_t
sp_utf8_json_encode_next (SpUtf8 *u, const void *src, size_t len)
{
	assert (u != NULL);
	assert (src != NULL);
	assert (len > 0);

	switch (*(const uint8_t *)src) {
	case '"':  return add_escape (u, "\\\"");
	case '\\': return add_escape (u, "\\\\");
	case '\b': return add_escape (u, "\\\b");
	case '\f': return add_escape (u, "\\\f");
	case '\n': return add_escape (u, "\\\n");
	case '\r': return add_escape (u, "\\\r");
	case '\t': return add_escape (u, "\\\t");
	}

	if (!is_valid_json_byte (src)) {
		return SP_EENCODING;
	}

	return sp_utf8_add_char (u, src, len);
}

int
sp_utf8_codepoint (const void *src, size_t len)
{
	assert (src != NULL);

	if (len == 0) {
		return SP_ETOOSHORT;
	}

	const uint8_t *m = src;
	int cp1, cp2, cp3, cp4;

	cp1 = m[0];
	if (cp1 < 0x80) {
		return cp1;
	}
	else if (cp1 < 0xC2) {
		return SP_EENCODING;
	}
	else if (cp1 < 0xE0) {
		if (len < 2) {
			return SP_EENCODING;
		}
		cp2 = m[1];
		if ((cp2 & 0xC0) != 0x80) {
			return SP_EENCODING;
		}
		return (cp1 << 6) + cp2 - 0x3080;
	}
	else if (cp1 < 0xF0) {
		if (len < 3) {
			return SP_EENCODING;
		}
		cp2 = m[1];
		if ((cp2 & 0xC0) != 0x80 || (cp1 == 0xE0 && cp2 < 0xA0)) {
			return SP_EENCODING;
		}
		cp3 = m[2];
		if ((cp3 & 0xC0) != 0x80) {
			return SP_EENCODING;
		}
		return (cp1 << 12) + (cp2 << 6) + cp3 - 0xE2080;
	}
	else if (cp1 < 0xF5) {
		if (len < 4) {
			return SP_EENCODING;
		}
		cp2 = m[1];
		if ((cp2 & 0xC0) != 0x80 || (cp1 == 0xF0 && cp2 < 0x90) || (cp1 == 0xF4 && cp2 >= 0x90)) {
			return SP_EENCODING;
		}
		cp3 = m[2];
		if ((cp3 & 0xC0) != 0x80) {
			return SP_EENCODING;
		}
		cp4 = m[3];
		if ((cp4 & 0xC0) != 0x80) {
			return SP_EENCODING;
		}
		return (cp1 << 18) + (cp2 << 12) + (cp3 << 6) + cp4 - 0x3C82080;
	}
	else {
		return SP_EENCODING;
	}
}

