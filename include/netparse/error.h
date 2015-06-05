#ifndef NETPARSE_ERROR_H
#define NETPARSE_ERROR_H

#define NP_ESYSTEM -1 /* system error */
#define NP_ESTATE  -2 /* parser state is invalid */
#define NP_ESYNTAX -3 /* protocol error */
#define NP_ESIZE   -4 /* size of value exceeded maximum allowed */
#define NP_ESTACK  -5 /* stack size exceeded */

extern const char *
np_strerror (int code);

#endif

