#include "../include/siphon/http.h"
#include "../include/siphon/vec.h"
#include "../include/siphon/map.h"
#include "../include/siphon/hash.h"
#include "../include/siphon/alloc.h"
#include "parser.h"

#include <assert.h>
#include <ctype.h>

// TODO: add support for strings and lws within header field values

static const uint8_t version_start[] = "HTTP/1.";
static const uint8_t sep[] = ": ";
static const uint8_t crlf[] = "\r\n";

#define START    0x000000

#define REQ      0x00000F
#define REQ_METH 0x000001
#define REQ_URI  0x000002
#define REQ_VER  0x000003
#define REQ_EOL  0x000004

#define RES      0x0000F0
#define RES_VER  0x000010
#define RES_SEP  0x000020
#define RES_CODE 0x000030
#define RES_MSG  0x000040

#define FLD      0x000F00
#define FLD_KEY  0x000100
#define FLD_LWS  0x000200
#define FLD_VAL  0x000300

#define CHK      0x00F000
#define CHK_NUM  0x001000
#define CHK_EOL1 0x002000
#define CHK_EOL2 0x003000

static int
scrape_field (SpHttp *restrict p, const uint8_t *restrict m)
{
	if (p->headers != NULL) {
		sp_http_map_put (p->headers,
				m+p->as.field.name.off, p->as.field.name.len,
				m+p->as.field.value.off, p->as.field.value.len);
	}

	if (p->trailers || p->body_len) {
		return 0;
	}

	if (LEQ16 ("content-length", m + p->as.field.name.off, p->as.field.name.len)) {
		if (p->as.field.value.len == 0) {
			YIELD_ERROR (SP_HTTP_ESYNTAX);
		}
		size_t num = 0;
		const uint8_t *s = m + p->as.field.value.off;
		const uint8_t *e = s + p->as.field.value.len;
		while (s < e) {
			if (!isdigit (*s)) {
				YIELD_ERROR (SP_HTTP_ESYNTAX);
			}
			num = num * 10 + (*s - '0');
			s++;
		}
		p->body_len = num;
		return 0;
	}

	if (LEQ ("transfer-encoding", m + p->as.field.name.off, p->as.field.name.len) &&
			LEQ16 ("chunked", m + p->as.field.value.off, p->as.field.value.len)) {
		p->chunked = true;
		return 0;
	}

	return 0;
}

static ssize_t
parse_request_line (SpHttp *restrict p, const uint8_t *restrict m, size_t len)
{
	static const uint8_t method_sep[] = "\0@[`{\xff"; // must match ' '
	static const uint8_t uri_sep[] = "\0 \x7f\xff"; // must match ' '

	const uint8_t *end = m + p->off;

	p->type = SP_HTTP_NONE;

	switch (p->cs) {
	case REQ:
		p->cs = REQ_METH;
		p->as.request.method.off = (uint8_t)p->off;

	case REQ_METH:
		EXPECT_RANGE_THEN_CHAR (method_sep, ' ', p->max_method, false,
				SP_HTTP_ESYNTAX, SP_HTTP_ESIZE);
		p->as.request.method.len = (uint8_t)(p->off - 1);
		p->cs = REQ_URI;
		p->as.request.uri.off = p->off;

	case REQ_URI:
		EXPECT_RANGE_THEN_CHAR (uri_sep, ' ', p->max_uri, false,
				SP_HTTP_ESYNTAX, SP_HTTP_ESIZE);
		p->as.request.uri.len = (uint16_t)(p->off - 1 - p->as.request.uri.off);
		p->cs = REQ_VER;

	case REQ_VER:
		EXPECT_PREFIX (version_start, 1, false, SP_HTTP_ESYNTAX);
		if (!isdigit (*end)) {
			YIELD_ERROR (SP_HTTP_ESYNTAX);
		}
		p->as.request.version = (uint8_t)(*end - '0');
		p->cs = REQ_EOL;
		end++;

	case REQ_EOL:
		EXPECT_PREFIX (crlf, 0, false, SP_HTTP_ESYNTAX);
		YIELD (SP_HTTP_REQUEST, FLD);

	default:
		YIELD_ERROR (SP_HTTP_ESTATE);
	}
}

