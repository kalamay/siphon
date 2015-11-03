#include "../include/siphon/vec.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

typedef struct {
	uintptr_t count, capacity;
} SpVec;

static SpVec NIL = { 0, 0 };

#define POS(v, s, n) ((uint8_t *)(v+1)+s*n)
#define HEAD(v) POS(v, 0, 0)
#define TAIL(v, s) POS(v, s, v->count)

#define VEC(v) __extension__ ({ \
	assert (v != NULL);         \
	SpVec *handle = *v;         \
	handle ? handle - 1 : &NIL; \
})

#define VECRANGE(v, n) __extension__ ({            \
	assert (v != NULL);                            \
	if (n == 0) { return 0; }                      \
	SpVec *handle = *v;                            \
	/* check if NULL or hop back and test SpVec */ \
	if (handle == NULL || (--handle)->count < n) { \
		return -ERANGE;                            \
	}                                              \
	handle;                                        \
})

#define INDEX(i, len) do {       \
	if (i < 0) {                 \
		i += len;                \
		if (i < 0) {             \
			return -ERANGE;      \
		}                        \
	}                            \
	else if ((size_t)i >= len) { \
		return -ERANGE;          \
	}                            \
} while (0)

static inline SpVec *
resize (SpVec *v, void **vec, size_t size, size_t cap, size_t count)
{
	if (cap & ~(size_t)15) {
		SP_POWER_OF_2 (cap);
	}
	else {
		cap = 16;
	}

	v = malloc (sizeof *v + size*cap);
	if (v == NULL) { return NULL; }

	memcpy (HEAD (v), vec, size*count);
	v->count = count;
	v->capacity = cap;
	*vec = v + 1;

	return v;
}

static inline SpVec *
ensure (SpVec *v, void **vec, size_t size, size_t nitems)
{
	size_t count = v->count, cap = count + nitems;
	if (cap <= v->capacity) { return v; }
	return resize (v, vec, size, cap, count);
}

size_t
sp_vecp_count (void **vec)
{
	return VEC (vec)->count;
}

size_t
sp_vecp_capacity (void **vec)
{
	return VEC (vec)->capacity;
}

void
sp_vecp_clear (void **vec)
{
	SpVec *v = VEC (vec);
	v->count = 0;
}

void
sp_vecp_free (void **vec)
{
	SpVec *v = VEC (vec);
	if (v != &NIL) {
		free (v);
		*vec = NULL;
	}
}

int
sp_vecp_ensure (void **vec, size_t size, size_t nitems)
{
	if (nitems == 0) { return 0; }

	return ensure (VEC (vec), vec, size, nitems) ? 0 : -1;
}

int
sp_vecp_push (void **vec, size_t size, const void *el, size_t nitems)
{
	if (nitems == 0) { return 0; }

	SpVec *v = ensure (VEC (vec), vec, size, nitems);
	if (v == NULL) { return -1; }

	memcpy (TAIL (v, size), el, size*nitems);
	v->count += nitems;
	return 0;
}

int
sp_vecp_pop (void **vec, size_t size, void *el, size_t nitems)
{
	if (nitems == 0) { return 0; }

	SpVec *v = VECRANGE (vec, nitems);
	v->count -= nitems;
	memcpy (el, TAIL (v, size), size*nitems);
	return 0;
}

int
sp_vecp_shift (void **vec, size_t size, void *el, size_t nitems)
{
	if (nitems == 0) { return 0; }

	SpVec *v = VECRANGE (vec, nitems);
	v->count -= nitems;
	memcpy (el, HEAD (v), size*nitems);
	memmove (HEAD (v), POS (v, size, nitems), size*v->count);
	return 0;
}

int
sp_vecp_splice (void **vec, size_t size, ssize_t start, ssize_t end, void *el, size_t nitems)
{
	SpVec *v = VEC (vec);

	if (v->count == 0) {
		if (start != 0 || end != 0) {
			return -ERANGE;
		}
	}
	else {
		INDEX (start, v->count);
		INDEX (end, v->count+1);
		if (end < start) {
			return -ERANGE;
		}
	}

	ssize_t new = (ssize_t)v->count + nitems - (end - start);
	if ((ssize_t)v->capacity < new) {
		v = resize (v, vec, size, new, v->count);
		if (v == NULL) { return -1; }
	}

	uint8_t *p = HEAD (v);
	memmove (p + size*(start+nitems), p + size*end, size*(v->count-end));
	memcpy (p + size*start, el, size*nitems);
	v->count = new;
	return 0;
}

void
sp_vecp_reverse (void **vec, size_t size, void *tmp)
{
	SpVec *v = VEC (vec);
	size_t a = 0, b = v->count, n = b/2;
	for (; a < n; a++, b--) {
		memcpy (tmp, POS (vec, size, a), size);
		memcpy (POS (vec, size, a), POS (vec, size, b), size);
		memcpy (POS (vec, size, b), tmp, size);
	}
}

