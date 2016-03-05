#ifndef SIPHON_UTF8_H
#define SIPHON_UTF8_H

#include "common.h"

typedef struct {
	uint8_t *buf;
    size_t len;
    size_t cap;
	bool fixed;
} SpUtf8;

typedef enum {
	SP_UTF8_NONE          = 0,      // no encoding
	SP_UTF8_JSON          = 1 << 0, // json string scheme
	SP_UTF8_URI           = 1 << 1, // percent scheme
	SP_UTF8_URI_COMPONENT = 1 << 2, // percent scheme for uri components
	SP_UTF8_SPACE_PLUS    = 1 << 3, // treat plus as a space character
} SpUtf8Flags;

#define SP_UTF8_MAKE() ((SpUtf8){ NULL, 0, 0, false })

#define SP_UTF8_MAKE_FIXED(buf, len) ((SpUtf8){ buf, 0, len, true })

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
sp_utf8_encode (SpUtf8 *u, const void *src, size_t len, SpUtf8Flags f);

SP_EXPORT ssize_t
sp_utf8_decode (SpUtf8 *u, const void *src, size_t len, SpUtf8Flags f);

SP_EXPORT ssize_t
sp_utf8_json_encode (SpUtf8 *u, const void *src, size_t len, SpUtf8Flags f);

SP_EXPORT ssize_t
sp_utf8_json_decode (SpUtf8 *u, const void *src, size_t len, SpUtf8Flags f);

SP_EXPORT ssize_t
sp_utf8_uri_encode (SpUtf8 *u, const void *src, size_t len, SpUtf8Flags f);

SP_EXPORT ssize_t
sp_utf8_uri_decode (SpUtf8 *u, const void *src, size_t len, SpUtf8Flags f);


SP_EXPORT int
sp_utf8_codepoint (const void *src, size_t len);

SP_EXPORT ssize_t
sp_utf8_charlen (const void *src, size_t len);

#endif