static ssize_t
parse_response_line (SpHttp *restrict p, const uint8_t *restrict m, size_t len)
{
	const uint8_t *end = m + p->off;

	p->type = SP_HTTP_NONE;

	switch (p->cs) {
	case RES:
		p->cs = RES_VER;

	case RES_VER:
		EXPECT_PREFIX (version_start, 1, false, SP_HTTP_ESYNTAX);
		if (!isdigit (*end)) {
			YIELD_ERROR (SP_HTTP_ESYNTAX);
		}
		p->as.response.version = (uint8_t)(*end - '0');
		p->cs = RES_SEP;
		end++;
	
	case RES_SEP:
		EXPECT_CHAR (' ', false, SP_HTTP_ESYNTAX);
		p->cs = RES_CODE;
		p->as.response.status = 0;

	case RES_CODE:
		do {
			if (p->off == len) return 0;
			if (*end == ' ') {
				p->off++;
				end++;
				break;
			}
			if (isdigit (*end)) {
				p->as.response.status = p->as.response.status * 10 + (*end - '0');
				p->off++;
				end++;
			}
			else {
				YIELD_ERROR (SP_HTTP_ESYNTAX);
			}
		} while (true);
		p->as.response.reason.off = p->off;
		p->cs = RES_MSG;

	case RES_MSG:
		EXPECT_CRLF (p->max_reason + p->as.response.reason.off, false,
				SP_HTTP_ESYNTAX, SP_HTTP_ESIZE);
		p->as.response.reason.len = (uint16_t)(p->off - p->as.response.reason.off - (sizeof crlf - 1));
		YIELD (SP_HTTP_RESPONSE, FLD);

	default:
		YIELD_ERROR (SP_HTTP_ESTATE);
	}
}

static ssize_t
parse_field (SpHttp *restrict p, const uint8_t *restrict m, size_t len)
{
	static const uint8_t field_sep[] = ":@\0 \"\"()[]//{{}}"; // must match ':', allows commas
	static const uint8_t field_lws[] = "\0\x08\x0A\x1f!\xff";

	const uint8_t *end = m + p->off;

	p->type = SP_HTTP_NONE;

	switch (p->cs) {
	case FLD:
		if (len < sizeof crlf - 1) {
			return 0;
		}
		if (end[0] == crlf[0] && end[1] == crlf[1]) {
			end += 2;
			p->as.body_start.chunked = p->chunked;
			p->as.body_start.content_length = p->body_len;
			if (p->trailers) {
				YIELD (SP_HTTP_TRAILER_END, DONE);
			}
			else {
				YIELD (SP_HTTP_BODY_START, p->chunked ? CHK : DONE);
			}
		}
		p->cs = FLD_KEY;
		p->as.field.name.off = 0;

	case FLD_KEY:
		EXPECT_RANGE_THEN_CHAR (field_sep, ':', p->max_field, false,
				SP_HTTP_ESYNTAX, SP_HTTP_ESIZE);
		p->as.field.name.len = (uint16_t)(p->off - 1);
		p->cs = FLD_LWS;

	case FLD_LWS:
		EXPECT_RANGE (field_lws, p->max_value + p->as.field.value.off, false,
				SP_HTTP_ESYNTAX, SP_HTTP_ESIZE);
		p->as.field.value.off = (uint16_t)p->off;
		p->cs = FLD_VAL;

	case FLD_VAL:
		EXPECT_CRLF (p->max_value + p->as.field.value.off, false,
				SP_HTTP_ESYNTAX, SP_HTTP_ESIZE);
		p->as.field.value.len = (uint16_t)(p->off - p->as.field.value.off - (sizeof crlf - 1));
		scrape_field (p, m);
		YIELD (p->headers == NULL ? SP_HTTP_FIELD : SP_HTTP_NONE, FLD);

	default:
		YIELD_ERROR (SP_HTTP_ESTATE);
	}
}

