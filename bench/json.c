#include "siphon/siphon.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

static const char data[] = "{\
	\"test\":\"value\",\
	\"foo\":\"b\\\"a and \\\\ r\",\
	\"array\":[1,2,\"three\",4],\
	\"baz\":{\"a\":\"b\"},\
	\"num\":123.45,\
	\"bool\":true,\
	\"utf8\":\"$¢€𤪤\",\
	\"key\":\"value\\n\\\"newline\\\"\",\
	\"obj\":{\"true\":true}\
}";

static int
bench (int iter_count, int silent)
{
	SpJson parser;
	int i;
	int err;
	struct timeval start;
	struct timeval end;
	float rps;

	if (!silent) {
		err = gettimeofday (&start, NULL);
		assert (err == 0);
	}

	for (i = 0; i < iter_count; i++) {
		const char *p = data;
		const char *pe = data + (sizeof (data) - 1);

		sp_json_init (&parser);
		while (p < pe) {
			ssize_t o = sp_json_next (&parser, p, pe - p, true);
			if (o < 0) {
				printf ("ERR: %zd\n", sp_strerror (o));
				return 1;
			}
			p += o;
		}
		assert (p == pe);
	}

	if (!silent) {
		err = gettimeofday (&end, NULL);
		assert (err == 0);

		fprintf (stdout, "Benchmark result:\n");

		rps = (float) (end.tv_sec - start.tv_sec) +
			(end.tv_usec - start.tv_usec) * 1e-6f;
		fprintf (stdout, "Took %f seconds to run\n", rps);

		rps = (float) iter_count / rps;
		fprintf (stdout, "%f req/sec\n", rps);
		fflush (stdout);
	}

	return 0;
}

int
main (int argc, char** argv)
{
	if (argc == 2 && strcmp(argv[1], "infinite") == 0) {
		for (;;)
			bench(1000000, 1);
		return 0;
	} else {
		return bench(1000000, 0);
	}
}

