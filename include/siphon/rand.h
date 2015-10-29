#ifndef SIPHON_RAND_H
#define SIPHON_RAND_H

#include "common.h"

SP_EXPORT int
sp_rand (void *const restrict dst, size_t len);

SP_EXPORT int
sp_rand_uint32 (uint32_t bound, uint32_t *out);

SP_EXPORT int
sp_rand_uint64 (uint64_t bound, uint64_t *out);

SP_EXPORT int
sp_rand_double (double *out);

#endif

