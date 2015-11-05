#ifndef SIPHON_FMT_H
#define SIPHON_FMT_H

#include "common.h"

SP_EXPORT size_t
sp_fmt_str (FILE *out, const void *restrict str, size_t len, bool quote);

SP_EXPORT size_t
sp_fmt_char (FILE *out, int c);

#endif

