#include "../include/siphon/fmt.h"

#include <ctype.h>

#define PRINT(buf, len) do {                   \
	size_t step = fwrite ((buf), len, 1, out); \
	if (step == 0) {                           \
		return -1 - total;                     \
	}                                          \
	else {                                     \
		total += step;                         \
	}                                          \
} while (0)

#define PRINTF(...) do {                       \
	int step = fprintf (out, __VA_ARGS__);     \
	if (step < 0) {                            \
		return -1 - total;                     \
	}                                          \
	else {                                     \
		total += step;                         \
	}                                          \
} while (0)

ssize_t
sp_fmt_str (FILE *out, const void *restrict str, size_t len, bool quote)
{
	ssize_t total = 0;

	if (quote) {
		PRINT ("\"", 1);
	}
	for (size_t i = 0; i < len; i++) {
		uint8_t c = ((uint8_t *)str)[i];
		switch (c) {
			case '\0': PRINT ("\\0", 2); break;
			case '\a': PRINT ("\\a", 2); break;
			case '\b': PRINT ("\\b", 2); break;
			case '\f': PRINT ("\\f", 2); break;
			case '\n': PRINT ("\\n", 2); break;
			case '\r': PRINT ("\\r", 2); break;
			case '\t': PRINT ("\\t", 2); break;
			case '\v': PRINT ("\\v", 2); break;
			case '\\': PRINT ("\\\\", quote ? 2 : 1); break;
			case '"':
				if (quote) {
					PRINT ("\\\"", 2); break;
					break;
				}
				// fallthrough
			default:
				if (isprint (c)) {
					PRINT (&c, 1);
				}
				else {
					char buf[8];
					int len = snprintf (buf, sizeof buf, "\\x%02" PRIx8, c);
					PRINT (buf, len);
				}
				break;
		}
	}
	if (quote) {
		PRINT ("\"", 1);
	}

	return total;
}

ssize_t
sp_fmt_char (FILE *out, int c)
{
	if (c < 0 || c > 255) {
		return 0;
	}
	return fprintf (out, (isgraph (c) ? "'%c'" : "x%02" PRIx8), (uint8_t)c);
}

ssize_t
sp_fmt_bytes (FILE *out, const void *in, size_t len, uintptr_t addr)
{
	static const char space[] = "                                                ";

	ssize_t total = 0;

	const uint8_t *p = in, *m = in;
	bool first = true;
	uintptr_t rem = addr % 16;

	if (rem) {
		PRINTF ("%08" PRIXPTR ":%.*s ", addr-rem, (int)(3*rem), space);
		first = false;
	}

	while (len > 0) {
		if (addr % 16 == 0) {
			if (first) {
				first = false;
			}
			else {
				rem = (p - m) % 16;
				if (rem) {
					PRINT (space, rem);
				}
				for (; m < p; m++) {
					PRINT (isprint (*m) ? (const char *)m : ".", 1);
				}
				PRINT ("\n", 1);
			}
			PRINTF ("%08" PRIXPTR ": ", addr);
		}
		PRINTF ("%02x ", *p);
		p++;
		addr++;
		len--;
	}

	rem = addr % 16;
	if (rem) {
		PRINT (space, 3 * (16 - rem));
	}
	for (; m < p; m++) {
		PRINT (isprint (*m) ? (const char *)m : ".", 1);
	}
	PRINT ("\n", 1);

	return total;
}

