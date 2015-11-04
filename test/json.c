#include "../include/siphon/json.h"
#include "../include/siphon/error.h"
#include "mu/mu.h"

#include <stdlib.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <inttypes.h>

static bool debug = false;

typedef struct {
	struct {
		const uint8_t *string;
		double number;
		SpJsonType type;
	} fields[64];
	size_t field_count;
} Message;

#define DEBUG(fn) do { \
	debug = true;      \
	fn;                \
	debug = false;     \
} while (0)

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

static void
print_value (SpJson *p)
{
	if (p->type == SP_JSON_NONE) return;
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

static bool
parse (SpJson *p, Message *msg, const uint8_t *in, size_t inlen, ssize_t speed)
{
	memset (msg, 0, sizeof *msg);

	const uint8_t *buf = in;
	size_t len, trim = 0;
	ssize_t rc;

	if (speed > 0) {
		len = speed;
	}
	else {
		len = inlen;
	}

	while (!sp_json_is_done (p)) {
		mu_assert_uint_ge (len, trim);
		if (len < trim) return false;

		rc = sp_json_next (p, buf, len - trim, len == inlen);

		mu_assert_int_ge (rc, 0);
		if (rc < 0) {
			fprintf (stderr, "FAILED PARSING: ");
			print_string (stderr, buf, len - trim);
			return false;
		}

		switch (p->type) {
			case SP_JSON_STRING:
				msg->fields[msg->field_count].string = sp_utf8_steal (&p->utf8, NULL, NULL);
				msg->fields[msg->field_count++].type = SP_JSON_STRING;
				break;
			case SP_JSON_NUMBER:
				msg->fields[msg->field_count].number = p->number;
				msg->fields[msg->field_count++].type = SP_JSON_NUMBER;
				break;
			case SP_JSON_OBJECT:
			case SP_JSON_OBJECT_END:
			case SP_JSON_ARRAY:
			case SP_JSON_ARRAY_END:
			case SP_JSON_TRUE:
			case SP_JSON_FALSE:
			case SP_JSON_NULL:
				msg->fields[msg->field_count++].type = p->type;
				break;
			case SP_JSON_NONE:
				break;
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
	SpJson p;
	sp_json_init (&p);

	static const uint8_t data[] = "{\
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

	Message msg;
	if (!parse (&p, &msg, data, sizeof data - 1, speed)) {
		return;
	}

	mu_fassert_uint_eq (msg.field_count, 31);
	mu_fassert_int_eq (msg.fields[0].type, SP_JSON_OBJECT);
	mu_fassert_int_eq (msg.fields[1].type, SP_JSON_STRING);
	mu_fassert_str_eq (msg.fields[1].string, "test");
	mu_fassert_int_eq (msg.fields[2].type, SP_JSON_STRING);
	mu_fassert_str_eq (msg.fields[2].string, "value");
	mu_fassert_int_eq (msg.fields[3].type, SP_JSON_STRING);
	mu_fassert_str_eq (msg.fields[3].string, "foo");
	mu_fassert_int_eq (msg.fields[4].type, SP_JSON_STRING);
	mu_fassert_str_eq (msg.fields[4].string, "b\"a and \\ r");
	mu_fassert_int_eq (msg.fields[5].type, SP_JSON_STRING);
	mu_fassert_str_eq (msg.fields[5].string, "array");
	mu_fassert_int_eq (msg.fields[6].type, SP_JSON_ARRAY);
	mu_fassert_int_eq (msg.fields[7].type, SP_JSON_NUMBER);
	mu_fassert_int_eq (msg.fields[7].number, 1);
	mu_fassert_int_eq (msg.fields[8].type, SP_JSON_NUMBER);
	mu_fassert_int_eq (msg.fields[8].number, 2);
	mu_fassert_int_eq (msg.fields[9].type, SP_JSON_STRING);
	mu_fassert_str_eq (msg.fields[9].string, "three");
	mu_fassert_int_eq (msg.fields[10].type, SP_JSON_NUMBER);
	mu_fassert_int_eq (msg.fields[10].number, 4);
	mu_fassert_int_eq (msg.fields[11].type, SP_JSON_ARRAY_END);
	mu_fassert_int_eq (msg.fields[12].type, SP_JSON_STRING);
	mu_fassert_str_eq (msg.fields[12].string, "baz");
	mu_fassert_int_eq (msg.fields[13].type, SP_JSON_OBJECT);
	mu_fassert_int_eq (msg.fields[14].type, SP_JSON_STRING);
	mu_fassert_str_eq (msg.fields[14].string, "a");
	mu_fassert_int_eq (msg.fields[15].type, SP_JSON_STRING);
	mu_fassert_str_eq (msg.fields[15].string, "b");
	mu_fassert_int_eq (msg.fields[16].type, SP_JSON_OBJECT_END);
	mu_fassert_int_eq (msg.fields[17].type, SP_JSON_STRING);
	mu_fassert_str_eq (msg.fields[17].string, "num");
	mu_fassert_int_eq (msg.fields[18].type, SP_JSON_NUMBER);
	mu_fassert_int_eq (msg.fields[18].number * 100, 12345);
	mu_fassert_int_eq (msg.fields[19].type, SP_JSON_STRING);
	mu_fassert_str_eq (msg.fields[19].string, "bool");
	mu_fassert_int_eq (msg.fields[20].type, SP_JSON_TRUE);
	mu_fassert_int_eq (msg.fields[21].type, SP_JSON_STRING);
	mu_fassert_str_eq (msg.fields[21].string, "utf8");
	mu_fassert_int_eq (msg.fields[22].type, SP_JSON_STRING);
	mu_fassert_str_eq (msg.fields[22].string, "$¢€𤪤");
	mu_fassert_int_eq (msg.fields[23].type, SP_JSON_STRING);
	mu_fassert_str_eq (msg.fields[23].string, "key");
	mu_fassert_int_eq (msg.fields[24].type, SP_JSON_STRING);
	mu_fassert_str_eq (msg.fields[24].string, "value\n\"newline\"");
	mu_fassert_int_eq (msg.fields[25].type, SP_JSON_STRING);
	mu_fassert_str_eq (msg.fields[25].string, "obj");
	mu_fassert_int_eq (msg.fields[26].type, SP_JSON_OBJECT);
	mu_fassert_int_eq (msg.fields[27].type, SP_JSON_STRING);
	mu_fassert_str_eq (msg.fields[27].string, "true");
	mu_fassert_int_eq (msg.fields[28].type, SP_JSON_TRUE);
	mu_fassert_int_eq (msg.fields[29].type, SP_JSON_OBJECT_END);
	mu_fassert_int_eq (msg.fields[30].type, SP_JSON_OBJECT_END);

	sp_json_final (&p);
}

static ssize_t
parse_value (const uint8_t *m, ssize_t len, uint16_t depth)
{
	SpJson p;
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
			values = n;
			goto out;
		}
		else if (n > 0) {
			if (p.depth >= depth) {
				values = SP_JSON_ESYNTAX;
				goto out;
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
		values = SP_JSON_ESYNTAX;
	}

out:
	sp_json_final (&p);
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
	ssize_t rc = 0;

	fd = open (path, O_RDONLY, 0);
	if (fd < 0) {
		rc = SP_ESYSTEM (errno);
		goto out;
	}

	struct stat st;
	if (fstat (fd, &st) < 0) goto out;

	m = mmap (NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (m == MAP_FAILED) {
		rc = SP_ESYSTEM (errno);
		goto out;
	}

	rc = parse_value (m, st.st_size, depth);
	
out:
	if (m != NULL) munmap (m, st.st_size);
	if (fd > -1) close (fd);
	return rc;
}

int
main (void)
{
	mu_init ("json");

	test_parse (-1);
	test_parse (1);

	test_parse (2);
	test_parse (11);

	// XXX: allowing bare values
	//mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail1.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail2.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail3.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail4.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail5.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail6.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail7.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail8.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail9.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail10.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail11.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail12.json", 20));
	// XXX: allowing extended number formats for now
	//mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail13.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail14.json", 20));
	mu_assert_int_eq (SP_UTF8_EESCAPE, parse_file ("fail15.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail16.json", 20));
	mu_assert_int_eq (SP_UTF8_EESCAPE, parse_file ("fail17.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail18.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail19.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail20.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail21.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail22.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail23.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail24.json", 20));
	mu_assert_int_eq (SP_JSON_EBYTE,   parse_file ("fail25.json", 20));
	mu_assert_int_eq (SP_UTF8_EESCAPE, parse_file ("fail26.json", 20));
	mu_assert_int_eq (SP_JSON_EBYTE,   parse_file ("fail27.json", 20));
	mu_assert_int_eq (SP_UTF8_EESCAPE, parse_file ("fail28.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail29.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail30.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail31.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail32.json", 20));
	mu_assert_int_eq (SP_JSON_ESYNTAX, parse_file ("fail33.json", 20));

	mu_assert_int_eq (112, parse_file ("pass1.json", 20));
	mu_assert_int_eq (39, parse_file ("pass2.json", 20));
	mu_assert_int_eq (9, parse_file ("pass3.json", 20));
	mu_assert_int_eq (31, parse_file ("pass4.json", 20));
	mu_assert_int_eq (5276, parse_file ("pass5.json", 469));

	// XXX: these next three test non-object/array values as root
	mu_assert_int_eq (1, parse_file ("pass6.json", 20));
	mu_assert_int_eq (1, parse_file ("pass7.json", 20));
	mu_assert_int_eq (1, parse_file ("pass8.json", 20));
}

