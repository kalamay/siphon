#include "../../include/siphon/http.h"
#include "../parser.h"

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
		int rc = sp_http_map_put (p->headers,
				m+p->as.field.name.off, p->as.field.name.len,
				m+p->as.field.value.off, p->as.field.value.len);
		if (rc < 0) {
			return rc;
		}
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
parse_request_line (SpHttp *restrict p, const uint8_t *const restrict m, const size_t len)
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
parse_response_line (SpHttp *restrict p, const uint8_t *const restrict m, const size_t len)
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
parse_field (SpHttp *restrict p, const uint8_t *const restrict m, const size_t len)
{
	static const uint8_t field_sep[] = ":@\0 \"\"()[]//{{}}"; // must match ':', allows commas
	static const uint8_t field_lws[] = "\0\x08\x0A\x1f!\xff";

	const uint8_t *end = m + p->off;
	size_t scan = 0;

#undef SCAN
#define SCAN scan

again:
	p->type = SP_HTTP_NONE;

	switch (p->cs) {
	case FLD:
		if (REMAIN < sizeof crlf - 1) {
			return SCAN;
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
		p->as.field.name.off = SCAN;
		p->as.field.value.off += SCAN;
		p->as.field.value.len = (uint16_t)(p->off + SCAN - p->as.field.value.off - (sizeof crlf - 1));
		CHECK_ERROR (scrape_field (p, m));
		if (p->headers == NULL) {
			YIELD (SP_HTTP_FIELD, FLD);
		}
		else {
			SCAN = end - m;
			p->cs = FLD;
			p->off = 0;
			goto again;
		}

	default:
		YIELD_ERROR (SP_HTTP_ESTATE);
	}

#undef SCAN
#define SCAN 0
}

static ssize_t
parse_chunk (SpHttp *restrict p, const uint8_t *const restrict m, const size_t len)
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

