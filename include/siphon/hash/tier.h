#ifndef SIPHON_HASH_TIER_H
#define SIPHON_HASH_TIER_H

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>

#include <siphon/common.h>
#include <siphon/hash.h>

/**
 * @defgroup HTier Hash base functionality
 *
 * These macros implement the underlying robin hood search, insert, and
 * remove behavior on a single-allocation, fixed size tier. Higher-level
 * data structures manage resizing across tiers.
 *
 * @{
 */

/**
 * Swaps the values at two indices.
 *
 * This uses a buffer and memcpy calls. This will get optimized into
 * register assignments if the element size is appropriate.
 *
 * @param  arr  const: array reference
 * @param  a    const: first array index
 * @param  b    const: second array index
 */
#define SP_HTIER_SWAP(arr, a, b) do {                   \
	uint8_t sp_sym(tmp)[sizeof ((arr)[0])];                \
	memcpy (sp_sym(tmp), &(arr)[(a)], sizeof sp_sym(tmp)); \
	(arr)[(a)] = (arr)[(b)];                               \
	memcpy (&(arr)[(b)], sp_sym(tmp), sizeof sp_sym(tmp)); \
} while (0)

/**
 * Determines the starting index for a hash value
 *
 * @param  hash  hash value of the key
 * @param  mod   mod value, typically prime
 * @return  starting probe index
 */
#define SP_HTIER_START(hash, mod) \
	(ssize_t)((uint64_t)(hash) % (size_t)(mod))
/**
 * Wraps an index for a new probe position
 * 
 * @param  idx   index position to wrap
 * @param  mask  bit mask value (should be equal to `size-1`)
 * @return  index position within the array
 */
#define SP_HTIER_WRAP(idx, mask) \
	(ssize_t)((size_t)(idx) & (size_t)(mask))

/**
 * Probes an open addressed array for a valid index
 *
 * @param  idx   index position to wrap
 * @param  size  number of buckets in the array
 * @param  hash  hash value of the key
 * @param  mod   mod value, typically prime
 * @param  mask  bit mask value (should be equal to `size-1`)
 * @return  index position within the array
 */
#define SP_HTIER_STEP(idx, size, hash, mod, mask) \
	SP_HTIER_WRAP((idx) + (size) - SP_HTIER_START(hash, mod), mask)

/**
 * Declares a tier structure for a given entry type
 *
 * The declared struct is anonymous and must be `typedef`'s in order to use it.
 *
 * Example:
 *     struct Thing {
 *         int key;
 *         const char *value;
 *     };
 *     typedef SP_HTIER (struct Thing) ThingTier;
 *
 * @param  TEnt  entry type
 * @return  anonymous struct definition
 */
#define SP_HTIER(TEnt)                                                         \
	SP_HTIER_NAMED (,TEnt)

/**
 * Declares a tier structure for a given entry type
 *
 * Example:
 *     struct Thing {
 *         int key;
 *         const char *value;
 *     };
 *     SP_HTIER_NAMED (ThingTier, struct Thing);
 *     ...
 *     printf ("size: %zu\n", sizeof (struct ThingTier));
 *
 * @param  name  struct name
 * @param  TEnt  entry type
 * @return  struct definition
 */
#define SP_HTIER_NAMED(name, TEnt)                                             \
	struct name {                                                              \
		size_t size;                                                           \
		size_t mod;                                                            \
		size_t count;                                                          \
		size_t remap;                                                          \
		struct {                                                               \
			uint64_t h;                                                        \
			TEnt entry;                                                        \
		} arr[1];                                                              \
	}

#define SP_HTIER_EACH(tier, entp)                                              \
	for (ssize_t sp_sym (i) = (ssize_t)(tier)->size - 1;                       \
			sp_sym (i) >= 0;                                                   \
			sp_sym (i)--)                                                      \
		if ((tier)->arr[sp_sym (i)].h &&                                       \
				(entp = &(tier)->arr[sp_sym (i)].entry))                       \

/**
 * Generates extern function prototypes for the tier
 *
 * @param  TTier  tier structure type
 * @param  TKey   key type
 * @param  pref   function name prefix
 */
#define SP_HTIER_PROTOTYPE(TTier, TKey, pref)                                  \
	SP_HTIER_PROTOTYPE_INTERNAL (TTier, TKey, pref,)

/**
 * Generates static function prototypes for the tier
 *
 * @param  TTier  tier structure type
 * @param  TKey   key type
 * @param  pref   function name prefix
 */
#define	SP_HTIER_PROTOTYPE_STATIC(TTier, TKey, pref)                           \
	SP_HTIER_PROTOTYPE_INTERNAL (TTier, TKey, pref, __attribute__((unused)) static)

