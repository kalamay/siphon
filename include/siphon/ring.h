#ifndef SIPHON_RING_H
#define SIPHON_RING_H

#include "common.h"
#include "map.h"
#include "vec.h"

typedef struct SpRing SpRing;

typedef struct {
	SpRing *ring;
	size_t keylen;
	int avail;
	uint8_t key[1];
} SpRingNode;

typedef struct {
	uint64_t hash;
	SpRingNode *node;
} SpRingReplica;

struct SpRing {
	SpMap nodes;
	SpRingReplica *replicas;
	SpHash hash;
};

SP_EXPORT int
sp_ring_init (SpRing *self, SpHash fn);

SP_EXPORT void
sp_ring_final (SpRing *self);

SP_EXPORT int
sp_ring_put (SpRing *self,
		const void *restrict key, size_t len,
		unsigned replicas, int avail);

SP_EXPORT const SpRingNode *
sp_ring_get (const SpRing *self, const void *restrict key, size_t len);

SP_EXPORT bool
sp_ring_del (SpRing *self, const void *restrict key, size_t len);

SP_EXPORT const SpRingReplica *
sp_ring_find (const SpRing *self, const void *restrict val, size_t len);

SP_EXPORT const SpRingReplica *
sp_ring_next (const SpRing *self, const SpRingReplica *rep);

SP_EXPORT const SpRingNode *
sp_ring_reserve (const SpRing *self, const SpRingReplica *rep);

SP_EXPORT void
sp_ring_restore (const SpRing *self, const SpRingNode *node);

SP_EXPORT void
sp_ring_print (const SpRing *self, FILE *out);

#endif

