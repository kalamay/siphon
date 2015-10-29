#include "../include/siphon/http.h"
#include "../include/siphon/error.h"
#include "../include/siphon/alloc.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>

#include <ctype.h>

static void
print_string (FILE *out, const void *val, size_t len)
{
	for (const uint8_t *p = val, *pe = p + len; p < pe; p++) {
		if (isprint (*p)) {
			fputc (*p, out);
		}
		else if (*p >= '\a' && *p <= '\r') {
			static const char tab[] = "abtnvfr";
			fprintf (out, "\\%c", tab[*p - '\a']);
		}
		else {
			fprintf (out, "\\x%02X", *p);
		}
	}
	fputc ('\n', out);
}

static ssize_t
read_body (SpHttp *p, char *buf, size_t len)
{
	if (!p->as.body_start.chunked) {
		if (len < p->as.body_start.content_length) {
			return SP_HTTP_ESYNTAX;
		}
		print_string (stdout, buf, p->as.body_start.content_length);
		return p->as.body_start.content_length;
	}

	char *cur = buf;
	char *end = buf + len;
	do {
		ssize_t rc = sp_http_next (p, cur, len);
		if (rc < 0) return rc;
		cur += rc;
		len -= rc;
		if (p->type == SP_HTTP_BODY_CHUNK) {
			if (len < p->as.body_chunk.length) {
				return SP_HTTP_ESYNTAX;
			}
			print_string (stdout, cur, p->as.body_chunk.length);
			cur += p->as.body_chunk.length;
			len -= p->as.body_chunk.length;
		}
		else {
			break;
		}
	} while (cur < end);
	return cur - buf;
}

static char *
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

	char *copy = sp_malloc (len);
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
	sp_alloc_init (sp_alloc_debug, NULL);

	size_t len;
	char *buf = readin (argc > 1 ? argv[1] : NULL, &len);
	char *cur = buf;
	char *end = buf + len;
	ssize_t rc;

	SpHttp p;
	sp_http_init_request (&p);

	while (!sp_http_is_done (&p) && cur < end) {
		rc = sp_http_next (&p, cur, len);
		if (rc < 0) goto error;

		if (rc > 0) {
			sp_http_print (&p, cur, stdout);
			cur += rc;
			len -= rc;
			if (p.type == SP_HTTP_BODY_START) {
				rc = read_body (&p, cur, len);
				if (rc < 0) goto error;
				cur += rc;
				len -= rc;
			}
		}
	}

	sp_free (buf, len);

	return 0;

error:
	fprintf (stderr, "http error: %s\n", sp_strerror (rc));
	sp_free (buf, len);
	return 1;
}

