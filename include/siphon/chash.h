#ifndef SIPHON_CHASH_H
#define SIPHON_CHASH_H

#include "common.h"

typedef struct SpCHash SpCHash;
typedef struct SpCNode SpCNode;

SP_EXPORT SpCHash *
sp_chash_new (size_t size);

#endif

