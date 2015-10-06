#include "siphon/msgpack.h"
#include "mu/mu.h"

#include <math.h>

#define count(a) (sizeof (a) / sizeof ((a)[0]))

typedef struct {
	struct {
		SpMsgpackTag tag;
		SpMsgpackType type;
		uint8_t str[64];
	} fields[64];
	size_t field_count;
} Message;

static bool
parse (SpMsgpack *p, Message *msg, const uint8_t *in, size_t inlen, ssize_t speed)
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

	while (!sp_msgpack_is_done (p)) {
		mu_assert_uint_ge (len, trim);
		if (len < trim) return false;

		rc = sp_msgpack_next (p, buf, len - trim, len == inlen);

		mu_assert_int_ge (rc, 0);
		if (rc < 0) {
			return false;
		}

		// trim the buffer
		buf += rc;
		trim += rc;

		switch (p->type) {
			case SP_MSGPACK_STRING:
			case SP_MSGPACK_BINARY:
				memcpy (msg->fields[msg->field_count].str, buf, p->tag.count);
				msg->fields[msg->field_count].str[p->tag.count] = '\0';
				buf += p->tag.count;
				// fallthrough
			case SP_MSGPACK_NIL:
			case SP_MSGPACK_TRUE:
			case SP_MSGPACK_FALSE:
			case SP_MSGPACK_NEGATIVE:
			case SP_MSGPACK_POSITIVE:
			case SP_MSGPACK_FLOAT:
			case SP_MSGPACK_DOUBLE:
			case SP_MSGPACK_ARRAY:
			case SP_MSGPACK_ARRAY_END:
			case SP_MSGPACK_MAP:
			case SP_MSGPACK_MAP_END:
			case SP_MSGPACK_EXT:
				msg->fields[msg->field_count].tag = p->tag;
				msg->fields[msg->field_count].type = p->type;
				msg->field_count++;
				break;
			case SP_MSGPACK_NONE:
				break;
		}

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
	/*
	 * [
	 *   "test",
	 *   "this is a longer string that should not pack to a fixed string",
	 *   True, False,
	 *   12, 123, 1234, 1234567, 123456789000,
	 *   -12, -123, -1234, -1234567, -123456789000,
	 *   {
	 *     'a': 1.23,
	 *     'b': True,
	 *     'c': 'value',
	 *     'd': [1,2,3],
	 *     'e': { 'x': 'y' }
	 *   }
	 * ]
	 */
	static const uint8_t data[] = {
		0x9f,0xa4,0x74,0x65,0x73,0x74,0xd9,0x3e,0x74,0x68,0x69,0x73,0x20,0x69,0x73,0x20,
		0x61,0x20,0x6c,0x6f,0x6e,0x67,0x65,0x72,0x20,0x73,0x74,0x72,0x69,0x6e,0x67,0x20,
		0x74,0x68,0x61,0x74,0x20,0x73,0x68,0x6f,0x75,0x6c,0x64,0x20,0x6e,0x6f,0x74,0x20,
		0x70,0x61,0x63,0x6b,0x20,0x74,0x6f,0x20,0x61,0x20,0x66,0x69,0x78,0x65,0x64,0x20,
		0x73,0x74,0x72,0x69,0x6e,0x67,0xc3,0xc2,0x0c,0x7b,0xcd,0x04,0xd2,0xce,0x00,0x12,
		0xd6,0x87,0xcf,0x00,0x00,0x00,0x1c,0xbe,0x99,0x1a,0x08,0xf4,0xd0,0x85,0xd1,0xfb,
		0x2e,0xd2,0xff,0xed,0x29,0x79,0xd3,0xff,0xff,0xff,0xe3,0x41,0x66,0xe5,0xf8,0x85,
		0xa1,0x61,0xcb,0x3f,0xf3,0xae,0x14,0x7a,0xe1,0x47,0xae,0xa1,0x62,0xc3,0xa1,0x63,
		0xa5,0x76,0x61,0x6c,0x75,0x65,0xa1,0x64,0x93,0x01,0x02,0x03,0xa1,0x65,0x81,0xa1,
		0x78,0xa1,0x79
	};

	SpMsgpack p;
	sp_msgpack_init (&p);

	Message msg;
	if (!parse (&p, &msg, data, sizeof data - 1, speed)) {
		return;
	}
	
	mu_assert_uint_eq (msg.field_count, 35);
	if (msg.field_count != 35) return;

	mu_assert_int_eq (msg.fields[0].type, SP_MSGPACK_ARRAY);
	mu_assert_uint_eq (msg.fields[0].tag.count, 15);
	mu_assert_int_eq (msg.fields[1].type, SP_MSGPACK_STRING);
	mu_assert_str_eq (msg.fields[1].str, "test");
	mu_assert_int_eq (msg.fields[2].type, SP_MSGPACK_STRING);
	mu_assert_str_eq (msg.fields[2].str, "this is a longer string that should not pack to a fixed string");
	mu_assert_int_eq (msg.fields[3].type, SP_MSGPACK_TRUE);
	mu_assert_int_eq (msg.fields[4].type, SP_MSGPACK_FALSE);
	mu_assert_int_eq (msg.fields[5].type, SP_MSGPACK_POSITIVE);
	mu_assert_uint_eq (msg.fields[5].tag.u64, 12);
	mu_assert_int_eq (msg.fields[6].type, SP_MSGPACK_POSITIVE);
	mu_assert_uint_eq (msg.fields[6].tag.u64, 123);
	mu_assert_int_eq (msg.fields[7].type, SP_MSGPACK_POSITIVE);
	mu_assert_uint_eq (msg.fields[7].tag.u64, 1234);
	mu_assert_int_eq (msg.fields[8].type, SP_MSGPACK_POSITIVE);
	mu_assert_uint_eq (msg.fields[8].tag.u64, 1234567);
	mu_assert_int_eq (msg.fields[9].type, SP_MSGPACK_POSITIVE);
	mu_assert_uint_eq (msg.fields[9].tag.u64, 123456789000);
	mu_assert_int_eq (msg.fields[10].type, SP_MSGPACK_NEGATIVE);
	mu_assert_int_eq (msg.fields[10].tag.i64, -12);
	mu_assert_int_eq (msg.fields[11].type, SP_MSGPACK_NEGATIVE);
	mu_assert_int_eq (msg.fields[11].tag.i64, -123);
	mu_assert_int_eq (msg.fields[12].type, SP_MSGPACK_NEGATIVE);
	mu_assert_int_eq (msg.fields[12].tag.i64, -1234);
	mu_assert_int_eq (msg.fields[13].type, SP_MSGPACK_NEGATIVE);
	mu_assert_int_eq (msg.fields[13].tag.i64, -1234567);
	mu_assert_int_eq (msg.fields[14].type, SP_MSGPACK_NEGATIVE);
	mu_assert_int_eq (msg.fields[14].tag.i64, -123456789000);
	mu_assert_int_eq (msg.fields[15].type, SP_MSGPACK_MAP);
	mu_assert_uint_eq (msg.fields[15].tag.count, 5);
	mu_assert_int_eq (msg.fields[16].type, SP_MSGPACK_STRING);
	mu_assert_str_eq (msg.fields[16].str, "a");
	mu_assert_int_eq (msg.fields[17].type, SP_MSGPACK_DOUBLE);
	mu_assert_int_eq (round (msg.fields[17].tag.f64 * 100), 123);
	mu_assert_int_eq (msg.fields[18].type, SP_MSGPACK_STRING);
	mu_assert_str_eq (msg.fields[18].str, "b");
	mu_assert_int_eq (msg.fields[19].type, SP_MSGPACK_TRUE);
	mu_assert_int_eq (msg.fields[20].type, SP_MSGPACK_STRING);
	mu_assert_str_eq (msg.fields[20].str, "c");
	mu_assert_int_eq (msg.fields[21].type, SP_MSGPACK_STRING);
	mu_assert_str_eq (msg.fields[21].str, "value");
	mu_assert_int_eq (msg.fields[22].type, SP_MSGPACK_STRING);
	mu_assert_str_eq (msg.fields[22].str, "d");
	mu_assert_int_eq (msg.fields[23].type, SP_MSGPACK_ARRAY);
	mu_assert_int_eq (msg.fields[23].tag.count, 3);
	mu_assert_int_eq (msg.fields[24].type, SP_MSGPACK_POSITIVE);
	mu_assert_uint_eq (msg.fields[24].tag.u64, 1);
	mu_assert_int_eq (msg.fields[25].type, SP_MSGPACK_POSITIVE);
	mu_assert_uint_eq (msg.fields[25].tag.u64, 2);
	mu_assert_int_eq (msg.fields[26].type, SP_MSGPACK_POSITIVE);
	mu_assert_uint_eq (msg.fields[26].tag.u64, 3);
	mu_assert_int_eq (msg.fields[27].type, SP_MSGPACK_ARRAY_END);
	mu_assert_int_eq (msg.fields[28].type, SP_MSGPACK_STRING);
	mu_assert_str_eq (msg.fields[28].str, "e");
	mu_assert_int_eq (msg.fields[29].type, SP_MSGPACK_MAP);
	mu_assert_uint_eq (msg.fields[29].tag.count, 1);
	mu_assert_int_eq (msg.fields[30].type, SP_MSGPACK_STRING);
	mu_assert_str_eq (msg.fields[30].str, "x");
	mu_assert_int_eq (msg.fields[31].type, SP_MSGPACK_STRING);
	mu_assert_str_eq (msg.fields[31].str, "y");
	mu_assert_int_eq (msg.fields[32].type, SP_MSGPACK_MAP_END);
	mu_assert_int_eq (msg.fields[33].type, SP_MSGPACK_MAP_END);
	mu_assert_int_eq (msg.fields[34].type, SP_MSGPACK_ARRAY_END);
}

