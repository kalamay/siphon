#include "siphon/siphon.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

static const char data[] =
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit."
	"Morbi mollis cursus metus vel tristique."
	"Proin congue massa massa, a malesuada dolor ullamcorper a."
	"Nulla eget leo vel orci venenatis placerat."
	"Donec semper condimentum justo, vel sollicitudin dolor consequat id."
	"Nunc sed aliquet felis, eget congue nisi."
	"Mauris eu justo suscipit, elementum turpis ut, molestie tellus."
	"Mauris ornare rutrum fringilla. Nulla dignissim luctus pretium."
	"Nullam nec eros hendrerit sapien pellentesque sollicitudin."
	"Integer eget ligula dui. Mauris nec cursus nibh."
	"Nunc interdum elementum leo, eu sagittis eros sodales nec."
	"Duis dictum nulla sed tincidunt malesuada. Quisque in vulputate sapien."
	"Sed sit amet tellus a est porta rhoncus sed eu metus."
	"Mauris non pulvinar nisl, volutpat luctus enim."
	"Suspendisse est nisi, sagittis at risus quis, ultricies rhoncus sem."
	"Donec ullamcorper purus eget sapien facilisis, eu eleifend felis viverra."
	"Suspendisse elit neque, semper aliquet neque sed, egestas tempus leo. "
	"uis condimentum turpis duis.";

static int
bench (int iter_count, int silent)
{
	int i;
	int err;
	struct timeval start;
	struct timeval end;
	float rps;

	if (!silent) {
		err = gettimeofday (&start, NULL);
		assert (err == 0);
	}

	uint32_t crc = 0;
	for (i = 0; i < iter_count; i++) {
		crc = sp_crc32c (crc, data, (sizeof data) - 1);
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

	return crc == 4053652342 ? 0 : 1;
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

