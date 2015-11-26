#ifndef SIPHON_ERROR_H
#define SIPHON_ERROR_H

#include "common.h"

#include <errno.h>
#include <netdb.h>
#include <stdarg.h>

#define SP_EAI_OFFSET (-200)
#define SP_EUSER_MIN (-2000)

#define SP_ESYSTEM(n) (-(n))

#if EAI_FAIL < 0
# define SP_EAI_CODE(n) (SP_EAI_OFFSET + (n))
#else
# define SP_EAI_CODE(n) (SP_EAI_OFFSET - (n))
#endif

#define SP_UTF8_ESIZE       (-1001)
#define SP_UTF8_EESCAPE     (-1002)
#define SP_UTF8_ECODEPOINT  (-1003)
#define SP_UTF8_EENCODING   (-1004)
#define SP_UTF8_ESURROGATE  (-1005)
#define SP_UTF8_ETOOSHORT   (-1006)

#define SP_HTTP_ESYNTAX     (-1010)
#define SP_HTTP_ESIZE       (-1011)
#define SP_HTTP_ESTATE      (-1012)
#define SP_HTTP_ETOOSHORT   (-1013)

#define SP_JSON_ESYNTAX     (-1020)
#define SP_JSON_ESIZE       (-1021)
#define SP_JSON_ESTACK      (-1022)
#define SP_JSON_ESTATE      (-1023)
#define SP_JSON_EESCAPE     (-1024)
#define SP_JSON_EBYTE       (-1025)

#define SP_MSGPACK_ESYNTAX  (-1030)
#define SP_MSGPACK_ESTACK   (-1032)

#define SP_LINE_ESYNTAX     (-1040)
#define SP_LINE_ESIZE       (-1041)

#define SP_PATH_EBUFS       (-1051)
#define SP_PATH_ENOTFOUND   (-1052)
#define SP_PATH_ECLOSED     (-1053)

#define SP_URI_ESYNTAX      (-1060)
#define SP_URI_EBUFS        (-1061)
#define SP_URI_ESEGMENT     (-1062)
#define SP_URI_ERANGE       (-1063)

typedef struct {
	int code;
	char domain[10], name[20];
	char msg[1]; /* expanded when allocated */
} SpError;

SP_EXPORT const char *
sp_strerror (int code);

SP_EXPORT int
sp_eai_code (int err);

SP_EXPORT size_t
sp_error_string (int code, char *buf, size_t size);

SP_EXPORT void
sp_error_print (int code, FILE *out);

SP_EXPORT void
sp_exit (int code, int exitcode);

SP_EXPORT void
sp_abort (int code);

SP_EXPORT void __attribute__((__format__ (__printf__, 1, 2)))
sp_fabort (const char *fmt, ...);

SP_EXPORT const SpError *
sp_error (int code);

SP_EXPORT const SpError *
sp_error_next (const SpError *err);

SP_EXPORT const SpError *
sp_error_add (int code, const char *domain, const char *name, const char *msg);

SP_EXPORT const SpError *
sp_error_checkset (int code, const char *domain, const char *name, const char *msg);

SP_EXPORT size_t
sp_stack (char *buf, size_t len);

#endif

