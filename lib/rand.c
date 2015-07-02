#include "siphon/rand.h"

#include <stdlib.h>
#include <fcntl.h>
#include <time.h>

#if defined(BSD) || defined( __APPLE__)

int
sp_rand_init (void)
{
	arc4random_stir ();
	return 0;
}

int
sp_rand (void *const restrict dst, size_t len)
{
	arc4random_buf (dst, len);
	return 0;
}

int
sp_rand_uint32 (uint32_t bound, uint32_t *out)
{
	*out = bound ? arc4random_uniform (bound) : arc4random ();
	return 0;
}

#elif defined(__linux__)

#include <errno.h>
#include <sys/stat.h>

static int fd = -1, err = ENOENT;

int
sp_rand_init (void)
{
	int err = 0;

	while (true) {
		fd = open ("/dev/urandom", O_RDONLY);
		if (fd < 0) {
			err = errno;
			if (err == EINTR) {
				// interrupted, try again
				err = 0;
				continue;
			}
			fprintf (stderr, "failed to open /dev/urandom: %s", strerror (err));
		}
		break;
	}

	// stat the fd so it can be verified
	struct stat sbuf;
	if (fstat (fd, &sbuf) < 0) {
		err = errno;
		close (fd);
		fd = -1;
		fprintf (stderr, "failed to stat /dev/urandom: %s", strerror (err));
		return -1;
	}

	// check that it is a char special
	if (!S_ISCHR (sbuf.st_mode) ||
			// verify that the device is /dev/random or /dev/urandom (linux only)
			(sbuf.st_rdev != makedev (1, 8) && sbuf.st_rdev != makedev (1, 9))) {
		err = ENODEV;
		close (fd);
		fd = -1;
		fprintf (stderr, "/dev/urandom is an invalid device");
		return -1;
	}

	return 0;
}

int
sp_rand (void *const restrict dst, size_t len)
{
	size_t amt = 0;
	while (amt < len) {
		ssize_t r = read (fd, (char *)dst+amt, len-amt);
		if (r > 0) {
			amt += (size_t)r;
		}
		else if (r == 0 || errno != EINTR) {
			return -1;
		}
	}
	return 0;
}

int
sp_rand_uint32 (uint32_t bound, uint32_t *out)
{
	uint32_t val;
	if (sp_rand (&val, sizeof val) < 0) {
		return -1;
	}

	if (bound) {
		val = ((double)val / (double)UINT32_MAX) * bound;
	}
	*out = val;
	return 0;
}

#endif

int
sp_rand_double (double *out)
{
	uint64_t val;
	if (sp_rand (&val, sizeof val) < 0) {
		return -1;
	}

	*out = (double)val / (double)UINT64_MAX;
	return 0;
}

