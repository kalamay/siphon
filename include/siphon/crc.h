#ifndef SIPHON_CRC_H
#define SIPHON_CRC_H

#include "common.h"

SP_EXPORT uint32_t
sp_crc32 (uint32_t crc, const void *bytes, size_t len);

SP_EXPORT uint32_t
sp_crc32c (uint32_t crc, const void *bytes, size_t len);

#endif

