#ifndef SIPHON_HTTP_H
#define SIPHON_HTTP_H

#include "common.h"

#include <stdio.h>

#define SP_HTTP_MAX_METHOD 32
#define SP_HTTP_MAX_URI 8192
#define SP_HTTP_MAX_REASON 256
#define SP_HTTP_MAX_FIELD 256
#define SP_HTTP_MAX_VALUE 1024

typedef union {
	// request line values
	struct {
		SpRange8 method;
		SpRange16 uri;
		uint8_t version;
	} request;

	// response line values
	struct {
		SpRange16 reason;
		uint16_t status;
		uint8_t version;
	} response;

	// header field name and value
	struct {
		SpRange16 name;
		SpRange16 value;
	} field;

	// beginning of body
	struct {
		size_t content_length;
		bool chunked;
	} body_start;

	// size of next chunk
	struct {
		size_t length;
	} body_chunk;
} SpHttpValue;

typedef enum {
	SP_HTTP_NONE = -1,
	SP_HTTP_REQUEST,     // complete request line
	SP_HTTP_RESPONSE,    // complete response line
	SP_HTTP_FIELD,       // header or trailer field name and value
	SP_HTTP_BODY_START,  // start of the body
	SP_HTTP_BODY_CHUNK,  // size for chunked body
	SP_HTTP_BODY_END,    // end of the body chunks
	SP_HTTP_TRAILER_END  // complete request or response
} SpHttpType;

typedef struct {
	SpHttpValue as;   // captured value
	SpHttpType type;  // type of the captured value
	unsigned cs;      // current scanner state
	size_t off;       // internal offset mark
	size_t body_len;  // content length or current chunk size
	uint16_t scans;   // number of passes through the scanner
	uint8_t cscans;   // number of scans in the current rule
	bool response;    // true if response, false if request
	bool chunked;     // set by field scanner
	bool trailers;    // parsing trailers
} SpHttp;

SP_EXPORT void
sp_http_init_request (SpHttp *p);

SP_EXPORT void
sp_http_init_response (SpHttp *p);

SP_EXPORT void
sp_http_reset (SpHttp *p);

SP_EXPORT ssize_t
sp_http_next (SpHttp *p, const void *restrict buf, size_t len);

SP_EXPORT bool
sp_http_is_done (const SpHttp *p);

SP_EXPORT void
sp_http_print (const SpHttp *p, const void *restrict buf, FILE *out);

#endif

