#ifndef SIPHON_HASH_MAP_H
#define SIPHON_HASH_MAP_H

#include "tier.h"

#include <math.h> 

#define SP_HMAP(TEnt, ntiers, pref)                                            \
	SP_HMAP_NAMED (, TEnt, ntiers, pref)

#define SP_HMAP_NAMED(name, TEnt, ntiers, pref)                                \
	struct name {                                                              \
		SP_HTIER_NAMED (pref##_tier, TEnt) *tiers[ntiers];                     \
		double loadf;                                                          \
		size_t count;                                                          \
		size_t max;                                                            \
	}

#define SP_HMAP_EACH(map, entp)                                                \
	for (size_t sp_sym (t) = 0;                                                \
			sp_sym (t) < sp_len ((map)->tiers) && (map)->tiers[sp_sym (t)];    \
			sp_sym (t)++)                                                      \
		SP_HTIER_EACH ((map)->tiers[sp_sym (t)], entp)

/**
 * Generates extern function prototypes for the map
 *
 * @param  TMap  map structure type
 * @param  TKey  key type
 * @param  TEnt  entry type
 * @param  pref  function name prefix
 */
#define SP_HMAP_PROTOTYPE(TMap, TKey, TEnt, pref)                              \
	SP_HMAP_PROTOTYPE_INTERNAL (TMap, TEnt, pref, extern, TKey k, size_t kn)

/**
 * Generates static function prototypes for the map
 *
 * @param  TMap  map structure type
 * @param  TKey  key type
 * @param  TEnt  entry type
 * @param  pref  function name prefix
 */
#define SP_HMAP_PROTOTYPE_STATIC(TMap, TKey, TEnt, pref)                       \
	SP_HMAP_PROTOTYPE_INTERNAL (TMap, TEnt, pref, __attribute__((unused)) static, TKey k, size_t kn)

/**
 * Generates extern function prototypes for the map using a direct key
 *
 * The direct key variant uses an integer-like value as the key. This includes
 * integer types and pointers. Using the direct type does not require the hash
 * function to be defined.
 *
 * @param  TMap  map structure type
 * @param  TKey  key type
 * @param  TEnt  entry type
 * @param  pref  function name prefix
 */
#define SP_HMAP_DIRECT_PROTOTYPE(TMap, TKey, TEnt, pref)                       \
	SP_HMAP_PROTOTYPE_INTERNAL (TMap, TEnt, pref, extern, TKey k)

/**
 * Generates static function prototypes for the map using a direct key
 *
 * The direct key variant uses an integer-like value as the key. This includes
 * integer types and pointers. Using the direct type does not require the hash
 * function to be defined.
 *
 * @param  TMap  map structure type
 * @param  TKey  key type
 * @param  TEnt  entry type
 * @param  pref  function name prefix
 */
#define SP_HMAP_DIRECT_PROTOTYPE_STATIC(TMap, TKey, TEnt, pref)                \
	SP_HMAP_PROTOTYPE_INTERNAL (TMap, TEnt, pref, __attribute__((unused)) static, TKey k)

/**
 * Generates attributed function prototypes for the map
 *
 * @param  TMap     map structure type
 * @param  TEnt     entry type
 * @param  pref     function name prefix
 * @param  attr     attributes to apply to the function prototypes
 * @param  ...      type and name arguments for the key
 */
#define SP_HMAP_PROTOTYPE_INTERNAL(TMap, TEnt, pref, attr, ...)                \
	attr int                                                                   \
	pref##_init (TMap *map, double loadf, size_t hint);                        \
	attr void                                                                  \
	pref##_final (TMap *map);                                                  \
	attr int                                                                   \
	pref##_resize (TMap *map, size_t extra);                                   \
	attr size_t                                                                \
	pref##_condense (TMap *map, size_t limit);                                 \
	attr TEnt *                                                                \
	pref##_get (TMap *map, __VA_ARGS__);                                       \
	attr TEnt *                                                                \
	pref##_reserve (TMap *map, __VA_ARGS__, bool *isnew);                      \
	attr int                                                                   \
	pref##_put (TMap *map, __VA_ARGS__, TEnt *entry);                          \
	attr bool                                                                  \
	pref##_remove (TMap *map, __VA_ARGS__, TEnt *entry);                       \

#define SP_HMAP_GENERATE(TMap, TKey, TEnt, pref)                               \
	SP_HMAP_GENERATE_INTERNAL (TMap, TKey, TEnt, pref)                         \
                                                                               \
	TEnt *                                                                     \
	pref##_get (TMap *map, TKey k, size_t kn)                                  \
	{                                                                          \
		return pref##_hget (map, k, kn, pref##_hash (k, kn));                  \
	}                                                                          \
                                                                               \
	TEnt *                                                                     \
	pref##_reserve (TMap *map, TKey k, size_t kn, bool *isnew)                 \
	{                                                                          \
		return pref##_hreserve (map, k, kn, pref##_hash (k, kn), isnew);       \
	}                                                                          \
                                                                               \
	int                                                                        \
	pref##_put (TMap *map, TKey k, size_t kn, TEnt *entry)                     \
	{                                                                          \
		return pref##_hput (map, k, kn, pref##_hash (k, kn), entry);           \
	}                                                                          \
                                                                               \
	bool                                                                       \
	pref##_remove (TMap *map, TKey k, size_t kn, TEnt *entry)                  \
	{                                                                          \
		return pref##_hremove (map, k, kn, pref##_hash (k, kn), entry);        \
	}                                                                          \

#define SP_HMAP_DIRECT_GENERATE(TMap, TKey, TEnt, pref)                        \
	__attribute__((unused)) static uint64_t                                    \
	pref##_hash (TKey k)                                                       \
	{                                                                          \
		uint64_t x = ((uint64_t)k << 1) | 1;                                   \
		x ^= x >> 33;                                                          \
		x *= 0xff51afd7ed558ccdULL;                                            \
		x ^= x >> 33;                                                          \
		x *= 0xc4ceb9fe1a85ec53ULL;                                            \
		x ^= x >> 33;                                                          \
		return x;                                                              \
	}                                                                          \
                                                                               \
	SP_HMAP_GENERATE_INTERNAL (TMap, TKey, TEnt, pref)                         \
                                                                               \
	TEnt *                                                                     \
	pref##_get (TMap *map, TKey k)                                             \
	{                                                                          \
		return pref##_hget (map, k, 0, pref##_hash (k));                       \
	}                                                                          \
                                                                               \
	TEnt *                                                                     \
	pref##_reserve (TMap *map, TKey k, bool *isnew)                            \
	{                                                                          \
		return pref##_hreserve (map, k, 0, pref##_hash (k), isnew);            \
	}                                                                          \
                                                                               \
	int                                                                        \
	pref##_put (TMap *map, TKey k, TEnt *entry)                                \
	{                                                                          \
		return pref##_hput (map, k, 0, pref##_hash (k), entry);                \
	}                                                                          \
                                                                               \
	bool                                                                       \
	pref##_remove (TMap *map, TKey k, TEnt *entry)                             \
	{                                                                          \
		return pref##_hremove (map, k, 0, pref##_hash (k), entry);             \
	}                                                                          \

#define SP_HMAP_GENERATE_INTERNAL(TMap, TKey, TEnt, pref)                      \
	SP_HTIER_PROTOTYPE_STATIC (struct pref##_tier, TKey, pref##_tier)          \
	SP_HTIER_GENERATE (struct pref##_tier, TKey, pref##_tier, pref##_has_key)  \
                                                                               \
	int                                                                        \
	pref##_resize (TMap *map, size_t extra)                                    \
	{                                                                          \
		size_t m = map->count + extra;                                         \
		if (m <= map->max && (m < 8 || m >= map->max/2)) { return 0; }         \
		size_t size = ceil (m / map->loadf);                                   \
		struct pref##_tier *end = map->tiers[sp_len (map->tiers) - 1];         \
		for (size_t i = sp_len (map->tiers) - 1; i > 0; i--) {                 \
			map->tiers[i] = map->tiers[i-1];                                   \
		}                                                                      \
		map->tiers[0] = end;                                                   \
		int rc = pref##_tier_create (map->tiers, size);                        \
		if (rc < 0) {                                                          \
			for (size_t i = 1; i < sp_len (map->tiers); i++) {                 \
				map->tiers[i-1] = map->tiers[i];                               \
			}                                                                  \
			map->tiers[sp_len (map->tiers) - 1] = end;                         \
			return rc;                                                         \
		}                                                                      \
		map->max = map->tiers[0]->size * map->loadf;                           \
		return 1;                                                              \
	}                                                                          \
                                                                               \
	int                                                                        \
	pref##_init (TMap *map, double loadf, size_t hint)                         \
	{                                                                          \
		for (size_t i = 0; i < sp_len (map->tiers); i++) {                     \
			map->tiers[i] = NULL;                                              \
		}                                                                      \
		if (loadf <= 0.0 || isnan (loadf)) { map->loadf = 0.9; }               \
		else if (loadf > 0.99) { map->loadf = 0.99; }                          \
		else { map->loadf = loadf; }                                           \
		map->count = 0;                                                        \
		map->max = 0;                                                          \
		return pref##_resize (map, hint);                                      \
	}                                                                          \
                                                                               \
	void                                                                       \
	pref##_final (TMap *map)                                                   \
	{                                                                          \
		if (map == NULL) { return; }                                           \
		for (size_t i = 0; i < sp_len (map->tiers) && map->tiers[i]; i++) {    \
			free (map->tiers[i]);                                              \
			map->tiers[i] = NULL;                                              \
		}                                                                      \
		map->count = 0;                                                        \
		map->max = 0;                                                          \
	}                                                                          \
                                                                               \
	size_t                                                                     \
	pref##_condense (TMap *map, size_t limit)                                  \
	{                                                                          \
		if (sp_len (map->tiers) == 1) { return 0; }                            \
		size_t total = 0;                                                      \
		for (size_t i = sp_len (map->tiers) - 1; i > 0 && limit > 0; i--) {    \
			if (map->tiers[i] == NULL) { break; }                              \
			size_t n = pref##_tier_nremap                                      \
				(map->tiers[i], map->tiers[0], limit);                         \
			limit -= n;                                                        \
			total += n;                                                        \
			if (map->tiers[i]->count == 0) {                                   \
				free (map->tiers[i]);                                          \
				map->tiers[i] = NULL;                                          \
			}                                                                  \
		}                                                                      \
		return total;                                                          \
	}                                                                          \
                                                                               \
	__attribute__((unused)) static int                                         \
	pref##_prune_index (TMap *map, size_t tier, size_t idx)                    \
	{                                                                          \
		int rc = pref##_tier_remove (map->tiers[tier], idx);                   \
		if (rc == 0 && tier > 0 && map->tiers[tier]->count == 0) {             \
			free (map->tiers[tier]);                                           \
			map->tiers[tier] = NULL;                                           \
			for (tier++; tier < sp_len (map->tiers); tier++) {                 \
				map->tiers[tier-1] = map->tiers[tier];                         \
				map->tiers[tier] = NULL;                                       \
			}                                                                  \
		}                                                                      \
		return rc;                                                             \
	}                                                                          \
                                                                               \
	__attribute__((unused)) static TEnt *                                      \
	pref##_hget (TMap *map, TKey k, size_t kn, uint64_t h)                     \
	{                                                                          \
		assert (h > 0);                                                        \
		for (size_t i = 0; i < sp_len (map->tiers) && map->tiers[i]; i++) {    \
			if (map->tiers[i]->count == 0) { continue; }                       \
			ssize_t idx = pref##_tier_get (map->tiers[i], k, kn, h);           \
			if (idx >= 0) {                                                    \
				if (sp_len (map->tiers) > 1 && i > 0 &&                        \
						pref##_tier_load (map->tiers[0]) < map->loadf) {       \
					bool isnew;                                                \
					ssize_t res = pref##_tier_reserve                          \
						(map->tiers[0], k, kn, h, &isnew);                     \
					if (res >= 0) {                                            \
						map->tiers[0]->arr[res].entry =                        \
							map->tiers[i]->arr[idx].entry;                     \
						pref##_prune_index (map, i, idx);                      \
						i = 0;                                                 \
						idx = res;                                             \
					}                                                          \
				}                                                              \
				return &map->tiers[i]->arr[idx].entry;                         \
			}                                                                  \
		}                                                                      \
		return NULL;                                                           \
	}                                                                          \
                                                                               \
	__attribute__((unused)) static TEnt *                                      \
	pref##_hreserve (TMap *map, TKey k, size_t kn, uint64_t h, bool *isnew)    \
	{                                                                          \
		assert (h > 0);                                                        \
		assert (isnew != NULL);                                                \
		int rc = pref##_resize (map, 1);                                       \
		if (rc < 0) {                                                          \
			errno = -rc;                                                       \
			return NULL;                                                       \
		}                                                                      \
		size_t idx = pref##_tier_reserve (map->tiers[0], k, kn, h, isnew);     \
		if (*isnew) {                                                          \
			for (size_t i = 1; i < sp_len (map->tiers) && map->tiers[i]; i++) {\
				if (map->tiers[i] == NULL) { break; }                          \
				ssize_t sidx = pref##_tier_get (map->tiers[i], k, kn, h);      \
				if (sidx >= 0) {                                               \
					map->tiers[0]->arr[idx].entry                              \
						= map->tiers[i]->arr[sidx].entry;                      \
					pref##_prune_index (map, i, sidx);                         \
					isnew = false;                                             \
					goto out;                                                  \
				}                                                              \
			}                                                                  \
			map->count++;                                                      \
		}                                                                      \
	out:                                                                       \
		return &map->tiers[0]->arr[idx].entry;                                 \
	}                                                                          \
                                                                               \
	__attribute__((unused)) static int                                         \
	pref##_hput (TMap *map, TKey k, size_t kn, uint64_t h, TEnt *entry)        \
	{                                                                          \
		assert (h > 0);                                                        \
		assert (entry != NULL);                                                \
		bool isnew;                                                            \
		TEnt *e = pref##_hreserve (map, k, kn, h, &isnew);                     \
		if (e == NULL) { return -errno; }                                      \
		if (!isnew) {                                                          \
			TEnt tmp = *e;                                                     \
			*e = *entry;                                                       \
			*entry = tmp;                                                      \
			return 1;                                                          \
		}                                                                      \
		*e = *entry;                                                           \
		return 0;                                                              \
	}                                                                          \
                                                                               \
	__attribute__((unused)) static bool                                        \
	pref##_hremove (TMap *map, TKey k, size_t kn, uint64_t h, TEnt *entry)     \
	{                                                                          \
		assert (h > 0);                                                        \
		for (size_t i = 0; i < sp_len (map->tiers); i++) {                     \
			if (map->tiers[i] == NULL) { break; }                              \
			ssize_t idx = pref##_tier_get (map->tiers[i], k, kn, h);           \
			if (idx >= 0) {                                                    \
				if (entry != NULL) { *entry = map->tiers[i]->arr[idx].entry; } \
				if (pref##_prune_index (map, i, idx) == 0) { map->count--; }   \
				return true;                                                   \
			}                                                                  \
		}                                                                      \
		return false;                                                          \
	}                                                                          \

#endif

