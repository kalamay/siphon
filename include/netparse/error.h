#ifndef NETPARSE_ERROR_H
#define NETPARSE_ERROR_H

#define NP_ESTATE  -1 /* parser state is invalid */
#define NP_ESYNTAX -2 /* protocol error */
#define NP_ESIZE   -3 /* size of value exceeded maximum allowed */

extern const char *
np_strerror (int code);

#endif

