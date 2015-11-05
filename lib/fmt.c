#include "../include/siphon/fmt.h"

#include <ctype.h>
#include <inttypes.h>

#define WRITE(buf, len) do {                 \
	size_t step = fwrite (buf, len, 1, out); \
	if (step == 0) {                         \
		active = false;                      \
	}                                        \
	else {                                   \
		total += step;                       \
	}                                        \
} while (0)

size_t
sp_fmt_str (FILE *out, const void *restrict str, size_t len, bool quote)
{
	size_t total = 0;
	bool active = true;

	if (quote) {
		WRITE ("\"", 1);
	}
	for (size_t i = 0; active && i < len; i++) {
		uint8_t c = ((uint8_t *)str)[i];
		switch (c) {
			case '\0': WRITE ("\\0", 2); break;
			case '\a': WRITE ("\\a", 2); break;
			case '\b': WRITE ("\\b", 2); break;
			case '\f': WRITE ("\\f", 2); break;
			case '\n': WRITE ("\\n", 2); break;
			case '\r': WRITE ("\\r", 2); break;
			case '\t': WRITE ("\\t", 2); break;
			case '\v': WRITE ("\\v", 2); break;
			case '\\': WRITE ("\\", 1);  break;
			case '"':
				if (quote) {
					WRITE ("\\\"", 2); break;
					break;
				}
				// fallthrough
			default:
				if (isprint (c)) {
					WRITE (&c, 1);
				}
				else {
					char buf[8];
					int len = snprintf (buf, sizeof buf, "\\x%02" PRIx8, c);
					WRITE (buf, len);
				}
				break;
		}
	}
	if (quote) {
		WRITE ("\"", 1);
	}

	return total;
}

size_t
sp_fmt_char (FILE *out, int c)
{
	if (c < 0 || c > 255) {
		return 0;
	}
	int n = fprintf (out, (isgraph (c) ? "'%c'" : "x%02" PRIx8), (uint8_t)c);
	return n < 0 ? 0 : n;
}

