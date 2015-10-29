#include "../include/siphon/line.h"
#include "mu/mu.h"

#include <stdlib.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <inttypes.h>

typedef struct {
	struct {
		const uint8_t *buf;
		size_t len;
	} fields[8];
	size_t count;
} Message;

static bool
parse (SpLine *p, Message *msg, const uint8_t *in, size_t inlen, ssize_t speed)
{
	memset (msg, 0, sizeof *msg);

	const uint8_t *buf = in;
	const uint8_t *end = buf + inlen;
	size_t len, trim = 0;
	ssize_t rc;

	if (speed > 0) {
		len = speed;
	}
	else {
		len = inlen;
	}

	while (buf < end) {
		mu_assert_uint_ge (len, trim);
		if (len < trim) return false;

		rc = sp_line_next (p, buf, len - trim, len == inlen);

		mu_assert_int_ge (rc, 0);
		if (rc < 0) {
			return false;
		}

		if (p->type == SP_LINE_VALUE) {
			msg->fields[msg->count].buf = buf;
			msg->fields[msg->count].len = rc;
			msg->count++;
		}

		// trim the buffer
		buf += rc;
		trim += rc;

		if (speed > 0) {
			len += speed;
			if (len > inlen) {
				len = inlen;
			}
		}
	}

	return true;
}

static void
test_parse (ssize_t speed)
{
	SpLine p;
	sp_line_init (&p);

	static const uint8_t data[] = 
		"line 1\n"
		"line 2\n"
		"line 3\n"
		;

	Message msg;
	if (!parse (&p, &msg, data, sizeof data - 1, speed)) {
		return;
	}

	mu_fassert_uint_eq (msg.count, 3);

	char cmp[64];

	snprintf (cmp, sizeof cmp, "%.*s", (int)msg.fields[0].len, msg.fields[0].buf);
	mu_assert_str_eq (cmp, "line 1\n");

	snprintf (cmp, sizeof cmp, "%.*s", (int)msg.fields[1].len, msg.fields[1].buf);
	mu_assert_str_eq (cmp, "line 2\n");

	snprintf (cmp, sizeof cmp, "%.*s", (int)msg.fields[2].len, msg.fields[2].buf);
	mu_assert_str_eq (cmp, "line 3\n");
}

int
main (void)
{
	mu_init ("line");

	test_parse (-1);
	test_parse (1);

	test_parse (2);
	test_parse (11);
}

