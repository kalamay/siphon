#include "siphon/siphon.h"
#include "siphon/alloc.h"

#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <err.h>

static uint8_t *
readin (size_t *outlen)
{
	char buffer[8192];
	size_t len = fread (buffer, 1, sizeof buffer, stdin);
	if (len == 0) {
		err (EXIT_FAILURE, "fread");
	}

	uint8_t *copy = sp_mallocn (len, 1);
	if (copy == NULL) {
		err (EXIT_FAILURE, "sp_mallocn");
	}

	memcpy (copy, buffer, len);
	*outlen = len;
	return copy;
}

int
main (void)
{
	size_t len;
	uint8_t *buf = readin (&len);

	SpMsgpack p;
	sp_msgpack_init (&p);

	while (!sp_msgpack_is_done (&p)) {
		ssize_t rc = sp_msgpack_next (&p, buf, len, true);
		if (rc < 0 || (size_t)rc > len) {
			errx (EXIT_FAILURE, "failed to parse");
		}

		buf += rc;
		len -= rc;

		switch (p.type) {
		case SP_MSGPACK_NONE:
			break;
		case SP_MSGPACK_MAP:
			printf ("map: %u entries\n", p.tag.count);
			break;
		case SP_MSGPACK_ARRAY:
			printf ("array: %u entries\n", p.tag.count);
			break;
		case SP_MSGPACK_MAP_END:
			printf ("end map\n");
			break;
		case SP_MSGPACK_ARRAY_END:
			printf ("end array\n");
			break;
		case SP_MSGPACK_NIL:
			printf ("nil\n");
			break;
		case SP_MSGPACK_TRUE:
			printf ("true\n");
			break;
		case SP_MSGPACK_FALSE:
			printf ("false\n");
			break;
		case SP_MSGPACK_NEGATIVE:
			printf ("negative: %" PRIi64 "\n", p.tag.i64);
			break;
		case SP_MSGPACK_POSITIVE:
			printf ("positive: %" PRIu64 "\n", p.tag.u64);
			break;
		case SP_MSGPACK_FLOAT:
			printf ("float: %f\n", p.tag.f32);
			break;
		case SP_MSGPACK_DOUBLE:
			printf ("double: %f\n", p.tag.f64);
			break;
		case SP_MSGPACK_STRING:
			if (p.tag.count > len) {
				errx (EXIT_FAILURE, "failed to parse");
			}
			printf ("string: %.*s\n", (int)p.tag.count, buf);
			buf += p.tag.count;
			len -= p.tag.count;
			break;
		case SP_MSGPACK_BINARY:
			if (p.tag.count > len) {
				errx (EXIT_FAILURE, "failed to parse");
			}
			printf ("binary: length=%u\n", p.tag.count);
			buf += p.tag.count;
			len -= p.tag.count;
			break;
		case SP_MSGPACK_EXT:
			if (p.tag.ext.len > len) {
				errx (EXIT_FAILURE, "failed to parse");
			}
			printf ("ext: type=%d, length=%u\n", p.tag.ext.type, p.tag.ext.len);
			buf += p.tag.ext.len;
			len -= p.tag.ext.len;
			break;
		}
	}
	return 0;
}