static void
test_encode_nil (void)
{
	uint8_t buf[64];
	ssize_t rc;

	rc = sp_msgpack_enc_nil (buf);
	mu_assert_int_eq (rc, 1);

	SpMsgpack p;
	sp_msgpack_init (&p);

	rc = sp_msgpack_next (&p, buf, 1, true);
	mu_assert_int_eq (rc, 1);
	mu_assert_int_eq (p.type, SP_MSGPACK_NIL);
}

static void
test_encode_true (void)
{
	uint8_t buf[64];
	ssize_t rc;

	rc = sp_msgpack_enc_true (buf);
	mu_assert_int_eq (rc, 1);

	SpMsgpack p;
	sp_msgpack_init (&p);

	rc = sp_msgpack_next (&p, buf, 1, true);
	mu_assert_int_eq (rc, 1);
	mu_assert_int_eq (p.type, SP_MSGPACK_TRUE);
}

static void
test_encode_false (void)
{
	uint8_t buf[64];
	ssize_t rc;

	rc = sp_msgpack_enc_false (buf);
	mu_assert_int_eq (rc, 1);

	SpMsgpack p;
	sp_msgpack_init (&p);

	rc = sp_msgpack_next (&p, buf, 1, true);
	mu_assert_int_eq (rc, 1);
	mu_assert_int_eq (p.type, SP_MSGPACK_FALSE);
}

