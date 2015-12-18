#include "../include/siphon/json.h"
#include "../include/siphon/alloc.h"

#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <err.h>

static char buffer[1024*1024];

static uint8_t *
readin (const char *path, size_t *outlen)
{
	FILE *in = stdin;
	if (path != NULL) {
		in = fopen (path, "r");
		if (in == NULL) {
			err (EXIT_FAILURE, "fopen");
		}
	}

	char buffer[8192];
	size_t len = fread (buffer, 1, sizeof buffer, in);
	if (len == 0) {
		err (EXIT_FAILURE, "fread");
	}

	fclose (in);

	uint8_t *copy = sp_malloc (len);
	if (copy == NULL) {
		err (EXIT_FAILURE, "sp_malloc");
	}

	memcpy (copy, buffer, len);
	*outlen = len;
	return copy;
}

int
main (int argc, char **argv)
{
	size_t len;
	uint8_t *val = readin (argc > 1 ? argv[1] : NULL, &len);
	uint8_t *buf = val;
	size_t freelen = len;

	SpJson p;
	sp_json_init (&p);

	while (!sp_json_is_done (&p)) {
		ssize_t rc = sp_json_next (&p, buf, len, true);
		if (rc < 0 || (size_t)rc > len) {
			sp_free (val, len);
			errx (EXIT_FAILURE, "failed to parse");
		}

		buf += rc;
		len -= rc;

		switch (p.type) {
		case SP_JSON_NONE:
			break;
		case SP_JSON_OBJECT:
			printf ("object\n");
			break;
		case SP_JSON_ARRAY:
			printf ("array\n");
			break;
		case SP_JSON_OBJECT_END:
			printf ("end object\n");
			break;
		case SP_JSON_ARRAY_END:
			printf ("end array\n");
			break;
		case SP_JSON_NULL:
			printf ("null\n");
			break;
		case SP_JSON_TRUE:
			printf ("true\n");
			break;
		case SP_JSON_FALSE:
			printf ("false\n");
			break;
		case SP_JSON_NUMBER:
			printf ("number: %f\n", p.number);
			break;
		case SP_JSON_STRING:
			printf ("string: %.*s\n", (int)p.utf8.len, p.utf8.buf);
			break;
		}
	}
	sp_free (val, freelen);
	return sp_alloc_summary () ? 0 : 1;
}