static ssize_t
parse_chunk (SpHttp *restrict p, const uint8_t *restrict m, size_t len)
{
	static const uint8_t hex[] = {
		['0'] = 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
		['A'] = 10, 11, 12, 13, 14, 15,
		['a'] = 10, 11, 12, 13, 14, 15
	};

	const uint8_t *end = m + p->off;

again:
	p->type = SP_HTTP_NONE;

	switch (p->cs) {
	case CHK:
		p->off = 0;
		p->body_len = 0;
		p->cs = CHK_NUM;
	
	case CHK_NUM:
		do {
			if (p->off == len) return 0;
			if (isxdigit (*end)) {
				p->body_len = (p->body_len << 4) | hex[*end];
				p->off++;
				end++;
			}
			else {
				break;
			}
		} while (true);
		p->cs = CHK_EOL1;
	
	case CHK_EOL1:
		EXPECT_PREFIX (crlf, 0, false, SP_HTTP_ESYNTAX);
		if (p->body_len == 0) {
			p->trailers = true;
			YIELD (SP_HTTP_BODY_END, FLD);
		}
		else {
			p->as.body_chunk.length = p->body_len;
			p->body_len = 0;
			YIELD (SP_HTTP_BODY_CHUNK, CHK_EOL2);
		}

	case CHK_EOL2:
		EXPECT_PREFIX (crlf, 0, false, SP_HTTP_ESYNTAX);
		p->cs = CHK_NUM;
		goto again;

	default:
		YIELD_ERROR (SP_HTTP_ESTATE);
	}
}

static void
init_sizes (SpHttp *p)
{
	p->max_method = SP_HTTP_MAX_METHOD;
	p->max_uri = SP_HTTP_MAX_URI;
	p->max_reason = SP_HTTP_MAX_REASON;
	p->max_field = SP_HTTP_MAX_FIELD;
	p->max_value = SP_HTTP_MAX_VALUE;
}

static int
init_capture (SpHttp *p)
{
	p->headers = sp_http_map_new ();
	if (p->headers == NULL) {
		return SP_ESYSTEM (errno);
	}
	return 0;
}

int
sp_http_init_request (SpHttp *p, bool capture)
{
	assert (p != NULL);

	memset (p, 0, sizeof *p);
	init_sizes (p);
	p->cs = REQ;
	return capture ? init_capture (p) : 0;
}

int
sp_http_init_response (SpHttp *p, bool capture)
{
	assert (p != NULL);

	memset (p, 0, sizeof *p);
	init_sizes (p);
	p->cs = RES;
	p->response = true;
	return capture ? init_capture (p) : 0;
}

void
sp_http_final (SpHttp *p)
{
	assert (p != NULL);

	if (p->headers != NULL) {
		sp_http_map_free (p->headers);
		p->headers = NULL;
	}
}

void
sp_http_reset (SpHttp *p)
{
	assert (p != NULL);

	uint16_t max_method = p->max_method;
	uint16_t max_uri = p->max_uri;
	uint16_t max_reason = p->max_reason;
	uint16_t max_field = p->max_field;
	uint16_t max_value = p->max_value;
	SpHttpMap *headers = p->headers;

	if (p->response) {
		sp_http_init_response (p, false);
	}
	else {
		sp_http_init_request (p, false);
	}

	p->max_method = max_method;
	p->max_uri = max_uri;
	p->max_reason = max_reason;
	p->max_field = max_field;
	p->max_value = max_value;

	if (headers) {
		sp_http_map_clear (headers);
		p->headers = headers;
	}
}

ssize_t
sp_http_next (SpHttp *p, const void *restrict buf, size_t len)
{
	assert (p != NULL);

	if (len == 0) {
		return 0;
	}

	ssize_t rc;
	p->scans++;
	p->cscans++;

	     if (p->cs & REQ) rc = parse_request_line (p, buf, len);
	else if (p->cs & RES) rc = parse_response_line (p, buf, len);
	else if (p->cs & FLD) rc = parse_field (p, buf, len);
	else if (p->cs & CHK) rc = parse_chunk (p, buf, len);
	else { YIELD_ERROR (SP_HTTP_ESTATE); }
	if (rc > 0) {
		p->cscans = 0;
	}
	else if (rc == 0 && p->cscans > 64) {
		YIELD_ERROR (SP_HTTP_ETOOSHORT);
	}
	return rc;
}

bool
sp_http_is_done (const SpHttp *p)
{
	assert (p != NULL);

	return IS_DONE (p->cs);
}

SpHttpMap *
sp_http_steal_headers (SpHttp *p)
{
	assert (p != NULL);

	SpHttpMap *headers = p->headers;
	if (headers != NULL) {
		init_capture (p);
	}
	return headers;
}

