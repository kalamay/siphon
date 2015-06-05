#include "netparse/error.h"

#include <errno.h>
#include <string.h>

static const char *undefined = "undefined error";
static const char *str[] = {
	"parser state is invalid",
	"invalid syntax",
	"size of value exceeded maximum allowed",
	"stack size exceeded"
};

const char *
np_strerror (int code)
{
	if (code == -1) {
		return strerror (errno);
	}
	if (code > -1 || code < -(int)(sizeof (str) / sizeof (str[0]))) {
		return undefined;
	}
	return str[(-1 * code) - 2];
}

