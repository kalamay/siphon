#include "../include/siphon/utf8.h"
#include "../include/siphon/alloc.h"
#include "../include/siphon/error.h"
#include "mu.h"

static void
test_err1 (void)
{
	// 80 is too high for single byte and too low for a multi-byte sequence
	const uint8_t m[] = "\x80";
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_json_decode (&u, m, sizeof (m) - 1, 0);
	mu_assert_int_eq (n, SP_UTF8_EENCODING);
	sp_utf8_final (&u);
}

static void
test_err2 (void)
{
	// BF is lower than allowable first byte continuation
	const uint8_t m[] = "\xBF\x80";
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_json_decode (&u, m, sizeof (m) - 1, 0);
	mu_assert_int_eq (n, SP_UTF8_EENCODING);
	sp_utf8_final (&u);
}

static void
test_err3 (void)
{
	// 7F is too low for second byte
	const uint8_t m[] = "\xC0\x7F";
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_json_decode (&u, m, sizeof (m) - 1, 0);
	mu_assert_int_eq (n, SP_UTF8_EENCODING);
	sp_utf8_final (&u);
}

static void
test_err4 (void)
{
	// C0 is too high for second byte
	const uint8_t m[] = "\xDF\xC0";
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_json_decode (&u, m, sizeof (m) - 1, 0);
	mu_assert_int_eq (n, SP_UTF8_EENCODING);
	sp_utf8_final (&u);
}

static void
test_err5 (void)
{
	// 9F is too low for second byte
	const uint8_t m[] = "\xE0\x9f\x80";
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_json_decode (&u, m, sizeof (m) - 1, 0);
	mu_assert_int_eq (n, SP_UTF8_EENCODING);
	sp_utf8_final (&u);
}

static void
test_err6 (void)
{
	// C0 is too high for second byte
	const uint8_t m[] = "\xE0\xC0\x80";
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_json_decode (&u, m, sizeof (m) - 1, 0);
	mu_assert_int_eq (n, SP_UTF8_EENCODING);
	sp_utf8_final (&u);
}

static void
test_err7 (void)
{
	// 7F is too low for third byte
	const uint8_t m[] = "\xE0\xA0\x7F";
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_json_decode (&u, m, sizeof (m) - 1, 0);
	mu_assert_int_eq (n, SP_UTF8_EENCODING);
	sp_utf8_final (&u);
}

static void
test_err8 (void)
{
	// C0 is too high for third byte
	const uint8_t m[] = "\xE0\xBF\xC0";
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_json_decode (&u, m, sizeof (m) - 1, 0);
	mu_assert_int_eq (n, SP_UTF8_EENCODING);
	sp_utf8_final (&u);
}

static void
test_err9 (void)
{
	// 8F is too low for second byte
	const uint8_t m[] = "\xF0\x8f\x80\x80";
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_json_decode (&u, m, sizeof (m) - 1, 0);
	mu_assert_int_eq (n, SP_UTF8_EENCODING);
	sp_utf8_final (&u);
}

static void
test_err10 (void)
{
	// C0 is too high for second byte
	const uint8_t m[] = "\xF0\xC0\x80\x80";
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_json_decode (&u, m, sizeof (m) - 1, 0);
	mu_assert_int_eq (n, SP_UTF8_EENCODING);
	sp_utf8_final (&u);
}

static void
test_err11 (void)
{
	// 7F is too low for second byte
	const uint8_t m[] = "\xF4\x7f\x80\x80";
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_json_decode (&u, m, sizeof (m) - 1, 0);
	mu_assert_int_eq (n, SP_UTF8_EENCODING);
	sp_utf8_final (&u);
}

static void
test_err12 (void)
{
	// C0 is too high for second byte
	const uint8_t m[] = "\xF4\xC0\x80\x80";
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_json_decode (&u, m, sizeof (m) - 1, 0);
	mu_assert_int_eq (n, SP_UTF8_EENCODING);
	sp_utf8_final (&u);
}