void
sp_http_print (const SpHttp *p, const void *restrict buf, FILE *out)
{
	assert (p != NULL);

	if (out == NULL) {
		out = stderr;
	}

	switch (p->type) {
	case SP_HTTP_REQUEST:
		fprintf (out, "> %.*s %.*s HTTP/1.%u\n",
				(int)p->as.request.method.len, (char *)buf+p->as.request.method.off,
				(int)p->as.request.uri.len, (char *)buf+p->as.request.uri.off,
				p->as.request.version);
		break;
	case SP_HTTP_RESPONSE:
		fprintf (out, "< HTTP/1.%u %u %.*s\n",
				p->as.response.version,
				p->as.response.status,
				(int)p->as.response.reason.len, (char *)buf+p->as.response.reason.off);
		break;
	case SP_HTTP_FIELD:
		fprintf (out, "%c %.*s: %.*s\n",
				p->response ? '<' : '>',
				(int)p->as.field.name.len, (char *)buf+p->as.field.name.off,
				(int)p->as.field.value.len, (char *)buf+p->as.field.value.off);
		break;
	case SP_HTTP_BODY_START:
	case SP_HTTP_TRAILER_END:
		fprintf (out, "%c\n", p->response ? '<' : '>');
		break;
	default: break;
	}
}



struct SpHttpMap {
	SpMap map;
	size_t encode_size;
	size_t scatter_count;
};

struct SpHttpEntry {
	uint16_t **values;
	uint16_t len;
} __attribute__((packed));

static void
pstr_assign (uint16_t *s, const void *val, uint16_t len)
{
	*s = len;
	uint8_t *buf = (uint8_t *)(s+1);
	memcpy (buf, val, len);
	buf[len] = 0;
}

static uint16_t *
pstr_new (const void *val, uint16_t len)
{
	uint16_t *s = sp_malloc (sizeof *s + len + 1);
	if (s != NULL) {
		pstr_assign (s, val, len);
	}
	return s;
}

static void
pstr_free (uint16_t *s)
{
	if (s != NULL) {
		sp_free (s, sizeof *s + *s + 1);
	}
}

static bool
pstr_case_eq (const uint16_t *s, const void *val, uint16_t len)
{
	return *s == len && strncasecmp ((const char *)(s+1), val, len) == 0;
}

static void
pstr_set (const uint16_t *s, struct iovec *iov)
{
	iov->iov_len = *s;
	iov->iov_base = (void *)(s+1);
}

static void
pstr_copy (const uint16_t *s, void *dst)
{
	memcpy (dst, s+1, *s);
}

static bool
entry_iskey (const void *val, const void *key, size_t len)
{
	const SpHttpEntry *e = val;
	return pstr_case_eq (&e->len, key, len);
}

static SpHttpEntry *
entry_new (const void *name, uint16_t len)
{
	SpHttpEntry *e = sp_malloc (sizeof *e + len + 1);
	if (e != NULL) {
		e->values = NULL;
		pstr_assign (&e->len, name, len);
	}
	return e;
}

static void
entry_free (void *val)
{
	SpHttpEntry *e = val;

	size_t i;
	sp_vec_each (e->values, i) {
		pstr_free (e->values[i]);
	}
	sp_vec_free (e->values);
	sp_free (e, sizeof *e + e->len + 1);
}

static const SpType map_type = {
	.hash = sp_siphash_case,
	.iskey = entry_iskey,
	.free = entry_free
};

SpHttpMap *
sp_http_map_new (void)
{
	SpHttpMap *m = sp_malloc (sizeof *m);
	if (m != NULL) {
		sp_map_init (&m->map, 0, 0.0, &map_type);
		m->encode_size = 0;
		m->scatter_count = 0;
	}
	return m;
}

void
sp_http_map_free (SpHttpMap *m)
{
	if (m != NULL) {
		sp_map_final (&m->map);
		sp_free (m, sizeof *m);
	}
}

int
sp_http_map_put (SpHttpMap *m,
		const void *name, uint16_t nlen,
		const void *value, uint16_t vlen)
{
	assert (m != NULL);
	assert (name != NULL);
	assert (value != NULL);

	uint16_t *s = pstr_new (value, vlen);
	bool new = false;
	SpHttpEntry *e = NULL;
	void **loc;
	int err;

	if (s == NULL) {
		goto err;
	}

	loc = sp_map_reserve (&m->map, name, nlen, &new);
	if (loc == NULL) {
		goto err;
	}

	if (new) {
		e = entry_new (name, nlen);
		if (e == NULL) {
			goto err;
		}
	}
	else {
		e = *loc;
	}

	if (sp_vec_push (e->values, s) < 0) {
		goto err;
	}

	if (new) {
		sp_map_assign (&m->map, loc, e);
	}
	else {
		e = *loc;
	}
	m->scatter_count += 4;
	m->encode_size += nlen + vlen + 4;

	return 0;

err:
	err = errno;
	if (new && e != NULL) {
		entry_free (e);
	}
	pstr_free (s);
	return SP_ESYSTEM (err);
}

