#ifndef SIPHON_MAP_H
#define SIPHON_MAP_H

#include "common.h"
#include "type.h"
#include "hash.h"
#include "bloom.h"

typedef struct SpMap SpMap;
typedef struct SpMapEntry SpMapEntry;

struct SpMapEntry {
	uint64_t hash;
	void *value;
};

struct SpMap {
	const SpType *type;
	SpMapEntry *entries;
	size_t capacity, max, count, mask;
	double loadf;
	SpBloom *bloom;
};

#define SP_MAP_MAKE(typ) ((SpMap){ \
	.type = (typ),                 \
	.entries = NULL,               \
	.capacity = 0,                 \
	.max = 0,                      \
	.count = 0,                    \
	.mask = 0,                     \
	.loadf = 0.8,                  \
	.bloom = NULL                  \
})

SP_EXPORT int
sp_map_init (SpMap *self, size_t hint, double loadf, const SpType *type);

SP_EXPORT void
sp_map_final (SpMap *self);

SP_EXPORT void
sp_map_clear (SpMap *self);

SP_EXPORT size_t
sp_map_count (const SpMap *self);

SP_EXPORT size_t
sp_map_capacity (const SpMap *self);

SP_EXPORT double
sp_map_load (const SpMap *self);

SP_EXPORT double
sp_map_load_factor (const SpMap *self);

SP_EXPORT int
sp_map_set_load_factor (SpMap *self, size_t hint, double loadf);

SP_EXPORT uint64_t
sp_map_hash (const SpMap *self, const void *restrict key, size_t len);

SP_EXPORT int
sp_map_resize (SpMap *self, size_t hint);

SP_EXPORT int
sp_map_use_bloom (SpMap *self, size_t hint, double fpp);

SP_EXPORT bool
sp_map_has_key (const SpMap *self, const void *restrict key, size_t len);

SP_EXPORT void *
sp_map_get (const SpMap *self, const void *restrict key, size_t len);

SP_EXPORT int
sp_map_put (SpMap *self, const void *restrict key, size_t len, void *val);

SP_EXPORT bool
sp_map_del (SpMap *self, const void *restrict key, size_t len);

SP_EXPORT void **
sp_map_reserve (SpMap *self, const void *restrict key, size_t len, bool *isnew);

SP_EXPORT void
sp_map_assign (SpMap *self, void **reserve, void *val);

SP_EXPORT void *
sp_map_steal (SpMap *self, const void *restrict key, size_t len);

SP_EXPORT void
sp_map_print (const SpMap *self, FILE *out);

#define sp_map_each(self, entry)                                       \
	for (size_t sp_sym(i)=0; sp_sym(i)<(self)->capacity; sp_sym(i)++)  \
		if ((self)->entries[sp_sym(i)].hash &&                         \
			(entry = (const SpMapEntry *)&(self)->entries[sp_sym(i)])) \

#endif

