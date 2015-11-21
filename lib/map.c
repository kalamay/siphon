#include "../include/siphon/map.h"
#include "../include/siphon/error.h"
#include "../include/siphon/alloc.h"

#include <unistd.h>
#include <assert.h>

/*
 * The SP_MAP_SPACE_OPTIMIZED definition will change the growth curve of the
 * hash table bucket array. With it undefined, the growth is always by a power
 * of two. With the value defined, the growth will flatten out once reaching a
 * multiple of 16 page bytes. The consequence of this is that the bucket
 * location must be found using a modulus instead of the faster bitwise and.
 */

#ifdef SP_MAP_SPACE_OPTIMIZED
static size_t max_unit;

static void __attribute__((constructor))
init (void)
{
	max_unit = getpagesize () * 16 / sizeof (struct SpMapEntry);
}
#endif

static inline size_t
start (const SpMap *self, uint64_t hash)
{
#ifdef SP_MAP_SPACE_OPTIMIZED
	return hash % self->capacity;
#else
	return hash & self->mask;
#endif
}

static inline size_t
probe (const SpMap *self, uint64_t hash, size_t idx)
{
	return start (self, idx + self->capacity - start (self, hash));
}

int
sp_map_init (SpMap *self, size_t hint, double loadf, const SpType *type)
{
	assert (self != NULL);
	assert (type != NULL);
	assert (type->hash != NULL);
	assert (type->iskey != NULL);

	*self = SP_MAP_MAKE (type);
	return sp_map_set_load_factor (self, hint, loadf);
}

void
sp_map_final (SpMap *self)
{
	assert (self != NULL);

	sp_bloom_free (self->bloom);
	self->bloom = NULL;

	sp_map_clear (self);
	sp_free (self->entries, self->capacity * sizeof *self->entries);
	*self = SP_MAP_MAKE (self->type);
}

void
sp_map_clear (SpMap *self)
{
	assert (self != NULL);

	if (self->type->free) {
		for (size_t i=0; i<self->capacity; i++) {
			if (self->entries[i].value != NULL) {
				self->type->free (self->entries[i].value);
			}
		}
	}
	memset (self->entries, 0, sizeof *self->entries * self->capacity);
	self->count = 0;
	sp_bloom_clear (self->bloom);
}

size_t
sp_map_count (const SpMap *self)
{
	assert (self != NULL);

	return self->count;
}

size_t
sp_map_capacity (const SpMap *self)
{
	assert (self != NULL);

	return self->capacity;
}

double
sp_map_load (const SpMap *self)
{
	assert (self != NULL);

	return (double)self->count / (double)self->capacity;
}

double
sp_map_load_factor (const SpMap *self)
{
	assert (self != NULL);

	return self->loadf;
}

int
sp_map_set_load_factor (SpMap *self, size_t hint, double loadf)
{
	assert (self != NULL);

	if (isnan (loadf) || loadf <= 0.0) {
		self->loadf = 0.8;
	}
	else if (loadf < 0.2) {
		self->loadf = 0.2;
	}
	else if (loadf > 0.95) {
		self->loadf = 0.95;
	}
	else {
		self->loadf = loadf;
	}
	if (hint == 0) {
		hint = self->count;
		if (hint == 0) {
			return 0;
		}
	}
	return sp_map_resize (self, hint);
}

uint64_t
sp_map_hash (const SpMap *self, const void *restrict key, size_t len)
{
	assert (self != NULL);
	assert (key != NULL);

	return self->type->hash (key, len, SP_SEED_RANDOM);
}

static size_t
capacity (size_t hint, double loadf)
{
	hint = (size_t)((double)hint / loadf);

	if (hint <= 16) {
		hint = 16;
	}
#ifdef SP_MAP_SPACE_OPTIMIZED
	else if (hint >= max_unit) {
		hint = (1 + ((hint - 1) / max_unit)) * max_unit;
	}
#endif
	else {
		hint = sp_power_of_2 (hint);
	}

	return hint;
}

