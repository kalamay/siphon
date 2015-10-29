#include "siphon/alloc.h"
#include "siphon/error.h"
#include "lock.h"

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

static size_t pagesize;
static SpAlloc alloc_fn = sp_alloc_system;
static void *alloc_data = NULL;

static void __attribute__((constructor))
init (void)
{
	pagesize = getpagesize ();
}



void
sp_alloc_init (SpAlloc alloc, void *data)
{
	alloc_fn = alloc;
	alloc_data = data;
}

void *
sp_malloc (size_t newsz)
{
	return alloc_fn (alloc_data, NULL, 0, newsz);
}

void *
sp_realloc (void *p, size_t oldsz, size_t newsz)
{
	return alloc_fn (alloc_data, p, oldsz, newsz);
}

void
sp_free (void *p, size_t oldsz)
{
	(void)alloc_fn (p, p, oldsz, 0);
}



void *
sp_alloc_system (void *data, void *ptr, size_t oldsz, size_t newsz)
{
	(void)data;
	(void)oldsz;
	if (newsz == 0) {
		free (ptr);
		return NULL;
	}
	else {
		return realloc (ptr, newsz);
	}
}



// TODO: select proper alignment requirements for platform
#define DBG_ALIGN 1

#define DBG_PREAMBLE_SIZE 384

struct dbg_preamble {
	uint64_t magic;
	void *ptr;
	uintptr_t size, cap;
	char stack[DBG_PREAMBLE_SIZE - 8 - (3 * sizeof (uintptr_t))];
};

static const uint64_t dbg_magic = 0xf0231f251949aa2ULL;

static size_t
dbg_size (size_t size)
{
	return ((((size + DBG_PREAMBLE_SIZE) - 1) / pagesize) + 1) * pagesize;
}

static struct dbg_preamble *
dbg_get_preamble (void *p)
{
	if (p == NULL) {
		return NULL;
	}
	return (struct dbg_preamble *)(
			(char *)p - (
				DBG_PREAMBLE_SIZE +
				((uintptr_t)p % DBG_PREAMBLE_SIZE)));
}

#define dbg_ensure_magic(p) do {                \
	if ((p)->magic != dbg_magic) {              \
		fprintf (stderr,                        \
			"*** incorrect preamble identity\n" \
			"  1) allocation stack:\n%s",       \
			(p)->stack);                        \
		sp_fabort ("  2) current stack:");      \
	}                                           \
} while (0)

#define dbg_ensure_size(p, sz) do {                           \
	if ((p)->size != (sz)) {                                  \
		fprintf (stderr,                                      \
			"*** incorrect free size, %zu expected got %zu\n" \
			"  1) allocation stack:\n%s",                     \
				(p)->size, (sz), (p)->stack);                 \
		sp_fabort ("  2) current stack:");                    \
	}                                                         \
} while (0)

#if 0
static size_t
dbg_capacity (struct dbg_preamble *pre)
{
	dbg_ensure_magic (pre);
	return pre ?
		(((pre->size - 1) / pagesize) + 1) * pagesize - sizeof (*pre) :
		0;
}

static size_t
dbg_available (struct dbg_preamble *pre)
{
	dbg_ensure_magic (pre);
	return pre ?
		dbg_capacity (pre) - pre->size :
		0;
}
#endif

static int
dbg_protect (struct dbg_preamble *pre, int flags)
{
	if (pre == NULL) { return 0; };
	dbg_ensure_magic (pre);
	return mprotect ((char *)pre->ptr + dbg_size (pre->size), pagesize, flags);
}

void *
sp_alloc_debug (void *data, void *ptr, size_t oldsz, size_t newsz)
{
	(void)data;
	(void)oldsz;

	void *ret = NULL;

	if (newsz > 0) {
		size_t alloc = dbg_size (newsz);
		char *p = mmap (NULL, alloc+pagesize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
		if (p == MAP_FAILED) {
			return NULL;
		}

		size_t off = newsz;
#if DBG_ALIGN > 1
		off = (((newsz - 1) / DBG_ALIGN) + 1) * DBG_ALIGN;
#endif

		ret = p + (alloc - off);
#if DBG_ALIGN > 1
		if ((uintptr_t)ret & (DBG_ALIGN - 1)) {
			fprintf (stderr, "*** incorrect alignment for returned pointer\n");
			abort ();
		}
#endif

		struct dbg_preamble *pre = dbg_get_preamble (ret);
		if (((char *)ret - (char *)pre) < (ssize_t)DBG_PREAMBLE_SIZE) {
			fprintf (stderr, "*** incorrect space for dbg_preamble\n");
			abort ();
		}
		if ((uintptr_t)pre & (sizeof (pre) - 1)) {
			fprintf (stderr, "*** incorrect alignment for preamble\n");
			abort ();
		}

		pre->magic = dbg_magic;
		pre->ptr = p;
		pre->size = newsz;
		pre->cap = alloc + pagesize;

		sp_stack (pre->stack, sizeof pre->stack);

		dbg_protect (pre, PROT_NONE);
	}

	if (ptr != NULL) {
		struct dbg_preamble *pre = dbg_get_preamble (ptr);
		dbg_ensure_magic (pre);
		dbg_ensure_size (pre, oldsz);
		if (ret != NULL) {
			memcpy (ret, ptr, newsz > pre->size ? pre->size : newsz);
		}
		dbg_protect (pre, PROT_READ | PROT_WRITE);
		pre->magic = 0;
		munmap (pre->ptr, pre->cap);
	}

	return ret;
}