static void
test_encode_negative (void)
{
	uint8_t buf[64], *m;
	ssize_t rc;

	int64_t values[] = { -12, -123, -1234, -1234567, -123456789000 };

	m = buf;
	for (size_t i = 0; i < count (values); i++) {
		rc = sp_msgpack_enc_negative (m, values[i]);
		mu_assert_int_ge (rc, 1);
		if (rc <= 0) return;
		m += rc;
	}

	SpMsgpack p;
	sp_msgpack_init (&p);

	m = buf;
	for (size_t i = 0; i < count (values); i++) {
		rc = sp_msgpack_next (&p, m, sizeof buf - (m - buf), true);
		mu_assert_int_ge (rc, 1);
		if (rc <= 0) return;
		mu_assert_int_eq (p.type, SP_MSGPACK_NEGATIVE);
		mu_assert_int_eq (p.tag.i64, values[i]);
		m += rc;
	}
}

static void
test_encode_positive (void)
{
	uint8_t buf[64], *m;
	ssize_t rc;

	uint64_t values[] = { 12, 123, 1234, 1234567, 123456789000 };

	m = buf;
	for (size_t i = 0; i < count (values); i++) {
		rc = sp_msgpack_enc_positive (m, values[i]);
		mu_assert_int_ge (rc, 1);
		if (rc <= 0) return;
		m += rc;
	}

	SpMsgpack p;
	sp_msgpack_init (&p);

	m = buf;
	for (size_t i = 0; i < count (values); i++) {
		rc = sp_msgpack_next (&p, m, sizeof buf - (m - buf), true);
		mu_assert_int_ge (rc, 1);
		if (rc <= 0) return;
		mu_assert_int_eq (p.type, SP_MSGPACK_POSITIVE);
		mu_assert_int_eq (p.tag.i64, values[i]);
		m += rc;
	}
}

