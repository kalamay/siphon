#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

struct preamble {
	void *ptr;
	uintptr_t size;
	size_t align;
};

static size_t pagesize;

static void __attribute__((constructor))
init (void)
{
	pagesize = getpagesize ();
}

static struct preamble *
preamble (void *p)
{
	if (p == NULL) {
		return NULL;
	}
	return (struct preamble *)(
			(char *)p - (
				sizeof (struct preamble) +
				((uintptr_t)p % sizeof (struct preamble))));
}

static size_t
capacity (struct preamble *pre)
{
	return pre ?
		(((pre->size - 1) / pagesize) + 1) * pagesize - sizeof (*pre) :
		0;
}

static size_t
available (struct preamble *pre)
{
	return pre ?
		capacity (pre) - pre->size :
		0;
}

static int
protect (struct preamble *pre, int flags)
{
	if (pre == NULL) { return 0; };
	return mprotect ((char *)pre->ptr + (pre->size / pagesize) + pagesize, pagesize, flags);
}

void *
sp_mallocn (size_t size, size_t align)
{
	if (align == 0) {
		align = 16;
	}
	else if (align & (align - 1)) {
		errno = EINVAL;
		return NULL;
	}

	if (size == 0) {
		return malloc (0);
	}

	size_t alloc = ((((size + sizeof (struct preamble)) - 1) / pagesize) + 1) * pagesize;
	char *p, *ret;
	
	if (posix_memalign ((void **)&p, pagesize, alloc + pagesize) == -1) {
		return NULL;
	}

	size_t off = size;
	if (align > 1) {
		off = (((size - 1) / align) + 1) * align;
	}

	ret = p + (alloc - off);
	if ((uintptr_t)ret & (align - 1)) {
		fprintf (stderr, "incorrect alignment for returned pointer\n");
		abort ();
	}

	struct preamble *pre = preamble (ret);
	if (((char *)ret - (char *)pre) < (ssize_t)sizeof (struct preamble)) {
		fprintf (stderr, "incorrect space for preamble\n");
		abort ();
	}
	if ((uintptr_t)pre & (__alignof__ (pre) - 1)) {
		fprintf (stderr, "incorrect alignment for preamble\n");
		abort ();
	}

	pre->ptr = p;
	pre->size = size;

	protect (pre, PROT_NONE);

	return ret;
}

void
sp_free (void *p)
{
	if (p != NULL) {
		struct preamble *pre = preamble (p);
		protect (pre, PROT_READ | PROT_WRITE);
		free (pre->ptr);
	}
}

void *
sp_malloc (size_t size)
{
	return sp_mallocn (size, 16);
}

void *
sp_calloc (size_t n, size_t size)
{
	void *p = sp_malloc (n * size);
	if (p != NULL) {
		memset (p, 0, n * size);
	}
	return p;
}

void *
sp_realloc (void *p, size_t size)
{
	if (size == 0) {
		sp_free (p);
		return malloc (0);
	}

	struct preamble *pre = preamble (p);

	if (available (p) >= size) {
	}

	void *pn = sp_malloc (size);
	if (pn) {
		memcpy (pn, p, size > pre->size ? pre->size : size);
	}
	return pn;
}

