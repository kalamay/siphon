#include "siphon/siphon.h"
#include "siphon/alloc.h"

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

	char *str = sp_mallocn (len, 1);
	if (str == NULL) {
		err (EXIT_FAILURE, "sp_mallocn");
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
		sp_free (str);
		errx (EXIT_FAILURE, "failed to parse");
	}
	sp_uri_print (&uri, str, stdout);
	sp_free (str);

	return 0;
}

