#include "../include/siphon/json.h"
#include "../include/siphon/alloc.h"
#include "../include/siphon/fmt.h"

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

static void
indent (size_t n)
{
	static const char spaces[] = 
		"                                                                "
		"                                                                "
		"                                                                "
		"                                                                ";
	fwrite (spaces, 1, n*4, stdout);
}

int
main (int argc, char **argv)
{
	size_t len;
	uint8_t *val = readin (argc > 1 ? argv[1] : NULL, &len);
	uint8_t *buf = val;
	size_t freelen = len;
	bool first = true;
	bool key = false;

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

		if (p.type == SP_JSON_ARRAY || p.type == SP_JSON_OBJECT) {
			if (key) {
				printf (": ");
			}
			else if (!first) {
				printf (", ");
			}
			first = true;
		}
		else if (p.type == SP_JSON_ARRAY_END || p.type == SP_JSON_OBJECT_END) {
			first = false;
			printf ("\n");
			indent (p.depth);
		}
		else if (key) {
			printf (": ");
			first = false;
		}
		else if (first) {
			first = false;
			printf ("\n");
			indent (p.depth);
		}
		else {
			printf (",\n");
			indent (p.depth);
		}

		switch (p.type) {
		case SP_JSON_NONE:
			break;
		case SP_JSON_OBJECT:
			printf ("{");
			break;
		case SP_JSON_ARRAY:
			printf ("[");
			break;
		case SP_JSON_OBJECT_END:
			printf ("}");
			break;
		case SP_JSON_ARRAY_END:
			printf ("]");
			break;
		case SP_JSON_NULL:
			printf ("null");
			break;
		case SP_JSON_TRUE:
			printf ("true");
			break;
		case SP_JSON_FALSE:
			printf ("false");
			break;
		case SP_JSON_NUMBER:
			printf ("%f", p.number);
			break;
		case SP_JSON_STRING:
			sp_fmt_str (stdout, p.utf8.buf, p.utf8.len, true);
			break;
		}

		key = sp_json_is_key (&p);
	}
	printf ("\n");
	sp_free (val, freelen);
	sp_json_final (&p);
	return sp_alloc_summary () ? 0 : 1;
}

