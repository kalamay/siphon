#ifndef SIPHON_HASH_H
#define SIPHON_HASH_H

#include "common.h"

SP_EXPORT uint64_t
sp_metrohash64 (const void *key, uint64_t len, uint32_t seed);

#endif