static void
test_encode_float (void)
{
	uint8_t buf[64], *m;
	ssize_t rc;

	float values[] = { 1.0, 1.23, 1.234, -123.4567, 0.123456789 };

	m = buf;
	for (size_t i = 0; i < count (values); i++) {
		rc = sp_msgpack_enc_float (m, values[i]);
		mu_assert_int_eq (rc, 5);
		if (rc <= 0) return;
		m += rc;
	}

	SpMsgpack p;
	sp_msgpack_init (&p);

	m = buf;
	for (size_t i = 0; i < count (values); i++) {
		rc = sp_msgpack_next (&p, m, sizeof buf - (m - buf), true);
		mu_assert_int_eq (rc, 5);
		if (rc <= 0) return;
		mu_assert_int_eq (p.type, SP_MSGPACK_FLOAT);
		char s1[16], s2[16];
		snprintf (s1, 16, "%f", p.tag.f32);
		snprintf (s2, 16, "%f", values[i]);
		mu_assert_str_eq (s1, s2);
		m += rc;
	}
}

static void
test_encode_double (void)
{
	uint8_t buf[64], *m;
	ssize_t rc;

	double values[] = { 1.0, 1.23, 1.234, -123.4567, 0.123456789 };

	m = buf;
	for (size_t i = 0; i < count (values); i++) {
		rc = sp_msgpack_enc_double (m, values[i]);
		mu_assert_int_eq (rc, 9);
		if (rc <= 0) return;
		m += rc;
	}

	SpMsgpack p;
	sp_msgpack_init (&p);

	m = buf;
	for (size_t i = 0; i < count (values); i++) {
		rc = sp_msgpack_next (&p, m, sizeof buf - (m - buf), true);
		mu_assert_int_eq (rc, 9);
		if (rc <= 0) return;
		mu_assert_int_eq (p.type, SP_MSGPACK_DOUBLE);
		char s1[16], s2[16];
		snprintf (s1, 16, "%f", p.tag.f64);
		snprintf (s2, 16, "%f", values[i]);
		mu_assert_str_eq (s1, s2);
		m += rc;
	}
}

static void
test_encode_string ()
{
	uint8_t buf[128], *m;
	ssize_t rc;

	const char *values[] = {
		"test",
		"this is a longer string that should not pack to a fixed string"
	};

	m = buf;
	for (size_t i = 0; i < count (values); i++) {
		size_t len = strlen (values[i]);
		rc = sp_msgpack_enc_string (m, len);
		mu_assert_int_ge (rc, 1);
		if (rc <= 0) return;
		m += rc;
		memcpy (m, values[i], len);
		m += len;
	}

	SpMsgpack p;
	sp_msgpack_init (&p);

	m = buf;
	for (size_t i = 0; i < count (values); i++) {
		rc = sp_msgpack_next (&p, m, sizeof buf - (m - buf), true);
		mu_assert_int_ge (rc, 1);
		if (rc <= 0) return;
		mu_assert_int_eq (p.type, SP_MSGPACK_STRING);
		m += rc;

		char s[64];
		memcpy (s, m, p.tag.count);
		s[p.tag.count] = '\0';
		mu_assert_str_eq (s, values[i]);
		m += p.tag.count;
	}
}

