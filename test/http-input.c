#include "siphon/siphon.h"
#include "siphon/alloc.h"

#include <stdlib.h>
#include <stdio.h>
#include <err.h>

#include <ctype.h>

static char *
readin (size_t *outlen)
{
	char buffer[8192];

	size_t len = fread (buffer, 1, sizeof buffer, stdin);
	if (len == 0) {
		err (EXIT_FAILURE, "fgets");
	}

	char *str = sp_mallocn (len, 1);
	if (str == NULL) {
		err (EXIT_FAILURE, "sp_mallocn");
	}

	memcpy (str, buffer, len);
	*outlen = len;
	return str;
}

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
			return SP_ESYNTAX;
		}
		print_string (stdout, buf, p->as.body_start.content_length);
		return p->as.body_start.content_length;
	}

	char *cur = buf;
	do {
		ssize_t rc = sp_http_next (p, cur, len);
		if (rc < 0) return rc;
		cur += rc;
		len -= rc;
		if (p->type == SP_HTTP_BODY_CHUNK) {
			if (len < p->as.body_chunk.length) {
				return SP_ESYNTAX;
			}
			print_string (stdout, cur, p->as.body_chunk.length);
			cur += p->as.body_chunk.length;
			len -= p->as.body_chunk.length;
		}
		else {
			break;
		}
	} while (true);
	return cur - buf;
}

int
main (void)
{
	size_t len;
	char *buf = readin (&len);
	char *cur = buf;
	ssize_t rc;

	SpHttp p;
	sp_http_init_request (&p);

	while (!sp_http_is_done (&p)) {
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

	sp_free (buf);
	return 0;

error:
	fprintf (stderr, "http error: %s\n", sp_strerror (rc));
	sp_free (buf);
	return 1;
}