/**
 * Generates attributed function prototypes for the tier 
 *
 * @param  TTier  tier structure type
 * @param  TKey   key type
 * @param  pref   function name prefix
 * @param  attr   attributes to apply to the function prototypes
 */
#define SP_HTIER_PROTOTYPE_INTERNAL(TTier, TKey, pref, attr)                   \
	attr double                                                                \
	pref##_load (const TTier *tier);                                           \
	attr size_t                                                                \
	pref##_get (TTier *tier, TKey k, size_t kn, uint64_t h);                   \
	attr size_t                                                                \
	pref##_reserve (TTier *tier, TKey k, size_t kn, uint64_t h, bool *isnew);  \
	attr size_t                                                                \
	pref##_force (TTier *tier, const TTier *src, size_t idx);                  \
	attr int                                                                   \
	pref##_remove (TTier *tier, size_t idx);                                   \
	attr size_t                                                                \
	pref##_remap (TTier *tier, TTier *dst);                                    \
	attr size_t                                                                \
	pref##_nremap (TTier *tier, TTier *dst, size_t limit);                     \
	attr TTier *                                                               \
	pref##_alloc (size_t n);                                                   \
	attr int                                                                   \
	pref##_create (TTier **tierp, size_t n);                                   \

/**
 * Generates functions for the tier
 *
 * @param  TTier    tier structure type
 * @param  TKey     key type
 * @param  pref     function name prefix
 * @param  has_key  function to test a key against an entry
 */
