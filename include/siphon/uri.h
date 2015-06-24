#ifndef SIPHON_URI_H
#define SIPHON_URI_H

#include "common.h"

typedef enum {
	SP_URI_NONE   = -1,
	SP_URI_SCHEME = 0,
	SP_URI_USER,
	SP_URI_PASSWORD,
	SP_URI_HOST,
	SP_URI_PORT,
	SP_URI_PATH,
	SP_URI_QUERY,
	SP_URI_FRAGMENT
} SpUriSegment;

#define SP_URI_SEGMENT_FIRST SP_URI_SCHEME
#define SP_URI_SEGMENT_LAST SP_URI_FRAGMENT
#define SP_URI_SEGMENT_VALID(s) \
	((s) >= SP_URI_SEGMENT_FIRST && (s) <= SP_URI_SEGMENT_LAST)

typedef enum {
	SP_HOST_NONE,
	SP_HOST_NAME,
	SP_HOST_IPV4,
	SP_HOST_IPV6,
	SP_HOST_IPV4_MAPPED,
	SP_HOST_IP_FUTURE
} SpHost;

typedef struct {
	SpRange16 seg[8];
	uint16_t port;
	SpUriSegment first:8, last:8;
	SpHost host:8;
} SpUri;

extern ssize_t
sp_uri_parse (SpUri *u, const char *restrict buf, size_t len);

extern ssize_t
sp_uri_copy (
		const SpUri *u, const char *buf,
		SpUri *out, char *outbuf, size_t len);

extern ssize_t
sp_uri_join (
		const SpUri *a, const char *abuf,
		const SpUri *b, const char *bbuf,
		SpUri *out, char *outbuf, size_t len);

extern bool
sp_uri_eq (const SpUri *a, const char *abuf, const SpUri *b, const char *bbuf);

extern uint16_t
sp_uri_length (const SpUri *u);

extern int
sp_uri_range (const SpUri *u, SpUriSegment start, SpUriSegment end, bool valid, SpRange16 *out);

extern int
sp_uri_adjust_range (const SpUri *u, SpUriSegment *start, SpUriSegment *end, bool valid);

extern SpUriSegment
sp_uri_find_segment (const SpUri *self, SpUriSegment start, bool nonempty);

extern SpUriSegment
sp_uri_rfind_segment (const SpUri *self, SpUriSegment start, bool nonempty);

extern bool
sp_uri_has_segment (const SpUri *u, SpUriSegment seg);

extern bool
sp_uri_is_absolute (const SpUri *u);

#endif

