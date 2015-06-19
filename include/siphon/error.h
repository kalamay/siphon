#ifndef SIPHON_ERROR_H
#define SIPHON_ERROR_H

#include "common.h"

#define SP_ESYSTEM -1 /* system error */
#define SP_ESTATE  -2 /* parser state is invalid */
#define SP_ESYNTAX -3 /* protocol error */
#define SP_ESIZE   -4 /* size of value exceeded maximum allowed */
#define SP_ESTACK  -5 /* stack size exceeded */

extern const char * SP_EXPORT
sp_strerror (int code);

#endif