static void
test_err13 (void)
{
	// 7F is too low for third byte
	const uint8_t m[] = "\xF4\x80\x7F\x80";
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_json_decode (&u, m, sizeof (m) - 1, 0);
	mu_assert_int_eq (n, SP_UTF8_EENCODING);
	sp_utf8_final (&u);
}

static void
test_err14 (void)
{
	// C0 is too high for third byte
	const uint8_t m[] = "\xF4\xBF\xC0\x80";
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_json_decode (&u, m, sizeof (m) - 1, 0);
	mu_assert_int_eq (n, SP_UTF8_EENCODING);
	sp_utf8_final (&u);
}

static void
test_err15 (void)
{
	// 7F is too low for fourth byte
	const uint8_t m[] = "\xF4\x80\x80\x7F";
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_json_decode (&u, m, sizeof (m) - 1, 0);
	mu_assert_int_eq (n, SP_UTF8_EENCODING);
	sp_utf8_final (&u);
}

static void
test_err16 (void)
{
	// C0 is too high for fourth byte
	const uint8_t m[] = "\xF4\xBF\x80\xC0";
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_json_decode (&u, m, sizeof (m) - 1, 0);
	mu_assert_int_eq (n, SP_UTF8_EENCODING);
	sp_utf8_final (&u);
}

static void
test_single_bytes (void)
{
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n;

	for (int cp = 0; cp < 0x7F; cp++) {
		n = sp_utf8_add_codepoint (&u, cp);
		mu_assert_int_eq (n, 1);
		mu_assert_int_eq (cp, sp_utf8_codepoint (u.buf, u.len));
		sp_utf8_reset (&u);
	}

	sp_utf8_final (&u);
}

static void
test_double_bytes (void)
{
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n;

	for (int cp = 0x80; cp < 0x7FF; cp++) {
		n = sp_utf8_add_codepoint (&u, cp);
		mu_assert_int_eq (n, 2);
		mu_assert_int_eq (cp, sp_utf8_codepoint (u.buf, u.len));
		sp_utf8_reset (&u);
	}

	sp_utf8_final (&u);
}

static void
test_triple_bytes (void)
{
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n;

	for (int cp = 0x800; cp < 0xFFFF; cp++) {
		n = sp_utf8_add_codepoint (&u, cp);
		mu_assert_int_eq (n, 3);
		mu_assert_int_eq (cp, sp_utf8_codepoint (u.buf, u.len));
		sp_utf8_reset (&u);
	}

	sp_utf8_final (&u);
}

static void
test_quad_bytes (void)
{
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n;

	for (int cp = 0x10000; cp < 0x10FFFF; cp++) {
		n = sp_utf8_add_codepoint (&u, cp);
		mu_assert_int_eq (n, 4);
		mu_assert_int_eq (cp, sp_utf8_codepoint (u.buf, u.len));
		sp_utf8_reset (&u);
	}

	sp_utf8_final (&u);
}

static void
test_invalid_codepoints (void)
{
	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n;

	n = sp_utf8_add_codepoint (&u, -1);
	mu_assert_int_eq (n, SP_UTF8_ECODEPOINT);

	n = sp_utf8_add_codepoint (&u, 0x110000);
	mu_assert_int_eq (n, SP_UTF8_ECODEPOINT);
	sp_utf8_final (&u);
}

static void
test_unescape (void)
{
	char in[] = "drop the \\\"bass\\\"\\n\\uD834\\uDD22";
	char cmp[] = "drop the \"bass\"\n\xF0\x9D\x84\xA2";

	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_decode (&u, in, sizeof (in) - 1, SP_UTF8_JSON);
	mu_assert_int_eq (n, (ssize_t)(sizeof (cmp) - 1));
	mu_assert_str_eq (u.buf, cmp);
	sp_utf8_final (&u);
}