bool
sp_http_map_del (SpHttpMap *m, const void *name, uint16_t nlen)
{
	SpHttpEntry *e = sp_map_steal (&m->map, name, nlen);
	if (e == NULL) {
		return false;
	}
	m->scatter_count -= sp_vec_count (e->values) * 4;

	size_t i;
	sp_vec_each (e->values, i) {
		m->encode_size -= e->len + *e->values[i] + 4;
	}
	entry_free (e);
	return true;
}

void
sp_http_map_clear (SpHttpMap *m)
{
	sp_map_clear (&m->map);
	m->scatter_count = 0;
	m->encode_size = 0;
}

const SpHttpEntry *
sp_http_map_get (const SpHttpMap *m, const void *name, uint16_t nlen)
{
	return sp_map_get (&m->map, name, nlen);
}

size_t
sp_http_map_encode_size (const SpHttpMap *m)
{
	assert (m != NULL);

	return m->encode_size;
}

void
sp_http_map_encode (const SpHttpMap *m, void *buf)
{
	assert (m != NULL);
	assert (buf != NULL);

	char *p = buf;
	const SpMapEntry *me;

	sp_map_each (&m->map, me) {
		const SpHttpEntry *e = me->value;
		size_t i;

		sp_vec_each (e->values, i) {
			const uint16_t *s = e->values[i];

			pstr_copy (&e->len, p);
			p += e->len;
			memcpy (p, sep, sizeof sep - 1);
			p += sizeof sep - 1;
			pstr_copy (s, p);
			p += *s;
			memcpy (p, crlf, sizeof crlf - 1);
			p += sizeof crlf - 1;
		}
	}
}

size_t
sp_http_map_scatter_count (const SpHttpMap *m)
{
	assert (m != NULL);

	return m->scatter_count;
}

void
sp_http_map_scatter (const SpHttpMap *m, struct iovec *iov)
{
	assert (m != NULL);
	assert (iov != NULL);

	const SpMapEntry *me;

	sp_map_each (&m->map, me) {
		const SpHttpEntry *e = me->value;
		size_t i;

		sp_vec_each (e->values, i) {
			const uint16_t *s = e->values[i];

			iov[0].iov_base = (void *)(e+1);
			iov[0].iov_len = e->len;
			iov[1].iov_base = (void *)sep;
			iov[1].iov_len = sizeof sep - 1;
			iov[2].iov_base = (void *)(s+1);
			iov[2].iov_len = *s;
			iov[3].iov_base = (void *)crlf;
			iov[3].iov_len = sizeof crlf - 1;
			iov += 4;
		}
	}
}

void
sp_http_entry_name (const SpHttpEntry *e, struct iovec *iov)
{
	if (e == NULL) {
		iov->iov_len = 0;
	}
	else {
		pstr_set (&e->len, iov);
	}
}

size_t
sp_http_entry_count (const SpHttpEntry *e)
{
	return e == NULL ? 0 : sp_vec_count (e->values);
}

bool
sp_http_entry_value (const SpHttpEntry *e, size_t idx, struct iovec *iov)
{
	if (e != NULL && idx < sp_vec_count (e->values)) {
		pstr_set (e->values[idx], iov);
		return true;
	}
	return false;
}

void
sp_http_map_print (const SpHttpMap *m, FILE *out)
{
	if (out == NULL) {
		out = stderr;
	}

	if (m == NULL) {
		fprintf (out, "#<SpHttpMap:(null)>\n");
	}
	else {
		fprintf (out, "#<SpHttpMap:%p> {\n", (void *)m);
		const SpMapEntry *me;
		sp_map_each (&m->map, me) {
			const SpHttpEntry *e = me->value;
			size_t i;
			sp_vec_each (e->values, i) {
				const uint16_t *s = e->values[i];
				fprintf (out, "    %.*s: %.*s\n",
					(int)e->len, (char *)(e+1),
					(int)*s, (char *)(s+1));
			}
		}
		fprintf (out, "}\n");
	}
}

