#ifndef SIPHON_ERROR_H
#define SIPHON_ERROR_H

#include "common.h"

#define SP_ESYSTEM    -1  /* system error */
#define SP_ESTATE     -2  /* parser state is invalid */
#define SP_ESYNTAX    -3  /* syntax error */
#define SP_ESIZE      -4  /* size of value exceeded maximum allowed */
#define SP_ESTACK     -5  /* stack size exceeded */
#define SP_EESCAPE    -6  /* invalid escape sequence */
#define SP_ECODEPOINT -7  /* invalid code point */
#define SP_EENCODING  -8  /* invalid encoding */
#define SP_ESURROGATE -9  /* invalid surrogate pair */
#define SP_ETOOSHORT  -10 /* input is too short */

extern const char * SP_EXPORT
sp_strerror (int code);

#endif