static void
test_uri_decode (void)
{
	char in[] = "drop+the+%22bass%22%0A%F0%9D%84%A2";
	char cmp[] = "drop the \"bass\"\n\xF0\x9D\x84\xA2";

	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_decode (&u, in, sizeof (in) - 1, SP_UTF8_URI | SP_UTF8_SPACE_PLUS);
	mu_assert_int_eq (n, (ssize_t)(sizeof (cmp) - 1));
	mu_assert_str_eq (u.buf, cmp);
	sp_utf8_final (&u);
}

static void
test_uri_encode (void)
{
	char in[] = "drop the \"bass\"\n\xF0\x9D\x84\xA2";
	char cmp[] = "drop+the+%22bass%22%0A%F0%9D%84%A2";

	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_encode (&u, in, sizeof (in) - 1, SP_UTF8_URI | SP_UTF8_SPACE_PLUS);
	mu_assert_int_eq (n, (ssize_t)(sizeof (cmp) - 1));
	mu_assert_str_eq (u.buf, cmp);
	sp_utf8_final (&u);
}

static void
test_uri_encode_ascii (void)
{
	char in[] = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
	char cmp[] = "%20!%22#$%25&'()*+,-./0123456789:;%3C=%3E?@ABCDEFGHIJKLMNOPQRSTUVWXYZ%5B%5C%5D%5E_%60abcdefghijklmnopqrstuvwxyz%7B%7C%7D~";

	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_encode (&u, in, sizeof (in) - 1, SP_UTF8_URI);
	mu_assert_int_eq (n, (ssize_t)(sizeof (cmp) - 1));
	mu_assert_str_eq (u.buf, cmp);
	sp_utf8_final (&u);
}

static void
test_uri_encode_ascii_space_plus (void)
{
	char in[] = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
	char cmp[] = "+!%22#$%25&'()*+,-./0123456789:;%3C=%3E?@ABCDEFGHIJKLMNOPQRSTUVWXYZ%5B%5C%5D%5E_%60abcdefghijklmnopqrstuvwxyz%7B%7C%7D~";

	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_encode (&u, in, sizeof (in) - 1, SP_UTF8_URI | SP_UTF8_SPACE_PLUS);
	mu_assert_int_eq (n, (ssize_t)(sizeof (cmp) - 1));
	mu_assert_str_eq (u.buf, cmp);
	sp_utf8_final (&u);
}

static void
test_uri_encode_ascii_comp (void)
{
	char in[] = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
	char cmp[] = "%20!%22%23%24%25%26'()*%2B%2C-.%2F0123456789%3A%3B%3C%3D%3E%3F%40ABCDEFGHIJKLMNOPQRSTUVWXYZ%5B%5C%5D%5E_%60abcdefghijklmnopqrstuvwxyz%7B%7C%7D~";

	SpUtf8 u = SP_UTF8_MAKE ();
	ssize_t n = sp_utf8_encode (&u, in, sizeof (in) - 1, SP_UTF8_URI_COMPONENT);
	mu_assert_int_eq (n, (ssize_t)(sizeof (cmp) - 1));
	mu_assert_str_eq (u.buf, cmp);
	sp_utf8_final (&u);
}

int
main (void)
{
	mu_init ("utf8");

	test_err1 ();
	test_err2 ();
	test_err3 ();
	test_err4 ();
	test_err5 ();
	test_err6 ();
	test_err7 ();
	test_err8 ();
	test_err9 ();
	test_err10 ();
	test_err11 ();
	test_err12 ();
	test_err13 ();
	test_err14 ();
	test_err15 ();
	test_err16 ();

	test_single_bytes ();
	test_double_bytes ();
	test_triple_bytes ();
	test_quad_bytes ();

	test_invalid_codepoints ();

	test_unescape ();
	test_uri_decode ();
	test_uri_encode ();
	test_uri_encode_ascii ();
	test_uri_encode_ascii_space_plus ();
	test_uri_encode_ascii_comp ();

	mu_assert (sp_alloc_summary ());
}