#define SP_HTIER_GENERATE(TTier, TKey, pref, has_key)                          \
	double                                                                     \
	pref##_load (const TTier *tier)                                            \
	{                                                                          \
		return (double)tier->count / (double)tier->size;                       \
	}                                                                          \
                                                                               \
	size_t                                                                     \
	pref##_get (TTier *tier, TKey k, size_t kn, uint64_t h)                    \
	{                                                                          \
		const size_t size = tier->size;                                        \
		const size_t mod = tier->mod;                                          \
		const size_t mask = size - 1;                                          \
		ssize_t dist, i = SP_HTIER_START (h, mod);                             \
		for (dist = 0; 1; dist++, i = SP_HTIER_WRAP (i+1, mask)) {             \
			uint64_t eh = tier->arr[i].h;                                      \
			if (eh == 0 || SP_HTIER_STEP (i, size, eh, mod, mask) < dist) {    \
				return -ENOENT;                                                \
			}                                                                  \
			if (eh == h && has_key (&tier->arr[i].entry, k, kn)) {             \
				return i;                                                      \
			}                                                                  \
		}                                                                      \
	}                                                                          \
                                                                               \
	size_t                                                                     \
	pref##_reserve (TTier *tier, TKey k, size_t kn, uint64_t h, bool *isnew)   \
	{                                                                          \
		assert (tier->remap == tier->size);                                    \
		assert (isnew != NULL);                                                \
		const size_t size = tier->size;                                        \
		const size_t mod = tier->mod;                                          \
		const size_t mask = size - 1;                                          \
		ssize_t dist, rc = -1, i = SP_HTIER_START (h, mod);                    \
		for (dist = 0; 1; dist++, i = SP_HTIER_WRAP (i+1, mask)) {             \
			uint64_t eh = tier->arr[i].h;                                      \
			if (eh == 0 ||                                                     \
					(eh == h && has_key (&tier->arr[i].entry, k, kn))) {       \
				if (rc < 0) { rc = i; }                                        \
				else {                                                         \
					tier->arr[i] = tier->arr[rc];                              \
					memset (&tier->arr[rc], 0, sizeof tier->arr[rc]);          \
				}                                                              \
				tier->arr[rc].h = h;                                           \
				if (eh == 0) {                                                 \
					*isnew = true;                                             \
					tier->count++;                                             \
				}                                                              \
				else {                                                         \
					*isnew = false;                                            \
				}                                                              \
				return (size_t)rc;                                             \
			}                                                                  \
			ssize_t next = SP_HTIER_STEP (i, size, eh, mod, mask);             \
			if (next < dist) {                                                 \
				if (sp_likely (rc < 0)) { rc = i; }                            \
				else { SP_HTIER_SWAP (tier->arr, rc, i); }                     \
				dist = next;                                                   \
			}                                                                  \
		}                                                                      \
	}                                                                          \
                                                                               \
	size_t                                                                     \
	pref##_force (TTier *tier, const TTier *src, size_t idx)                   \
	{                                                                          \
		const size_t size = tier->size;                                        \
		const size_t mod = tier->mod;                                          \
		const size_t mask = size - 1;                                          \
		ssize_t dist, rc = -1, i = SP_HTIER_START (src->arr[idx].h, mod);      \
		for (dist = 0; 1; dist++, i = SP_HTIER_WRAP (i+1, mask)) {             \
			uint64_t eh = tier->arr[i].h;                                      \
			if (eh == 0) {                                                     \
				if (rc < 0) { rc = i; }                                        \
				else { tier->arr[i] = tier->arr[rc]; }                         \
				tier->arr[rc] = src->arr[idx];                                 \
				tier->count++;                                                 \
				return (size_t)rc;                                             \
			}                                                                  \
			ssize_t next = SP_HTIER_STEP (i, size, eh, mod, mask);             \
			if (next < dist) {                                                 \
				if (sp_likely (rc < 0)) { rc = i; }                            \
				else { SP_HTIER_SWAP (tier->arr, rc, i); }                     \
				dist = next;                                                   \
			}                                                                  \
		}                                                                      \
	}                                                                          \
                                                                               \
	int                                                                        \
	pref##_remove (TTier *tier, size_t idx)                                    \
	{                                                                          \
		if (idx >= tier->size) { return -ERANGE; }                             \
		if (tier->arr[idx].h == 0) { return -ENOENT; }                         \
		const size_t size = tier->size;                                        \
		const size_t mod = tier->mod;                                          \
		const size_t mask = size - 1;                                          \
		do {                                                                   \
			tier->arr[idx].h = 0;                                              \
			memset (&tier->arr[idx], 0, sizeof tier->arr[idx]);                \
			ssize_t p = idx;                                                   \
			idx = SP_HTIER_WRAP (idx+1, mask);                                 \
			uint64_t eh = tier->arr[idx].h;                                    \
			if (eh == 0 || SP_HTIER_STEP (idx, size, eh, mod, mask) == 0) {    \
				tier->count--;                                                 \
				return 0;                                                      \
			}                                                                  \
			tier->arr[p] = tier->arr[idx];                                     \
		} while (1);                                                           \
	}                                                                          \
                                                                               \
	size_t                                                                     \
	pref##_remap (TTier *tier, TTier *dst)                                     \
	{                                                                          \
		size_t n = 0;                                                          \
		for (size_t i = tier->remap; i > 0; i--) {                             \
			if (tier->arr[i-1].h != 0) {                                       \
				pref##_force (dst, tier, i-1);                                 \
				tier->arr[i-1].h = 0;                                          \
				n++;                                                           \
			}                                                                  \
		}                                                                      \
		tier->remap = 0;                                                       \
		tier->count = 0;                                                       \
		return n;                                                              \
	}                                                                          \
                                                                               \
	size_t                                                                     \
	pref##_nremap (TTier *tier, TTier *dst, size_t limit)                      \
	{                                                                          \
		size_t n = 0, i = tier->remap;                                         \
		for (; i > 0 && n < limit; i--) {                                      \
			if (tier->arr[i-1].h != 0) {                                       \
				pref##_force (dst, tier, i-1);                                 \
				if (sp_unlikely (i == tier->size)) {                           \
					pref##_remove (tier, i-1);                                 \
					i++;                                                       \
				}                                                              \
				else {                                                         \
					tier->arr[i-1].h = 0;                                      \
					tier->count--;                                             \
				}                                                              \
				n++;                                                           \
			}                                                                  \
		}                                                                      \
		tier->remap = i;                                                       \
		return n;                                                              \
	}                                                                          \
                                                                               \
	TTier *                                                                    \
	pref##_alloc (size_t n)                                                    \
	{                                                                          \
		TTier *tier = calloc (1,                                               \
				sizeof *tier + (sizeof tier->arr[0] * (n - 1)));               \
		if (tier != NULL) {                                                    \
			tier->size = n;                                                    \
			tier->mod = sp_power_of_2_prime (n);                               \
			tier->remap = n;                                                   \
		}                                                                      \
		return tier;                                                           \
	}                                                                          \
                                                                               \
	int                                                                        \
	pref##_create (TTier **tierp, size_t n)                                    \
	{                                                                          \
		TTier *tier = *tierp;                                                  \
		if (tier == NULL) {                                                    \
			n = n < 8 ? 8 : sp_power_of_2 (n);                                 \
			tier = pref##_alloc (n);                                           \
			if (tier == NULL) { return -errno; }                               \
			*tierp = tier;                                                     \
			return 1;                                                          \
		}                                                                      \
		if (n < tier->count) { n = tier->count; }                              \
		n = n < 8 ? 8 : sp_power_of_2 (n);                                     \
		if (n == tier->size) { return 0; }                                     \
		TTier *dst = pref##_alloc (n);                                         \
		if (dst == NULL) { return -errno; }                                    \
		pref##_remap (tier, dst);                                              \
		free (tier);                                                           \
		*tierp = dst;                                                          \
		return 1;                                                              \
	}                                                                          \
                                                                               \
/**@}*/

#endif

