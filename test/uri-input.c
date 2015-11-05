#include "../include/siphon/uri.h"
#include "../include/siphon/alloc.h"

#include <stdlib.h>
#include <stdio.h>
#include <err.h>

static char *
readin (size_t *outlen)
{
	char buffer[8192];

	if (fgets (buffer, sizeof buffer, stdin) == NULL) {
		err (EXIT_FAILURE, "fgets");
	}

	size_t len = strlen (buffer);
	while (buffer[len-1] == '\n') {
		len--;
	}

	char *str = sp_malloc (len);
	if (str == NULL) {
		err (EXIT_FAILURE, "sp_malloc");
	}

	memcpy (str, buffer, len);
	*outlen = len;
	return str;
}

int
main (void)
{
	size_t len;
	char *str = readin (&len);

	SpUri uri;
	ssize_t rc = sp_uri_parse (&uri, str, len);
	if (rc <= 0) {
		sp_free (str, len);
		errx (EXIT_FAILURE, "failed to parse");
	}
	sp_uri_print (&uri, str, stdout);
	sp_free (str, len);

	return 0;
}