int
sp_map_resize (SpMap *self, size_t hint)
{
	assert (self != NULL);

	hint = capacity (hint, self->loadf);

	// hint will overflow to 0 if too large for next power of two
	if (hint == 0) {
		errno = EINVAL;
		return -1;
	}

	if (hint == self->capacity) {
		return 0;
	}

	SpMapEntry *const entries = self->entries;
	const size_t capacity = self->capacity;

	self->entries = sp_calloc (hint, sizeof *self->entries);
	if (self->entries == NULL) {
		self->entries = entries;
		return -1;
	}

	self->capacity = hint;
	self->mask = hint - 1;
	self->max = hint * self->loadf;

	if (self->count == 0) {
		return 0;
	}

	for (size_t i = 0; i < capacity; i++) {
		if (entries[i].hash == 0) {
			continue;
		}
		SpMapEntry entry = entries[i];
		size_t idx = start (self, entry.hash);
		size_t dist, next;

		for (dist = 0; true; dist++, idx = start (self, idx+1)) {
			if (self->entries[idx].hash == 0) {
				self->entries[idx] = entry;
				break;
			}
			next = probe (self, self->entries[idx].hash, idx);
			if (next < dist) {
				SpMapEntry tmp = self->entries[idx];
				self->entries[idx] = entry;
				entry = tmp;
				dist = next;
			}
		}
	}

	sp_free (entries, sizeof *entries * capacity);

	return 0;
}

int
sp_map_use_bloom (SpMap *self, size_t hint, double fpp)
{
	assert (self != NULL);

	if (isnan (fpp) || fpp <= 0.0) {
		sp_bloom_free (self->bloom);
		self->bloom = NULL;
		return 0;
	}

	if (hint < self->count) {
		hint = self->count;
	}
	if (sp_bloom_is_capable (self->bloom, hint, fpp)) {
		return 0;
	}

	sp_bloom_free (self->bloom);
	self->bloom = sp_bloom_new (hint, fpp);
	if (self->bloom == NULL) {
		return -errno;
	}

	const SpMapEntry *entry;
	sp_map_each (self, entry) {
		sp_bloom_put_hash (self->bloom, entry->hash);
	}
	return 0;
}

static bool
definitely_no (const SpMap *self, uint64_t h)
{
	return self->count == 0 ||
		(self->bloom != NULL && !sp_bloom_maybe_hash (self->bloom, h));

}

static SpMapEntry *
get (const SpMap *self, uint64_t h, const void *restrict key, size_t len)
{
	if (definitely_no (self, h)) {
		return NULL;
	}

	size_t idx = start (self, h);
	size_t dist;

	for (dist = 0; true; dist++, idx = start (self, idx+1)) {
		uint64_t hash_tmp = self->entries[idx].hash;
		if (hash_tmp == 0 || probe (self, hash_tmp, idx) < dist) {
			return NULL;
		}
		if (h == hash_tmp) {
			if (sp_likely (self->type->iskey (self->entries[idx].value, key, len))) {
				return &self->entries[idx];
			}
		}
	}
}

bool
sp_map_has_key (const SpMap *self, const void *restrict key, size_t len)
{
	assert (self != NULL);
	assert (key != NULL);

	uint64_t h = sp_map_hash (self, key, len);
	return get (self, h, key, len) != NULL;
}

void *
sp_map_get (const SpMap *self, const void *restrict key, size_t len)
{
	assert (self != NULL);
	assert (key != NULL);

	uint64_t h = sp_map_hash (self, key, len);
	SpMapEntry *e = get (self, h, key, len);
	return e ? e->value : NULL;
}

int
sp_map_put (SpMap *self, const void *restrict key, size_t len, void *val)
{
	assert (self != NULL);
	assert (key != NULL);

	if (val == NULL) {
		return sp_map_del (self, key, len);
	}

	bool new;
	void **pos = sp_map_reserve (self, key, len, &new);
	if (pos == NULL) {
		return -errno;
	}
	if (!new && self->type->free) {
		self->type->free (*pos);
	}
	*pos = self->type->copy ? self->type->copy (val) : val;
	return !new;
}

