#ifndef SIPHON_JSON_H
#define SIPHON_JSON_H

#include "common.h"
#include "utf8.h"

#define SP_JSON_MAX_STRING 262144

typedef enum {
	SP_JSON_NONE       = -1,
	SP_JSON_OBJECT     = '{',
	SP_JSON_OBJECT_END = '}',
	SP_JSON_ARRAY      = '[',
	SP_JSON_ARRAY_END  = ']',
	SP_JSON_STRING     = '"',
	SP_JSON_NUMBER     = '#',
	SP_JSON_TRUE       = 't',
	SP_JSON_FALSE      = 'f',
	SP_JSON_NULL       = 'n'
} SpJsonType;

typedef struct {
	SpUtf8 utf8;        // stores the unescaped string
	double number;      // parsed number value
	SpJsonType type;    // type of the captured value
	unsigned cs;        // current scanner state
	size_t off;         // internal offset mark
	size_t mark;        // mark position for scanning doubles
	uint16_t depth;     // stack entry size
	uint8_t stack[64];  // object/array bit stack
} SpJson;

SP_EXPORT void
sp_json_init (SpJson *p);

SP_EXPORT void
sp_json_reset (SpJson *p);

SP_EXPORT void
sp_json_final (SpJson *p);

SP_EXPORT ssize_t
sp_json_next (SpJson *p, const void *restrict buf, size_t len, bool eof);

SP_EXPORT bool
sp_json_is_done (const SpJson *p);

SP_EXPORT uint8_t *
sp_json_steal_string (SpJson *p, size_t *len, size_t *cap);

#endif

