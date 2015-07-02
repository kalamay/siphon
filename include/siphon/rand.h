#ifndef SIPHON_HASH_H
#define SIPHON_HASH_H

#include "common.h"

SP_EXPORT int
sp_rand_init (void);

SP_EXPORT int
sp_rand (void *const restrict dst, size_t len);

SP_EXPORT int
sp_rand_uint32 (uint32_t bound, uint32_t *out);

SP_EXPORT int
sp_rand_double (double *out);

#endif