bool
sp_map_del (SpMap *self, const void *restrict key, size_t len)
{
	void *value = sp_map_steal (self, key, len);
	if (value == NULL) {
		return false;
	}
	if (self->type->free) {
		self->type->free (value);
	}
	return true;
}

void **
sp_map_reserve (SpMap *self, const void *restrict key, size_t len, bool *isnew)
{
	assert (self != NULL);
	assert (key != NULL);
	assert (isnew != NULL);

	if (self->count == self->max) {
		if (sp_map_resize (self, self->capacity + 1) < 0) {
			return NULL;
		}
	}

	uint64_t h = sp_map_hash (self, key, len);
	SpMapEntry entry = { h, NULL };
	size_t idx = start (self, entry.hash);
	size_t dist, next;
	void **result = NULL;

	for (dist = 0; true; dist++, idx = start (self, idx+1)) {
		SpMapEntry tmp = self->entries[idx];
		if (tmp.hash == 0) {
			self->entries[idx] = entry;
			if (!result) {
				result = &self->entries[idx].value;
			}
			*isnew = true;
			self->count++;
			break;
		}
		if (entry.hash == tmp.hash) {
			if (self->type->iskey (tmp.value, key, len)) {
				result = &self->entries[idx].value;
				*isnew = false;
				break;
			}
		}
		next = probe (self, tmp.hash, idx);
		if (next < dist) {
			tmp.value = self->entries[idx].value;
			if (!result) {
				result = &self->entries[idx].value;
			}
			self->entries[idx] = entry;
			entry = tmp;
			dist = next;
		}
	}

	if (sp_likely (result != NULL)) {
		sp_bloom_put_hash (self->bloom, h);
	}

	return result;
}

void *
sp_map_steal (SpMap *self, const void *restrict key, size_t len)
{
	assert (self != NULL);
	assert (key != NULL);

	uint64_t h = sp_map_hash (self, key, len);

	if (definitely_no (self, h)) {
		return NULL;
	}

	SpMapEntry entry = { h, NULL };
	size_t idx = start (self, entry.hash), prev = 0;
	size_t dist;
	void *value = NULL;
	bool shift = false;

	for (dist = 0; true; dist++, idx = start (self, idx+1)) {
		SpMapEntry tmp = self->entries[idx];
		if (tmp.hash == 0) {
			break;
		}
		if (shift) {
			if (probe (self, tmp.hash, idx) == 0) {
				break;
			}
			self->entries[prev] = tmp;
			self->entries[idx].hash = 0;
			prev = idx;
		}
		else if (entry.hash == tmp.hash) {
			if (sp_likely (self->type->iskey (tmp.value, key, len))) {
				value = tmp.value;
				self->entries[idx].hash = 0;
				shift = true;
				prev = idx;
				self->count--;
			}
		}
	}

	return value;
}

void
sp_map_print (const SpMap *self, FILE *out)
{
	if (out == NULL) {
		out = stderr;
	}

	SpPrint print = self->type->print;
	if (print == NULL) {
		print = sp_print_ptr;
	}

	if (self == NULL) {
		fprintf (out, "#<SpMap:(null)>\n");
	}
	else {
		flockfile (out);
		fprintf (out, "#<SpMap:%p count=%zu, capacity=%zu> {\n", (void *)self, self->count, self->capacity);
		for (size_t i=0; i<self->capacity; i++) {
			if (self->entries[i].hash) {
				fprintf (out, "    %3zu %016" PRIx64 ": ", i, self->entries[i].hash);
				print (self->entries[i].value, out);
				fprintf (out, "\n");
			}
			else {
				fprintf (out, "    %3zu 0000000000000000: (null)\n", i);
			}
		}
		fprintf (out, "}\n");
		funlockfile (out);
	}
}