static void
test_encode_binary ()
{
	uint8_t buf[128], *m;
	ssize_t rc;

	const char *values[] = {
		"test",
		"this is a longer string that should not pack to a fixed string"
	};

	m = buf;
	for (size_t i = 0; i < count (values); i++) {
		size_t len = strlen (values[i]);
		rc = sp_msgpack_enc_binary (m, len);
		mu_assert_int_eq (rc, 2);
		if (rc <= 0) return;
		m += rc;
		memcpy (m, values[i], len);
		m += len;
	}

	SpMsgpack p;
	sp_msgpack_init (&p);

	m = buf;
	for (size_t i = 0; i < count (values); i++) {
		rc = sp_msgpack_next (&p, m, sizeof buf - (m - buf), true);
		mu_assert_int_eq (rc, 2);
		if (rc <= 0) return;
		mu_assert_int_eq (p.type, SP_MSGPACK_BINARY);
		m += rc;

		char s[64];
		memcpy (s, m, p.tag.count);
		s[p.tag.count] = '\0';
		mu_assert_str_eq (s, values[i]);
		m += p.tag.count;
	}
}

static void
test_encode_array ()
{
	uint8_t buf[64], *m;
	ssize_t rc;

	uint64_t values[] = { 12, 123, 1234, 1234567, 1234567890 };

	m = buf;
	for (size_t i = 0; i < count (values); i++) {
		rc = sp_msgpack_enc_array (m, values[i]);
		mu_assert_int_ge (rc, 1);
		if (rc <= 0) return;
		m += rc;
	}

	SpMsgpack p;
	sp_msgpack_init (&p);

	m = buf;
	for (size_t i = 0; i < count (values); i++) {
		rc = sp_msgpack_next (&p, m, sizeof buf - (m - buf), true);
		mu_assert_int_ge (rc, 1);
		if (rc <= 0) return;
		mu_assert_int_eq (p.type, SP_MSGPACK_ARRAY);
		mu_assert_int_eq (p.tag.count, values[i]);
		m += rc;
	}
}

static void
test_encode_map ()
{
	uint8_t buf[64], *m;
	ssize_t rc;

	uint64_t values[] = { 12, 123, 1234, 1234567, 1234567890 };

	m = buf;
	for (size_t i = 0; i < count (values); i++) {
		rc = sp_msgpack_enc_map (m, values[i]);
		mu_assert_int_ge (rc, 1);
		if (rc <= 0) return;
		m += rc;
	}

	SpMsgpack p;
	sp_msgpack_init (&p);

	m = buf;
	for (size_t i = 0; i < count (values); i++) {
		rc = sp_msgpack_next (&p, m, sizeof buf - (m - buf), true);
		mu_assert_int_ge (rc, 1);
		if (rc <= 0) return;
		mu_assert_int_eq (p.type, SP_MSGPACK_MAP);
		mu_assert_int_eq (p.tag.count, values[i]);
		m += rc;
	}
}

static void
test_encode_ext ()
{
	uint8_t buf[352], *m;
	ssize_t rc;

	uint32_t lengths[] = { 1, 2, 3, 4, 5, 8, 10, 16, 17, 256 };

	m = buf;
	for (size_t i = 0; i < count (lengths); i++) {
		rc = sp_msgpack_enc_ext (m, (int8_t)i, lengths[i]);
		mu_assert_int_ge (rc, 2);
		if (rc <= 0) return;
		m += rc;
		memset (m, 0xff, lengths[i]);
		m += lengths[i];
	}

	SpMsgpack p;
	sp_msgpack_init (&p);

	m = buf;
	for (size_t i = 0; i < count (lengths); i++) {
		rc = sp_msgpack_next (&p, m, sizeof buf - (m - buf), true);
		mu_assert_int_ge (rc, 2);
		if (rc <= 0) return;
		mu_assert_int_eq (p.type, SP_MSGPACK_EXT);
		mu_assert_int_eq (p.tag.ext.type, (int8_t)i);
		mu_assert_int_eq (p.tag.ext.len, lengths[i]);
		m += rc + p.tag.ext.len;
		if (m > buf + sizeof buf) {
			mu_fail ("invalid buffer range");
			return;
		}
	}
}

int
main (void)
{
	test_parse (-1);
	test_parse (1);

	test_parse (2);
	test_parse (11);

	test_encode_nil ();
	test_encode_true ();
	test_encode_false ();
	test_encode_negative ();
	test_encode_positive ();
	test_encode_float ();
	test_encode_double ();
	test_encode_string ();
	test_encode_binary ();
	test_encode_array ();
	test_encode_map ();
	test_encode_ext ();

	mu_exit ("msgpack");
}

