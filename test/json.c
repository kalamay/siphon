#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <inttypes.h>

#include "siphon/siphon.h"
#include "mu/mu.h"

static bool debug = false;

#define DEBUG(fn) do { \
	debug = true;      \
	fn;                \
	debug = false;     \
} while (0)

static void
print_value (SpJson *p)
{
	int d = (int)p->depth;
	if (p->type == SP_JSON_OBJECT || p->type == SP_JSON_ARRAY) {
		d--;
	}
	printf ("%.*s", d*2, "                                               ");
	switch (p->type) {
		case SP_JSON_ARRAY: case SP_JSON_ARRAY_END:
		case SP_JSON_OBJECT: case SP_JSON_OBJECT_END:
			printf ("%c\n", p->type);
			break;
		case SP_JSON_STRING:
			printf ("\"%.*s\"\n", (int)p->utf8.len, p->utf8.buf);
			break;
		case SP_JSON_NUMBER:
			printf ("%g\n", p->number);
			break;
		case SP_JSON_TRUE:
			printf ("true\n");
			break;
		case SP_JSON_FALSE:
			printf ("false\n");
			break;
		case SP_JSON_NULL:
			printf ("null\n");
			break;
		case SP_JSON_NONE:
			break;
	}
}

static ssize_t
parse_value (const uint8_t *m, ssize_t len, uint16_t depth)
{
	SpJson p;
	//printf ("SIZE: %zu\n", sizeof p);
	sp_json_init (&p);

	ssize_t values = 0;
	ssize_t off = 0;

	if (debug) {
		printf ("LEN: %zu\n", len);
	}

	do {
		ssize_t n = sp_json_next (&p, m, len - off, true);
		if (n < 0) {
			if (debug) {
				printf ("ERROR: %s, %zd\n", sp_strerror (n), off);
			}
			return n;
		}
		else if (n > 0) {
			if (p.depth >= depth) {
				return SP_ESYNTAX;
			}
			if (p.type != SP_JSON_NONE) {
				values++;
				if (debug) {
					print_value (&p);
				}
			}
			if (debug) {
				//printf ("TRIM: '%.*s'\n", (int)n, m);
			}
			m += n;
			off += n;
		}
	} while (!sp_json_is_done (&p));

	ssize_t rem = len - off;
	if (rem > 1 || (rem == 1 && !isspace (*m))) {
		return SP_ESYNTAX;
	}
	return values;
}

static ssize_t
parse_file (const char *name, uint16_t depth)
{
	char path[4096] = __FILE__;
	size_t len = strlen (path);
	path[len-2] = '/';
	strcpy (path + len - 1, name);

	int fd = -1;
	void *m = NULL;
	ssize_t rc = SP_ESYSTEM;

	fd = open (path, O_RDONLY, 0);
	if (fd < 0) goto out;

	struct stat st;
	if (fstat (fd, &st) < 0) goto out;

	m = mmap (NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (m == NULL) goto out;

	rc = parse_value (m, st.st_size, depth);
	
out:
	if (m != NULL) munmap (m, st.st_size);
	if (fd > -1) close (fd);
	return rc;
}

int
main (void)
{
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail1.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail2.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail3.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail4.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail5.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail6.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail7.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail8.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail9.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail10.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail11.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail12.json", 20));
	//mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail13.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail14.json", 20));
	mu_assert_int_eq (SP_EESCAPE, parse_file ("fail15.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail16.json", 20));
	mu_assert_int_eq (SP_EESCAPE, parse_file ("fail17.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail18.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail19.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail20.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail21.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail22.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail23.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail24.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail25.json", 20));
	mu_assert_int_eq (SP_EESCAPE, parse_file ("fail26.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail27.json", 20));
	mu_assert_int_eq (SP_EESCAPE, parse_file ("fail28.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail29.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail30.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail31.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail32.json", 20));
	mu_assert_int_eq (SP_ESYNTAX, parse_file ("fail33.json", 20));

	mu_assert_int_eq (112, parse_file ("pass1.json", 20));
	mu_assert_int_eq (39, parse_file ("pass2.json", 20));
	mu_assert_int_eq (9, parse_file ("pass3.json", 20));
	mu_assert_int_eq (31, parse_file ("pass4.json", 20));
	mu_assert_int_eq (5276, parse_file ("pass5.json", 469));

	mu_exit ("json");
}

