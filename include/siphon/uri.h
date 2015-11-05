#ifndef SIPHON_URI_H
#define SIPHON_URI_H

#include "common.h"
#include "range.h"

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
	SpHost host;
	uint16_t port;
	int8_t first, last;
} SpUri;

SP_EXPORT ssize_t
sp_uri_parse (SpUri *u, const char *restrict buf, size_t len);

SP_EXPORT ssize_t
sp_uri_copy (
		SpUri *dst, char *dstbuf, size_t dstlen,
		const SpUri *u, const char *ubuf);

SP_EXPORT ssize_t
sp_uri_join (
		SpUri *dst, char *dstbuf, size_t dstlen,
		const SpUri *a, const char *abuf,
		const SpUri *b, const char *bbuf);

SP_EXPORT ssize_t
sp_uri_join_paths (
		char *dstbuf, size_t dstlen,
		const char *abuf, size_t alen,
		const char *bbuf, size_t blen);

SP_EXPORT bool
sp_uri_eq (const SpUri *a, const char *abuf, const SpUri *b, const char *bbuf);

SP_EXPORT uint16_t
sp_uri_length (const SpUri *u);

SP_EXPORT SpUriSegment
sp_uri_sub (const SpUri *u, SpUriSegment start, SpUriSegment end, SpRange16 *out);

SP_EXPORT SpUriSegment
sp_uri_range (const SpUri *u, SpUriSegment start, SpUriSegment end, SpRange16 *out);

SP_EXPORT int
sp_uri_adjust_range (const SpUri *u, SpUriSegment *start, SpUriSegment *end, bool valid);

SP_EXPORT SpUriSegment
sp_uri_find_segment (const SpUri *self, SpUriSegment start, bool nonempty);

SP_EXPORT SpUriSegment
sp_uri_rfind_segment (const SpUri *self, SpUriSegment start, bool nonempty);

SP_EXPORT bool
sp_uri_has_segment (const SpUri *u, SpUriSegment seg);

SP_EXPORT bool
sp_uri_is_absolute (const SpUri *u);

SP_EXPORT const char *
sp_uri_segment_name (SpUriSegment seg);

SP_EXPORT void
sp_uri_print (const SpUri *u, const char *buf, FILE *out);

#endif

