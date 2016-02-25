#include "../include/siphon/common.h"
#include "../include/siphon/fmt.h"

typedef enum {
	// what to encode
	CONTROL   = 0x0001, // encode control characters
	SQUOTE    = 0x0002, // encode single quotes
	DQUOTE    = 0x0004, // encode double quotes
	NONASCII  = 0x0008, // encode all non-ASCII characters
	URI       = 0x0010, // encode uri unsafe characters
	URI_COMP  = 0x0020, // encode uri component unsafe characters
	SPACEPLUS = 0x0040, // encode space as plus, decode plus as space
	EXTENDED  = 0x0060, // 
} EncodeType;

static uint8_t enc_tab[256] = {
	['\0']   = CONTROL   | URI | URI_COMP,
	['\x01'] = CONTROL   | URI | URI_COMP,
	['\x02'] = CONTROL   | URI | URI_COMP,
	['\x03'] = CONTROL   | URI | URI_COMP,
	['\x04'] = CONTROL   | URI | URI_COMP,
	['\x05'] = CONTROL   | URI | URI_COMP,
	['\x06'] = CONTROL   | URI | URI_COMP,
	['\x07'] = CONTROL   | URI | URI_COMP,
	['\b']   = CONTROL   | URI | URI_COMP,
	['\t']   = CONTROL   | URI | URI_COMP,
	['\n']   = CONTROL   | URI | URI_COMP,
	['\x0b'] = CONTROL   | URI | URI_COMP,
	['\f']   = CONTROL   | URI | URI_COMP,
	['\r']   = CONTROL   | URI | URI_COMP,
	['\x0e'] = CONTROL   | URI | URI_COMP,
	['\x0f'] = CONTROL   | URI | URI_COMP,
	['\x10'] = CONTROL   | URI | URI_COMP,
	['\x11'] = CONTROL   | URI | URI_COMP,
	['\x12'] = CONTROL   | URI | URI_COMP,
	['\x13'] = CONTROL   | URI | URI_COMP,
	['\x14'] = CONTROL   | URI | URI_COMP,
	['\x15'] = CONTROL   | URI | URI_COMP,
	['\x16'] = CONTROL   | URI | URI_COMP,
	['\x17'] = CONTROL   | URI | URI_COMP,
	['\x18'] = CONTROL   | URI | URI_COMP,
	['\x19'] = CONTROL   | URI | URI_COMP,
	['\x1a'] = CONTROL   | URI | URI_COMP,
	['\x1b'] = CONTROL   | URI | URI_COMP,
	['\x1c'] = CONTROL   | URI | URI_COMP,
	['\x1d'] = CONTROL   | URI | URI_COMP,
	['\x1e'] = CONTROL   | URI | URI_COMP,
	['\x1f'] = CONTROL   | URI | URI_COMP,
	[' ']    = SPACEPLUS | URI | URI_COMP,
	['"']    = DQUOTE    | URI | URI_COMP,
	['#']    =                   URI_COMP,
	['$']    =                   URI_COMP,
	['%']    =             URI | URI_COMP,
	['&']    =                   URI_COMP,
	['\'']   = SQUOTE                    ,
	['+']    = SPACEPLUS |       URI_COMP,
	[',']    =                   URI_COMP,
	['/']    =                   URI_COMP,
	[':']    =                   URI_COMP,
	[';']    =                   URI_COMP,
	['<']    =             URI | URI_COMP,
	['=']    =                   URI_COMP,
	['>']    =             URI | URI_COMP,
	['?']    =                   URI_COMP,
	['@']    =                   URI_COMP,
	['[']    =             URI | URI_COMP,
	['\\']   =             URI | URI_COMP,
	[']']    =             URI | URI_COMP,
	['^']    =             URI | URI_COMP,
	['`']    =             URI | URI_COMP,
	['{']    =             URI | URI_COMP,
	['|']    =             URI | URI_COMP,
	['}']    =             URI | URI_COMP,
	['\x7f'] = CONTROL   | URI | URI_COMP,
};

static void
dump (const char *name, EncodeType etype)
{
	printf ("static const uint8_t %s[] = ", name);
	uint8_t buf[64];
	size_t pos = 0;
	bool start = true;

	for (size_t i = 0x80; i < sp_len (enc_tab); i++) {
		enc_tab[i] = EXTENDED | URI | URI_COMP;
	}

	for (size_t i = 0; i < sp_len (enc_tab); i++) {
		if (start) {
			if (enc_tab[i] & etype) {
				// start a new character range for the same byte value
				buf[pos++] = i;
				buf[pos++] = i;
				start = false;
			}
		}
		else {
			if (enc_tab[i] & etype) {
				// push out the second byte value in the range
				buf[pos-1] = i;
			}
			else {
				// start a new range on the next match
				start = true;
			}
		}
	}

	sp_fmt_str (stdout, buf, pos, true);
	printf (";\n");
}

int
main (void)
{
	dump ("rng_uri", URI);
	dump ("rng_uri_comp", URI_COMP);
	dump ("rng_uri_space", URI | SPACEPLUS);
	dump ("rng_uri_space_comp", URI_COMP | SPACEPLUS);
	return 0;
}

