#undef sp_malloc
#undef sp_calloc
#undef sp_realloc
#undef sp_free

#ifdef SP_ALLOC_DEBUG

#define sp_malloc(size) sp_alloc_debug (NULL, 0, (size), false)
#define sp_calloc(count, size) sp_alloc_debug (NULL, 0, (count)*(size), true)
#define sp_realloc(ptr, oldsz, newsz) sp_alloc_debug ((ptr), (oldsz), (newsz), false)
#define sp_free(ptr, size) sp_alloc_debug ((ptr), (size), 0, false)

#else

#include <stdlib.h>

#define sp_malloc(size) malloc (size)
#define sp_calloc(count, size) calloc ((count), ((size))
#define sp_realloc(ptr, oldsz, newsz) __extension__ ({ \
	(void)(oldsz);                                     \
	realloc ((ptr), (newsz));                          \
})
#define sp_free(ptr, size) do { \
	(void)(size);               \
	free ((ptr));               \
} while (0)

#endif

#ifndef SIPHON_ALLOC_H
#define SIPHON_ALLOC_H

#include "common.h"

/**
 * The goal of this allocator is to cause a program crash when invalid 
 * memory is accessed. It does this by allocating page-sized blocks of
 * memory and aligning the returned pointer at the end of the allocated
 * page(s). One additional page is allocated and protected so that the
 * program will crash if read from or written to. This is intended as a
 * debugging memory allocator only. The design wastes a LOT of memory,
 * but can be quite helpful; especially when combined with a fuzzer.
 *
 * Any value allocated with this function MUST also be freed with it.
 */
SP_EXPORT void *
sp_alloc_debug (void *ptr, size_t oldsz, size_t newsz, bool zero);

#endif

