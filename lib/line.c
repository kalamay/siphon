#include "../include/siphon/line.h"
#include "common.h"

#include <assert.h>

void
sp_line_init (SpLine *p)
{
	assert (p != NULL);

	p->type = SP_LINE_NONE;
	p->cs = 0;
	p->off = 0;
}

ssize_t
sp_line_next (SpLine *p, const void *restrict buf, size_t len, bool eof)
{
	assert (p != NULL);
	assert (buf != NULL);

	static const uint8_t eol[] = "\n";

	const uint8_t *restrict m = buf;
	const uint8_t *restrict end = m + p->off;

	p->type = SP_LINE_NONE;

	EXPECT_SET (eol, len, eof, SP_LINE_ESYNTAX, SP_LINE_ESIZE);
	end++;
	YIELD (SP_LINE_VALUE, 0);
}

