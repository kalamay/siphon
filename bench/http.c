#include "netparse/http.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

static const char data[] =
    "POST /kalamay/netparse HTTP/1.1\r\n"
    "Host: github.com\r\n"
    "DNT: 1\r\n"
    "Accept-Encoding: gzip, deflate, sdch\r\n"
    "Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4\r\n"
    "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_3) "
		"AppleWebKit/537.36 (KHTML, like Gecko) "
		"Chrome/43.0.2357.81 Safari/537.36\r\n"
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,"
		"image/webp,*/*;q=0.8\r\n"
    "Referer: https://github.com/kalamay/netparse\r\n"
    "Connection: keep-alive\r\n"
    "Transfer-Encoding: chunked\r\n"
    "Cache-Control: max-age=0\r\n"
	"\r\n"
	"b\r\n"
	"hello world\r\n"
	"0\r\n"
	"\r\n";

static int
bench (int iter_count, int silent)
{
	NpHttp parser;
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

		np_http_init_request (&parser);
		while (p < pe) {
			ssize_t o = np_http_next (&parser, p, pe - p);
			if (o < 0) {
				printf ("ERR: %zd\n", o);
				return 1;
			}
			p += o;
			if (parser.type == NP_HTTP_BODY_CHUNK) {
				p += parser.as.body_chunk.length;
			}
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
			bench(5000000, 1);
		return 0;
	} else {
		return bench(5000000, 0);
	}
}

