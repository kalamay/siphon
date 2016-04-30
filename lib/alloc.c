#include "../include/siphon/alloc.h"
#include "../include/siphon/error.h"
#include "../include/siphon/list.h"
#include "lock.h"
#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <errno.h>

static size_t pagesize = 0;

static void __attribute__((constructor(110)))
init (void)
{
	pagesize = getpagesize ();
}

// TODO: select proper alignment requirements for platform
#define DBG_ALIGN 1

#define DBG_PREAMBLE_SIZE 512

struct dbg_preamble {
	uint64_t magic;
	SpList entry;
	void *ptr;
	uintptr_t size, cap;
	char stack[DBG_PREAMBLE_SIZE - 8 - sizeof (SpList) - (3 * sizeof (uintptr_t))];
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

static SpList dbg_active;
static SpLock dbg_lock;

static int
dbg_protect (struct dbg_preamble *pre, int flags)
{
	if (pre == NULL) { return 0; };
	dbg_ensure_magic (pre);
	return mprotect ((char *)pre->ptr + dbg_size (pre->size), pagesize, flags);
}

static void
dbg_free (struct dbg_preamble *pre)
{
	dbg_protect (pre, PROT_READ | PROT_WRITE);
	pre->magic = 0;
	SP_LOCK (dbg_lock);
	sp_list_del (&pre->entry);
	SP_UNLOCK (dbg_lock);
	munmap (pre->ptr, pre->cap);
}

void *
sp_alloc_debug (void *ptr, size_t oldsz, size_t newsz, bool zero)
{
	if (
			sp_unlikely (pagesize == 0)
#if HAVE_VALGRIND_H
			|| RUNNING_ON_VALGRIND
#endif
	) {
		if (newsz == 0) {
			free (ptr);
			return NULL;
		}
		else {
			ptr = realloc (ptr, newsz);
			if (zero && ptr != NULL && oldsz < newsz) {
				memset ((char *)ptr+oldsz, 0, newsz-oldsz);
			}
			return ptr;
		}
	}

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

		SP_LOCK (dbg_lock);
		if (sp_unlikely (dbg_active.link[0] == NULL)) {
			sp_list_init (&dbg_active);
		}
		sp_list_add (&dbg_active, &pre->entry, SP_ASCENDING);
		SP_UNLOCK (dbg_lock);

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
		dbg_free (pre);
	}

	return ret;
}

bool
sp_alloc_summary (void)
{
	size_t leak = 0;
	SpList leaks, *entry;
	sp_list_init (&leaks);

	SP_LOCK (dbg_lock);
	if (dbg_active.link[0] != NULL) {
		sp_list_splice (&leaks, &dbg_active, SP_ASCENDING);
	}
	SP_UNLOCK (dbg_lock);

	sp_list_each (&leaks, entry, SP_ASCENDING) {
		struct dbg_preamble *pre = sp_container_of (entry, struct dbg_preamble, entry);
		fprintf (stderr,
			"*** leak of size %" PRIuPTR "\n%s",
			pre->size,
			pre->stack);
		leak += pre->size;
	}

	return leak == 0;
}

