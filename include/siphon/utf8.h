#ifndef SIPHON_UTF8_H
#define SIPHON_UTF8_H

#include "common.h"

typedef enum {
	SP_UTF8_JSON
} SpUtf8Type;

typedef struct {
	uint8_t *buf;
    size_t len;
    size_t cap;
	bool fixed;
} SpUtf8;

#define SP_UTF8_MAKE() ((SpUtf8){ NULL, 0, 0, false })

#define SP_UTF8_MAKE_FIXED(buf, len) ((SpUtf8){ buf, len, 0, true })

#define SP_UTF8_JSON_RANGE "\\\\\x00\x1F\"\"\x7F\xFF"

SP_EXPORT void
sp_utf8_init (SpUtf8 *u);

SP_EXPORT void
sp_utf8_init_fixed (SpUtf8 *u, void *buf, size_t len);

SP_EXPORT void
sp_utf8_reset (SpUtf8 *u);

SP_EXPORT void
sp_utf8_final (SpUtf8 *u);

SP_EXPORT uint8_t *
sp_utf8_steal (SpUtf8 *u, size_t *len, size_t *cap);

SP_EXPORT size_t
sp_utf8_copy (const SpUtf8 *u, void *buf, size_t len);

SP_EXPORT int
sp_utf8_ensure (SpUtf8 *u, size_t len);

SP_EXPORT ssize_t
sp_utf8_add_raw (SpUtf8 *u, const void *src, size_t len);

SP_EXPORT ssize_t
sp_utf8_add_codepoint (SpUtf8 *u, int cp);

SP_EXPORT ssize_t
sp_utf8_add_char (SpUtf8 *u, const void *src, size_t len);

SP_EXPORT ssize_t
sp_utf8_json_decode (SpUtf8 *u, const void *src, size_t len);

SP_EXPORT ssize_t
sp_utf8_json_decode_next (SpUtf8 *u, const void *src, size_t len);

SP_EXPORT ssize_t
sp_utf8_json_encode (SpUtf8 *u, const void *src, size_t len);

SP_EXPORT ssize_t
sp_utf8_json_encode_next (SpUtf8 *u, const void *src, size_t len);

SP_EXPORT int
sp_utf8_codepoint (const void *src, size_t len);

#endif

