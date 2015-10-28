#ifndef SIPHON_ALLOC_H
#define SIPHON_ALLOC_H

#include "common.h"

typedef void *(*SpAlloc)(void *data, void *ptr, size_t oldsz, size_t newsz);

/**
 * This maps to the system allocator or any allocator preloaded using
 * the system allocator interface.
 */
SP_EXPORT void *
sp_alloc_system (void *data, void *ptr, size_t oldsz, size_t newsz);

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
SP_EXPORT void *
sp_alloc_debug (void *data, void *ptr, size_t oldsz, size_t newsz);



SP_EXPORT void
sp_alloc_init (SpAlloc alloc, void *data);

SP_EXPORT void *
sp_malloc (size_t newsz);

SP_EXPORT void *
sp_realloc (void *p, size_t oldsz, size_t newsz);

SP_EXPORT void
sp_free (void *p, size_t oldsz);

#endif

