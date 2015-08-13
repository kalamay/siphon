#ifndef SIPHON_ALLOC_H
#define SIPHON_ALLOC_H

/**
 * The goal of this allocator is to cause a program crash when invalid 
 * memory is accessed. It does this by allocating page-sized blocks of
 * memory and aligning the returned pointer at the end of the allocated
 * page(s). One additional page is allocated and protected so that the
 * program will crash if read from or written to. This is intended as a
 * debugging memory allocator only. The design wastes a LOT of memory,
 * but can be quite helpful; especially when combined with a fuzzer.
 *
 * The sp_mallocn function allows the returned pointer to be aligned to
 * an arbitraty alignment. This is particularly helpful when using byte
 * arrays or strings with an acceptable alignment of 1. When allocating
 * with sp_malloc, the alignment will be 16.
 *
 * Any value allocated with sp_mallocn, sp_malloc, sp_calloc, or sp_realloc
 * MUST only be freed with sp_free as free(3) is unable to directly free
 * these allocations.
 */

#include "common.h"

SP_EXPORT void *
sp_mallocn (size_t size, size_t align);

SP_EXPORT void
sp_free (void *p);

SP_EXPORT void *
sp_malloc (size_t size);

SP_EXPORT void *
sp_calloc (size_t n, size_t size);

SP_EXPORT void *
sp_realloc (void *p, size_t size);

#ifndef NDEBUG

#define mallocn sp_mallocn
#define malloc sp_malloc
#define free sp_free
#define calloc sp_calloc
#define realloc sp_realloc

#else

#include <stdlib.h>

#define mallocn(s,a) ({                        \
	void *buf;                                 \
	if (posix_memalign (&buf, (a), (s)) < 0) { \
		buf = NULL;                            \
	}                                          \
	buf;                                       \
})

#endif

#endif

