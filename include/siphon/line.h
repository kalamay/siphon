#ifndef SIPHON_LINE_H
#define SIPHON_LINE_H

#include "common.h"

typedef enum {
	SP_LINE_NONE     = -1,
	SP_LINE_COMPLETE = 1
} SpLineType;

typedef struct {
	SpLineType type; // type of the captured value
	unsigned cs;     // current scanner state
	size_t off;      // internal offset mark
} SpLine;

SP_EXPORT void
sp_line_init (SpLine *p);

SP_EXPORT ssize_t
sp_line_next (SpLine *p, const void *restrict buf, size_t len, bool eof);

#endif

