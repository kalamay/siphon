#ifndef SIPHON_HTTP_H
#define SIPHON_HTTP_H

#include "common.h"
#include "range.h"

#include <sys/socket.h>

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

typedef struct SpHttpMap SpHttpMap;
typedef struct SpHttpEntry SpHttpEntry;

typedef struct {
	// public
	uint16_t max_method; // max size for a request method
	uint16_t max_uri;    // max size for a request uri
	uint16_t max_reason; // max size for a response status message
	uint16_t max_field;  // max size for a header field
	uint16_t max_value;  // max size for a header value

	// readonly
	uint16_t scans;      // number of passes through the scanner
	uint8_t cscans;      // number of scans in the current rule
	bool response;       // true if response, false if request
	bool chunked;        // set by field scanner
	bool trailers;       // parsing trailers
	SpHttpValue as;      // captured value
	SpHttpType type;     // type of the captured value
	unsigned cs;         // current scanner state
	size_t off;          // internal offset mark
	size_t body_len;     // content length or current chunk size
	SpHttpMap *headers;  // map reference to capture headers
} SpHttp;

typedef enum {
	SP_CACHE_PUBLIC            = 1 << 0,
	SP_CACHE_PRIVATE           = 1 << 1,
	SP_CACHE_NO_CACHE          = 1 << 2,
	SP_CACHE_NO_STORE          = 1 << 3,
	SP_CACHE_MAX_AGE           = 1 << 4,
	SP_CACHE_S_MAXAGE          = 1 << 5,
	SP_CACHE_MAX_STALE         = 1 << 6,
	SP_CACHE_MAX_STALE_TIME    = 1 << 7,
	SP_CACHE_MIN_FRESH         = 1 << 8,
	SP_CACHE_NO_TRANSFORM      = 1 << 9,
	SP_CACHE_ONLY_IF_CACHED    = 1 << 10,
	SP_CACHE_MUST_REVALIDATE   = 1 << 11,
	SP_CACHE_PROXY_REVALIDATE  = 1 << 12
} SpCacheControlType;

#define SP_CACHE_CONTROL_HAS(s, k) ((s) && ((s)->type & (k)))

typedef struct {
	time_t max_age, s_maxage, max_stale, min_fresh;
	SpCacheControlType type;
	SpRange16 private, no_cache;
} SpCacheControl;



SP_EXPORT int
sp_http_init_request (SpHttp *p, bool capture);

SP_EXPORT int
sp_http_init_response (SpHttp *p, bool capture);

SP_EXPORT void
sp_http_final (SpHttp *p);

SP_EXPORT void
sp_http_reset (SpHttp *p);

SP_EXPORT ssize_t
sp_http_next (SpHttp *p, const void *restrict buf, size_t len);

SP_EXPORT bool
sp_http_is_done (const SpHttp *p);

SP_EXPORT SpHttpMap *
sp_http_steal_headers (SpHttp *p);

SP_EXPORT void
sp_http_print (const SpHttp *p, const void *restrict buf, FILE *out);



SP_EXPORT SpHttpMap *
sp_http_map_new (void);

SP_EXPORT void
sp_http_map_free (SpHttpMap *m);

SP_EXPORT int
sp_http_map_put (SpHttpMap *m,
		const void *name, size_t nlen,
		const void *value, size_t vlen);

SP_EXPORT bool
sp_http_map_del (SpHttpMap *m, const void *name, size_t nlen);

SP_EXPORT void
sp_http_map_clear (SpHttpMap *m);

SP_EXPORT const SpHttpEntry *
sp_http_map_get (const SpHttpMap *m, const void *name, size_t nlen);

SP_EXPORT size_t
sp_http_map_encode_size (const SpHttpMap *m);

SP_EXPORT void
sp_http_map_encode (const SpHttpMap *m, void *buf);

SP_EXPORT size_t
sp_http_map_scatter_count (const SpHttpMap *m);

SP_EXPORT void
sp_http_map_scatter (const SpHttpMap *m, struct iovec *iov);

SP_EXPORT void
sp_http_entry_name (const SpHttpEntry *e, struct iovec *iov);

SP_EXPORT size_t
sp_http_entry_count (const SpHttpEntry *e);

SP_EXPORT bool
sp_http_entry_value (const SpHttpEntry *e, size_t idx, struct iovec *iov);

SP_EXPORT void
sp_http_map_print (const SpHttpMap *m, FILE *out);



SP_EXPORT ssize_t
sp_cache_control_parse (SpCacheControl *cc, const char *buf, size_t len);

#endif

