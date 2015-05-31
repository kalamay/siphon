#include "netparse/error.h"

static const char *undefined = "undefined error";
static const char *str[] = {
	"parser state is invalid",
	"invalid syntax",
	"size of value exceeded maximum allowed",
};

const char *
np_strerror (int code)
{
	if (code > -1 || code < -(int)(sizeof (str) / sizeof (str[0]))) {
		return undefined;
	}
	return str[(-1 * code) - 1];
}

